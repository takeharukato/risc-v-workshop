/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Timer operation                                                   */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>
#include <kern/spinlock.h>
#include <kern/page-if.h>
#include <kern/timer.h>

/* システム時刻 */
static system_timer g_walltime = __SYSTEM_TIMER_INITIALIZER;
static kmem_cache callout_ent_cache;  /* コールアウトエントリのキャッシュ */
/* コールアウトキュー */
static call_out_que g_callout_que = __CALLOUT_QUE_INITIALIZER(&g_callout_que);

/** 
    コールアウトエントリ比較関数
    @param[in] key 比較対象エントリ
    @param[in] ent キュー内の各エントリ
    @retval 正  keyの実行時刻がentより前にある
    @retval 負  keyの実行時刻がentより後にある
    @retval 0   keyの実行時刻がentに等しい
 */
static int
calloutent_cmp(call_out_ent *key, call_out_ent *ent) {

	if ( key->expire < ent->expire )
		return -1;

	if ( key->expire > ent->expire )
		return 1;

	return 0;
}

/**
   コールアウトを追加する
   @param[in] rel_expire_ms  タイマの相対起動時刻(単位: ms)
   @param[in] callout        コールアウト関数
   @param[in] private        コールアウト関数プライベート情報
   @retval    0              正常終了
   @retval   -ENOMEM         メモリ不足
   @note LO: コールアウトキューのロック, 時刻情報のロックの順で獲得
 */
int
tim_callout_add(tim_tmout rel_expire_ms, tim_callout_type callout, void *private){
	int                     rc;
	uptime_counter  abs_expire;
	call_out_ent          *cur;
	call_out_que        *coque;
	intrflags           iflags;

	coque = &g_callout_que;

	rc = slab_kmem_cache_alloc(&callout_ent_cache, KMALLOC_ATOMIC, (void **)&cur);
	if ( rc != 0 )
		goto error_out;  /* コールアウトエントリ獲得失敗 */

	/*  コールアウトキューのロックを獲得  */
	spinlock_lock_disable_intr(&coque->lock, &iflags);

	/*  時刻情報のロックを獲得  */
	spinlock_lock(&g_walltime.lock);

	 /* タイマ起動時刻をjiffies単位で算出 */
	abs_expire = g_walltime.uptime + MS_TO_JIFFIES(rel_expire_ms); 

	/*  時刻情報のロックを解放  */
	spinlock_unlock(&g_walltime.lock);

	/* コールアウトエントリを設定
	 */
	list_init(&cur->link);      /* キューへのリストエントリを初期化 */
	cur->callout = callout;     /* コールアウト関数を設定           */
	cur->private = private;     /* プライベート情報を設定           */
	cur->expire = abs_expire;   /* タイマ起動時刻を設定             */
	
	/* タイマ起動時刻をキーに昇順でキューに接続
	 */
	queue_add_sort(&coque->head, call_out_ent, link, &cur->link, 
	    calloutent_cmp, QUEUE_ADD_ASCENDING);

	/* コールアウトキューのロックを解放 */
	spinlock_unlock_restore_intr(&coque->lock, &iflags);

	return 0;

error_out:
	return rc;
}

/**
   コールアウトを呼び出す
   @note LO: コールアウトキューのロック, 時刻情報のロックの順で獲得
 */
static void
invoke_callout(void){
	call_out_ent          *cur;
	call_out_que        *coque;
	intrflags           iflags;

	coque = &g_callout_que;

	/*  コールアウトキューのロックを獲得  */
	spinlock_lock_disable_intr(&coque->lock, &iflags);

	/*  時刻情報のロックを獲得  */
	spinlock_lock(&g_walltime.lock);

	/* コールアウトキューが空でなければ先頭の要素の起動時刻と現在時刻を比較し, 
	 * 現在時刻がコールアウト起動時刻以降の時刻の場合はコールアウトを呼び出す
	 */
	while( !queue_is_empty(&coque->head) ) {

		/* コールアウトキューの先頭を参照 */
		cur = container_of(queue_ref_top(&coque->head), call_out_ent, link);

		/* 現在時刻を比較 */
		if ( cur->expire > g_walltime.uptime ) 
			break;  /* 呼び出し対象のコールアウトがない */

		/* コールアウトを取りだし */
		cur = container_of(queue_get_top(&coque->head), call_out_ent, link);

		/*  時刻情報のロックを解放  */
		spinlock_unlock(&g_walltime.lock);
		
		/* コールアウトキューのロックを解放 */
		spinlock_unlock_restore_intr(&coque->lock, &iflags);
		
		cur->callout(cur->private);  /* コールアウト呼び出し */
		
		/*  コールアウトキューのロックを獲得  */
		spinlock_lock_disable_intr(&coque->lock, &iflags);
		
		/*  時刻情報のロックを獲得  */
		spinlock_lock(&g_walltime.lock);
	}
	
	/*  時刻情報のロックを解放  */
	spinlock_unlock(&g_walltime.lock);

	/* コールアウトキューのロックを解放 */
	spinlock_unlock_restore_intr(&coque->lock, &iflags);
}
/**
   システム時刻を更新する
   @param[in] diff 時刻の加算値 (単位: timespec)
   @param[in] utimediff 時刻の加算値 (単位: ticks)
 */
void 
tim_update_walltime(ktimespec *diff, uptime_counter utimediff){
	ktimespec     ld;
	intrflags iflags;

	/*
	 * 引数をローカル変数に退避
	 */
	ld.tv_nsec = diff->tv_nsec;
	ld.tv_sec  = diff->tv_sec;

	/*  コールアウトキューのロックを獲得  */
	spinlock_lock_disable_intr(&g_walltime.lock, &iflags);

	g_walltime.curtime.tv_nsec += ld.tv_nsec;
	if ( ( g_walltime.curtime.tv_nsec / TIMER_NS_PER_SEC ) > 0 ) {

		g_walltime.curtime.tv_sec  += g_walltime.curtime.tv_nsec / TIMER_NS_PER_SEC;
		g_walltime.curtime.tv_nsec %= TIMER_NS_PER_SEC;
	}
	g_walltime.curtime.tv_sec += ld.tv_sec;

	g_walltime.uptime += utimediff;	

	/*  コールアウトキューのロックを解放  */
	spinlock_unlock_restore_intr(&g_walltime.lock, &iflags);

	invoke_callout();  /* コールアウト呼び出し */

#if defined(SHOW_WALLTIME)
	if ( ( g_walltime.uptime % 100 ) == 0 )
		kprintf("uptime: %qu sec: %qu nsec: %qu\n", 
			g_walltime.uptime, g_walltime.curtime.tv_sec, g_walltime.curtime.tv_nsec);
#endif  /*  SHOW_WALLTIME  */
}
/**
   コールアウト機構の初期化
 */
void
tim_callout_init(void) {
	int rc;

	/* コールアウトエントリキャッシュを初期化する
	 */
	rc = slab_kmem_cache_create(&callout_ent_cache, "call entry cache", 
	    sizeof(call_out_ent), SLAB_ALIGN_NONE, 0, KMALLOC_NORMAL, NULL, NULL);
	kassert( rc == 0 );

	return;
}
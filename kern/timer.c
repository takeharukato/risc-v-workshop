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

#include <hal/hal-traps.h>

/* システム時刻 */
static system_timer g_walltime = __SYSTEM_TIMER_INITIALIZER(&g_walltime);
static kmem_cache callout_ent_cache;  /* コールアウトエントリのキャッシュ */

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
   @param[in]  rel_expire_ms タイマの相対起動時刻(単位: ms)
   @param[in]  callout       コールアウト関数
   @param[in]  private       コールアウト関数プライベート情報
   @param[out] entp          登録したコールアウトエントリのアドレスを指し示すポインタのアドレス
   @retval    0              正常終了
   @retval   -ENOMEM         メモリ不足
 */
int
tim_callout_add(tim_tmout rel_expire_ms, tim_callout_type callout, void *private, 
		call_out_ent **entp){
	int                     rc;
	uptime_counter  abs_expire;
	call_out_ent          *cur;
	intrflags           iflags;

	rc = slab_kmem_cache_alloc(&callout_ent_cache, KMALLOC_ATOMIC, (void **)&cur);
	if ( rc != 0 )
		goto error_out;  /* コールアウトエントリ獲得失敗 */

	/*  時刻情報のロックを獲得  */
	spinlock_lock_disable_intr(&g_walltime.lock, &iflags);


	 /* タイマ起動時刻をjiffies単位で算出 */
	abs_expire = g_walltime.uptime + MS_TO_JIFFIES(rel_expire_ms); 

	/* コールアウトエントリを設定
	 */
	list_init(&cur->link);      /* キューへのリストエントリを初期化 */
	cur->callout = callout;     /* コールアウト関数を設定           */
	cur->private = private;     /* プライベート情報を設定           */
	cur->expire = abs_expire;   /* タイマ起動時刻を設定             */
	
	/* タイマ起動時刻をキーに昇順でキューに接続
	 */
	queue_add_sort(&g_walltime.head, call_out_ent, link, &cur->link, 
	    calloutent_cmp, QUEUE_ADD_ASCENDING);

	*entp = cur;  /* コールアウトエントリを返却 */

	/* 時刻情報のロックを解放 */
	spinlock_unlock_restore_intr(&g_walltime.lock, &iflags);

	return 0;

error_out:
	return rc;
}

/**
   コールアウトをキャンセルする
   @param[in] ent            キャンセルするコールアウトエントリ
   @retval    0              正常終了
   @retval   -ENOENT         指定されたエントリがない
 */
int
tim_callout_cancel(call_out_ent *ent){
	list            *lp, *next;
	call_out_ent          *cur;
	intrflags           iflags;

	/*  時刻情報のロックを獲得  */
	spinlock_lock_disable_intr(&g_walltime.lock, &iflags);

	/* コールアウトエントリを検索
	 */
	queue_for_each_safe(lp, &g_walltime.head, next)	{

		cur = container_of(lp, call_out_ent, link);
		if ( cur == ent ) { /* 見つかったエントリを外す */

			queue_del(&g_walltime.head, &cur->link);
			goto free_ent_out;  /* エントリの解放へ */
		}
	}

	/* 時刻情報のロックを解放 */
	spinlock_unlock_restore_intr(&g_walltime.lock, &iflags);

	return -ENOENT;  /* 指定されたエントリが見つからなかった */

free_ent_out:

	slab_kmem_cache_free((void *)cur);  /* コールアウトエントリを解放  */	

	/* 時刻情報のロックを解放 */
	spinlock_unlock_restore_intr(&g_walltime.lock, &iflags);

	return 0;
}

/**
   コールアウトを呼び出す
   @param[in] ctx       割込みコンテキスト
 */
static void
invoke_callout(trap_context *ctx){
	call_out_ent          *cur;
	intrflags           iflags;

	/*  時刻情報のロックを獲得  */
	spinlock_lock_disable_intr(&g_walltime.lock, &iflags);

	/* コールアウトキューが空でなければ先頭の要素の起動時刻と現在時刻を比較し, 
	 * 現在時刻がコールアウト起動時刻以降の時刻の場合はコールアウトを呼び出す
	 */
	while( !queue_is_empty(&g_walltime.head) ) {

		/* コールアウトキューの先頭を参照 */
		cur = container_of(queue_ref_top(&g_walltime.head), call_out_ent, link);

		/* 現在時刻を比較 */
		if ( cur->expire > g_walltime.uptime ) 
			break;  /* 呼び出し対象のコールアウトがない */

		/* コールアウトを取りだし */
		cur = container_of(queue_get_top(&g_walltime.head), call_out_ent, link);

		/*  時刻情報のロックを解放  */
		spinlock_unlock_restore_intr(&g_walltime.lock, &iflags);
		
		cur->callout(ctx, cur->private);  /* コールアウト呼び出し */
		
		/*  時刻情報のロックを獲得  */
		spinlock_lock_disable_intr(&g_walltime.lock, &iflags);
	}
	
	/*  時刻情報のロックを解放  */
	spinlock_unlock_restore_intr(&g_walltime.lock, &iflags);
}
/**
   システム時刻を更新する
   @param[in] ctx       割込みコンテキスト
   @param[in] diff      時刻の加算値 (単位: timespec)
   @param[in] utimediff 時刻の加算値 (単位: ticks)
 */
void 
tim_update_walltime(trap_context *ctx, ktimespec *diff, uptime_counter utimediff){
	ktimespec     ld;
	intrflags iflags;

	/*
	 * 引数をローカル変数に退避
	 */
	ld.tv_nsec = diff->tv_nsec;
	ld.tv_sec  = diff->tv_sec;

	/*  時刻情報のロックを獲得  */
	spinlock_lock_disable_intr(&g_walltime.lock, &iflags);

	g_walltime.curtime.tv_nsec += ld.tv_nsec;
	if ( ( g_walltime.curtime.tv_nsec / TIMER_NS_PER_SEC ) > 0 ) {

		g_walltime.curtime.tv_sec  += g_walltime.curtime.tv_nsec / TIMER_NS_PER_SEC;
		g_walltime.curtime.tv_nsec %= TIMER_NS_PER_SEC;
	}
	g_walltime.curtime.tv_sec += ld.tv_sec;

	g_walltime.uptime += utimediff;	

	/*  時刻情報のロックを解放  */
	spinlock_unlock_restore_intr(&g_walltime.lock, &iflags);

	invoke_callout(ctx);  /* コールアウト呼び出し */

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

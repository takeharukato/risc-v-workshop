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

#include <klib/epoch_time.h>

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

	if ( key->expire.tv_sec < ent->expire.tv_sec )
		return -1;

	if ( key->expire.tv_sec > ent->expire.tv_sec )
		return 1;

	if ( key->expire.tv_nsec < ent->expire.tv_nsec )
		return -1;

	if ( key->expire.tv_nsec > ent->expire.tv_nsec )
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
	epoch_time             sec;
	long                  nsec;
	call_out_ent          *cur;
	intrflags           iflags;

	rc = slab_kmem_cache_alloc(&callout_ent_cache, KMALLOC_ATOMIC, (void **)&cur);
	if ( rc != 0 )
		goto error_out;  /* コールアウトエントリ獲得失敗 */

	/*  時刻情報のロックを獲得  */
	spinlock_lock_disable_intr(&g_walltime.lock, &iflags);

	/* コールアウトエントリを設定
	 */
	list_init(&cur->link);      /* キューへのリストエントリを初期化 */
	cur->callout = callout;     /* コールアウト関数を設定           */
	cur->private = private;     /* プライベート情報を設定           */

	 /* タイマ起動時刻をtimespec単位で算出 */
	sec = rel_expire_ms / TIMER_MS_PER_SEC;
	nsec = (rel_expire_ms % TIMER_MS_PER_SEC) * TIMER_US_PER_MS * TIMER_NS_PER_US;

	cur->expire.tv_sec = g_walltime.curtime.tv_sec + sec;
	if ( g_walltime.curtime.tv_nsec + nsec >= TIMER_NS_PER_SEC ) {

		++cur->expire.tv_sec;
		nsec %= TIMER_NS_PER_SEC;
	}
	cur->expire.tv_nsec = g_walltime.curtime.tv_nsec + nsec;

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
		if ( cur->expire.tv_sec > g_walltime.curtime.tv_sec )
			break;  /* 呼び出し対象のコールアウトがない */
		if ( ( cur->expire.tv_sec == g_walltime.curtime.tv_sec )
		    && ( cur->expire.tv_nsec > g_walltime.curtime.tv_nsec ) )
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
   コールアウト機構の初期化
 */
static void
init_callout(void) {
	int rc;

	/* コールアウトエントリキャッシュを初期化する
	 */
	rc = slab_kmem_cache_create(&callout_ent_cache, "call entry cache",
	    sizeof(call_out_ent), SLAB_ALIGN_NONE, 0, KMALLOC_NORMAL, NULL, NULL);
	kassert( rc == 0 );

	return;
}

/**
   システム時刻を更新する
   @param[in] ctx       割込みコンテキスト
   @param[in] diff      時刻の加算値 (単位: timespec)
 */
void
tim_update_walltime(trap_context *ctx, ktimespec *diff){
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

	/*  時刻情報のロックを解放  */
	spinlock_unlock_restore_intr(&g_walltime.lock, &iflags);

	invoke_callout(ctx);  /* コールアウト呼び出し */

#if defined(SHOW_WALLTIME)
	if ( ( g_walltime.curtime.tv_sec % 5 ) == 0 )
		kprintf("sec: %qu nsec: %qu\n",
			g_walltime.curtime.tv_sec, g_walltime.curtime.tv_nsec);
#endif  /*  SHOW_WALLTIME  */
}

/**
   現在のシステム時刻を取得する
   @param[out] ktsp 時刻返却領域
 */
void
tim_walltime_get(ktimespec *ktsp){
	intrflags iflags;

	if ( ktsp == NULL )
		return; /* 何もせず抜ける */

	/*
	 * システム時刻を取得する
	 */
	/*  時刻情報のロックを獲得  */
	spinlock_lock_disable_intr(&g_walltime.lock, &iflags);

	ktsp->tv_sec = g_walltime.curtime.tv_sec;    /* 秒を取得する     */
	ktsp->tv_nsec = g_walltime.curtime.tv_nsec;  /* ナノ秒を取得する */

	/*  時刻情報のロックを解放  */
	spinlock_unlock_restore_intr(&g_walltime.lock, &iflags);
}

/**
   タイマ管理機構を初期化する
 */
void
tim_timer_init(void){
	int           rc;
	ktimespec     ld;
	kernel_tm     tm;
	intrflags iflags;

	rc = hal_read_rtc(&ld);  /* RTCから現在時刻を得る  */
	if ( rc == 0 ) {  /* RTCから時刻を得られた場合は, システム時刻を更新する */

		/* エポック時刻をtm構造体に変換する */
		clock_epoch_time_to_tm(ld.tv_sec, &tm);

		/*
		 * システム時刻を更新する
		 */
		/*  時刻情報のロックを獲得  */
		spinlock_lock_disable_intr(&g_walltime.lock, &iflags);

		g_walltime.curtime.tv_sec = ld.tv_sec;    /* 秒を更新する */
		g_walltime.curtime.tv_nsec = ld.tv_nsec;  /* ナノ秒を更新する */

		/*  時刻情報のロックを解放  */
		spinlock_unlock_restore_intr(&g_walltime.lock, &iflags);
	}

	init_callout();  /* コールアウト機構を初期化する */

	/* 時刻を表示する */
	kprintf("Current wall time: %04d/%02d/%02d %02d:%02d:%02d\n",
	    1900 + tm.tm_year, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
}

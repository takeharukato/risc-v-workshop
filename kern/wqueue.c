/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  wait_queue                                                        */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>
#include <kern/spinlock.h>
#include <kern/mutex.h>
#include <kern/wqueue.h>
#include <kern/thr-if.h>
#include <kern/sched-if.h>

/**
   ウエイトキューにウエイトキューエントリを追加する
   @param[in] wque 操作対象のウエイトキュー
   @param[in] ent  追加するウエイトキューエントリ
   @retval 起床要因
 */
static void
enque_wque_entry(wque_waitqueue *wque, wque_entry *ent){
	intrflags iflags;

	kassert(list_not_linked(&ent->thr->link));  
	spinlock_lock_disable_intr(&wque->lock, &iflags);   /* ウエイトキューをロック     */

	ent->reason = WQUE_WAIT;            /* 待ち中に遷移する */
	queue_add(&wque->que, &ent->link);  /* キューに追加     */

	spinlock_unlock_restore_intr(&wque->lock, &iflags); /* ウエイトキューをアンロック */
}

/**
   ウエイトキューの初期化
   @param[in] wque 操作対象のウエイトキュー
 */
void
wque_init_wait_queue(wque_waitqueue *wque){

	spinlock_init(&wque->lock);         /* ロックの初期化             */
	queue_init(&wque->que);             /* キューの初期化             */
	queue_init(&wque->prio_que);        /* 優先度継承キューの初期化   */
	wque->wqflag = WQUE_WAKEFLAG_ALL;   /* 起床方法の初期化           */
}

/**
   ウエイトキューが空であることを確認する
   @param[in] wque 操作対象のウエイトキュー
   @retval  真 ウエイトキューが空である
   @retval  偽 ウエイトキューが空でない
 */
bool
wque_is_empty(wque_waitqueue *wque){
	bool         res;
	intrflags iflags;

	spinlock_lock_disable_intr(&wque->lock, &iflags); /* ウエイトキューをロック */
	res = queue_is_empty(&wque->que); /* キューの状態を確認 */
	spinlock_unlock_restore_intr(&wque->lock, &iflags); /* ウエイトキューをアンロック */
	
	return res;
}

/**
   ウエイトエントリを初期化する
   @param[in] ent  操作対象のウエイトキューエントリ
   @note 時間待ち合わせ処理中から使用するためのIF関数
 */
void
wque_init_wque_entry(wque_entry *ent){
	bool         res;
	thread      *cur;

	cur = ti_get_current_thread();  /* 自スレッドの管理情報を取得     */
	res = thr_ref_inc(cur);         /* 自スレッドの参照を取得         */
	kassert( res );                 /* 自スレッドは終了していないはず */

	list_init(&ent->link);               /* キューへのリンクを初期化する           */
	ent->reason = WQUE_WAIT;             /* 待ち中に初期化する                     */
	ent->thr = ti_get_current_thread();  /* 自スレッドを休眠させるように初期化する */
	ent->thr->state = THR_TSTATE_WAIT;   /* 状態を更新                             */

	res = thr_ref_dec(cur);         /* 自スレッドの参照を解放         */
	kassert( !res );                /* 自スレッドは終了していないはず */
}

/**
   自スレッドのウエイトキューで待ち合わせる
   @param[in] wque 操作対象のウエイトキュー
   @retval 起床要因
   @note 自スレッドのウエイトキューで待ち合わせるため自スレッドのロックをとって呼び出すこと
 */
wque_reason
wque_wait_for_curthr(wque_waitqueue *wque){
	bool         res;
	thread      *cur;
	wque_entry   ent;

	cur = ti_get_current_thread();  /* 自スレッドの管理情報を取得 */

	res = thr_ref_inc(cur);         /* 自スレッドの参照を取得     */
	kassert( res );                 /* 自スレッドは終了していないはず */

	kassert( spinlock_locked_by_self( &cur->lock ) );

	wque_init_wque_entry(&ent);     /* ウエイトキューエントリを初期化する */
	enque_wque_entry(wque, &ent);   /* ウエイトキューエントリをウエイトキューに追加する */

	spinlock_unlock(&cur->lock);    /* スピンロックを解放する */

	res = thr_ref_dec(cur);         /* 自スレッドの参照を解放     */
	kassert( !res );                /* 自スレッドは終了していないはず */

	sched_schedule(); 	        /* スレッド休眠に伴う再スケジュール */

	res = thr_ref_inc(cur);         /* 自スレッドの参照を取得     */
	kassert( res );                 /* 自スレッドは終了していないはず */

	spinlock_lock(&cur->lock);      /* スピンロックを獲得する */

	res = thr_ref_dec(cur);         /* 自スレッドの参照を解放     */
	kassert( !res );                /* 自スレッドは終了していないはず */

	return ent.reason;              /* 起床要因を返却する */
}

/**
   スピンロックで排他している資源を待ち合わせる
   @param[in] wque 操作対象のウエイトキュー
   @param[in] lock 資源排他用ロック
   @retval 起床要因
 */
wque_reason
wque_wait_on_queue_with_spinlock(wque_waitqueue *wque, spinlock *lock){
	wque_entry   ent;

	wque_init_wque_entry(&ent);   /* ウエイトキューエントリを初期化する */

	enque_wque_entry(wque, &ent); /* ウエイトキューエントリをウエイトキューに追加する */

	spinlock_unlock(lock);        /* スピンロックを解放する */

	sched_schedule(); 	      /* スレッド休眠に伴う再スケジュール */

	spinlock_lock(lock);          /* スピンロックを獲得する */

	return ent.reason;            /* 起床要因を返却する */
}

/**
   ミューテックスで排他している資源を待ち合わせる
   @param[in] wque 操作対象のウエイトキュー
   @param[in] mtx  資源排他用ミューテックス
   @retval 起床要因
 */
wque_reason
wque_wait_on_event_with_mutex(wque_waitqueue *wque, mutex *mtx){
	wque_entry   ent;

	wque_init_wque_entry(&ent);   /* ウエイトキューエントリを初期化する */

	enque_wque_entry(wque, &ent); /* ウエイトキューエントリをウエイトキューに追加する */

	mutex_unlock(mtx);  /* ミューテックスを解放する */

	sched_schedule();   /* スレッド休眠に伴う再スケジュール */

	mutex_lock(mtx);    /* ミューテックスを獲得する */

	return ent.reason;  /* 起床要因を返却する */
}

/**
   ウエイトキューで休眠しているスレッドを起床する
   @param[in] wque   操作対象のウエイトキュー
   @param[in] reason 起床要因
   @note LO: ウエイトキューのロック, スレッドのロックの順に獲得
*/
void
wque_wakeup(wque_waitqueue *wque, wque_reason reason){
	bool         res;
	wque_entry  *ent;
	intrflags iflags;

	spinlock_lock_disable_intr(&wque->lock, &iflags);   /* ウエイトキューをロック     */

	while( !queue_is_empty(&wque->que) ) { /* ウエイトキューが空でなければ */

		/* ウエイトエントリを取り出す */
		ent = container_of(queue_get_top(&wque->que), wque_entry, link);
		ent->reason = reason; /* 起床要因を通知する */

		res = thr_ref_inc(ent->thr);    /* スレッドの参照を取得     */
		if ( !res )
			continue;  /* 終了中のスレッドを無視する */

		spinlock_lock(&ent->thr->lock); /* スレッドをロックする */
		ent->thr->state = THR_TSTATE_RUNABLE;  /* 状態を更新 */
		spinlock_unlock(&ent->thr->lock); /* スレッドをアンロックする */

		sched_thread_add(ent->thr); /* スレッドをレディーキューに追加 */

		res = thr_ref_dec(ent->thr);    /* スレッドの参照を解放     */
		kassert( !res );                /* スレッドは終了していないはず */
		
		if ( wque->wqflag == WQUE_WAKEFLAG_ONE )
			break;  /* 先頭のスレッドだけを起こして抜ける */
	}

	spinlock_unlock_restore_intr(&wque->lock, &iflags); /* ウエイトキューをアンロック */
}

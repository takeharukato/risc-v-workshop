/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Scheduler queue                                                   */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>
#include <kern/spinlock.h>
#include <kern/thr-if.h>
#include <kern/sched-if.h>

/** レディキュー */
static sched_queue ready_queue={.lock = __SPINLOCK_INITIALIZER,};

/**
   実行可能なスレッドを返却する
   @return 実行可能なスレッド
   @return NULL 実行可能なスレッドがない
   @note TODO: unusedを落とす
 */
static thread * 
get_next_thread(void){
	thread          *thr;
	singned_cnt_type idx;
	intrflags     iflags;

	/* レディキューをロック */
	spinlock_lock_disable_intr(&ready_queue.lock, &iflags); 

	/* ビットマップの最初に立っているビットの位置を確認  */
	idx = bitops_ffs(&ready_queue.bitmap);
	if ( idx == 0 )
		goto unlock_out;  /* 実行可能なスレッドがない  */

	--idx;  /* レディキュー配列のインデックスに変換 */

	/* キューの最初のスレッドを取り出す */
	thr = container_of(queue_get_top(&ready_queue.que[idx].que), thread, link);
	if ( queue_is_empty(&ready_queue.que[idx].que) )   /*  キューが空になった  */
		bitops_clr(idx, &ready_queue.bitmap);  /* ビットマップ中のビットをクリア */
	kassert( thr->state == THR_TSTATE_RUNABLE );   /* 実行可能スレッドである事を確認する */

	 /* レディキューをアンロック */
	spinlock_unlock_restore_intr(&ready_queue.lock, &iflags);

	return thr;

unlock_out:
	spinlock_unlock_restore_intr(&ready_queue.lock, &iflags); /* レディキューをアンロック */
	return NULL;
}

/**
   スレッドをレディキューに追加する
   @param[in] thr 追加するスレッド
 */
void
sched_thread_add(thread *thr){
	thr_prio        prio;
	intrflags     iflags;

	/* スレッドがどこにもリンクされていないことを確認する */
	kassert(list_not_linked(&thr->link));  
	kassert( thr->state == THR_TSTATE_RUNABLE );   /* 実行可能スレッドである事を確認する */

	spinlock_lock_disable_intr(&ready_queue.lock, &iflags); /* レディキューをロック */

	prio = thr->attr.cur_prio;

	if ( queue_is_empty(&ready_queue.que[prio].que) )   /*  キューが空だった場合            */
		bitops_set(prio, &ready_queue.bitmap);      /*  ビットマップ中のビットをセット  */

	queue_add(&ready_queue.que[prio].que, &thr->link);  /*  キューにスレッドを追加          */

	spinlock_unlock_restore_intr(&ready_queue.lock, &iflags); /* レディキューをアンロック   */
}

/**
   スケジューラ本体
 */
void
sched_schedule(void) {
	thread  *prev, *next;
	thread          *cur;
	thread_info      *ti;
	intrflags     iflags;

	krn_cpu_save_and_disable_interrupt(&iflags); /* 割り込み禁止 */
	ti_set_preempt_active();                     /* プリエンプションの抑止 */
	if ( ti_dispatch_disabled() ) {

		/* プリエンプション不可能な区間から呼ばれた場合は, 
		 * 遅延ディスパッチ要求をセットして呼び出し元へ復帰し,
		 * 例外出口処理でディスパッチを実施
		 */
		ti_set_delay_dispatch();  
		goto schedule_out;
	}

	prev = ti_get_current_thread();
	next = get_next_thread(); 
	if ( prev == next ) /* ディスパッチする必要なし  */
		goto schedule_out;

	/* TODO: thr_thread_switchの実装 */
	//thr_thread_switch(prev, next);  /* スレッド切り替え */

	cur = ti_get_current_thread();
	cur->status = THR_TSTATE_RUN;

schedule_out:
	ti_clr_preempt_active(); /* プリエンプションの許可 */
	krn_cpu_restore_interrupt(&iflags); /* 割り込み復元 */
}

/**
   スケジューラの初期化
 */
void
sched_init(void){
	int i;

	spinlock_init(&ready_queue.lock);
	bitops_zero(&ready_queue.bitmap);
	for( i = 0; SCHED_MAX_PRIO > i; ++i) {

		queue_init(&ready_queue.que[i].que);
	}
}

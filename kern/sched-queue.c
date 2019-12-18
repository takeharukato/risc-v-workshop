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
static __unused thread * 
find_next_thread(void){
	struct _thread  *thr;
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

	 /* レディキューをアンロック */
	spinlock_unlock_restore_intr(&ready_queue.lock, &iflags);

	return thr;

unlock_out:
	spinlock_unlock_restore_intr(&ready_queue.lock, &iflags); /* レディキューをアンロック */
	return NULL;
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

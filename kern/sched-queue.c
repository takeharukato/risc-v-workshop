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
#include <kern/page-if.h>
#include <kern/sched-queue.h>

static sched_queue ready_queue={.lock = __SPINLOCK_INITIALIZER,};

/**
 */
struct _thread *
sched_find_next(void){
	struct _thread  *thr;
	singned_cnt_type idx;
	intrflags     iflags;

	spinlock_lock_disable_intr(&ready_queue.lock, &iflags);   /* レディキューをロック */

	/* ビットマップの最初に立っているビットの位置を確認  */
	idx = bitops_ffs(&ready_queue.bitmap);  
	if ( idx == 0 )
		goto unlock_out;  /* 実行可能なスレッドがない  */

	spinlock_unlock_restore_intr(&ready_queue.lock, &iflags); /* レディキューをアンロック */
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

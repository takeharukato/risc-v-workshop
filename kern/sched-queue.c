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
void
sched_schedqueue_init(void){
	int i;

	spinlock_init(&ready_queue.lock);
	bitops_zero(&ready_queue.bitmap);
	for( i = 0; SCHED_MAX_PRIO > i; ++i) {

		queue_init(&ready_queue.que[i].que);
	}
}

/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  test routine                                                      */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>
#include <kern/thr-if.h>
#include <kern/proc-if.h>
#include <kern/sched-if.h>
#include <kern/mutex.h>
#include <kern/ktest.h>

static ktest_stats tstat_mutex=KTEST_INITIALIZER;
static mutex mtxA;

static void
threada(void *arg){
	int rc;

	while(1){

		kprintf("ThreadA\n");
		rc = mutex_try_lock(&mtxA);
		if ( rc == 0 ) 
			break;

		kprintf("ThreadA: can not lock mutexA\n");
		sched_schedule();
	}
	mutex_unlock(&mtxA);
	thr_thread_exit(0);

}

static void __unused
threadb(void *arg){
	int rc;

	while(1){

		kprintf("ThreadB\n");
		rc = mutex_try_lock(&mtxA);
		if ( rc == 0 ) 
			break;

		kprintf("ThreadB: can not lock mutexA\n");
		sched_schedule();
	}
	mutex_unlock(&mtxA);
	thr_thread_exit(0);
}

static void
mutex1(struct _ktest_stats *sp, void __unused *arg){
	int            rc;
	thread      *thrA;
//	thread      *thrB;
	thr_wait_res  res;

	mutex_init(&mtxA);

	mutex_lock(&mtxA);

	rc = thr_kernel_thread_create(THR_TID_AUTO, (entry_addr )threada, NULL, 
			       SCHED_MIN_USER_PRIO, THR_THRFLAGS_KERNEL, &thrA);
	kassert( rc == 0 );
#if 0
	rc = thr_kernel_thread_create(THR_TID_AUTO, (entry_addr )threadb, NULL, 
			       SCHED_MIN_USER_PRIO, THR_THRFLAGS_KERNEL, &thrB);
	kassert( rc == 0 );
	sched_thread_add(thrB);
#endif
	sched_thread_add(thrA);

	sched_schedule();
	mutex_unlock(&mtxA);

	rc = thr_thread_wait(&res);
//	rc = thr_thread_wait(&res);
	thr_thread_exit(0);
}

void
tst_mutex(void){

	ktest_def_test(&tstat_mutex, "mutex1", mutex1, NULL);
	ktest_run(&tstat_mutex);
}


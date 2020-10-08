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
static mutex mtxB;

static void
threada(void *arg){
	int   rc;
	bool res;

	while(1){

		kprintf("ThreadA\n");
		rc = mutex_try_lock(&mtxA);
		if ( rc == 0 )
			break;

		kprintf("ThreadA: can not trylock mutexA. wait lock\n");
		kprintf("ThreadA: lock mutexB.wait threadB.\n");
		rc = mutex_lock(&mtxB);
		kassert( rc == -ENODEV );
		mutex_lock(&mtxA);
		kprintf("ThreadA: lock mutexA.\n");
		break;
	}

	res = mutex_locked_by_self(&mtxA);
	kassert( res );
	kprintf("ThreadA: locked mutexA.\n");
	mutex_unlock(&mtxA);
	kprintf("ThreadA: unlocked mutexA.\n");
	kprintf("ThreadA: exit\n");
	thr_thread_exit(0);
}

static void
threadb(void *arg){
	int rc;

	while(1){

		kprintf("ThreadB: try lock mutexA\n");
		rc = mutex_try_lock(&mtxA);
		if ( rc == 0 )
			break;

		kprintf("ThreadB: can not lock mutexA\n");
		sched_schedule();
	}
	kprintf("ThreadB: destroy mutexB\n");
	mutex_destroy(&mtxB);

	if ( mutex_locked_by_self(&mtxA) ) {

		kprintf("ThreadA: unlock mutexA.\n");
		mutex_unlock(&mtxA);
	}
	kprintf("ThreadB: exit\n");
	thr_thread_exit(0);
}

static void
mutex1(struct _ktest_stats *sp, void __unused *arg){
	int            rc;
	thread      *thrA;
	thread      *thrB;
	thr_wait_res  res;

	mutex_init(&mtxA);
	mutex_init(&mtxB);

	mutex_lock(&mtxA);
	mutex_lock(&mtxB);

	rc = thr_kernel_thread_create(THR_TID_AUTO, (entry_addr )threada, NULL,
			       SCHED_MIN_USER_PRIO, THR_THRFLAGS_KERNEL, &thrA);
	kassert( rc == 0 );

	rc = thr_kernel_thread_create(THR_TID_AUTO, (entry_addr )threadb, NULL,
			       SCHED_MIN_USER_PRIO, THR_THRFLAGS_KERNEL, &thrB);
	kassert( rc == 0 );

	sched_thread_add(thrA);
	sched_thread_add(thrB);

	sched_schedule();
	mutex_unlock(&mtxA);

	rc = thr_thread_wait(&res);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	rc = thr_thread_wait(&res);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	kprintf("tst-mutex: end\n");
}

void
tst_mutex(void){

	ktest_def_test(&tstat_mutex, "mutex1", mutex1, NULL);
	ktest_run(&tstat_mutex);
}


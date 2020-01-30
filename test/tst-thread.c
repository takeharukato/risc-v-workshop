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
#include <kern/ktest.h>

static ktest_stats tstat_thread=KTEST_INITIALIZER;

static void
thread_test(void *arg){

	thr_thread_exit(0);
}

static void
threada(void *arg){
	int           rc;
	thread      *thr;
	thr_wait_res res;

	rc = thr_thread_create(THR_TID_AUTO, (entry_addr )thread_test, NULL, NULL, 
			       SCHED_MIN_USER_PRIO, THR_THRFLAGS_KERNEL, &thr);
	kassert( rc == 0 );

	sched_thread_add(thr);
	thr_thread_wait(&res);
	while(1)
		sched_schedule();
}
static void
thread1(struct _ktest_stats *sp, void __unused *arg){
	int      rc;
	thread *thr;

	rc = thr_thread_create(THR_TID_AUTO, (entry_addr )threada, NULL, NULL, 
			       SCHED_MIN_USER_PRIO, THR_THRFLAGS_KERNEL, &thr);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	sched_thread_add(thr);
	while(1)
		sched_schedule();
}

void
tst_thread(void){

	ktest_def_test(&tstat_thread, "thread1", thread1, NULL);
	ktest_run(&tstat_thread);
}


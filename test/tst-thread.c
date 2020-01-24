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

#include <kern/ktest.h>

static ktest_stats tstat_thread=KTEST_INITIALIZER;

static void
thread1(struct _ktest_stats *sp, void __unused *arg){

	if ( true )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	return;
}

void
tst_thread(void){

	ktest_def_test(&tstat_thread, "thread1", thread1, NULL);
	ktest_run(&tstat_thread);
}


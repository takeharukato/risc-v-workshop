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
#include <kern/proc-if.h>
#include <kern/thr-if.h>
#include <kern/ktest.h>

#define TST_PROC1_BUF_NR   (4)
#define TST_CH0            ('a')
#define TST_NR               (8)

static ktest_stats tstat_proc=KTEST_INITIALIZER;

static void
proc1(struct _ktest_stats *sp, void __unused *arg){
	int      rc;
	proc    *p1;
	proc  *pres;
	thread *thr;
	proc    *kp;
	bool    res;

	rc = proc_user_allocate(0x10390, &p1);
	if ( rc == 0 ) 
		ktest_pass( sp );
	else
		ktest_fail( sp );

	if ( rc == 0 ) {

		res = proc_ref_inc(p1);
		if ( res )
			ktest_pass( sp );
		else
			ktest_fail( sp );
		res = proc_ref_dec(p1);
		if ( !res )
			ktest_pass( sp );
		else
			ktest_fail( sp );
		pres = proc_find_by_pid(p1->id);  /* プロセスへの参照を取得 */
		if ( pres != NULL )
			ktest_pass( sp );
		else
			ktest_fail( sp );
		if ( pres != NULL )
			res = proc_ref_dec(pres);   /* プロセスへの参照を解放 */

		thr = proc_find_thread(p1->id);  /* マスタースレッド取得                    */
		proc_del_thread(p1, thr);        /* マスタースレッド除去に伴うプロセス解放  */
		/* TODO: 本来は, 自スレッド終了処理を呼び出す  */
		thr_ref_dec(thr);                /* スレッドの参照返却                      */
	}
	kp = proc_kproc_refer();
	if ( kp != NULL )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	return;
}

void
tst_proc(void){

	ktest_def_test(&tstat_proc, "proc1", proc1, NULL);
	ktest_run(&tstat_proc);
}


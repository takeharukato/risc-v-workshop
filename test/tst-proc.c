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

static const char __unused *tst_args[]={"init", "arg1", NULL};  //"arg2", "arg3",
static const char __unused *tst_envs[]={"TERM=rv64ws", NULL};

static void
proc1(struct _ktest_stats *sp, void __unused *arg){
	int              rc;
	proc            *p1;
	proc          *pres;
	thread         *thr;
	thread      *master;
	proc            *kp;
	bool            res;
	vm_vaddr        usp;
	thread_args thrargs;
#if defined(CONFIG_HAL)
	vm_vaddr      argcp;
	vm_vaddr      argvp;
	vm_vaddr       envp;
#endif  /* CONFIG_HAL */

	usp = truncate_align(HAL_USER_END_ADDR, HAL_STACK_ALIGN_SIZE);
	memset(&thrargs, 0, sizeof(thread_args));

	rc = proc_user_allocate(&p1);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	if ( rc == 0 ) {

#if defined(CONFIG_HAL)
		usp = HAL_USER_END_ADDR;
		rc = proc_argument_copy(proc_kernel_process_refer(),
		    p1->segments[PROC_STACK_SEG].prot, tst_args, tst_envs, p1,
		    &usp, &argcp, &argvp, &envp);
		if ( rc == 0 )
			ktest_pass( sp );
		else
			ktest_fail( sp );
		hal_pgtbl_activate(p1->pgt);

		hal_pgtbl_deactivate(p1->pgt);
		hal_pgtbl_activate(hal_refer_kernel_pagetable());

		thrargs.arg1 = argcp;
		thrargs.arg2 = argvp;
		thrargs.arg3 = envp;
#endif

		rc = thr_user_thread_create(THR_TID_AUTO, 0x10390, &thrargs, p1,
		    (void *)usp, SCHED_MIN_USER_PRIO, THR_THRFLAGS_USER, &master);
		if ( rc == 0 )
			ktest_pass( sp );
		else
			ktest_fail( sp );

		rc = proc_add_thread(p1, master);
		if ( rc == 0 )
			ktest_pass( sp );
		else
			ktest_fail( sp );
		/*
		 * 参照獲得処理のテスト
		 */
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
	kp = proc_kernel_process_refer();
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


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
/**
   引数領域の大きさを算出する
   @param[in]  src         引数を読込むプロセス
   @param[in]  argv        引数配列のアドレス
   @param[in]  environment 環境変数配列のアドレス
   @param[out] sizp        引数領域長返却領域
   @retval     0           正常終了
   @retval    -EFAULT      メモリアクセス不可
 */
int
thr_argarea_size_calc(proc *src, const char *argv[], const char *environment[], size_t *sizp){
	int          rc;
	int           i;
	size_t      len;
	size_t argc_len;
	size_t argv_len;
	size_t  env_len;

	/* argc, argv, environmentを配置するために必要な領域長を算出する
	 */
	for(i = 0, argv_len = 1; argv[i] != NULL; ++i) { /* NULLターミネート分を足す */

		len = vm_strlen(src->pgt, argv[i]);
		if ( len == 0 ) {

			rc = -EFAULT;  /* アクセスできなかった */
			goto error_out;
		}
		argv_len +=  len + 1;  /* 文字列長+NULLターミネート */
	}

	for(i = 0, env_len = 1; environment[i] != NULL; ++i) { /* NULLターミネート分を足す */

		len = vm_strlen(src->pgt, environment[i]);
		if ( len == 0 ) {

			rc = -EFAULT;  /* アクセスできなかった */
			goto error_out;
		}
		env_len += len + 1;  /* 文字列長+NULLターミネート */
	}

	/* argc, argv, environmentに対してスタックアラインメントで
	 * アクセスできるようにサイズを調整する
	 */
	argc_len = roundup_align(sizeof(reg_type), HAL_STACK_ALIGN_SIZE);
	argv_len = roundup_align(argv_len, HAL_STACK_ALIGN_SIZE);
	env_len = roundup_align(env_len, HAL_STACK_ALIGN_SIZE);

	if ( sizp != NULL )
		*sizp = argc_len + argv_len + env_len;  /* 引数領域長を返却する */

	return  0;

error_out:
	return rc;
}
#if 0
int
setup_thread_args(proc *src, proc *dest, void *sp, reg_type argc, const char *argv[], 
    const char *environment[]){
	int           i;
	size_t      len;
	size_t argc_len;
	size_t argv_len;
	size_t  env_len;
	void     *destp;

	/* argc, argv, environmentを配置するために必要な領域長を算出する
	 */
	for(i = 0, argv_len = 1; argv[i] != NULL; ++i) { /* NULLターミネート分を足す */

		argv_len += strlen(argv[i]) + 1;  /* 文字列長+NULLターミネート */
	}

	for(i = 0, env_len = 1; environment[i] != NULL; ++i) { /* NULLターミネート分を足す */

		env_len += strlen(environment[i]) + 1;  /* 文字列長+NULLターミネート */
	}

	argc_len = roundup_align(sizeof(reg_type), HAL_STACK_ALIGN_SIZE);
	argv_len = roundup_align(argv_len, HAL_STACK_ALIGN_SIZE);
	env_len = roundup_align(env_len, HAL_STACK_ALIGN_SIZE);
}
#endif

static const char *tst_args[]={"init", "arg1", "arg2", "arg3"};
static const char *tst_envs[]={"TERM=rv64ws"};

static void
thread_test(void *arg){

	thr_thread_exit(0);
}

static void
threada(void *arg){
	int           rc;
	thread      *thr;

	kassert( (reg_type)arg == 1 );
	rc = thr_kernel_thread_create(THR_TID_AUTO, (entry_addr )thread_test, NULL, 
			       SCHED_MIN_USER_PRIO, THR_THRFLAGS_KERNEL, &thr);
	kassert( rc == 0 );

	sched_thread_add(thr);
	/* 親を先に終了させる */
	thr_thread_exit(rc);
}
static void
thread1(struct _ktest_stats *sp, void __unused *arg){
	int           rc;
	size_t       siz;
	thread      *thr;
	thr_wait_res res;
	thread_args args;
	tid           id;

	/*
	 * 引数エラーテスト
	 */
	/* カーネルスレッド */
	rc = thr_kernel_thread_create(THR_TID_AUTO, (entry_addr )threada, NULL, 
			       SCHED_MAX_PRIO - 1, THR_THRFLAGS_KERNEL, &thr);
	if ( rc == -EINVAL )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	rc = thr_kernel_thread_create(THR_TID_AUTO, (entry_addr )threada, NULL, 
			       SCHED_MIN_PRIO + 1, THR_THRFLAGS_KERNEL, &thr);
	if ( rc == -EINVAL )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/* ユーザスレッド */
	rc = thr_user_thread_create(THR_TID_AUTO, (entry_addr )threada, NULL, 
	    proc_kernel_process_refer(), NULL, SCHED_MAX_USER_PRIO - 1, 
	    THR_THRFLAGS_USER, &thr);
	if ( rc == -EINVAL )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	rc = thr_user_thread_create(THR_TID_AUTO, (entry_addr )threada, NULL, 
	    proc_kernel_process_refer(), NULL, SCHED_MIN_USER_PRIO + 1, 
	    THR_THRFLAGS_USER, &thr);
	if ( rc == -EINVAL )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/*
	 * 正常系テスト
	 */
	args.arg1 = 1;
	args.arg2 = 2;
	args.arg3 = 3;
	args.arg4 = 4;
	args.arg5 = 5;
	args.arg6 = 6;
	rc = thr_kernel_thread_create(THR_TID_AUTO, (entry_addr )threada, &args, 
			       SCHED_MIN_USER_PRIO, THR_THRFLAGS_KERNEL, &thr);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	sched_thread_add(thr);
	id = thr->id;

	rc = thr_thread_wait(&res);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	if ( res.id == id )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	rc = thr_argarea_size_calc(proc_kernel_process_refer(), tst_args, tst_envs, &siz);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	kprintf("arg siz=%lu\n", siz);
	return;
}

void
tst_thread(void){

	ktest_def_test(&tstat_thread, "thread1", thread1, NULL);
	ktest_run(&tstat_thread);
}


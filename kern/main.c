/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  main routine                                                      */
/*                                                                    */
/**********************************************************************/

#include <kern/kern-common.h>
#include <kern/ktest.h>
#include <kern/kern-if.h>
#include <kern/page-if.h>
#include <kern/vm-if.h>
#include <kern/proc-if.h>
#include <kern/thr-if.h>
#include <kern/sched-if.h>
#include <kern/fs-fsimg.h>
#include <kern/dev-pcache.h>
#include <kern/irq-if.h>
#include <kern/timer.h>
#include <klib/asm-offset.h>
#if !defined(CONFIG_HAL)
#include <stdlib.h>
#endif  /*  !CONFIG_HAL  */
/** 
    カーネルのアーキ共通テスト
 */
void
kern_common_tests(void){

	tst_memset();
	tst_fixedpoint();
	tst_spinlock();
	tst_atomic();
	tst_atomic64();
	tst_cpuinfo();
	tst_vmmap();
	tst_pcache();
	tst_proc();
#if defined(CONFIG_HAL)
	tst_vmcopy();
	tst_vmstrlen();
#endif  /*  CONFIG_HAL  */
#if !defined(CONFIG_HAL)
	tst_rv64pgtbl();
	tst_irqctrlr();
#endif  /*  CONFIG_HAL  */ 
	tst_thread();
	kprintf("end\n");
#if !defined(CONFIG_HAL)
	exit(0);
#endif  /*  CONFIG_HAL  */
}

/**
   テストスレッド
   @param[in] _arg スレッド引数
 */
static void
test_thread(void __unused *_arg){

	kern_common_tests();
	thr_thread_exit(0);
}

/** カーネルの初期化
 */
void
kern_init(void) {
	int       rc;
	thread  *thr;

	kassert( ti_dispatch_disabled() ); /* ディスパッチ禁止であることを確認 */

	kprintf("fsimage: [%p, %p) len:%u\n", 
		(uintptr_t)&_fsimg_start, (uintptr_t)&_fsimg_end, 
		(uintptr_t)&_fsimg_end - (uintptr_t)&_fsimg_start);

	proc_init();  /* プロセス管理情報を初期化する */
	thr_init(); /* スレッド管理機構を初期化する */
	sched_init(); /* スケジューラを初期化する */
	irq_init(); /* 割込み管理を初期化する */
	tim_callout_init();  /* コールアウト機構を初期化する */
	pagecache_init(); /* ページキャッシュ機構を初期化する */
	fsimg_load();     /* ファイルシステムイメージをページキャッシュに読み込む */
	hal_platform_init();  /* アーキ固有のプラットフォーム初期化処理 */

	/**
	   テスト処理用スレッドを起動する
	 */
	rc = thr_thread_create(THR_TID_AUTO, (entry_addr )test_thread, NULL, NULL, 
			       SCHED_MIN_USER_PRIO, THR_THRFLAGS_KERNEL, &thr);
	kassert( rc == 0 );
	sched_thread_add(thr); /* スレッドを開始する */

	ti_dec_preempt();  /* ディスパッチを許可する */
	kassert( !ti_dispatch_disabled() ); /* ディスパッチ可能であることを確認 */

	thr_idle_loop(NULL);  /* アイドル処理を実行する */
}

#if !defined(CONFIG_HAL)
/**
   ユーザランドテスト用の初期化を行う
 */
static void
uland_test_init(void){
	cpu_id log_id;

	tflib_kernlayout_init();
	slab_prepare_preallocate_cahches();
	vm_pgtbl_cache_init();  /* ページテーブル情報のキャッシュを初期化する */

	krn_cpuinfo_init();  /* CPU情報を初期化する */
	krn_cpuinfo_cpu_register(0, &log_id); /* BSPを登録する */
	krn_cpuinfo_online(log_id); /* CPUをオンラインにする */

	kern_init();  /* カーネルの初期化 */

	tflib_kernlayout_finalize();
}
int 
main(int argc, char *argv[]) {
	void *new_sp;
	thread_info *ti;

	kprintf("Kernel\n");
	ti = calc_thread_info_from_kstack_top( (void *)&_tflib_bsp_stack );
	ti->kstack = (void *)&_tflib_bsp_stack;
	new_sp = (void *)ti 
		- (TI_THREAD_INFO_SIZE + X64_TRAP_CONTEXT_SIZE + X64_THRSW_CONTEXT_SIZE);
	ti->preempt = 1; /* プリエンプション禁止 */
	hal_call_with_newstack(uland_test_init, NULL, new_sp);

	return 0;
}
#endif  /*  !CONFIG_HAL  */

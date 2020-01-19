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
//	tst_proc();
#if defined(CONFIG_HAL)
	tst_vmcopy();
	tst_vmstrlen();
#endif 
#if !defined(CONFIG_HAL)
	tst_rv64pgtbl();
	tst_irqctrlr();
#endif 
}
/** カーネルの初期化
 */
void
kern_init(void) {

	kprintf("fsimage: [%p, %p) len:%u\n", 
		(uintptr_t)&_fsimg_start, (uintptr_t)&_fsimg_end, 
		(uintptr_t)&_fsimg_end - (uintptr_t)&_fsimg_start);

	proc_init();  /* プロセス管理情報を初期化する */
	sched_init(); /* スケジューラを初期化する */
	thr_init(); /* スレッド管理機構を初期化する */
	irq_init(); /* 割込み管理を初期化する */
	tim_callout_init();  /* コールアウト機構を初期化する */
	pagecache_init(); /* ページキャッシュ機構を初期化する */
	fsimg_load();     /* ファイルシステムイメージをページキャッシュに読み込む */
	hal_platform_init();  /* アーキ固有のプラットフォーム初期化処理 */
	kern_common_tests();
	krn_cpu_enable_interrupt();
}

#if !defined(CONFIG_HAL)
int 
main(int argc, char *argv[]) {
	cpu_id log_id;
	void *new_sp;

	kprintf("Kernel\n");

	new_sp = ( (void *)&_tflib_bsp_stack ) + TI_KSTACK_SIZE - sizeof(thread_info);

	tflib_kernlayout_init();
	slab_prepare_preallocate_cahches();
	vm_pgtbl_cache_init();  /* ページテーブル情報のキャッシュを初期化する */

	krn_cpuinfo_init();  /* CPU情報を初期化する */
	krn_cpuinfo_cpu_register(0, &log_id); /* BSPを登録する */
	krn_cpuinfo_online(log_id); /* CPUをオンラインにする */
	hal_call_with_newstack(kern_init, NULL, new_sp);

	tflib_kernlayout_finalize();
	return 0;
}
#endif  /*  !CONFIG_HAL  */

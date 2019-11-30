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
#include <kern/fs-fsimg.h>
#include <kern/fs-pcache.h>
#include <kern/irq-if.h>

/** 
    カーネルのアーキ共通テスト
 */
void
kern_common_tests(void){

	tst_memset();
	tst_spinlock();
	tst_atomic();
	tst_atomic64();
	tst_vmmap();
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

	irq_init(); /* 割込み管理を初期化する */
	hal_platform_init();  /* アーキ固有のプラットフォーム初期化処理 */

	kern_common_tests();
}

#if !defined(CONFIG_HAL)
int 
main(int argc, char *argv[]) {
	
	kprintf("Kernel\n");

	tflib_kernlayout_init();
	slab_prepare_preallocate_cahches();
	vm_pgtbl_cache_init();  /* ページテーブル情報のキャッシュを初期化する */

	krn_cpuinfo_init();  /* CPU情報を初期化する */
	krn_cpuinfo_fill(0, 0); /* BSPを登録する */

	kern_init();

	tflib_kernlayout_finalize();
	return 0;
}
#endif  /*  !CONFIG_HAL  */

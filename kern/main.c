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
#include <kern/page-if.h>
#include <kern/vm-if.h>
#include <kern/fsimg.h>

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
#endif 
}
/** カーネルの初期化
 */
void
kern_init(void) {

	kern_common_tests();
}

#if !defined(CONFIG_HAL)
int 
main(int argc, char *argv[]) {
	
	kprintf("Kernel\n");

	tflib_kernlayout_init();
	slab_prepare_preallocate_cahches();
	vm_pgtbl_cache_init();  /* ページテーブル情報のキャッシュを初期化する */

	kprintf("fsimage: [%p, %p)\n", &_fsimg_start, &_fsimg_end);
	kern_common_tests();
	tflib_kernlayout_finalize();
	return 0;
}
#endif  /*  !CONFIG_HAL  */

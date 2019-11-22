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

/** 
    カーネルのアーキ共通テスト
 */
void
kern_common_tests(void){

	tst_memset();
	tst_spinlock();
	tst_atomic();
	tst_atomic64();
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
	kern_common_tests();
	tflib_kernlayout_finalize();
	return 0;
}
#endif  /*  !CONFIG_HAL  */

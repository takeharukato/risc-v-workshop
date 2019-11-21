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

/** 
    カーネルのアーキ共通テスト
 */
void
kern_common_tests(void){

	tst_spinlock();
	tst_atomic();
	tst_atomic64();
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

	kern_common_tests();

	return 0;
}
#endif  /*  !CONFIG_HAL  */

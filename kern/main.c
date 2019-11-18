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

/** カーネルの初期化
 */
void
kern_init(void) {

	tst_spinlock();
	tst_atomic();
	tst_atomic64();
}

#if !defined(CONFIG_HAL)
int 
main(int argc, char *argv[]) {


	return 0;
}
#endif  /*  !CONFIG_HAL  */

/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  main routine                                                      */
/*                                                                    */
/**********************************************************************/

#include <kern/kern-common.h>

/** カーネルの初期化
 */
void
kern_init(void) {
	
}

#if !defined(CONFIG_HAL)
int 
main(int argc, char *argv[]) {


	return 0;
}
#endif  /*  !CONFIG_HAL  */

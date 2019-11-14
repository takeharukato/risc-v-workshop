/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  main routine                                                      */
/*                                                                    */
/**********************************************************************/

#include <kern/kern-common.h>
#include <kern/spinlock.h>

spinlock lock;

/** カーネルの初期化
 */
void
kern_init(void) {
	intrflags iflags;

	spinlock_init(&lock);
	spinlock_lock_disable_intr(&lock, &iflags);
	spinlock_unlock_restore_intr(&lock, &iflags);
}

#if !defined(CONFIG_HAL)
int 
main(int argc, char *argv[]) {


	return 0;
}
#endif  /*  !CONFIG_HAL  */

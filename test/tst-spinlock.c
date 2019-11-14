/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  test routine                                                      */
/*                                                                    */
/**********************************************************************/

#include <kern/kern-common.h>
#include <kern/ktest.h>
#include <kern/spinlock.h>

spinlock lock;

void
tst_spinlock(void){
	int            i;
	intrflags iflags;

	spinlock_init(&lock);
	spinlock_lock_disable_intr(&lock, &iflags);
	spinlock_unlock_restore_intr(&lock, &iflags);
	for(i = 0; SPINLOCK_BT_DEPTH > i; ++i ){

		if ( lock.backtrace[i] == NULL )
			break;
		kprintf("trace[%d] %p\n", i, lock.backtrace[i]);
	}
}

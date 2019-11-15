/* -*- mode: c; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  RISC-V preparation routines                                       */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>
#include <kern/spinlock.h>

#include <hal/hal-dbg-console.h>

void kern_init(void);

static spinlock prepare_lock=__SPINLOCK_INITIALIZER;

int rv64_map_kernel_space(void);

void
prepare(uint64_t hartid){
	intrflags iflags;

	if ( hartid == 0 )
		hal_dbg_console_init();
	spinlock_lock_disable_intr(&prepare_lock, &iflags);
	kprintf("Boot on supervisor mode\n");
	spinlock_unlock_restore_intr(&prepare_lock, &iflags);
	if ( hartid != 0 )
		goto loop;
	rv64_map_kernel_space();
	kern_init();
loop:
	while(1);
}

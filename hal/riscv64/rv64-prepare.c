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

#include <hal/hal-dbg-console.h>

void kern_init(void);

int rv64_map_kernel_space(void);

void
prepare(void){

	hal_dbg_console_init();
	kprintf("Boot on supervisor mode\n");
	rv64_map_kernel_space();
	kern_init();
	while(1);
}

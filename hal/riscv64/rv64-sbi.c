/* -*- mode: gas; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  RISC-V64 in kernel Supervisor Binary Interface                    */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>
#include <klib/backtrace.h>
#include <kern/page-macros.h>

#include <hal/rv64-clint.h>
#include <hal/rv64-sbi.h>

void
sbi_send_ipi(const unsigned long *hart_mask){
	unsigned long
	
}


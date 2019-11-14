/* -*- mode: c; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  RISC-V 64 cpu level interrupt control                             */
/*                                                                    */
/**********************************************************************/
#if !defined(_HAL_CPUINTR_H)
#define  _HAL_CPUINTR_H 

#include <klib/freestanding.h>
#include <kern/kern-common.h>

#include <hal/riscv64.h>

void rv64_cpu_disable_interrupt(void);
void rv64_cpu_enable_interrupt(void);
void rv64_cpu_restore_flags(intrflags *_flags);
void rv64_cpu_save_flags(intrflags *_flags);
#endif  /*  _HAL_CPUINTR_H   */

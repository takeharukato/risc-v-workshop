/* -*- mode: c; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Pseudo cpu level interrupt control                                */
/*                                                                    */
/**********************************************************************/
#if !defined(_HAL_CPUINTR_H)
#define  _HAL_CPUINTR_H

#include <klib/freestanding.h>
#include <kern/kern-types.h>
#include <hal/x64-rflags.h>

void x64_cpu_disable_interrupt(void);
void x64_cpu_enable_interrupt(void);
void x64_cpu_restore_flags(intrflags *_iflags);
void x64_cpu_save_flags(intrflags *_iflags);
#endif  /*  _HAL_CPUINTR_H   */

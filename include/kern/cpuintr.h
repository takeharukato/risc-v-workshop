/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  cpu level interrupt control                                       */
/*                                                                    */
/**********************************************************************/
#if !defined(_KERN_CPUINTR_H)
#define  _KERN_CPUINTR_H 

#include <klib/freestanding.h>
#include <kern/kern-types.h>

void krn_cpu_save_and_disable_interrupt(intrflags *_iflags);
void krn_cpu_restore_interrupt(intrflags *_iflags);
void krn_cpu_disable_interrupt(void);
void krn_cpu_enable_interrupt(void);
bool krn_cpu_interrupt_disabled(void);

void hal_cpu_save_interrupt(intrflags *_iflags);
void hal_cpu_restore_interrupt(intrflags *_iflags);
void hal_cpu_disable_interrupt(void);
void hal_cpu_enable_interrupt(void);
bool hal_cpu_interrupt_disabled(void);
bool hal_intrflags_interrupt_disabled(intrflags *_iflags);
#endif  /*  _KERN_CPUINTR_H   */

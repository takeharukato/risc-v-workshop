/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Thread interface definitions                                      */
/*                                                                    */
/**********************************************************************/
#if !defined(_KERN_THR_IF_H)
#define  _KERN_THR_IF_H 
#if !defined(ASM_FILE)
#include <klib/freestanding.h>
#include <kern/kern-types.h>
#include <kern/thr-preempt.h>
#include <kern/thr-thread.h>

int thr_create_kernel_thread(struct _thread_attr *_attr, struct _thread **_thr);
void thr_init(void);

#endif  /*  !ASM_FILE  */
#endif  /*  _KERN_THR_IF_H   */

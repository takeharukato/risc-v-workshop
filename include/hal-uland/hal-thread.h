/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  x64 userland specific thread definitions                          */
/*                                                                    */
/**********************************************************************/
#if !defined(_HAL_HAL_THREAD_H)
#define  _HAL_HAL_THREAD_H
#if !defined(ASM_FILE)
#include <klib/freestanding.h>
#include <kern/kern-types.h>
#include <hal/x64-rflags.h>

/**
   スレッドスイッチコンテキスト
 */
typedef struct _x64_thrsw_context{
	reg_type     r15;  /* r15    */
	reg_type     r14;  /* r14    */
	reg_type     r13;  /* r13    */
	reg_type     r12;  /* r12    */
	reg_type     rbp;  /* rbp    */
	reg_type     rbx;  /* rbx    */
	reg_type  rflags;  /* rflags */
	reg_type     rip;  /* rip    */
}x64_thrsw_context;

void hal_setup_thread_context(entry_addr _entry, void *_thr_sp, thr_flags _flags, 
    void **_stkp);

void hal_thread_switch(void **_prev, void **_next);

#endif  /*  ASM_FILE  */
#endif  /* _HAL_HAL_THREAD_H  */

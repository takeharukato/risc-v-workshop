/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  RISC-V 64 back trace operations                                   */
/*                                                                    */
/**********************************************************************/
#if !defined(_HAL_BACKTRACE_H)
#define  _HAL_BACKTRACE_H 

#include <klib/freestanding.h>

void hal_backtrace(int (*_trace_out)(int depth, uintptr_t *_bpref, void *_caller, 
	void *_next_bp, void *_argp), void *_basep, void *argp);


#endif  /*  _HAL_BACKTRACE_H   */

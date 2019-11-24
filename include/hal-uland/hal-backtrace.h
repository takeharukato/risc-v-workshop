/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  x64 back trace operations                                         */
/*                                                                    */
/**********************************************************************/
#if !defined(_HAL_BACKTRACE_H)
#define  _HAL_BACKTRACE_H 

#include <klib/freestanding.h>

void hal_backtrace(int (*_trace_out)(int depth, uintptr_t *_bpref, void *_caller, 
	void *_next_bp, void *_argp), void *_basep, void *argp);


#endif  /*  _HAL_BACKTRACE_H   */

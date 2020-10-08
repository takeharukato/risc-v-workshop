/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  backtrace relevant definitions                                    */
/*                                                                    */
/**********************************************************************/
#if !defined(_KLIB_BACKTRACE_H)
#define  _KLIB_BACKTRACE_H

#include <klib/freestanding.h>

#include <hal/hal-backtrace.h>

#define BACKTRACE_MAX_DEPTH (16)  /*<  バックトレース探索長  */

void print_backtrace(void *_basep);

#endif  /*  _KLIB_BACKTRACE_H   */

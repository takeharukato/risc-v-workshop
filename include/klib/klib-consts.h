/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  constant values for kernel library                                */
/*                                                                    */
/**********************************************************************/
#if !defined(_KLIB_KLIB_CONSTS_H)
#define  _KLIB_KLIB_CONSTS_H

#include <klib/misc.h>

#define BITS_PER_BYTE                     (8)  /*<  バイト当たりのビット数      */
#define TIMER_MS_PER_US   (ULONGLONG_C(1000))  /*<  1ミリ秒あたりのマイクロ秒数 */
#endif  /* KLIB_KLIB_CONSTS_H */

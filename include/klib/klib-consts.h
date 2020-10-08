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
#define TIMER_NS_PER_US   (ULONGLONG_C(1000))  /*<  1マイクロ秒あたりのナノ秒数 */
#define TIMER_US_PER_MS   (ULONGLONG_C(1000))  /*<  1ミリ秒あたりのマイクロ秒数 */
#define TIMER_MS_PER_SEC  (ULONGLONG_C(1000))  /*<  1秒あたりのミリ秒数         */
/** 1秒あたりのナノ秒数  */
#define TIMER_NS_PER_SEC  ( TIMER_NS_PER_US * TIMER_US_PER_MS * TIMER_MS_PER_SEC )
#endif  /* KLIB_KLIB_CONSTS_H */

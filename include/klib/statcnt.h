/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  64bit atomic operations                                           */
/*                                                                    */
/**********************************************************************/
#if !defined(_KLIB_STATCNT_H)
#define  _KLIB_STATCNT_H

#include <klib/freestanding.h>
#include <kern/kern-types.h>

#include <klib/atomic64.h>
/** 統計情報カウンタ
 */
typedef struct _stat_cnt{
	struct _atomic64 counter;  /*<  統計情報カウンタ  */
}stat_cnt;

typedef atomic64_val stat_cnt_val;  /**< 整数型統計情報カウンタ */

/**
   統計情報カウンタの初期化子
   @param[in] _val 初期値
 */
#define __STATCNT_INITIALIZER(_val) \
	{ .counter = __ATOMIC_INITIALIZER(_val), }

void statcnt_set(stat_cnt *_cnt, stat_cnt_val _val);
stat_cnt_val statcnt_read(stat_cnt *_cnt);
stat_cnt_val statcnt_add(stat_cnt *_cnt, stat_cnt_val _val);
stat_cnt_val statcnt_sub(stat_cnt *_cnt, stat_cnt_val _val);
stat_cnt_val statcnt_inc(stat_cnt *_cnt);
stat_cnt_val statcnt_dec(stat_cnt *_cnt);
#endif  /*  _KLIB_STATCNT_H   */

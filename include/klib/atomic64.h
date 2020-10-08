/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  64bit atomic operations                                           */
/*                                                                    */
/**********************************************************************/
#if !defined(_KLIB_ATOMIC64_H)
#define  _KLIB_ATOMIC64_H

#include <klib/freestanding.h>
#include <kern/kern-types.h>

#include <hal/hal-atomic64.h>

/** atomic64構造体
 */
typedef struct _atomic64{
	atomic64_val val;  /*< アトミック変数の値  */
}atomic64;

/**
   atomic64構造体の初期化子
   @param[in] _val 初期値
 */
#define __ATOMIC64_INITIALIZER(_val) { .val = (_val), }

atomic64_val atomic64_add_fetch(struct _atomic64 *_val, atomic64_val _incr);
atomic64_val atomic64_sub_fetch(struct _atomic64 *_val, atomic64_val _decr);
atomic64_val atomic64_and_fetch(struct _atomic64 *_val, atomic64_val _v);
atomic64_val atomic64_or_fetch(struct _atomic64 *_val, atomic64_val _v);
atomic64_val atomic64_xor_fetch(struct _atomic64 *_val, atomic64_val _v);
atomic64_val atomic64_set_fetch(struct _atomic64 *_val, atomic64_val _set_to);
atomic64_val atomic64_cmpxchg_fetch(struct _atomic64 *_val, atomic64_val _old, atomic64_val _new);
void *atomic64_cmpxchg_ptr_fetch(void **_valp, void *_old, void *_new);
bool atomic64_try_cmpxchg_fetch(struct _atomic64 *_val, atomic64_val *_oldp, atomic64_val _new);

atomic64_val atomic64_add_return(struct _atomic64 *_val, atomic64_val _incr);
atomic64_val atomic64_sub_return(struct _atomic64 *_val, atomic64_val _decr);
atomic64_val atomic64_and_return(struct _atomic64 *_val, atomic64_val _v);
atomic64_val atomic64_or_return(struct _atomic64 *_val, atomic64_val _v);
atomic64_val atomic64_xor_return(struct _atomic64 *_val, atomic64_val _v);

bool atomic64_sub_and_test(struct _atomic64 *_val, atomic64_val _decr, atomic64_val *_newp);

atomic64_val atomic64_read(struct _atomic64 *_val);
void atomic64_set(struct _atomic64 *_val, atomic64_val _new);

atomic64_val atomic64_add_fetch_unless(struct _atomic64 *_val, atomic64_val _unexpected, atomic64_val _decr);
atomic64_val atomic64_sub_fetch_unless(struct _atomic64 *_val, atomic64_val _unexpected, atomic64_val _decr);

#endif  /*  _KLIB_ATOMIC64_H   */

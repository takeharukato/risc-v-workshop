/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Atomic operations                                                 */
/*                                                                    */
/**********************************************************************/
#if !defined(_KLIB_ATOMIC_H)
#define  _KLIB_ATOMIC_H 

#include <klib/freestanding.h>
#include <kern/kern-types.h>

#include <hal/hal-atomic.h>

/** atomic構造体
 */
typedef struct _atomic{
	atomic_val val;  /*< アトミック変数の値  */
}atomic;

/**
   atomic構造体の初期化子
   @param[in] _val 初期値
 */
#define __ATOMIC_INITIALIZER(_val) { .val = (_val), }

atomic_val atomic_add_fetch(struct _atomic *_val, atomic_val _incr);
atomic_val atomic_sub_fetch(struct _atomic *_val, atomic_val _decr);
atomic_val atomic_and_fetch(struct _atomic *_val, atomic_val _v);
atomic_val atomic_or_fetch(struct _atomic *_val, atomic_val _v);
atomic_val atomic_xor_fetch(struct _atomic *_val, atomic_val _v);
atomic_val atomic_set_fetch(struct _atomic *_val, atomic_val _set_to);
atomic_val atomic_cmpxchg_fetch(struct _atomic *_val, atomic_val _old, atomic_val _new);
void *atomic_cmpxchg_ptr_fetch(void **_valp, void *_old, void *_new);
bool atomic_try_cmpxchg_fetch(struct _atomic *_val, atomic_val *_oldp, atomic_val _new);

atomic_val atomic_add_return(struct _atomic *_val, atomic_val _incr);
atomic_val atomic_sub_return(struct _atomic *_val, atomic_val _decr);
atomic_val atomic_and_return(struct _atomic *_val, atomic_val _v);
atomic_val atomic_or_return(struct _atomic *_val, atomic_val _v);
atomic_val atomic_xor_return(struct _atomic *_val, atomic_val _v);

bool atomic_sub_and_test(struct _atomic *_val, atomic_val _decr, atomic_val *_newp);

atomic_val atomic_read(struct _atomic *_val);
void atomic_set(struct _atomic *_val, atomic_val _new);

atomic_val atomic_add_fetch_unless(struct _atomic *_val, atomic_val _unexpected, atomic_val _decr);
atomic_val atomic_sub_fetch_unless(struct _atomic *_val, atomic_val _unexpected, atomic_val _decr);

#endif  /*  _KLIB_ATOMIC_H   */

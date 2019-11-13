/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  x64 64bit atomic operations                                       */
/*                                                                    */
/**********************************************************************/
#if !defined(_HAL_ATOMIC64_H)
#define  _HAL_ATOMIC64_H 

#include <klib/freestanding.h>
#include <kern/kern-types.h>

struct _atomic64;
/*
 * 64bit アトミック操作
 */
atomic64_val hal_atomic64_add_fetch(struct _atomic64 *_val, atomic64_val _incr);
atomic64_val hal_atomic64_and_fetch(struct _atomic64 *_val, atomic64_val _v);
atomic64_val hal_atomic64_or_fetch(struct _atomic64 *_val, atomic64_val _v);
atomic64_val hal_atomic64_xor_fetch(struct _atomic64 *_val, atomic64_val _v);
atomic64_val hal_atomic64_set_fetch(struct _atomic64 *_val, atomic64_val _set_to);
atomic64_val hal_atomic64_cmpxchg_fetch(struct _atomic64 *_val, atomic64_val _old, atomic64_val _new);
void *hal_atomic64_cmpxchg_ptr_fetch(void **_valp, void *_old, void *_new);

#endif  /*  _HAL_ATOMIC64_H   */

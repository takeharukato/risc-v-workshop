/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  RISC-V 64 atomic operations                                       */
/*                                                                    */
/**********************************************************************/
#if !defined(_HAL_ATOMIC_H)
#define  _HAL_ATOMIC_H 

#include <klib/freestanding.h>
#include <kern/kern-types.h>

struct _atomic;
/*
 * 32bit アトミック操作
 */
atomic_val hal_atomic_add_fetch(struct _atomic *_val, atomic_val _incr);
atomic_val hal_atomic_and_fetch(struct _atomic *_val, atomic_val _v);
atomic_val hal_atomic_or_fetch(struct _atomic *_val, atomic_val _v);
atomic_val hal_atomic_xor_fetch(struct _atomic *_val, atomic_val _v);
atomic_val hal_atomic_set_fetch(struct _atomic *_val, atomic_val _set_to);
atomic_val hal_atomic_cmpxchg_fetch(struct _atomic *_val, atomic_val _old, atomic_val _new);
void *hal_atomic_cmpxchg_ptr_fetch(void **_valp, void *_old, void *_new);

#endif  /*  _HAL_ATOMIC_H   */

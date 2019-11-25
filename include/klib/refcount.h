/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Reference counter relevant definitions                            */
/*                                                                    */
/**********************************************************************/
#if !defined(_KERN_REFCOUNT_H)
#define  _KERN_REFCOUNT_H 

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <kern/kern-types.h>
#include <klib/atomic.h>

#define REFCNT_INITIAL_VAL          (1)   /**<  参照カウンタの初期値  */

/** 参照カウンタ
 */
typedef struct _refcounter{
	atomic                 counter;  /**< 参照カウンタ                          */
}refcounter;

/** リファレンスカウンタの初期化子
 */
#define __REFCNT_INITIALIZER {						\
		.counter = __ATOMIC_INITIALIZER(REFCNT_INITIAL_VAL),    \
	}

struct _mutex;
struct _spinlock;

void refcnt_init_with_value(struct _refcounter *_counterp, refcounter_val _v);
void refcnt_init(struct _refcounter *_counterp);
void refcnt_set(struct _refcounter *_counterp, refcounter_val _v);
refcounter_val refcnt_read(struct _refcounter *_counterp);

refcounter_val refcnt_add_if_valid(struct _refcounter *_counterp, refcounter_val _v);
refcounter_val refcnt_add(struct _refcounter *_counterp, refcounter_val _v);

refcounter_val refcnt_inc_if_valid(struct _refcounter *_counterp);
refcounter_val refcnt_inc(struct _refcounter *_counterp);

bool refcnt_sub_and_test(struct _refcounter *_counterp, refcounter_val _v);
bool refcnt_dec_and_test(struct _refcounter *_counterp);
refcounter_val refcnt_dec(struct _refcounter *_counterp);

bool refcnt_dec_and_mutex_lock(struct _refcounter *_counterp, struct _mutex *_mtx);
bool refcnt_dec_and_lock(struct _refcounter *_counterp, struct _spinlock *_lock);
bool refcnt_dec_and_lock_disable_intr(struct _refcounter *_counterp, 
    struct _spinlock *_lock, intrflags *_iflags);

int  refcnt_get(struct _refcounter *counterp, refcounter_val *valp);
int  refcnt_put(struct _refcounter *counterp, refcounter_val *valp);

#endif  /*  _KERN_REFCOUNT_H  */

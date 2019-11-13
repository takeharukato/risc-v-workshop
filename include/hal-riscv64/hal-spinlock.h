/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  x64 spinlock                                                      */
/*                                                                    */
/**********************************************************************/
#if !defined(_HAL_SPINLOCK_H)
#define  _HAL_SPINLOCK_H 

#include <klib/freestanding.h>
#include <kern/kern-common.h>

struct _spinlock;

void hal_spinlock_lock(struct _spinlock *_lock);
void hal_spinlock_unlock(struct _spinlock *_lock);

#endif  /*  _HAL_SPINLOCK_H   */

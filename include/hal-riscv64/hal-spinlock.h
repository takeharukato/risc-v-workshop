/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  RISC-V 64 spinlock                                                */
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

/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  spinlock definitions                                              */
/*                                                                    */
/**********************************************************************/
#if !defined(_KERN_SPINLOCK_H)
#define  _KERN_SPINLOCK_H 

#include <klib/freestanding.h>

#include <kern/kern-types.h>
#include <klib/errno.h>

#include <klib/backtrace.h>  /*  BACKTRACE_MAX_DEPTH  */
#include <kern/cpuintr.h>
#include <hal/hal-spinlock.h>

#define SPINLOCK_BT_DEPTH        BACKTRACE_MAX_DEPTH

/** ロック種別
 */
#define SPINLOCK_TYPE_NORMAL     (0x0)  /**< 通常のロック  */
#define SPINLOCK_TYPE_RECURSIVE  (0x1)  /**< 再帰ロック    */

struct _thread;

typedef struct _spinlock {
	uint32_t                    locked;  /**<  ロック変数             */
	uint32_t                      type;  /**<  ロック種別             */
	uint32_t                     depth;  /**<  ロックの深度           */
	struct _thread              *owner;  /**<  ロック獲得スレッド     */
	void *backtrace[SPINLOCK_BT_DEPTH];  /**<  バックトレース情報     */
}spinlock;

/**  スピンロック初期化子
 */
#define __SPINLOCK_INITIALIZER		 \
	{				 \
		.locked = 0,		 \
		.type  = SPINLOCK_TYPE_NORMAL, \
		.depth  = 0,             \
		.owner  = NULL,          \
	}

bool spinlock_locked_by_self(struct _spinlock *_lock);
void spinlock_init(struct _spinlock *_lock);

void spinlock_raw_lock(struct _spinlock *_lock);
void spinlock_raw_unlock(struct _spinlock *_lock);

void spinlock_lock(struct _spinlock *_lock);
void spinlock_unlock(struct _spinlock *_lock);

void spinlock_raw_lock_disable_intr(struct _spinlock *_lock, intrflags *_iflags);
void spinlock_raw_unlock_restore_intr(struct _spinlock *_lock, intrflags *_iflags);

void spinlock_lock_disable_intr(struct _spinlock *_lock, intrflags *_iflags);
void spinlock_unlock_restore_intr(struct _spinlock *_lock, intrflags *_iflags);

#endif  /*  _KERN_SPINLOCK_H   */

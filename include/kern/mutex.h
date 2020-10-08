/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  kernel mutex definitions                                          */
/*                                                                    */
/**********************************************************************/
#if !defined(_KERN_MUTEX_H)
#define  _KERN_MUTEX_H

#include <klib/freestanding.h>
#include <kern/kern-types.h>

#include <kern/spinlock.h>
#include <kern/wqueue.h>
#include <klib/queue.h>
#include <klib/list.h>

#define MUTEX_INITIAL_VALUE      (1)  /* カウンタの初期値  */

struct _thread;

typedef uint32_t mutex_counter;  /* ミューテックスカウンタ */

/**
   カーネル内ミューテックス
 */
typedef struct _mutex{
	struct _spinlock       lock; /*< カウンタのロック           */
	struct _thread       *owner; /*< ミューテックス獲得スレッド */
	struct _wque_waitqueue wque; /*< ウエイトキュー             */
	mutex_counter     resources; /*< 利用可能資源数 (単位:個)   */
}mutex;

/**
   ミューテックス初期化子
   @param _mtx ミューテックスへのポインタ
 */
#define __MUTEX_INITIALIZER(_mtx) {		               \
	.lock = __SPINLOCK_INITIALIZER,		               \
	.owner  = NULL,	                                       \
	.wque = __WQUE_WAITQUEUE_INITIALIZER(&((_mtx)->wque)), \
	.resources = MUTEX_INITIAL_VALUE,                      \
}

void mutex_init(struct _mutex *_mtx);
void mutex_destroy(struct _mutex *_mtx);
bool mutex_locked_by_self(struct _mutex *_mtx);
int mutex_try_lock(struct _mutex *_mtx);
int mutex_lock(struct _mutex *_mtx);
void mutex_unlock(struct _mutex *mtx);
#endif  /* _KERN_MUTEX_H */

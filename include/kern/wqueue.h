/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Wait queue definitions                                            */
/*                                                                    */
/**********************************************************************/
#if !defined(_KERN_WQUEUE_H)
#define  _KERN_WQUEUE_H

#include <klib/freestanding.h>
#include <kern/kern-types.h>
#include <kern/spinlock.h>
#include <klib/queue.h>
#include <klib/list.h>

struct _thread;
struct  _mutex;

/** スレッド起床方針
 */
typedef enum _wque_wakeflag{
	WQUE_WAKEFLAG_ALL = 0,  /*<  待機中の全スレッドを起こす        */
	WQUE_WAKEFLAG_ONE = 1,  /*<  キューの先頭スレッドだけを起こす  */
}wque_wakeflag;

/** 起床要因
 */
typedef enum _wque_reason{
	WQUE_WAIT = 0,       /*<  待ち状態                       */
	WQUE_RELEASED = 1,   /*<  待ち状態が解除された           */
	WQUE_DESTROYED = 2,  /*<  対象オブジェクトが破棄された   */
	WQUE_TIMEOUT = 3,    /*<  タイムアウト                   */
	WQUE_DELIVEV = 4,    /*<  イベント受信                   */
}wque_reason;

/** ウエイトキュー
 */
typedef struct _wque_waitqueue{
	struct _spinlock      lock; /*< スピンロック         */
	struct _queue          que; /*< ウエイトキュー       */
	struct _queue     prio_que; /*< 優先度継承管理キュー */
	wque_wakeflag       wqflag; /*< 起床方法             */
}wque_waitqueue;

/** ウエイトキュー初期化子
   @param[in] _wque ウエイトキューへのポインタ
 */
#define __WQUE_WAITQUEUE_INITIALIZER(_wque) {		        \
	.lock = __SPINLOCK_INITIALIZER,		                \
	.que  =	__QUEUE_INITIALIZER(&((_wque)->que)),	        \
	.prio_que  = __QUEUE_INITIALIZER(&((_wque)->prio_que)),	\
	.wqflag = WQUE_WAKEFLAG_ALL,                            \
	}

/** ウエイトキューエントリ
 */
typedef struct _wque_entry{
	struct _list    link;   /*< ウエイトキューへのリンク    */
	struct _list prilink;   /*< 優先度順キューへのリンク    */
	struct _thread  *thr;   /*< 休眠しているスレッド        */
	wque_reason   reason;   /*< 起床要因                    */
}wque_entry;

void wque_init_wait_queue(struct _wque_waitqueue *_wque);
bool wque_is_empty(struct _wque_waitqueue *_wque);

void wque_init_wque_entry(struct _wque_entry *_ent);
wque_reason wque_wait_for_curthr(struct _wque_waitqueue *_wque);
wque_reason wque_wait_on_queue_with_spinlock(struct _wque_waitqueue *_wque,
    struct _spinlock *_lock);
wque_reason wque_wait_on_event_with_mutex(struct _wque_waitqueue *_wque, struct _mutex *_mtx);

void wque_wakeup(struct _wque_waitqueue *_wque, wque_reason _reason);

#endif  /*  _KERN_WQUEUE_H   */

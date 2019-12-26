/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Thread relevant definitions                                       */
/*                                                                    */
/**********************************************************************/
#if !defined(_KERN_THR_THREAD_H)
#define  _KERN_THR_THREAD_H

#if !defined(ASM_FILE)

#include <klib/freestanding.h>
#include <kern/kern-consts.h>
#include <kern/kern-types.h>
#include <kern/spinlock.h>
#include <kern/wqueue.h>

#include <klib/refcount.h>
#include <klib/list.h>
#include <klib/rbtree.h>
#include <klib/bitops.h>

#include <hal/hal-thread.h>

/**
   スレッドの属性
 */
#define THR_THRFLAGS_KERNEL       (0)  /**< カーネルスレッド                            */
#define THR_THRFLAGS_USER         (1)  /**< ユーザスレッド                              */
#define THR_THRFLAGS_MANAGED_STK  (2)  /**< カーネルスタックからページプールから割当て  */
/**
   スレッドID
 */
#define THR_TID_MAX               (~(UINT64_C(0)))   /**< スレッドID総数   */
#define THR_TID_RSV_ID_NR         (32)               /**< 予約ID数         */
#define THR_TID_INVALID           (THR_TID_MAX)      /**< 不正スレッドID   */

struct _thread_info;

/**
   スレッドの状態
 */
typedef enum _thr_state{
	THR_TSTATE_RUN  = 0,      /**<  実行中     */
	THR_TSTATE_RUNABLE  = 1,  /**<  実行可能   */
	THR_TSTATE_WAIT = 2,      /**<  待ち合せ中 */
	THR_TSTATE_EXIT = 3,      /**<  終了処理中 */
	THR_TSTATE_DEAD = 4,      /**<  回収待ち中 */
}thr_state;

/**
   スレッドの属性情報
 */
typedef struct _thread_attr{
	void         *kstack_top;  /**< カーネルスタック開始アドレス                 */
	void             *kstack;  /**< ディスパッチ後のカーネルスタック値           */
	thr_prio        ini_prio;  /**< 初期化時スレッド優先度                       */
	thr_prio       base_prio;  /**< ベーススレッド優先度                         */
	thr_prio        cur_prio;  /**< 現在のスレッド優先度                         */
}thread_attr;

/**
   スレッド管理情報
 */
typedef struct _thread{
	spinlock                   lock;  /**< スレッド管理情報のロック     */
	thr_flags                 flags;  /**< スレッドの属性               */
	thr_state                 state;  /**< スレッドの状態               */
	tid                          id;  /**< Thread ID                    */
	RB_ENTRY(_thread)           ent;  /**< スレッド管理ツリーのエントリ */
	refcounter                 refs;  /**< 参照カウンタ                 */
	struct _list               link;  /**< スレッドキューへのリンク     */
	struct _thread_info      *tinfo;  /**< スレッド情報へのポインタ     */
	struct _thread_attr        attr;  /**< スレッド属性                 */
	struct _thread          *parent;  /**< 親スレッド                   */
	struct _wque_waitqueue     wque;  /**< wait待ち合せ中子スレッド     */
	exit_code              exitcode;  /**< 終了コード                   */
}thread;

/**
   スレッド管理DB
 */
typedef struct _thread_db{
	spinlock                               lock;  /**< スレッドDBのロック             */
	BITMAP_TYPE(, uint64_t, THR_TID_MAX)  idmap;  /**< 利用可能スレッドIDビットマップ */
	RB_HEAD(_thrdb_tree, _thread)          head;  /**< スレッドDB                     */
}thread_db;

/**
   スレッド管理DB初期化子
 */
#define __THRDB_INITIALIZER(thrdb) {		                \
		.lock = __SPINLOCK_INITIALIZER,		        \
		.head  = RB_INITIALIZER(&(thrdb)->head),	\
	}

/**
   有効なスレッドであることを確認する
   @param[in] _thr スレッド管理情報
   @retval    真   有効なスレッドである
   @retval    偽   終了しようとしているスレッドである
 */
#define thr_thread_is_active(_thr)			\
	( ( ( (_thr)->state ) != THR_TSTATE_EXIT ) &&	\
	  ( ( (_thr)->state ) != THR_TSTATE_DEAD ) )

int thr_thread_create(tid _id, entry_addr _entry, void *_usp, void *_kstktop, thr_prio _prio, 
		      thr_flags _flags, struct _thread **_thrp);
void thr_thread_switch(struct _thread *prev, struct _thread *next);
void thr_init(void);
#endif  /*  !ASM_FILE */
#endif  /*  _KERN_THR_THREAD_H  */

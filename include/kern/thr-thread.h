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

struct _thread_info;

/**
   スレッドの状態
 */
typedef enum _thr_state{
	THR_TSTATE_RUN  = 0,  /**<  実行中     */
	THR_TSTATE_RUNABLE  = 1,  /**<  実行中     */
	THR_TSTATE_WAIT = 2,  /**<  待ち合せ中 */
	THR_TSTATE_EXIT = 3,  /**<  終了処理中 */
	THR_TSTATE_DEAD = 4,  /**<  回収待ち中 */
}thr_state;

/**
   スレッドの属性情報
 */
typedef struct _thread_attr{
	void         *kstack_top;  /**< カーネルスタック開始アドレス                 */
	void             *kstack;  /**< ディスパッチ後のカーネルスタック値           */
	size_t       kstack_size;  /**< カーネルスタックサイズ (単位:バイト)         */
	thr_prio        ini_prio;  /**< 初期化時スレッド優先度                       */
	thr_prio       base_prio;  /**< ベーススレッド優先度                         */
	thr_prio        cur_prio;  /**< 現在のスレッド優先度                         */
}thread_attr;

/**
   スレッド管理情報
 */
typedef struct _thread{
	spinlock                   lock;  /**< スレッド管理情報のロック     */
	thr_state                 state;  /**< スレッドの状態               */
	tid                         tid;  /**< Thread ID                    */
	RB_ENTRY(_thread)           ent;  /**< スレッド管理ツリーのエントリ */
	refcounter                 refs;  /**< 参照カウンタ                 */
	struct _list               link;  /**< スレッドキューへのリンク     */
	struct _thread_info      *tinfo;  /**< スレッド情報へのポインタ     */
	struct _thread_attr        attr;  /**< スレッド属性                 */
	struct _thread          *parent;  /**< 親スレッド                   */
	struct _wque_waitqueue chldwque;  /**< wait待ち合せ中子スレッド     */
	exit_code              exitcode;  /**< 終了コード                   */
}thread;

/**
   スレッド管理DB
 */
typedef struct _thread_db{
	spinlock                         lock;  /**< スレッドDBのロック     */
	RB_HEAD(_threaddb_tree, _thread) root;  /**< スレッドDB             */
}thread_db;
#endif  /*  !ASM_FILE */
#endif  /*  _KERN_THR_THREAD_H  */

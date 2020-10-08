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
#include <kern/sched-queue.h>

#include <klib/refcount.h>
#include <klib/list.h>
#include <klib/rbtree.h>
#include <klib/bitops.h>

#include <hal/hal-thread.h>

/**
   スレッドの属性
 */
#define THR_THRFLAGS_KERNEL        (0)  /**< カーネルスレッド                          */
#define THR_THRFLAGS_USER        (0x1)  /**< ユーザスレッド                            */
#define THR_THRFLAGS_MANAGED_STK (0x2)  /**< カーネルスタックをページプールから割当て  */
#define THR_THRFLAGS_IDLE        (0x4)  /**< アイドルスレッド                          */
/**
   スレッドID/優先度
 */
#define THR_TID_MAX               (KC_THR_MAX)        /**< スレッドID総数   */
#define THR_TID_RSV_ID_NR         (ULONGLONG_C(32))   /**< 予約ID数         */
#define THR_TID_INVALID           (~(ULONGLONG_C(0))) /**< 不正スレッドID   */
#define THR_TID_AUTO              THR_TID_INVALID     /**< ID自動割り当て   */

#define THR_TID_IDLE              (ULONGLONG_C(0))          /**< アイドルスレッドのスレッドID */
#define THR_TID_REAPER            (ULONGLONG_C(2))          /**< 刈り取りスレッドのスレッドID */
#define THR_PRIO_REAPER           (SCHED_MAX_SYS_PRIO - 1)  /**< 刈り取りスレッドの優先度 */

struct _thread_info;
struct _proc;

/**
   スレッドの状態
 */
typedef enum _thr_state{
	THR_TSTATE_DORMANT = 0,   /**<  生成済み   */
	THR_TSTATE_RUN  = 1,      /**<  実行中     */
	THR_TSTATE_RUNABLE  = 2,  /**<  実行可能   */
	THR_TSTATE_WAIT = 3,      /**<  待ち合せ中 */
	THR_TSTATE_EXIT = 4,      /**<  終了処理中 */
	THR_TSTATE_DEAD = 5,      /**<  回収待ち中 */
}thr_state;

/**
   thr_thread_wait処理返却情報
 */
typedef struct _thr_wait_res{
	tid             id;  /**< スレッドID  */
	exit_code exitcode;  /**< 終了コード  */
}thr_wait_res;

/** wait待ちスレッドキューのエントリ
 */
typedef struct _thr_wait_entry{
	struct _list    link;   /*< キューへのリンク         */
	struct _thread  *thr;   /*< 待ち合わせ中の子スレッド */
}thr_wait_entry;

/**
   スレッドの属性情報
 */
typedef struct _thread_attr{
	void         *kstack_top;  /**< カーネルスタック開始アドレス                 */
	thr_prio        ini_prio;  /**< 初期化時スレッド優先度                       */
	thr_prio       base_prio;  /**< ベーススレッド優先度                         */
	thr_prio        cur_prio;  /**< 現在のスレッド優先度                         */
}thread_attr;

/**
   スレッド管理情報
 */
typedef struct _thread{
	spinlock                   lock;  /**< スレッド管理情報のロック           */
	refcounter                 refs;  /**< 参照カウンタ                       */
	RB_ENTRY(_thread)           ent;  /**< スレッド管理ツリーのエントリ       */
	thr_flags                 flags;  /**< スレッドの属性                     */
	thr_state                 state;  /**< スレッドの状態                     */
	tid                          id;  /**< Thread ID                          */
	struct _proc                 *p;  /**< プロセス管理情報へのポインタ       */
	void                       *ksp;  /**< スレッドコンテキスト               */
	struct _thread_info      *tinfo;  /**< スレッド情報へのポインタ           */
	struct _list               link;  /**< スケジューラキュー/wait待ちへのリンク  */
	struct _list          proc_link;  /**< プロセス管理情報のリンク           */
	struct _list      children_link;  /**< 親スレッドのchildrenキューのリンク */
	struct _thread_attr        attr;  /**< スレッド属性                       */
	struct _thread          *parent;  /**< 親スレッド                         */
	struct _queue          children;  /**< 子スレッド                         */
	struct _queue           waiters;  /**< wait待ち合わせ中の子スレッド       */
	struct _wque_waitqueue     pque;  /**< wait待ち合せ中親スレッド           */
	exit_code              exitcode;  /**< 終了コード                         */
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
   引数情報
 */
typedef struct _thread_args{
	reg_type  arg1;  /**< 第1引数 */
	reg_type  arg2;  /**< 第2引数 */
	reg_type  arg3;  /**< 第3引数 */
	reg_type  arg4;  /**< 第4引数 */
	reg_type  arg5;  /**< 第5引数 */
	reg_type  arg6;  /**< 第6引数 */
}thread_args;

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

int thr_thread_wait(struct _thr_wait_res *_resp);
void thr_thread_exit(exit_code _ec);
int thr_user_thread_create(tid _id, entry_addr _entry, struct _thread_args *_args,
    struct _proc *_p, void *_usp, thr_prio _prio, thr_flags _flags, struct _thread **_thrp);
int thr_kernel_thread_create(tid _id, entry_addr _entry, struct _thread_args *_args,
    thr_prio _prio, thr_flags _flags, thread **_thrp);
void thr_thread_switch(struct _thread *_prev, struct _thread *_next);
bool thr_ref_dec(struct _thread *_thr);
bool thr_ref_inc(struct _thread *_thr);
void thr_idle_loop(void *_arg);
int thr_idlethread_create(cpu_id _cpu, thread **_thrp);
void thr_system_thread_create(void);
void thr_init(void);
#endif  /*  !ASM_FILE */
#endif  /*  _KERN_THR_THREAD_H  */

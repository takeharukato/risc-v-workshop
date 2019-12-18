/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  scheduler queue definitions                                       */
/*                                                                    */
/**********************************************************************/
#if !defined(_KERN_SCHED_QUEUE_H)
#define  _KERN_SCHED_QUEUE_H 

/*
 * 優先度定義 (小さい数値ほど優先度が高い)
 */

/**< 各スケジューリング方針毎の優先度数 */
#define SCHED_PRIO_PER_POLICY   (64)

/**< 割込みスレッドクラスの最高優先度   */
#define SCHED_MAX_ITHR_PRIO     (0)
/**< 割込みスレッドクラスの最低優先度   */
#define SCHED_MIN_ITHR_PRIO     ( SCHED_MIN_ITHR_PRIO + SCHED_PRIO_PER_POLICY )

/**< システムスレッドの最高優先度 */
#define SCHED_MAX_SYS_PRIO      ( SCHED_MIN_ITHR_PRIO )
/**< システムスレッドの最低優先度 */
#define SCHED_MIN_SYS_PRIO      ( SCHED_MAX_SYS_PRIO + SCHED_PRIO_PER_POLICY )

/**< First Come First Servedクラスの最高優先度   */
#define SCHED_MAX_FCFS_PRIO     ( SCHED_MIN_SYS_PRIO )
/**< First Come First Servedクラスの最低優先度   */
#define SCHED_MIN_FCFS_PRIO     ( SCHED_MAX_FCFS_PRIO + SCHED_PRIO_PER_POLICY )

/**< ラウンドロビンクラスの最低優先度   */
#define SCHED_MAX_RR_PRIO       ( SCHED_MIN_FCFS_PRIO )
/**< ラウンドロビンクラスの最高優先度   */
#define SCHED_MIN_RR_PRIO       ( SCHED_MAX_RR_PRIO + SCHED_PRIO_PER_POLICY )

/* ユーザスレッドが動作しうる最低優先度 */
#define SCHED_MIN_USER_PRIO     (SCHED_MIN_RR_PRIO)
/* ユーザスレッドが動作しうる最高優先度 */
#define SCHED_MAX_USER_PRIO     (SCHED_MAX_FCFS_PRIO)

/* スレッドの最低優先度 */
#define SCHED_MIN_PRIO          (SCHED_MIN_USER_PRIO)
/* スレッドの最高優先度 */
#define SCHED_MAX_PRIO          (SCHED_MAX_ITHR_PRIO)

#if !defined(ASM_FILE)
#include <klib/freestanding.h>
#include <kern/kern-types.h>
#include <kern/spinlock.h>
#include <klib/queue.h>
#include <klib/list.h>
#include <klib/bitops.h>

struct _thread;

/**
   スケジューラのレディキューへのエントリ
 */
typedef struct _sched_readyqueue_ent{
	struct _list   link;  /**< スケジューラキューへのリンク */
	struct _thread *thr;  /**< スレッドへのポインタ         */
}sched_queue_ent;

/**
   スケジューラレディーキュー
   @note キュー間での移動があるのでスケジューラキューのロックを獲得して操作する
 */
typedef struct _sched_readyqueue{
	struct _queue  que;  /**< スケジューラエントリキュー */
}sched_readyqueue;

/**
   スケジューラキュー
 */
typedef struct _sched_queue{
	spinlock                                   lock;  /**< スケジューラキューのロック */
	struct _sched_readyqueue    que[SCHED_MAX_PRIO];  /**< スケジューラキュー         */
	BITMAP_TYPE(, uint64_t, SCHED_MAX_PRIO)  bitmap;  /**< スケジューラビットマップ   */
}sched_queue;
void sched_init(void);
#endif  /*  !ASM_FILE  */
#endif  /*  _KERN_SCHED_IF_H   */

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
#define SCHED_MIN_ITHR_PRIO     ( SCHED_MAX_ITHR_PRIO + SCHED_PRIO_PER_POLICY - 1 )

/**< システムスレッドの最高優先度 */
#define SCHED_MAX_SYS_PRIO      ( SCHED_MIN_ITHR_PRIO )
/**< システムスレッドの最低優先度 */
#define SCHED_MIN_SYS_PRIO      ( SCHED_MAX_SYS_PRIO + SCHED_PRIO_PER_POLICY - 1 )

/**< First Come First Servedクラスの最高優先度   */
#define SCHED_MAX_FCFS_PRIO     ( SCHED_MIN_SYS_PRIO )
/**< First Come First Servedクラスの最低優先度   */
#define SCHED_MIN_FCFS_PRIO     ( SCHED_MAX_FCFS_PRIO + SCHED_PRIO_PER_POLICY - 1)

/**< ラウンドロビンクラスの最低優先度   */
#define SCHED_MAX_RR_PRIO       ( SCHED_MIN_FCFS_PRIO )
/**< ラウンドロビンクラスの最高優先度   */
#define SCHED_MIN_RR_PRIO       ( SCHED_MAX_RR_PRIO + SCHED_PRIO_PER_POLICY - 1)

/* ユーザスレッドが動作しうる最低優先度 */
#define SCHED_MIN_USER_PRIO     (SCHED_MIN_RR_PRIO)
/* ユーザスレッドが動作しうる最高優先度 */
#define SCHED_MAX_USER_PRIO     (SCHED_MAX_FCFS_PRIO)

/* スレッドの最低優先度 */
#define SCHED_MIN_PRIO          (SCHED_MIN_USER_PRIO)
/* スレッドの最高優先度 */
#define SCHED_MAX_PRIO          (SCHED_MAX_ITHR_PRIO)

/**
   プロセスの優先度として有効であることを確認する
   @param[in] _prio 優先度
   @retval 真 プロセスの優先度として有効である
   @retval 偽 不正な優先度である
 */
#define SCHED_VALID_PRIO(_prio) \
	( ( SCHED_MAX_PRIO <= (_prio) ) && ( (_prio) <= SCHED_MIN_PRIO  ) )

/**
   ユーザプロセスの優先度として有効であることを確認する
   @param[in] _prio 優先度
   @retval 真 ユーザプロセスの優先度として有効である
   @retval 偽 不正な優先度である
 */
#define SCHED_VALID_USER_PRIO(_prio) \
	( ( SCHED_MAX_USER_PRIO <= (_prio) ) && ( ( (_prio) <= SCHED_MIN_USER_PRIO )  ) )

#if !defined(ASM_FILE)
#include <klib/freestanding.h>
#include <kern/kern-types.h>
#include <kern/spinlock.h>
#include <klib/queue.h>
#include <klib/list.h>
#include <klib/bitops.h>

struct _thread;

/**
   スケジューラキュー
 */
typedef struct _sched_queue{
	spinlock                                   lock;  /**< スケジューラキューのロック */
	struct _queue               que[SCHED_MAX_PRIO];  /**< スケジューラキュー         */
	BITMAP_TYPE(, uint64_t, SCHED_MAX_PRIO)  bitmap;  /**< スケジューラビットマップ   */
}sched_queue;

void sched_thread_add(struct _thread *_thr);
void sched_thread_del(struct _thread *_thr);
void sched_schedule(void);
bool sched_delay_disptach(void);
void sched_idlethread_add(void);
void sched_init(void);
#endif  /*  !ASM_FILE  */
#endif  /*  _KERN_SCHED_IF_H   */

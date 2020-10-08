/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Timer definitions                                                 */
/*                                                                    */
/**********************************************************************/

#if !defined(_KERN_TIMER_H)
#define  _KERN_TIMER_H

#if !defined(ASM_FILE)
#include <klib/freestanding.h>
#include <kern/kern-autoconf.h>
#include <kern/kern-types.h>
#include <kern/spinlock.h>
#include <kern/timer-if.h>

#include <klib/klib-consts.h>
#include <klib/queue.h>
#include <klib/list.h>

struct _trap_context;
struct _ktimespec;

/**
   システム時刻情報
 */
typedef struct _system_timer{
	spinlock             lock;  /**< 排他用ロック                                       */
	struct _ktimespec curtime;  /**< 現在時刻                                           */
	struct _queue        head;  /**< コールアウトキューのヘッド */
}system_timer;

/**
   システムタイマの初期化子
   @param[in] _walltime システム時刻情報へのポインタ
 */
#define __SYSTEM_TIMER_INITIALIZER(_walltime)   {	\
	.lock = __SPINLOCK_INITIALIZER,		\
	.curtime   = __KTIMESPEC_INITIALIZER,   \
	.head = __QUEUE_INITIALIZER(&((_walltime)->head)), \
	}

void tim_update_walltime(struct _trap_context *_ctx, struct _ktimespec *_diff);
void tim_timer_init(void);

int hal_read_rtc(struct _ktimespec *_ktsp);
#endif  /*  ASM_FILE */
#endif  /*  _KERN_TIMER_H   */

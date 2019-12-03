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
#include <kern/kern_types.h>
#include <kern/spinlock.h>

#include <klib/klib-consts.h>
#include <klib/queue.h>
#include <klib/list.h>

/**
   システム時間情報
 */
typedef struct _system_timer{
	spinlock            lock;  /**< 排他用ロック                                       */
	hwtimer_counter  hwcount;  /**< 電源投入時からのハードウエアタイマの累積加算値     */
	ticks              ticks;  /**< 電源投入時からのティック発生回数                   */
	uptime_counter    uptime;  /**< 起動後の時間 (単位:タイムスライス単位での経過時間) */
}system_timer;

/**
   コールアウトキュー
 */
typedef struct _call_out_que{
	spinlock         lock;  /**< 排他用ロック   */
	struct _queue    head;  /**< キューのヘッド */
}call_out_que;

/**
   コールアウトエントリ
 */
typedef struct _call_out_ent{
	struct _list               link;  /**< コールアウトキューへのリンク     */
	uptime_counter           expire;  /**< コールアウト時間 (uptime単位)    */
	void (*callout)(void *_private);  /**< コールアウト関数                 */
	void                   *private;  /**< コールアウト関数プライベート情報 */
}call_out_ent;

#define __SYSTEM_TIMER_INITIALIZER   {		\
	.lock = __SPINLOCK_INITIALIZER,		\
	.hwcount = 0,			        \
	.ticks   = 0,			        \
	.uptime  = 0,			        \
	}


void timer_update_uptime(void);
void timer_update_thread_time(void);
void callout_ent_init(call_out_ent *_entp, void (*_callout)(void *_private), void *_private);
int callout_add(call_out_ent *_entp, uptime_cnt _rel_expire);
void callout_init(void);
#endif  /*  ASM_FILE */
#endif  /*  _KERN_TIMER_H   */

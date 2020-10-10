/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Timer Interface definitions                                       */
/*                                                                    */
/**********************************************************************/

#if !defined(_KERN_TIMER_IF_H)
#define  _KERN_TIMER_IF_H

#if !defined(ASM_FILE)
#include <klib/freestanding.h>
#include <kern/kern-autoconf.h>
#include <kern/kern-types.h>
#include <kern/spinlock.h>

#include <klib/klib-consts.h>
#include <klib/queue.h>
#include <klib/list.h>

struct _trap_context;

/**
   カーネル内timespec
 */
typedef struct _ktimespec{
	epoch_time    tv_sec;  /**< 秒      */
	long         tv_nsec;  /**< ナノ秒  */
}ktimespec;

/**
   コールアウト関数定義
   @param[in] _ctx     割込みコンテキスト
   @param[in] _private プライベート情報
 */
typedef void (*tim_callout_type)(struct _trap_context *_ctx, void *_private);

/**
   コールアウトエントリ
 */
typedef struct _call_out_ent{
	struct _list               link;  /**< コールアウトキューへのリンク     */
	struct _ktimespec        expire;  /**< コールアウト時間 (timespec単位)  */
	tim_callout_type        callout;  /**< コールアウト関数 */
	void                   *private;  /**< コールアウト関数プライベート情報 */
}call_out_ent;

/**
   カーネルtimespecの初期化子
 */
#define __KTIMESPEC_INITIALIZER   {	\
		.tv_sec = 0,		\
		.tv_nsec = 0,	        \
	}

#if defined(CONFIG_TIMER_INTERVAL_MS_1MS)
#define MS_PER_TICKS       (1)
/**
   ミリ秒をjiffiesに変換する
   @param[in] _ms ミリ秒
   @return jiffies値
 */
#define MS_TO_JIFFIES(_ms) ((_ms))
#elif defined(CONFIG_TIMER_INTERVAL_MS_10MS)
#define MS_PER_TICKS       (10)
/**
   ミリ秒をjiffiesに変換する
   @param[in] _ms ミリ秒
   @return jiffies値
 */
#define MS_TO_JIFFIES(_ms) (roundup_align((_ms), 10)/10)
#elif defined(CONFIG_TIMER_INTERVAL_MS_100MS)
#define MS_PER_TICKS       (100)
/**
   ミリ秒をjiffiesに変換する
   @param[in] _ms ミリ秒
   @return jiffies値
 */
#define MS_TO_JIFFIES(_ms) (roundup_align((_ms), 100)/100)
#else
#error "Invalid timer interval"
#endif  /*  CONFIG_TIMER_INTERVAL_MS_1MS  */

int tim_callout_ref(struct _ktimespec *_ts);
void tim_walltime_get(struct _ktimespec *_ktsp);
int tim_callout_add(tim_tmout _rel_expire_ms, tim_callout_type _callout, void *_private,
		    struct _call_out_ent **_entp);
int tim_callout_cancel(struct _call_out_ent *_ent);

#endif  /*  ASM_FILE */
#endif  /*  _KERN_TIMER_IF_H   */

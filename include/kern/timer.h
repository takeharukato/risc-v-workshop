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

#include <klib/klib-consts.h>
#include <klib/queue.h>
#include <klib/list.h>

/**
   カーネル内timespec
 */
typedef struct _ktimespec{
	epoch_time  tv_sec;  /**< 秒      */
	long       tv_nsec;  /**< ナノ秒  */
}ktimespec;

/**
   システム時間情報
 */
typedef struct _system_timer{
	spinlock             lock;  /**< 排他用ロック                                       */
	hwtimer_counter   hwcount;  /**< 電源投入時からのハードウエアタイマの累積加算値     */
	uptime_counter     uptime;  /**< 起動後の時間 (単位:ティック発生回数)               */
	struct _ktimespec curtime;  /**< 現在時刻                                           */
}system_timer;

/**
   コールアウトキュー
 */
typedef struct _call_out_que{
	spinlock         lock;  /**< 排他用ロック   */
	struct _queue    head;  /**< キューのヘッド */
}call_out_que;

/**
   コールアウト関数定義
 */
typedef void (*tim_callout_type)(void *_private);  

/**
   コールアウトエントリ
 */
typedef struct _call_out_ent{
	struct _list               link;  /**< コールアウトキューへのリンク     */
	uptime_counter           expire;  /**< コールアウト時間 (uptime単位)    */
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

/**
   システムタイマの初期化子
 */
#define __SYSTEM_TIMER_INITIALIZER   {		\
	.lock = __SPINLOCK_INITIALIZER,		\
	.hwcount = 0,			        \
	.uptime  = 0,			        \
	.curtime   = __KTIMESPEC_INITIALIZER,   \
	}
/**
   コールアウトキューの初期化子
   @param[in] _que コールアウトキューへのポインタ
 */
#define __CALLOUT_QUE_INITIALIZER(_coque)   {			\
		.lock = __SPINLOCK_INITIALIZER,			\
		.head = __QUEUE_INITIALIZER(&(_coque)->head),	\
	}

/**
   ミリ秒をjiffiesに変換する
   @param[in] _ms ミリ秒
   @return jiffies値
 */
#if defined(CONFIG_TIMER_INTERVAL_MS_1MS)
#define MS_TO_JIFFIES(_ms) ((_ms))
#elif defined(CONFIG_TIMER_INTERVAL_MS_10MS)
#define MS_TO_JIFFIES(_ms) (roundup_align((_ms), 10)/10)
#elif defined(CONFIG_TIMER_INTERVAL_MS_100MS)
#define MS_TO_JIFFIES(_ms) (roundup_align((_ms), 100)/100)
#else
#error "Invalid timer interval"
#endif
void tim_update_walltime(struct _ktimespec *_diff, uptime_counter _utimediff);
int tim_callout_add(tim_tmout _rel_expire_ms, tim_callout_type _callout, void *_private);
void tim_callout_init(void);
#endif  /*  ASM_FILE */
#endif  /*  _KERN_TIMER_H   */

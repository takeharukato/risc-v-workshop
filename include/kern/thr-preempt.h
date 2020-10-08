/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  preemption relevant definitions                                   */
/*                                                                    */
/**********************************************************************/
#if !defined(_KERN_THR_PREEMPT_H)
#define  _KERN_THR_PREEMPT_H

#include <kern/kern-consts.h>
#include <kern/page-macros.h>

/** ディスパッチ許可状態
 */
#define TI_PREEMPT_ACTIVE    (UINT32_C(0x80000000))  /**< ディスパッチ要求受付中  */

#define TI_DISPATCH_DELAYED  (UINT32_C(0x1))         /**< 遅延ディスパッチ        */
#define TI_EVENT_PENDING     (UINT32_C(0x2))         /**< 非同期イベント有り      */

/*
 * スレッド情報
 */
#define TI_MAGIC     (UINT32_C(0xabcdfeed))  /**< スレッド情報マジック番号          */
#define TI_KSTACK_SIZE       \
	( PAGE_SIZE << KC_KSTACK_ORDER ) /** カーネルスタックサイズ(単位:バイト  */

#if !defined(ASM_FILE)

#include <klib/freestanding.h>
#include <kern/kern-types.h>

#include <klib/misc.h>
#include <klib/stack.h>

struct _thread;

/** スレッド管理情報(スタック部分)
 */
typedef struct _thread_info{
	thread_info_magic      magic;   /**< スタックの底を表すマジック番号 */
	intr_depth           intrcnt;   /**< 割込み多重度                   */
	preempt_count        preempt;   /**< ディスパッチ禁止状態管理情報   */
	thread_info_flags      flags;   /**< 例外出口制御フラグ             */
	thread_info_flags    mdflags;   /**< アーキ固有の例外出口制御フラグ */
	cpu_id                   cpu;   /**< 論理CPU番号                    */
	void                 *kstack;   /**< カーネルスタックへのリンク     */
	struct _thread          *thr;   /**< スレッド情報への逆リンク       */
}thread_info;

#if !defined(_IN_ASM_OFFSET)

/**
   カーネルスタックの先頭からスレッド情報を得る
   @param[in] _kstk_top カーネルスタックの先頭アドレス
 */
#define calc_thread_info_from_kstack_top(_kstk_top)	\
	( (struct _thread_info *)					\
	    ( ( (void *)(_kstk_top) ) + TI_KSTACK_SIZE - sizeof(thread_info) ) )

void ti_show_thread_info(struct _thread_info *_ti);
struct _thread *ti_get_current_thread(void);
struct _thread_info *ti_get_current_thread_info(void);
cpu_id ti_current_cpu_get(void);
void ti_update_current_cpu(void);
void ti_thread_info_init(struct _thread_info *_ti, struct _thread *_thr);

bool ti_dispatch_disabled(void);
void ti_inc_preempt(void);
bool ti_dec_preempt(void);

bool ti_in_intr(void);
void ti_inc_intr(void);
bool ti_dec_intr(void);

bool ti_dispatch_delayed(void);
void ti_set_delay_dispatch(struct _thread_info *_ti);
void ti_clr_delay_dispatch(void);

bool ti_has_events(void);
void ti_set_events(struct _thread_info *_ti);
void ti_clr_events(void);

void ti_set_preempt_active(void);
void ti_clr_preempt_active(void);
#endif  /*  !_IN_ASM_OFFSET  */
#endif  /*  !ASM_FILE  */

#endif  /* _KERN_THR_PREEMPT_H */

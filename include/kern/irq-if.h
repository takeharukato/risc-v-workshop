/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  IRQ Management                                                    */
/*                                                                    */
/**********************************************************************/
#if !defined(_KERN_IRQ_IF_H)
#define  _KERN_IRQ_IF_H 

#include <klib/freestanding.h>
#include <kern/kern-types.h>

#include <klib/list.h>
#include <klib/queue.h>
#include <klib/rbtree.h>
#include <klib/refcount.h>

#include <kern/spinlock.h>

/*
 * 割込みハンドラ属性/割込み線の属性値
 */
#define IRQ_ATTR_EXCLUSIVE     (0x1)   /*< 割込み線共有不可                   */
#define IRQ_ATTR_SHARED        (0x0)   /*< 割込み線共有可能                   */
#define IRQ_ATTR_NESTABLE      (0x0)   /*< 多重割込み可能                     */
#define IRQ_ATTR_NON_NESTABLE  (0x2)   /*< 多重割込み不可能                   */
#define IRQ_ATTR_HANDLER_MASK  (0xf)   /*< 割込みハンドラ属性フラグのマスク   */
#define IRQ_ATTR_LEVEL         (0x00)  /*< レベルトリガ割込み                 */
#define IRQ_ATTR_EDGE          (0x10)  /*< エッジトリガ割込み                 */
#define IRQ_ATTR_TRIGGER_MASK  (0xf0)  /*< トリガマスク                       */

/* 割込み検出ハンドラの返値
 */
#define IRQ_FOUND              (0)  /*< 割込みが見つかった       */
#define IRQ_NOT_FOUND          (1)  /*< 割込みが見つからなかった */

/* 割込みハンドラの返値
 */
#define IRQ_HANDLED            (0)  /*< 割込みを処理した            */
#define IRQ_NOT_HANDLED        (1)  /*< 割込みを処理できなかった    */

/* 割込み線の数 (単位:個)
 */
#if defined(CONFIG_IRQ_NR)
#define NR_IRQS         (CONFIG_NR_IRQS)
#else
#define NR_IRQS         (256)
#endif  /*  CONFIG_IRQ_MAX_NR  */

struct _irq_ctrlr;
struct _trap_context;

/**
   割込みハンドラ定義
 */
typedef int (*irq_handler)(irq_no _irq, struct _trap_context *_ctx, void *_private);

/**
   割込みハンドラエントリ
 */
typedef struct _irq_handler_ent{
	struct _list    link; /**< 割込みハンドラキューのリストエントリ */
	refcounter      refs; /**< 参照カウンタ                         */
	irq_handler  handler; /**< 割込みハンドラ                       */
	void        *private; /**< ハンドラ固有情報へのポインタ         */
}irq_handler_ent;

/* 
 * 割込みコントローラ操作IF定義
 */
typedef int (*irq_ctrl_config_irq)(struct _irq_ctrlr *_ctrlr, irq_no _irq, irq_attr _attr, 
    irq_prio _prio);
typedef	bool (*irq_ctrl_irq_is_pending)(irq_no _irq, struct _trap_context *_ctx, 
    irq_prio *_priop);
typedef void (*irq_ctrl_enable_irq)(struct _irq_ctrlr *_ctrlr, irq_no _irq);
typedef void (*irq_ctrl_disable_irq)(struct _irq_ctrlr *_ctrlr, irq_no _irq);
typedef void (*irq_ctrl_get_priority)(struct _irq_ctrlr *_ctrlr, irq_prio *_prio);
typedef	void (*irq_ctrl_set_priority)(struct _irq_ctrlr *_ctrlr, irq_prio _prio);
typedef	void (*irq_ctrl_eoi)(struct _irq_ctrlr *_ctrlr, irq_no _irq);
typedef	int (*irq_ctrl_initialize)(struct _irq_ctrlr *_ctrlr);
typedef void (*irq_ctrl_finalize)(struct _irq_ctrlr *_ctrlr);

/**
   割込みコントローラエントリ
 */
typedef struct _irq_ctrlr{
	const char                        *name; /**< コントローラ名                     */
	struct _list                       link; /**< 割込みコントローラキューのエントリ */
	irq_no                          min_irq; /**< 割込み番号最小値                   */
	irq_no                          max_irq; /**< 割込み番号最大値                   */
	stat_cnt                    nr_handlers; /**< ハンドラ登録数 (単位:個)           */
	irq_ctrl_config_irq          config_irq; /**< 割込み線の初期化                   */
	irq_ctrl_irq_is_pending  irq_is_pending; /**< 割込み発生の確認                   */
	irq_ctrl_enable_irq 	     enable_irq; /**< 割込み許可                         */
	irq_ctrl_disable_irq        disable_irq; /**< 割込み禁止                         */
	irq_ctrl_get_priority      get_priority; /**< 割込み優先度獲得                   */
	irq_ctrl_set_priority      set_priority; /**< 割込み優先度設定                   */
	irq_ctrl_eoi                        eoi; /**< 割込み処理完了通知                 */
	irq_ctrl_initialize          initialize; /**< コントローラの初期化               */
	irq_ctrl_finalize              finalize; /**< コントローラの終了                 */
	void                           *private; /**< コントローラ固有情報へのポインタ   */
}irq_ctrlr;

/**
   割込み線情報
 */
typedef struct _irq_line{
	RB_ENTRY(_irq_line)  ent;  /**< 割り込み管理情報へのリンク                 */
	irq_no               irq;  /**< 割込み番号                                 */
	irq_prio            prio;  /**< 割込み優先度                               */
	irq_attr            attr;  /**< 割込み線の属性値                           */
	struct _queue   handlers;  /**< 割込みハンドラキュー                       */
	irq_ctrlr         *ctrlr;  /**< 割込みコントローラエントリへのリンク       */
}irq_line;

/**
   割込み管理情報
 */
typedef struct _irq_info{
	spinlock                               lock;  /**< 割込み管理情報のロック   */
	RB_HEAD(_irq_line_tree, _irq_line) prio_que;  /**< 割り込み優先度キュー     */
	struct _queue                     ctrlr_que;  /**< 割込みコントローラキュー */
	struct _irq_line              irqs[NR_IRQS];  /**< 割込み線情報             */
}irq_info;


int irq_handle_irq(struct _trap_context *_ctx);

int irq_register_handler(irq_no _irq, irq_attr _attr, irq_prio _prio, irq_handler _handler,
    void *_private);
int irq_unregister_handler(irq_no _irq, irq_handler _handler, void *_private);

int irq_register_ctrlr(irq_ctrlr *_ctrlr);
void irq_unregister_ctrlr(irq_ctrlr *_ctrlr);

void irq_init(void);
#endif  /*  _KERN_IRQ_IF_H   */

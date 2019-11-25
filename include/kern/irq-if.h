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

struct _trap_context;

/**
   割込みハンドラエントリ
 */
typedef struct _irq_handler_ent{
	/** 割込みハンドラキューのリストエントリ */
	struct _list link;
	/** 割込みハンドラ */
	int (*handler)(irq_no _irq, struct _trap_context *_ctx, void *_private);
	/** ハンドラ固有情報へのポインタ */
	void *private;
}irq_handler_ent;

/**
   割込みコントローラエントリ
 */
typedef struct _irq_ctrlr{
	/** 割込みコントローラキューへのリストエントリ */
	struct _list  link;
	/** 割込み線の初期化 */
	int (*config_irq)(struct _irq_ctrlr *_ctrlr, irq_no _irq, irq_attr _attr, irq_prio _prio);
	/** 割込み検出ハンドラ */
	int (*find_pending)(struct _trap_context *_ctx, irq_no *_irqp, irq_prio *_priop);
	/** 割込み線単位での割込みの有効化 */
	void (*enable_irq)(struct _irq_ctrlr *_ctrlr, irq_no _irq);
	/** 割込み線単位での割込みのマスク */
	void (*disable_irq)(struct _irq_ctrlr *_ctrlr, irq_no _irq);
	/** 割込みコントローラに設定されている割込み優先度の取得 */
	void (*get_priority)(struct _irq_ctrlr *_ctrlr, irq_prio *_prio);
	/** 割込みコントローラへの割込み優先度の設定 */
	void (*set_priority)(struct _irq_ctrlr *_ctrlr, irq_prio _prio);
	/** 割込み線単位での割込み処理完了通知  */
	void (*eoi)(struct _irq_ctrlr *_ctrlr, irq_no _irq);
	/** 割込みコントローラの初期化 */
	int (*initialize)(struct _irq_ctrlr *_ctrlr);
	/** 割込みコントローラの終了処理 */
	void (*finalize)(struct _irq_ctrlr *_ctrlr);
	/** 割込みコントローラ固有情報へのポインタ */
	void *private;
}irq_ctrlr;

/**
   割込み線情報
 */
typedef struct _irq_line{
	spinlock          lock;  /*< 割込み線情報のロック                       */
	irq_no              nr;  /*< 割込み番号                                 */
	irq_prio          prio;  /*< 割込み優先度                               */
	irq_attr          attr;  /*< 割込み線の属性値                           */
	struct _queue handlers;  /*< 割込みハンドラキュー                       */
	irq_ctrlr      *ctrlrp;  /*< 割込み線につながっている割込みコントローラ */
}irq_line;

/**
   割込み管理情報
 */
typedef struct _irq_manage{
	spinlock                  lock;  /*< 割込み管理情報のロック               */
	struct _queue        ctrlr_que;  /*< 割込みコントローラキュー             */
	struct _irq_line irqs[NR_IRQS];  /*< 割込み線情報                         */
}irq_manage;

int irq_initialize_manager(void);

int irq_register_ctrlr(irq_no _irq, irq_ctrlr *_ctrlrp);
int irq_unregister_ctrlr(irq_no _irq);
int irq_register_handler(irq_no _irq, irq_attr _attr, irq_prio _prio, void *_private, int (*_handler)(irq_no _irq, struct _trap_context *_ctx, void *_private));
int irq_unregister_handler(irq_no irq, int (*handler)(irq_no _irq, struct _trap_context *_ctx, void *_private));
void irq_register_pending_irq_finder(struct _irq_finder *_ent);
void irq_unregister_pending_irq_finder(struct _irq_finder *_ent);
int irq_handle_irq(struct _trap_context *_ctx);
#endif  /*  _KERN_IRQ_IF_H   */

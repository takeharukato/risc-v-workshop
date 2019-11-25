/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Page Frame definitions                                            */
/*                                                                    */
/**********************************************************************/
#if !defined(_PAGE_PFRAME_H)
#define  _PAGE_PFRAME_H 

/** ページの状態(共通部)
 */
#define PAGE_STATE_STATE_SHIFT   (0)  /**<  使用状態シフト  */
#define PAGE_STATE_USECASE_SHIFT (4)  /**<  使用用途シフト  */

#define PAGE_STATE_FREE     \
	(0x0 << PAGE_STATE_STATE_SHIFT )  /**< 未使用  */
#define PAGE_STATE_USED     \
	(0x1 << PAGE_STATE_STATE_SHIFT )  /**< 使用中  */
#define PAGE_STATE_RESERVED \
	(0x2 << PAGE_STATE_STATE_SHIFT)  /**< カーネルやメモリマップトデバイスが予約  */

#define PAGE_STATE_CLUSTERED     \
	(0x4 << PAGE_STATE_STATE_SHIFT )  /**< クラスタ化されたページ  */

#define PAGE_STATE_UCASE_KERN     \
	(0x1 << PAGE_STATE_USECASE_SHIFT )  /**< カーネル内部データ用に使用中 */
#define PAGE_STATE_UCASE_KSTACK   \
	(0x2 << PAGE_STATE_USECASE_SHIFT )  /**< カーネルスタックとして使用中 */
#define PAGE_STATE_UCASE_PGTBL   \
	(0x4 << PAGE_STATE_USECASE_SHIFT )  /**< ページテーブルとして使用中 */
#define PAGE_STATE_UCASE_SLAB     \
	(0x8 << PAGE_STATE_USECASE_SHIFT )  /**< SLABとして使用中 */
#define PAGE_STATE_UCASE_ANON     \
	(0x10 << PAGE_STATE_USECASE_SHIFT )  /**< 無名ページとして使用中 */
#define PAGE_STATE_UCASE_PCACHE   \
	(0x20 << PAGE_STATE_USECASE_SHIFT )  /**< ページキャッシュとして使用中 */
#define PAGE_STATE_UCASE_MASK      \
	(   PAGE_STATE_UCASE_KERN | PAGE_STATE_UCASE_KSTACK |	\
	    PAGE_STATE_UCASE_PGTBL | PAGE_STATE_UCASE_SLAB  |	\
	    PAGE_STATE_UCASE_ANON | PAGE_STATE_UCASE_PCACHE )  /**<  使用用途マスク  */

#if !defined(ASM_FILE)
#include <klib/freestanding.h>
#include <kern/kern-types.h>
#include <klib/list.h>
#include <klib/queue.h>
#include <kern/spinlock.h>

/**
   ページが空いていないことを確認する
   @param[in] _pf ページフレーム情報
   @retval 真  ページが空いていない
   @retval 偽  ページが空いていない
 */
#define PAGE_STATE_NOT_FREED(_pf) \
	( ( ( (struct _page_frame *)(_pf) )->state ) & ( PAGE_STATE_USED | PAGE_STATE_RESERVED ) )

/**
   ページを予約する
   @param[in] _pf ページフレーム情報
 */
#define PAGE_MARK_RESERVED(_pf) \
	do{ ( (struct _page_frame *)(_pf) )->state |= PAGE_STATE_RESERVED; }while(0)

/**
   ページ予約を解除する
   @param[in] _pf ページフレーム情報
 */
#define PAGE_UNMARK_RESERVED(_pf) \
	do{ ( (struct _page_frame *)(_pf) )->state &= ~PAGE_STATE_RESERVED; }while(0)

/**
   ページを使用中にする
   @param[in] _pf ページフレーム情報
 */
#define PAGE_MARK_USED(_pf) \
	do{ ( (struct _page_frame *)(_pf) )->state |= PAGE_STATE_USED; }while(0)

/**
   ページの使用中フラグを落とす
   @param[in] _pf ページフレーム情報
 */
#define PAGE_UNMARK_USED(_pf) \
	do{ ( (struct _page_frame *)(_pf) )->state &= ~PAGE_STATE_USED; }while(0)

/**
   ページが使用中であることを確認する
   @param[in] _pf ページフレーム情報
 */
#define PAGE_IS_USED(_pf) \
	( ( (struct _page_frame *)(_pf) )->state & PAGE_STATE_USED )


/**
   ページをクラスタページに設定する
   @param[in] _pf   ページフレーム情報
   @param[in] _head ヘッドページ
 */
#define PAGE_MARK_CLUSTERED(_pf, _head)					              \
	do{								              \
		( (struct _page_frame *)(_pf) )->state |= PAGE_STATE_CLUSTERED;       \
		( (struct _page_frame *)(_pf) )->headp = (struct _page_frame *)_head; \
	}while(0)

/**
   クラスタページの設定を解除する
   @param[in] _pf ページフレーム情報
 */
#define PAGE_UNMARK_CLUSTERED(_pf) \
	do{								              \
		( (struct _page_frame *)(_pf) )->state &= ~PAGE_STATE_CLUSTERED;       \
		( (struct _page_frame *)(_pf) )->headp = (struct _page_frame *)NULL; \
	}while(0)

/**
   ページがクラスタページの一部であることを確認する
   @param[in] _pf ページフレーム情報
 */
#define PAGE_IS_CLUSTERED(_pf) \
	( ( (struct _page_frame *)(_pf) )->state & PAGE_STATE_CLUSTERED )


/**
   ページをカーネルデータとして使用する
   @param[in] _pf ページフレーム情報
 */
#define PAGE_MARK_KERN(_pf) \
	do{ ( (struct _page_frame *)(_pf) )->state |= PAGE_STATE_UCASE_KERN; }while(0)

/**
   ページをカーネルデータとして使用しない
   @param[in] _pf ページフレーム情報
 */
#define PAGE_UNMARK_KERN(_pf) \
	do{ ( (struct _page_frame *)(_pf) )->state &= ~PAGE_STATE_UCASE_KERN; }while(0)

/**
   ページがカーネルデータとして使用中であることを確認する
   @param[in] _pf ページフレーム情報
 */
#define PAGE_USED_BY_KERN(_pf) \
	( ( (struct _page_frame *)(_pf) )->state & PAGE_STATE_UCASE_KERN )

/**
   ページをページテーブルとして使用する
   @param[in] _pf ページフレーム情報
 */
#define PAGE_MARK_PGTBL(_pf) \
	do{ ( (struct _page_frame *)(_pf) )->state |= PAGE_STATE_UCASE_PGTBL; }while(0)

/**
   ページをページテーブルとして使用しない
   @param[in] _pf ページフレーム情報
 */
#define PAGE_UNMARK_PGTBL(_pf) \
	do{ ( (struct _page_frame *)(_pf) )->state &= ~PAGE_STATE_UCASE_PGTBL; }while(0)

/**
   ページがページテーブルとして使用中であることを確認する
   @param[in] _pf ページフレーム情報
 */
#define PAGE_USED_BY_PGTBL(_pf) \
	( ( (struct _page_frame *)(_pf) )->state & PAGE_STATE_UCASE_PGTBL )

/**
   ページをカーネルスタックとして使用する
   @param[in] _pf ページフレーム情報
 */
#define PAGE_MARK_KSTACK(_pf) \
	do{ ( (struct _page_frame *)(_pf) )->state |= PAGE_STATE_UCASE_KSTACK; }while(0)

/**
   ページをカーネルスタックとして使用しない
   @param[in] _pf ページフレーム情報
 */
#define PAGE_UNMARK_KSTACK(_pf) \
	do{ ( (struct _page_frame *)(_pf) )->state &= ~PAGE_STATE_UCASE_KSTACK; }while(0)

/**
   ページがカーネルスタックとして使用中であることを確認する
   @param[in] _pf ページフレーム情報
 */
#define PAGE_USED_BY_KSTACK(_pf) \
	( ( (struct _page_frame *)(_pf) )->state & PAGE_STATE_UCASE_KSTACK )

/**
   ページをSLABとして使用する
   @param[in] _pf ページフレーム情報
 */
#define PAGE_MARK_SLAB(_pf) \
	do{ ( (struct _page_frame *)(_pf) )->state |= PAGE_STATE_UCASE_SLAB; }while(0)

/**
   ページをSLABとして使用しない
   @param[in] _pf ページフレーム情報
 */
#define PAGE_UNMARK_SLAB(_pf) \
	do{ ( (struct _page_frame *)(_pf) )->state &= ~PAGE_STATE_UCASE_SLAB; }while(0)

/**
   ページがSLABとして使用中であることを確認する
   @param[in] _pf ページフレーム情報
 */
#define PAGE_USED_BY_SLAB(_pf) \
	( ( (struct _page_frame *)(_pf) )->state & PAGE_STATE_UCASE_SLAB )

/**
   ページを無名ページとして使用する
   @param[in] _pf ページフレーム情報
 */
#define PAGE_MARK_ANON(_pf) \
	do{ ( (struct _page_frame *)(_pf) )->state |= PAGE_STATE_UCASE_ANON; }while(0)

/**
   ページを無名ページとして使用しない
   @param[in] _pf ページフレーム情報
 */
#define PAGE_UNMARK_ANON(_pf) \
	do{ ( (struct _page_frame *)(_pf) )->state &= ~PAGE_STATE_UCASE_ANON; }while(0)

/**
   ページが無名ページとして使用中であることを確認する
   @param[in] _pf ページフレーム情報
 */
#define PAGE_USED_BY_ANON(_pf) \
	( ( (struct _page_frame *)(_pf) )->state & PAGE_STATE_UCASE_ANON )

/**
   ページをページキャッシュとして使用する
   @param[in] _pf ページフレーム情報
 */
#define PAGE_MARK_PCACHE(_pf) \
	do{ ( (struct _page_frame *)(_pf) )->state |= PAGE_STATE_UCASE_PCACHE; }while(0)

/**
   ページをページキャッシュとして使用しない
   @param[in] _pf ページフレーム情報
 */
#define PAGE_UNMARK_PCACHE(_pf) \
	do{ ( (struct _page_frame *)(_pf) )->state &= ~PAGE_STATE_UCASE_PCACHE; }while(0)

/**
   ページがページキャッシュとして使用中であることを確認する
   @param[in] _pf ページフレーム情報
 */
#define PAGE_USED_BY_PCACHE(_pf) \
	( ( (struct _page_frame *)(_pf) )->state & PAGE_STATE_UCASE_PCACHE )

/**
   ページの使用用途をクリアする
   @param[in] _pf ページフレーム情報
 */
#define PAGE_CLEAR_USECASE(_pf) \
	do{ ( (struct _page_frame *)(_pf) )->state &= ~PAGE_STATE_UCASE_MASK; }while(0)


struct _slab;
struct _page_buddy;

/** ページフレーム情報
 */
typedef struct _page_frame{
	spinlock                lock;  /**< 排他用のロック                 */
	list                    link;  /**< バディキューへのリンク         */
	page_state             state;  /**< ページの状態(アーキ共通)       */
	page_state        arch_state;  /**< アーキ依存のページ状態         */
	obj_cnt_type             pfn;  /**< ページフレーム番号             */
	struct _refcounter    usecnt;  /**< 利用カウンタ                   */
	struct _refcounter    mapcnt;  /**< マップカウンタ                 */
	page_order             order;  /**< ページオーダ                   */
	struct _page_buddy   *buddyp;  /**< バディキュー                   */
	struct _queue        pv_head;  /**< 物理->仮想アドレス変換用キュー */
	struct _page_frame    *headp;  /**< クラスタ化ページの先頭ページ   */
	struct _slab          *slabp;  /**< スラブキャッシュ               */
}page_frame;
#endif  /* !ASM_FILE  */
#endif  /*  _PAGE_PFRAME_H  */

/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  kernel cache and page allocation interface definitions            */
/*                                                                    */
/**********************************************************************/
#if !defined(_KERN_PAGE_IF_H)
#define  _KERN_PAGE_IF_H

#include <kern/kern-types.h>
#include <kern/page-macros.h>
#include <kern/page-pframe.h>
#include <kern/page-slab.h>

/** ページ利用用途
 */
#define PAGE_USAGE_KERN   (PAGE_STATE_UCASE_KERN)    /**< カーネルメモリ     */
#define PAGE_USAGE_KSTACK (PAGE_STATE_UCASE_KSTACK)  /**< カーネルメモリ     */
#define PAGE_USAGE_PGTBL  (PAGE_STATE_UCASE_PGTBL)   /**< ページテーブル     */
#define PAGE_USAGE_SLAB   (PAGE_STATE_UCASE_SLAB)    /**< カーネルデータ構造 */
#define PAGE_USAGE_ANON   (PAGE_STATE_UCASE_ANON)    /**< 無名ページ         */
#define PAGE_USAGE_PCACHE (PAGE_STATE_UCASE_PCACHE)  /**< ページキャッシュ   */

#define PAGE_MASK PAGE_MASK_COMMON(PAGE_SIZE)  /**< ノーマルページのページマスク  */

/** メモリ獲得フラグ
 */
#define KMALLOC_NORMAL          \
	(KM_SFLAGS_DEFAULT)    /*< メモリ獲得を待ち合わせる   */
#define KMALLOC_ATOMIC          \
	(KM_SFLAGS_ATOMIC)     /*< メモリ獲得を待ち合わせない */
#define KMALLOC_NOCLR           \
	(KM_SFLAGS_CLR_NONE)   /*< メモリをクリアしない       */

#if !defined(ASM_FILE)

#include <klib/freestanding.h>
#include <kern/page-pfdb.h>

int pgif_calc_page_order(size_t _size, page_order *_res);

int pgif_get_free_page_cluster(void **_addrp, page_order _order,
    pgalloc_flags _pgflags, page_usage _usage);
int pgif_get_free_page(void **_addrp, pgalloc_flags _pgflags, page_usage _usage);
void pgif_free_page(void *_addr);
#endif  /*  !ASM_FILE  */
#endif  /*  _KERN_PAGE_IF_H   */

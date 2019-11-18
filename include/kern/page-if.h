/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  kernel cache and page allocation interface definitions            */
/*                                                                    */
/**********************************************************************/
#if !defined(_PAGE_PAGE_H)
#define  _PAGE_PAGE_H 

#include <klib/freestanding.h>
#include <kern/kern-types.h>

#include <kern/page-macros.h>

#include <kern/page-pframe.h>
#include <kern/page-pfdb.h>

/** ページ利用用途
 */
#define PAGE_USAGE_KERN   (PAGE_STATE_UCASE_KERN)    /**< カーネルメモリ     */
#define PAGE_USAGE_KSTACK (PAGE_STATE_UCASE_KSTACK)  /**< カーネルメモリ     */
#define PAGE_USAGE_PGTBL  (PAGE_STATE_UCASE_PGTBL)   /**< ページテーブル     */
#define PAGE_USAGE_SLAB   (PAGE_STATE_UCASE_SLAB)    /**< カーネルデータ構造 */
#define PAGE_USAGE_ANON   (PAGE_STATE_UCASE_ANON)    /**< 無名ページ         */
#define PAGE_USAGE_PCACHE (PAGE_STATE_UCASE_PCACHE)  /**< ページキャッシュ   */

#define PAGE_MASK PAGE_MASK_COMMON(PAGE_SIZE)  /**< ノーマルページのページマスク  */

/**    kmem-cache属性情報
 */
#if !defined(KM_SFLAGS_ARG_SHIFT)
#define KM_SFLAGS_ARG_SHIFT          (0)  /*< 引数指定属性値のシフト数  */
#endif  /*  KM_SFLAGS_ARG_SHIFT  */

#if !defined(KM_SFLAGS_DEFAULT)
#define KM_SFLAGS_DEFAULT            \
	( ULONG_C(0) << KM_SFLAGS_ARG_SHIFT)  /*< キャッシュ獲得のデフォルト  */
#endif  /*  KM_SFLAGS_DEFAULT  */

#if !defined(KM_SFLAGS_CLR_NONE)
#define KM_SFLAGS_CLR_NONE	     \
	( ULONG_C(1) << KM_SFLAGS_ARG_SHIFT)  /*< メモリをゼロクリアしない */
#endif  /*  KM_SFLAGS_CLR_NONE  */

#if !defined(KM_SFLAGS_ATOMIC)
#define KM_SFLAGS_ATOMIC	     \
	( ULONG_C(2) << KM_SFLAGS_ARG_SHIFT)  /*< メモリ獲得を待ち合わせない */
#endif  /*  KM_SFLAGS_ATOMIC  */

#if !defined(KM_SFLAGS_ALIGN_HW)
#define KM_SFLAGS_ALIGN_HW           \
	( ULONG_C(4) << KM_SFLAGS_ARG_SHIFT)  /*< ハードウエアアラインメントに合わせる */
#endif  /*  KM_SFLAGS_ALIGN_HW  */

#if !defined(KM_SFLAGS_COLORING)
#define KM_SFLAGS_COLORING           \
	( ULONG_C(8) << KM_SFLAGS_ARG_SHIFT)  /*< カラーリングを行う                   */
#endif  /*  KM_SFLAGS_COLORING  */

#if !defined(KM_SFLAGS_ARGS)
/** 引数で指定可能なフラグ  */
#define KM_SFLAGS_ARGS		     \
	( KM_SFLAGS_CLR_NONE | KM_SFLAGS_ATOMIC | KM_SFLAGS_ALIGN_HW | KM_SFLAGS_COLORING ) 
#endif  /* KM_SFLAGS_ARGS  */

/** メモリ獲得フラグ
 */
#define KMALLOC_NORMAL          \
	(KM_SFLAGS_DEFAULT)    /*< メモリ獲得を待ち合わせる   */
#define KMALLOC_ATOMIC          \
	(KM_SFLAGS_ATOMIC)     /*< メモリ獲得を待ち合わせない */
#define KMALLOC_NOCLR           \
	(KM_SFLAGS_CLR_NONE)   /*< メモリをクリアしない       */

#if !defined(ASM_FILE)
int pgif_calc_page_order(size_t _size, page_order *_res);

int pgif_get_free_page_cluster(void **_addrp, page_order _order, 
    pgalloc_flags _pgflags, page_usage _usage);
int pgif_get_free_page(void **_addrp, pgalloc_flags _pgflags, page_usage _usage);
void pgif_free_page(void *_addr);
#endif  /*  !ASM_FILE  */
#endif  /*  _PAGE_PAGE_H   */

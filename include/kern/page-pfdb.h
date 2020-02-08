/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Page Frame Data Base relevant definitions                         */
/*                                                                    */
/**********************************************************************/
#if !defined(_PAGE_PFDB_H)
#define  _PAGE_PFDB_H 

#include <klib/freestanding.h>
#include <klib/queue.h>

#include <kern/kern-types.h>
#include <kern/spinlock.h>

#include <kern/page-macros.h>
#include <kern/wqueue.h>

struct _page_frame;
struct   _pfdb_ent;

/** ページフレームDB統計情報
 */
typedef struct _pfdb_stat{
	obj_cnt_type          nr_pages;  /**<  総ページ数                          */
	obj_cnt_type     nr_free_pages;  /**<  ノーマルページ換算での空きページ数  */
	obj_cnt_type    reserved_pages;  /**<  ノーマルページ換算での予約ページ数
					       (空きページと利用可能ページ数から算出)  */
	obj_cnt_type   available_pages; /**< 利用可能ページ数 */
	obj_cnt_type free_nr[PAGE_POOL_MAX_ORDER];  /**< ページオーダ単位でのフリーページ数 */
	obj_cnt_type       kdata_pages;  /**<  カーネルデータページ    */
	obj_cnt_type      kstack_pages;  /**<  カーネルスタックページ  */
	obj_cnt_type       pgtbl_pages;  /**<  ページテーブル          */
	obj_cnt_type        slab_pages;  /**<  SLABページ              */
	obj_cnt_type        anon_pages;  /**<  アノニマスページ        */
	obj_cnt_type      pcache_pages;  /**<  ページキャッシュ        */
}pfdb_stat;

/** バディページ管理情報
 */
typedef struct _page_buddy{
	spinlock                            lock;  /**< バディページ管理情報のロック       */
	obj_cnt_type free_nr[PAGE_POOL_MAX_ORDER]; /**< ページオーダ単位でのフリーページ数 */
	struct _queue page_list[PAGE_POOL_MAX_ORDER]; /**< ページオーダ単位でのページリスト */
	obj_cnt_type                     nr_pages; /**< ページフレーム管理配列の要素数     */
	obj_cnt_type              available_pages; /**< 利用可能ページ数                   */
	obj_cnt_type                  kdata_pages; /**< カーネルデータページ数             */
	obj_cnt_type                 kstack_pages; /**< カーネルスタックページ数           */
	obj_cnt_type                  pgtbl_pages; /**< ページテーブルページ数             */
	obj_cnt_type                   slab_pages; /**< SLABページ数                       */
	obj_cnt_type                   anon_pages; /**< アノニマスページ数                 */
	obj_cnt_type                 pcache_pages; /**< ページキャッシュページ数           */
	struct _pfdb_ent                *pfdb_ent; /**< ページフレームDBエントリへのリンク */
	struct _page_frame                 *array; /**< ページフレーム配列                 */
}page_buddy;

/** メモリ中のページフレーム管理情報, ページフレームDB配列の配置
    1) kvaddrは物理メモリをカーネル空間にマップしたアドレスで,  [kvaddr, kvaddr + length)の
       範囲のメモリを管理する
    2) kenter_pa相当の操作により連続する物理メモリをカーネル空間にマップした後で,
       マップ先の開始アドレスkvaddrと領域長lengthを指定して, pfdb_addを呼び出すことで
       領域を追加する

          kvaddr +----------------------+<-ページフレーム配列[0], 
                 |                      |   min_pfnに対応するページの先頭
                 |                      |
                 |  利用可能ページ      |
                 |                      |
                 |                      |
                 |                      |
                 |                      |
                 |                      |
                 |                      |
                 +----------------------+
                 | ページフレーム配列   |
                 |(struct _page_frame[])|
                 +----------------------+
                 | struct _pfdb_ent     |
                 | (buddy含む)          |
kvaddr + length  +----------------------+<-ページフレーム配列[max_pfn-min_pfn],
                                           max_pfnに対応するページの末尾
   
 */

/** ページフレーム管理情報(バディページプール含む)
    [min_pfn, max_pfn)の範囲のページを管理する
 */
typedef struct _pfdb_ent{
	RB_ENTRY(_pfdb_ent)      ent; /**< ページフレーム管理情報のリンク                */
	void                 *kvaddr; /**< ストレートマップ領域での開始アドレス          */
	size_t                length; /**< 領域サイズ(単位:バイト)                       */
	obj_cnt_type         min_pfn; /**< 最小ページフレーム番号                        */
	obj_cnt_type         max_pfn; /**< 最大ページフレーム番号                        */
	private_inf          private; /**< プライベート情報                              */
	private_inf     mach_private; /**< アーキ固有プライベート情報                    */
	page_buddy         page_pool; /**< ページプール(buddy ページプール)              */
}pfdb_ent;

/** ページフレームDB
 */
typedef struct _page_frame_db{
	spinlock                         lock;  /**< ページフレームDBキューのロック */
	wque_waitqueue              page_wque;  /**< ページ待ちキュー               */
	RB_HEAD(_pfdb_tree, _pfdb_ent) dbroot;  /**< ページフレームDB               */
}page_frame_db;

/** ページフレームDB初期化子
   @param[in] _pfque ページフレームDBのポインタ 
 */
#define __PFDB_INITIALIZER(_pfque) {		                            \
	.lock = __SPINLOCK_INITIALIZER,		                            \
	.page_wque = __WQUE_WAITQUEUE_INITIALIZER(&((_pfque)->page_wque)),  \
	.dbroot  = RB_INITIALIZER(&((_pfque)->dbroot)),		            \
	}

void pfdb_add(uintptr_t _phys_start, size_t _length, struct _pfdb_ent **_pfdb_ent);
int pfdb_remove(pfdb_ent *_ent);

void pfdb_free(void);

int pfdb_buddy_dequeue(page_order _order, page_usage _usage, pgalloc_flags _alloc_flags, 
    obj_cnt_type *_pfnp);
void pfdb_mark_phys_range_reserved(vm_paddr _start, vm_paddr _end);
void pfdb_unmark_phys_range_reserved(vm_paddr _start, vm_paddr _end);

int pfdb_pfn_to_kvaddr(obj_cnt_type _pfn, void **_kvaddrp);
int pfdb_kvaddr_to_pfn(void *_kvaddr, obj_cnt_type *_pfnp);

int pfdb_kvaddr_to_page_frame(void *_kvaddr, struct _page_frame  **_pp);
int pfdb_pfn_to_page_frame(obj_cnt_type _pfn, struct _page_frame  **_pp);

int pfdb_kvaddr_to_paddr(void *_kvaddr, void **_paddrp);
int pfdb_paddr_to_kvaddr(void *_paddr, void **_kvaddrp);

void pfdb_page_map_count_init(page_frame *_pf, refcounter_val _val);
refcounter_val pfdb_ref_page_map_count(page_frame *_pf);
bool pfdb_inc_page_map_count(page_frame *_pf);
bool pfdb_dec_page_map_count(page_frame *_pf);

refcounter_val pfdb_ref_page_use_count(struct _page_frame *_pf);
bool pfdb_inc_page_use_count(struct _page_frame  *_pf);
bool pfdb_dec_page_use_count(struct _page_frame  *_pf);

bool kcom_is_pfn_valid(obj_cnt_type _pfn);
void kcom_obtain_pfdb_stat(pfdb_stat *_statp);

#endif  /*  _PAGE_PFDB_H  */

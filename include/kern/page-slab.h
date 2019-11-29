/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  SLAB Cache relevant definitions                                   */
/*                                                                    */
/**********************************************************************/
#if !defined(_PAGE_SLAB_H)
#define  _PAGE_SLAB_H 

#include <kern/kern-types.h>
#include <kern/kern-cpuinfo.h>

/**    kmem-cache属性情報
 */
#define KM_SFLAGS_ARG_SHIFT          (0)  /*< 引数指定属性値のシフト数  */
#define KM_SFLAGS_DEFAULT            \
	( ULONG_C(0) << KM_SFLAGS_ARG_SHIFT)  /*< キャッシュ獲得のデフォルト  */
#define KM_SFLAGS_CLR_NONE	     \
	( ULONG_C(1) << KM_SFLAGS_ARG_SHIFT)  /*< メモリをゼロクリアしない */
#define KM_SFLAGS_ATOMIC	     \
	( ULONG_C(2) << KM_SFLAGS_ARG_SHIFT)  /*< メモリ獲得を待ち合わせない */
#define KM_SFLAGS_ALIGN_HW           \
	( ULONG_C(4) << KM_SFLAGS_ARG_SHIFT)  /*< ハードウエアアラインメントに合わせる */
#define KM_SFLAGS_COLORING           \
	( ULONG_C(8) << KM_SFLAGS_ARG_SHIFT)  /*< カラーリングを行う                   */

/** 引数で指定可能なフラグ  */
#define KM_SFLAGS_ARGS		     \
	( KM_SFLAGS_CLR_NONE | KM_SFLAGS_ATOMIC | KM_SFLAGS_ALIGN_HW | KM_SFLAGS_COLORING ) 

#define KM_LIMITS_DEFAULT            (0)  /*< デフォルトのリミット(即時解放)       */
#define KM_SLAB_TYPE_DIVISOR         (8)  /*< ページの1/8未満は, オンスラブキャッシュ使用  */

/**  スラブキャッシュの内部属性値
 */
#define KM_SFLAGS_INTERNAL_SHIFT      (8)  /*< 内部属性値のシフト数  */
#define KM_SFLAGS_ON_SLAB              \
	( ULONG_C(0) << KM_SFLAGS_INTERNAL_SHIFT)  /*< オンスラブキャッシュ  */
#define KM_SFLAGS_OFF_SLAB             \
	( ULONG_C(1) << KM_SFLAGS_INTERNAL_SHIFT)  /*< オフスラブキャッシュ  */
#define KM_SFLAGS_PREDEFINED_CACHE     \
	( ULONG_C(2) << KM_SFLAGS_INTERNAL_SHIFT)  /*< 規定のスラブキャッシュ  */
#define KM_SFLAGS_MARK_DESTROY   \
	( ULONG_C(4) << KM_SFLAGS_INTERNAL_SHIFT)  /*< 解放予約  */

#if !defined(HAL_CPUCACHE_HW_ALIGN)
#define HAL_CPUCACHE_HW_ALIGN ( sizeof(void *) * 2 )  /*  ポインタ長の2倍に合わせる  */
#endif  /* !HAL_CPUCACHE_HW_ALIGN */

/**
   キャッシュフラグをONスラブに設定する
   @param[in] _flags  カーネルキャッシュ管理情報フラグ
 */
#define KMEM_CACHE_SFLAGS_MARK_ON_SLAB(_flags) do{			\
		(_flags) &=  ~KM_SFLAGS_OFF_SLAB;			\
	}while(0)

/**
   キャッシュフラグをOFFスラブに設定する
   @param[in] _flags  カーネルキャッシュ管理情報フラグ
 */
#define KMEM_CACHE_SFLAGS_MARK_OFF_SLAB(_flags) do{			\
		(_flags) |=  KM_SFLAGS_OFF_SLAB;			\
	}while(0)

/**
   キャッシュがOFFスラブであることを確認する
   @param[in] _cache  カーネルキャッシュ管理情報
 */
#define KMEM_CACHE_IS_OFF_SLAB(_cache)  \
	((_cache)->sflags & KM_SFLAGS_OFF_SLAB)

/**
   キャッシュがONスラブであることを確認する
   @param[in] _cache  カーネルキャッシュ管理情報
 */
#define KMEM_CACHE_IS_ON_SLAB(_cache)  \
	(!KMEM_CACHE_IS_OFF_SLAB(_cache))

/** SLAB解放処理フラグ
 */
#define SLAB_REAP_NORMAL        (0) /** 最低限保持するSLAB数を保持する  */
#define SLAB_REAP_FORCE         (1) /** 最低限保持するSLAB数を保持する  */

/**
   事前割当て済みカーネルキャッシュ
 */
#define SLAB_PREALLOC_CACHE_NR   (14)  /** 事前割当て済みカーネルキャッシュの数      */
#define SLAB_PREALLOC_BASE       (ULONG_C(8)) /** 事前割当て済みカーネルキャッシュの基準サイズ  */

/**  事前割当て済みカーネルキャッシュの最小サイズ 8Byte (単位:バイト)  */
#define SLAB_PREALLOC_MIN	(SLAB_PREALLOC_BASE << 0)
/**  事前割当て済みカーネルキャッシュの最大サイズ 64KiB (単位:バイト) */
#define SLAB_PREALLOC_MAX       (SLAB_PREALLOC_BASE << (SLAB_PREALLOC_CACHE_NR - 1)) 

/**  事前割当て済みカーネルキャッシュのアラインメント(アラインメントを設定しない)  */
#define SLAB_ALIGN_NONE         (0) /** アラインメントをつけない  */
#define SLAB_PREALLOC_ALIGN     \
	(SLAB_ALIGN_NONE) /** 事前割当て済みカーネルキャッシュはアラインメントをつけない  */
#define SLAB_PREALLOC_LIMIT     (0) /** 事前割当て済みカーネルキャッシュの最小保持SLAB  */

#if !defined(ASM_FILE)
#include <klib/freestanding.h>
#include <kern/spinlock.h>
#include <klib/slist.h>
#include <klib/list.h>
#include <klib/queue.h>

/** カーネルメモリキャッシュ
 */
typedef struct _kmem_cache{
	struct _spinlock                        lock;  /*< ロック変数  */
	const char                             *name;  /*< メモリキャッシュの名前   */
	SPLAY_ENTRY(_kmem_cache)                node;  /*< メモリキャッシュ管理へのリンク  */
	size_t                          payload_size;  /*< オブジェクトサイズ(単位:バイト) */
	size_t                              obj_size;  /*< アライメントを含むオブジェクトサイズ(単位: バイト)  */
	page_order                             order;  /*< SLAB1つ分のサイズ(単位:ページオーダ)  */
	uintptr_t                              align;  /*< オブジェクトのアライメント(単位: バイト) */
	uintptr_t                       color_offset;  /*< カラーリングの単位 (単位:バイト) */
	obj_cnt_type                       color_num;  /*< キャッシュライン数     */
	obj_cnt_type                     color_index;  /*< 次に割り当てるオブジェクトのオフセット (インデクス値)     */
	obj_cnt_type                          limits;  /*< 最低限保持するSLAB数  */
	obj_cnt_type                      slab_count;  /*< 作成済みSLAB数  */
	obj_cnt_type                   objs_per_slab;  /*< SLAB1つに入るオブジェクトの数(単位:個) */
	slab_flags                            sflags;  /*< スラブのフラグ  */
	struct _queue                        partial;  /*< 使用中SLABのリスト  */
	struct _queue                           full;  /*< 空きがないSLABのリスト  */
	struct _queue                           free;  /*< 未使用SLABのリスト  */
	void (*constructor)(void *_obj, size_t _siz);  /*< オブジェクト獲得時のコンストラクタ */
	void  (*destructor)(void *_obj, size_t _siz);  /*< オブジェクト解放時のデストラクタ  */
}kmem_cache;

/** SLAB管理情報
 *  @note count操作はkmem_cacheのロックを獲得して行う
 */
typedef struct _slab{
	struct _spinlock      lock;  /*< SLAB内の空きオブジェクトリストのロック     */
	struct _kmem_cache  *cache;  /*< kmem_cacheへのバックリンク                 */
	struct _list          link;  /*< kmem_cacheへのリンクエントリ               */
	struct _slist_head objects;  /*< 空き領域キュー                             */
	obj_cnt_type         count;  /*< SLAB内に含まれるオブジェクトの数(単位:個)  */
	void                 *page;  /*< オブジェクトを格納したページへのポインタ   */
}slab;

/** OFF SLAB用のバッファ制御
    @note link情報を２重解放抑止に使用する
 */
typedef struct _kmem_bufctl{
	struct _slist_node link;  /*< 空き領域キューへのリンク  */
	struct _slab  *backlink;  /*< SLAB情報へのバックリンク  */
	void               *obj;  /*< オブジェクトへのポインタ  */
}kmem_bufctl;

/** ON SLAB用のバッファ制御 (small object用管理情報)
    @note link情報を２重解放抑止に使用する
 */
typedef struct _kmem_s_bufctl{
	struct _slist_node link;  /*< 空き領域キューへのリンク  */
}kmem_s_bufctl;

int slab_kmem_cache_create(struct _kmem_cache *_cache, const char *_name, size_t _size,
			 uintptr_t _align, obj_cnt_type _limits, slab_flags _sflags,
			 void  (*_constructor)(void *_obj, size_t _siz),
			 void  (*_destructor)(void *_obj, size_t _siz));

void slab_kmem_cache_destroy(kmem_cache *_cache);
void slab_kmem_cache_reap(kmem_cache *_cache, int _reap_flags);
void slab_kmem_cache_free(void *_obj);
int slab_kmem_cache_alloc(kmem_cache *_cache, pgalloc_flags _mflags, void **_objp);

void slab_prepare_preallocate_cahches(void);
void slab_finalize_preallocate_cahches(void);

void *kmalloc(size_t _size, pgalloc_flags _mflags);
void kfree(void *);
#endif  /* !ASM_FILE  */
#endif  /*  _PAGE_SLAB_H   */

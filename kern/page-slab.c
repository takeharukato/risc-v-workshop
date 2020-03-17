/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  SLAB Allocator                                                    */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>

#include <kern/page-if.h>
#include <kern/kern-cpuinfo.h>

/**
   キャッシュが使用中であることを確認する
   @param[in] _cache  カーネルキャッシュ管理情報
   @note カーネルキャッシュ管理情報のロックを獲得して呼び出すこと
 */
#define KMEM_CACHE_IS_BUSY(_cache)  \
	( !queue_is_empty(&(_cache)->partial) || !queue_is_empty(&(_cache)->full) )

/**
   管理情報へのポインタを配置するために必要なサイズ
 */
#define SLAB_BUFCTL_POINTER_SIZE (sizeof(void *))

/**
   事前割当て済みキャッシュ初期化情報
 */
typedef struct _slab_prealloc_cache_info{
	const char *name;  /** 名前   */
	size_t size;       /** サイズ */
}slab_prealloc_cache_info;

static kmem_cache prealloc_caches[SLAB_PREALLOC_CACHE_NR];  /** 事前割当て済みキャッシュ  */

/**  事前割当て済みキャッシュ初期化情報  */
static slab_prealloc_cache_info prealloc_caches_info[]={
	{"kmalloc-8", SLAB_PREALLOC_BASE << 0},
	{"kmalloc-16", SLAB_PREALLOC_BASE << 1},
	{"kmalloc-32", SLAB_PREALLOC_BASE << 2},
	{"kmalloc-64", SLAB_PREALLOC_BASE << 3},
	{"kmalloc-128", SLAB_PREALLOC_BASE << 4},
	{"kmalloc-256", SLAB_PREALLOC_BASE << 5},
	{"kmalloc-512", SLAB_PREALLOC_BASE << 6},
	{"kmalloc-1024", SLAB_PREALLOC_BASE << 7},
	{"kmalloc-2048", SLAB_PREALLOC_BASE << 8},
	{"kmalloc-4096", SLAB_PREALLOC_BASE << 9},
	{"kmalloc-8192", SLAB_PREALLOC_BASE << 10},
	{"kmalloc-16384", SLAB_PREALLOC_BASE << 11},
	{"kmalloc-32768", SLAB_PREALLOC_BASE << 12},
	{"kmalloc-65536", SLAB_PREALLOC_BASE << 13},
	{"kmalloc-131072", SLAB_PREALLOC_BASE << 14},
	{"kmalloc-262144", SLAB_PREALLOC_BASE << 15},
	{"kmalloc-524288", SLAB_PREALLOC_BASE << 16},
	{"kmalloc-1048576", SLAB_PREALLOC_BASE << 17},
};

/**
   OFF SLABのスラブサイズを算出する (内部関数)
   @param[in] obj_size オブジェクトサイズ
   @param[in] coloring_area キャッシュカラーリング用の領域長 (単位:バイト)
   @param[out] orderp  ページーオーダ返却領域
   @retval  0      正常終了
   @retval -ESRCH  格納可能なページオーダを越えている   
 */
static int
cacl_off_slab_size(size_t obj_size, size_t coloring_area, page_order *orderp) {
	page_order           order;
	size_t           slab_size;
	size_t           candidate;
	size_t   candidate_remains;
	size_t      data_area_size;
	size_t         remain_size;

	/* オブジェクトページサイズを算出する。
	 * 未使用領域が(領域長/KM_SLAB_TYPE_DIVISOR)未満になるか, 
	 * またはより大きな領域を確保していく中で最も未使用領域が
	 * 小さくなるサイズをオブジェクトページサイズとする。
	 */
	
	/*  最大バディページサイズを超えるサイズを番兵として設定  */
	candidate_remains = PAGE_SIZE << PAGE_POOL_MAX_ORDER;  

	/*  小さいサイズから順に試していき最適なオブジェクトページサイズを算出する */
	for(order = 0, candidate = 0;
	    (page_order)PAGE_POOL_MAX_ORDER > order; ++order) {
		
		slab_size = PAGE_SIZE << order;
		if ( slab_size < ( obj_size + coloring_area ) )
			continue;  /* オブジェクトを格納不可能なサイズ  */

		 /*  オブジェクトを配置可能な領域の長さを求める */
		data_area_size = slab_size - coloring_area; 

		/* 余りサイズが閾値を下回ったら検索を終了 */
		remain_size = data_area_size 
			- ( ( data_area_size / obj_size ) * obj_size );
		if ( ( slab_size / KM_SLAB_TYPE_DIVISOR ) >= remain_size ) {

			candidate = order;
			candidate_remains = remain_size;
			goto success_out;
		}

		/* 以前より余り領域が小さい場合は, サイズの候補を更新する  */
		if ( candidate_remains > remain_size ) {
		
			candidate = order;
			candidate_remains = remain_size;
		}
	}

	if ( candidate_remains == (PAGE_SIZE << PAGE_POOL_MAX_ORDER) )
		return -ESRCH; /* 格納可能なページオーダを越えている */

success_out:
	*orderp = candidate;
	return 0;
}

/**
    スラブ長を算出する (内部関数)
    @param[in] cache        カーネルメモリキャッシュ
    @param[in] payload_size メモリオブジェクト 
    @param[in] align        アラインメント(単位:バイト)
    @param[in] cache_flags  キャッシュ割り当てフラグ
    @retval  0 正常終了
    @retval -EINVAL 奇数アラインメントを指定した
    @retval -ESRCH  格納可能なページオーダを越えている
 */
static int 
calc_slab_size(kmem_cache *cache, size_t payload_size, uintptr_t align, 
    slab_flags cache_flags){
	int                      rc;
	page_order            order;
	size_t             obj_size;
	uintptr_t         obj_align;
	size_t            slab_size;
	obj_cnt_type  objs_per_slab;
	slab_flags           sflags;
	size_t        coloring_area;

	if ( align & 0x1 ) 
		return -EINVAL;  /*  奇数アラインメントを指定した  */
	
	/*  引数で指定されたメモリ獲得条件を記憶する  */
	sflags = cache_flags & KM_SFLAGS_ARGS; 

	/*
	 * オブジェクトサイズとアライメントサイズを初期化する
	 */
	obj_align = 0;
	obj_size = payload_size;

	/*
	 * キャッシュアラインメント調整用領域を追加する
	 */
	if ( align > 0 )   
		obj_align = align;  /*  アラインメント指定を記憶する  */

	if ( cache_flags & KM_SFLAGS_ALIGN_HW )
		obj_align = HAL_CPUCACHE_HW_ALIGN;  /* ハードウエアアラインメント */
	
	/* 管理領域へのポインタ(ポインタサイズ分アライン可能なサイズを確保)と
	 * アラインメントオフセットを含めて領域を確保する
	 */
	if ( obj_align > 0 ) {
	
		/*  格納するデータ(payload)の直前にポインタ境界にポインタを配置する.
		 *  格納するデータ(payload)のサイズとポインタ長を足して, 次のポインタ境界
		 *  からオブジェクトを配置するようにサイズを補正することで領域を確保する。
		 */
		obj_size = roundup_align(payload_size + 
		    SLAB_BUFCTL_POINTER_SIZE + 1,
		    sizeof(void *));
		obj_size = roundup_align(obj_size, obj_align);
	} 
	
	/* 
	 * SLABの種類, オブジェクト配置先ページのサイズと配置可能オブジェクト数を決定する
	 */
	if ( ( !( sflags & KM_SFLAGS_COLORING ) ) &&
	    ( ( PAGE_SIZE / KM_SLAB_TYPE_DIVISOR ) > obj_size ) ) {

		/* カラーリングを行わない場合で, かつ, 1ページ内に8オブジェクト
		 * (KM_SLAB_TYPE_DIVISOR)収まるならON SLABでキャッシュを構成する。
		 * ON SLABの場合バッファ管理情報を含めてオブジェクト長を設定する。
		 */
		obj_size = roundup_align(obj_size + sizeof(kmem_s_bufctl), 
		    sizeof(void *));
		KMEM_CACHE_SFLAGS_MARK_ON_SLAB(sflags);  /* ON SLABに設定する  */
		order = 0;              /*  ページオーダ0(=1ページ)  */
		slab_size = PAGE_SIZE;  /*  1ページ割り当てる  */
		/*  SLAB内のオブジェクト数を算出
		 *  ON SLABの場合は, SLAB情報をページ内に格納するので
		 *  ページサイズからSLAB情報サイズを減らして割り当てオブジェクト数を
		 *  算出する
		 */
		objs_per_slab = ( slab_size - sizeof(slab) ) / obj_size; 
	} else {

		/* 
		 * OFF SLABの場合
		 */

		/*
		 * キャッシュカラーリング用にキャッシュサイズ分の領域を獲得する
		 * ラストレベルキャッシュサイズ分の領域を用意する 
		 */
		coloring_area = 0;
		if ( sflags & KM_SFLAGS_COLORING ) {

			/* キャッシュラインサイズ単位で, オブジェクト開始アドレスを
			 * ずらす。
			 */
			cache->color_offset = krn_get_cpu_l1_dcache_linesize(); 

			/*  同時にキャッシュに存在しうるキャッシュライン数(セット数)
			 *  周期でオフセットをラウンドする
			 */
			cache->color_num = krn_get_cpu_l1_dcache_color_num();
			
			/*  カラーリング用領域を確保  */
			coloring_area = cache->color_offset * cache->color_num;

			cache->color_index = 0;  /*  最初のオフセット  */
		}
		
		/* アラインメントがついていない場合は, 
		 * オブジェクトの直前にバッファ管理情報へのポインタを配置する
		 * 領域を割り当てる。
		 * アライメント補正がある場合はすでに領域を割り当てているので割り当て不要。
		 */
		if ( obj_align == 0 )  
			obj_size = roundup_align(obj_size + SLAB_BUFCTL_POINTER_SIZE, 
			    sizeof(void *));
		rc = cacl_off_slab_size(obj_size, coloring_area, 
		    &order);  /* オブジェクトページサイズ算出 */
		if ( rc != 0 )
			goto error_out;

		KMEM_CACHE_SFLAGS_MARK_OFF_SLAB(sflags);  /* OFF SLABフラグに設定する  */
		slab_size = PAGE_SIZE << order;  /*  SLABサイズを算出  */
		/*  SLAB内のオブジェクト数を算出  */
		objs_per_slab = (slab_size - coloring_area) / obj_size;  
	}
	
	/* オブジェクトの格納方式（ONスラブ/OFFスラブ), アラインメント
	 * オブジェクトサイズ, スラブサイズ, スラブ当たりの格納オブジェクト数
	 * を設定する
	 */
	cache->align = obj_align;
	cache->sflags = sflags;
	cache->obj_size = obj_size;
	cache->order = order;
	cache->objs_per_slab = objs_per_slab;

	return 0;

error_out:
	return rc;
}

/**
   SLAB管理情報を初期化する (内部関数)
   @param[in] cache      カーネルメモリキャッシュ
   @param[in] sinfo      SLAB管理情報
   @param[in] kpage_addr カーネルページアドレス
 */
static void
init_slab_info(kmem_cache *cache, slab  *sinfo, void *kpage_addr) {

	spinlock_init(&sinfo->lock);  /*  ロックの初期化  */
	sinfo->cache = cache;         /*  キャッシュ情報へのリンクを設定  */
	list_init(&sinfo->link);      /*  SLAB情報のリンクを初期化  */
	slist_init_head(&sinfo->objects); /*  空きオブジェクトのリストを初期化  */
	sinfo->count = 0;                 /*  割り当て済みオブジェクト数を初期化  */
	sinfo->page = kpage_addr;         /*  オブジェクトページへのリンクを設定  */
}

/**
   SLABからオブジェクトを割り当てる (内部関数)
   @param[in]  cache  カーネルメモリキャッシュ
   @param[in]  sinfo  SLAB管理情報
   @param[out] objp   獲得したオブジェクトを指し示すポインタ変数のアドレス
 */
static void
alloc_slab_obj(kmem_cache *cache, slab *sinfo, void **objp){
	intrflags       iflags;
	void              *ptr;
	void         *mctl_ptr;
	kmem_s_bufctl  *s_bctl;
	kmem_bufctl      *bctl;

	/*
	 *  空きオブジェクトリストから空きオブジェクトを獲得
	 */

	/*  空きオブジェクトリストのロックを獲得  */
	spinlock_lock_disable_intr(&sinfo->lock, &iflags);  

	/* ON SLABの場合は, kmem_s_bufctl分後の領域から
	 * オブジェクトが配置されている
	 */
	s_bctl = container_of(slist_ref_top(&sinfo->objects), kmem_s_bufctl, link);

	/* OFF SLABの場合は, kmem_bufctlからオブジェクトのアドレスを求める
	 */
	bctl = container_of(slist_ref_top(&sinfo->objects), kmem_bufctl, link);	
	if ( KMEM_CACHE_IS_ON_SLAB(cache) ) {  

		/* ON SLABの場合
		 */
		slist_del(&sinfo->objects, &s_bctl->link);
		kassert(s_bctl->link.next == NULL);  /*  リンクを外したことを確認  */
		ptr = (void *)(&s_bctl[1]);  /*  ペイロードへのポインタを取得  */
	} else {

		/* OFF SLABの場合
		 */
		slist_del(&sinfo->objects, &bctl->link);
		kassert(bctl->link.next == NULL);  /*  リンクを外したことを確認  */
		ptr = bctl->obj;    /*  ペイロードへのポインタを取得  */
	}

	/*  空きオブジェクトリストのロックを解放  */
	spinlock_unlock_restore_intr(&sinfo->lock, &iflags);

	/* アライメントありでの割り当ての場合, バッファ制御情報をペイロードの直前に
	 * 配置し, オブジェクト解放時にバッファ制御情報を参照可能にする。
	 */
	if ( cache->align > 0 ) {

		/* バッファ制御情報へのポインタをオブジェクトの直前に格納するため,
		 * ポインタサイズを含めてアライメント補正を行い, ペイロードの
		 * アドレスを算出する
		 */
		ptr = (void *)roundup_align(ptr + sizeof(void *),
		    cache->align);  /*  返却するポインタをアライメントに合わせて補正  */
		
		/*  オブジェクト位置からポインタサイズ分減算し, そのアドレスを
		 *  ポインタサイズで切り詰めた領域に管理情報へのポインタを配置する
		 */
		mctl_ptr = (void *)truncate_align(ptr - sizeof(void *),
		    sizeof(void *)); 

		/* バッファ制御情報のアドレスを格納する  */
		if ( KMEM_CACHE_IS_ON_SLAB(cache) )
			*((void **)mctl_ptr) = (void *)&s_bctl[0]; 
		else
			*((void **)mctl_ptr) = (void *)bctl;
	}

	if ( cache->constructor != NULL ) /*  コンストラクタ呼び出し  */
		cache->constructor(ptr, cache->payload_size);

	*objp = ptr;  /*  オブジェクトのアドレスを返却  */	

	/* ON SLABの場合のオブジェクトサイズを確認  */
	kassert( ( !KMEM_CACHE_IS_ON_SLAB(cache) ) ||
	    ( cache->obj_size >= 
		( (size_t)
		    ( ( (void *)ptr + cache->payload_size ) - (void *)&s_bctl[0] ) ) ) );

	return ;
}

/**
   オブジェクトをSLABに返却する (内部関数)
   @param[in]  cache  カーネルメモリキャッシュ
   @param[in]  sinfo  SLAB管理情報
   @param[in]  bufctl バッファ管理情報へのポインタ
 */
static void
free_slab_obj(kmem_cache *cache, slab *sinfo, void *bufctl){
	intrflags       iflags;
	kmem_s_bufctl  *s_bctl;
	kmem_bufctl      *bctl;

	/*
	 * SLAB管理情報の空きオブジェクトキューを更新する
	 */

	/*  空きオブジェクトリストのロックを獲得  */
	spinlock_lock_disable_intr(&sinfo->lock, &iflags);

	if ( KMEM_CACHE_IS_ON_SLAB(cache) ) { /* ON SLABの場合  */

		s_bctl = (kmem_s_bufctl *)bufctl;  /* バッファ制御情報のアドレスを取得  */
		kassert(s_bctl->link.next == NULL); /* 空きオブジェクトリスト中にない  */
		slist_add(&sinfo->objects, &s_bctl->link); /* 空きオブジェクトリストに追加  */
	} else {

		bctl = (kmem_bufctl *)bufctl;   /* バッファ制御情報のアドレスを取得  */
		kassert(bctl->link.next == NULL);  /* 空きオブジェクトリスト中にない  */
		slist_add(&sinfo->objects, &bctl->link);  /* 空きオブジェクトリストに追加  */
	}

	/*  空きオブジェクトリストのロックを解放  */
	spinlock_unlock_restore_intr(&sinfo->lock, &iflags);

	return ;
}

/**
   SLABを解放する (内部関数)
   @param[in]  cache  カーネルメモリキャッシュ
   @param[in]  sinfo  SLAB管理情報
*/
static void
free_slab_info(kmem_cache *cache, slab *sinfo) {
	int                rc;
	kmem_bufctl     *bctl;
	page_frame        *pf;
	void         *objpage;

	/* OFF SLABのSLAB情報の解放前にオブジェクトページのページフレーム情報を得ておく  */
	rc = pfdb_kvaddr_to_page_frame(sinfo->page, &pf);
	kassert( rc == 0 );

	objpage = sinfo->page;  /* オブジェクトページをページフレーム情報から取得  */

	/* ON SLABの場合は, オブジェクトページの最後にSLAB管理領域があり, かつ
	 * バッファ管理情報は各オブジェクトの直前にあるのでオブジェクトページを
	 * 解放するだけでよい
	 */
	if ( !KMEM_CACHE_IS_ON_SLAB(cache) ) {  

		/* OFF SLABの場合は, バッファ制御情報と
		 * SLAB管理情報とを解放する
		 */
		while( !slist_is_empty(&sinfo->objects) ) { 
			
			/* 空きオブジェクトリストが空でなければ,
			 * バッファ制御情報を取り出し, 解放する
			 */
			bctl = container_of(slist_get_top(&sinfo->objects), 
			    kmem_bufctl, link);  /*  バッファ制御情報を取り出し  */
			kassert(bctl->link.next == NULL); /* 空きオブジェクトリスト中にない */
			kfree( bctl ); /*  バッファ制御情報を解放  */
		}
		kfree(sinfo);  /* SLAB管理情報を解放  */
	}

	pgif_free_page(objpage);  /*  オブジェクトページを解放  */
}

/**
   カーネルメモリキャッシュを伸長する (内部関数)
   @param[in]  cache  カーネルキャッシュ管理情報
   @param[in]  mflags メモリ獲得時のフラグ指定
   @param[out] sinfop SLAB管理情報を指し示すポインタのアドレス
   @retval    0     正常終了
   @retval   -ENOMEM メモリ不足
 */
static int
slab_kmem_cache_grow(kmem_cache *cache, pgalloc_flags mflags, slab **sinfop) {
	int                 rc;
	void            *kaddr;
	slab            *sinfo;
	size_t       slab_size;
	obj_cnt_type         i;
	kmem_s_bufctl  *sbfctl;
	kmem_bufctl      *bctl;
	page_frame         *pf;
	void    *off_slab_objp;
	size_t     this_coloff;
	/*
	 * オブジェクトページを獲得する
	 */
	rc = pgif_get_free_page_cluster(&kaddr, cache->order, mflags, PAGE_USAGE_SLAB);
	if ( rc != 0 )
		goto error_out;  /*  メモリ獲得失敗  */

	rc = pfdb_kvaddr_to_page_frame(kaddr, &pf);  /*  ページフレーム情報を取得  */
	kassert( rc == 0 );

	slab_size = PAGE_SIZE << cache->order;  /*  オブジェクトページサイズを算出  */

	/*
	 * SLABオブジェクトを割り当てる
	 */
	if ( KMEM_CACHE_IS_ON_SLAB(cache) ) { /*  ON SLABの場合  */

		/*  SLAB管理情報をページの末尾に配置  */
		sinfo = (slab *)(kaddr + slab_size - sizeof(slab));
		init_slab_info(cache, sinfo, kaddr);  /* SLAB管理情報を初期化 */
		pf->slabp = sinfo; /* SLAB管理情報を指し示す */

		/*
		 *  SLABに空きオブジェクトを追加
		 */
		for( i = 0; cache->objs_per_slab > i; ++i) {  

			/* オブジェクトの直前にリスト構造を配置し, 
			 * 空きオブジェクトをリストの先頭に追加
			 */
			sbfctl = (kmem_s_bufctl *)
				((void *)kaddr +
				    cache->obj_size * i );
			slist_add(&sinfo->objects, &sbfctl->link); 
		}
	} else {  /*  OFF SLABの場合  */
		
		/* SLAB情報を事前割当て済みカーネルキャッシュから割り当てる  */
		sinfo = kmalloc(sizeof(slab), mflags);
		if ( sinfo == NULL ) {

			rc = -ENOMEM;  /*  メモリ獲得失敗  */
			goto free_obj_page_out;
		}

		init_slab_info(cache, sinfo, kaddr);  /* SLAB管理情報を初期化 */
		pf->slabp = sinfo; /* SLAB管理情報を指し示す */

		/*
		 * カラーリングオフセットを算出する
		 */
		this_coloff = 0;
		if ( cache->sflags & KM_SFLAGS_COLORING ) {
			
			/* カラーリングオフセット分ずらして
			 * オブジェクトを配置する
			 */
			this_coloff = cache->color_offset * cache->color_index;
			/*  カラーリングオフセットのインデクスを更新する  */
			cache->color_index = 
				( cache->color_index + 1 ) % cache->color_num;
			kassert( krn_get_cpu_dcache_size() > this_coloff );
		}

		/*
		 *  SLABに空きオブジェクトを追加
		 */
		for( i = 0; cache->objs_per_slab > i; ++i) {  

			/* オブジェクトの直前にリスト構造を配置し, 
			 * 空きオブジェクトをリストの先頭に追加
			 */
			
			/*  バッファ制御情報を事前割当て済みカーネルキャッシュから獲得  */
			bctl = (kmem_bufctl *)kmalloc(sizeof(kmem_bufctl), mflags);
			if ( bctl == NULL ) {

				/*
				 * バッファ制御情報の割り当てに失敗したらSLABを破棄する
				 */
				free_slab_info(cache, sinfo);
				rc = -ENOMEM;
				goto error_out;
			}

			/*
			 * バッファ制御情報を初期化する
			 */
			/* バッファ制御情報へのリンク配置先アドレスを算出  */
			off_slab_objp = (void *)kaddr + this_coloff + cache->obj_size * i;
			/* バッファ制御情報へのリンクを設定  */
			*(kmem_bufctl **)off_slab_objp = bctl;
			/* オブジェクト配置先アドレスを算出  */
			bctl->obj = off_slab_objp + sizeof(kmem_bufctl *);

			/* 空きオブジェクトリストエントリを初期化  */
			slist_init_node(&bctl->link);  
			bctl->backlink = sinfo;  /* SLAB情報へのバックリンクを設定 */

			/*  空きオブジェクトリストに追加  */
			slist_add(&sinfo->objects, &bctl->link); 
		}
	}

	*sinfop = sinfo;  /*  SLAB管理情報を返却  */

	return 0;

free_obj_page_out:
	pgif_free_page(kaddr);  /*  オブジェクトページを解放する  */

error_out:
	return rc;
}

/**
   カーネルメモリキャッシュを縮小する
   @param[in] cache  カーネルキャッシュ管理情報
   @param[in] reap_flags 解放条件
 */
void
slab_kmem_cache_reap(kmem_cache *cache, int reap_flags){
	intrflags      iflags;
	slab           *sinfo;

	/* 空きSLABキューを操作するためキャッシュロックを獲得 */
	spinlock_lock_disable_intr(&cache->lock, &iflags);
	while( !queue_is_empty(&cache->free) ) {

		/*
		 * 空きキャッシュがあれば解放する
		 */
		if ( ( !( reap_flags & SLAB_REAP_FORCE ) ) &&
		    ( cache->slab_count <= cache->limits ) )
			break;  /*  最低保持数以下になったので解放を中止  */
			
		/*
		 * SLAB管理情報をキューから外して解放する
		 */

		/* フリーキューの先頭のSLAB管理情報を取り出し, 
		 * メモリ獲得処理と衝突しないようにする
		 */
		sinfo = container_of(queue_get_top(&cache->free), slab, link);
		--cache->slab_count;  /*  確保済みSLAB数を減算する  */

		/* SLAB管理情報解放のためキャッシュロックを解放 */
		spinlock_unlock_restore_intr(&cache->lock, &iflags);

		/*  SLAB管理情報を解放する
		 *  空きリストから取り外しているので
		 *  他に参照者はいない
		 */
		free_slab_info(cache, sinfo);
		/* 空きSLABキューを操作するためキャッシュロックを獲得 */
		spinlock_lock_disable_intr(&cache->lock, &iflags);
	}

	/* キャッシュロックを解放 */
	spinlock_unlock_restore_intr(&cache->lock, &iflags);

	return;
}

/**
   カーネルメモリキャッシュからオブジェクトを割り当てる
   @param[in]  cache  カーネルメモリキャッシュ
   @param[in]  mflags メモリ獲得時のフラグ指定
   @param[out] objp   獲得したオブジェクトを指し示すポインタ変数のアドレス
   @retval     0      正常終了
   @retval    -ENOMEM メモリ不足
 */
int
slab_kmem_cache_alloc(kmem_cache *cache, pgalloc_flags mflags, void **objp){
	int                rc;
	intrflags      iflags;
	slab           *sinfo;
	void     *alloced_obj;

	/*
	 * パーシャルキューのSLABからメモリを獲得する
	 */

	spinlock_lock_disable_intr(&cache->lock, &iflags); /* キャッシュロックを獲得 */
	if ( !queue_is_empty(&cache->partial) ) {

		/*  途中まで使用中のSLABからオブジェクトを獲得  */
		sinfo = container_of(queue_ref_top(&cache->partial), slab, link);
	} else {  /* 使用中のSLABがなければ  */

		if ( !queue_is_empty(&cache->free) ) { /*  割り当て済み空きSLABがある場合 */
			
			/*
			 * フリーキューから空きSLABを取り出す
			 */
			sinfo = container_of(queue_get_top(&cache->free), slab, link);
		} else {  /*  SLABを新たに割り当てる場合   */
			
			/* SLABを割り当てによる休眠を考慮し, キャッシュロックを解放 */
			spinlock_unlock_restore_intr(&cache->lock, &iflags); 
			
			/*  新規にSLABを割り当て  */
			rc = slab_kmem_cache_grow(cache, mflags, &sinfo);
			if ( rc != 0 )
				goto error_out;
			
			/* キャッシュロックを獲得 */
			spinlock_lock_disable_intr(&cache->lock, &iflags);
			++cache->slab_count;  /*  確保済みSLAB数をインクリメントする  */
		}

		/* フリーキューから取り出したSLABまたは新規に割り当てた
		 * SLABをパーシャルキュー(使用中SLABのリスト)につなぐ
		 */
		kassert( list_not_linked(&sinfo->link) );  /* 他のキューに接続されていない */
		queue_add(&cache->partial, &sinfo->link); /* パーシャルキューにつなぐ */
	}
	/* キャッシュロック獲得中にSLABの使用オブジェクト数を
	 * インクリメントすることで, パーシャルキューにつないだSLABが
	 * 空きオブジェクトリスト操作中に空きスラブキューに接続されないようにする
	 * (オブジェクト開放操作との衝突を避ける)。
	 */
	++sinfo->count;

	/* SLABのオブジェクトカウントが最大オブジェクト数なら
	 * フルキューにつなぎなおす
	 */
	if ( sinfo->count == cache->objs_per_slab ) {

		kassert( !list_not_linked(&sinfo->link) );  /* パーシャルキューに接続 */
		queue_del(&cache->partial, &sinfo->link); /* パーシャルキューから取り出し */
		queue_add(&cache->full, &sinfo->link);  /* フルキューに接続 */
	}

	/* キャッシュロック解放 */
	spinlock_unlock_restore_intr(&cache->lock, &iflags); 

	/*
	 * SLABからメモリオブジェクトを獲得
	 */
	alloc_slab_obj(cache, sinfo, &alloced_obj);
	if ( !( mflags & KM_SFLAGS_CLR_NONE ) )
		memset(alloced_obj, 0, cache->payload_size );  /*  メモリをクリアする  */

	*objp = alloced_obj;  /*  オブジェクトを返却する  */

	return 0;

error_out:
	return rc;
}

/**
   オブジェクトをカーネルメモリキャッシュに返却する
   @param[in] obj   解放するオブジェクト
 */
void
slab_kmem_cache_free(void *obj){
	int                 rc;
	intrflags       iflags;
	slab            *sinfo;
	void           *bufctl;
	kmem_cache      *cache;
	page_frame         *pf;

	/* オブジェクトが配置されているページのページフレーム情報を取得  */
	rc = pfdb_kvaddr_to_page_frame(obj, &pf);
	kassert( rc == 0 );

	if ( PAGE_IS_CLUSTERED(pf) )
		pf = pf->headp;  /*  クラスタページの場合先頭ページを参照する  */

	kassert( PAGE_USED_BY_SLAB(pf) );  /*  SLABとして使用していることを確認 */

	sinfo = pf->slabp;     /*  SLAB管理情報を得る  */
	cache = sinfo->cache;  /*  キャッシュ管理情報を得る  */

	spinlock_lock_disable_intr(&cache->lock, &iflags); /* キャッシュロックを獲得 */

	kassert( sinfo->count > 0 );  /*  2重解放されていないことを確認  */

	/*  キューから取り外す  */
	if ( sinfo->count == cache->objs_per_slab )  /*  フルキューに接続中  */
		queue_del(&cache->full, &sinfo->link);   /* フルキューから取り出し */
	else
		queue_del(&cache->partial, &sinfo->link); /* パーシャルキューから取り出し */

	--sinfo->count;  /*  割り当て済みオブジェクト数を減算  */

	/*  割り当て済みオブジェクト数が0でなければパーシャルキューにつなぎ替える
	 *  割り当て済みオブジェクト数が0ならリンクから外したままにし, 
	 *  獲得と衝突しないようにする
	 *  フリーキューに接続しないことで, カウンタ更新後の空きオブジェクトリスト操作中に
	 *  SLAB管理情報が解放されないようにしている
	 */
	if ( sinfo->count > 0 ) {  /* SLAB中に割り当て済みオブジェクトが存在する  */

		/* パーシャルキューに接続 
		 */
		kassert( list_not_linked(&sinfo->link) ); /* 他のキューに接続されていない */
		queue_add(&cache->partial, &sinfo->link); /* パーシャルキューに接続 */
	}

	/* キャッシュロックを解放 */
	spinlock_unlock_restore_intr(&cache->lock, &iflags);

	/*
	 * バッファ制御情報を取得
	 */
	if ( ( cache->align == 0 ) && ( KMEM_CACHE_IS_ON_SLAB(cache) ) ) { 		

		/* アライメントなしで, ON SLABの場合は, バッファ制御情報の直後に
		 * オブジェクトが配置されている
		 */
		bufctl = (void *)obj - sizeof(kmem_s_bufctl);
	} else {  

		/* OFF SLABやアライメント付き割り当ての場合は, バッファ制御情報へのポインタが
		 * オブジェクトの直前に配置されている
		 */
		bufctl = *(void **)((void *)obj - sizeof(void *));
	}

	if ( cache->destructor != NULL )
		cache->destructor(obj, cache->payload_size);  /*  デストラクタ呼び出し  */

	free_slab_obj(cache, sinfo, bufctl);  /*  オブジェクトを返却する  */

	/* SLAB管理情報のキューを操作するためにキャッシュロックを獲得 */
	spinlock_lock_disable_intr(&cache->lock, &iflags); 

	/* 割り当て済みオブジェクトがなければ,  フリーキューをつなぎ替える  */
	if ( sinfo->count == 0 ) { 

		kassert( list_not_linked(&sinfo->link) );  /*  他のキューにつながっていない */
		queue_add(&cache->free, &sinfo->link); /* フリーキューに接続する  */
	}

	spinlock_unlock_restore_intr(&cache->lock, &iflags);  /* キャッシュロックを解放 */
}

/**
   カーネルメモリキャッシュを初期化する
   @param[in] cache キャッシュ管理領域
   @param[in] name  キャッシュ名
   @param[in] limits slab_kmem_cache_reap時でも確保しておくSLAB数(単位:個)
   @param[in] sflags メモリ獲得条件フラグ
   @param[in] size  格納するオブジェクトのサイズ
   @param[in] align オブジェクト割り当て時のアラインメント
   @param[in] constructor オブジェクト割り当て時の初期化処理関数
   @param[in] destructor  オブジェクト解放時の解放処理関数
   @retval  0      正常終了
   @retval -EINVAL 奇数アラインメントを指定した
   @retval -ENOMEM メモリ不足
   @retval -ESRCH  格納可能なページオーダを越えている
 */
int 
slab_kmem_cache_create(kmem_cache *cache, const char *name, size_t size,
		uintptr_t align, obj_cnt_type limits, slab_flags sflags, 
		void (*constructor)(void *_obj, size_t _siz),
		void (*destructor)(void *_obj, size_t _siz)){
	int rc;

	/*
	 * カーネルメモリキャッシュを初期化する
	 */
	memset(cache, 0, sizeof(kmem_cache));
	spinlock_init(&cache->lock);  /*  ロックを初期化  */
	cache->name = name;    /*  キャッシュ名を設定  */
	cache->payload_size = size; /*  ペイロードサイズを設定  */

	cache->align = 0;        /*  アライメントを初期化  */

	cache->color_offset = 0; /*  キャッシュラインオフセットを初期化  */
	cache->color_num = 0;    /*  キャッシュラインオフセット数を初期化  */
	cache->color_index = 0;  /*  最初のオフセット  */

	cache->limits = limits;  /* 保持SLAB数を設定  */

	queue_init(&cache->partial); /* パーシャルキューを初期化 */
	queue_init(&cache->full);    /* フルキューを初期化 */
	queue_init(&cache->free);    /* フリーキューを初期化 */

	cache->constructor = constructor;  /* コンストラクタを設定 */
	cache->destructor = destructor;    /* デストラクタを設定 */

	/*
	 * オブジェクトページの長さを算出
	 */
	rc = calc_slab_size(cache, size, align, sflags);
	if ( rc != 0 )
		goto error_out;  /*  割り当て可能なサイズを超えている  */

	return 0;

error_out:
	return rc;
}

/**
   キャッシュを破棄する
   @param[in] cache キャッシュ管理情報
 */
void
slab_kmem_cache_destroy(kmem_cache *cache) {
	intrflags       iflags;

	/* 使用中のキャッシュがあれば解放を取りやめ, そうでなければ
	 * Freeキャッシュが空になるまでキャッシュを解放する
	 */
	spinlock_lock_disable_intr(&cache->lock, &iflags); /* キャッシュロックを獲得 */
	while( !queue_is_empty(&cache->free) ) {
		
		if ( KMEM_CACHE_IS_BUSY(cache) ) 
			goto unlock_out;  /* 使用中のSLABがあるため解放を取りやめ  */

		/* 空きスラブ解放前にキャッシュロックを解放 */
		spinlock_unlock_restore_intr(&cache->lock, &iflags);

		slab_kmem_cache_reap(cache, SLAB_REAP_FORCE);  /*  空きスラブを解放  */

		/* SLAB管理情報キューを操作するためにキャッシュロックを獲得 */
		spinlock_lock_disable_intr(&cache->lock, &iflags); 
	}

unlock_out:
	/* キャッシュロックを解放 */
	spinlock_unlock_restore_intr(&cache->lock, &iflags);  

	return ;
}

/**
   事前割当て済みカーネルキャッシュからメモリを割り当てる
   @param[in] size   割り当てるメモリサイズ
   @param[in] mflags メモリ獲得条件
   @return 割り当てたメモリ領域のアドレスまたはメモリ割り当て失敗時はNULLを返却
 */
void *
kmalloc(size_t size, pgalloc_flags mflags){
	int           rc;
	unsigned int   i;
	void          *m;

	for(i = 0; SLAB_PREALLOC_CACHE_NR > i; ++i) {
		
		/* 事前割当て済みカーネルキャッシュ中から指定されたサイズの
		 * メモリを格納可能なキャッシュを検索
		 */
		if ( ( SLAB_PREALLOC_BASE << i) >= size ) {

			/* 事前割当て済みカーネルキャッシュからメモリを割り当て
			 */
			rc = slab_kmem_cache_alloc(&prealloc_caches[i], mflags, &m);
			if ( rc != 0 )
				goto fail;
			return m;
		}
	}
fail:
	return NULL;
}

/**
   事前割当て済みカーネルキャッシュにメモリを返却する
   @param[in] m  返却するメモリ領域へのポインタ
 */
void
kfree(void *m){
	int                 rc;
	page_frame         *pf;

	if ( m == NULL )
		return;  /* NULLを指定された場合何もせず抜ける  */

	/*  返却するメモリ領域のページフレーム情報を取得  */
	rc = pfdb_kvaddr_to_page_frame(m, &pf);
	if ( rc != 0 )
		return;

	if ( PAGE_IS_CLUSTERED(pf) )
		pf = pf->headp;  /*  クラスタページの場合先頭ページを参照する  */
	
	/*
	 * SLABページの場合, メモリオブジェクトをSLABに返却する
	 */
	if ( PAGE_USED_BY_SLAB(pf) )
		slab_kmem_cache_free(m);  /*  メモリオブジェクトをSLABに返却する  */
}

/**
   事前割当て済みキャッシュを初期化する
 */
void
slab_prepare_preallocate_cahches(void){
	int           rc;
	unsigned int   i;

	for(i = 0; SLAB_PREALLOC_CACHE_NR > i; ++i) {

		/*  事前割当て済みキャッシュを初期化する  */
		rc = slab_kmem_cache_create(&prealloc_caches[i], 
		    prealloc_caches_info[i].name, prealloc_caches_info[i].size,
		    SLAB_PREALLOC_ALIGN,  SLAB_PREALLOC_LIMIT, KMALLOC_NORMAL, 
		    NULL, NULL);
		kassert( rc == 0 );
	}
}

/**
   事前割当て済みキャッシュを解放する
 */
void
slab_finalize_preallocate_cahches(void){
	unsigned int   i;

	for(i = SLAB_PREALLOC_CACHE_NR;  i > 0 ; --i) {

		/*  事前割当て済みキャッシュを解放する
		 *  サイズの大きなSLABから解放することで
		 *  SLAB管理情報やバッファ管理情報が最後に解放されるようにする
		 */
		slab_kmem_cache_destroy(&prealloc_caches[i - 1]);
	}
}




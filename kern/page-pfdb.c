/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Page Frame Data Base                                              */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>

#include <kern/page-if.h>
#include <kern/spinlock.h>

//#define DEQUEUE_PAGE_DEBUG  /*  デバッグ情報表示  */

/** ページフレームデータベース
 */
static page_frame_db g_pfdb = __PFDB_INITIALIZER(&g_pfdb.dbroot);

/** ページフレームデータベースRB木
 */
static int _pfdb_ent_cmp(struct _pfdb_ent *_key, struct _pfdb_ent *_ent);
RB_GENERATE_STATIC(_pfdb_tree, _pfdb_ent, ent, _pfdb_ent_cmp);

/** 
    ページフレームデータベースエントリ比較関数
    @param[in] key 比較対象領域1
    @param[in] ent RB木内の各エントリ
    @retval 正  keyの領域全体が entより前にある
    @retval 負  keyの領域全体が entより後にある
    @retval 0   keyの領域全体が entに含まれる
 */
static int 
_pfdb_ent_cmp(struct _pfdb_ent *key, struct _pfdb_ent *ent){
	
	if ( key->max_pfn <= ent->min_pfn )
		return 1;

	if ( ent->max_pfn <= key->min_pfn )
		return -1;

	kassert( ( ent->min_pfn <= key->min_pfn ) &&
		 ( key->max_pfn < ent->max_pfn ) );

	return 0;	
}

/** ページフレーム番号からページフレーム情報を得る
    @param[in]  pfn   ページフレーム番号
    @param[out] pagep ページフレーム情報を指し示すポインタのアドレス
    @retval  0     正常終了
    @retval -ESRCH 指定されたページフレーム番号に対応するページがなかった
 */
static int
pfn_to_page_frame(obj_cnt_type pfn, page_frame **pagep){
	int           rc;
	pfdb_ent     key;
	pfdb_ent    *res;
	int          idx;	
	intrflags iflags;

	/* ページフレームDBから指定されたページフレーム番号を
	 * 含むエントリを取り出す
	 */
	memset(&key, 0, sizeof(pfdb_ent));
	key.min_pfn = pfn;
	key.max_pfn = pfn+1;

	/**  ページフレームDBから指定されたページフレーム番号に対応する
	 * ページフレーム情報を得る
	 */
	spinlock_lock_disable_intr(&g_pfdb.lock, &iflags);
	res = RB_FIND(_pfdb_tree, &g_pfdb.dbroot, &key); 
	if ( res == NULL ) {  /*  対応するページフレーム情報が見つからなかった */

		rc = -ESRCH;
		goto unlock_out;
	}

	/* pfnに対応するページフレーム情報を算出する
	 */
	idx = pfn - res->min_pfn;  /*  配列のインデクスを獲得  */
	*pagep = &res->page_pool.array[idx];  /* ページフレーム情報を返却  */

	rc = 0;

unlock_out:
	spinlock_unlock_restore_intr(&g_pfdb.lock, &iflags);
	return rc;
}

/**
   ページフレーム情報の利用用途を更新する
 */
static void
mark_page_usage(page_frame *pf, page_usage usage){
	obj_cnt_type pages;

	/*  ページプールロックを獲得中に呼び出されることを確認  */
	kassert( spinlock_locked_by_self(&pf->buddyp->lock) );
	
	pages = 1 << pf->order;  /*  獲得ページ数  */

	/*
	 * ページの利用用途を更新する
	 */
	switch(usage){
		
	case PAGE_USAGE_KERN:

		/** その他カーネルデータページの利用ページ数を加算する
		 */
		PAGE_MARK_KERN(pf);
		pf->buddyp->kdata_pages += pages;
		break;
	case PAGE_USAGE_KSTACK:

		/** カーネルスタックページの利用ページ数を加算する
		 */
		PAGE_MARK_KSTACK(pf);
		pf->buddyp->kstack_pages += pages;
		break;
	case PAGE_USAGE_PGTBL:

		/** ページテーブルページの利用ページ数を加算する
		 */
		PAGE_MARK_PGTBL(pf);
		pf->buddyp->pgtbl_pages += pages;
		break;
	case PAGE_USAGE_SLAB:

		/** SLABページの利用ページ数を加算する
		 */
		PAGE_MARK_SLAB(pf);
		pf->buddyp->slab_pages += pages;
		break;
	case PAGE_USAGE_ANON:

		/** 無名ページの利用ページ数を加算する
		 */
		PAGE_MARK_ANON(pf);
		pf->buddyp->anon_pages += 1 << pf->order;
		break;
	case PAGE_USAGE_PCACHE:

		/** ページキャッシュの利用ページ数を加算する
		 */
		PAGE_MARK_PCACHE(pf);
		pf->buddyp->pcache_pages += 1 << pf->order;
		break;
	default:
		kassert_no_reach();
		break;
	}
}

/**
   ページフレーム情報の利用用途をクリアする
 */
static void
unmark_page_usage(page_frame *pf){
	obj_cnt_type pages;

	/*  ページプールロックを獲得中に呼び出されることを確認  */
	kassert( spinlock_locked_by_self(&pf->buddyp->lock) );
	
	pages = 1 << pf->order;  /*  解放ページ数  */

	if ( PAGE_USED_BY_KERN(pf) ) {

		/** その他カーネルデータページの利用を終了する
		 */
		PAGE_UNMARK_KERN(pf);
		pf->buddyp->kdata_pages -= pages;
	}

	if ( PAGE_USED_BY_KSTACK(pf) ) {

		/** カーネルスタックページの利用を終了する
		 */
		PAGE_UNMARK_KSTACK(pf);
		pf->buddyp->kstack_pages -= pages;
	}

	if ( PAGE_USED_BY_PGTBL(pf) ) {

		/** ページテーブルページの利用を終了する
		 */
		PAGE_UNMARK_PGTBL(pf);
		pf->buddyp->pgtbl_pages -= pages;
	}

	if ( PAGE_USED_BY_SLAB(pf) ) {

		/** ページの利用を終了する
		 */
       		PAGE_UNMARK_SLAB(pf);
		pf->buddyp->slab_pages -= pages;
	}

	if ( PAGE_USED_BY_ANON(pf) ) {

		/** ページの利用を終了する
		 */
		PAGE_UNMARK_ANON(pf);
		pf->buddyp->anon_pages -= pages;
	}

	if ( PAGE_USED_BY_PCACHE(pf) ) {

		/** ページの利用を終了する
		 */
		PAGE_UNMARK_PCACHE(pf);
		pf->buddyp->pcache_pages -= pages;
	}
}

/**
   クラスタページの設定を行う
   @param[in] pf ページフレーム情報
 */
static void
setup_clustered_pages(page_frame *pf) {
	obj_cnt_type      i;

	if ( pf->order == 0 ) {  /*  クラスタ化されていないページ  */

		PAGE_UNMARK_CLUSTERED(&pf[0]);  
		return;
	}

	/*
	 * 各ページのページフレーム情報の状態をクラスタページに設定し,
	 * 先頭ページを記録する
	 */
	for(i = 0; ( ULONG_C(1) << pf->order) > i; ++i) 
		PAGE_MARK_CLUSTERED(&pf[i], &pf[0]);  
}

/**
   クラスタページの設定を解除する
   @param[in] pf ページフレーム情報
 */
static void
clear_clustered_pages(page_frame *pf) {
	obj_cnt_type      i;

	/*
	 * 各ページのページフレーム情報の状態をクラスタページの設定を解除し,
	 * 先頭ページをNULLに設定する
	 */
	for(i = 0; ( ULONG_C(1) << pf->order) > i; ++i) 
		PAGE_UNMARK_CLUSTERED(&pf[i]);
}

/** 指定されたページフレームのページオーダを下げる (内部関数) 
    @param[in]  pf            ページフレーム情報
    @param[in]  request_order 要求ページオーダ
 */
static void
adjust_page_order(page_frame *pf, page_order request_order){
	pfdb_ent          *pfdb;
	page_buddy        *pool;
	page_order    cur_order;
	page_order   orig_order;
	page_frame  *buddy_page;
	queue         *pg_queue;
	int                 idx;
	obj_cnt_type  buddy_idx;

	pool = pf->buddyp;    /*  ページプール情報を参照  */

	kassert( spinlock_locked_by_self(&g_pfdb.lock) );  /* PFDBロック獲得済み */
	kassert( spinlock_locked_by_self(&pool->lock) );   /* ページプールロック獲得済み */

	pfdb= pool->pfdb_ent;  /* ページフレームDBエントリを参照  */
	idx = pf->pfn - pfdb->min_pfn;  /*  配列のインデクスを獲得  */

	/*  要求ページオーダよりページオーダが大きい場合は後続のページを
	 *  ページプールに返却する
	 */
	orig_order = cur_order = pf->order;
	while( cur_order > request_order ) {

		--cur_order;  /* オーダを一段落とす  */

		buddy_idx = idx ^ ( 1 << cur_order ); /*  バディページのインデクスを算出  */
		/*  バディページのインデクスが配列の要素数を超えることはない  */
		kassert(pool->nr_pages > buddy_idx);

		/*  バディページのページフレーム情報を取得  */
		buddy_page = &pool->array[buddy_idx]; 
		buddy_page->order = cur_order;           /*  バディページのオーダを更新     */
		pg_queue = &pool->page_list[cur_order];  /*  対象オーダのページキューを参照 */
		queue_add(pg_queue, &buddy_page->link);  /*  バディページをキューに追加     */
		++pool->free_nr[cur_order];              /*  空きページ数を更新             */

		pf->order = cur_order;                   /*  自ページのオーダを更新         */
	}

	if ( pf->order != request_order ) { /*  ページオーダを更新できなかった  */
		
		kprintf(KERN_PNC "adjust page order was failed:%p "
		    "pfn:%u flags=%x order:%d reqest-order:%d"
		    "original-order:%d\n", pf, pf->pfn,
		    pf->state, pf->order, request_order, orig_order);
		kassert_no_reach();
	}

	return;
}

/** 
    バディプールにページを追加する(内部関数)
    @param[in] pool      追加対象のバディプール
    @param[in] req_page  追加するページのページフレーム情報
 */
static void
enqueue_page_to_buddy_pool(page_buddy *pool, page_frame *req_page) {
	page_order         order;
	page_order     cur_order;
	obj_cnt_type     cur_idx;
	obj_cnt_type   buddy_idx;
	page_frame         *base;
	page_frame     *cur_page;
	page_frame   *buddy_page;
	queue              *area;
	page_order_mask     mask;

	kassert( pool->array != NULL );
	kassert( spinlock_locked_by_self(&pool->lock) );
	
	cur_order = order = req_page->order;  /*  ページオーダを取得  */
	kassert( order < PAGE_POOL_MAX_ORDER);

	/*  
	 *  バディページのページフレーム番号を取得するマスク
	 */
	mask =  ( ~((page_order_mask)0) ) << order;

	/*  バディの先頭ページを取得  */
	base = &pool->array[0];

	/*  ページ配列のインデクスを算出  */
	kassert( (uintptr_t)req_page >= (uintptr_t)base );
	cur_idx = req_page - base;

	/*  ページインデクスが対象オーダのページであることを確認  */
	kassert( !(cur_idx & ~mask) );

	/*  解放対象のページのキューを取得  */
	area = &pool->page_list[cur_order];

	/*  利用用途をクリアする  */	
	unmark_page_usage(req_page);    
	/*  ページを開放状態に設定  */
	PAGE_UNMARK_USED(req_page);

	/*  最大オーダまでページの解放処理を繰り返す  */
	while(cur_order < PAGE_POOL_MAX_ORDER ) {
		
		/*  ページキューが対象のバディ内になければならない  */
		kassert( ( (uintptr_t)area ) < 
		    ( (uintptr_t)&pool->page_list[PAGE_POOL_MAX_ORDER] ) );

		/*  ページインデクスがバディのページ数内に収まらなければならない  */
		kassert(cur_idx < pool->nr_pages);

		/*  格納対象のページフレーム情報を更新  */
		cur_page = &base[cur_idx];

		/*  格納対象のページオーダを更新  */
		cur_page->order = cur_order;

		/*  配列内に格納不能なオーダだった場合は, 接続不能  */
		if ( cur_order >= (PAGE_POOL_MAX_ORDER - 1) )
			break;

		/*  バディページのインデクスを算出  */
		buddy_idx = cur_idx ^ ( 1 << cur_order );

		/*  バディページのインデクスが配列の要素数を超える場合は接続不能  */
		if (buddy_idx >= pool->nr_pages) 
			break;

		/*  バディページのページフレーム情報を取得  */
		buddy_page = &base[buddy_idx];

#if  defined(ENQUEUE_PAGE_LOOP_DEBUG)
		kprintf(KERN_DBG "enque-dbg mask 0x%lx cur_idx %u, buddy_idx %u "
		    "cur:%p buddy:%p array%p[%d]\n", 
		    mask, cur_idx, buddy_idx, cur_page, buddy_page, 
		    area, cur_order);
#endif  /*  ENQUEUE_PAGE_LOOP_DEBUG  */

		/*  
		 *  互いのオーダが異なる場合は接続不能
		 */
		if (cur_page->order != buddy_page->order)
			break;

		/*  いずれかのページが使用中だった場合は接続不能  */
		if ( PAGE_STATE_NOT_FREED(buddy_page) )
			break;

		/*  バディページをキューから外す  */
		queue_del(&pool->page_list[buddy_page->order], &buddy_page->link);
		--pool->free_nr[buddy_page->order];

		/*  ページオーダーを一段あげる  */
		mask <<= 1;
		++cur_order;

		/*  ページキューを更新する  */
		area = &pool->page_list[cur_order];

		/*  ページのインデクスを当該オーダでの先頭ページに更新する  */
		cur_idx &= mask;
	}

	/* 追加対象ページのインデクスと追加対象キューがレンジ内にあることを確認 */
	kassert(cur_idx < pool->nr_pages);
	cur_page = &base[cur_idx];

	kassert( cur_page->order < PAGE_POOL_MAX_ORDER );
	area = &pool->page_list[cur_page->order];

#if  defined(ENQUEUE_PAGE_DEBUG)
	kprintf(KERN_DBG "pfn %d addr:%p order=%d add into %p[%d]\n", 
	    cur_idx, (void *)( (uintptr_t)(cur_idx << PAGE_SHIFT) ), 
	    cur_page->order, area, (pool->free_nr[cur_page->order] + 1));
#endif  /*  ENQUEUE_PAGE_DEBUG  */

	clear_clustered_pages(cur_page);  /* ページクラスタ情報をクリアする */
	
	/*
	 * ページをキューに追加する  
	 */
	queue_add(area, &cur_page->link);  
	++pool->free_nr[cur_page->order];

	return;
}

/**
   所定のオーダのページを指定されたメモリ領域から取り出しページフレーム番号を返す
   @param[in]  ent    メモリ獲得を試みるメモリ領域のページフレームデータベースエントリ
   @param[in]  order  取得するページのオーダ
   @param[in]  usage  ページ利用用途
   @param[out] pfnp   取得したページのページフレーム番号を返却する領域
   @retval     0      正常にページを獲得した
   @retval    -EINVAL ページフレーム情報返却域が不正か要求したページオーダが不正
   @retval    -ENOMEM 空きページがない
*/
static int
dequeue_page_from_memory_area(pfdb_ent *ent, page_order order, 
    page_usage usage, obj_cnt_type *pfnp){
	int               rc;
	page_order cur_order;
	page_frame *cur_page;
	page_buddy     *pool;
	intrflags     iflags;

	if (order >= PAGE_POOL_MAX_ORDER) 
		return -EINVAL;  /* 要求したページオーダが不正 */

	/*  ページフレームDBロック獲得済みであることを確認  */
	kassert( spinlock_locked_by_self(&g_pfdb.lock) );  

	pool = &ent->page_pool;  /*  ページフレームDBエントリのページプール情報を参照  */

	spinlock_lock_disable_intr(&pool->lock, &iflags);  /*  ページプールロックを獲得 */

	cur_order = order;

	while (cur_order < PAGE_POOL_MAX_ORDER) { /* 空きページがあるキューを順番に調べる */

		if ( !queue_is_empty(&pool->page_list[cur_order]) ) {
			
			cur_page = container_of(
				queue_get_top(&pool->page_list[cur_order]),
				page_frame, link); /* 空きページを取り出す */
			--pool->free_nr[cur_order];
			
			/* 要求オーダまでページオーダを落とす */
			adjust_page_order(cur_page, order);

			/*
			 * ページ返却
			 */
			setup_clustered_pages(cur_page);  /* ページクラスタ情報を設定する */
			mark_page_usage(cur_page, usage); /* ページ利用用途を更新する  */

			PAGE_MARK_USED(cur_page); /* ページを使用中にする */

			*pfnp = cur_page->pfn;  /* ページフレーム番号を返却する */
			refcnt_set(&cur_page->usecnt, REFCNT_INITIAL_VAL);  /* 参照を上げる */
			
			rc = 0;
			goto unlock_out;
		}
		++cur_order;  /*  より上のオーダからページを切り出す  */
	}

	rc = -ENOMEM;
unlock_out:
	spinlock_unlock_restore_intr(&pool->lock, &iflags);   /*  ページプールロックを解放 */
	return rc;
}

/** 指定されたページフレーム番号のページを予約する (内部関数) 
    @param[in]  pfn   ページフレーム番号
    @retval  0     正常終了
    @retval -ESRCH 指定されたページフレーム番号に対応するページがなかった
 */
static int 
mark_page_frame_reserved(obj_cnt_type pfn){
	int                  rc;
	pfdb_ent            key;
	pfdb_ent           *res;
	page_buddy        *pool;
	page_frame          *pf;
	int                 idx;
	intrflags        iflags;

	/* ページフレームDBから指定されたページフレーム番号を
	 * 含むエントリを取り出す
	 */
	memset(&key, 0, sizeof(pfdb_ent));
	key.min_pfn = pfn;
	key.max_pfn = pfn+1;

	/**  ページフレームDBから指定されたページフレーム番号に対応する
	 * ページフレーム情報を得る
	 */
	spinlock_lock_disable_intr(&g_pfdb.lock, &iflags);
	res = RB_FIND(_pfdb_tree, &g_pfdb.dbroot, &key); 
	if ( res == NULL ) {  /*  対応するページフレーム情報が見つからなかった */

		rc = -ESRCH;
		goto unlock_out;
	}

	/* pfnに対応するページフレーム情報を算出する
	 */
	pool = &res->page_pool;    /*  ページプールを参照      */

	idx = pfn - res->min_pfn;  /*  配列のインデクスを獲得  */
	pf = &pool->array[idx];    /* ページフレーム情報を取得 */
	kassert( !( pf->state & PAGE_STATE_RESERVED ) ); /* 利用可能ページでない */

	/*  ページプールロックを獲得 */
	spinlock_lock_disable_intr(&pool->lock, &iflags);
	queue_del(&pool->page_list[pf->order], &pf->link); /* ページをキューから外す */
	--pool->free_nr[pf->order];                        /* 空きページ数を更新     */

	/*  クラスタページの場合後続のページをキューに返却する
	 */
	adjust_page_order(pf, 0);

	/* ページが使用されていないことを確認
	 */
	kassert( pfdb_ref_page_use_count(pf) == 0 ); 
	kassert( ( pf->state & PAGE_STATE_UCASE_MASK ) == 0 );
	kassert( !PAGE_IS_USED(pf) );

	setup_clustered_pages(pf);  /* ページクラスタ情報をクリアする */
	--pool->available_pages;    /* 利用可能ページ数を減算         */

	/*  ページプールロックを解放 */
	spinlock_unlock_restore_intr(&pool->lock, &iflags);

	PAGE_MARK_RESERVED(pf);     /* ページを予約中にする         */
	
	rc = 0;

unlock_out:
	spinlock_unlock_restore_intr(&g_pfdb.lock, &iflags);
	return rc;
}

/** 指定されたページフレーム番号のページの予約を解除する (内部関数)
    @param[in]  pfn   ページフレーム番号
    @retval  0     正常終了
    @retval -ENOLCK 指定されたページフレーム番号が予約されていない
    @retval -ESRCH  指定されたページフレーム番号に対応するページがなかった
 */
static int
unmark_page_frame_reserved(obj_cnt_type pfn){
	int                  rc;
	pfdb_ent            key;
	pfdb_ent           *res;
	page_buddy        *pool;
	page_frame          *pf;
	int                 idx;
	intrflags        iflags;

	/* ページフレームDBから指定されたページフレーム番号を
	 * 含むエントリを取り出す
	 */
	memset(&key, 0, sizeof(pfdb_ent));
	key.min_pfn = pfn;
	key.max_pfn = pfn+1;

	/**  ページフレームDBから指定されたページフレーム番号に対応する
	 * ページフレーム情報を得る
	 */
	spinlock_lock_disable_intr(&g_pfdb.lock, &iflags);
	res = RB_FIND(_pfdb_tree, &g_pfdb.dbroot, &key); 
	if ( res == NULL ) {  /*  対応するページフレーム情報が見つからなかった */

		rc = -ESRCH;
		goto unlock_out;
	}

	/* pfnに対応するページフレーム情報を算出する
	 */
	pool = &res->page_pool;    /*  ページプールを参照      */

	idx = pfn - res->min_pfn;  /*  配列のインデクスを獲得  */
	pf = &pool->array[idx];    /* ページフレーム情報を取得 */

	if ( !( pf->state & PAGE_STATE_RESERVED ) ) {

		rc = -ENOLCK;  /* 予約ページでない */
		goto unlock_out;
	}

	/* ページが使用されていないことを確認
	 */
	kassert( pfdb_ref_page_use_count(pf) == 0 );  
	kassert( ( pf->state & PAGE_STATE_UCASE_MASK ) == 0 ); 
	kassert( !PAGE_IS_USED(pf) );

	PAGE_UNMARK_RESERVED(pf);   /* ページ予約をクリアする     */

	/*  ページプールロックを獲得      */
	spinlock_lock_disable_intr(&pool->lock, &iflags);

	enqueue_page_to_buddy_pool(pool, pf); /* ページをキューに返却 */
	++pool->available_pages;  /*  利用可能ページ数を減算          */

	/*  ページプールロックを解放      */
	spinlock_unlock_restore_intr(&pool->lock, &iflags);
	
	rc = 0;

unlock_out:
	spinlock_unlock_restore_intr(&g_pfdb.lock, &iflags);
	return rc;
}

/**
   ページフレームDBのエントリの登録を抹消する (内部関数)
   @param[in] ent   ページフレームDBエントリ
   @retval  0      正常終了
   @retval -EBUSY  使用中ページが存在する
   @retval -ENOENT 指定されたページフレームDBのエントリが登録されていない
 */
static int
remove_pfdb_ent_common(pfdb_ent *ent){
	int               rc;
	pfdb_ent        *res;
	page_buddy     *pool;
	page_order     order;
	obj_cnt_type free_nr;
	intrflags     iflags;

	pool = &ent->page_pool;  /*  ページプールを参照  */
	kassert( spinlock_locked_by_self(&g_pfdb.lock) ); /* ページフレームDBロック獲得済み */

	spinlock_lock_disable_intr(&pool->lock, &iflags); /*  ページプールロックを獲得 */
	/*
	 *  空きページ数を算出する
	 */
	for(order = 0, free_nr = 0; PAGE_POOL_MAX_ORDER > order; ++order) {

		/*  ノーマルページ単位での総空きページ数を加算  */
		free_nr += pool->free_nr[order] << order;
	}

	if ( pool->available_pages != free_nr ) {
		
		rc = -EBUSY;  /*  使用中ページがある  */
		goto unlock_out;
	}

	/*
	 * ページフレームDBのエントリの登録を抹消する
	 */
	res = RB_REMOVE(_pfdb_tree, &g_pfdb.dbroot, ent);

	spinlock_unlock_restore_intr(&pool->lock, &iflags); /*  ページプールロックを解放 */

	if ( res == NULL ) {
		
		rc = -ENOENT;  /*  すでに登録抹消済み  */
		goto error_out;
	}

	return 0;

unlock_out:
	spinlock_unlock_restore_intr(&pool->lock, &iflags); /*  ページプールロックを解放 */
error_out:
	return rc;
}

/*
 * IF関数
 */

/**
   所定のオーダのページを取り出しページフレーム番号を返す
   @param[in]  order  取得するページのオーダ
   @param[in]  usage  ページ利用用途
   @param[out] pfnp   取得したページのページフレーム番号を返却する領域
   @retval     0      正常にページを獲得した
   @retval    -EINVAL ページフレーム情報返却域が不正か要求したページオーダが不正
   @retval    -ENOMEM 空きページがない
*/
int
pfdb_buddy_dequeue(page_order order, page_usage usage, obj_cnt_type *pfnp){
	int           rc;   
	pfdb_ent    *ent;
	obj_cnt_type pfn;
	intrflags iflags;

	/*
	 * ページフレームDBの全エントリを走査し, 空きページの獲得を試みる
	 */
	spinlock_lock_disable_intr(&g_pfdb.lock, &iflags);
	RB_FOREACH(ent, _pfdb_tree, &g_pfdb.dbroot) {

		/* 空きページの獲得を試みる
		 */
		rc = dequeue_page_from_memory_area(ent, order, usage, &pfn);
		if ( rc == 0 ) 
			*pfnp = pfn;  /*  ページが獲得できたらページフレーム番号を返却  */

		/*  エントリ内にメモリがない場合は, 次のエントリから獲得を試み,
		 *  メモリ不足以外のエラーがあった場合は, 即時に復帰する
		 */
		if ( rc != -ENOMEM )
			goto unlock_out;  
	}

	rc = -ENOMEM;  /*  メモリ不足によるメモリ獲得失敗  */

unlock_out:
	spinlock_unlock_restore_intr(&g_pfdb.lock, &iflags);
	return rc;
}

/**
   ページフレームDBのエントリを初期化する
   @param[in]  phys_start 連続した物理メモリ領域の開始物理アドレス
   @param[in]  length     連続した物理メモリ領域のサイズ
   @param[out] pfdbp      ページフレームDBエントリの返却領域
 */
void
pfdb_add(uintptr_t phys_start, size_t length, pfdb_ent **pfdbp){
	pfdb_ent     *pfdb;
	pfdb_ent      *res;
	obj_cnt_type     i;
	void        *kaddr;
	void *kvaddr_start;
	int            idx;
	page_buddy   *pool;
	page_frame    *pgf;
	size_t       total;
	intrflags   iflags;

	/*  領域先頭の物理メモリに対応するカーネルストレートマップ領域アドレスを得る  */
	hal_phys_to_kvaddr((void *)phys_start, &kvaddr_start);

	/*  領域の末尾にページフレームDBエントリを配置する  */
	pfdb = (pfdb_ent *)( (uintptr_t)kvaddr_start + length - sizeof(pfdb_ent));

	/*
	 * メンバ初期化
	 */
	RB_ENT_INIT(pfdb, ent);  /*< 赤黒木のエントリを初期化する  */
	pfdb->kvaddr = kvaddr_start;  /*< カーネル仮想アドレスでの開始アドレスを設定 */
	pfdb->length = length; /*< 登録する物理メモリ領域の長さ(単位:バイト)を設定  */

	/*  領域中の最小/最大ページフレーム番号を算出する  */
	hal_kvaddr_to_pfn(pfdb->kvaddr, &pfdb->min_pfn);
	hal_kvaddr_to_pfn((void *)PAGE_ROUNDUP(pfdb->kvaddr + pfdb->length - 1),
			  &pfdb->max_pfn);
	/*
	 * buddy poolを初期化する
	 */
	pool = &pfdb->page_pool;
	spinlock_init(&pool->lock);

	pool->nr_pages = pfdb->max_pfn - pfdb->min_pfn;  /*  領域中のページ数  */

	/* ページフレーム配列の開始アドレスを算出する
	 * (page/page-pfdb.h参照)
	 */
	pool->array = (void *)(pfdb) - (pool->nr_pages * sizeof(page_frame));
	pool->pfdb_ent = pfdb;  /* ページフレームDBエントリへの逆リンクを設定 */

	/* buddy管理情報を初期化
	 */
	for(idx = 0; PAGE_POOL_MAX_ORDER > idx; ++idx) {

		pool->free_nr[idx] = 0;
		queue_init(&pool->page_list[idx]);
	}

	/*
	 * ページフレーム配列初期化
	 */
	for(i = 0; pool->nr_pages > i; ++i) {

		pgf = &pool->array[i];
		/*
		 * ページフレーム情報の各要素を初期化
		 */
		spinlock_init(&pgf->lock);     /* ロックを初期化                  */
		list_init(&pgf->link);         /* ページキューへのリンクを初期化  */
		PAGE_MARK_RESERVED(pgf);       /* ページフレームを予約            */
		pgf->arch_state = 0;           /* アーキ依存ページ状態を初期化    */
		pgf->pfn = pfdb->min_pfn + i;  /* 最小ページ番号を初期化          */
		refcnt_init_with_value(&pgf->usecnt, 0);  /* 利用カウント初期化   */
		refcnt_init_with_value(&pgf->mapcnt, 0);  /* マップカウント初期化 */
		pgf->order = 0;                           /* ページオーダ初期化   */
		queue_init(&pgf->pv_head);  /* 物理->仮想アドレス変換キュー初期化 */
		pgf->buddyp = pool;         /* ページプールを設定                 */
		pgf->headp = NULL;          /* ページクラスタ情報初期化           */
		list_init(&pgf->lru_ent);   /* LRUエントリの初期化                */
		pgf->slabp = NULL;          /* SLABアドレスを初期化               */
		pgf->pcachep = NULL;        /* ページキャッシュアドレスを初期化   */
	}

	/*
	 * page frame dbに登録
	 */
	spinlock_lock_disable_intr(&g_pfdb.lock, &iflags);
	res = RB_INSERT(_pfdb_tree, &g_pfdb.dbroot, pfdb);
	spinlock_unlock_restore_intr(&g_pfdb.lock, &iflags);
	kassert( res == NULL );

	/* 利用可能なページをbuddy poolに返却する
	 */
	pool->available_pages = 0;  /*  利用可能ページ数を初期化  */
	for(i = 0; pool->nr_pages > i; ++i) {

		/*  ページフレーム配列やページフレームDBエントリを
		 *  配置した領域のページは予約したままとする
		 */
		/*  ページに対応したカーネルストレートマップ領域のアドレスを算出する  */
		hal_pfn_to_kvaddr(pfdb->min_pfn + i, &kaddr);

		/*  ページフレーム配列を含むページの先頭から
		 *  ページフレームDBエントリを含むページの末尾
		 *  (ページフレームDBエントリ配置ページの次のページの先頭アドレス)
		 *  までの範囲に算出したアドレスが含まれている場合は,
		 *  予約を解除しない
		 */
		if ( ( (void *)PAGE_TRUNCATE(&pool->array[0]) <= kaddr ) &&
		    ( kaddr <  (void *)PAGE_ROUNDUP(pfdb->kvaddr + pfdb->length - 1) ) ) 
			continue;

		/*
		 * ページフレーム配列以前のページの場合は, 予約を解除し,
		 * バディページキューに格納
		 */
		PAGE_UNMARK_RESERVED(&pool->array[i]);  /*  予約解除  */
		spinlock_lock_disable_intr(&pool->lock, &iflags);
		enqueue_page_to_buddy_pool(pool, &pool->array[i]); /* ページをキューに追加 */
		++pool->available_pages;  /*  利用可能ページ数を加算  */
		spinlock_unlock_restore_intr(&pool->lock, &iflags);
	}

	/*
	 * 初期化の検証
	 */
	total = 0;
	for(idx = 0; PAGE_POOL_MAX_ORDER > idx; ++idx) {

		/*  buddyプール内の総容量を算出  */
		total += pool->free_nr[idx] * (PAGE_SIZE << idx);
	}

	/*  buddyプール内の総容量 + ページフレーム配列+ページフレームDBエントリの領域長が
	 *  登録された物理メモリ領域と等しいことを確認する
	 */
	kassert( ( total + PAGE_ROUNDUP(pfdb->kvaddr + pfdb->length - 1) -
		PAGE_TRUNCATE(&pool->array[0]) ) == pfdb->length );

	*pfdbp = pfdb;  /*  ページフレームDBエントリを返却する  */

	return ;
}

/**
   ページフレームDBのエントリの登録を抹消する
   @param[in] ent   ページフレームDBエントリ
   @retval  0      正常終了
   @retval -EBUSY  使用中ページが存在する
   @retval -ENOENT 指定されたページフレームDBのエントリが登録されていない
 */
int
pfdb_remove(pfdb_ent *ent){
	int               rc;
	intrflags     iflags;

	spinlock_lock_disable_intr(&g_pfdb.lock, &iflags);  /* ページフレームDBのロック獲得 */

	rc = remove_pfdb_ent_common(ent);  /*  指定されたエントリを解放する  */

	/* ページフレームDBのロック解放 */
	spinlock_unlock_restore_intr(&g_pfdb.lock, &iflags);

	return rc;
}

/**
   ページフレームDBの全エントリを解放する
 */
void
pfdb_free(void){
	pfdb_ent     *nxt;
	pfdb_ent     *ent;
	intrflags  iflags;

	spinlock_lock_disable_intr(&g_pfdb.lock, &iflags);  /* ページフレームDBのロック獲得 */

	/*  ループ内で削除処理を行うのでRB_FOREACH_SAFEを使用  */
	RB_FOREACH_SAFE(ent, _pfdb_tree, &g_pfdb.dbroot, nxt) { /*  登録済み領域を探査 */
		
		remove_pfdb_ent_common(ent);  /*  指定されたエントリを解放する  */
	}

	/* ページフレームDBのロック解放 */
	spinlock_unlock_restore_intr(&g_pfdb.lock, &iflags);
}

/**
   指定された物理メモリ範囲を予約する
   @param[in]  start  開始アドレス
   @param[in]  end    終了アドレス
 */
void
pfdb_mark_phys_range_reserved(vm_paddr start, vm_paddr end){
	int              rc;
	vm_paddr page_start;
	vm_paddr   page_end;
	vm_paddr  resv_page;
	obj_cnt_type    pfn;

	/* ページ境界での開始, 終了アドレスを算出する
	 */
	page_start = PAGE_TRUNCATE(start);
	page_end = PAGE_ROUNDUP(end);

	/* 指定されたページをページプールから外して予約する
	 */
	for( resv_page = page_start; page_end > resv_page; resv_page += PAGE_SIZE) {

		pfn = resv_page >> PAGE_SHIFT;  /* ページフレーム番号を算出  */
		rc = mark_page_frame_reserved(pfn); /* ページを予約 */
		kassert( rc == 0 );
	}
}

/**
   指定された物理メモリ範囲の予約を解除する
   @param[in]  start  開始アドレス
   @param[in]  end    終了アドレス
 */
void
pfdb_unmark_phys_range_reserved(vm_paddr start, vm_paddr end){
	int              rc;
	vm_paddr page_start;
	vm_paddr   page_end;
	vm_paddr  resv_page;
	obj_cnt_type    pfn;

	/* ページ境界での開始, 終了アドレスを算出する
	 */
	page_start = PAGE_TRUNCATE(start);
	page_end = PAGE_ROUNDUP(end);

	/* 指定されたページの予約を解除する
	 */
	for( resv_page = page_start; page_end > resv_page; resv_page += PAGE_SIZE) {

		pfn = resv_page >> PAGE_SHIFT;  /* ページフレーム番号を算出  */
		rc = unmark_page_frame_reserved(pfn); /* 予約を解除 */
		kassert( rc == 0 );
	}
}

/**
   ストレートマップ領域アドレスに対応する物理アドレスを返却する
   @param[in]  kvaddr  ストレートマップ領域アドレス
   @param[out] paddrp  物理アドレス返却領域
   @retval  0     正常終了
   @retval -ESRCH 変換に失敗した
 */
int
pfdb_kvaddr_to_paddr(void *kvaddr, void **paddrp){
	int           rc;
	obj_cnt_type pfn;
	void      *paddr;

	rc = hal_kvaddr_to_pfn(kvaddr, &pfn);
	if ( rc != 0 ) {

		rc = -ESRCH;  /*  指定されたアドレスに対応するページフレームがない  */
		goto error_out;
	}

	rc = hal_pfn_to_paddr(pfn, &paddr);  
	if ( rc != 0 ) {

		rc = -ESRCH;  /*  ページフレーム番号に対応する物理ページがない  */
		goto error_out;
	}

	*paddrp = paddr;  /*  物理アドレスを返却する  */

	return  0;

error_out:
	return rc;
}
/**
   物理アドレスに対応するストレートマップ領域アドレスを返却する
   @param[in]  paddr    物理アドレス
   @param[out] kvaddrp  ストレートマップ領域アドレス返却領域
   @retval  0     正常終了
   @retval -ESRCH 変換に失敗した
 */
int
pfdb_paddr_to_kvaddr(void *paddr, void **kvaddrp){
	int           rc;
	obj_cnt_type pfn;
	void     *kvaddr;

	rc = hal_paddr_to_pfn(paddr, &pfn);
	if ( rc != 0 ) {

		rc = -ESRCH;  /*  指定された物理アドレスに対応するページフレームがない  */
		goto error_out;
	}

	rc = hal_pfn_to_kvaddr(pfn, &kvaddr);  
	if ( rc != 0 ) {

		rc = -ESRCH;  /*  指定されたページフレーム番号に対応するページがない  */
		goto error_out;
	}

	*kvaddrp = kvaddr;  /*  カーネルストレートマップ領域のアドレスを返却する  */

	return  0;

error_out:
	return rc;
}

/**
   ページフレーム番号に対するストレートマップ領域アドレスを返却する
   @param[in]  pfn     ページフレーム番号
   @param[out] kvaddrp ストレートマップ領域アドレス返却領域
   @retval  0     正常終了
   @retval -ESRCH 変換に失敗した
 */
int 
pfdb_pfn_to_kvaddr(obj_cnt_type pfn, void **kvaddrp){
	int rc;
	void *kvaddr;

	/*  ページフレーム番号からカーネルストレートマップ領域のアドレスを算出する  */
	rc = hal_pfn_to_kvaddr(pfn, &kvaddr);  
	if ( rc == 0 )
		*kvaddrp = kvaddr;  /*  カーネルストレートマップ領域のアドレスを返却する  */
	else
		rc = -ESRCH;  /*  指定されたページフレーム番号に対応するページがない  */

	return rc;
}

/**
   ストレートマップ領域アドレスに対するページフレーム番号を返却する
   @param[in]  kvaddr  ストレートマップ領域アドレス
   @param[out] pfnp     ページフレーム番号返却領域
   @retval  0     正常終了
   @retval -ESRCH 変換に失敗した
 */
int
pfdb_kvaddr_to_pfn(void *kvaddr, obj_cnt_type *pfnp){
	int           rc;
	obj_cnt_type pfn;

	/*  カーネルストレートマップ領域のアドレスに対応するページフレーム番号を算出する  */
	rc = hal_kvaddr_to_pfn(kvaddr, &pfn);
	if ( rc == 0 )
		*pfnp = pfn;  /*  ページフレーム番号を返却  */
	else
		rc = -ESRCH;  /* 指定されたアドレスに対応するページがない  */

	return rc;
}

/**
   ストレートマップ領域中のアドレスを含むノーマルページのページフレーム情報を返却する
   @param[in]  kvaddr ストレートマップ領域中のアドレス
   @param[out] pp    ページフレーム情報返却域
   @retval  0     正常終了
   @retval -ESRCH 変換に失敗した
 */
int
pfdb_kvaddr_to_page_frame(void *kvaddr, page_frame **pp){
	int           rc;
	obj_cnt_type pfn;
	page_frame   *pf;

	/* ストレートマップ領域のアドレスをページフレーム番号に変換  */
	rc = hal_kvaddr_to_pfn(kvaddr, &pfn);
	if ( rc != 0 ) {

		rc = -ESRCH;  /* 指定されたアドレスに対応するページがない  */
		goto error_out;
	}

	/*  ページフレーム番号を元にページフレーム情報を取得  */
	rc = pfn_to_page_frame(pfn, &pf);	
	kassert( rc == 0 );  /* 上記でページが登録済みであることを確認済みなので成功する  */

	*pp = pf;  /*  ページフレーム情報を返却  */

error_out:
	return rc;
}

/**
   ページフレーム番号に対するページフレーム情報を返却する
   @param[in]  pfn ページフレーム番号
   @param[out] pp  ページフレーム情報返却域
   @retval  0     正常終了
   @retval -ESRCH 変換に失敗した
 */
int
pfdb_pfn_to_page_frame(obj_cnt_type pfn, page_frame **pp){
	int         rc;
	page_frame *pf;

	rc = pfn_to_page_frame(pfn, &pf); /* ページフレーム番号からページフレーム情報を得る */
	if ( rc == 0 )
		*pp = pf;  /*  ページフレーム情報を返却する  */
	else
		rc = -ESRCH; /*  対応するページフレーム情報が見つからなかった  */

	return rc;
}

/**
   ページフレームのマップカウントを初期化する
   @param[in] pf ページフレーム
   @param[in] val マップカウントの初期値
 */
void
pfdb_page_map_count_init(page_frame *pf, refcounter_val val){

	refcnt_set(&pf->mapcnt, val);
}

/**
   ページフレームのマップカウントを返す 
   @param[in] pf ページフレーム
   @return マップカウント
 */
refcounter_val 
pfdb_ref_page_map_count(page_frame *pf){

	return refcnt_read(&pf->mapcnt);  /*  利用カウントを返す  */
}

/**
   ページフレームのマップカウントをあげる
   @param[in] pf ページフレーム
   @retval 真 ページフレームのマップカウント加算に成功した
   @retval 偽 ページフレームのマップカウント加算に失敗した
 */
bool
pfdb_inc_page_map_count(page_frame *pf){

	/*  マップ解放中でなければ, 利用カウンタを加算し, 加算前の値を返す  
	 */
	return ( refcnt_inc_if_valid(&pf->mapcnt) != 0 );  /* 以前の値が0の場合加算できない */
}

/**
   ページフレームのマップカウントを下げる
   @param[in] pf ページフレーム
   @retval 真 ページフレームの最終マップだった
   @retval 偽 ページフレームの最終マップでない
 */
bool
pfdb_dec_page_map_count(page_frame *pf){

	return refcnt_dec_and_test(&pf->mapcnt);
}

/**
   ページフレームの利用カウントを返す 
   @param[in] pf ページフレーム
   @return 利用カウント
 */
refcounter_val 
pfdb_ref_page_use_count(page_frame *pf){

	return refcnt_read(&pf->usecnt);  /*  利用カウントを返す  */
}

/**
   ページフレームの利用カウントをあげる
   @param[in] pf ページフレーム
   @retval 真 ページフレームのマップカウント加算に成功した
   @retval 偽 ページフレームのマップカウント加算に失敗した
 */
bool
pfdb_inc_page_use_count(page_frame *pf){

	/*  解放中でない場合のみ, 利用カウンタを加算し, 加算前の値を返す  
	 */
	return ( refcnt_inc_if_valid(&pf->usecnt) != 0 ); /* 以前の値が0の場合加算できない */
}

/**
   ページフレームの利用カウントを下げ, 利用されなくなったページを解放する
   @param[in] pf ページフレーム
   @retval 真 ページフレームの最終利用者だった
   @retval 偽 ページフレームの最終利用者でない
 */
bool
pfdb_dec_page_use_count(page_frame *pf){
	intrflags   iflags;
	bool            rc;

	kassert(pf->buddyp != NULL);

	rc = refcnt_dec_and_test(&pf->usecnt); /* 最終参照者であることを確認 */
	if ( rc ) { /*  利用カウントが0になったら解放する  */
		
		kassert( PAGE_IS_USED(pf) );  /*  多重開放でないことを確認  */

		/* マップされていないことを確認 */
		kassert( pfdb_ref_page_map_count(pf) == 0 ); 

		/* LRUにつながっていないことを確認 */
		list_not_linked(&pf->lru_ent);

		spinlock_lock_disable_intr(&pf->buddyp->lock, &iflags);
		enqueue_page_to_buddy_pool(pf->buddyp, pf);  /*  ページを解放する  */
		spinlock_unlock_restore_intr(&pf->buddyp->lock, &iflags);
	}

	return rc;
}

/**
   指定されたページフレーム番号に対応するページが存在することを確認する
   @param[in]  pfn   ページフレーム番号
   @retval  真  指定されたページフレーム番号に対応するページが存在する
   @retval  偽  指定されたページフレーム番号に対応するページが存在しない
 */
bool
kcom_is_pfn_valid (obj_cnt_type pfn){
	int rc;
	page_frame *pf;

	/*  ページフレーム番号に対応したページフレーム情報の取得を試みる
	 */
	rc = pfn_to_page_frame(pfn, &pf);  

	return (rc == 0);  /*  正常終了することを確認し, その結果を返却する  */
}

/**
   ページフレームDBの統計情報を取得する
   @param[out]   statp   ページフレームDB統計情報
 */
void
kcom_obtain_pfdb_stat(pfdb_stat *statp){
	int          order;
	pfdb_ent      *ent;
	page_buddy   *pool;
	intrflags   iflags;

	memset(statp, 0 , sizeof(pfdb_stat));

	spinlock_lock_disable_intr(&g_pfdb.lock, &iflags);

	RB_FOREACH(ent, _pfdb_tree, &g_pfdb.dbroot) {  /*  各物理メモリ領域を探査 */

		pool = &ent->page_pool;  /*  ページプールリストを獲得  */

		statp->nr_pages += pool->nr_pages;  /*  総ページ数に反映  */
		statp->available_pages += pool->available_pages;  /* 利用可能ページ数に反映 */
		 /* 予約ページ数算出  */
		statp->reserved_pages += pool->nr_pages - pool->available_pages; 
		/*
		 * ページ利用用途数を取得
		 */
		statp->kdata_pages += pool->kdata_pages;
		statp->kstack_pages += pool->kstack_pages;
		statp->pgtbl_pages += pool->pgtbl_pages;
		statp->slab_pages += pool->slab_pages;
		statp->anon_pages += pool->anon_pages;
		statp->pcache_pages += pool->pcache_pages;

		/*
		 * オーダ別空きページ情報を取得
		 */
		for(order = 0; PAGE_POOL_MAX_ORDER > order; ++order) {

			/*  ノーマルページ単位での総空きページ数を加算  */
			statp->nr_free_pages += pool->free_nr[order] << order;
			/*  オーダ単位での空きページ数を加算  */
			statp->free_nr[order] += pool->free_nr[order];	
		}
	}

	spinlock_unlock_restore_intr(&g_pfdb.lock, &iflags);
}

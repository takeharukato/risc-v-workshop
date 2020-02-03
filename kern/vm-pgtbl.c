/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Page Table                                                        */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>

#include <kern/page-if.h>
#include <kern/vm-if.h>

static kmem_cache pgtbl_cache;  /* ページテーブル情報のキャッシュ */
static vm_asid_map  g_asidmap;  /* アドレス空間IDマップ           */

/**
   アドレス空間IDマップを初期化する
 */
static void
init_asidmap(void){

	bitops_zero(&g_asidmap.idmap);  /* アドレス空間IDマップを初期化 */
	bitops_set(HAL_PGTBL_KERNEL_ASID, &g_asidmap.idmap);  /* カーネルIDを使用済みに設定 */
}

/**
   アドレス空間IDを割当てる (内部関数)
   @param[out] idp    アドレス空間ID返却域
   @retval     0      正常終了
   @retval    -ENOSPC IDに空きがない
 
  */
static int
alloc_asid(hal_asid *idp){
	tid         newid;

	newid = bitops_ffc(&g_asidmap.idmap);  /* 空きIDを取得 */
	if ( newid == 0 ) 
		return  -ENOSPC;  /* 空IDがない */

	--newid;  /* スレッドIDに変換 */

	/* 割当てたIDに対応するビットマップ中のビットを使用中にセット */
	bitops_set(newid, &g_asidmap.idmap); 

	if ( idp != NULL )
		*idp = newid;  /* スレッドIDを返却 */

	return 0;
}

/**
   アドレス空間IDを解放する (内部関数)
   @param[in] id    アドレス空間ID
   @retval    0     正常終了
   @retval   -ENOSPC IDに空きがない
  */
static void
release_asid(hal_asid id){

	bitops_clr(id, &g_asidmap.idmap);  /* IDを解放する */
}

/**
   ページテーブル用にページを割り当てる
   @param[in]  pgt    割当先のアドレス空間のページテーブル情報
   @param[out] tblp   ページテーブル用のページのカーネル仮想アドレス返却域
   @param[out] paddrp ページテーブル用のページの物理アドレス返却域
   @retval     0      正常終了
   @retval    -ENOMEM メモリ不足
 */
int
pgtbl_alloc_pgtbl_page(vm_pgtbl pgt, hal_pte **tblp, vm_paddr *paddrp){
	int          rc;
	void       *tbl;
	void     *paddr;

	/* ページテーブル割り当て
	 */
	rc = pgif_get_free_page(&tbl, KMALLOC_NORMAL, PAGE_USAGE_PGTBL);
	if ( rc != 0 ) {
		
		rc = -ENOMEM;   /* メモリ不足  */
		goto error_out;
	}
	
	/* ページテーブルの物理アドレスを算出 */
	rc = hal_kvaddr_to_phys(tbl, &paddr);
	kassert( rc == 0 );

	statcnt_inc(&pgt->nr_pages);  /* ページテーブルのページ数を加算 */

	/* ページテーブル用ページを返却
	 */
	*tblp = (hal_pte *)tbl;
	*paddrp = (vm_paddr)paddr;

	return 0;

error_out:
	return rc;
}

/**
   ページテーブルを割り当てる (カーネル/ユーザ共通処理)
   @param[out] pgtp 割り当てたページテーブル情報を指し示すポインタのアドレス
   @retval     0    正常終了
 */
int
pgtbl_alloc_pgtbl(vm_pgtbl *pgtp){
	int             rc;
	vm_pgtbl_type *pgt;

	/*  ページテーブル情報のキャッシュからメモリを割り当てる */
	rc = slab_kmem_cache_alloc(&pgtbl_cache, KMALLOC_NORMAL, (void **)&pgt);
	if ( rc != 0 ) {
	
		kprintf(KERN_PNC "Can not allocate kernel page table cache.\n");
		kprintf("Machine Dependent (MD) part might have not called "
		    "vm_pgtbl_cache_init().\n");
		kassert_no_reach();
	}

	/* テーブル以外の要素を初期化
	 * @note pgtbl_alloc_pgtbl_pageで統計情報を操作する前に初期化する必要がある
	 */
	spinlock_init(&pgt->lock);      /* ロックの初期化                      */
	bitops_zero(&pgt->active);      /* ビットマップを初期化                */
	mutex_init(&pgt->mtx);          /* ミューテックスの初期化              */
	statcnt_set(&pgt->nr_pages, 0); /* ページテーブルのページ数を0に初期化 */

	/* カーネルのページテーブルベースページを割り当てる
	 */
	rc = pgtbl_alloc_pgtbl_page(pgt, &pgt->pgtbl_base, &pgt->tblbase_paddr);
	if ( rc != 0 ) {
	
		kprintf(KERN_PNC "Can not allocate kernel page table base:rc = %d\n", rc);
		kassert_no_reach();
	}

	hal_pgtbl_init(pgt);            /* アーキ依存部の初期化                */

	pgt->p = NULL; /* TODO: プロセス管理実装後にカーネルプロセスを参照するように修正 */

	*pgtp = pgt;            /* ページテーブル情報を返却する */

	return 0;
}

/**
   ユーザ用ページテーブルを割り当てる
   @param[out] pgtp 割り当てたページテーブル情報を指し示すポインタのアドレス
   @retval     0    正常終了
   @retval    -ENODEV ミューテックスが破棄された
   @retval    -EINTR  非同期イベントを受信した
 */
int
pgtbl_alloc_user_pgtbl(vm_pgtbl *pgtp){
	int             rc;
	vm_pgtbl_type *pgt;
	hal_asid      asid;

	rc = alloc_asid(&asid);
	if ( rc != 0 )
		goto error_out;

	rc = pgtbl_alloc_pgtbl(&pgt);  /* ページテーブル情報を割り当てる */
	if ( rc != 0 )
		goto free_asid_out;

	pgt->asid = asid;  /* アドレス空間IDを設定  */

	rc = mutex_lock(&pgt->mtx);    /* ユーザ空間側のロックを獲得する  */
	if ( rc != 0 )
		goto free_pgtbl_out;

	rc = hal_copy_kernel_pgtbl(pgt);    /* カーネル空間部分を初期化する   */
	if ( rc != 0 )
		goto unlock_mtx_out;

	*pgtp = pgt;  /* ページテーブル情報を返却する */

	mutex_unlock(&pgt->mtx);      /* ユーザ空間側のロックを解放する  */

	return 0;


unlock_mtx_out:
	mutex_unlock(&pgt->mtx);      /* ユーザ空間側のロックを解放する  */

free_pgtbl_out:
	pgif_free_page(pgt->pgtbl_base);    /* ベースページテーブルを解放  */
	slab_kmem_cache_free((void *)pgt);  /* ページテーブル情報を解放    */

free_asid_out:
	release_asid(asid);  /* アドレス空間ID解放 */

error_out:
	return rc;
}

/**
   ユーザ用ページテーブルを解放する
   @param[in] pgt ページテーブル情報
 */
void
pgtbl_free_user_pgtbl(vm_pgtbl pgt){
	int rc;

	rc = mutex_lock(&pgt->mtx);              /* ミューテックスの獲得           */
	kassert( rc == 0 );  /* オブジェクト破棄にはならないはず */

	/* ベースページテーブル以外のテーブルが解放済みであることを確認する */
	kassert(statcnt_read(&pgt->nr_pages) == 1);  

	release_asid(pgt->asid);  /* アドレス空間ID解放 */

	pgif_free_page(pgt->pgtbl_base);    /* ベースページテーブルを解放  */

	mutex_unlock(&pgt->mtx);            /* ミューテックスの解放        */

	slab_kmem_cache_free((void *)pgt);  /* ページテーブル情報を解放    */

	return ;
}

/**
   ページテーブルキャッシュを初期化する
 */
void
vm_pgtbl_cache_init(void){
	int rc;

	/* ページテーブルキャッシュを初期化する
	 */
	rc = slab_kmem_cache_create(&pgtbl_cache, "pgtbl cache", sizeof(vm_pgtbl_type),
	    SLAB_ALIGN_NONE,  0, KMALLOC_NORMAL, NULL, NULL);
	kassert( rc == 0 );

	init_asidmap();  /* アドレス空間IDマップを初期化 */
}

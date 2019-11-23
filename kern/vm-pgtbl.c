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
#include <kern/vm.h>

static kmem_cache pgtbl_cache;  /* ページテーブル情報のキャッシュ */

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

	atomic_add_fetch(&pgt->nr_pages, 1);  /* ページテーブルのページ数を加算 */

	/* ページテーブル用ページを返却
	 */
	*tblp = (hal_pte *)tbl;
	*paddrp = (vm_paddr)paddr;

	return 0;

error_out:
	return rc;
}

/**
   ページテーブルを割り当てる
   @param[out] pgtp 割り当てたページテーブル情報を指し示すポインタのアドレス
   @retval     0    正常終了
 */
int
pgtbl_alloc_pgtbl(vm_pgtbl *pgtp){
	int             rc;
	vm_pgtbl_type *pgt;

	/*  ページテーブル情報のキャッシュからメモリを割り当てる */
	rc = slab_kmem_cache_alloc(&pgtbl_cache, KMALLOC_NORMAL, (void **)&pgt);
	kassert( rc == 0 );

	mutex_init(&pgt->mtx);          /* ミューテックスの初期化              */
	atomic_set(&pgt->nr_pages, 0);  /* ページテーブルのページ数を0に初期化 */
	hal_pgtbl_init(pgt);            /* アーキ依存部の初期化                */

	*pgtp = pgt;            /* ページテーブル情報を返却する */

	return 0;
}
/**
   ユーザ用ページテーブルを割り当てる
   @param[out] pgtp 割り当てたページテーブル情報を指し示すポインタのアドレス
 */
int
pgtbl_alloc_user_pgtbl(vm_pgtbl *pgtp){
	int             rc;
	vm_pgtbl_type *pgt;

	rc = pgtbl_alloc_pgtbl(&pgt);  /* ページテーブル情報を割り当てる */
	if ( rc != 0 )
		goto error_out;

	hal_copy_kernel_pgtbl(pgt);    /* カーネル空間部分を初期化する   */

	*pgtp = pgt;  /* ページテーブル情報を返却する */

	return 0;

error_out:
	return rc;
}

/**
   ユーザ用ページテーブルを解放する
   @param[in] pgt ページテーブル情報
 */
void
pgtbl_free_user_pgtbl(vm_pgtbl pgt){

	mutex_lock(&pgt->mtx);              /* ミューテックスの獲得           */

	/* ページテーブル解放済みであることを確認する */
	kassert(atomic_read(&pgt->nr_pages) == 0);  

	mutex_unlock(&pgt->mtx);            /* ミューテックスの解放           */

	slab_kmem_cache_free((void *)pgt);  /* ページテーブル情報を解放する */

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
}

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

/**
   ページテーブル用にページを割り当てる
   @param[out] tblp   ページテーブル用のページのカーネル仮想アドレス返却域
   @param[out] paddrp ページテーブル用のページの物理アドレス返却域
   @retval     0      正常終了
   @retval    -ENOMEM メモリ不足
 */
int
pgtbl_alloc_pgtbl_page(hal_pte **tblp, vm_paddr *paddrp){
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

	/* ページテーブル用ページを返却
	 */
	*tblp = (hal_pte *)tbl;
	*paddrp = (vm_paddr)paddr;

	return 0;

error_out:
	return rc;
}

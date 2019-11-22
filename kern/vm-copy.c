/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Copy Page                                                         */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>

#include <kern/page-if.h>
#include <kern/vm.h>

/**
   カーネル仮想アドレス中のページの内容をコピーする
   @param[in] dest コピー先ページのカーネル仮想アドレス
   @param[in] src  コピー元ページのカーネル仮想アドレス
 */
void
vm_copy_kmap_page(void *dest, void *src){
	uint64_t    *sp;
	uint64_t    *dp;
	int           i;

	sp = (uint64_t *)PAGE_TRUNCATE(src);
	dp = (uint64_t *)PAGE_TRUNCATE(dest);

	/* 8バイト単位でページをコピーする  */
	for(i = 0; PAGE_SIZE/sizeof(uint64_t) > i; ++i) 
		dp[i] = sp[i];
}

/**
   カーネル仮想アドレス中のページの内容をコピーする
   @param[in] dest_pgt コピー先のページテーブル
   @param[in] dest     コピー先の仮想アドレス
   @param[in] src_pgt  コピー元のページテーブル
   @param[in] src      コピー元の仮想アドレス
   @param[in] len      コピー長
   @retval    未コピーサイズ (単位:バイト)
 */
size_t
vm_copy(vm_pgtbl dest_pgt, void *dest, vm_pgtbl src_pgt, void *src, size_t len){
	int               rc;
	void     *dest_kpage;
	void      *src_kpage;
	size_t      dest_off;
	size_t       src_off;
	size_t   dest_remain;
	size_t    src_remain;
	vm_size  dest_pgsize;
	vm_size   src_pgsize;
	void             *dp;
	void             *sp;
	page_frame  *dest_pf;
	page_frame   *src_pf;
	size_t       cpy_len;
	size_t  finished_len;
	size_t     total_len;
	vm_paddr       paddr;
	vm_prot         prot;
	vm_flags       flags;

	total_len = len;
	finished_len = 0;

	/* TODO:ページテーブルmutexをとる */
	while( total_len > finished_len ) {

		dp = dest;
		sp =  src;

		rc = hal_pgtbl_extract(dest_pgt, (vm_vaddr)dp, &paddr, &prot, &flags,
		    &dest_pgsize);
		if ( rc != 0 )
			goto unlock_out;
		pfdb_paddr_to_kvaddr((void *)paddr, &dest_kpage);
		kassert( rc == 0 );

		rc = pfdb_kvaddr_to_page_frame(dest_kpage, &dest_pf);
		kassert( rc == 0 );
		/* コピー先ページの利用カウントをあげる */

		pfdb_inc_page_use_count(dest_pf);


		rc = hal_pgtbl_extract(src_pgt, (vm_vaddr)sp, &paddr, &prot, &flags, 
		    &src_pgsize);
		if ( rc != 0 )
			goto unlock_out;
		pfdb_paddr_to_kvaddr((void *)paddr, &src_kpage);
		kassert( rc == 0 );

		rc = pfdb_kvaddr_to_page_frame(src_kpage, &src_pf);
		kassert( rc == 0 );

		pfdb_inc_page_use_count(src_pf);

		/* TODO:ページテーブルmutexを解放する */

		/* ページ内でのコピー
		 */
		

		/* コピー先ページの利用カウントを下げる */
		pfdb_dec_page_use_count(dest_pf);
		/* コピー元ページの利用カウントを下げる */
		pfdb_dec_page_use_count(src_pf);
	}


	return 0;

unlock_out:
	/* TODO:ページテーブルmutexを解放する */
	return total_len - finished_len;
}

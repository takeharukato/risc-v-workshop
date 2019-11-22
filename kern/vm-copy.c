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
pgtbl_copy_kmap_page(void *dest, void *src){
	uint64_t    *sp;
	uint64_t    *dp;
	int           i;

	sp = (uint64_t *)PAGE_TRUNCATE(src);
	dp = (uint64_t *)PAGE_TRUNCATE(dest);

	/* 8バイト単位でページをコピーする  */
	for(i = 0; PAGE_SIZE/sizeof(uint64_t) > i; ++i) 
		dp[i] = sp[i];
}

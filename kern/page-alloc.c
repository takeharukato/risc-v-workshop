/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Page Allocator Interface                                          */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>

#include <kern/page-if.h>

/**
   指定された大きさの領域を格納可能なページ数のページオーダを返却する
   @param[in] size 領域長(単位:バイト)
   @param[out] res ページオーダ返却領域
   @retval  0      正常終了
   @retval -ESRCH  格納可能なページオーダを越えている
 */
int
pgif_calc_page_order(size_t size, page_order *res){
	page_order   order;
	uintptr_t nr_pages;

	/*  指定されたサイズを格納するために必要なページ数を算出する  */
	nr_pages = PAGE_ROUNDUP(size) >> PAGE_SHIFT;
	
	/*   ページサイズは, 典型的なデータ構造を格納可能なサイズとなるように規定
	 * されていることから典型的なケースでは小オーダでのメモリ獲得となると考えられる。
	 *   従って, コード上は, PAGE_POOL_MAX_ORDER - 1から小さいサイズに対して順に
	 * 最上位ビットを探査するほうが短くなるが, この場合, ループ回数が0からループを
	 * 開始する場合より多く実際の処理時間が長くなる。
	 * このため, オーダ0からループを構成するようにしている.
	 */
	for(order = 0; PAGE_POOL_MAX_ORDER > order; ++order) { /*  最上位ビットを算出  */

		/*  指定されたビット以上の位置のビットを全て立てたマスクとnr_pagesとの
		 *  論理積がゼロなら指定ビットが最上位ビットとなる
		 */
		if ( ( nr_pages & ~( ( ULONG_C(1) << ( order + 1 ) ) - 1 ) ) == 0 ) {

			/*  2のべき乗ページサイズになっていない場合はオーダを一つ上げる  */
			if ( nr_pages & ( ( ULONG_C(1) << order ) - 1  ) )
				++order;

			if ( order == PAGE_POOL_MAX_ORDER )
				break;  /*  格納可能なページオーダを越えている  */

			*res = order;
			goto success;
		}
	}

	return -ESRCH;  /*  格納可能なページオーダを越えている  */

success:
	return 0;
}

/**
   指定されたページオーダの連続物理メモリを獲得する
   @param[out] addrp       ページに対するカーネル領域内のアドレスを返却する領域
   @param[in]  order       要求ページオーダ
   @param[in]  alloc_flags ページ獲得条件
   @param[in]  usage       ページ利用用途
   @retval  0      正常終了
   @retval -ESRCH  格納可能なページオーダを越えている
   @retval -ENOMEM メモリ不足
 */
int
pgif_get_free_page_cluster(void **addrp, page_order order, 
    pgalloc_flags alloc_flags, page_usage usage){
	int           rc;
	obj_cnt_type pfn;
	void     *kvaddr;

	do{
		rc = pfdb_buddy_dequeue(order, usage, 
		    alloc_flags, &pfn);  /* 指定されたオーダーのページを取り出す */
		if ( ( rc != 0 ) && ( rc != -ESRCH ) )
			rc = -ENOMEM;  /*  ページが見つからなかった  */

		if ( ( rc != 0 ) && ( alloc_flags & KMALLOC_ATOMIC ) )
			goto error_out;  /*  ページ待ちを行わない場合はエラー復帰  */
	}while( rc != 0 );

	/* 獲得したページフレーム番号のページのカーネルアドレスを算出する
	 * TODO: カーネル空間へのマッピングを行うように修正
	 */
	rc = pfdb_pfn_to_kvaddr(pfn, &kvaddr);
	kassert( rc == 0 );
	if ( !( alloc_flags & KM_SFLAGS_CLR_NONE ) )
		memset(kvaddr, 0, PAGE_SIZE << order );  /*  メモリをクリアする  */

	*addrp = kvaddr;  /*  カーネル空間中のアドレスを返却する  */

	return 0;

error_out:
	return rc;
}
/**
   物理メモリを1ノーマルページ獲得する   
   @param[out] addrp       ページに対するカーネル領域内のアドレスを返却する領域
   @param[in]  alloc_flags ページ獲得条件
   @param[in]  usage       ページ利用用途
   @retval  0      正常終了
   @retval -ESRCH  格納可能なページオーダを越えている
   @retval -ENOMEM メモリ不足
 */
int 
pgif_get_free_page(void **addrp, pgalloc_flags alloc_flags, page_usage usage){

	/*  ページオーダー0 (1ページ)のページを取得する  */
	return pgif_get_free_page_cluster(addrp, 0, alloc_flags, usage);
}
/**
   物理メモリを解放する
   @param[in] addr 解放するページのカーネル領域内のアドレス
 */
void
pgif_free_page(void *addr){
	int           rc;
	page_frame   *pf;

	/*  解放対象ページのページフレーム情報を得る  */
	rc = pfdb_kvaddr_to_page_frame(addr, &pf);
	kassert( rc == 0 );

	pfdb_dec_page_use_count(pf);  /*  参照を落とし, ページ開放を促す  */
}

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
   仮想空間中の文字列の長さを求める
   @param[in] pgt      参照先アドレス空間のページテーブル
   @param[in] s        参照先仮想アドレス
   @retval    文字列長 (単位: バイト)
              NULLターミネート文字が見つからなかった場合は0を返す
*/
size_t
vm_strlen(vm_pgtbl pgt, char const *s){
	int             rc;
	size_t     cur_len;
	void           *sp;
	void        *kpage;
	char const *s_kmem;
	vm_size     pgsize;
	size_t         off;
	size_t      remain;
	vm_paddr     paddr;
	vm_prot       prot;
	vm_flags     flags;

	sp = (void *)s; /* 参照先アドレスを設定 */

	cur_len = 0; /* 文字列長を0で初期化する */

	rc = mutex_lock(&pgt->mtx); /*  アドレス空間のページテーブルmutexを獲得する */
	if ( rc != 0 )
		goto error_out;

	for( ; ; ++sp) {

		/* 参照先ページの物理アドレスを求める */
		rc = hal_pgtbl_extract(pgt, (vm_vaddr)sp, &paddr, &prot, &flags, &pgsize);
		if ( rc != 0 )
			goto unlock_out;
	
		/* 参照先ページのカーネル仮想アドレスを求める */
		rc = pfdb_paddr_to_kvaddr((void *)paddr, &kpage);
		kassert( rc == 0 );

		/* ページ内オフセットを求める */
		off = (uintptr_t)sp % pgsize;
		
		/* 参照可能量を求める */
		remain = pgsize - off;

		/* カーネルメモリ中での参照先アドレスを求める */
		s_kmem = kpage + off;

		for( ; remain > 0 ; --remain){  /* ヌル文字を探査する */

			if ( *s_kmem++ == '\0' )
				goto success; /* ヌル文字が見つかった */
			
			++cur_len; /* 文字列長をインクリメント */
		}
	}

success:
	mutex_unlock(&pgt->mtx); /*  アドレス空間のページテーブルmutexを獲得する */
	return cur_len;

unlock_out:
	mutex_unlock(&pgt->mtx); /*  アドレス空間のページテーブルmutexを獲得する */

error_out:
	return 0;
}
/**
   カーネル仮想アドレス中のページの内容をコピーする
   @param[in] dest_pgt コピー先のページテーブル
   @param[in] dest     コピー先の仮想アドレス
   @param[in] src_pgt  コピー元のページテーブル
   @param[in] src      コピー元の仮想アドレス
   @param[in] len      コピー長
   @retval    未コピーサイズ (単位:バイト)
   @note LO:  転送先, 転送元の順にロックする
 */
size_t
vm_memmove(vm_pgtbl dest_pgt, void *dest, vm_pgtbl src_pgt, void *src, size_t len){
	int               rc;
	void     *dest_kpage;
	void      *src_kpage;
	vm_size  dest_pgsize;
	vm_size   src_pgsize;
	size_t      dest_off;
	size_t       src_off;
	size_t   dest_remain;
	size_t    src_remain;
	void      *dest_kmem;
	void       *src_kmem;
	void             *dp;
	void             *sp;
	size_t   max_cpy_len;
	size_t       cpy_len;
	size_t  total_remain;
	vm_paddr       paddr;
	vm_prot         prot;
	vm_flags       flags;

	total_remain = len;  /* コピーするバイト数 */

	dp = dest;  /* 転送先仮想アドレス */
	sp =  src;  /* 転送元仮想アドレス */

	/* コピー先アドレス空間のページテーブルmutexを獲得する */
	rc = mutex_lock(&dest_pgt->mtx);
	if ( rc != 0 )
		goto error_out;

	/* コピー元アドレス空間のページテーブルmutexを獲得する */
	if ( dest_pgt != src_pgt ) {

		rc = mutex_lock(&src_pgt->mtx);
		if ( rc != 0 )
			goto unlock_dest_out;
	}

	while( total_remain > 0 ) {

		/* コピー先ページの物理アドレスを求める */
		rc = hal_pgtbl_extract(dest_pgt, (vm_vaddr)dp, &paddr, &prot, &flags,
		    &dest_pgsize);
		if ( rc != 0 )
			goto unlock_out;

		/* コピー先ページのカーネル仮想アドレスを求める */
		rc = pfdb_paddr_to_kvaddr((void *)paddr, &dest_kpage);
		kassert( rc == 0 );

		/* コピー元ページの物理アドレスを求める */
		rc = hal_pgtbl_extract(src_pgt, (vm_vaddr)sp, &paddr, &prot, &flags, 
		    &src_pgsize);
		if ( rc != 0 ) 
			goto unlock_out;

		/* コピー元ページのカーネル仮想アドレスを求める */
		rc = pfdb_paddr_to_kvaddr((void *)paddr, &src_kpage);
		kassert( rc == 0 );

		/* ページ内でのコピー
		 */
		/* コピー先のページ内オフセットを求める */
		dest_off = (uintptr_t)dp % dest_pgsize;
		
		/* コピー先の転送可能量を求める */
		dest_remain = dest_pgsize - dest_off;

		/* カーネルメモリ中でのコピー先アドレスを求める */
		dest_kmem = dest_kpage + dest_off;

		/* コピー元のページ内オフセットを求める */
		src_off = (uintptr_t)dp % src_pgsize;
		
		/* コピー元の転送可能量を求める */
		src_remain = src_pgsize - src_off;
		
		/* カーネルメモリ中でのコピー先アドレスを求める */
		src_kmem = src_kpage + src_off;

		/* コピー可能バイト数を求める */
		max_cpy_len =  MIN(dest_remain, src_remain);

		/* コピーするバイト数を求める */
		cpy_len = MIN(total_remain, max_cpy_len);

		/* メモリをコピーする */
		memmove(dest_kmem, src_kmem, cpy_len);

		/* 転送量を更新 */
		kassert( total_remain >= cpy_len );
		total_remain -= cpy_len;

		/* 転送先, 転送元仮想アドレスを更新 */
		dp += cpy_len;
		sp += cpy_len;
	}

	/*  コピー元アドレス空間のページテーブルmutexを解放する */
	mutex_unlock(&src_pgt->mtx);
	/*  コピー先アドレス空間のページテーブルmutexを解放する */
	if ( dest_pgt != src_pgt )
		mutex_unlock(&dest_pgt->mtx);

	return 0;

unlock_out:

	/*  コピー元アドレス空間のページテーブルmutexを解放する */
	mutex_unlock(&src_pgt->mtx);

unlock_dest_out:
	/*  コピー先アドレス空間のページテーブルmutexを解放する */
	if ( dest_pgt != src_pgt )
		mutex_unlock(&dest_pgt->mtx);

error_out:
	return total_remain;
}

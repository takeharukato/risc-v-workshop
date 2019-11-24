/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Map Page                                                          */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>

#include <kern/page-if.h>
#include <kern/vm.h>

/**
   仮想空間からページをアンマップする (内部関数)
   @param[in]  pgt        アドレス空間のページテーブル情報
   @param[in]  vaddr      アンマップする仮想アドレス
   @param[in]  flags      ページ割り当て要否の判断に使用するマップ属性
   @param[in]  size       コピーする領域長(単位:バイト)
   @param[in]  force      エラーを無視してアンマップを試みる
   @retval     0          正常終了
   @retval    -ENOENT     ページテーブルまたはラージページがマップされていない
   @retval    -ESRCH      ページがマップされていない
   @note      ページ単位でのアンマップを行う
   @note      アドレス空間のロックを獲得した状態で呼び出す
 */
static int
vm_unmap_common(vm_pgtbl pgt, vm_vaddr vaddr, vm_flags flags, vm_size size, bool force){
	int               rc;
	vm_paddr   map_paddr;
	vm_prot     map_prot;
	vm_flags   map_flags;
	vm_size   map_pgsize;
	vm_flags unmap_flags;
	vm_vaddr   sta_vaddr;
	vm_vaddr   end_vaddr;
	vm_vaddr   cur_vaddr;
	void         *kvaddr;

	/* 転送元アドレスと転送先アドレスをページ境界にそろえる
	 */
	sta_vaddr = PAGE_TRUNCATE(vaddr);         /* 開始仮想アドレス */
	end_vaddr = PAGE_ROUNDUP(vaddr + size);   /* 終了仮想アドレス */

	for( cur_vaddr = sta_vaddr; end_vaddr > cur_vaddr; ) {

		/* マップしているページの物理アドレスを求める */
		rc = hal_pgtbl_extract(pgt, cur_vaddr, &map_paddr, &map_prot, &map_flags,
		    &map_pgsize);
		if ( !force && ( rc != 0 ) )
			goto error_out;

		unmap_flags = flags | map_flags; /* アンマップに使用するフラグ値を算出 */
		
		/* ページをアンマップする */
		hal_pgtbl_remove(pgt, cur_vaddr, unmap_flags, map_pgsize);

		/* ページプール内で管理されているページを割り当てている場合は,
		 * ページを解放する
		 */
		if ( ( unmap_flags & VM_FLAGS_UNMANAGED ) == 0 ) {

			/* コピー元のページのカーネル仮想アドレスを算出する */
			rc = pfdb_paddr_to_kvaddr((void *)map_paddr, &kvaddr);
			kassert( rc == 0 );  /* マネージドページなので成功するはず */

			pgif_free_page(kvaddr);  /* ページを解放する */
		}

		cur_vaddr += map_pgsize;  /* 次のページを処理する */
	}

	return 0;

error_out:
	return rc;
}

/**
   仮想空間にページをマップする  (内部関数)
   @param[in]  pgt        アドレス空間のページテーブル情報
   @param[in]  vaddr      マップする仮想アドレス
   @param[in]  flags      ページ割り当て要否の判断に使用するマップ属性
   @param[in]  size       コピーする領域長(単位:バイト)
   @retval     0          正常終了
   @retval    -ENOENT     ページテーブルまたはラージページがマップされていない
   @retval    -ESRCH      ページがマップされていない, ページサイズが大きすぎる
   @note      ページ単位でのアンマップを行う
   @note      アドレス空間のロックを獲得した状態で呼び出す
 */
static int __unused
vm_map_common(vm_pgtbl pgt, vm_vaddr vaddr, vm_prot prot, vm_flags flags, vm_size pgsize){
	int               rc;
	vm_paddr       paddr;
	void         *kvaddr;
	page_order     order;

	/* 転送元アドレスと転送先アドレスをページ境界にそろえる
	 */
	kassert( !addr_not_aligned(vaddr, pgsize) ); /* ページ境界に揃っていることを確認 */

	/* ページオーダを算出する */
	rc = pgif_calc_page_order((size_t)pgsize, &order);
	if ( rc != 0 ) 
		goto error_out;

	/* メモリを獲得する */
	rc = pgif_get_free_page_cluster(&kvaddr, order, KMALLOC_NORMAL, 
	    PAGE_USAGE_ANON);
	if ( rc != 0 ) 
		goto error_out;

	/* 割り当てたメモリの物理アドレスを求める */
	rc = pfdb_kvaddr_to_paddr(kvaddr, (void *)&paddr);
	kassert( rc == 0 );  /* マネージドページなので成功するはず */

	/* ページをマップする */
	rc = hal_pgtbl_enter(pgt, vaddr, paddr, prot, flags, pgsize);
	if ( rc != 0 ) 
		goto error_out;
	
	return 0;

error_out:
	return rc;
}

/**
   仮想空間をコピーする
   @param[in]  dest       コピー先のアドレス空間のページテーブル情報
   @param[in]  src        コピー元のアドレス空間のページテーブル情報
   @param[in]  vaddr      コピー開始仮想アドレス
   @param[in]  flags      ページ割り当て要否の判断に使用するマップ属性
   @param[in]  size       コピーする領域長(単位:バイト)
   @retval     0          正常終了
   @retval    -ENOENT     ページテーブルまたはラージページがマップされていない
   @retval    -ESRCH      ページがマップされていない
   @retval    -ENOMEM     メモリ不足
   @note      ページ単位でのコピーを行う
   @note LO:  転送先, 転送元の順にロックする(アンマップの時に転送元を先に解放するため)
 */
int
vm_copy_range(vm_pgtbl dest, vm_pgtbl src, vm_vaddr vaddr, vm_flags flags, vm_size size){
	int                rc;
	vm_paddr    src_paddr;
	vm_prot      src_prot;
	vm_flags    src_flags;
	vm_size    src_pgsize;
	vm_paddr   dest_paddr;
	vm_prot     dest_prot;
	vm_flags   dest_flags;
	vm_size   dest_pgsize;
	page_order      order;
	vm_vaddr    sta_vaddr;
	vm_vaddr    end_vaddr;
	vm_vaddr    cur_vaddr;
	void      *src_kvaddr;
	void     *dest_kvaddr;

	/* 転送元アドレスと転送先アドレスをページ境界にそろえる
	 */
	sta_vaddr = PAGE_TRUNCATE(vaddr);         /* 開始仮想アドレス */
	end_vaddr = PAGE_ROUNDUP(vaddr + size);   /* 終了仮想アドレス */

	/*  コピー先アドレス空間のページテーブルmutexを獲得する */
	rc = mutex_lock(&dest->mtx);	
	if ( rc != 0 )
		goto error_out;
	/*  コピー元アドレス空間のページテーブルmutexを獲得する */
	if ( dest != src ) {

		rc = mutex_lock(&src->mtx);
		if ( rc != 0 )
			goto unlock_dest_out;
	}

	/*
	 * 指定されたアドレス範囲をコピーする
	 */
	for( cur_vaddr = sta_vaddr; end_vaddr > cur_vaddr; ) {

		/* コピー元ページの保護属性とマップ属性を求める */
		rc = hal_pgtbl_extract(src, cur_vaddr, &src_paddr, &src_prot, &src_flags,
		    &src_pgsize);
		if ( rc != 0 )
			goto unmap_out;

		dest_prot = src_prot;		 /* 同一の保護属性を設定           */
		dest_flags = flags | src_flags;  /* ページ割当て要否を引数から得る */

		/* ページプール内で管理されているページを割り当てている場合は,
		 * コピー先ページを新規に割り当て, コピー元ページの内容を
		 * コピーする。
		 */
		if ( dest_flags & VM_FLAGS_UNMANAGED ) {

			/* 非管理対象メモリの場合同一物理アドレスを
			 *  同一サイズでマップする
			 */
			dest_paddr = src_paddr;  
			dest_pgsize = src_pgsize;
		} else {

			/* コピー元のページのカーネル仮想アドレスを算出する */
			rc = pfdb_paddr_to_kvaddr((void *)src_paddr, &src_kvaddr);
			kassert( rc == 0 );  /* マネージドページなので成功するはず */

			/* コピー元のページオーダを算出する */
			rc = pgif_calc_page_order((size_t)src_pgsize, &order);
			kassert( rc == 0 );  /* マネージドページなので成功するはず */

			dest_pgsize = PAGE_SIZE << order;  /*  コピー先のページサイズ */

			/* メモリを獲得する */
			rc = pgif_get_free_page_cluster(&dest_kvaddr, order, KMALLOC_NORMAL, 
			    PAGE_USAGE_ANON);
			if ( rc != 0 ) 
				goto unmap_out;

			/*  ページの内容をコピーする */
			vm_copy_kmap_page(dest_kvaddr, src_kvaddr);
			
			/* コピー先ページの物理アドレスを得る */
			rc = pfdb_kvaddr_to_paddr(dest_kvaddr, (void *)&dest_paddr);
			kassert( rc == 0 );  /* マネージドページなので成功するはず */
		}

		/*  ページをマップする */
		rc = hal_pgtbl_enter(dest, vaddr, dest_paddr, dest_prot, dest_flags, 
		    dest_pgsize);
		if ( ( rc != 0 ) && ( ( dest_flags & VM_FLAGS_UNMANAGED ) == 0 ) ) {
			
			/* 管理ページを獲得済みの場合は, 獲得したページを解放 */
			pgif_free_page(dest_kvaddr);
			goto unmap_out;
		}

		cur_vaddr += src_pgsize;  /* 次のページをコピーする  */
	}

	/*  コピー元アドレス空間のページテーブルmutexを解放する */
	mutex_unlock(&src->mtx);
	/*  コピー先アドレス空間のページテーブルmutexを解放する */
	if ( dest != src )
		mutex_unlock(&dest->mtx);

	return  0;

unmap_out:
	/*  コピー元アドレス空間のページテーブルmutexを解放する */
	mutex_unlock(&src->mtx);

	/*
	 * マップ済みの領域を解放する
	 */
	kassert( cur_vaddr >= sta_vaddr );
	vm_unmap_common(dest, sta_vaddr, flags, cur_vaddr - sta_vaddr, true);

unlock_dest_out:
	/*  コピー先アドレス空間のページテーブルmutexを解放する */
	if ( dest != src )
		mutex_unlock(&dest->mtx);
error_out:
	return rc;
}

/**
   仮想空間からページをアンマップする
   @param[in]  pgt        アドレス空間のページテーブル情報
   @param[in]  vaddr      アンマップする仮想アドレス
   @param[in]  flags      ページ割り当て要否の判断に使用するマップ属性
   @param[in]  size       コピーする領域長(単位:バイト)
   @retval     0          正常終了
   @retval    -ENOENT     ページテーブルまたはラージページがマップされていない
   @retval    -ESRCH      ページがマップされていない
   @note      ページ単位でのアンマップを行う
   @note      アドレス空間のロックを獲得した状態で呼び出す
 */
int
vm_unmap(vm_pgtbl pgt, vm_vaddr vaddr, vm_flags flags, vm_size size){
	int                rc;
	vm_vaddr     rm_vaddr;
	vm_paddr     rm_paddr;
	vm_prot       rm_prot;
	vm_flags     rm_flags;
	vm_size     rm_pgsize;
	vm_vaddr    sta_vaddr;
	vm_vaddr    end_vaddr;

	/* 転送元アドレスと転送先アドレスをページ境界にそろえる
	 */
	sta_vaddr = PAGE_TRUNCATE(vaddr);         /* 開始仮想アドレス */
	end_vaddr = PAGE_ROUNDUP(vaddr + size);   /* 終了仮想アドレス */

	rc = mutex_lock(&pgt->mtx);	 /*  アドレス空間のページテーブルmutexを獲得する */
	if ( rc != 0 )
		goto error_out;

	/*
	 * 領域に割り当てられたマッピングと物理メモリとを解放する
	 */
	for( rm_vaddr = sta_vaddr; end_vaddr > rm_vaddr; ) {

		rc = hal_pgtbl_extract(pgt, rm_vaddr, &rm_paddr, &rm_prot, &rm_flags,
		    &rm_pgsize);
		if ( rc != 0 )
			goto unlock_out;

		rm_flags |= flags;    /* ページ割当て要否を引数から得る */

		/* ページをアンマップする */
		rc = vm_unmap_common(pgt, rm_vaddr, rm_flags, rm_pgsize, false);
		if ( rc != 0 )
			goto unlock_out;

		rm_vaddr += rm_pgsize;  /* 次のページ */
	}

	mutex_unlock(&pgt->mtx); /*  アドレス空間のページテーブルmutexを解放する */
	return 0;

unlock_out:	
	mutex_unlock(&pgt->mtx); /*  アドレス空間のページテーブルmutexを解放する */

error_out:
	return rc;
}

/**
   仮想空間に物理メモリをマップする
   @param[in]  pgt        アドレス空間のページテーブル情報
   @param[in]  vaddr      マップする仮想アドレス
   @param[in]  prot       保護属性
   @param[in]  flags      ページ割り当て要否の判断に使用するマップ属性
   @param[in]  pgsize     マップ時に使用するページサイズ (単位: バイト)
   @param[in]  size       マップする領域長 (単位: バイト)
   @retval     0          正常終了
   @retval    -ENOENT     ページテーブルまたはラージページがマップされていない
   @retval    -ESRCH      ページがマップされていない
   @note      ページ単位でのアンマップを行う
   @note      アドレス空間のロックを獲得した状態で呼び出す
 */
int
vm_map_userpage(vm_pgtbl pgt, vm_vaddr vaddr, vm_prot prot, vm_flags flags, 
    vm_size pgsize, vm_size size){
	int                rc;
	vm_vaddr    sta_vaddr;
	vm_vaddr    end_vaddr;
	vm_vaddr    map_vaddr;

	/* 転送元アドレスと転送先アドレスをページ境界にそろえる
	 */
	sta_vaddr = PAGE_TRUNCATE(vaddr);         /* 開始仮想アドレス */
	end_vaddr = PAGE_ROUNDUP(vaddr + size);   /* 終了仮想アドレス */

	rc = mutex_lock(&pgt->mtx);	 /*  アドレス空間のページテーブルmutexを獲得する */
	if ( rc != 0 )
		goto error_out;

	/*
	 * 物理ページをマップする
	 */
	for( map_vaddr = sta_vaddr; end_vaddr > map_vaddr; ) {

		rc = vm_map_common(pgt, map_vaddr, prot, flags, pgsize);
		if ( rc != 0 )
			goto unmap_out;

		map_vaddr += pgsize;  /* 次のページへ */
	}
	mutex_unlock(&pgt->mtx);	 /*  アドレス空間のページテーブルmutexを解放する */
	return 0;

unmap_out:
	vm_unmap_common(pgt, sta_vaddr, flags, map_vaddr - sta_vaddr, true);
	mutex_unlock(&pgt->mtx);	 /*  アドレス空間のページテーブルmutexを解放する */

error_out:
	return rc;
}


/*
 * RISC-V64 pgtbl
 */

#include <klib/freestanding.h>
#include <kern/kern-common.h>
#include <hal-riscv64/hal-page.h>
#include <hal-riscv64/rv64-platform.h>
#include <hal-riscv64/rv64-pgtbl.h>
#include <kern/page-if.h>
#include <kern/vm-if.h>

#include <kern/ktest.h>

static ktest_stats tstat_rv64pgtbl=KTEST_INITIALIZER;

/**
   カーネルストレートマップ領域中のアドレスを物理アドレスに変換する
   @param[in] _kvaddr カーネルストレートマップ領域中のアドレス
   @return 物理アドレス
 */
#define HAL_KERN_STRAIGHT_TO_PHY(_kvaddr) \
	tflib_kern_straight_to_phy((void *)(_kvaddr))

/**
   物理アドレスをカーネルストレートマップ領域中のアドレスに変換する
   @param[in] _paddr 物理アドレス
   @return カーネルストレートマップ領域中のアドレス
 */
#define HAL_PHY_TO_KERN_STRAIGHT(_paddr) \
	(vm_vaddr)tflib_phy_to_kern_straight(_paddr)


#define ENABLE_SHOW_PAGE_MAP

static vm_pgtbl kpgtbl = NULL; /* カーネルページテーブル */

/**
   ページマップ情報を表示する
   @param[in]  pgt    ページテーブル
   @param[in]  vaddr  仮想アドレス
   @param[in]  size   領域サイズ
 */
void 
show_page_map(vm_pgtbl __unused pgt, vm_vaddr __unused vaddr, vm_size __unused size){
#if defined(ENABLE_SHOW_PAGE_MAP)
	int               rc;
	vm_vaddr   vaddr_sta;
	vm_vaddr   vaddr_end;
	vm_vaddr   cur_vaddr;
	vm_paddr   cur_paddr;
	vm_prot         prot;
	vm_flags       flags;
	vm_size       pgsize;

	/* マップを確認
	 */
	vaddr_sta = vaddr;
	vaddr_end = vaddr + size;
	for( cur_vaddr = vaddr_sta; vaddr_end > cur_vaddr; cur_vaddr += pgsize) {

		rc = hal_pgtbl_extract(pgt, cur_vaddr, &cur_paddr, 
		    &prot, &flags, &pgsize);
		if ( rc != 0 ) {

			kprintf("vaddr=%p has no map.\n", cur_vaddr);
			continue;
		}

		kprintf("vaddr=%p paddr=%p pgsize=%ld prot=[%c|%c|%c] flags=[%c]\n",
		    cur_vaddr, cur_paddr, pgsize,
		    ( (prot & VM_PROT_READ)         ? ('R') : ('-') ),
		    ( (prot & VM_PROT_WRITE)        ? ('W') : ('-') ),
		    ( (prot & VM_PROT_EXECUTE)      ? ('E') : ('-') ),
		    ( (flags & VM_FLAGS_SUPERVISOR) ? ('S') : ('U') ));
	}
#endif  /*  ENABLE_SHOW_PAGE_MAP  */
}

/**
   ページテーブルエントリから保護属性/マップ属性に変換する
   @param[in] pte ページテーブルエントリ
   @param[out] protp  保護属性返却域
   @param[out] flagsp マップ属性返却域
 */
static void
pte_to_vmprot_and_vmflags(hal_pte pte, vm_prot *protp, vm_flags *flagsp){
	vm_prot   prot;
	vm_flags flags;

	prot = VM_PROT_NONE;
	flags = VM_FLAGS_NONE;

	if ( !RV64_PTE_IS_VALID(pte) )
		goto out; /* アクセス不能 */

	if ( RV64_PTE_FLAGS(pte) & RV64_PTE_R )	
		prot |= VM_PROT_READ;  /* 読み込み可能  */
	if ( RV64_PTE_FLAGS(pte) & RV64_PTE_W )	
		prot |= VM_PROT_WRITE;  /* 書き込み可能 */
	if ( RV64_PTE_FLAGS(pte) & RV64_PTE_X )	
		prot |= VM_PROT_EXECUTE;  /* 実行可能      */

	if ( !( RV64_PTE_FLAGS(pte) & RV64_PTE_U ) )
		flags |= VM_FLAGS_SUPERVISOR;  /* スーパバイザページ  */
	
out:
	*protp =   prot;  /* アクセス属性を返却 */
	*flagsp = flags;  /* マップ属性を返却 */
	return;
}

/**
   保護属性/マップ属性からページテーブルエントリの属性値を取得する
   @param[in]  prot  保護属性
   @param[in]  flags マップ属性
   @param[out] ptep  ページテーブルエントリ属性値返却域
 */
static void
vmprot_and_vmflags_to_pte(vm_prot prot, vm_flags flags, hal_pte *ptep){
	hal_pte pte;
	    
	pte = 0;

	if ( prot & VM_PROT_READ )
		pte |= RV64_PTE_R; /* 読み込み可能  */

	if ( prot & VM_PROT_WRITE )
		pte |= RV64_PTE_W; /* 書き込み可能 */

	if ( prot & VM_PROT_EXECUTE )
		pte |= RV64_PTE_X;  /* 実行可能      */

	if ( !( flags & VM_FLAGS_SUPERVISOR ) )
		pte |= RV64_PTE_U;  /* ユーザアクセス可能 */
	
	*ptep = pte;  /* PTE属性値を返却 */

	return;
}

/**
   ページテーブルを構成するテーブルを走査する
   @param[in]  kvtbl        操作対象テーブルのカーネル仮想アドレス
   @param[in]  lvl          ページテーブルの段数(単位:段)
   @param[in]  vaddr        マップ先仮想アドレス
   @param[out] paddrp       マップ先物理アドレス返却域
   @param[out] protp        保護属性返却域
   @param[out] flagsp       マップ属性返却域
   @param[out] pgsizep      マップするページサイズ返却域
   @param[out] next_tbl     次のテーブルのカーネル仮想アドレス返却域
   @retval     0            正常終了
   @retval    -ENOENT       ページがマップされていない
   @retval    -EAGAIN       次のテーブルを探査する指示
 */
static int
walk_pgtbl(hal_pte *kvtbl, int lvl, vm_vaddr vaddr, vm_paddr *paddrp, vm_prot *protp, 
    vm_flags *flagsp, vm_size *pgsizep, hal_pte **next_tbl){
	int               rc;
	int              idx;
	hal_pte          pte;
	vm_paddr       paddr;
	vm_prot         prot;
	vm_flags       flags;
	vm_size       pgsize;
	void         *kvaddr;

	kassert( ( ( RV64_PGTBL_LVL_NR - 1 ) >= lvl ) && ( lvl >= 0 ) );

	idx = rv64_pgtbl_calc_index_val(vaddr, lvl); /* テーブルインデクスを得る */
	pte = kvtbl[idx];  /* ページテーブルエントリを得る */

	if ( !RV64_PTE_IS_VALID(pte) ) { /* 無効なページテーブルエントリだった場合 */

		rc = -ENOENT;   /* ページがマップされていない */
		goto error_out;
	}

	/* マップ先のアドレスを取得する
	 */
	paddr = RV64_PTE_TO_PADDR(pte);  /* PTEから物理アドレスを取得 */
	/* テーブルの物理アドレスをカーネル仮想アドレスに変換 */
	rc = pfdb_paddr_to_kvaddr((void *)paddr, (void **)&kvaddr);
	kassert( rc == 0 );

	/* ページマップエントリでなければ次のテーブルのアドレスを返却する
	 */
	if ( !RV64_PTE_IS_LEAF(pte) ) {
		
		*next_tbl = kvaddr;  /* テーブルのカーネル仮想アドレスを返却 */
		rc = -EAGAIN; /* 次のテーブルの探索を指示 */
		goto error_out;
	}
	
	/* ページマップエントリの場合
	 */
	pte_to_vmprot_and_vmflags(pte, &prot, &flags); /* 保護属性とマップ属性を得る */
	pgsize = rv64_pgtbl_calc_pagesize(lvl); /* ページサイズを得る */

	/* マップ先ページの情報を返却する
	 */
	*paddrp = paddr;     /* 物理アドレスを返却する */
	*protp = prot;       /* 保護属性を返却する     */
	*flagsp = flags;     /* マップ属性を返却する   */
	*pgsizep = pgsize;   /* ページサイズを返却する */

	return 0;
	
error_out:
	return rc;
}

/**
   ページテーブルを構成するテーブル間の参照を削除する
   @param[in] pgt           ページテーブル
   @param[in] vaddr         仮想アドレス
 */
static void
remove_table_reference(vm_pgtbl pgt, vm_vaddr vaddr){
	int                                   rc;
	int                                  lvl;
	int                              min_lvl;
	int                                  idx;
	hal_pte                         *cur_tbl;
	hal_pte        *pgtbl[RV64_PGTBL_LVL_NR];
	hal_pte                              pte;
	vm_paddr                      next_paddr;
	page_frame                       *tbl_pf;
	page_frame                       *low_pf;
	hal_pte                         *low_tbl;
	vm_paddr                       low_paddr;

	cur_tbl = pgt->pgtbl_base;  /* ページテーブルベース (VPN2テーブル)を設定 */

	/* テーブルを走査する
	 */
	for(lvl = RV64_PGTBL_LVL_NR - 1; lvl >= 0; --lvl) {

		/* テーブルインデクスを得る */
		idx = rv64_pgtbl_calc_index_val(vaddr, lvl); 
		pte = cur_tbl[idx];  /* ページテーブルエントリを得る */
		if ( !RV64_PTE_IS_VALID(pte) ) { /* 有効なページテーブルエントリでない */

			kprintf(KERN_PNC "hal_pgtbl_remove: Invalid pte tbl=%p lvl=%d "
			    "vaddr=%p pte=%p\n", cur_tbl, lvl, vaddr, pte);
			kassert_no_reach();
		}
		pgtbl[lvl] = cur_tbl; /* ページテーブル構成を記憶する */
		if ( ( lvl == 0 ) || ( RV64_PTE_IS_LEAF(pte) ) )
			break;  /* ページマップエントリに到達した */
		
		next_paddr = RV64_PTE_TO_PADDR(pte);
		/* テーブルのカーネル仮想アドレスを得る */
		rc = pfdb_paddr_to_kvaddr((void *)next_paddr, (void **)&cur_tbl);
		kassert( rc == 0 );
	}
	
	/* テーブル間の参照を削除する
	 */
	for(min_lvl = lvl ; RV64_PGTBL_LVL_NR > lvl; ++lvl) {

		cur_tbl = pgtbl[lvl]; /* テーブルを参照 */

		idx = rv64_pgtbl_calc_index_val(vaddr, lvl); /* テーブルインデクスを得る */
		pte = cur_tbl[idx]; /* PTEを得る */

		low_paddr = RV64_PTE_TO_PADDR(pte);       /* 参照先物理アドレスを得る */
		cur_tbl[idx] = RV64_PTE_PADDR_TO_PPN(0);  /* テーブル参照を削除       */

		/* テーブルのページフレーム情報を得る */
		rc = pfdb_kvaddr_to_page_frame((void *)cur_tbl, &tbl_pf);
		kassert( rc == 0 );

		pfdb_dec_page_use_count(tbl_pf); /* ページテーブルの参照を落とす */
		if ( pfdb_ref_page_use_count(tbl_pf) > 1 )
			break;  /* 最終参照でない */

		/* 下位のページの参照を落とす
		 */
		if ( lvl > min_lvl ) {

			/* 下位のテーブルのカーネル仮想アドレスを得る */
			rc = pfdb_paddr_to_kvaddr((void *)low_paddr, (void **)&low_tbl);
			kassert( rc == 0 );

			/* テーブルのページフレーム情報を得る */
			rc = pfdb_kvaddr_to_page_frame((void *)low_tbl, &low_pf);
			kassert( rc == 0 );

			kassert( pfdb_dec_page_use_count(low_pf) ); /* 最終参照のはず */

			/* ページテーブルのページ数を減算 */
			statcnt_dec(&pgt->nr_pages); 
		}
	}
}

/**
   ページテーブルを伸張する
   @param[in]  pgt          操作対象アドレス空間のページテーブル情報
   @param[in]  kvtbl        操作対象テーブルのカーネル仮想アドレス
   @param[in]  lvl          ページテーブルの段数(単位:段)
   @param[in]  vaddr        マップ先仮想アドレス
   @param[in]  paddr        マップ先物理アドレス
   @param[in]  prot         保護属性
   @param[in]  flags        マップ属性
   @param[in]  pgsize       マップするページサイズ
   @param[out] tbl_changedp ページテーブル更新有無を返却する領域
   @param[out] next_tbl     次のテーブルのカーネル仮想アドレス返却域
   @retval     0            正常終了
   @retval    -EBUSY        すでにページがマップされている
   @retval    -ENOMEM       メモリ不足
 */
static int
grow_pgtbl(vm_pgtbl pgt, hal_pte *kvtbl, int lvl, vm_vaddr vaddr, vm_paddr paddr,
    vm_prot prot, vm_flags flags, vm_size pgsize, bool *tbl_changedp, hal_pte **next_tbl){
	int               rc;
	int              idx;
	bool             res;
	page_frame       *pf;
	hal_pte      cur_pte;
	hal_pte      new_pte;
	hal_pte     *new_tbl;
	vm_paddr   new_paddr;

	kassert( ( ( RV64_PGTBL_LVL_NR - 1 ) >= lvl ) && ( lvl >= 0 ) );

	idx = rv64_pgtbl_calc_index_val(vaddr, lvl); /* テーブルインデクスを得る */
	cur_pte = kvtbl[idx];  /* ページテーブルエントリを得る */

	/* テーブルのページフレーム情報を得る */
	rc = pfdb_kvaddr_to_page_frame((void *)kvtbl, &pf); 
	kassert( rc == 0 );

	new_paddr = RV64_PTE_TO_PADDR(cur_pte); /* 次のテーブルの物理アドレス算出 */
	if ( RV64_PTE_IS_VALID(cur_pte) ) { /* 有効なページテーブルエントリだった場合 */

		/* ページマップエントリの場合 */
		if ( rv64_pgtbl_calc_pagesize(lvl) == pgsize ) { 

			rc = -EBUSY;  /* ラージページをマップするためのエントリが使用中 */
			goto error_out;
		}
		goto pte_no_change;  /* ページテーブルエントリの更新はない */
	}
	
	/* ページのマップまたはページテーブルにテーブルを追加する
	 */
	if ( rv64_pgtbl_calc_pagesize(lvl) == pgsize ) {

		/* ページのマップ
		 */

		/* 属性値に物理ページ番号値を付加しPTEを生成 */
		vmprot_and_vmflags_to_pte(prot, flags, &new_pte);
		new_pte |= RV64_PTE_V | RV64_PTE_PADDR_TO_PPN(paddr);

		kvtbl[idx] = new_pte;  /* PTEを更新 */

		res = pfdb_inc_page_use_count(pf); /* 参照数を加算 */
		kassert( res );  /* 加算できたことを確認 */
		*tbl_changedp = true;  /* ページテーブル更新を通知 */
		goto success; /* マップ処理終了 */
	}
	
	/* ページテーブル割り当て
	 */
	rc = pgtbl_alloc_pgtbl_page(pgt, &new_tbl, &new_paddr);
	if ( rc != 0 ) {
		
		rc = -ENOMEM;   /* メモリ不足  */
		goto error_out;
	}
	
	res = pfdb_inc_page_use_count(pf); /* 参照数を加算 */
	kassert( res );  /* 加算できたことを確認 */

	/* 属性値に物理ページ番号値を付加し次のテーブルへのPTEを生成
	 */
	new_pte = RV64_PTE_V | RV64_PTE_PGTBL_FLAGS | RV64_PTE_PADDR_TO_PPN(new_paddr);
	kvtbl[idx] = new_pte; /* PTEを更新 */

	*tbl_changedp = true;  /* ページテーブル更新を通知 */

pte_no_change:
	 /* テーブル仮想アドレス  */
	rc = pfdb_paddr_to_kvaddr((void *)new_paddr, (void **)&new_tbl);
	kassert( rc == 0 );

	*next_tbl = new_tbl;  /* 割り当てたテーブルのアドレスを返却する */

success:
	return 0;
	
error_out:
	return rc;
}
/**
   仮想アドレスに物理ページをマップする
   @param[in] pgt           ページテーブル
   @param[in] vaddr         仮想アドレス
   @param[in] paddr         物理アドレス
   @param[in] prot          保護属性
   @param[in] flags         マップ属性
   @param[in] pgsize        ページサイズ
   @retval    0             正常終了
   @retval   -EINVAL        ページサイズ境界と仮想アドレスまたは
   物理アドレスの境界があっていない
   @retval   -ENOMEM        メモリ不足
   @retval   -EBUSY         すでにメモリが割り当てられている
 */
static int
map_vaddr_to_paddr(vm_pgtbl pgt, vm_vaddr vaddr, vm_paddr paddr, 
    vm_prot prot, vm_flags flags, vm_size pgsize){
	int                    rc;
	int                   lvl;
	hal_pte          *cur_tbl;
	hal_pte         *next_tbl;
	bool          tbl_changed;

	/* 引数チェック
	 */
	if ( ( pgsize == HAL_PAGE_SIZE_1G ) &&
	    ( !PAGE_1GB_ALIGNED(vaddr) || !PAGE_1GB_ALIGNED(paddr) ) )
		return -EINVAL;  /* 1GiB境界でない仮想アドレスまたは物理アドレスを指定 */

	if ( ( pgsize == HAL_PAGE_SIZE_2M ) &&
	    ( !PAGE_2MB_ALIGNED(vaddr) || !PAGE_2MB_ALIGNED(paddr) ) )
		return -EINVAL;  /* 2MiB境界でない仮想アドレスまたは物理アドレスを指定 */

	if ( ( pgsize == HAL_PAGE_SIZE ) &&
	    ( !PAGE_ALIGNED(vaddr) || !PAGE_ALIGNED(paddr) ) )
		return -EINVAL;  /* 4KiB境界でない仮想アドレスまたは物理アドレスを指定 */

	cur_tbl = pgt->pgtbl_base;  /* VPN2テーブルのカーネル仮想アドレスを得る */

	tbl_changed = false;                     /* ページテーブル更新未         */
	
	/* ページテーブルを構成する
	 */
	for(lvl = RV64_PGTBL_LVL_NR; lvl > 0; --lvl) {

		rc = grow_pgtbl(pgt, cur_tbl, lvl - 1, vaddr, paddr, prot, flags, pgsize,
		    &tbl_changed, &next_tbl);  /* ページテーブルの伸張  */
		if ( rc != 0 ) 			
			goto error_out;

		if (  pgsize == rv64_pgtbl_calc_pagesize(lvl - 1) )
			goto sucess_out;  /* ページをマップした */

		cur_tbl = next_tbl; /* 次のテーブルへ */
	}

sucess_out:
	tbl_changed = true; /* ページテーブル更新 */
	rc = 0;

error_out: /* TODO: カレントページテーブルが操作対象ページテーブルの場合TLBをフラッシュする */
	/* テーブル割り当てに失敗した場合はテーブル間の参照を削除する
	 */
	if ( rc == -ENOMEM )
		remove_table_reference(pgt, vaddr);

	if ( tbl_changed )
		hal_flush_tlb(pgt);  /* テーブル更新に伴うTLBのフラッシュ */

	return rc;
}

/**
   TLBをフラッシュする
   @param[in]  pgt  ページテーブル
 */
void
hal_flush_tlb(vm_pgtbl __unused pgt){

	//rv64_flush_tlb_local();  /* 自hartのTLBをフラッシュする */
}

/**
   仮想アドレスにマップされている物理アドレスを得る
   @param[in]  pgt           ページテーブル
   @param[in]  vaddr         仮想アドレス
   @param[out] paddrp        物理アドレス返却域
   @param[out] protp         保護属性返却域
   @param[out] flagsp        マップ属性返却域
   @param[out] pgsizep       ページサイズ返却領域
   @retval     0             正常終了
   @retval    -ENOENT        ページテーブルまたはラージページがマップされていない
   @retval    -ESRCH         ページがマップされていない
 */
int
hal_pgtbl_extract(vm_pgtbl pgt, vm_vaddr vaddr, vm_paddr *paddrp, 
    vm_prot *protp, vm_flags *flagsp, vm_size *pgsizep){
	int                  rc;
	int                 lvl;
	hal_pte        *cur_tbl;
	hal_pte       *next_tbl;
	vm_paddr          paddr;
	vm_prot            prot;
	vm_flags          flags;
	vm_size          pgsize;

	cur_tbl = pgt->pgtbl_base;  /* ページテーブルベース (VPN2テーブル)を設定 */

	/* テーブルを走査する
	 */
	for(lvl = RV64_PGTBL_LVL_NR; lvl > 0; --lvl) {
		
		/* ページテーブルエントリから参照先のテーブルの情報を得る */
		rc = walk_pgtbl(cur_tbl, lvl - 1, vaddr, &paddr, &prot, 
		    &flags, &pgsize, &next_tbl);
		if ( rc != -EAGAIN ) 
			break;  /* ページがマップされていないか最終マップページを取得した */

		cur_tbl = next_tbl;  /* 次のテーブルを走査する */
	}

	if ( rc != 0 )
		goto error_out;  /* ページがマップされていなかった */
	
	/* マップ先ページの情報を返却する
	 */
	*paddrp = paddr;     /* 物理アドレスを返却する */
	*protp = prot;       /* 保護属性を返却する     */
	*flagsp = flags;     /* マップ属性を返却する   */
	*pgsizep = pgsize;   /* ページサイズを返却する */

	return 0;

error_out:
	return rc;
}

/**
   仮想アドレスから物理ページをアンマップする
   @param[in] pgt           ページテーブル
   @param[in] vaddr         仮想アドレス
   @param[in] flags         マップ属性
   @param[in] len           アンマップする長さ
   @retval    0             正常終了
   @retval   -EINVAL        ページサイズ境界と仮想アドレスまたは物理アドレスの境界が
                            あっていない
   @note      アドレス空間のmutexを獲得した状態で呼び出す
 */
void
hal_pgtbl_remove(vm_pgtbl pgt, vm_vaddr vaddr, vm_flags flags, vm_size len){
	int               rc;
	vm_vaddr   cur_vaddr;
	vm_vaddr   vaddr_sta;
	vm_vaddr   vaddr_end;
	vm_paddr       paddr;
	vm_prot         prot;
	vm_flags   map_flags;
	vm_size       pgsize;
	void         *kvaddr;
	page_frame       *pf;

	/* マップ範囲の仮想アドレスと開始物理アドレスを算出
	 */
	vaddr_sta = vaddr;  /* 開始仮想アドレス */
	vaddr_end = vaddr + len;  /* 終了仮想アドレス */

	/* ページサイズの決定
	 */
	pgsize = HAL_PAGE_SIZE;  /* ノーマルページを仮定 */

	if ( ( !PAGE_ALIGNED(vaddr_sta) )
	    || ( !PAGE_ALIGNED(vaddr_end ) ) ) 
		return ;

	for( cur_vaddr = vaddr_sta; vaddr_end > cur_vaddr; cur_vaddr += pgsize) {

		rc = hal_pgtbl_extract(pgt, cur_vaddr, &paddr, &prot, &map_flags, &pgsize);
		if ( ( rc == 0 ) && ( !( flags & VM_FLAGS_UNMANAGED ) ) ) {

			/* 割当先ページのカーネル仮想アドレスを参照 */
			rc = pfdb_paddr_to_kvaddr((void *)paddr, &kvaddr);
			kassert( rc == 0 );

			rc = pfdb_kvaddr_to_page_frame(kvaddr, &pf);
			kassert( rc == 0 );

			pfdb_dec_page_use_count(pf); /* マネージドマップの場合ページを解放 */
		}
		remove_table_reference(pgt, cur_vaddr);  /* ページテーブルの参照を落とす */
	}
}

/**
   ページをマップする
   @param[in] pgt           ページテーブル
   @param[in] vaddr         仮想アドレス
   @param[in] paddr         物理アドレス
   @param[in] prot          保護属性
   @param[in] flags         マップ属性
   @param[in] len           マップする長さ(単位:バイト)
   @retval    0             正常終了
   @retval   -ENOMEM        メモリ不足
   @retval   -EBUSY         すでにマップ済みの領域だった
   @note      アドレス空間のmutexを獲得した状態で呼び出す
 */
int
hal_pgtbl_enter(vm_pgtbl pgt, vm_vaddr vaddr, vm_paddr paddr, vm_prot prot, 
    vm_flags flags, vm_size len){
	int               rc;
	vm_vaddr   cur_vaddr;
	vm_vaddr   vaddr_sta;
	vm_vaddr   vaddr_end;
	vm_paddr   cur_paddr;
	vm_paddr   paddr_end;
	vm_size       pgsize;

	/* マップ範囲の仮想アドレスと開始物理アドレスを算出
	 */
	vaddr_sta = vaddr;  /* 開始仮想アドレス */
	vaddr_end = vaddr + len;  /* 終了仮想アドレス */
	cur_paddr = paddr; /* 開始物理アドレス */
	paddr_end = paddr + len;

	/* ページサイズの決定
	 */
	pgsize = HAL_PAGE_SIZE;  /* ノーマルページを仮定 */

	if ( ( !PAGE_ALIGNED(vaddr_sta) )
	    || ( !PAGE_ALIGNED(cur_paddr) ) 
	    || ( !PAGE_ALIGNED(vaddr_end ) )
	    || ( !PAGE_ALIGNED(paddr_end ) ) ) {
	
		kprintf(KERN_PNC "Invalid page alignment(vstart, vend, pstart, pend)\n", 
		    vaddr_sta, (vaddr_end ),
		    cur_paddr, (paddr_end ) );
		kassert_no_reach();
	}

	if ( PAGE_2MB_ALIGNED(vaddr_sta) &&
	    PAGE_2MB_ALIGNED(cur_paddr)  &&
	    PAGE_2MB_ALIGNED(vaddr_end ) &&
	    PAGE_2MB_ALIGNED(paddr_end ) )
		pgsize = HAL_PAGE_SIZE_2M; /* 2MiBを指定 */

	if ( PAGE_1GB_ALIGNED(vaddr_sta) &&
	    PAGE_1GB_ALIGNED(cur_paddr)  &&
	    PAGE_1GB_ALIGNED(vaddr_end ) &&
	    PAGE_1GB_ALIGNED(paddr_end ) )
		pgsize = HAL_PAGE_SIZE_1G; /* 1GiBを指定 */

	/* ページをマップする
	 */
	for( cur_vaddr = vaddr_sta; 
	     vaddr_end > cur_vaddr;
	     cur_vaddr += pgsize, cur_paddr += pgsize) {

		rc = map_vaddr_to_paddr(pgt, cur_vaddr, cur_paddr, prot, flags, pgsize);
		if ( rc != 0 )
			return rc;
	}

	return 0;
}

/**
   カーネルのページテーブル情報を返却する
   @return  カーネルのページテーブル情報
   @note    TODO: プロセス管理実装後にrefer_kernel_proc()に置き換える
 */
vm_pgtbl
hal_refer_kernel_pagetable(void){
	
	return kpgtbl;
}

/**
   ページテーブル情報のアーキ依存部を初期化する
   @param[in] pgt  ページテーブル情報
 */
void
hal_pgtbl_init(vm_pgtbl pgt){
	hal_pgtbl_md     *md;

	md = &pgt->md;  /* ページテーブルアーキテクチャ依存部を参照 */

	/*
	 * SATPレジスタ値を設定
	 */
	md->satp = RV64_SATP_VAL(RV64_SATP_MODE_SV39, ULONGLONG_C(0), 
	    pgt->tblbase_paddr >> PAGE_SHIFT);

	return ;
}

/**
   カーネルのページテーブルをユーザ用ページテーブルにコピーする
   @param[in] upgtbl ユーザ用ページテーブル情報
   @retval    0      正常終了
   @retval   -ENODEV ミューテックスが破棄された
   @retval   -EINTR  非同期イベントを受信した
   @note LO: 転送先(ユーザページテーブル), 転送元(カーネルページテーブル)の順にロックする
 */
int
hal_copy_kernel_pgtbl(vm_pgtbl upgtbl){
	int rc;

	rc = mutex_lock(&upgtbl->mtx);  /* ユーザ空間側のロックを獲得する  */
	if ( rc != 0 )
		goto error_out;

	if ( kpgtbl != upgtbl ) {

		rc = mutex_lock(&kpgtbl->mtx);  /* カーネル空間側のロックを獲得する  */
		if ( rc != 0 )
			goto unlock_out;
	}

	/* カーネルのVPN2テーブル (ユーザ空間側が空になっているテーブル)を
	 * コピーする
	 */
	vm_copy_kmap_page(upgtbl->pgtbl_base, kpgtbl->pgtbl_base);

	mutex_unlock(&kpgtbl->mtx);         /* カーネル空間側のロックを解放する  */

	if ( kpgtbl != upgtbl )
		mutex_unlock(&upgtbl->mtx); /* ユーザ空間側のロックを解放する  */

	return 0;

unlock_out:
	if ( kpgtbl != upgtbl )
		mutex_unlock(&upgtbl->mtx); /* ユーザ空間側のロックを解放する  */

error_out:
	return rc;
}

void
prepare_map(void){
	int               rc;
	vm_pgtbl       pgtbl;
	vm_vaddr   vaddr_sta;
	vm_paddr       paddr;
	hal_pgtbl_md     *md;

	/* カーネルのページテーブルを割り当てる
	 */
	rc = pgtbl_alloc_pgtbl(&pgtbl);
	if ( rc != 0 ) {
	
		kprintf(KERN_PNC "Can not allocate kernel page table\n");
		kassert_no_reach();
	}

	md = &pgtbl->md;  /* ページテーブルアーキテクチャ依存部を参照 */

	kprintf("Kernel base: %p I/O base:%p kpgtbl-vaddr: %p kpgtbl-paddr: %p\n", 
	    HAL_KERN_VMA_BASE, HAL_KERN_IO_BASE, pgtbl->pgtbl_base, md->satp);

	/* マップ範囲の仮想アドレスと開始物理アドレスを算出
	 */
	vaddr_sta = HAL_PHY_TO_KERN_STRAIGHT(HAL_KERN_PHY_BASE);  /* 開始仮想アドレス */
	/* 終了仮想アドレス */
	paddr = HAL_KERN_PHY_BASE; /* 開始物理アドレス */

	/* メモリをマップ
	 */
	rc = hal_pgtbl_enter(pgtbl, vaddr_sta, paddr, 
	    VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE, 
	    VM_FLAGS_WIRED | VM_FLAGS_UNMANAGED | VM_FLAGS_SUPERVISOR, 
	    MIB_TO_BYTE(KC_PHYSMEM_MB) );
	kassert( rc == 0 );
	show_page_map(pgtbl, HAL_PHY_TO_KERN_STRAIGHT(HAL_KERN_PHY_BASE), 
	    MIB_TO_BYTE(KC_PHYSMEM_MB) );
#if 0
	/* MMIOをマップ
	 */
	rc = hal_pgtbl_enter(pgtbl, RV64_UART0, RV64_UART0_PADDR,
	    VM_PROT_READ|VM_PROT_WRITE, VM_FLAGS_UNMANAGED|VM_FLAGS_SUPERVISOR, 
	    RV64_UART0_SIZE);
	show_page_map(pgtbl, RV64_UART0, RV64_UART0_SIZE);

	rc = hal_pgtbl_enter(pgtbl, RV64_PLIC, RV64_PLIC_PADDR,
	    VM_PROT_READ|VM_PROT_WRITE, VM_FLAGS_UNMANAGED|VM_FLAGS_SUPERVISOR, 
	    RV64_PLIC_SIZE);
	show_page_map(pgtbl, RV64_PLIC, RV64_PLIC_SIZE);

	rc = hal_pgtbl_enter(pgtbl, RV64_CLINT, RV64_CLINT_PADDR,
	    VM_PROT_READ|VM_PROT_WRITE, VM_FLAGS_UNMANAGED|VM_FLAGS_SUPERVISOR, 
	    RV64_CLINT_SIZE);
	show_page_map(pgtbl, RV64_CLINT, RV64_CLINT_SIZE);

	rc = hal_pgtbl_enter(pgtbl, RV64_VIRTIO0, RV64_VIRTIO0_PADDR,
	    VM_PROT_READ|VM_PROT_WRITE, VM_FLAGS_UNMANAGED|VM_FLAGS_SUPERVISOR, 
	    RV64_VIRTIO0_SIZE);
	show_page_map(pgtbl, RV64_VIRTIO0, RV64_VIRTIO0_SIZE);
#endif
	//rv64_write_satp(md->satp);  /* ページテーブル読み込み */

	kpgtbl = pgtbl; /* カーネルページテーブルを設定  */

	return;
}

static void
rv64pgtbl1(struct _ktest_stats *sp, void __unused *arg){
	int               rc;
	pfdb_stat         st;
	page_frame       *pf;

	kcom_obtain_pfdb_stat(&st);

	kprintf("Physical memory pool-before\n");
	kprintf("\tnr_pages: %qu\n", st.nr_pages);
	kprintf("\tnr_free_pages: %qu\n", st.nr_free_pages);
	kprintf("\treserved_pages: %qu\n", st.reserved_pages);
	kprintf("\tavailable_pages: %qu\n", st.available_pages);
	kprintf("\tkdata_pages: %qu\n", st.kdata_pages);
	kprintf("\tkstack_pages: %qu\n", st.kstack_pages);
	kprintf("\tpgtbl_pages: %qu\n", st.pgtbl_pages);
	kprintf("\tslab_pages: %qu\n", st.slab_pages);
	kprintf("\tanon_pages: %qu\n", st.anon_pages);
	kprintf("\tpcache_pages: %qu\n", st.pcache_pages);

	prepare_map();
	rc = pfdb_kvaddr_to_page_frame(kpgtbl->pgtbl_base, &pf);
	kassert( rc == 0 );

	pfdb_dec_page_use_count(pf);

	kcom_obtain_pfdb_stat(&st);

	kprintf("Physical memory pool-after\n");
	kprintf("\tnr_pages: %qu\n", st.nr_pages);
	kprintf("\tnr_free_pages: %qu\n", st.nr_free_pages);
	kprintf("\treserved_pages: %qu\n", st.reserved_pages);
	kprintf("\tavailable_pages: %qu\n", st.available_pages);
	kprintf("\tkdata_pages: %qu\n", st.kdata_pages);
	kprintf("\tkstack_pages: %qu\n", st.kstack_pages);
	kprintf("\tpgtbl_pages: %qu\n", st.pgtbl_pages);
	kprintf("\tslab_pages: %qu\n", st.slab_pages);
	kprintf("\tanon_pages: %qu\n", st.anon_pages);
	kprintf("\tpcache_pages: %qu\n", st.pcache_pages);
	if ( true )
		ktest_pass( sp );
	else
		ktest_fail( sp );
}

void
tst_rv64pgtbl(void){

	ktest_def_test(&tstat_rv64pgtbl, "rv64pgtbl1", rv64pgtbl1, NULL);
	ktest_run(&tstat_rv64pgtbl);
}


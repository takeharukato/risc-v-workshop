/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  RISC-V64 kernel map routines                                      */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>
#include <kern/page-if.h>
#include <kern/vm.h>

#include <hal/riscv64.h>
#include <hal/rv64-platform.h>
#include <hal/hal-pgtbl.h>

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
   カーネルメモリをマップする
   @param[in] pgt ページテーブル
 */
static void
map_kernel_memory(vm_pgtbl pgt){
	int               rc;
	vm_vaddr       vaddr;
	vm_vaddr   vaddr_sta;
	vm_vaddr   vaddr_end;
	vm_paddr       paddr;
	vm_prot         prot;
	vm_flags       flags;
	vm_size       pgsize;

	/* マップ範囲の仮想アドレスと開始物理アドレスを算出
	 */
	vaddr_sta = HAL_PHY_TO_KERN_STRAIGHT(HAL_KERN_PHY_BASE);  /* 開始仮想アドレス */
	vaddr_end = ( HAL_PHY_TO_KERN_STRAIGHT(HAL_KERN_PHY_BASE)
	    + MIB_TO_BYTE(KC_PHYSMEM_MB) );  /* 終了仮想アドレス */
	paddr = HAL_KERN_PHY_BASE; /* 開始物理アドレス */

	/* ページサイズの決定
	 */
	pgsize = HAL_PAGE_SIZE;  /* ノーマルページを仮定 */

	if ( ( !PAGE_ALIGNED(vaddr_sta) )
	    || ( !PAGE_ALIGNED(paddr) ) 
	    || ( !PAGE_ALIGNED(vaddr_sta + MIB_TO_BYTE(KC_PHYSMEM_MB) ) )
	    || ( !PAGE_ALIGNED(paddr + MIB_TO_BYTE(KC_PHYSMEM_MB) ) ) ) {
	
		kprintf("Invalid page alignment(vstart, vend, pstart, pend)\n", 
		    vaddr_sta, (vaddr_sta + MIB_TO_BYTE(KC_PHYSMEM_MB) ),
		    paddr, (paddr + MIB_TO_BYTE(KC_PHYSMEM_MB) ) );
		kassert_no_reach();
	}

	if ( PAGE_2MB_ALIGNED(vaddr_sta) &&
	    PAGE_2MB_ALIGNED(paddr)  &&
	    PAGE_2MB_ALIGNED(vaddr_sta + MIB_TO_BYTE(KC_PHYSMEM_MB) ) &&
	    PAGE_2MB_ALIGNED(paddr + MIB_TO_BYTE(KC_PHYSMEM_MB) ) )
		pgsize = HAL_PAGE_SIZE_2M; /* 2MiBを指定 */

	if ( PAGE_1GB_ALIGNED(vaddr_sta) &&
	    PAGE_1GB_ALIGNED(paddr)  &&
	    PAGE_1GB_ALIGNED(vaddr_sta + MIB_TO_BYTE(KC_PHYSMEM_MB) ) &&
	    PAGE_1GB_ALIGNED(paddr + MIB_TO_BYTE(KC_PHYSMEM_MB) ) )
		pgsize = HAL_PAGE_SIZE_1G; /* 1GiBを指定 */

	/* ページをマップする
	 */
	for( vaddr = vaddr_sta;	vaddr_end > vaddr; vaddr += pgsize, paddr += pgsize) {

		rc = hal_pgtbl_enter(pgt, vaddr, paddr, 
		    VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE, 
		    VM_FLAGS_WIRED | VM_FLAGS_UNMANAGED | VM_FLAGS_SUPERVISOR, 
		    pgsize);
		kassert( rc == 0 );
	}

	/* マップを確認
	 */
	for( vaddr = vaddr_sta;	vaddr_end > vaddr; vaddr += pgsize, paddr += pgsize) {

		rc = hal_pgtbl_extract(pgt, vaddr,&paddr, 
		    &prot, &flags, &pgsize);
		kassert( rc == 0 );
		kprintf("vaddr=%p paddr=%p pgsize=%ld prot=[%c|%c|%c] flags=[%c]\n",
		    vaddr, paddr, pgsize,
		    ( (prot & VM_PROT_READ)         ? ('R') : ('-') ),
		    ( (prot & VM_PROT_WRITE)        ? ('W') : ('-') ),
		    ( (prot & VM_PROT_EXECUTE)      ? ('E') : ('-') ),
		    ( (flags & VM_FLAGS_SUPERVISOR) ? ('S') : ('U') ));
	}
}

/**
   TLBをフラッシュする
   @param[in]  pgt  ページテーブル
 */
void
hal_flush_tlb(vm_pgtbl __unused pgt){

	rv64_flush_tlb_local();  /* 自hartのTLBをフラッシュする */
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
	int            vpn2_idx;
	int            vpn1_idx;
	int            vpn0_idx;
	vm_paddr vpn1_tbl_paddr;
	vm_paddr vpn0_tbl_paddr;
	hal_pte        vpn2_pte;
	hal_pte        vpn1_pte;
	hal_pte        vpn0_pte;
	hal_pte       *vpn2_tbl;
	hal_pte       *vpn1_tbl;
	hal_pte       *vpn0_tbl;
	vm_paddr          paddr;
	vm_prot            prot;
	vm_flags          flags;

	prot = VM_PROT_NONE;   /* アクセス不能に初期化 */
	flags = VM_FLAGS_NONE; /* 無属性に初期化  */

	/*
	 * VPN2テーブル
	 */
	vpn2_tbl = pgt->pgtbl_base;  /* VPN2テーブルのカーネル仮想アドレスを得る */
	vpn2_idx = rv64_pgtbl_vpn2_index(vaddr); /* VPN2テーブルインデクスを得る */
	vpn2_pte = vpn2_tbl[vpn2_idx];  /* ページテーブルエントリを得る */

	if ( !RV64_PTE_IS_VALID(vpn2_pte) ) {  /* 有効エントリでなければ抜ける */

		rc = -ENOENT;   /* ページテーブルがない  */
		goto error_out;
	}

	if ( RV64_PTE_IS_LEAF(vpn2_pte) ) {  /* ページマップエントリの場合 */

		paddr = RV64_PTE_1GPA_TO_PADDR(vpn2_pte); /* 1GiBページのアドレスを得る */
		pte_to_vmprot_and_vmflags(vpn2_pte, &prot, &flags); /* 保護属性を得る */
		*pgsizep = HAL_PAGE_SIZE_1G;               /* ページサイズを返却 */
		goto walk_end;
	}

	/* VPN1テーブルの物理アドレスを得る */
	vpn1_tbl_paddr = RV64_PTE_TO_PADDR(vpn2_pte); 
	/* VPN1テーブルのカーネル仮想アドレスを得る */
	vpn1_tbl = (hal_pte *)HAL_PHY_TO_KERN_STRAIGHT(vpn1_tbl_paddr); 

	/*
	 * VPN1テーブル
	 */
	vpn1_idx = rv64_pgtbl_vpn1_index(vaddr); /* VPN1テーブルインデクスを得る */
	vpn1_pte = vpn1_tbl[vpn1_idx];  /* ページテーブルエントリを得る */

	if ( !RV64_PTE_IS_VALID(vpn1_pte) ) {  /* 有効エントリでなければ抜ける */

		rc = -ENOENT;  /* ページテーブルがない  */
		goto error_out;
	}

	if ( RV64_PTE_IS_LEAF(vpn1_pte) ) {  /* ページマップエントリの場合 */

		paddr = RV64_PTE_2MPA_TO_PADDR(vpn1_pte); /* 2MiBページのアドレスを得る */
		pte_to_vmprot_and_vmflags(vpn1_pte, &prot, &flags); /* 保護属性を得る */
		*pgsizep = HAL_PAGE_SIZE_2M;               /* ページサイズを返却 */
		goto walk_end;
	}

	/* VPN0テーブルの物理アドレスを得る */
	vpn0_tbl_paddr = RV64_PTE_TO_PADDR(vpn1_pte); 
	/* VPN0テーブルのカーネル仮想アドレスを得る */
	vpn0_tbl = (hal_pte *)HAL_PHY_TO_KERN_STRAIGHT(vpn0_tbl_paddr); 

	/*
	 * VPN0テーブル
	 */
	vpn0_idx = rv64_pgtbl_vpn0_index(vaddr); /* VPN1テーブルインデクスを得る */
	vpn0_pte = vpn0_tbl[vpn0_idx];  /* ページテーブルエントリを得る */

	if ( !RV64_PTE_IS_VALID(vpn0_pte) ) {  /* 有効エントリでなければ抜ける */

		rc = -ENOENT;
		goto error_out;
	}

	if ( !RV64_PTE_IS_LEAF(vpn0_pte) ) {  /* ページマップエントリでなければ抜ける */

		rc = -ESRCH;                  /* ページがマップされていない  */
		goto error_out;
	}

	paddr = RV64_PTE_TO_PADDR(vpn0_pte);  /* ページのアドレスを得る */
	pte_to_vmprot_and_vmflags(vpn0_pte, &prot, &flags); /* 保護属性を得る */
	*pgsizep = HAL_PAGE_SIZE;               /* ページサイズを返却 */

walk_end:
	*paddrp = paddr;  /* 物理アドレスを返却する */
	*protp = prot;    /* 保護属性を返却する     */
	*flagsp = flags;  /* マップ属性を返却する   */

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
int
hal_pgtbl_enter(vm_pgtbl pgt, vm_vaddr vaddr, vm_paddr paddr, 
    vm_prot prot, vm_flags flags, vm_size pgsize){
	int                    rc;
	hal_pte      cur_vpn2_pte;
	hal_pte      cur_vpn1_pte;
	hal_pte          vpn2_pte;
	hal_pte          vpn1_pte;
	hal_pte          vpn0_pte;
	int              vpn2_idx;
	int              vpn1_idx;
	int              vpn0_idx;
	hal_pte         *vpn2_tbl;
	hal_pte         *vpn1_tbl;
	hal_pte         *vpn0_tbl;
	hal_pte           new_pte;
	vm_paddr   vpn1_tbl_paddr;
	vm_paddr   vpn0_tbl_paddr;
	vm_paddr        tbl_paddr;
	page_frame    *cur_tbl_pf;
	bool                  res;

	/* 引数チェック
	 */
	if ( ( pgsize == HAL_PAGE_SIZE_1G ) &&
	    ( !PAGE_1GB_ALIGNED(vaddr) || !PAGE_1GB_ALIGNED(paddr) ) )
		return -EINVAL;  /* 1GiB境界でない仮想アドレスまたは物理アドレスを指定 */

	if ( ( pgsize == HAL_PAGE_SIZE_2M ) &&
	    ( !PAGE_2MB_ALIGNED(vaddr) || !PAGE_2MB_ALIGNED(paddr) ) )
		return -EINVAL;  /* 2GiB境界でない仮想アドレスまたは物理アドレスを指定 */

	if ( ( pgsize == HAL_PAGE_SIZE ) &&
	    ( !PAGE_ALIGNED(vaddr) || !PAGE_ALIGNED(paddr) ) )
		return -EINVAL;  /* 4KiB境界でない仮想アドレスまたは物理アドレスを指定 */

	cur_vpn2_pte = cur_vpn1_pte = vpn2_pte = vpn1_pte = 0; /* PTE値初期化 */

	/* PTE属性値を算出 */
	vmprot_and_vmflags_to_pte(prot, flags, &new_pte);

	vpn2_tbl = pgt->pgtbl_base;  /* VPN2テーブルのカーネル仮想アドレスを得る */

	/*
	 * VPN2テーブル
	 */
	vpn2_idx = rv64_pgtbl_vpn2_index(vaddr); /* VPN2テーブルインデクスを得る */
	cur_vpn2_pte = vpn2_tbl[vpn2_idx];  /* ページテーブルエントリを得る */
	/* VPN2テーブルのページフレーム情報を得る */
	rc = pfdb_kvaddr_to_page_frame((void *)vpn2_tbl, &cur_tbl_pf); 
	kassert( rc == 0);

	if ( RV64_PTE_IS_VALID(cur_vpn2_pte) )
		vpn2_pte = cur_vpn2_pte;  /* vpn2_pteを設定 */
	else { /* 有効エントリでなければページを割当てる */

		if ( ( RV64_PTE_IS_LEAF(new_pte) ) && ( pgsize == HAL_PAGE_SIZE_1G ) ) {

			/*
			 * 1GiBページのマップ
			 */

			/* 属性値に物理ページ番号値を付加しPTEを生成 */
			vpn2_pte = ( RV64_PTE_V | new_pte | RV64_PTE_1GPADDR_TO_PPN(paddr) );
			vpn2_tbl[vpn2_idx] = vpn2_pte;  /* PTEを更新 */
			res = pfdb_inc_page_use_count(cur_tbl_pf); /* 参照数を加算 */
			kassert( res );  /* 加算できたことを確認 */
			goto map_end; /* マップ処理終了 */
		}

		/* ページテーブル割り当て
		 */
		rc = pgtbl_alloc_pgtbl_page(&vpn1_tbl, &tbl_paddr);
		if ( rc != 0 ) {

			rc = -ENOMEM;   /* メモリ不足  */
			goto error_out;
		}
		
		/* 属性値に物理ページ番号値を付加しVPN1テーブルへのPTEを生成 */
		vpn2_pte = 
			( RV64_PTE_V|RV64_PTE_PGTBL_FLAGS|RV64_PTE_PADDR_TO_PPN(tbl_paddr) );
		vpn2_tbl[vpn2_idx] = vpn2_pte; /* PTEを更新 */
		res = pfdb_inc_page_use_count(cur_tbl_pf); /* 参照数を加算 */
		kassert( res );  /* 加算できたことを確認 */
	}

	/* VPN1テーブルの物理アドレスを得る */
	vpn1_tbl_paddr = RV64_PTE_TO_PADDR(vpn2_pte); 
	/* VPN1テーブルのカーネル仮想アドレスを得る */
	vpn1_tbl = (hal_pte *)HAL_PHY_TO_KERN_STRAIGHT(vpn1_tbl_paddr); 

	/*
	 * VPN1テーブル
	 */
	vpn1_idx = rv64_pgtbl_vpn1_index(vaddr); /* VPN1テーブルインデクスを得る */
	cur_vpn1_pte = vpn1_tbl[vpn1_idx];  /* ページテーブルエントリを得る */
	/* VPN1テーブルのページフレーム情報を得る */
	rc = pfdb_kvaddr_to_page_frame((void *)vpn1_tbl, &cur_tbl_pf); 
	kassert( rc == 0);
	
	if ( RV64_PTE_IS_VALID(cur_vpn1_pte) ) 
		vpn1_pte = cur_vpn1_pte;  /* vpn1_pteを設定 */
	else { /* 有効エントリでなければページを割当てる */

		if ( ( RV64_PTE_IS_LEAF(new_pte) ) && ( pgsize == HAL_PAGE_SIZE_2M ) ) {

			/*
			 * 2MiBページのマップ
			 */

			/* 属性値に物理ページ番号値を付加しPTEを生成 */
			vpn1_pte = ( RV64_PTE_V | new_pte | RV64_PTE_2MPADDR_TO_PPN(paddr) );
			vpn1_tbl[vpn1_idx] = vpn1_pte;  /* PTEを更新 */
			res = pfdb_inc_page_use_count(cur_tbl_pf); /* 参照数を加算 */
			kassert( res );  /* 加算できたことを確認 */
			goto map_end; /* マップ処理終了 */
		}

		/* ページテーブル割り当て
		 */
		rc = pgtbl_alloc_pgtbl_page(&vpn0_tbl, &tbl_paddr);
		if ( rc != 0 ) {

			rc = -ENOMEM;   /* メモリ不足  */
			goto error_out;
		}
		
		/* 属性値に物理ページ番号値を付加しVPN0テーブルへのPTEを生成 */
		vpn1_pte = 
			( RV64_PTE_V|RV64_PTE_PGTBL_FLAGS|RV64_PTE_PADDR_TO_PPN(tbl_paddr) );
		vpn1_tbl[vpn1_idx] = vpn1_pte; /* PTEを更新 */
		res = pfdb_inc_page_use_count(cur_tbl_pf); /* 参照数を加算 */
		kassert( res );  /* 加算できたことを確認 */
	}

	/* VPN0テーブルの物理アドレスを得る */
	vpn0_tbl_paddr = RV64_PTE_TO_PADDR(vpn1_pte); 
	/* VPN0テーブルのカーネル仮想アドレスを得る */
	vpn0_tbl = (hal_pte *)HAL_PHY_TO_KERN_STRAIGHT(vpn0_tbl_paddr); 

	/*
	 * VPN0テーブル
	 */
	vpn0_idx = rv64_pgtbl_vpn0_index(vaddr); /* VPN0テーブルインデクスを得る */
	vpn0_pte = vpn0_tbl[vpn0_idx];  /* ページテーブルエントリを得る */
	/* VPN1テーブルのページフレーム情報を得る */
	rc = pfdb_kvaddr_to_page_frame((void *)vpn1_tbl, &cur_tbl_pf); 
	kassert( rc == 0);

	/* 属性値に物理ページ番号値を付加しページへのPTEを生成 */
	vpn0_pte = ( RV64_PTE_V | new_pte | RV64_PTE_PADDR_TO_PPN(paddr) );
	vpn0_tbl[vpn0_idx] = vpn0_pte; /* PTEを更新 */
	res = pfdb_inc_page_use_count(cur_tbl_pf); /* 参照数を加算 */
	kassert( res );  /* 加算できたことを確認 */
	
map_end: /* TODO: カレントページテーブルが操作対象ページテーブルの場合TLBをフラッシュする */
	hal_flush_tlb(pgt);  /* テーブル更新に伴うTLBのフラッシュ */

	return 0;

error_out: /* TODO: カレントページテーブルが操作対象ページテーブルの場合TLBをフラッシュする */
	if ( ( cur_vpn2_pte != vpn2_pte ) || ( cur_vpn1_pte != vpn1_pte ) )
		hal_flush_tlb(pgt);  /* テーブル更新に伴うTLBのフラッシュ */

	return rc;
}

/**
   仮想アドレスから物理ページをアンマップする
   @param[in] pgt           ページテーブル
   @param[in] vaddr         仮想アドレス
   @param[in] pgsize        ページサイズ
   @retval    0             正常終了
   @retval   -EINVAL        ページサイズ境界と仮想アドレスまたは
                            物理アドレスの境界があっていない
   @retval   -ENOENT        メモリが割り当てられていない
   @note     ページテーブルベースの解放(VPN2->VPN1参照数を減算)は, 呼び出し元が行う
 */
int
hal_pgtbl_remove(vm_pgtbl pgt, vm_vaddr vaddr, vm_size pgsize){
	int                    rc;
	int              vpn2_idx;
	int              vpn1_idx;
	int              vpn0_idx;
	hal_pte      cur_vpn2_pte;
	hal_pte      cur_vpn1_pte;
	hal_pte          vpn2_pte;
	hal_pte          vpn1_pte;
	hal_pte          vpn0_pte;
	hal_pte         *vpn2_tbl;
	hal_pte         *vpn1_tbl;
	hal_pte         *vpn0_tbl;
	vm_paddr   vpn1_tbl_paddr;
	vm_paddr   vpn0_tbl_paddr;
	page_frame   *vpn2_tbl_pf;
	page_frame   *vpn1_tbl_pf;
	page_frame   *vpn0_tbl_pf;
	bool                  res;

	/* 引数チェック
	 */
	if ( ( pgsize == HAL_PAGE_SIZE_1G ) && ( !PAGE_1GB_ALIGNED(vaddr) ) )
		return -EINVAL;  /* 1GiB境界でない仮想アドレスまたは物理アドレスを指定 */

	if ( ( pgsize == HAL_PAGE_SIZE_2M ) && ( !PAGE_2MB_ALIGNED(vaddr) ) )
		return -EINVAL;  /* 2GiB境界でない仮想アドレスまたは物理アドレスを指定 */

	if ( ( pgsize == HAL_PAGE_SIZE ) && ( !PAGE_ALIGNED(vaddr) ) )
		return -EINVAL;  /* 4KiB境界でない仮想アドレスまたは物理アドレスを指定 */

	vpn2_tbl = pgt->pgtbl_base;  /* VPN2テーブルのカーネル仮想アドレスを得る */


	/*
	 * VPN2テーブル
	 */
	vpn2_idx = rv64_pgtbl_vpn2_index(vaddr); /* VPN2テーブルインデクスを得る */
	cur_vpn2_pte = vpn2_pte = vpn2_tbl[vpn2_idx];  /* ページテーブルエントリを得る */
	/* VPN2テーブルのページフレーム情報を得る */
	rc = pfdb_kvaddr_to_page_frame((void *)vpn2_tbl, &vpn2_tbl_pf); 
	kassert( rc == 0);
	/* satp->VPN2親参照数を減算は, 呼び出し元が行う */

	if ( !RV64_PTE_IS_VALID(vpn2_pte) ) {
		
		rc = -ENOENT;   /* ページテーブルがない  */
		goto error_out;
	}

	if ( pgsize == HAL_PAGE_SIZE_1G ) {

		if ( !RV64_PTE_IS_LEAF(vpn2_pte) ) { 

			rc = -ENOENT;   /* 1GiBページが割り振られていない  */
			goto error_out;
		}

		/* 1GiBページマップエントリをクリアする */
		vpn2_pte = RV64_PTE_1GPADDR_TO_PPN(0);
		vpn2_tbl[vpn2_idx] = vpn2_pte;  /* PTEを更新 */
		res = pfdb_dec_page_use_count(vpn2_tbl_pf); /* VPN2テーブル参照数を減算 */
		kassert( res );  /* 減算できたことを確認 */
		goto walk_end;
	}
	
	/* VPN1テーブルの物理アドレスを得る */
	vpn1_tbl_paddr = RV64_PTE_TO_PADDR(vpn2_pte); 
	/* VPN1テーブルのカーネル仮想アドレスを得る */
	vpn1_tbl = (hal_pte *)HAL_PHY_TO_KERN_STRAIGHT(vpn1_tbl_paddr); 

	/* VPN2-VPN1テーブル参照をクリアする */
	vpn2_tbl[vpn2_idx] = RV64_PTE_PADDR_TO_PPN(0);  /* PTEを更新 */
	res = pfdb_dec_page_use_count(vpn2_tbl_pf); /* VPN2テーブル参照数を減算 */
	kassert( res );  /* 減算できたことを確認 */


	/*
	 * VPN1テーブル
	 */
	vpn1_idx = rv64_pgtbl_vpn1_index(vaddr); /* VPN1テーブルインデクスを得る */
	cur_vpn1_pte = vpn1_pte = vpn1_tbl[vpn1_idx];  /* ページテーブルエントリを得る */
	/* VPN1テーブルのページフレーム情報を得る */
	rc = pfdb_kvaddr_to_page_frame((void *)vpn1_tbl, &vpn1_tbl_pf); 
	kassert( rc == 0);
	res = pfdb_dec_page_use_count(vpn1_tbl_pf); /* VPN2->VPN1親参照数を減算 */
	if ( res ) {  /* 最終参照者だった場合 */

		/*  VPN1ページテーブルが空になった
		 */
		rc = -ENOENT;   /* ページが割り振られていない  */
		goto error_out;
	}

	if ( !RV64_PTE_IS_VALID(vpn1_pte) ) {
		
		rc = -ENOENT;   /* ページテーブルがない  */
		goto error_out;
	}

	if ( pgsize == HAL_PAGE_SIZE_2M ) {

		if ( !RV64_PTE_IS_LEAF(vpn1_pte) ) { 

			rc = -ENOENT;   /* 2MiBページが割り振られていない  */
			goto error_out;
		}

		/* 2MiBページマップエントリをクリアする */
		vpn1_pte = RV64_PTE_2MPADDR_TO_PPN(0);
		vpn1_tbl[vpn1_idx] = vpn1_pte;  /* PTEを更新 */
		res = pfdb_dec_page_use_count(vpn1_tbl_pf); /* VPN1テーブル参照数を減算 */
		kassert( res );  /* 減算できたことを確認 */
		goto walk_end;
	}
	
	/* VPN0テーブルの物理アドレスを得る */
	vpn0_tbl_paddr = RV64_PTE_TO_PADDR(vpn1_pte); 
	/* VPN0テーブルのカーネル仮想アドレスを得る */
	vpn0_tbl = (hal_pte *)HAL_PHY_TO_KERN_STRAIGHT(vpn0_tbl_paddr); 

	/* VPN1->VPN0テーブル参照をクリアする */
	vpn1_tbl[vpn1_idx] = RV64_PTE_PADDR_TO_PPN(0);  /* PTEを更新 */
	res = pfdb_dec_page_use_count(vpn1_tbl_pf);     /* VPN1テーブル参照数を減算 */
	kassert( res );  /* 減算できたことを確認 */

	/*
	 * VPN0テーブル
	 */
	vpn0_idx = rv64_pgtbl_vpn0_index(vaddr); /* VPN0テーブルインデクスを得る */
	vpn0_pte = vpn1_tbl[vpn0_idx];  /* ページテーブルエントリを得る */

	/* VPN0テーブルのページフレーム情報を得る */
	rc = pfdb_kvaddr_to_page_frame((void *)vpn0_tbl, &vpn0_tbl_pf); 
	kassert( rc == 0);
	res = pfdb_dec_page_use_count(vpn0_tbl_pf); /* VPN1->VPN0親参照数を減算 */
	if ( res ) {  /* 最終参照者だった場合 */

		/*  VPN0ページテーブルが空になった
		 */
		rc = -ENOENT;   /* ページが割り振られていない  */
		goto error_out;
	}

	if ( ( !RV64_PTE_IS_VALID(vpn0_pte) ) || ( !RV64_PTE_IS_LEAF(vpn0_pte) ) ) {

		/*  有効なページテーブルエントリでないか
		 *  ページマップエントリでない場合はエラー復帰する
		 */
		rc = -ENOENT;   /* ページが割り振られていない  */
		goto error_out;
	}

	/* ページマップエントリをクリアする */
	vpn0_pte = RV64_PTE_PADDR_TO_PPN(0);
	vpn0_tbl[vpn0_idx] = vpn0_pte;  /* PTEを更新 */
	res = pfdb_dec_page_use_count(vpn0_tbl_pf); /* VPN0参照数を減算 */
	kassert( res );  /* 減算できたことを確認 */

walk_end: /* TODO: カレントページテーブルが操作対象ページテーブルの場合TLBをフラッシュする */
	hal_flush_tlb(pgt);  /* テーブル更新に伴うTLBのフラッシュ */

	return 0;

error_out: /* TODO: カレントページテーブルが操作対象ページテーブルの場合TLBをフラッシュする */
	if ( ( cur_vpn2_pte != vpn2_pte ) || ( cur_vpn1_pte != vpn1_pte ) )
		hal_flush_tlb(pgt);  /* テーブル更新に伴うTLBのフラッシュ */

	return rc;
}

/**
   カーネル空間をマップする
   @param[out] pgtp ページテーブル
 */
void
hal_map_kernel_space(vm_pgtbl *pgtp){
	int               rc;
	vm_pgtbl       pgtbl;
	vm_paddr   tbl_paddr;

	/* カーネルのページテーブルを割り当てる
	 */
	pgtbl = kmalloc(sizeof(vm_pgtbl_type), KMALLOC_NORMAL);	
	if ( pgtbl == NULL ) {
	
		kprintf("Can not allocate kernel page table\n");
		kassert_no_reach();
	}

	/* カーネルのページテーブルベースページを割り当てる
	 */
	rc = pgtbl_alloc_pgtbl_page(&pgtbl->pgtbl_base, &tbl_paddr);
	if ( rc != 0 ) {
	
		kprintf("Can not allocate kernel page table base:rc = %d\n", rc);
		kassert_no_reach();
	}

	kprintf("Kernel base: %p I/O base:%p nr:%d\n", 
	    HAL_KERN_VMA_BASE, HAL_KERN_IO_BASE, RV64_BOOT_PGTBL_VPN1_NR);
	map_kernel_memory(pgtbl);

	*pgtp = pgtbl; /* カーネルページテーブルを返却  */

	return;
}

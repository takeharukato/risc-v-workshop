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
   ページテーブルエントリの内容を表示する
   @param[in] lvl      テーブルのレベル
   @param[in] idx      テーブル中のインデックス
   @param[in] phy_ptep ページテーブルエントリの物理アドレス
   @param[in] pte      ページテーブルエントリ値
 */
void
rv64_pte_show(int lvl, int idx, vm_paddr phy_ptep, hal_pte pte){
	vm_paddr paddr;

	/*
	 * ページ参照エントリから物理ページアドレスを算出する
	 */
	if ( RV64_PTE_IS_LEAF(pte) ) {

		switch(lvl) {

		case 0: /* Level 0: ノーマルページサイズ */
			paddr = RV64_PTE_TO_PADDR(pte);
			break;

		case 1: /* Level 1: 2MiBページサイズ */
			paddr = RV64_PTE_2MPA_TO_PADDR(pte);
			break;

		case 2: /* Level 2: 1GiBページサイズ */
			paddr = RV64_PTE_1GPA_TO_PADDR(pte);
			break;

		default:
			kassert_no_reach();
			break;
		}
	} else 
		paddr = RV64_PTE_TO_PADDR(pte);  /* ページテーブルはノーマルページサイズ */

	kprintf("vpn%d[%d]=%p paddr=%p flags=[%c|%c|%c|%c|%c|%c|%c|%c]\n",
		lvl, idx, phy_ptep, paddr,
		( ( RV64_PTE_FLAGS(pte) & RV64_PTE_V ) ? ('V') : ('-') ),
		( ( RV64_PTE_FLAGS(pte) & RV64_PTE_R ) ? ('R') : ('-') ),
		( ( RV64_PTE_FLAGS(pte) & RV64_PTE_W ) ? ('W') : ('-') ),
		( ( RV64_PTE_FLAGS(pte) & RV64_PTE_X ) ? ('X') : ('-') ),
		( ( RV64_PTE_FLAGS(pte) & RV64_PTE_U ) ? ('U') : ('-') ),
		( ( RV64_PTE_FLAGS(pte) & RV64_PTE_G ) ? ('G') : ('-') ),
		( ( RV64_PTE_FLAGS(pte) & RV64_PTE_A ) ? ('A') : ('-') ),
		( ( RV64_PTE_FLAGS(pte) & RV64_PTE_D ) ? ('D') : ('-') ));
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
   TLBをフラッシュする
 */
void
rv64_flush_tlb(void){

	rv64_flush_tlb_local();  /* 自hartのTLBをフラッシュする */
}

/**
   仮想アドレスにマップされている物理アドレスを得る
   @param[in]  kv_pgtbl_base ページテーブルベース
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
rv64_pgtbl_extract(void *kv_pgtbl_base, vm_vaddr vaddr, vm_paddr *paddrp, 
    vm_prot *protp, vm_flags *flagsp, vm_size *pgsizep){
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
	int                  rc;

	prot = VM_PROT_NONE;   /* アクセス不能に初期化 */
	flags = VM_FLAGS_NONE; /* 無属性に初期化  */

	/*
	 * VPN2テーブル
	 */
	vpn2_tbl = kv_pgtbl_base;  /* VPN2テーブルのカーネル仮想アドレスを得る */
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
	*paddrp = paddr;
	*protp = prot;
	*flagsp = flags;

	return 0;

error_out:
	return rc;
}

/**
   仮想アドレスに物理アドレスをマップする
   @param[in] kv_pgtbl_base ページテーブルベース
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
rv64_pgtbl_enter(void *kv_pgtbl_base, vm_vaddr vaddr, vm_paddr paddr, 
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

	vpn2_tbl = kv_pgtbl_base;  /* VPN2テーブルのカーネル仮想アドレスを得る */

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
			( RV64_PTE_V | new_pte | RV64_PTE_PADDR_TO_PPN(tbl_paddr) );
		vpn2_tbl[vpn2_idx] = vpn2_pte; /* PTEを更新 */
		res = pfdb_inc_page_use_count(cur_tbl_pf); /* 参照数を加算 */
		kassert( res );  /* 加算できたことを確認 */
	}

	if ( RV64_PTE_IS_LEAF(vpn2_pte) ) {  /* 有効なページマップエントリの場合 */

		rc = -EBUSY;                 /* すでにページがマップされている  */
		goto error_out;
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
			( RV64_PTE_V | new_pte | RV64_PTE_PADDR_TO_PPN(tbl_paddr) );
		vpn1_tbl[vpn1_idx] = vpn1_pte; /* PTEを更新 */
		res = pfdb_inc_page_use_count(cur_tbl_pf); /* 参照数を加算 */
		kassert( res );  /* 加算できたことを確認 */
	}

	if ( RV64_PTE_IS_LEAF(vpn1_pte) ) {  /* 有効なページマップエントリの場合 */

		rc = -EBUSY;                 /* すでにページがマップされている  */
		goto error_out;
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

	  /* 有効なページマップエントリの場合 */
	if ( RV64_PTE_IS_VALID(vpn0_pte) ) {

		rc = -EBUSY;                 /* すでにページがマップされている  */
		goto error_out;
	}

	/* 属性値に物理ページ番号値を付加しページへのPTEを生成 */
	vpn0_pte = ( RV64_PTE_V | new_pte | RV64_PTE_PADDR_TO_PPN(paddr) );
	vpn0_tbl[vpn0_idx] = vpn0_pte; /* PTEを更新 */
	res = pfdb_inc_page_use_count(cur_tbl_pf); /* 参照数を加算 */
	kassert( res );  /* 加算できたことを確認 */
	
map_end:
	rv64_flush_tlb();  /* テーブル更新に伴うTLBのフラッシュ */

	return 0;

error_out:
	if ( ( cur_vpn2_pte != vpn2_pte ) || ( cur_vpn1_pte != vpn1_pte ) )
		rv64_flush_tlb();  /* テーブル更新に伴うTLBのフラッシュ */

	return rc;
}
/**
   カーネル空間をマップする
 */
int
rv64_map_kernel_space(void){
#if 0
	int               rc;
	vm_vaddr       vaddr;
	vm_paddr       paddr;
	vm_prot         prot;
	vm_size       pgsize;
#endif
	kprintf("Kernel base: %p I/O base:%p nr:%d\n", HAL_KERN_VMA_BASE, HAL_KERN_IO_BASE, RV64_BOOT_PGTBL_VPN1_NR);

	return 0;
}

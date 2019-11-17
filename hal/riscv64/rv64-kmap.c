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
rv64_show_pte(int lvl, int idx, vm_paddr phy_ptep, hal_pte pte){
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
   ページテーブルエントリから保護属性に変換する
   @param[in] pte ページテーブルエントリ
   @param[in] protp 保護属性返却域
 */
void
rv64_pgtbl_pte_to_vmprot(hal_pte pte, vm_prot *protp){
	vm_prot prot;

	prot = VM_PROT_NONE;
	if ( !RV64_PTE_IS_VALID(pte) )
		goto out; /* アクセス不能 */

	if ( RV64_PTE_FLAGS(pte) & RV64_PTE_R )	
		prot |= VM_PROT_READ;  /* 読み込み可能  */
	if ( RV64_PTE_FLAGS(pte) & RV64_PTE_W )	
		prot |= VM_PROT_WRITE;  /* 書き込み可能 */
	if ( RV64_PTE_FLAGS(pte) & RV64_PTE_X )	
		prot |= VM_PROT_EXEC;  /* 実行可能      */

out:
	*protp = prot;  /* アクセス属性を返却 */
	return;
}

/**
   仮想アドレスにマップされている物理アドレスを得る
   @param[in]  kv_pgtbl_base ページテーブルベース
   @param[in]  vaddr         仮想アドレス
   @param[out] paddrp        物理アドレス返却域
   @param[out] protp         保護属性返却域
   @param[out] pgsizep       ページサイズ返却領域
 */
int
rv64_pgtbl_extract(void *kv_pgtbl_base, vm_vaddr vaddr, vm_paddr *paddrp, 
    vm_prot *protp, vm_size *pgsizep){
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
	int                  rc;

	prot = VM_PROT_NONE;  /* アクセス不能に初期化 */

	/*
	 * VPN2テーブル
	 */
	vpn2_tbl = kv_pgtbl_base;  /* VPN2テーブルのカーネル仮想アドレスを得る */
	vpn2_idx = rv64_pgtbl_vpn2_index(vaddr); /* VPN2テーブルインデクスを得る */
	vpn2_pte = vpn2_tbl[vpn2_idx];  /* ページテーブルエントリを得る */

	if ( !RV64_PTE_IS_VALID(vpn2_pte) ) {  /* 有効エントリでなければ抜ける */

		rc = -ENOENT;
		goto error_out;
	}

	if ( RV64_PTE_IS_LEAF(vpn2_pte) ) {  /* ページマップエントリの場合 */

		paddr = RV64_PTE_1GPA_TO_PADDR(vpn2_pte); /* 1GiBページのアドレスを得る */
		rv64_pgtbl_pte_to_vmprot(vpn2_pte, &prot); /* 保護属性を得る */
		*pgsizep = HAL_PAGE_SIZE_1G;               /* ページサイズを返却 */
		goto walk_end;
	}

	/* VPN1テーブルの物理アドレスを得る */
	vpn1_tbl_paddr = RV64_PTE_TO_PADDR(vpn2_pte); 
	/* VPN1テーブルのカーネル仮想アドレスを得る */
#if 1 /* TODO: */
	vpn1_tbl = (hal_pte *)vpn1_tbl_paddr; 
#else
	vpn1_tbl = (hal_pte *)HAL_PHY_TO_KERN_STRAIGHT(vpn1_tbl_paddr); 
#endif
	/*
	 * VPN1テーブル
	 */
	vpn1_idx = rv64_pgtbl_vpn1_index(vaddr); /* VPN1テーブルインデクスを得る */
	vpn1_pte = vpn1_tbl[vpn1_idx];  /* ページテーブルエントリを得る */

	if ( !RV64_PTE_IS_VALID(vpn1_pte) ) {  /* 有効エントリでなければ抜ける */

		rc = -ENOENT;
		goto error_out;
	}

	if ( RV64_PTE_IS_LEAF(vpn1_pte) ) {  /* ページマップエントリの場合 */

		paddr = RV64_PTE_2MPA_TO_PADDR(vpn1_pte); /* 2MiBページのアドレスを得る */
		rv64_pgtbl_pte_to_vmprot(vpn1_pte, &prot); /* 保護属性を得る */
		*pgsizep = HAL_PAGE_SIZE_2M;               /* ページサイズを返却 */
		goto walk_end;
	}

	/* VPN0テーブルの物理アドレスを得る */
	vpn0_tbl_paddr = RV64_PTE_TO_PADDR(vpn1_pte); 
	/* VPN0テーブルのカーネル仮想アドレスを得る */
#if 1 /* TODO: */
	vpn0_tbl = (hal_pte *)vpn0_tbl_paddr; 
#else
	vpn0_tbl = (hal_pte *)HAL_PHY_TO_KERN_STRAIGHT(vpn0_tbl_paddr); 
#endif 
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

		rc = -ENOENT;
		goto error_out;
	}

	paddr = RV64_PTE_TO_PADDR(vpn0_pte);  /* ページのアドレスを得る */
	rv64_pgtbl_pte_to_vmprot(vpn0_pte, &prot); /* 保護属性を得る */
	*pgsizep = HAL_PAGE_SIZE;               /* ページサイズを返却 */

walk_end:
	*paddrp = paddr;
	*protp = prot;
	return 0;

error_out:
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

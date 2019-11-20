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

#define ENABLE_SHOW_PAGE_MAP

/**
   ページマップ情報を表示する
   @param[in]  pgt    ページテーブル
   @param[in]  vaddr  仮想アドレス
   @param[in]  size   領域サイズ
 */
static void 
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
		kassert( rc == 0 );
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
		return -EINVAL;  /* 2MiB境界でない仮想アドレスまたは物理アドレスを指定 */

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
	kassert( rc == 0 );

	if ( RV64_PTE_IS_VALID(cur_vpn2_pte) )
		vpn2_pte = cur_vpn2_pte;  /* vpn2_pteを設定 */
	else { /* 有効エントリでなければページを割当てる */

		if ( pgsize == HAL_PAGE_SIZE_1G ) {

			/*
			 * 1GiBページのマップ
			 */

			/* 属性値に物理ページ番号値を付加しPTEを生成 */
			vpn2_pte = ( RV64_PTE_V | new_pte | RV64_PTE_PADDR_TO_PPN(paddr) );
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

	/* VPN1テーブル物理アドレス  */
	vpn1_tbl_paddr = RV64_PTE_TO_PADDR(vpn2_pte);
	 /* VPN1テーブル仮想アドレス  */
	rc = pfdb_paddr_to_kvaddr((void *)vpn1_tbl_paddr, (void **)&vpn1_tbl);
	kassert( rc == 0 );


	/*
	 * VPN1テーブル
	 */
	vpn1_idx = rv64_pgtbl_vpn1_index(vaddr); /* VPN1テーブルインデクスを得る */
	cur_vpn1_pte = vpn1_tbl[vpn1_idx];  /* ページテーブルエントリを得る */
	/* VPN1テーブルのページフレーム情報を得る */
	rc = pfdb_kvaddr_to_page_frame(vpn1_tbl, &cur_tbl_pf); 
	kassert( rc == 0 );

	if ( RV64_PTE_IS_VALID(cur_vpn1_pte) )
		vpn1_pte = cur_vpn1_pte;  /* vpn1_pteを設定 */
	else { /* 有効エントリでなければページを割当てる */

		if ( pgsize == HAL_PAGE_SIZE_2M ) {

			/*
			 * 2MiBページのマップ
			 */

			/* 属性値に物理ページ番号値を付加しPTEを生成 */
			vpn1_pte = ( RV64_PTE_V | new_pte | RV64_PTE_PADDR_TO_PPN(paddr) );
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

	/* VPN0テーブル物理アドレス  */
	vpn0_tbl_paddr = RV64_PTE_TO_PADDR(vpn1_pte);
	 /* VPN0テーブル仮想アドレス  */
	rc = pfdb_paddr_to_kvaddr((void *)vpn0_tbl_paddr, (void **)&vpn0_tbl);
	kassert( rc == 0 );

	/* VPN0テーブル
	 */
	vpn0_idx = rv64_pgtbl_vpn0_index(vaddr); /* VPN0テーブルインデクスを得る */
	vpn0_pte = vpn0_tbl[vpn0_idx];  /* ページテーブルエントリを得る */
	/* VPN0テーブルのページフレーム情報を得る */
	rc = pfdb_kvaddr_to_page_frame((void *)vpn0_tbl, &cur_tbl_pf); 
	kassert( rc == 0 );

	/*
	 * 属性値に物理ページ番号値を付加しノーマルページへのPTEを生成
	 */
	if ( RV64_PTE_IS_VALID(vpn0_pte) ) {

		rc = -EBUSY;
		goto error_out;
	}

	vpn0_pte = 
		( RV64_PTE_V | new_pte | RV64_PTE_PADDR_TO_PPN(paddr) );
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

		paddr = RV64_PTE_TO_PADDR(vpn2_pte); /* 1GiBページのアドレスを得る */
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

		paddr = RV64_PTE_TO_PADDR(vpn1_pte); /* 2MiBページのアドレスを得る */
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
	
		kprintf("Invalid page alignment(vstart, vend, pstart, pend)\n", 
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
   カーネル空間をマップする
   @param[out] pgtp ページテーブル
 */
void
hal_map_kernel_space(vm_pgtbl *pgtp){
	int               rc;
	vm_pgtbl       pgtbl;
	vm_paddr   tbl_paddr;
	vm_vaddr   vaddr_sta;
	vm_paddr       paddr;

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
	pgtbl->satp = RV64_SATP_VAL(RV64_SATP_MODE_SV39, ULONGLONG_C(0), 
	    tbl_paddr >> PAGE_SHIFT);

	kprintf("Kernel base: %p I/O base:%p kpgtbl-vaddr: %p kpgtbl-paddr: %p\n", 
	    HAL_KERN_VMA_BASE, HAL_KERN_IO_BASE, pgtbl->pgtbl_base, pgtbl->satp);

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

	/* MMIOをマップ
	 */
	rc = hal_pgtbl_enter(pgtbl, RV64_UART0, RV64_UART0_PADDR,
	    VM_PROT_READ|VM_PROT_WRITE, VM_FLAGS_UNMANAGED|VM_FLAGS_SUPERVISOR, 
	    RV64_UART0_SIZE);
	show_page_map(pgtbl, RV64_UART0, RV64_UART0_SIZE);

	rc = hal_pgtbl_enter(pgtbl, RV64_UART1, RV64_UART1_PADDR,
	    VM_PROT_READ|VM_PROT_WRITE, VM_FLAGS_UNMANAGED|VM_FLAGS_SUPERVISOR, 
	    RV64_UART1_SIZE);
	show_page_map(pgtbl, RV64_UART1, RV64_UART1_SIZE);

	rc = hal_pgtbl_enter(pgtbl, RV64_PLIC, RV64_PLIC_PADDR,
	    VM_PROT_READ|VM_PROT_WRITE, VM_FLAGS_UNMANAGED|VM_FLAGS_SUPERVISOR, 
	    RV64_PLIC_SIZE);
	show_page_map(pgtbl, RV64_PLIC, RV64_PLIC_SIZE);

	rc = hal_pgtbl_enter(pgtbl, RV64_CLINT, RV64_CLINT_PADDR,
	    VM_PROT_READ|VM_PROT_WRITE, VM_FLAGS_UNMANAGED|VM_FLAGS_SUPERVISOR, 
	    RV64_CLINT_SIZE);
	show_page_map(pgtbl, RV64_CLINT, RV64_CLINT_SIZE);

	rc = hal_pgtbl_enter(pgtbl, RV64_QEMU_VIRTIO0, RV64_QEMU_VIRTIO0_PADDR,
	    VM_PROT_READ|VM_PROT_WRITE, VM_FLAGS_UNMANAGED|VM_FLAGS_SUPERVISOR, 
	    RV64_QEMU_VIRTIO0_SIZE);
	show_page_map(pgtbl, RV64_QEMU_VIRTIO0, RV64_QEMU_VIRTIO0_SIZE);

	rv64_write_satp(pgtbl->satp);  /* ページテーブル読み込み */

	*pgtp = pgtbl; /* カーネルページテーブルを返却  */

	return;
}

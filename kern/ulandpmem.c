/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Userland memory system for test                                   */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>

#include <kern/page-if.h>

#include <sys/mman.h>

#define MEM_AREA_SIZE    (MIB_TO_BYTE(8))
#define AREA_NUM         (MIB_TO_BYTE(KC_PHYSMEM_MB)/MEM_AREA_SIZE)
#define NORMAL_AREA_NUM  (AREA_NUM - 1)
#define KHEAP_IDX        (AREA_NUM - 1)

static struct _uland_minfo{
	void        *base;
	void       *kheap;
	size_t kheap_size;
}uland_minfo = {.base = (void *)(~(ULONG_C(0))), .kheap = NULL, .kheap_size = 0};

static struct _phys_memmap{
	void            *pmem;
	uintptr_t       start;
	size_t           size;
}area[AREA_NUM];

/**
   指定されたアラインメントで仮想メモリを割り当てる
   @param[in] length 割り当てサイズ (単位: バイト)
   @param[in] prot   mmap(2)の保護属性
   @param[in] flags  mmap(2)のフラグ
   @param[in] align  領域の開始アラインメント
   @param[in] addrp 割り当てたメモリ領域のアドレス返却先
   @retval   0 正常終了
   @retval 非0 対応するページフレームが見つからなかった
 */
static int
aligned_mmap(size_t length, int prot, int flags, size_t align, void **addrp) {
	int                   rc;
	size_t        alloc_size;
	uintptr_t     area_start;
	void                  *m;

	/*  要求サイズがページサイズ境界にそろっていることを確認する  */
	kassert( !addr_not_aligned(length, PAGE_SIZE) );

	/* 指定境界に合わせるために要求サイズに境界長を加算した分の領域を獲得する */
	alloc_size = length + align;

	m = mmap(NULL, alloc_size, prot, flags, -1, 0); /* メモリ領域を獲得する  */
	if ( m == NULL )
		return -ENOMEM;  /*  メモリ不足  */

	/* 開始アドレスを指定された境界に合わせて
	 * 先頭の未使用領域をアンマップする。
	 */
	area_start = roundup_align(m, align);  /*  領域内の最初の指定境界アドレスを算出  */

	/* 指定アラインメントに開始アドレスがそろっていない場合
	 * 領域の開始から指定境界までの領域をアンマップする
	 */
	if ( addr_not_aligned(m, align) ) {

		kassert( area_start > ( (uintptr_t)m ) );
		/* 領域の先頭から指定境界の直前までの領域をアンマップする  */
		rc = munmap(m, area_start - ( (uintptr_t)m) - 1 );
		kassert( rc == 0 );

		/* 獲得サイズからアンマップした領域長を減算する */
		alloc_size -= area_start - ( (uintptr_t)m );
	}

	/* 末尾を獲得サイズに合わせてアンマップする
	 */
	if ( alloc_size > length ) {

		/* 要求サイズ以降のメモリ領域をアンマップする */
		rc = munmap((void *)(area_start + length),
		    alloc_size - length - 1 );
		kassert( rc == 0 );

		/* 獲得サイズからアンマップした領域長を減算する */
		alloc_size -= alloc_size - length;
	}
	kassert( alloc_size == length );
	memset( (void *)area_start, 0, length);
	*addrp = (void *)area_start;  /*  獲得した領域を返却する  */

	return 0;  /* 正常終了  */
}

/**
   カーネルの仮想アドレスからページフレーム番号を算出する
   @param[in] kvaddr カーネル仮想アドレス
   @param[out] pfnp  ページフレーム番号返却領域
   @retval   0 正常終了
   @retval 非0 対応するページフレームが見つからなかった
   @note   カーネル仮想アドレスやページフレーム番号が搭載物理メモリの
           範囲内にあることを保証しないことに留意すること
 */
int
tflib_kvaddr_to_pfn(void *kvaddr, obj_cnt_type *pfnp){
	void *phys_addr;

	phys_addr = (void *)((uintptr_t)kvaddr - (uintptr_t)uland_minfo.base);
	hal_paddr_to_pfn(phys_addr, pfnp);

	return 0;
}

/**
   ページフレーム番号からカーネルの仮想アドレスを算出する
   @param[in]  pfn     ページフレーム番号
   @param[out] kvaddrp カーネル仮想アドレス返却領域
   @retval   0 正常終了
   @retval 非0 対応するページフレームが見つからなかった
   @note   カーネル仮想アドレスやページフレーム番号が搭載物理メモリの
           範囲内にあることを保証しないことに留意すること
 */
int
tflib_pfn_to_kvaddr(obj_cnt_type pfn, void **kvaddrp){
	void *phys_addr;

	hal_pfn_to_paddr(pfn, &phys_addr);

	*kvaddrp =(void *)(((uintptr_t)phys_addr) + (uintptr_t)uland_minfo.base);

	return 0;
}

/**
   カーネルストレートマップ領域中のアドレスを物理アドレスに変換する
   @param[in] kvaddr カーネルストレートマップ領域中のアドレス
   @return 物理アドレス
 */
vm_paddr
tflib_kern_straight_to_phy(void *kvaddr){
	int           rc;
	obj_cnt_type pfn;

	rc = tflib_kvaddr_to_pfn(kvaddr, &pfn);
	kassert( rc == 0 );

	return (vm_paddr)(pfn << PAGE_SHIFT);
}
/**
   物理アドレスをカーネルストレートマップ領域中のアドレスに変換する
   @param[in] paddr 物理アドレス
   @return カーネルストレートマップ領域中のアドレス
 */
void *
tflib_phy_to_kern_straight(vm_paddr paddr){
	int           rc;
	void     *kvaddr;

	rc = tflib_pfn_to_kvaddr( paddr >> PAGE_SHIFT, &kvaddr);
	kassert( rc == 0 );

	return kvaddr;
}

/**
   カーネルメモリレイアウトを初期化する
   @retval   0 正常終了
 */
int
tflib_kernlayout_init(void){
	int                 rc;
	int                  i;
	size_t          length;
	pfdb_ent         *pfdb;
	void       *phys_start;
	void                *m;

	for(i = 0; AREA_NUM > i; ++i) {

		length = roundup_align( MEM_AREA_SIZE, PAGE_SIZE );
		/* ページプールの最大ページ長にアラインメントを合わせる
		 */
		rc = aligned_mmap(length, PROT_READ|PROT_WRITE,
		    MAP_PRIVATE|MAP_ANONYMOUS|MAP_POPULATE,
		    PAGE_SIZE << ( PAGE_POOL_MAX_ORDER - 1),
		    &m);
		kassert( rc == 0 );

		/* 疑似メモリ領域管理情報を更新する
		 */
		area[i].pmem = (void *)m;
		area[i].start = (uintptr_t)m;
		area[i].size = (size_t)length;
		if ( uland_minfo.base > area[i].pmem )
			uland_minfo.base = area[i].pmem;
	}

	for(i = 0; AREA_NUM > i; ++i) {

		hal_kvaddr_to_phys(area[i].pmem, &phys_start);
		pfdb_add((uintptr_t)phys_start, area[i].size, &pfdb);
		if ( i == KHEAP_IDX ) /* ヒープサイズをpfdbの利用可能ページ数から算出 */
			uland_minfo.kheap_size = pfdb->page_pool.available_pages * PAGE_SIZE;

	}

	/*  カーネルヒープの初期化
	 */
	uland_minfo.kheap = area[KHEAP_IDX].pmem; /* ヒープ領域のカーネル仮想アドレス設定 */
	kassert( uland_minfo.kheap != NULL );

	hal_kvaddr_to_phys(uland_minfo.kheap, &phys_start); /* 開始物理アドレス取得 */
	/* ヒープ領域の初期化 */
	ekheap_init((vm_paddr)phys_start, uland_minfo.kheap, uland_minfo.kheap_size);

	/* ヒープ領域を予約   */
	pfdb_mark_phys_range_reserved((vm_paddr)phys_start,
	    (vm_paddr)phys_start + uland_minfo.kheap_size - 1);

	return 0;
}

/**
   カーネルメモリレイアウトのファイナライズを行う
 */
void
tflib_kernlayout_finalize(void){
	int            i;
	int           rc;
	void *phys_start;

	/* ヒープ領域の予約を解除
	 */
	hal_kvaddr_to_phys(area[KHEAP_IDX].pmem, &phys_start);
	pfdb_unmark_phys_range_reserved((vm_paddr)phys_start,
	    (vm_paddr)phys_start + uland_minfo.kheap_size - 1);

	pfdb_free();  /*  ページフレームDBを破棄する  */
	for(i = 0; AREA_NUM > i; ++i) {

		if ( area[i].pmem == NULL )
			continue;

		rc = munmap(area[i].pmem, area[i].size);
		kassert( rc == 0 );

		area[i].pmem = NULL;
		area[i].start = 0;
		area[i].size = 0;
	}

	kassert( rc == 0 );
}

/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  early kernel heap routines                                        */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>
#include <klib/early-kheap.h>
#include <kern/page-macros.h>

static early_kernel_heap ekheap = {
	.pstart = 0,
	.start = NULL,
	.end = NULL,
	.cur = NULL
};

/**
   初期カーネルヒープを伸長する
   @param[in] inc  伸長するサイズ (単位: バイト, 負の場合は縮小する)
   @return 変更前のヒープの位置
 */
void *
ekheap_sbrk(vm_size inc){
	void *prev;

	prev = ekheap.cur;
	if ( ( ekheap.cur + inc < ekheap.start ) || ( ekheap.cur + inc >=  ekheap.end ) )
		return EARLY_KHEAP_SBRK_FAILED;

	ekheap.cur += inc;

	return prev;
}

/**
   初期カーネルヒープの状態を返却する
   @param[in] st  状態を返却する領域
   @note 初期カーネルヒープで使用している領域を予約するために使用する
 */
void
ekheap_stat(early_kernel_heap *st){
	
	memcpy(st, &ekheap, sizeof(early_kernel_heap));  /*  ヒープの状態を返却する  */
}

/**
   初期カーネルヒープ内の仮想アドレスから物理アドレスを算出する
   @param[in] kvaddr  カーネル仮想アドレス
   @param[out] paddrp 物理アドレス返却領域
 */
void
ekheap_kvaddr_to_paddr(void *kvaddr, void **paddrp){
	vm_size paddr_off;

	/*  ヒープ内にアドレスが存在することを確認する  */
	kassert( ( ekheap.start <= kvaddr ) && ( kvaddr <= ekheap.end ) );

	/*  ヒープ先頭からのオフセット位置を算出する  */
	paddr_off = (vm_size)((uintptr_t)kvaddr - (uintptr_t)ekheap.start);
	
	/*  物理アドレスを算出して返却する  */
	*paddrp = (void *)((uintptr_t)ekheap.pstart + (uintptr_t)paddr_off);
}

/**
   初期カーネルヒープ内の物理アドレスから仮想アドレスを算出する
   @param[in]  paddr   物理アドレス
   @param[out] kvaddrp カーネル仮想アドレス返却領域
 */
void
ekheap_paddr_to_kvaddr(void *paddr, void **kvaddrp){
	vm_size paddr_off;
	void      *kvaddr;

	/*  ヒープ先頭からのオフセット位置を算出する  */
	paddr_off = (vm_size)((uintptr_t)paddr - (uintptr_t)ekheap.pstart);

	/*  ヒープ内アドレスを算出する  */
	kvaddr = (void *)((uintptr_t)ekheap.start + (uintptr_t)paddr_off);

	/*  ヒープ内にアドレスが存在することを確認する  */
	kassert( ( ekheap.start <= kvaddr ) && ( kvaddr <= ekheap.end ) );

	*kvaddrp = kvaddr;  /*  変換結果を返却する  */
}

/**
   初期カーネルヒープを初期化する
   @param[in] pstart 開始物理アドレス
   @param[in] start  開始アドレス
   @param[in] size   サイズ (単位: バイト)
 */
void
ekheap_init(vm_paddr pstart, void *start, vm_size size){

	ekheap.pstart = pstart;
	ekheap.start = start;
	ekheap.end = (void *)PAGE_ROUNDUP(((uintptr_t)start) + size);
	ekheap.cur = start;
}

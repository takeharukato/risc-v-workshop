/* -*- mode: gas; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  RISC-V 64 page operation                                          */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>

#include <kern/page-if.h>

#include <hal/rv64-platform.h>

/**
   物理アドレスからページフレーム番号を算出する
   @param[in]  paddr 物理アドレス
   @param[out] pfnp  ページフレーム番号返却領域
   @retval   0 正常終了
   @retval 非0 対応するページフレームが見つからなかった
   @note   物理アドレスやページフレーム番号が搭載物理メモリの
           範囲内にあることを保証しないことに留意すること
 */
int
hal_paddr_to_pfn(void *paddr, obj_cnt_type *pfnp){

	*pfnp = (obj_cnt_type)( ( (uintptr_t)paddr ) >> PAGE_SHIFT );

	return 0;
}

/**
   ページフレーム番号から物理アドレスを算出する
   @param[in]  pfn     ページフレーム番号
   @param[out] paddrp 物理アドレス返却領域
   @retval   0 正常終了
   @retval 非0 対応するページフレームが見つからなかった
   @note   物理アドレスやページフレーム番号が搭載物理メモリの
           範囲内にあることを保証しないことに留意すること
 */
int
hal_pfn_to_paddr(obj_cnt_type pfn, void **paddrp){

	*paddrp = (void *)( pfn << PAGE_SHIFT );

	return 0;
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
hal_kvaddr_to_pfn(void *kvaddr, obj_cnt_type *pfnp){
	void *phys_addr;

	phys_addr = (void *)HAL_KERN_STRAIGHT_TO_PHY(kvaddr);
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
hal_pfn_to_kvaddr(obj_cnt_type pfn, void **kvaddrp){
	void *phys_addr;

	hal_pfn_to_paddr(pfn, &phys_addr);

	*kvaddrp = (void *)HAL_PHY_TO_KERN_STRAIGHT(phys_addr);

	return 0;
}
/**
   カーネルの仮想アドレスから物理アドレスを算出する
   @param[in] kvaddr カーネル仮想アドレス
   @param[out] phys_addrp 物理アドレス返却領域
   @retval 0 正常終了
   @note   カーネル仮想アドレスや物理アドレスが搭載物理メモリの
           範囲内にあることを保証しないことに留意すること
 */
int
hal_kvaddr_to_phys(void *kvaddr, void **phys_addrp){
	obj_cnt_type pfn;

	hal_kvaddr_to_pfn(kvaddr, &pfn);
	*phys_addrp = (void *)( pfn << PAGE_SHIFT );

	return 0;
}

/**
   物理アドレスからカーネルの仮想アドレスを算出する
   @param[in]  phys_addr 物理アドレス
   @param[out] kvaddrp   カーネル仮想アドレス返却領域
   @retval 0 正常終了
   @note   カーネル仮想アドレスや物理アドレスが搭載物理メモリの
           範囲内にあることを保証しないことに留意すること
 */
int
hal_phys_to_kvaddr(void *phys_addr, void **kvaddrp){
	obj_cnt_type pfn;
	void     *kvaddr;

	hal_paddr_to_pfn(phys_addr, &pfn);

	hal_pfn_to_kvaddr(pfn, &kvaddr);

	*kvaddrp = kvaddr;

	return 0;
}

/**
   カーネルメモリレイアウトを初期化する
   @retval   0 正常終了
 */
int
hal_kernlayout_init(void){

#if !defined(CONFIG_HAL)
	tflib_kernlayout_init();
#endif  /*  !CONFIG_HAL  */

	return 0;
}

/**
   カーネルメモリレイアウトのファイナライズを行う
 */
void
hal_kernlayout_finalize(void){

#if !defined(CONFIG_HAL)
	tflib_kernlayout_finalize();
#endif  /*  !CONFIG_HAL  */

	return ;
}

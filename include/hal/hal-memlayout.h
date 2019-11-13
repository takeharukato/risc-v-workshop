/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  x64 pc platform information                                       */
/*                                                                    */
/**********************************************************************/
#if !defined(_HAL_HAL_MEMLAYOUT_H)
#define  _HAL_HAL_MEMLAYOUT_H 
#include <klib/misc.h>
#include <klib/align.h>

#define HAL_KERN_VMA_BASE     (ULONGLONG_C(0xFFFF800000000000))  /**< カーネル仮想アドレス  */
#define HAL_KERN_PHY_BASE     (ULONGLONG_C(0x0000000000000000))  /**< 開始物理アドレス      */
#define X64_BOOT_MAPSIZE      ( GIB_TO_BYTE(4) )  /**< 4GiBをマップする  */
/**
  必要なページディレクトリ数
*/
#define X64_BOOT_PGTBL_PD_NR \
	( ( roundup_align(X64_BOOT_MAPSIZE, HAL_PAGE_SIZE_2M) / HAL_PAGE_SIZE_2M ) \
	/ X64_PGTBL_ENTRIES_NR )

#if !defined(ASM_FILE)
#include <klib/freestanding.h>
#include <kern/kern-types.h>

#if !defined(CONFIG_HAL)
#include <ktest/ktest.h>
#endif  /* !CONFIG_HAL */

#if defined(CONFIG_HAL)
/**
   カーネルストレートマップ領域中のアドレスを物理アドレスに変換する
   @param[in] _kvaddr カーネルストレートマップ領域中のアドレス
   @return 物理アドレス
 */
#define HAL_KERN_STRAIGHT_TO_PHY(_kvaddr) \
	( (vm_paddr)( ((vm_vaddr)(_kvaddr)) - HAL_KERN_VMA_BASE ) )

/**
   物理アドレスをカーネルストレートマップ領域中のアドレスに変換する
   @param[in] _paddr 物理アドレス
   @return カーネルストレートマップ領域中のアドレス
 */
#define HAL_PHY_TO_KERN_STRAIGHT(_paddr) \
	( (vm_vaddr)( ((vm_paddr)(_paddr)) + HAL_KERN_VMA_BASE ) )
#else
/**
   カーネルストレートマップ領域中のアドレスを物理アドレスに変換する
   @param[in] _kvaddr カーネルストレートマップ領域中のアドレス
   @return 物理アドレス
 */
#define HAL_KERN_STRAIGHT_TO_PHY(_kvaddr) \
	( tflib_kern_straight_to_phy((_kvaddr)) ) 

/**
   物理アドレスをカーネルストレートマップ領域中のアドレスに変換する
   @param[in] _paddr 物理アドレス
   @return カーネルストレートマップ領域中のアドレス
 */
#define HAL_PHY_TO_KERN_STRAIGHT(_paddr) \
	( tflib_phy_to_kern_straight((_paddr)) )

#endif  /* CONFIG_HAL */
#endif  /* !ASM_FILE */

#endif  /*  _HAL_HAL_MEMLAYOUT_H   */

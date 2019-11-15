/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  RISC-V 64 QEMU platform definitions                               */
/*                                                                    */
/**********************************************************************/
#if !defined(_HAL_RV64_PLATFORM_H)
#define  _HAL_RV64_PLATFORM_H 
#include <klib/misc.h>
#include <klib/align.h>

/** カーネル仮想アドレス   */
#define HAL_KERN_VMA_BASE     (CONFIG_HAL_KERN_VMA_BASE) 
/** 開始物理メモリアドレス */
#define HAL_KERN_PHY_BASE     (ULONGLONG_C(0x80000000))
/** ストレートマップ長 */
#define RV64_STRAIGHT_MAPSIZE ( GIB_TO_BYTE(4) )  /**< 4GiBをマップする  */

/** QEMU UART レジスタ   */
#define RV64_UART0     (ULONGLONG_C(0x10000000))

/** QEMU UART 割込み番号 */
#define RV64_UART0_IRQ (10)

/** QEMU Platform-Level Interrupt Controller (PLIC) レジスタ   */
#define RV64_PLIC     (ULONGLONG_C(0x0C000000))

/** QEMU UART Core Local Interruptor(CLINT)レジスタ            */
#define RV64_CLINT    (ULONGLONG_C(0x02000000))

/**
  必要なページディレクトリ数 (単位:個)
*/
#define RV64_BOOT_PGTBL_VPN1_NR \
	( ( roundup_align(RV64_STRAIGHT_MAPSIZE, HAL_PAGE_SIZE_2M) / HAL_PAGE_SIZE_2M ) \
	/ RV64_PGTBL_ENTRIES_NR )

#if !defined(ASM_FILE)
#include <klib/freestanding.h>
#include <kern/kern-types.h>

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
#endif  /* CONFIG_HAL */
#endif  /* !ASM_FILE */

#endif  /*  _HAL_RV64_PLATFORM_H   */

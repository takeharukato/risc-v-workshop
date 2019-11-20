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

#include <kern/kern-autoconf.h>

#include <klib/misc.h>
#include <klib/align.h>

/** カーネル仮想アドレス (物理メモリストレートマップ領域)   
 * 
 * RISC-V64 Sv39では, 38ビット目から63ビット目までの26ビットの値は
 * 等しくなければならない(そうでなければページフォルトを発生させる)
 * The RISC-V Instruction Set Manual Volume II: Privileged Architecture
 * 4.4.1 Addressing and Memory Protection参照
 *
 * 上記から,  アッパーハーフカーネルのカーネル空間として利用可能な仮想アドレスは, 
 * 0xFFFFFFC0_00000000 から 0xFFFFFFFF_FFFFFFFF までとなる。
 *
 * I/Oデバイスをマップするための仮想アドレス領域が不足するが, シングルアドレス空間構成を
 * 採用することによって得られる利点を優先し, 搭載メモリ量に制限がかからないように
 * 最下位アドレスにI/Oマップ領域を用意しそれ以降に物理メモリをマップする構成とした
 * 
 * I/O マップ領域        0xFFFFFFC0_00000000 から 0xFFFFFFCF_FFFFFFFF まで (127GiB)
 * ストレートマップ領域  0xFFFFFFE0_00000000 から 0xFFFFFFFF_FFFFFFFF まで (127GiB)
 */
#define HAL_KERN_VMA_BASE     (CONFIG_HAL_KERN_VMA_BASE) 
/** カーネルメモリマップI/Oレジスタベースアドレス */
#if defined(CONFIG_UPPERHALF_KERNEL)
#define HAL_KERN_IO_BASE      (0xFFFFFFC000000000)
#else
#define HAL_KERN_IO_BASE      (0x0000000000000000)
#endif  /*  CONFIG_UPPERHALF_KERNEL */
/** 開始物理メモリアドレス */
#define HAL_KERN_PHY_BASE     (ULONGLONG_C(0x80000000))
/** ストレートマップ長 */                  
#define RV64_STRAIGHT_MAPSIZE ( GIB_TO_BYTE(4) )  /**< 4GiBをマップする  */

/*
 * U54 RISC-V コアのキャッシュ情報
 */
#define RV64_L1_DCACHE_LINESIZE  (64)               /* 64バイト */
#define RV64_L1_DCACHE_WAY       (8)                /* 8way */
#define RV64_L1_DCACHE_SIZE      (KIB_TO_BYTE(32))  /* 32KiB */
/** カラーリング数: 64個 */
#define RV64_L1_DCACHE_COLOR_NUM \
	( RV64_L1_DCACHE_SIZE / (RV64_L1_DCACHE_WAY * RV64_L1_DCACHE_LINESIZE) )

/** QEMU UART0 レジスタ物理アドレス   */
#define RV64_UART0_PADDR         (ULONGLONG_C(0x10000000))
/** QEMU UART0 レジスタ仮想アドレス   */
#define RV64_UART0               (RV64_UART0_PADDR + HAL_KERN_IO_BASE)
/** QEMU UART1 レジスタ物理アドレス   */
#define RV64_UART1_PADDR         (ULONGLONG_C(0x10001000))
/** QEMU UART1 レジスタ仮想アドレス   */
#define RV64_UART1               (RV64_UART1_PADDR + HAL_KERN_IO_BASE)
/** QEMU UART レジスタ領域サイズ     */
#define RV64_UART_AREA_SIZE      (PAGE_SIZE*2)
/** QEMU UART 割込み番号 */
#define RV64_UART0_IRQ           (10)

/** QEMU Platform-Level Interrupt Controller (PLIC) レジスタ物理アドレス   */
#define RV64_PLIC_PADDR          (ULONGLONG_C(0x0C000000))
/** QEMU Platform-Level Interrupt Controller (PLIC) レジスタ仮想アドレス   */
#define RV64_PLIC                (RV64_PLIC_PADDR + HAL_KERN_IO_BASE)

/** QEMU UART Core Local Interruptor(CLINT)レジスタ物理アドレス            */
#define RV64_CLINT_PADDR         (ULONGLONG_C(0x02000000))
/** QEMU UART Core Local Interruptor(CLINT)レジスタ仮想アドレス            */
#define RV64_CLINT               (RV64_CLINT_PADDR + HAL_KERN_IO_BASE)

/** QEMU virtI/O MMIO Interface 物理アドレス */
#define RV64_QEMU_VIRTIO0_PADDR  (ULONGLONG_C(0x10001000))
/** QEMU virtI/O MMIO Interface 仮想アドレス */
#define RV64_QEMU_VIRTIO0        (RV64_QEMU_VIRTIO0_PADDR + HAL_KERN_IO_BASE)
/** QEMU virtI/O MMIO Interface 割込み番号   */
#define RV64_QEMU_VIRTIO0_IRQ    (1)

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

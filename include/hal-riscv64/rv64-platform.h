/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  RISC-V64 QEMU platform definitions                                */
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
 *
 * ユーザ  : 0x00000000_00000000 から 0x0000003F_FFFFFFFF まで
 * カーネル: 0xFFFFFFC0_00000000 から 0xFFFFFFFF_FFFFFFFF まで
 *
 * となる。
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

/*
 * QEMU virtターゲット定義
 * (include/hw/riscv/virt.h参照)
 */

/** QEMU UART0 レジスタ物理アドレス   */
#define RV64_UART0_PADDR         (ULONGLONG_C(0x10000000))
/** QEMU UART0 レジスタ仮想アドレス   */
#define RV64_UART0               (RV64_UART0_PADDR + HAL_KERN_IO_BASE)
/** QEMU UART0 レジスタ領域サイズ (単位:バイト)   */
#define RV64_UART0_SIZE          (PAGE_SIZE)
/** QEMU UART 割込み番号 */
#define RV64_UART0_IRQ           (10)

/** QEMU Platform-Level Interrupt Controller (PLIC) レジスタ物理アドレス   */
#define RV64_PLIC_PADDR          (ULONGLONG_C(0x0C000000))
/** QEMU Platform-Level Interrupt Controller (PLIC) レジスタ仮想アドレス   */
#define RV64_PLIC                (RV64_PLIC_PADDR + HAL_KERN_IO_BASE)
/** QEMU Platform-Level Interrupt Controller (PLIC) レジスタ長 (単位: バイト) */
#define RV64_PLIC_SIZE           (ULONGLONG_C(0x4000000))
/* QEMU Platform-Level Interrupt Controller (PLIC) 配下の割込み数 */
#define RV64_PLIC_NR_IRQS   (53)                

/** QEMU Core Local Interruptor (CLINT)レジスタ物理アドレス            */
#define RV64_CLINT_PADDR            (ULONGLONG_C(0x02000000))
/** QEMU Core Local Interruptor (CLINT)レジスタ仮想アドレス            */
#define RV64_CLINT                  (RV64_CLINT_PADDR + HAL_KERN_IO_BASE)
/** QEMU Core Local Interruptor (CLINT)レジスタサイズ  (単位:バイト)   */
#define RV64_CLINT_SIZE             (ULONGLONG_C(0x10000))
/** QEMU Core Local Interruptor (CLINT) MSIP レジスタ オフセットアドレス    */
#define RV64_CLINT_MSIP_OFFSET      (ULONGLONG_C(0x0))
/** QEMU Core Local Interruptor (CLINT) MTIMECMP レジスタ サイズ (単位:バイト) */
#define RV64_CLINT_MSIP_SIZE        (4)

/** QEMU Core Local Interruptor (CLINT) MSIP レジスタ 物理アドレス   
    @param[in] hart コアのhartid
 */
#define RV64_CLINT_MSIP_PADDR(_hart)		\
	( RV64_CLINT_PADDR + RV64_CLINT_MSIP_OFFSET	\
	    + ( (_hart) * RV64_CLINT_MSIP_SIZE) )

/** QEMU Core Local Interruptor (CLINT) MSIP レジスタ 仮想アドレス   
    @param[in] hart コアのhartid
 */
#define RV64_CLINT_MSIP(_hart)			\
	( RV64_CLINT + RV64_CLINT_MSIP_OFFSET	\
	    + ( (_hart) * RV64_CLINT_MSIP_SIZE) )

/** QEMU Core Local Interruptor (CLINT) MTIMECMP レジスタ オフセットアドレス    */
#define RV64_CLINT_MTIMECMP_OFFSET  (ULONGLONG_C(0x4000))

/** QEMU Core Local Interruptor (CLINT) MTIMECMP レジスタ サイズ (単位:バイト) */
#define RV64_CLINT_MTIMECMP_SIZE    (8)

/** QEMU Core Local Interruptor (CLINT) MTIMECMP レジスタ 物理アドレス   
    @param[in] hart コアのhartid
 */
#define RV64_CLINT_MTIMECMP_PADDR(_hart)		\
	( RV64_CLINT_PADDR + RV64_CLINT_MTIMECMP_OFFSET	\
	    + ( (_hart) * RV64_CLINT_MTIMECMP_SIZE) )

/** QEMU Core Local Interruptor (CLINT) MTIMECMP レジスタ 仮想アドレス   
    @param[in] hart コアのhartid
 */
#define RV64_CLINT_MTIMECMP(_hart)			\
	( RV64_CLINT + RV64_CLINT_MTIMECMP_OFFSET	\
	    + ( (_hart) * RV64_CLINT_MTIMECMP_SIZE) )

/** QEMU Core Local Interruptor (CLINT) MTIMECMP レジスタ のインターバル 
    (デフォルト:100ms周期)
 */
#if defined(CONFIG_TIMER_INTERVAL_MS_1MS)
#define RV64_CLINT_MTIME_INTERVAL    (ULONGLONG_C(10000))
#elif defined(CONFIG_TIMER_INTERVAL_MS_10MS)
#define RV64_CLINT_MTIME_INTERVAL    (ULONGLONG_C(100000))
#else
#define RV64_CLINT_MTIME_INTERVAL    (ULONGLONG_C(1000000))              
#endif 

/** QEMU Core Local Interruptor (CLINT) MTIME オフセットアドレス    */
#define RV64_CLINT_MTIME_OFFSET      (ULONGLONG_C(0xBFF8))

/** QEMU Core Local Interruptor (CLINT) MTIME レジスタのカウンタ更新周期
    (100us毎にカウントアップする)
 */ 
#define RV64_CLINT_MTIME_FREQ        (ULONGLONG_C(1000000000))

/** QEMU Core Local Interruptor (CLINT) MTIME レジスタのカウンタ1us単位での更新値
 */
#define RV64_CLINT_MTIME_PER_US      (ULONGLONG_C(1000))

/** QEMU virtI/O MMIO Interface 物理アドレス */
#define RV64_VIRTIO0_PADDR           (ULONGLONG_C(0x10001000))
/** QEMU virtI/O MMIO Interface 仮想アドレス */
#define RV64_VIRTIO0                 (RV64_VIRTIO0_PADDR + HAL_KERN_IO_BASE)
/** QEMU virtI/O MMIO Interface レジスタサイズ */
#define RV64_VIRTIO0_SIZE            (PAGE_SIZE)
/** QEMU virtI/O MMIO Interface 割込み番号   */
#define RV64_VIRTIO0_IRQ             (1)

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

void rv64_plic_init(void);
#endif  /* !ASM_FILE */

#endif  /*  _HAL_RV64_PLATFORM_H   */

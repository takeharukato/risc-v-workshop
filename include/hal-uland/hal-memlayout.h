/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  x64 memory layout                                                 */
/*                                                                    */
/**********************************************************************/
#if !defined(_HAL_MEMLAYOUT_H)
#define  _HAL_MEMLAYOUT_H 

#include <kern/kern-consts.h>
#include <klib/misc.h>

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
#define HAL_KERN_IO_BASE      (ULONGLONG_C(0xFFFFFFC000000000))
#else
#define HAL_KERN_IO_BASE      (ULONGLONG_C(0x0000000000000000))
#endif  /*  CONFIG_UPPERHALF_KERNEL */
/** 開始物理メモリアドレス */
#define HAL_KERN_PHY_BASE     (ULONGLONG_C(0x80000000))
/** ストレートマップ長 */                  
#define RV64_STRAIGHT_MAPSIZE ( GIB_TO_BYTE(4) )  /**< 4GiBをマップする  */
/** ユーザ空間終了アドレス */
#define HAL_USER_END_ADDR     (ULONGLONG_C(0x0000003FFFFFFFFF))

#endif /* _HAL_MEMLAYOUT_H */

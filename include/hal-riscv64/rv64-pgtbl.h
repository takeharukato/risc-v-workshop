/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  RISC-V 64 page table definitions                                  */
/*                                                                    */
/**********************************************************************/
#if !defined(_HAL_RV64_PGTBL_H)
#define  _HAL_RV64_PGTBL_H

#include <klib/misc.h>
#include <klib/align.h>
#include <klib/regbits.h>
#include <hal/hal-page.h>  /*  HAL_PAGE_SIZE  */
/**
   ページテーブルインデクスシフト
   @note FU540-C000 U54コアはSv39 (ページベース 39ビット仮想アドレッシング)をサポートしている
   (物理アドレッシングは38ビット)
 */
#define RV64_PGTBL_LINER_ADDR_SIZE         (39) /**< Sv39のリニアアドレス長(39ビット) */
#define RV64_PGTBL_VPN2_INDEX_SHIFT        (30) /**< VPN2のインデクスシフト値 */
#define RV64_PGTBL_VPN1_INDEX_SHIFT        (21) /**< VPN1のインデクスシフト値 */
#define RV64_PGTBL_VPN0_INDEX_SHIFT        (12) /**< VPN0のインデクスシフト値 */

/**
   ページテーブルエントリサイズ (アセンブラから使用するため即値を定義)
 */
#define RV64_PGTBL_ENTRY_SIZE              (8)
/**
   ページテーブルのエントリ数 (単位: 個)
 */
#define RV64_PGTBL_ENTRIES_NR              (HAL_PAGE_SIZE/RV64_PGTBL_ENTRY_SIZE)

/**
  ブートページテーブルに必要なページディレクトリ数 (単位:個)
*/
#define RV64_BOOT_PGTBL_VPN1_NR \
	( ( roundup_align(RV64_STRAIGHT_MAPSIZE, HAL_PAGE_SIZE_2M) / HAL_PAGE_SIZE_2M ) \
	/ RV64_PGTBL_ENTRIES_NR )

/*
 * RISC-V64 アドレス変換スキーム
 */
/** Bare アドレス変換なし */
#define RV64_SATP_MODE_BARE_VAL            ULONGLONG_C(0)  
/** Sv39 ページベース39ビット仮想アドレス */
#define RV64_SATP_MODE_SV39_VAL            ULONGLONG_C(8)  
/** Sv48 ページベース48ビット仮想アドレス */
#define RV64_SATP_MODE_SV48_VAL            ULONGLONG_C(9)  
/** Sv57 ページベース57ビット仮想アドレス */
#define RV64_SATP_MODE_SV57_VAL            ULONGLONG_C(10) 
/** Sv64 ページベース64ビット仮想アドレス */
#define RV64_SATP_MODE_SV64_VAL            ULONGLONG_C(11) 

#define RV64_SATP_MODE_SHIFT               (60) /* SATPレジスタのmode値のシフト数 */
#define RV64_SATP_ASID_SHIFT               (44) /* SATPレジスタのasid値のシフト数 */
#define RV64_SATP_PPN_SHIFT                (0)  /* SATPレジスタのppn値のシフト数 */
#define RV64_SATP_MODE_BARE                \
	( RV64_SATP_MODE_BARE_VAL << RV64_SATP_MODE_SHIFT )
#define RV64_SATP_MODE_SV39                \
	( RV64_SATP_MODE_SV39_VAL << RV64_SATP_MODE_SHIFT )
#define RV64_SATP_MODE_SV48                \
	( RV64_SATP_MODE_SV48_VAL << RV64_SATP_MODE_SHIFT )
#define RV64_SATP_MODE_SV57                \
	( RV64_SATP_MODE_SV57_VAL << RV64_SATP_MODE_SHIFT )
#define RV64_SATP_MODE_SV64                \
	( RV64_SATP_MODE_SV64_VAL << RV64_SATP_MODE_SHIFT )

/**
   物理アドレス長の最大ビット数 (38bit = 128 GiB)
   @note U54コアの物理アドレス長
 */
#define RV64_PGTBL_MAXPHYADDR            (38)
/** 物理ページ番号のマスク  */
#define RV64_PPN_MASK                    regops_calc_mask_for_val(12, 55)
/**
   ページテーブルインデクスマスク生成マクロ
   @param[in] _ent1 インデクスの最上位ビット+1 (上位インデクスの最下位ビット)
   @param[in] _ent2 インデクスの最下位ビット
   @return ページテーブルインデクスマスク
 */
#define _rv64_pgtbl_calc_index_mask(_ent1, _ent2) \
	( calc_align_mask( ( ULONGLONG_C(1) << ( (_ent1) - (_ent2) ) ) ) )

/**
   ページテーブルインデクスマスク
 */
/** VPN2インデクスのマスク値
 */
#define RV64_PGTBL_VPN2_INDEX_MASK					\
	_rv64_pgtbl_calc_index_mask(RV64_PGTBL_LINER_ADDR_SIZE, RV64_PGTBL_VPN2_INDEX_SHIFT)

/** VPN1インデクスのマスク値
 */
#define RV64_PGTBL_VPN1_INDEX_MASK					\
	_rv64_pgtbl_calc_index_mask(RV64_PGTBL_VPN2_INDEX_SHIFT, RV64_PGTBL_VPN1_INDEX_SHIFT)

/** VPN0インデクスのマスク値
 */
#define RV64_PGTBL_VPN0_INDEX_MASK					\
	_rv64_pgtbl_calc_index_mask(RV64_PGTBL_VPN1_INDEX_SHIFT, RV64_PGTBL_VPN0_INDEX_SHIFT)

/**
   ページテーブルインデクス算出マクロ
   @param[in] _vaddr 仮想アドレス 
   @param[in] _mask  インデクスのマスク
   @param[in] _shift インデクスのシフト
   @return ページテーブルインデクス
*/
#define _rv64_pgtbl_calc_index(_vaddr, _mask, _shift)		\
	( ( (_vaddr) >> (_shift) ) & (_mask) )

/**
   VPN2インデクス算出マクロ
   @param[in] _vaddr 仮想アドレス
   @return テーブル内でのインデクス値
*/
#define rv64_pgtbl_vpn2_index(_vaddr)					\
	(_rv64_pgtbl_calc_index((_vaddr), RV64_PGTBL_VPN2_INDEX_MASK,	\
	    RV64_PGTBL_VPN2_INDEX_SHIFT))

/**
   VPN1インデクス算出マクロ
   @param[in] _vaddr 仮想アドレス
   @return テーブル内でのインデクス値
*/
#define rv64_pgtbl_vpn1_index(_vaddr)					\
	(_rv64_pgtbl_calc_index((_vaddr), RV64_PGTBL_VPN1_INDEX_MASK,	\
	    RV64_PGTBL_VPN1_INDEX_SHIFT))

/**
   VPN0インデクス算出マクロ
   @param[in] _vaddr 仮想アドレス
   @return テーブル内でのインデクス値
*/
#define rv64_pgtbl_vpn0_index(_vaddr)					\
	(_rv64_pgtbl_calc_index((_vaddr), RV64_PGTBL_VPN0_INDEX_MASK,	\
	    RV64_PGTBL_VPN0_INDEX_SHIFT))
/*
 * Page table entryの属性ビット
 */
#define RV64_PTE_V_BIT      (0)  /* Valid   */
#define RV64_PTE_R_BIT      (1)  /* Read    */
#define RV64_PTE_W_BIT      (2)  /* Write   */
#define RV64_PTE_X_BIT      (3)  /* Execute */
#define RV64_PTE_U_BIT      (4)  /* User    */
#define RV64_PTE_G_BIT      (5)  /* Global  */
#define RV64_PTE_A_BIT      (6)  /* Access  */
#define RV64_PTE_D_BIT      (7)  /* Dirty   */

#define RV64_PTE_V          (regops_set_bit(RV64_PTE_V_BIT))
#define RV64_PTE_R          (regops_set_bit(RV64_PTE_R_BIT))
#define RV64_PTE_W          (regops_set_bit(RV64_PTE_W_BIT))
#define RV64_PTE_X          (regops_set_bit(RV64_PTE_X_BIT))
#define RV64_PTE_U          (regops_set_bit(RV64_PTE_U_BIT))
#define RV64_PTE_G          (regops_set_bit(RV64_PTE_G_BIT))
#define RV64_PTE_A          (regops_set_bit(RV64_PTE_A_BIT))
#define RV64_PTE_D          (regops_set_bit(RV64_PTE_D_BIT))

/** PTE属性値のマスク
 */
#define PV64_PTE_FLAGS_MASK ( RV64_PTE_V | RV64_PTE_R | RV64_PTE_W | RV64_PTE_X | \
			     RV64_PTE_U | RV64_PTE_G | RV64_PTE_A | RV64_PTE_D )

/** ページテーブル用フラグ */
#define RV64_PTE_PGTBL_FLAGS (RV64_PTE_A|RV64_PTE_D)

/*
 * ノーマルページ関連定義
 */
#define RV64_PTE_PA_SHIFT   (10) /* PTE中のページ番号位置 */

/**
   物理アドレスをPTE中のページ番号に変換する
   @param[in] _pa 物理アドレス
   @return ページ番号
 */
#define RV64_PTE_PADDR_TO_PPN(_pa) \
	( ( (_pa) >> HAL_PAGE_SHIFT ) << RV64_PTE_PA_SHIFT)

/**
   PTE中のページ番号を物理アドレスに変換する
   @param[in] _pte ページテーブルエントリ値
   @return 物理アドレス
*/
#define RV64_PTE_TO_PADDR(_pte) \
	( ( (_pte) >> RV64_PTE_PA_SHIFT ) << HAL_PAGE_SHIFT )

/*
 * 2MiBページ関連定義
 */
#define RV64_PTE_2MPA_SHIFT       (19)  /* 2MiBページのPTEページ番号 */

/**
   2MiBページ物理アドレスをPTE中のページ番号に変換する
   @param[in] _pa 物理アドレス
   @return ページ番号
 */
#define RV64_PTE_2MPADDR_TO_PPN(_pa) \
	( ( (_pa) >> HAL_PAGE_SHIFT_2M ) << RV64_PTE_2MPA_SHIFT )

/**
   PTE中の2MiBページのページ番号を物理アドレスに変換する
   @param[in] _pte ページテーブルエントリ値
   @return 物理アドレス
*/
#define RV64_PTE_2MPA_TO_PADDR(_pte) \
	(  ( (_pte) >> RV64_PTE_2MPA_SHIFT ) << HAL_PAGE_SHIFT_2M )

/*
 * 1GiBページ関連定義
 */
#define RV64_PTE_1GPA_SHIFT       (28)  /* 1GiBページのPTEページ番号 */
/**
   1GiBページ物理アドレスをPTE中のページ番号に変換する
   @param[in] _pa 物理アドレス
   @return ページ番号
 */
#define RV64_PTE_1GPADDR_TO_PPN(_pa) \
	( ( (_pa) >> HAL_PAGE_SHIFT_1G ) << RV64_PTE_1GPA_SHIFT )

/**
   PTE中の1GiBページのページ番号を物理アドレスに変換する
   @param[in] _pte ページテーブルエントリ値
   @return 物理アドレス
*/
#define RV64_PTE_1GPA_TO_PADDR(_pte) \
	( ( (_pte) >> RV64_PTE_1GPA_SHIFT ) << HAL_PAGE_SHIFT_1G )

/**
   PTE中の属性値を取得する
   @param[in] _pte ページテーブルエントリ値
   @return ページテーブル属性値
*/
#define RV64_PTE_FLAGS(_pte) ( (_pte) & PV64_PTE_FLAGS_MASK )

/**
   ページテーブルエントリが有効であることを確認する
   @param[in] _pte ページテーブルエントリ値
   @retval 真 有効エントリである
   @retval 偽 有効エントリでない
*/
#define RV64_PTE_IS_VALID(_pte)			\
	( RV64_PTE_FLAGS((_pte)) & RV64_PTE_V )

/**
   ページを参照しているエントリであることを確認する
   @param[in] _pte ページテーブルエントリ値
   @retval 真 ページを参照しているエントリである
   @retval 偽 ページを参照しているエントリでない
   @note   Table 4.4: Encoding of PTE R/W/X fields.参照
*/
#define RV64_PTE_IS_LEAF(_pte) \
	( ( RV64_PTE_FLAGS((_pte)) & (RV64_PTE_R|RV64_PTE_W|RV64_PTE_X) ) != 0 )

/**
   RISC-V 64のSATPレジスタ値を算出する
   @param[in] _mode アドレス変換スキーム
   @param[in] _asid アドレス空間ID
   @param[in] _ppn  ページテーブルベースの物理アドレス
 */
#define RV64_SATP_VAL(_mode, _asid, _ppn)  \
	( (_mode) | ( (_asid) << RV64_SATP_ASID_SHIFT ) | ( (_ppn) << RV64_SATP_PPN_SHIFT ) )

#if !defined(ASM_FILE)

#include <klib/freestanding.h>
typedef uint64_t hal_pte;  /* ページテーブルエントリ型 */

#endif  /* !ASM_FILE */
#endif  /*  _HAL_RV64_PGTBL_H  */

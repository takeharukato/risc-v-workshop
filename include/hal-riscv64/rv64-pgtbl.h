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
   RISC-V64 仮想アドレス最大ビット数
*/
#define RV64_PGTBL_SV39_ADDR_BITS          (39) /**< Sv39のリニアアドレス長(39ビット)      */
#define RV64_PGTBL_SV48_ADDR_BITS          (48) /**< Sv48のリニアアドレス長(48ビット)      */
#define RV64_PGTBL_SV57_ADDR_BITS          (57) /**< Sv57のリニアアドレス長(57ビット)      */
#define RV64_PGTBL_SV64_ADDR_BITS          (64) /**< Sv64のリニアアドレス長(64ビット)      */

#define RV64_PGTBL_LINER_ADDR_BITS         (RV64_PGTBL_SV39_ADDR_BITS) /**< Sv39を使用する */

/**
   物理アドレス長の最大ビット数 (38bit = 128 GiB)
   @note U54コアの物理アドレス長
 */
#define RV64_PGTBL_MAXPHYADDR            (38)

/*
 * ページテーブル構成
 */
#define RV64_PGTBL_SV39_LVL_NR           (3)  /**< Sv39のページテーブル段数(単位:段)     */
#define RV64_PGTBL_SV39_INDEX_BITS       (9)  /**< Sv39のインデクスビット数(単位:ビット) */

#define RV64_PGTBL_SV48_LVL_NR           (4)  /**< Sv48のページテーブル段数(単位:段)     */
#define RV64_PGTBL_SV48_INDEX_BITS       (9)  /**< Sv48のインデクスビット数(単位:ビット) */

#define RV64_PGTBL_LVL_NR                (RV64_PGTBL_SV39_LVL_NR)
#define RV64_PGTBL_INDEX_BITS            (RV64_PGTBL_SV39_INDEX_BITS) 

/**
   ページテーブルインデクスマスク生成マクロ
   @param[in] _ent1 インデクスの最上位ビット+1 (上位インデクスの最下位ビット)
   @param[in] _ent2 インデクスの最下位ビット
   @return ページテーブルインデクスマスク
 */
#define _rv64_pgtbl_calc_index_mask(_ent1, _ent2) \
	( calc_align_mask( ( ULONGLONG_C(1) << ( (_ent1) - (_ent2) ) ) ) )

/**
   ページテーブルインデクスシフトを算出する
   @param[in] _lvl ページテーブルの段数
   (RV64_PGTBL_LVL_NRを指定するとリニアアドレスビット数を返す)
   @return ページテーブルインデクスのシフト値 (単位:ビット)
 */
#define rv64_pgtbl_vpn_lvl_to_indexshift(_lvl)					\
	( RV64_PGTBL_LINER_ADDR_BITS					\
	    - ( ( RV64_PGTBL_LVL_NR - (_lvl) ) * RV64_PGTBL_INDEX_BITS ) )

/**
   ページテーブルインデクスマスクを算出する
   @param[in] _lvl ページテーブルの段数
   @return ページテーブルインデクスのマスク (アドレスマスク)
 */
#define rv64_pgtbl_vpn_lvl_to_indexmask(_lvl)				\
	( _rv64_pgtbl_calc_index_mask(rv64_pgtbl_vpn_lvl_to_indexshift((_lvl) + 1), \
	    rv64_pgtbl_vpn_lvl_to_indexshift((_lvl)) ) )

/**
   ページテーブルインデクス算出マクロ
   @param[in] _vaddr 仮想アドレス 
   @param[in] _lvl   テーブルの段数
   @return ページテーブルインデクス
*/
#define rv64_pgtbl_calc_index_val(_vaddr, _lvl)		\
	( ( ( _vaddr ) >> ( rv64_pgtbl_vpn_lvl_to_indexshift( (_lvl) ) ) ) \
	    & rv64_pgtbl_vpn_lvl_to_indexmask( (_lvl) ) )

/**
   マップページサイズ算出マクロ
   @param[in] _lvl   テーブルの段数
   @return ページサイズ
*/
#define rv64_pgtbl_calc_pagesize(_lvl)		\
	( ULONGLONG_C(1) << ( rv64_pgtbl_vpn_lvl_to_indexshift( (_lvl) ) ) )
/*
 * Supervisor Address Translation and Protection (SATP) レジスタ
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

/**
   ページテーブルエントリサイズ (アセンブラから使用するため即値を定義)
 */
#define RV64_PGTBL_ENTRY_SIZE              (8)
/**
   ページテーブルのエントリ数 (単位: 個)
 */
#define RV64_PGTBL_ENTRIES_NR              (HAL_PAGE_SIZE/RV64_PGTBL_ENTRY_SIZE)

/**
  ブートページテーブルに必要なVPN1テーブル数 (単位:個)
*/
#define RV64_BOOT_PGTBL_VPN1_NR \
	( ( roundup_align(RV64_STRAIGHT_MAPSIZE, HAL_PAGE_SIZE_2M) / HAL_PAGE_SIZE_2M ) \
	/ RV64_PGTBL_ENTRIES_NR )

/**
   ページテーブルインデクスシフト
   @note FU540-C000 U54コアはSv39 (ページベース 39ビット仮想アドレッシング)をサポートしている
   (物理アドレッシングは38ビット)
 */
/** VPN2のインデクスシフト値(30) */
#define RV64_PGTBL_VPN2_INDEX_SHIFT        \
	rv64_pgtbl_vpn_lvl_to_indexshift(2)
/** VPN1のインデクスシフト値(21) */
#define RV64_PGTBL_VPN1_INDEX_SHIFT        \
	rv64_pgtbl_vpn_lvl_to_indexshift(1)
/** VPN0のインデクスシフト値(12) */
#define RV64_PGTBL_VPN0_INDEX_SHIFT        \
	rv64_pgtbl_vpn_lvl_to_indexshift(0)

/**
   ページテーブルインデクスマスク
 */
/** VPN2インデクスのマスク値
 */
#define RV64_PGTBL_VPN2_INDEX_MASK					\
	(rv64_pgtbl_vpn_lvl_to_indexmask(2))

/** VPN1インデクスのマスク値
 */
#define RV64_PGTBL_VPN1_INDEX_MASK					\
	(rv64_pgtbl_vpn_lvl_to_indexmask(1))

/** VPN0インデクスのマスク値
 */
#define RV64_PGTBL_VPN0_INDEX_MASK					\
	(rv64_pgtbl_vpn_lvl_to_indexmask(0))

/*
 * RISC-V64 アドレス変換スキーム
 */
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

/** 物理ページ番号のマスク  */
#define RV64_PPN_MASK                    (regops_calc_mask_for_val(12, 55))

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
	( ( RV64_PPN_MASK & ( (_pa) >> HAL_PAGE_SHIFT ) ) << RV64_PTE_PA_SHIFT )

/**
   PTE中のページ番号を物理アドレスに変換する
   @param[in] _pte ページテーブルエントリ値
   @return 物理アドレス
*/
#define RV64_PTE_TO_PADDR(_pte) \
	( ( RV64_PPN_MASK & ( (_pte) >> RV64_PTE_PA_SHIFT ) ) << HAL_PAGE_SHIFT )

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
   @param[in] _ppn  ページテーブルベースの物理ページ番号
 */
#define RV64_SATP_VAL(_mode, _asid, _ppn)  \
	( (_mode) | ( (_asid) << RV64_SATP_ASID_SHIFT ) | ( (_ppn) << RV64_SATP_PPN_SHIFT ) )

#if !defined(ASM_FILE)

#include <klib/freestanding.h>
typedef uint64_t        hal_pte;  /* ページテーブルエントリ型 */

#endif  /* !ASM_FILE */
#endif  /*  _HAL_RV64_PGTBL_H  */

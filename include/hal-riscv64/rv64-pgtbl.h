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

/*
 * RISC-V64 アドレス変換スキーム
 */
#define RV64_STAP_MODE_BARE_VAL            (0)  /* Bare アドレス変換なし */
#define RV64_STAP_MODE_SV39_VAL            (8)  /* Sv39 ページベース39ビット仮想アドレス */
#define RV64_STAP_MODE_SV48_VAL            (9)  /* Sv48 ページベース48ビット仮想アドレス */
#define RV64_STAP_MODE_SV57_VAL            (10) /* Sv57 ページベース57ビット仮想アドレス */
#define RV64_STAP_MODE_SV64_VAL            (11) /* Sv64 ページベース64ビット仮想アドレス */

#define RV64_STAP_MODE_SHIFT               (60) /* STAPレジスタのmode値のシフト数 */
#define RV64_STAP_ASID_SHIFT               (44) /* STAPレジスタのasid値のシフト数 */
#define RV64_STAP_PPN_SHIFT                (0)  /* STAPレジスタのppn値のシフト数 */
#define RV64_STAP_MODE_BARE                \
	( ULONGLONG_C(RV64_STAP_MODE_BARE_VAL) << RV64_STAP_MODE_SHIFT )
#define RV64_STAP_MODE_SV39                \
	( ULONGLONG_C(RV64_STAP_MODE_SV39_VAL) << RV64_STAP_MODE_SHIFT )
#define RV64_STAP_MODE_SV48                \
	( ULONGLONG_C(RV64_STAP_MODE_SV48_VAL) << RV64_STAP_MODE_SHIFT )
#define RV64_STAP_MODE_SV57                \
	( ULONGLONG_C(RV64_STAP_MODE_SV57_VAL) << RV64_STAP_MODE_SHIFT )
#define RV64_STAP_MODE_SV64                \
	( ULONGLONG_C(RV64_STAP_MODE_SV64_VAL) << RV64_STAP_MODE_SHIFT )

/**
   物理アドレス長の最大ビット数 (38bit = 128 GiB)
   @note U54コアの物理アドレス長
 */
#define RV64_PGTBL_MAXPHYADDR            (38)

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

/**
   RISC-V 64のSTAPレジスタ値を算出する
   @param[in] _mode アドレス変換スキーム
   @param[in] _asid アドレス空間ID
   @param[in] _ppn  ページテーブルベースの物理アドレス
 */
#define RV64_STAP_VAL(_mode, _asid, _ppn)  \
	( (_mode) | ( (_asid) << RV64_STAP_ASID_SHIFT ) | ( (_ppn) << RV64_STAP_PPN_SHIFT ) )


#endif  /*  _HAL_RV64_PGTBL_H  */

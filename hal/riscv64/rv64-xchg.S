/* -*- mode: gas; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  RISC-V64 cpu exchange operations                                  */
/*                                                                    */
/**********************************************************************/
#define ASM_FILE   1

#include <klib/asm-macros.h>

ASMMAC_TEXT_SECTION

ASMMAC_DECLARE_NAME(rv64_xchg)
ASMMAC_DECLARE_NAME(rv64_xchg64)
	
/** 指定された32ビット値で所定のアドレスの内容をアトミックに交換する
    @param[in] addr ロック変数のアドレス
    @param[in] newval 更新する値
    @return 交換前の値
    @note  uint32_t rv64_xchg(volatile uint32_t *addr, uint32_t newval) 相当
*/
ASMMAC_FUNCTION(rv64_xchg)
	addi sp,sp,-24 /* レジスタ3つ分のアドレスを確保する                              */
	sd s1, 16(sp)  /*  s1レジスタを待避する                                          */
	sd ra,  8(sp)  /*  raレジスタを待避する                                          */
	sd s0,  0(sp)  /*  s0レジスタを待避する                                          */
	add s0, sp, 24 /*  fpを更新する                                                  */

	mv s1, a0      /*  第1引数(ロック変数格納先アドレス)をs1に代入                   */
	amoswap.w.aqrl a0,a1,(s1)  /* s1のメモリの内容をa1の値に設定し, 前の値をa0に設定 */
	sext.w	       a0,a0       /* 符号拡張                                           */

	ld s0, 0(sp)    /*  s0レジスタを復元する                                         */
	ld ra, 8(sp)    /*  raレジスタを復元する                                         */
	ld s1, 16(sp)   /*  s1レジスタを復元する                                         */
	addi sp,sp,24   /*  スタック位置を復元する                                       */
	jr ra           /*  呼び出し元に戻る                                             */

/** 指定された64ビット値で所定のアドレスの内容をアトミックに交換する
    @param[in] addr ロック変数のアドレス
    @param[in] newval 更新する値
    @return 交換前の値
    @note  uint64_t rv64_xchg64(volatile uint64_t *addr, uint64_t newval) 相当
*/
ASMMAC_FUNCTION(rv64_xchg64)
	addi sp,sp,-24 /* レジスタ3つ分のアドレスを確保する                              */
	sd s1, 16(sp)  /*  s1レジスタを待避する                                          */
	sd ra,  8(sp)  /*  raレジスタを待避する                                          */
	sd s0,  0(sp)  /*  s0レジスタを待避する                                          */
	add s0, sp, 24 /*  fpを更新する                                                  */

	mv s1, a0      /*  第1引数(ロック変数格納先アドレス)をs1に代入                   */
	amoswap.d.aqrl a0,a1,(s1)  /* s1のメモリの内容をa1の値に設定し, 前の値をa0に設定 */

	ld s0, 0(sp)    /*  s0レジスタを復元する                                         */
	ld ra, 8(sp)    /*  raレジスタを復元する                                         */
	ld s1, 16(sp)   /*  s1レジスタを復元する                                         */
	addi sp,sp,24   /*  スタック位置を復元する                                       */
	jr ra           /*  呼び出し元に戻る                                             */


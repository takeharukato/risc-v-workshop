/* -*- mode: gas; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  RISC-V64 cpu thread pointer operations                            */
/*                                                                    */
/**********************************************************************/
#define ASM_FILE   1

#include <klib/asm-macros.h>

ASMMAC_TEXT_SECTION

ASMMAC_DECLARE_NAME(rv64_read_tp)	
ASMMAC_DECLARE_NAME(rv64_write_tp)
	
/** 
    tpレジスタの内容を返却
    @retval tpレジスタの内容
    @note  uint64_t rv64_read_tp(void) 相当
*/
rv64_read_tp:
	addi sp,sp,-16   /*  レジスタ2つ分のアドレスを確保する               */
	sd   ra, 8(sp)   /*  raレジスタを待避する                            */
	sd   s0, 0(sp)   /*  s0レジスタを待避する                            */
	add  s0, sp, 16  /*  fpを更新する                                    */

	mv a0, tp        /* tpレジスタの内容を返却     */

	ld s0, 0(sp)     /*  s0レジスタを復元する                            */
	ld ra, 8(sp)     /*  raレジスタを復元する                            */
	addi sp,sp,16    /*  スタック位置を復元する                          */
	jr ra            /*  呼び出し元に戻る                                */

/** 
    tpレジスタの内容を設定
    @param[in] new 設定するレジスタ値
    @retval 更新前のtpレジスタの内容
    @note  uint64_t rv64_write_tp(uint64_t new) 相当
*/
rv64_write_tp:
	addi sp,sp,-24   /*  レジスタ3つ分のアドレスを確保する               */
	sd   s1, 16(sp)  /*  s1レジスタを待避する                            */
	sd   ra,  8(sp)  /*  raレジスタを待避する                            */
	sd   s0, 0(sp)   /*  s0レジスタを待避する                            */
	add  s0, sp, 24  /*  fpを更新する                                    */

	mv s1, a0     /* 第1引数(設定するレジスタ値)をs1に代入 */
	mv a0, tp     /* tpレジスタの内容を返却                */
	mv tp, s1     /* tpレジスタの内容を設定                */

	ld s0,  0(sp)    /*  s0レジスタを復元する                            */
	ld ra,  8(sp)    /*  raレジスタを復元する                            */
	ld s1, 16(sp)    /*  s1レジスタを復元する                            */	
	addi sp,sp,24    /*  スタック位置を復元する                          */
	jr ra            /*  呼び出し元に戻る                                */

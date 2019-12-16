/* -*- mode: gas; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  RISC-V64 cpu machine interrupt operations                         */
/*                                                                    */
/**********************************************************************/
#define ASM_FILE   1

#include <klib/asm-macros.h>

ASMMAC_TEXT_SECTION

ASMMAC_DECLARE_NAME(rv64_read_mie)	
ASMMAC_DECLARE_NAME(rv64_write_mie)
	
/** 
    Machine interrupt enableレジスタの内容を返却
    @return Machine interrupt enableレジスタの内容
    @note  uint64_t rv64_read_mie(void) 相当
*/
ASMMAC_FUNCTION(rv64_read_mie)
	addi sp,sp,-16   /*  レジスタ2つ分のアドレスを確保する               */
	sd   ra, 8(sp)   /*  raレジスタを待避する                            */
	sd   s0, 0(sp)   /*  s0レジスタを待避する                            */
	add  s0, sp, 16  /*  fpを更新する                                    */

	csrr a0, mie     /* Machine interrupt enableレジスタの内容を返却     */

	ld s0, 0(sp)     /*  s0レジスタを復元する                            */
	ld ra, 8(sp)     /*  raレジスタを復元する                            */
	addi sp,sp,16    /*  スタック位置を復元する                          */
	jr ra            /*  呼び出し元に戻る                                */

/** 
    Machine interrupt enableレジスタの内容を設定
    @param[in] new 設定するレジスタ値
    @return 更新前のMachine statusレジスタの内容
    @note  uint64_t rv64_write_mie(uint64_t new) 相当
*/
ASMMAC_FUNCTION(rv64_write_mie)
	addi sp,sp,-24   /*  レジスタ3つ分のアドレスを確保する               */
	sd   s1, 16(sp)  /*  s1レジスタを待避する                            */
	sd   ra,  8(sp)  /*  raレジスタを待避する                            */
	sd   s0, 0(sp)   /*  s0レジスタを待避する                            */
	add  s0, sp, 24  /*  fpを更新する                                    */

	mv s1, a0        /*  第1引数(設定するレジスタ値)をs1に代入          */
	csrr a0, mie     /* Machine interrupt enableレジスタの内容を返却    */
	csrw mie, s1     /* Machine interrupt enableレジスタの内容を設定    */

	ld s0,  0(sp)    /*  s0レジスタを復元する                            */
	ld ra,  8(sp)    /*  raレジスタを復元する                            */
	ld s1, 16(sp)    /*  s1レジスタを復元する                            */	
	addi sp,sp,24    /*  スタック位置を復元する                          */
	jr ra            /*  呼び出し元に戻る                                */

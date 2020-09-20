/* -*- mode: gas; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  RISC-V64 Supervisor Binary Interface Call                         */
/*                                                                    */
/**********************************************************************/
#define ASM_FILE   1

#include <klib/asm-macros.h>

ASMMAC_TEXT_SECTION

ASMMAC_DECLARE_NAME(rv64_sbi_call)

/** 
    Supervisor Binary Interface Callを発行する
    @return a0 返値 (sbiret構造体のerrorメンバ), a1 値(sbiret構造体のvalueメンバ)

    @note struct _rv64_sbi_sbiret rv64_sbi_call(uint64_t func, uint64_t ext, uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3) 相当
    @param[in] func SBI callの機能番号(Function ID)
    @param[in] ext  SBI callの拡張番号(Extension ID)
    @param[in] arg0 SBI callの第1引数
    @param[in] arg1 SBI callの第2引数
    @param[in] arg2 SBI callの第3引数
    @param[in] arg3 SBI callの第4引数	
*/
ASMMAC_FUNCTION(rv64_sbi_call)
	addi sp,sp,-64   /*  レジスタ8つ分の領域を確保する               */
	sd   ra, 56(sp)  /*  raレジスタを待避する                        */	
	sd   s0, 48(sp)  /*  レジスタを待避する                              */	
	sd   s1, 40(sp)  /*  レジスタを待避する                              */	
	sd   s2, 32(sp)  /*  レジスタを待避する                              */	
	sd   s3, 24(sp)  /*  レジスタを待避する                              */
	sd   s4, 16(sp)  /*  レジスタを待避する                              */
	sd   s5, 8(sp)   /*  raレジスタを待避する                            */
	sd   s6, 0(sp)   /*  s0レジスタを待避する                            */
	add  s0, sp, 64  /*  fpを更新する                                    */

	mv s1, a0         /*  SBI callの機能番号をs1に代入                  */
	mv s2, a1         /*  SBI callの拡張番号をs2に代入                  */
	mv s3, a2         /*  SBI callの第1引数をs3に代入                   */
	mv s4, a3         /*  SBI callの第2引数をs4に代入                   */
	mv s5, a4         /*  SBI callの第3引数をs5に代入                   */
	mv s6, a5         /*  SBI callの第4引数をs6に代入                   */
	
	mv a7, s1         /*  SBI callの機能番号をa7に代入                  */
	mv a6, s2         /*  SBI callの拡張番号をa6に代入                  */
	mv a0, s3         /*  SBI callの第1引数をa0に代入                   */
	mv a1, s4         /*  SBI callの第2引数をa1に代入                   */
	mv a2, s5         /*  SBI callの第3引数をa2に代入                   */
	mv a3, s6         /*  SBI callの第4引数をa3に代入                   */

	ecall             /*  SBI callを発行                                */
	
	ld s6,  0(sp)     /*  レジスタを復元する                            */
	ld s5,  8(sp)     /*  レジスタを復元する                            */
	ld s4, 16(sp)     /*  レジスタを復元する                            */
	ld s3, 24(sp)     /*  レジスタを復元する                            */
	ld s2, 32(sp)     /*  レジスタを復元する                            */
	ld s1, 40(sp)     /*  レジスタを復元する                            */
	ld s0, 48(sp)     /*  fpを復元する                                  */
	ld ra, 56(sp)     /*  レジスタを復元する                            */
	addi sp,sp,64     /*  スタック位置を復元する                        */
	jr ra             /*  呼び出し元に戻る                              */


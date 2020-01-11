/* -*- mode: gas; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  RISC-V64 cpu mmu operations                                       */
/*                                                                    */
/**********************************************************************/
#define ASM_FILE   1

#include <klib/asm-macros.h>

ASMMAC_TEXT_SECTION

ASMMAC_DECLARE_NAME(rv64_write_satp)	
ASMMAC_DECLARE_NAME(rv64_flush_tlb_local)
	
/** 
    SATPレジスタを設定する
    @param[in] a0   satpレジスタ値
    @note  void rv64_write_satp(uint64_t stap) 相当
*/
rv64_write_satp:
	addi sp,sp,-24   /*  レジスタ3つ分のアドレスを確保する               */
	sd   s1, 16(sp)  /*  s1レジスタを待避する                            */
	sd   ra,  8(sp)  /*  raレジスタを待避する                            */
	sd   s0, 0(sp)   /*  s0レジスタを待避する                            */
	add  s0, sp, 24  /*  fpを更新する                                    */

	mv s1, a0        /*  第1引数(設定するレジスタ値)をs1に代入           */
	csrw satp, s1    /*  satpレジスタ値をロードする                      */
	sfence.vma zero, zero /* TLBをフラッシュする                         */	

	ld s0,  0(sp)    /*  s0レジスタを復元する                            */
	ld ra,  8(sp)    /*  raレジスタを復元する                            */
	ld s1, 16(sp)    /*  s1レジスタを復元する                            */	
	addi sp,sp,24    /*  スタック位置を復元する                          */
	jr ra            /*  呼び出し元に戻る                                */

/** 
    TLBをフラッシュする
    @note  void rv64_flush_tlb_local(void) 相当
*/
rv64_flush_tlb_local:
	addi sp,sp,-16   /*  レジスタ2つ分のアドレスを確保する               */
	sd   ra,  8(sp)  /*  raレジスタを待避する                            */
	sd   s0, 0(sp)   /*  s0レジスタを待避する                            */
	add  s0, sp, 16  /*  fpを更新する                                    */

	sfence.vma zero, zero

	ld s0, 0(sp)   /*  s0レジスタを復元する                            */
	ld ra, 8(sp)   /*  raレジスタを復元する                            */
	addi sp,sp,16  /*  スタック位置を復元する                          */
	jr ra          /*  呼び出し元に戻る                                */

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
	addi sp,sp,-8  /*  fp分のアドレスを確保する                        */
	sd s0, 0(sp)   /*  s0レジスタを待避する                            */
	csrw satp, a0  /*  ページテーブルをロードする                      */
	sfence.vma zero, zero /* TLBをフラッシュする                       */	
	ld s0, 0(sp)   /*  s0レジスタを復元する                            */
	addi sp,sp,8   /*  スタック位置を復元する                          */
	jr ra          /*  呼び出し元に戻る                                */

/** 
    TLBをフラッシュする
    @note  void rv64_flush_tlb_local(void) 相当
*/
rv64_flush_tlb_local:
	addi sp,sp,-8  /* fpレジスタ保存アドレスを確保する               */
	sd s0, 0(sp)   /*  s0レジスタを待避する                           */
	sfence.vma zero, zero
	ld s0, 0(sp)   /*  s0レジスタを復元する                           */
	addi sp,sp,8  /*  スタック位置を復元する                         */
	jr ra          /*  呼び出し元に戻る                               */

	
	

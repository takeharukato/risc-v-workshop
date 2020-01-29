/* -*- mode: gas; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  RISC-V64 cpu halt operations                                      */
/*                                                                    */
/**********************************************************************/
#define ASM_FILE   1

#include <klib/asm-macros.h>
#include <hal/riscv64.h>

ASMMAC_TEXT_SECTION

ASMMAC_DECLARE_NAME(hal_cpu_halt)
	
/** 
    CPUを休眠させる
    @note  void hal_cpu_halt(void) 相当
*/
ASMMAC_FUNCTION(hal_cpu_halt)
	addi sp,sp,-16   /*  レジスタ2つ分のアドレスを確保する               */
	sd   ra, 8(sp)   /*  raレジスタを待避する                            */
	sd   s0, 0(sp)   /*  s0レジスタを待避する                            */
	add  s0, sp, 16  /*  fpを更新する                                    */

	csrc sstatus, SSTATUS_SIE /* Supervisorへの割込みを禁止する          */
	wfi                       /* 割込み待ち合せて停止する                */

	ld s0, 0(sp)     /*  s0レジスタを復元する                            */
	ld ra, 8(sp)     /*  raレジスタを復元する                            */
	addi sp,sp,16    /*  スタック位置を復元する                          */
	jr ra            /*  呼び出し元に戻る                                */


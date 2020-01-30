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
	addi sp,sp,-32   /*  レジスタ4つ分のアドレスを確保する               */
	sd   s2,24(sp)   /*  s1レジスタを待避する                            */
	sd   s1,16(sp)   /*  s1レジスタを待避する                            */
	sd   ra, 8(sp)   /*  raレジスタを待避する                            */
	sd   s0, 0(sp)   /*  s0レジスタを待避する                            */
	add  s0, sp, 24  /*  fpを更新する                                    */

	csrr s1, sie     /* sieの内容をs1に退避する                          */

	csrc sstatus, SSTATUS_SIE /* Supervisorへの割込みを禁止する          */

	li s2, SIE_SEIE|SIE_STIE|SIE_SSIE /* 割込みを受け付ける              */
	or s2, s1, s2             /* sieレジスタの現在値に反映する           */
	csrw sie, s2              /* sieを更新する                           */

	fence                     /* メモリへの書き込みを待ち合わせる        */
	wfi                       /* 割込み待ち合せて停止する                */
	nop                       /* 念のためwfi復帰先にnopを入れておく      */

	csrw sie, s1     /* sieの内容を復元する                              */
	
	ld s0, 0(sp)     /*  s0レジスタを復元する                            */
	ld ra, 8(sp)     /*  raレジスタを復元する                            */
	ld s1, 16(sp)    /*  s1レジスタを復元する                            */
	ld s2, 24(sp)    /*  s2レジスタを復元する                            */		
	addi sp,sp,32    /*  スタック位置を復元する                          */
	jr ra            /*  呼び出し元に戻る                                */


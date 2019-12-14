/* -*- mode: gas; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  RISC-V64 thread context switch                                    */
/*                                                                    */
/**********************************************************************/
#define ASM_FILE   1

#include <klib/asm-macros.h>
#include <klib/asm-offset.h>
#include <hal/hal-thread.h>

ASMMAC_TEXT_SECTION

ASMMAC_DECLARE_NAME(hal_do_context_switch)

/**
    スレッドを切り替える
    @param[in] prev 現在のスレッドのスレッドスイッチコンテキスト格納先アドレスを指し示すポインタのアドレス
    @param[in] next 切り替え先のスレッドのスレッドスイッチコンテキスト格納先アドレスを指し示すポインタのアドレス
    @note  void hal_do_context_switch(void **prev, void **next) 相当
*/
ASMMAC_FUNCTION(hal_do_context_switch)
	addi sp,sp,-16   /*  レジスタ2つ分のアドレスを確保する               */
	sd   ra, 8(sp)   /*  raレジスタを待避する                            */
	sd   s0,  (sp)   /*  s0レジスタを待避する                            */
	add  s0, sp, 16  /*  fpを更新する                                    */

	addi sp, sp, -RV64_THRSW_CONTEXT_SIZE /* コンテキスト保存領域を確保  */
	RV64_ASM_SAVE_THRSW_CONTEXT(sp)  /* スレッドスイッチコンテキストを保存 */
	sd   sp, (a0)  /* 現在のスレッドのコンテキスト格納先アドレスを更新  */

	ld   sp, (a1)  /* 切り替え先スレッドのコンテキスト格納先アドレスをspにロード */
	RV64_ASM_RESTORE_THRSW_CONTEXT(sp)   /* スレッドスイッチコンテキストを復元 */
	addi sp, sp, RV64_THRSW_CONTEXT_SIZE /* コンテキスト保存領域を解放  */
	
	ld s0,  (sp)     /*  s0レジスタを復元する                            */
	ld ra, 8(sp)     /*  raレジスタを復元する                            */
	addi sp,sp,16    /*  スタック位置を復元する                          */
	jr ra            /*  呼び出し元に戻る                                */

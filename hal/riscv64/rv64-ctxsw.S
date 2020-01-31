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

ASMMAC_DECLARE_NAME(hal_thread_switch)

/**
    スレッドを切り替える
    @param[in] prev 現在のスレッドのスレッドスイッチコンテキスト格納先アドレスを指し示すポインタのアドレス
    @param[in] next 切り替え先のスレッドのスレッドスイッチコンテキスト格納先アドレスを指し示すポインタのアドレス
    @note  void hal_thread_switch(void **prev, void **next) 相当
*/
ASMMAC_FUNCTION(hal_thread_switch)
	/* @note スタック操作を伴うリーフ関数なので,
	   関数エントリ/復帰処理(リターンアドレス/フレームポインタの退避・復元)は行わない
	*/
	
	addi sp, sp, -RV64_THRSW_CONTEXT_SIZE /* コンテキスト保存領域を確保  */
	RV64_ASM_SAVE_THRSW_CONTEXT(sp)  /* スレッドスイッチコンテキストを保存 */
	sd   sp, (a0)  /* 現在のスレッドのコンテキスト格納先アドレスを更新  */

	ld   sp, (a1)  /* 切り替え先スレッドのコンテキスト格納先アドレスをspにロード */
	RV64_ASM_RESTORE_THRSW_CONTEXT(sp)   /* スレッドスイッチコンテキストを復元 */
	addi sp, sp, RV64_THRSW_CONTEXT_SIZE /* コンテキスト保存領域を解放  */
	
	jr ra            /*  呼び出し元に戻る                                */

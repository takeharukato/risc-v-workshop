/* -*- mode: gas; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  x64 userland thread context switch                                */
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
    @param[in] prev 切り替えられるスレッドのスタックアドレスを格納しているポインタ変数のアドレス
    @param[in] next 切り替えるスレッドのスタックアドレスを格納しているポインタ変数のアドレス
    @note PSW(Processor Status Word - x64の場合, RFLAGSとRIPの組)と
          AMD64 ABI Draft 0.99.5(http://www.x86-64.org/documentation/abi.pdf)で規定された
          callee savedレジスタのうち x87 FPU 制御ワード以外のレジスタを退避/復元する
	  x64のcallee savedレジスタ(AMD64 ABI Draft 0.99.5 Figure 3.4: Register Usage参照)
    @note  void hal_thread_switch(void **prev, void **next) 相当
*/
ASMMAC_FUNCTION(hal_thread_switch)
	/* スレッドコンテキストを保存 */
	pushfq            /* rflags */
	pushq %rbx        /* rbx    */
	pushq %rbp        /* rbp    */
	pushq %r12        /* r12    */
	pushq %r13        /* r13    */
	pushq %r14        /* r14    */
	pushq %r15        /* r15    */
	movq %rsp, (%rdi) /* 現在のスレッドのコンテキスト格納先アドレスを更新 */

	movq (%rsi), %rsp /* 切り替え先スレッドのコンテキスト格納先アドレスをrspにロード */
	popq %r15         /* r15    */
	popq %r14         /* r14    */
	popq %r13         /* r13    */
	popq %r12         /* r12    */
	popq %rbp         /* rbp    */
	popq %rbx         /* rbx    */
	popfq             /* rflags */
	ret               /* ripを復元 */

	
	

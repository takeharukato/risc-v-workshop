/* -*- mode: gas; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  x64 stack operation                                               */
/*                                                                    */
/**********************************************************************/

#define ASM_FILE   1
#include <kern/kern-autoconf.h>
#include <kern/kern-consts.h>
#include <klib/asm-macros.h>

ASMMAC_TEXT_SECTION

/**
	スタックポインタを設定してvoid func(void *)型の関数を呼び出す
	void hal_call_with_newstack(void *func, void *argp, void *new_sp,
	void **old_sp)
	相当
	@param[in]  func      呼び出す関数のアドレス (%rdi)
	@param[in]  argp      関数呼び出しの際に引き渡す引数 (%rsi)
	@param[in]  new_sp    呼び出された関数にエントリした際のスタックポインタ値 (%rdx)
*/
ASMMAC_FUNCTION(hal_call_with_newstack)
	mov %rdx, %rsp	/* スタックを更新 */
	mov %rdi, %rax  /* 呼び出し先アドレスを保存 */
	mov %rsi, %rdi  /* 第1引数にargpを設定 */
	/** @note 関数エントリ後のスタックポインタのアドレスが第3引数で指定された値に
	 *  設定されていなければならないのでcall命令ではなくjmp命令を使用する
	 */
	jmp	*%rax /* 指定された関数を呼び出し */
1:	
	jmp 1b  /* ここには来ない */

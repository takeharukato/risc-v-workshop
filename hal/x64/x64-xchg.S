/* -*- mode: gas; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  x64 cpu exchange operations                                       */
/*                                                                    */
/**********************************************************************/
#define ASM_FILE   1

.code64

.global x64_xchg, x64_pause

/** 指定された値で所定のアドレスの内容をアトミックに交換する
    @param[in] addr ロック変数のアドレス
    @param[in] newval 更新する値
    @retval 交換前の値
    @note  uint32_t x64_xchg(volatile uint32_t *addr, uint32_t newval) 相当
*/
x64_xchg:
	pushq %rbp
	mov   %rsp, %rbp
	lock; xchgl (%rdi), %esi
	movq  %rsi, %rax
	leaveq	
	retq

/** Spinlock用電力消費削減付き待ち処理
*/
x64_pause:
	pushq %rbp
	mov   %rsp, %rbp
	pause
	leaveq	
	retq
	

/* -*- mode: gas; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  x64 cpu interrupt flags operations                                */
/*                                                                    */
/**********************************************************************/
#define ASM_FILE   1

.code64

.global x64_cpu_disable_interrupt
.global x64_cpu_enable_interrupt
.global x64_cpu_save_flags
.global x64_cpu_restore_flags

/** CPUへの割込みを禁止する
    @note void x64_cpu_disable_interrupt(void) 相当 
*/
x64_cpu_disable_interrupt:
	pushq %rbp
	mov   %rsp, %rbp
	cli
	leaveq	
	retq

/** CPUへの割込みを許可する
    @note void x64_cpu_enable_interrupt(void) 相当 
 */
x64_cpu_enable_interrupt:
	pushq %rbp
	mov   %rsp, %rbp
	sti
	leaveq	
	retq
	
/** CPUへの割込み禁止状態を復元する
    @param[in] flags 割込禁止状態の保存先アドレス
    @note void x64_cpu_restore_flags(intrflags *flags) 相当
 */
x64_cpu_restore_flags:
	pushq %rbp
	mov   %rsp, %rbp
	movq	(%rdi), %rax
	pushq	%rax	
	popfq
	leaveq	
	retq

/** CPUへの割込み禁止状態を保存する
    @param[in] flags フラグ状態の保存先アドレス
    @note void x64_cpu_save_flags(intrflags *flags)相当
 */
x64_cpu_save_flags:
	pushq %rbp
	mov   %rsp, %rbp
	pushfq
	popq	%rax
	movq	%rax, (%rdi)
	leaveq	
	retq
	

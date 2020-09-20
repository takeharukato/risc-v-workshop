/* -*- mode: c; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Offset definitions for assembler routines                         */
/*                                                                    */
/**********************************************************************/

#include <klib/asm-offset-helper.h>
#include <klib/compiler.h>
#include <klib/freestanding.h>

#include <hal/hal-traps.h>
#include <hal/hal-thread.h>

#include <kern/thr-preempt.h>

int
main(int __unused argc, char __unused *argv[]) {

	DEFINE_VAL(TI_THREAD_INFO_SIZE, sizeof(struct _thread_info));
	OFFSET(TI_MAGIC_OFFSET, _thread_info, magic);
	OFFSET(TI_INTRCNT_OFFSET, _thread_info, intrcnt);
	OFFSET(TI_PREEMPT_OFFSET, _thread_info, preempt);
	OFFSET(TI_FLAGS_OFFSET, _thread_info, flags);
	OFFSET(TI_MDFLAGS_OFFSET, _thread_info, mdflags);
	OFFSET(TI_CPU_OFFSET, _thread_info, cpu);
	OFFSET(TI_KSTACK_OFFSET, _thread_info, kstack);
	OFFSET(TI_THR_OFFSET, _thread_info, thr);

	/*
	 * Trapコンテキスト情報
	 */
	DEFINE_VAL(X64_TRAP_CONTEXT_SIZE, sizeof(struct _trap_context));
	OFFSET(X64_TRAP_CONTEXT_RAX, _trap_context, rax);
	OFFSET(X64_TRAP_CONTEXT_RBX, _trap_context, rbx);
	OFFSET(X64_TRAP_CONTEXT_RCX, _trap_context, rcx);
	OFFSET(X64_TRAP_CONTEXT_RDX, _trap_context, rdx);
	OFFSET(X64_TRAP_CONTEXT_RBP, _trap_context, rbp);
	OFFSET(X64_TRAP_CONTEXT_RSI, _trap_context, rsi);
	OFFSET(X64_TRAP_CONTEXT_RDI, _trap_context, rdi);
	OFFSET(X64_TRAP_CONTEXT_R8, _trap_context, r8);
	OFFSET(X64_TRAP_CONTEXT_R9, _trap_context, r9);
	OFFSET(X64_TRAP_CONTEXT_R10, _trap_context, r10);
	OFFSET(X64_TRAP_CONTEXT_R11, _trap_context, r11);
	OFFSET(X64_TRAP_CONTEXT_R12, _trap_context, r12);
	OFFSET(X64_TRAP_CONTEXT_R13, _trap_context, r13);
	OFFSET(X64_TRAP_CONTEXT_R14, _trap_context, r14);
	OFFSET(X64_TRAP_CONTEXT_R15, _trap_context, r15);
	OFFSET(X64_TRAP_CONTEXT_TRAPNO, _trap_context, trapno);
	OFFSET(X64_TRAP_CONTEXT_ERRNO, _trap_context, errno);
#if !defined(CONFIG_HAL)
	OFFSET(X64_TRAP_CONTEXT_RFLAGS, _trap_context, rflags);
	OFFSET(X64_TRAP_CONTEXT_RIP, _trap_context, rip);
#else
	OFFSET(X64_TRAP_CONTEXT_RIP, _trap_context, rip);
	OFFSET(X64_TRAP_CONTEXT_CS, _trap_context, cs);
	OFFSET(X64_TRAP_CONTEXT_RFLAGS, _trap_context, rflags);
	OFFSET(X64_TRAP_CONTEXT_RSP, _trap_context, rsp);
	OFFSET(X64_TRAP_CONTEXT_SS, _trap_context, ss);
#endif  /*  !CONFIG_HAL  */

	/*
	 * スレッドコンテキスト情報
	 */
	DEFINE_VAL(X64_THRSW_CONTEXT_SIZE, sizeof(struct _x64_thrsw_context));
	OFFSET(X64_THRSW_CONTEXT_R15, _x64_thrsw_context, r15);
	OFFSET(X64_THRSW_CONTEXT_R14, _x64_thrsw_context, r14);
	OFFSET(X64_THRSW_CONTEXT_R13, _x64_thrsw_context, r13);
	OFFSET(X64_THRSW_CONTEXT_R12, _x64_thrsw_context, r12);
	OFFSET(X64_THRSW_CONTEXT_RBP, _x64_thrsw_context, rbp);
	OFFSET(X64_THRSW_CONTEXT_RBX, _x64_thrsw_context, rbx);
	OFFSET(X64_THRSW_CONTEXT_RFLAGS, _x64_thrsw_context, rflags);
	OFFSET(X64_THRSW_CONTEXT_RIP, _x64_thrsw_context, rip);

	return 0;
}

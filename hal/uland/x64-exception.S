/* -*- mode: gas; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  RISC-V64 vector routines                                          */
/*                                                                    */
/**********************************************************************/
#define ASM_FILE   1
#include <kern/kern-consts.h>
#include <klib/asm-macros.h>
#include <klib/asm-offset.h>
#include <hal/hal-traps.h>
/**
    void x64_return_from_trap(void)相当のサブルーチン
 */
ASMMAC_FUNCTION(x64_return_from_trap)
	/*
	 * コンテキスト復元
	 */
	popq %rax
	popq %rbx
	popq %rcx
	popq %rdx
	popq %rbp
	popq %rsi
	popq %rdi
	popq %r8
	popq %r9
	popq %r10
	popq %r11
	popq %r12
	popq %r13
	popq %r14
	popq %r15
        # remove trap number and errorcode
	add $16, %rsp
#if !defined(CONFIG_HAL)
	popfq             /* rflags */
	ret               /* ripを復元 */
#else
	iretq
#endif

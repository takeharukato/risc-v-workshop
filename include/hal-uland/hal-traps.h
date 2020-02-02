/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  x64 userland trap context definitions                             */
/*                                                                    */
/**********************************************************************/
#if !defined(_HAL_HAL_TRAPS_H)
#define  _HAL_HAL_TRAPS_H

#define X64_DIV_ERR       (0)
#define X64_DEBUG_EX      (1)
#define X64_NMI           (2)
#define X64_BREAKPOINT    (3)
#define X64_OVERFLOW      (4)
#define X64_BOUND_RANGE   (5)
#define X64_INVALID_OP    (6)
#define X64_DEVICE_NA     (7)
#define X64_DFAULT        (8)
#define X64_CO_SEG_OF     (9)
#define X64_INVALID_TSS   (10)
#define X64_SEG_NP        (11)
#define X64_STACK_FAULT   (12)
#define X64_GPF           (13)
#define X64_PAGE_FAULT    (14)
#define X64_FPE           (16)
#define X64_ALIGN_CHECK   (17)
#define X64_MCE           (18)
#define X64_SIMD_FPE      (19)

#define X64_NR_EXCEPTIONS (20)

#define TRAP_SYSCALL  (0x90)
#define NR_TRAPS      (256)

/** ページフォルトエラーコード
 */
#define PGFLT_ECODE_PROT     (1)  /*<  プロテクションフォルト  */
#define PGFLT_ECODE_WRITE    (2)  /*<  書込みフォルト          */
#define PGFLT_ECODE_USER     (4)  /*<  ユーザフォルト          */
#define PGFLT_ECODE_RSV      (8)  /*<  予約                    */
#define PGFLT_ECODE_INSTPREF (16) /*<  命令プリフェッチ        */

#if !defined(ASM_FILE)
#include <klib/freestanding.h>
#include <kern/kern-types.h>
#include <hal/x64-rflags.h>

/**
   トラップコンテキスト
 */
typedef struct _trap_context{	
	reg_type rax;
	reg_type rbx;
	reg_type rcx;
	reg_type rdx;
	reg_type rbp;
	reg_type rsi;
	reg_type rdi;
	reg_type  r8;
	reg_type  r9;
	reg_type r10;
	reg_type r11;
	reg_type r12;
	reg_type r13;
	reg_type r14;
	reg_type r15;
	reg_type trapno;
	reg_type errno;
#if !defined(CONFIG_HAL)
	reg_type rflags;
	reg_type rip;
#else
	reg_type rip;
	reg_type cs;
	reg_type rflags;
	reg_type rsp;
	reg_type ss;
#endif
}trap_context;
void x64_return_from_trap(void);
#endif  /*  ASM_FILE  */
#endif  /* _HAL_HAL_TRAPS_H  */

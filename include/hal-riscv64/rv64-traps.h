/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  RISC-V 64 trap context definitions                                */
/*                                                                    */
/**********************************************************************/
#if !defined(_HAL_RV64_TRAPS_H)
#define  _HAL_RV64_TRAPS_H
#if !defined(ASM_FILE)
#include <klib/freestanding.h>
#include <kern/kern-types.h>
/**
   トラップコンテキスト
   @note xレジスタの順に格納 (x0は不変なので退避不要)
   RISC-V ELF psABI specification
   https://github.com/riscv/riscv-elf-psabi-doc/blob/master/riscv-elf.md
 */
typedef struct _trap_context{	
	uint64_t  epc;  /*  saved user program counter */
	uint64_t   ra;  /*  x1 */
	uint64_t   sp;  /*  x2 */
	uint64_t   gp;  /*  x3 */
	uint64_t   tp;  /*  x4 */
	uint64_t   t0;  /*  x5 */
	uint64_t   t1;  /*  x6 */
	uint64_t   t2;  /*  x7 */
	uint64_t   s0;  /*  x8 */
	uint64_t   s1;  /*  x9 */
	uint64_t   a0;  /* x10 */
	uint64_t   a1;  /* x11 */
	uint64_t   a2;  /* x12 */
	uint64_t   a3;  /* x13 */
	uint64_t   a4;  /* x14 */
	uint64_t   a5;  /* x15 */
	uint64_t   a6;  /* x16 */
	uint64_t   a7;  /* x17 */
	uint64_t   s2;  /* x18 */
	uint64_t   s3;  /* x19 */
	uint64_t   s4;  /* x20 */
	uint64_t   s5;  /* x21 */
	uint64_t   s6;  /* x22 */
	uint64_t   s7;  /* x23 */
	uint64_t   s8;  /* x24 */
	uint64_t   s9;  /* x25 */
	uint64_t  s10;  /* x26 */
	uint64_t  s11;  /* x27 */
	uint64_t   t3;  /* x28 */
	uint64_t   t4;  /* x29 */
	uint64_t   t5;  /* x30 */
	uint64_t   t6;  /* x31 */
}trap_context;
#endif  /*  ASM_FILE  */
#endif  /* _HAL_RV64_MSCRATCH_H  */

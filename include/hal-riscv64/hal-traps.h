/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  RISC-V 64 trap context definitions                                */
/*                                                                    */
/**********************************************************************/
#if !defined(_HAL_HAL_TRAPS_H)
#define  _HAL_HAL_TRAPS_H
#if defined(ASM_FILE)

#define RV64_VECTORBASE_ALIGN       (4)  /**< ベクタベースアラインメント(単位:バイト) */

/**
   割込みコンテスト退避共通処理
   @param[in] _ctx  トラップコンテキスト先頭アドレスを保存しているレジスタ
   @note epc, estatus, sp, t0以外のレジスタを保存する (rv64-mmode.S, rv64-vector.S参照)
 */
#define RV64_ASM_SAVE_CONTEXT_COMMON(_ctx)        \
        sd ra,  RV64_TRAP_CONTEXT_RA(_ctx);        \
	sd gp,  RV64_TRAP_CONTEXT_GP(_ctx);        \
	sd t1,  RV64_TRAP_CONTEXT_T1(_ctx);        \
	sd t2,  RV64_TRAP_CONTEXT_T2(_ctx);        \
	sd a0,  RV64_TRAP_CONTEXT_A0(_ctx);        \
	sd a1,  RV64_TRAP_CONTEXT_A1(_ctx);        \
	sd a2,  RV64_TRAP_CONTEXT_A2(_ctx);        \
	sd a3,  RV64_TRAP_CONTEXT_A3(_ctx);        \
	sd a4,  RV64_TRAP_CONTEXT_A4(_ctx);        \
	sd a5,  RV64_TRAP_CONTEXT_A5(_ctx);        \
	sd a6,  RV64_TRAP_CONTEXT_A6(_ctx);        \
	sd a7,  RV64_TRAP_CONTEXT_A7(_ctx);        \
	sd t3,  RV64_TRAP_CONTEXT_T3(_ctx);        \
	sd t4,  RV64_TRAP_CONTEXT_T4(_ctx);        \
	sd t5,  RV64_TRAP_CONTEXT_T5(_ctx);        \
	sd t6,  RV64_TRAP_CONTEXT_T6(_ctx);

/**
   スーパバイザモード割込みコンテスト退避処理
   @param[in] _ctx  トラップコンテキスト先頭アドレスを保存しているレジスタ
   @note sepc, sstatusレジスタを保存する (rv64-vector.S参照). 事前にs1の退避が必要.
 */
#define RV64_ASM_SAVE_SUPERVISOR_CONTEXT_COMMON(_ctx)	\
	csrr s1, sepc;					\
	sd   s1, RV64_TRAP_CONTEXT_EPC(_ctx);		\
        csrr s1, sstatus;				\
        sd   s1, RV64_TRAP_CONTEXT_ESTATUS(_ctx);

/**
   割込みコンテスト復元共通処理
   @param[in] _ctx  トラップコンテキスト先頭アドレスを保存しているレジスタ
   @note epc, estatus, sp, tp以外のレジスタを復元する (rv64-vector.S参照)
 */
#define RV64_ASM_RESTORE_CONTEXT_COMMON(_ctx) \
        ld t6, RV64_TRAP_CONTEXT_T6(_ctx);     \
	ld t5, RV64_TRAP_CONTEXT_T5(_ctx);     \
	ld t4, RV64_TRAP_CONTEXT_T4(_ctx);     \
	ld t3, RV64_TRAP_CONTEXT_T3(_ctx);     \
	ld a7, RV64_TRAP_CONTEXT_A7(_ctx);     \
	ld a6, RV64_TRAP_CONTEXT_A6(_ctx);     \
	ld a5, RV64_TRAP_CONTEXT_A5(_ctx);     \
	ld a4, RV64_TRAP_CONTEXT_A4(_ctx);     \
	ld a3, RV64_TRAP_CONTEXT_A3(_ctx);     \
	ld a2, RV64_TRAP_CONTEXT_A2(_ctx);     \
	ld a1, RV64_TRAP_CONTEXT_A1(_ctx);     \
	ld a0, RV64_TRAP_CONTEXT_A0(_ctx);     \
	ld t2, RV64_TRAP_CONTEXT_T2(_ctx);     \
	ld t1, RV64_TRAP_CONTEXT_T1(_ctx);     \
	ld t0, RV64_TRAP_CONTEXT_T0(_ctx);     \
	ld gp, RV64_TRAP_CONTEXT_GP(_ctx);     \
	ld ra, RV64_TRAP_CONTEXT_RA(_ctx);

#else
#include <klib/freestanding.h>
#include <kern/kern-types.h>

/**
   トラップコンテキスト
   @note xレジスタの順に格納 (x0は不変なので退避不要)
   RISC-V ELF psABI specification
   https://github.com/riscv/riscv-elf-psabi-doc/blob/master/riscv-elf.md
 */
typedef struct _trap_context{
	reg_type       ra;  /*  x1 */
	reg_type       sp;  /*  x2 */
	reg_type       gp;  /*  x3 */
	reg_type       t0;  /*  x5 */
	reg_type       t1;  /*  x6 */
	reg_type       t2;  /*  x7 */
	reg_type       a0;  /* x10 */
	reg_type       a1;  /* x11 */
	reg_type       a2;  /* x12 */
	reg_type       a3;  /* x13 */
	reg_type       a4;  /* x14 */
	reg_type       a5;  /* x15 */
	reg_type       a6;  /* x16 */
	reg_type       a7;  /* x17 */
	reg_type       t3;  /* x28 */
	reg_type       t4;  /* x29 */
	reg_type       t5;  /* x30 */
	reg_type       t6;  /* x31 */
	reg_type  estatus;  /* mstatus/status             */
	reg_type      epc;  /* saved user program counter */
}trap_context;
void rv64_return_from_trap(void);
#endif  /*  ASM_FILE  */
#endif  /* _HAL_HAL_TRAPS_H  */

/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  RISC-V 64 specific thread definitions                             */
/*                                                                    */
/**********************************************************************/
#if !defined(_HAL_HAL_THREAD_H)
#define  _HAL_HAL_THREAD_H
#if defined(ASM_FILE)

/**
   スレッドスイッチコンテスト退避処理
   @param[in] _ctx  スレッドスイッチコンテキスト保存先アドレス(caller saveレジスタを指定)
   @note 関数呼び出し間で保存しなければならないs0-s11の12レジスタ
   (Callee savedレジスタ)を保存する。spは別途スレッド管理情報中に保存する。
   スレッドスタートアドレスの指定のためにリターンアドレス(ra)を保存する。
   RISC-V ELF psABI specificationのInteger Register Convention
   https://github.com/riscv/riscv-elf-psabi-doc/blob/master/riscv-elf.md
   参照。
 */
#define RV64_ASM_SAVE_THRSW_CONTEXT(_ctx)        \
	sd ra,  RV64_THRSW_CONTEXT_RA(_ctx);        \
	sd s0,  RV64_THRSW_CONTEXT_S0(_ctx);        \
	sd s1,  RV64_THRSW_CONTEXT_S1(_ctx);        \
	sd s2,  RV64_THRSW_CONTEXT_S2(_ctx);        \
	sd s3,  RV64_THRSW_CONTEXT_S3(_ctx);        \
	sd s4,  RV64_THRSW_CONTEXT_S4(_ctx);        \
	sd s5,  RV64_THRSW_CONTEXT_S5(_ctx);        \
	sd s6,  RV64_THRSW_CONTEXT_S6(_ctx);        \
	sd s7,  RV64_THRSW_CONTEXT_S7(_ctx);        \
	sd s8,  RV64_THRSW_CONTEXT_S8(_ctx);        \
	sd s9,  RV64_THRSW_CONTEXT_S9(_ctx);        \
	sd s10, RV64_THRSW_CONTEXT_S10(_ctx);       \
	sd s11, RV64_THRSW_CONTEXT_S11(_ctx);       \

/**
   スレッドスイッチコンテスト復元処理
   @param[in] _ctx  スレッドスイッチコンテキスト保存先アドレス(caller saveレジスタを指定)
   @note 関数呼び出し間で保存しなければならないs0-s11の12レジスタ
   (Callee savedレジスタ)を復元する。spは別途スレッド管理情報から復元する。
   スレッドスタートアドレスの指定のためにリターンアドレス(ra)を復元する。
   RISC-V ELF psABI specificationのInteger Register Convention
   https://github.com/riscv/riscv-elf-psabi-doc/blob/master/riscv-elf.md
   参照。
 */
#define RV64_ASM_RESTORE_THRSW_CONTEXT(_ctx) \
	ld s11, RV64_THRSW_CONTEXT_S11(_ctx);   \
	ld s10, RV64_THRSW_CONTEXT_S10(_ctx);   \
	ld s9, RV64_THRSW_CONTEXT_S9(_ctx);     \
	ld s8, RV64_THRSW_CONTEXT_S8(_ctx);     \
	ld s7, RV64_THRSW_CONTEXT_S7(_ctx);     \
	ld s6, RV64_THRSW_CONTEXT_S6(_ctx);     \
	ld s5, RV64_THRSW_CONTEXT_S5(_ctx);     \
	ld s4, RV64_THRSW_CONTEXT_S4(_ctx);     \
	ld s3, RV64_THRSW_CONTEXT_S3(_ctx);     \
	ld s2, RV64_THRSW_CONTEXT_S2(_ctx);     \
	ld s1, RV64_THRSW_CONTEXT_S1(_ctx);     \
	ld s0, RV64_THRSW_CONTEXT_S0(_ctx);     \
	ld ra, RV64_THRSW_CONTEXT_RA(_ctx);

#else
#include <klib/freestanding.h>
#include <kern/kern-types.h>

/**
   スレッドスイッチコンテキスト
   @note 関数呼び出し間で保存しなければならないs0-s11の12レジスタ
   (Callee savedレジスタ)を退避・復元するための領域の定義。
   RISC-V ELF psABI specificationのInteger Register Convention
   https://github.com/riscv/riscv-elf-psabi-doc/blob/master/riscv-elf.md
   参照。
 */
typedef struct _rv64_thrsw_context{
	reg_type       ra;  /*  x1 */
	reg_type       s0;  /*  x8 */
	reg_type       s1;  /*  x9 */
	reg_type       s2;  /* x18 */
	reg_type       s3;  /* x19 */
	reg_type       s4;  /* x20 */
	reg_type       s5;  /* x21 */
	reg_type       s6;  /* x22 */
	reg_type       s7;  /* x23 */
	reg_type       s8;  /* x24 */
	reg_type       s9;  /* x25 */
	reg_type      s10;  /* x26 */
	reg_type      s11;  /* x27 */
}rv64_thrsw_context;

void hal_thread_switch(void **_prev, void **_next);

#endif  /*  ASM_FILE  */
#endif  /* _HAL_HAL_THREAD_H  */

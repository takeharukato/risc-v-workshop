/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  RISC-V 64 mscratch definitions                                    */
/*                                                                    */
/**********************************************************************/
#if !defined(_HAL_RV64_MSCRATCH_H)
#define  _HAL_RV64_MSCRATCH_H

#if !defined(ASM_FILE)

#include <klib/freestanding.h>

/**
   マシンモードエントリ処理情報情報 (mscratchレジスタから参照)
 */
typedef struct _mscratch_info{
	uintptr_t         mstack_sp;  /* マシンモードエントリ時に設定するスタックポインタ値 */
	uintptr_t          saved_sp;  /* マシンモードエントリ時のスタックポインタ保存領域   */
	uint64_t             hartid;  /* hartid                                             */
	uintptr_t    mtimecmp_paddr;  /* CLINT MTIMECMPレジスタの物理アドレス               */
	uintptr_t       mtime_paddr;  /* CLINT MTIMEレジスタの物理アドレス                  */
	uint64_t timer_interval_cyc;  /* タイマ周期 (単位: cycle)                           */
}mscratch_info;
#endif  /* !ASM_FILE */
#endif  /* _HAL_RV64_MSCRATCH_H  */

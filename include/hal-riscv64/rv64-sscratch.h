/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  RISC-V 64 sscratch definitions                                    */
/*                                                                    */
/**********************************************************************/
#if !defined(_HAL_RV64_SSCRATCH_H)
#define  _HAL_RV64_SSCRATCH_H

#if !defined(ASM_FILE)

#include <klib/freestanding.h>
#include <kern/kern-cpuinfo.h>

/**
   スーパバイザモードエントリ処理情報情報 (sscratchレジスタから参照)
 */
typedef struct _sscratch_info{
	cpu_info            *cpuinf;  /* CPU情報                                            */
	uintptr_t         sstack_sp;  /* スーパーバイザエントリ時に設定するスタックポインタ */
	uintptr_t          saved_sp;  /* スーパーバイザエントリ時のスタックポインタ保存域   */
	uintptr_t         istack_sp;  /* 割込みスタック切り替え時に設定するスタックポインタ */
}sscratch_info;
#endif  /* !ASM_FILE */
#endif  /* _HAL_RV64_SSCRATCH_H  */

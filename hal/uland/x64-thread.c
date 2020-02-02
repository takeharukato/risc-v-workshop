/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  X64 specific thread operations                                    */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>
#include <kern/kern-cpuinfo.h>
#include <kern/thr-if.h>

#include <hal/hal-traps.h>
#include <hal/hal-thread.h>

/**
   スレッドコンテキストを作成
   @param[in]  entry  スレッドの開始アドレス
   @param[in]  thr_sp スレッド開始時のユーザランドスタックポインタ
   @param[in]  flags  スレッド属性フラグ
   @param[out] stkp コンテキスト保存先を確保するスタックアドレスを指し示すポインタのアドレス
 */
void
hal_setup_thread_context(entry_addr entry, void *thr_sp, thr_flags flags, void **stkp) {
	trap_context         *trap;
	x64_thrsw_context  *thrctx;

	/* 例外コンテキスト位置 */
	trap = (trap_context *)((void *)*stkp - sizeof(trap_context)); 
	/* スレッドスイッチコンテキスト位置 */
	thrctx = (x64_thrsw_context *)((void *)trap - sizeof(x64_thrsw_context)); 

	/* コンテキスト内のレジスタをゼロ初期化
	 */
	memset(trap, 0, sizeof(trap_context));  
	memset(thrctx, 0, sizeof(x64_thrsw_context));

	/* スレッドスイッチ後の復帰先アドレスに例外出口処理ルーチンを設定 */
	thrctx->rip = (reg_type)x64_return_from_trap; 
	thrctx->rflags = X64_RFLAGS_RESVBITS;  /* 予約ビットを立てる */

	trap->rip = entry; /* 例外復帰後のアドレスにスレッドのエントリアドレスを設定 */
	trap->rflags = X64_RFLAGS_RESVBITS;  /* 予約ビットを立てる */

	*stkp = (void *)thrctx;  /* スレッドスイッチコンテキストを指すようにスタックを更新 */
}

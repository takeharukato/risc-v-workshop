/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  RISC-V64 specific thread operations                               */
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
   @param[out] stkp コンテキスト保存先を確保するスタックアドレスを指し示すポインタのアドレス
 */
void
hal_setup_thread_context(entry_addr entry, void *thr_sp, thr_flags flags, void **stkp) {
	thread_info            *ti;
	trap_context         *trap;
	rv64_thrsw_context *thrctx;
	void                   *sp;

	/* カーネルスタックポインタをスレッド情報アドレスに設定
	 */
	ti = (thread_info *)( ( (void *)*stkp ) + TI_KSTACK_SIZE - sizeof(thread_info) );
	ti_thread_info_init(ti); /* スレッド情報初期化 */

	/* 例外コンテキスト位置 */
	trap = (trap_context *)((void *)ti - sizeof(trap_context)); 
	/* スレッドスイッチコンテキスト位置 */
	thrctx = (rv64_thrsw_context *)((void *)trap - sizeof(rv64_thrsw_context)); 

	/* コンテキスト内のレジスタをゼロ初期化
	 */
	memset(trap, 0, sizeof(trap_context));  
	memset(thrctx, 0, sizeof(rv64_thrsw_context));

	/* スレッドスイッチ後の復帰先アドレスに例外出口処理ルーチンを設定 */
	thrctx->ra = rv64_return_from_trap;

	trap->epc = entry; /* 例外復帰後のアドレスにスレッドのエントリアドレスを設定 */

	/* 例外復帰時のスタックポインタとCPUの状態(sstatus)を設定
	 */
	/* 例外復帰先モードと割込み要因単位での割込み許可フラグをクリア */
	trap->estatus &= ~( SSTATUS_SPP | SSTATUS_SPIE | SSTATUS_UPIE );
	if ( flags & THR_THRFLAGS_USER ) {  /* ユーザスレッドの場合 */

		trap->sp = thr_sp;  /* スタックポインタをユーザスタックに設定 */
		trap->estatus |= SSTATUS_SPIE | SSTATUS_UPIE ; /* ユーザモードに復帰 */
	} else {

		trap->sp = (void *)ti;  /* スタックポインタをカーネルスタックに設定 */
		trap->estatus |= SSTATUS_SPP | SSTATUS_SPIE; /* スーパバイザモードに復帰 */
	}

	*stkp = (void *)thrctx;  /* スレッドスイッチコンテキストを指すようにスタックを更新 */
}
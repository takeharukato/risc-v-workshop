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

#include <hal/hal-traps.h>
#include <hal/hal-thread.h>

/**
   スレッドコンテキストを作成
   @param[out] stkp コンテキスト保存先を確保するスタックアドレスを指し示すポインタのアドレス
 */
void
hal_setup_thread_context(entry_addr entry, void *tsk_sp, thr_flags flags, void **stkp) {
	rv64_thrsw_context *thrctx;
	void                  *stk;
	trap_context         *trap;
	void                   *sp;

	stk = (void *)*stkp;
	trap = (void *)stk - sizeof(trap_context);
	thrctx = (void *)stk - sizeof(trap_context) - sizeof(rv64_thrsw_context);
	memset(trap, 0, sizeof(trap_context));
	memset(thrctx, 0, sizeof(rv64_thrsw_context));

	thrctx->ra = rv64_return_from_trap;

	trap->epc = entry;
	trap->estatus &= ~( SSTATUS_SPP | SSTATUS_SPIE | SSTATUS_UPIE );
	if ( flags & THR_THRFLAGS_USER ) {

		trap->sp = tsk_sp;
		trap->estatus |= SSTATUS_SPIE | SSTATUS_UPIE ;
	} else {

		trap->estatus |= SSTATUS_SPP | SSTATUS_SPIE;
	}
	
}

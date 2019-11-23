/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  RISC-V64 common trap routines                                     */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>

#include <hal/riscv64.h>
#include <hal/hal-traps.h>
void
handle_interrupt(trap_context __unused *ctx, scause_type __unused cause,
    stval_type __unused stval){

	return;
}
void
handle_syscall(trap_context __unused *ctx, scause_type __unused cause,
    stval_type __unused stval){

	return;
}
void
handle_exception(trap_context __unused *ctx, scause_type __unused cause,
    stval_type __unused stval){

	return;
}
/**
   例外共通処理
   @pram[in] ctx   割込みコンテキスト
   @pram[in] cause 割込み要因 (scauseの値)
   @pram[in] stval アドレス例外発生要因アドレス
 */
void
trap_common(trap_context *ctx, scause_type cause, stval_type stval){

	if ( cause & SCAUSE_INTR )
		handle_interrupt(ctx, cause, stval);
	else if ( cause & (SCAUSE_ENVCALL_UMODE|SCAUSE_ENVCALL_SMODE ) )
		handle_syscall(ctx, cause, stval);
	else 
		handle_exception(ctx, cause, stval);
}

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
#include <kern/irq-if.h>

#include <hal/riscv64.h>
#include <hal/hal-traps.h>

/**
   CPU例外名称
 */
static const char *scause_string[]={
	"Instruction address misaligned",
	"Instruction access fault",
	"Illegal instruction",
	"Breakpoint",
	"Reserved(4)",
	"Load access fault",
	"AMO address misaligned",
	"Store/AMO access fault",
	"Environment call",
	"Reserved(9)",
	"Reserved(10)",
	"Reserved(11)",
	"Instruction page fault",
	"Load page fault",
	"Reserved(14)",
	"Store/AMO page fault",
};

/**
   割込みを処理する
   @param[in] ctx   例外/割込みコンテキスト
   @param[in] casue トラップ要因
   @param[in] stval アドレス例外発生要因アドレス
 */
void
handle_interrupt(trap_context *ctx, scause_type __unused cause,
    stval_type __unused stval){
	
	irq_handle_irq(ctx);  /* 割込みを処理する */
	return;
}

/**
   システムコールを処理する
   @param[in] ctx   例外/割込みコンテキスト
   @param[in] casue トラップ要因
   @param[in] stval アドレス例外発生要因アドレス
 */
void
handle_syscall(trap_context __unused *ctx, scause_type __unused cause,
    stval_type __unused stval){

	return;
}

/**
   CPU例外を処理する
   @param[in] ctx   例外/割込みコンテキスト
   @param[in] casue トラップ要因
   @param[in] stval アドレス例外発生要因アドレス
 */
void
handle_exception(trap_context *ctx, scause_type cause, stval_type stval){

	kprintf("cpu exception: cause=0x%qx stval=0x%qx\n",
	    cause, stval);

	if ( ( MCAUSE_IMISALIGN_BIT <= cause ) && ( cause <= MCAUSE_ST_PGFAULT_BIT) )
		kprintf("reason: %s\n", scause_string[cause]);

	kprintf("ra: 0x%qx sp: 0x%qx gp: 0x%qx tp: 0x%qx\n",
	    ctx->ra, ctx->sp, ctx->gp, ctx->tp);
	kprintf("t0: 0x%qx t1: 0x%qx t2: 0x%qx fp: 0x%qx\n",
	    ctx->t0, ctx->t1, ctx->t2, ctx->s0);
	kprintf("s1: 0x%qx a0: 0x%qx a1: 0x%qx a2: 0x%qx\n",
	    ctx->s1, ctx->a0, ctx->a1, ctx->a2);
	kprintf("a3: 0x%qx a4: 0x%qx a5: 0x%qx a6: 0x%qx\n",
	    ctx->a3, ctx->a4, ctx->a5, ctx->a6);
	kprintf("a4: 0x%qx a5: 0x%qx a6: 0x%qx a7: 0x%qx\n",
	    ctx->a4, ctx->a5, ctx->a6, ctx->a7);
	kprintf("s2: 0x%qx s3: 0x%qx s4: 0x%qx s5: 0x%qx\n",
	    ctx->s2, ctx->s3, ctx->s4, ctx->s5);
	kprintf("s6: 0x%qx s7: 0x%qx s8: 0x%qx s9: 0x%qx\n",
	    ctx->s6, ctx->s7, ctx->s8, ctx->s9);
	kprintf("s10: 0x%qx s11: 0x%qx t3: 0x%qx t4: 0x%qx\n",
	    ctx->s10, ctx->s11, ctx->t3, ctx->t4);
	kprintf("t5: 0x%qx t6: 0x%qx sstatus: 0x%qx epc: 0x%qx\n",
	    ctx->t5, ctx->t6, ctx->estatus, ctx->epc);
	while(1);
	return;
}
/**
   例外共通処理
   @param[in] ctx   例外/割込みコンテキスト
   @param[in] cause 割込み要因 (scauseの値)
   @param[in] stval アドレス例外発生要因アドレス
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

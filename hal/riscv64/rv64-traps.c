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
#include <kern/thr-if.h>
#include <klib/stack.h>

#include <hal/riscv64.h>
#include <hal/hal-traps.h>

#define RV64_BACKTRACE_DEPTH (32)

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
   FS/XSの意味
 */
static const char *fsxs_string[]={"Off","Initial","Clean","Dirty"};

/**
   トレース情報を記録する(HALレイヤからのコールバック関数)
   @param[in] depth   呼び出しの深さ
   @param[in] bpref   ベースポインタ
   @param[in] caller  呼び出し元アドレス
   @param[in] next_bp 次のベースポインタ返却領域
   @param[in] argp    spinlockのアドレス
   @retval  0 正常終了
   @retval -ENOSPC 呼び出しネストが深すぎる
 */
static int
_trace_trap(int depth, uintptr_t __unused *bpref, void *caller,
		void __unused *next_bp, void  *argp){
	void **array = (void **)argp;

	if ( depth >= RV64_BACKTRACE_DEPTH )
		return -ENOSPC;  /* 呼び出しネストが深すぎる */

	array[depth] = caller;  /*  呼び出し元を記録する  */

	return 0;
}

/**
   トレース情報を記録する
 */
static void
show_trap_backtrace(void) {
	int                                 i;
	void                              *bp;
	void     *array[RV64_BACKTRACE_DEPTH];

	get_base_pointer(&bp);  /*  現在のベースポインタを取得  */

	/* バックトレース情報をクリア */
	memset(&array[0], 0, sizeof(void *) * RV64_BACKTRACE_DEPTH );

	/* バックトレース情報を埋める */
	hal_backtrace(_trace_trap, bp, (void *)array);

	kprintf("Backtrace:\n");
	for( i = 0; RV64_BACKTRACE_DEPTH > i; ++i) {

		if ( array[i] != NULL )
			kprintf("  [%d] %016qx\n", i, (void *)array[i]);
	}
}

/**
   例外/割込みコンテキストを表示する
   @param[in] ctx   例外/割込みコンテキスト
   @param[in] cause トラップ要因
   @param[in] stval アドレス例外発生要因アドレス
 */
void
rv64_show_trap_context(trap_context *ctx, scause_type cause, stval_type stval){
	thread *cur;

	cur = ti_get_current_thread();
	kprintf("Thread: 0x%016qx Thread ID: %d\n",cur, cur->id);
	ti_show_thread_info(cur->tinfo);
	kprintf("Trap Information:\n");
	kprintf("cause: 0x%016qx epc: 0x%016qx stval: 0x%016qx\n", cause, ctx->epc, stval);
	if ( ( MCAUSE_IMISALIGN_BIT <= cause ) && ( cause <= MCAUSE_ST_PGFAULT_BIT) )
		kprintf("reason: %s\n", scause_string[cause]);
	kprintf("sstatus: 0x%016qx\n", ctx->estatus);
	kprintf("sstatus: [%s|%s|%s|%s|%s|%s|%s|%s]\n",
	    ((ctx->estatus & SSTATUS_SD) ? ("SD") : ("--")),
	    ((ctx->estatus & SSTATUS_MXR) ? ("MXR") : ("---")),
	    ((ctx->estatus & SSTATUS_SUM) ? ("SUM") : ("---")),
	    ((ctx->estatus & SSTATUS_SPP) ? ("SPP") : ("---")),
	    ((ctx->estatus & SSTATUS_SPIE) ? ("SPIE") : ("----")),
	    ((ctx->estatus & SSTATUS_UPIE) ? ("UPIE") : ("----")),
	    ((ctx->estatus & SSTATUS_SIE) ? ("SIE") : ("---")),
	    ((ctx->estatus & SSTATUS_UIE) ? ("UIE") : ("---")) );

	kprintf("sstatus: XS=%-7s FS=%-7s UXL=%d\n",
	    fsxs_string[(ctx->estatus & SSTATUS_XS)],
	    fsxs_string[(ctx->estatus & SSTATUS_FS)],
	    (ctx->estatus & SSTATUS_UXL));

	kprintf(" ra: 0x%016qx  sp: 0x%016qx\n", ctx->ra, ctx->sp);
	kprintf(" t0: 0x%016qx  t1: 0x%016qx t2: 0x%016qx fp: 0x%016qx\n",
	    ctx->t0, ctx->t1, ctx->t2, ctx->s0);
	kprintf(" s1: 0x%016qx  a0: 0x%016qx a1: 0x%016qx a2: 0x%016qx\n",
	    ctx->s1, ctx->a0, ctx->a1, ctx->a2);
	kprintf(" a3: 0x%016qx  a4: 0x%016qx a5: 0x%016qx a6: 0x%016qx\n",
	    ctx->a3, ctx->a4, ctx->a5, ctx->a6);
	kprintf(" a4: 0x%016qx  a5: 0x%016qx a6: 0x%016qx a7: 0x%016qx\n",
	    ctx->a4, ctx->a5, ctx->a6, ctx->a7);
	kprintf(" s2: 0x%016qx  s3: 0x%016qx s4: 0x%016qx s5: 0x%016qx\n",
	    ctx->s2, ctx->s3, ctx->s4, ctx->s5);
	kprintf(" s6: 0x%016qx  s7: 0x%016qx s8: 0x%016qx s9: 0x%016qx\n",
	    ctx->s6, ctx->s7, ctx->s8, ctx->s9);
	kprintf("s10: 0x%016qx s11: 0x%016qx t3: 0x%016qx t4: 0x%016qx\n",
	    ctx->s10, ctx->s11, ctx->t3, ctx->t4);
	kprintf(" t5: 0x%016qx  t6: 0x%016qx\n", ctx->t5, ctx->t6);

	show_trap_backtrace();
}

/**
   割込みを処理する
   @param[in] ctx   例外/割込みコンテキスト
   @param[in] cause トラップ要因
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
   @param[in] cause トラップ要因
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
   @param[in] cause トラップ要因
   @param[in] stval アドレス例外発生要因アドレス
 */
void
handle_exception(trap_context *ctx, scause_type cause, stval_type stval){

	if ( ctx->estatus & SSTATUS_SPP ) {

		kprintf("An exception occurs in kernel.\n");
		rv64_show_trap_context(ctx, cause, stval);
		while(1);
	}
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

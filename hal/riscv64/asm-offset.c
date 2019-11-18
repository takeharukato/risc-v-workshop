/* -*- mode: c; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Offset definitions for assembler routines                         */
/*                                                                    */
/**********************************************************************/

#include <klib/asm-offset-helper.h>
#include <klib/compiler.h>
#include <klib/freestanding.h>

#include <kern/thr-preempt.h>
#include <hal/rv64-mscratch.h>
#include <hal/rv64-traps.h>

int
main(int __unused argc, char __unused *argv[]) {

	DEFINE_VAL(TI_THREAD_INFO_SIZE, sizeof(struct _thread_info));
	OFFSET(TI_MAGIC_OFFSET, _thread_info, magic);
	OFFSET(TI_INTRCNT_OFFSET, _thread_info, intrcnt);
	OFFSET(TI_PREEMPT_OFFSET, _thread_info, preempt);
	OFFSET(TI_FLAGS_OFFSET, _thread_info, flags);
	OFFSET(TI_MDFLAGS_OFFSET, _thread_info, mdflags);
	OFFSET(TI_CPU_OFFSET, _thread_info, cpu);
	OFFSET(TI_KSTACK_OFFSET, _thread_info, kstack);
	OFFSET(TI_THR_OFFSET, _thread_info, thr);

	/*
	 * mscratch情報
	 */
	DEFINE_VAL(MSCRATCH_MSCRATCH_INFO_SIZE, sizeof(struct _mscratch_info));
	OFFSET(MSCRATCH_MSTACK_SP, _mscratch_info, mstack_sp);
	OFFSET(MSCRATCH_SAVED_SP, _mscratch_info, saved_sp);
	OFFSET(MSCRATCH_HARTID, _mscratch_info, hartid);
	OFFSET(MSCRATCH_MTIMECMP_PADDR, _mscratch_info, mtimecmp_paddr);
	OFFSET(MSCRATCH_TIMER_INTERVAL_CYC, _mscratch_info, timer_interval_cyc);

	/*
	 * Trapコンテキスト情報
	 */
	DEFINE_VAL(TRAP_TRAP_CONTEXT_SIZE, sizeof(struct _trap_context));
	OFFSET(TRAP_TRAP_CONTEXT_EPC, _trap_context, epc);
	OFFSET(TRAP_TRAP_CONTEXT_SP, _trap_context, sp);

	return 0;
}

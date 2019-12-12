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

#include <kern/kern-cpuinfo.h>
#include <kern/thr-preempt.h>
#include <hal/rv64-mscratch.h>
#include <hal/rv64-sscratch.h>
#include <hal/hal-traps.h>
#include <hal/hal-thread.h>

int
main(int __unused argc, char __unused *argv[]) {

	/*
	 * スレッド情報
	 */
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
	OFFSET(MSCRATCH_MTIMECMP_PADDR, _mscratch_info, mtimecmp_paddr);
	OFFSET(MSCRATCH_MTIME_PADDR, _mscratch_info, mtime_paddr);
	OFFSET(MSCRATCH_TIMER_INTERVAL, _mscratch_info, timer_interval_cyc);
	OFFSET(MSCRATCH_LAST_TIME_VAL, _mscratch_info, last_time_val);
	OFFSET(MSCRATCH_HARTID, _mscratch_info, hartid);

	/*
	 * sscratch情報
	 */
	DEFINE_VAL(SSCRATCH_SSCRATCH_INFO_SIZE, sizeof(struct _sscratch_info));
	OFFSET(SSCRATCH_SSTACK_SP, _sscratch_info, sstack_sp);
	OFFSET(SSCRATCH_SAVED_SP, _sscratch_info, saved_sp);
	OFFSET(SSCRATCH_ISTACK_SP, _sscratch_info, istack_sp);
	OFFSET(SSCRATCH_HARTID, _sscratch_info, hartid);

	/*
	 * Trapコンテキスト情報
	 */
	DEFINE_VAL(RV64_TRAP_CONTEXT_SIZE, sizeof(struct _trap_context));
	OFFSET(RV64_TRAP_CONTEXT_RA, _trap_context, ra);
	OFFSET(RV64_TRAP_CONTEXT_SP, _trap_context, sp);
	OFFSET(RV64_TRAP_CONTEXT_GP, _trap_context, gp);
	OFFSET(RV64_TRAP_CONTEXT_TP, _trap_context, tp);
	OFFSET(RV64_TRAP_CONTEXT_T0, _trap_context, t0);
	OFFSET(RV64_TRAP_CONTEXT_T1, _trap_context, t1);
	OFFSET(RV64_TRAP_CONTEXT_T2, _trap_context, t2);
	OFFSET(RV64_TRAP_CONTEXT_S0, _trap_context, s0);
	OFFSET(RV64_TRAP_CONTEXT_S1, _trap_context, s1);
	OFFSET(RV64_TRAP_CONTEXT_A0, _trap_context, a0);
	OFFSET(RV64_TRAP_CONTEXT_A1, _trap_context, a1);
	OFFSET(RV64_TRAP_CONTEXT_A2, _trap_context, a2);
	OFFSET(RV64_TRAP_CONTEXT_A3, _trap_context, a3);
	OFFSET(RV64_TRAP_CONTEXT_A4, _trap_context, a4);
	OFFSET(RV64_TRAP_CONTEXT_A5, _trap_context, a5);
	OFFSET(RV64_TRAP_CONTEXT_A6, _trap_context, a6);
	OFFSET(RV64_TRAP_CONTEXT_A7, _trap_context, a7);
	OFFSET(RV64_TRAP_CONTEXT_S2, _trap_context, s2);
	OFFSET(RV64_TRAP_CONTEXT_S3, _trap_context, s3);
	OFFSET(RV64_TRAP_CONTEXT_S4, _trap_context, s4);
	OFFSET(RV64_TRAP_CONTEXT_S5, _trap_context, s5);
	OFFSET(RV64_TRAP_CONTEXT_S6, _trap_context, s6);
	OFFSET(RV64_TRAP_CONTEXT_S7, _trap_context, s7);
	OFFSET(RV64_TRAP_CONTEXT_S8, _trap_context, s8);
	OFFSET(RV64_TRAP_CONTEXT_S9, _trap_context, s9);
	OFFSET(RV64_TRAP_CONTEXT_S10, _trap_context, s10);
	OFFSET(RV64_TRAP_CONTEXT_S11, _trap_context, s11);
	OFFSET(RV64_TRAP_CONTEXT_T3, _trap_context, t3);
	OFFSET(RV64_TRAP_CONTEXT_T4, _trap_context, t4);
	OFFSET(RV64_TRAP_CONTEXT_T5, _trap_context, t5);
	OFFSET(RV64_TRAP_CONTEXT_T6, _trap_context, t6);
	OFFSET(RV64_TRAP_CONTEXT_ESTATUS, _trap_context, estatus);
	OFFSET(RV64_TRAP_CONTEXT_EPC, _trap_context, epc);

	/*
	 * スレッドスイッチコンテキスト情報
	 */
	DEFINE_VAL(RV64_THRSW_CONTEXT_SIZE, sizeof(struct _rv64_thrsw_context));
	OFFSET(RV64_THRSW_CONTEXT_S0, _rv64_thrsw_context, s0);
	OFFSET(RV64_THRSW_CONTEXT_S1, _rv64_thrsw_context, s1);
	OFFSET(RV64_THRSW_CONTEXT_S2, _rv64_thrsw_context, s2);
	OFFSET(RV64_THRSW_CONTEXT_S3, _rv64_thrsw_context, s3);
	OFFSET(RV64_THRSW_CONTEXT_S4, _rv64_thrsw_context, s4);
	OFFSET(RV64_THRSW_CONTEXT_S5, _rv64_thrsw_context, s5);
	OFFSET(RV64_THRSW_CONTEXT_S6, _rv64_thrsw_context, s6);
	OFFSET(RV64_THRSW_CONTEXT_S7, _rv64_thrsw_context, s7);
	OFFSET(RV64_THRSW_CONTEXT_S8, _rv64_thrsw_context, s8);
	OFFSET(RV64_THRSW_CONTEXT_S9, _rv64_thrsw_context, s9);
	OFFSET(RV64_THRSW_CONTEXT_S10, _rv64_thrsw_context, s10);
	OFFSET(RV64_THRSW_CONTEXT_S11, _rv64_thrsw_context, s11);

	return 0;
}

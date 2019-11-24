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

	return 0;
}

/* -*- mode: gas; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  RISC-V64 in kernel Supervisor Binary Interface                    */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>
#include <klib/backtrace.h>
#include <kern/page-macros.h>

#include <hal/rv64-platform.h>
#include <hal/rv64-clint.h>
#include <hal/rv64-sbi.h>
/**
   SBIコールを発行する
   @param[in] arg7 SBIコール番号
   @param[in] arg0 第1引数
   @param[in] arg1 第2引数
   @param[in] arg2 第3引数
   @param[in] arg3 第4引数
   @retval SBIコールの返値
 */
uint64_t
sbi_call(uint64_t arg7, uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3){
	register uintptr_t a0 __asm ("a0") = (uintptr_t)(arg0);
	register uintptr_t a1 __asm ("a1") = (uintptr_t)(arg1);
	register uintptr_t a2 __asm ("a2") = (uintptr_t)(arg2);
	register uintptr_t a3 __asm ("a3") = (uintptr_t)(arg3);
	register uintptr_t a7 __asm ("a7") = (uintptr_t)(arg7);

	__asm__ __volatile__(			\
		"ecall"				\
		:"+r"(a0)			\
		:"r"(a1), "r"(a2), "r" (a3), "r"(a7)	\
		:"memory");

	return a0;
}
/**
 */
void 
ksbi_send_ipi(const unsigned long *hart_mask){
	int i;
	int idx;
	int off;
	volatile uint32_t *reg;  /* TODO:レジスタアクセスマクロを統一 */

	for(i = 0; KC_CPUS_NR > i; ++i) {

		idx = i / ( sizeof(unsigned long)*BITS_PER_BYTE );
		off = i % ( sizeof(unsigned long)*BITS_PER_BYTE );
		if ( hart_mask[idx] & (1 << off ) ) {

			reg = (volatile uint32_t *)RV64_CLINT_MSIP(i);
			*reg = 1;  /* TODO: アクセスマクロ作成 */
		}
	}
}


void __section(".boot.text")
ksbi_handle_sbicall(reg_type __unused arg0, reg_type __unused arg1, 
		reg_type __unused arg2, reg_type __unused arg3, 
		reg_type __unused arg4, reg_type __unused arg5, 
		reg_type __unused arg6, reg_type __unused service){
	
	return;
}

void __section(".boot.text")
ksbi_handle_ipi(void){
	
	return;
}

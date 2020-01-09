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
   プロセッサ間割込みを発行する
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


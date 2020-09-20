/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  RISC-V64 in-kernel Supervisor Binary Interface routines           */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>

#include <klib/backtrace.h>
#include <kern/page-macros.h>
#include <kern/kern-cpuinfo.h>

#include <hal/rv64-platform.h>
#include <hal/rv64-clint.h>
#include <hal/rv64-sbi.h>

/**
   CLINT MSIPレジスタを参照する
   @param[in] _hart IPI送信先hartid
*/
#define msip_reg(_hart) ((volatile uint32_t *)(RV64_CLINT_MSIP((_hart))))
 
/**
   IPI割込み要求を発行する
   @param[in] _hart IPI送信先hartid
 */
#define msip_set_ipi(_hart) do{						\
		*(msip_reg((_hart))) = 0x1;				\
	}while(0)

/**
   プロセッサ間割込みを発行する
   @param[in] hart_mask 割込み送信先CPUのビットマップ
 */
void 
ksbi_send_ipi(const cpu_bitmap *hart_mask){
	cpu_id cpu;

	FOREACH_ONLINE_CPUS(cpu) {  /* すべてのオンラインプロセッサをたどる */

		if ( bitops_isset(cpu, hart_mask) )  /* 宛先CPUに含まれる場合 */
			msip_set_ipi(cpu);  /* IPIを発行する */
	}
}

/**
   Supervisor Binary Interface呼び出しを処理する   
   @param[in] func SBI callの機能番号 (Function ID)
   @param[in] ext  SBI callの拡張番号 (Extension ID)
   @param[in] arg0 SBI callの第1引数
   @param[in] arg1 SBI callの第2引数
   @param[in] arg2 SBI callの第3引数
   @param[in] arg3 SBI callの第4引数
 */
void __section(".boot.text")
ksbi_handle_sbicall(reg_type __unused func, reg_type __unused ext,
    reg_type __unused arg0, reg_type __unused arg1, 
    reg_type __unused arg2, reg_type __unused arg3){
	
	return;
}

/**
   IPI割り込みを処理する
 */
void __section(".boot.text")
ksbi_handle_ipi(void){

	return;
}

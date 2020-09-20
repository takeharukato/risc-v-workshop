/*
 * RISC-V64 cycle counter read
 */

#include <klib/freestanding.h>
#include <kern/kern-common.h>
#include <hal/riscv64.h>

#include <kern/ktest.h>

static ktest_stats tstat_rv64cycle_regs=KTEST_INITIALIZER;

static void
rv64cycle_regs1(struct _ktest_stats *sp, void __unused *arg){
	int i;

	kprintf("Current time1:%qx\n", rv64_read_time());

	for(i=0; 1000 > i; ++i)
		__asm__ __volatile__("nop\n\t");

	kprintf("Current time2:%qx\n", rv64_read_time());
	kprintf("Current cycle:%qx\n", rv64_read_cycle());
	kprintf("Current instret:%qx\n", rv64_read_instret());
	ktest_pass( sp );
}

void
tst_rv64cycle_regs(void){

	ktest_def_test(&tstat_rv64cycle_regs, "rv64cycle_regs1", rv64cycle_regs1, NULL);
	ktest_run(&tstat_rv64cycle_regs);
}


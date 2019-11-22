/*
 * RISC-V64 pgtbl
 */

#include <klib/freestanding.h>
#include <kern/kern-common.h>
#include <kern/page-if.h>
#include <kern/vm.h>

#include <kern/ktest.h>

static ktest_stats tstat_rv64pgtbl=KTEST_INITIALIZER;

static void
rv64pgtbl1(struct _ktest_stats *sp, void __unused *arg){

	if ( true )
		ktest_pass( sp );
	else
		ktest_fail( sp );

}

void
tst_rv64pgtbl(void){

	ktest_def_test(&tstat_rv64pgtbl, "rv64pgtbl1", rv64pgtbl1, NULL);
	ktest_run(&tstat_rv64pgtbl);
}


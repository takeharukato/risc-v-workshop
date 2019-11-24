/*
 * vmmap
 */

#include <klib/freestanding.h>
#include <kern/kern-common.h>
#include <kern/page-if.h>
#include <kern/vm.h>

#include <kern/ktest.h>

#if !defined(CONFIG_HAL)
void prepare_map(void);
#endif  /*  CONFIG_HAL  */

static ktest_stats tstat_vmmap=KTEST_INITIALIZER;

static void
vmmap1(struct _ktest_stats *sp, void __unused *arg){
	int               rc;
	vm_pgtbl         pgt;

#if !defined(CONFIG_HAL)
	prepare_map();
#endif  /*  CONFIG_HAL  */

	rc = pgtbl_alloc_user_pgtbl(&pgt);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );
}

void
tst_vmmap(void){

	ktest_def_test(&tstat_vmmap, "vmmap1", vmmap1, NULL);
	ktest_run(&tstat_vmmap);
}


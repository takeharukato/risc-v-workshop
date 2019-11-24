/*
 * vmmap
 */

#include <klib/freestanding.h>
#include <kern/kern-common.h>
#include <kern/page-if.h>
#include <kern/vm.h>

#include <kern/ktest.h>

#define USER_VMA_ADDR (0x10000)

#if !defined(CONFIG_HAL)
void prepare_map(void);
#endif  /*  CONFIG_HAL  */

static ktest_stats tstat_vmmap=KTEST_INITIALIZER;
void show_page_map(vm_pgtbl pgt, vm_vaddr vaddr, vm_size size);
static void
vmmap1(struct _ktest_stats *sp, void __unused *arg){
	int               rc;
	vm_pgtbl        pgt1;
	vm_pgtbl        pgt2;

#if !defined(CONFIG_HAL)
	prepare_map();
#endif  /*  CONFIG_HAL  */

	rc = pgtbl_alloc_user_pgtbl(&pgt1);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	rc = pgtbl_alloc_user_pgtbl(&pgt2);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	rc = vm_map_userpage(pgt1, USER_VMA_ADDR, VM_PROT_READ|VM_PROT_EXECUTE, VM_FLAGS_USER, 
	    PAGE_SIZE, PAGE_SIZE*2);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	show_page_map(pgt1, USER_VMA_ADDR, PAGE_SIZE*2);

	rc = vm_copy_range(pgt2, pgt1, USER_VMA_ADDR, VM_FLAGS_USER, PAGE_SIZE*2);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	show_page_map(pgt2, USER_VMA_ADDR, PAGE_SIZE*2);

	rc = vm_unmap(pgt2, USER_VMA_ADDR, VM_FLAGS_USER, PAGE_SIZE*2);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	show_page_map(pgt2, USER_VMA_ADDR, PAGE_SIZE*2);

	rc = vm_unmap(pgt1, USER_VMA_ADDR, VM_FLAGS_USER, PAGE_SIZE*2);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	show_page_map(pgt1, USER_VMA_ADDR, PAGE_SIZE*2);

	pgtbl_free_user_pgtbl(pgt2);
	pgtbl_free_user_pgtbl(pgt1);
}

void
tst_vmmap(void){

	ktest_def_test(&tstat_vmmap, "vmmap1", vmmap1, NULL);
	ktest_run(&tstat_vmmap);
}


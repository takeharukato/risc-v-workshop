/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  test routine                                                      */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>
#include <kern/page-if.h>
#include <kern/vm.h>

#include <kern/ktest.h>

static ktest_stats tstat_vmcopy=KTEST_INITIALIZER;

#define TST_VMCOPY_BUFSIZ  (1024)
#define TST_VMCOPY_SIZ     (512)
#define TST_VMCOPY_OFF     (256)
#define TST_VMCOPY_CH      ('a')

static char src_block[PAGE_SIZE*2];
static char dest_block[PAGE_SIZE*2];

static void
vmcopy1(struct _ktest_stats *sp, void __unused *arg){
	void         *src;
	void        *dest;
	size_t         rc;
	vm_pgtbl dest_pgt;
	vm_pgtbl  src_pgt;

	dest_pgt = hal_refer_kernel_pagetable();
	src_pgt = hal_refer_kernel_pagetable();

	dest = &dest_block[0] + TST_VMCOPY_OFF;
	src = &src_block[0] + TST_VMCOPY_OFF;

	memset(dest, 0, TST_VMCOPY_BUFSIZ);
	memset(src, TST_VMCOPY_CH, TST_VMCOPY_BUFSIZ);

	rc = vm_memmove(dest_pgt, dest, src_pgt, src + TST_VMCOPY_OFF, TST_VMCOPY_SIZ);

	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );
}

void
tst_vmcopy(void){

	ktest_def_test(&tstat_vmcopy, "vmcopy1", vmcopy1, NULL);
	ktest_run(&tstat_vmcopy);
}


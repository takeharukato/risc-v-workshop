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
#include <kern/vm-if.h>

#include <kern/ktest.h>

static ktest_stats tstat_vmstrlen=KTEST_INITIALIZER;

#define TST_VMSTRLEN_CH      ('a')
#define TST_VMSTRLEN_OFF     (512)

static char src_block[PAGE_SIZE*2];

static void
vmstrlen1(struct _ktest_stats *sp, void __unused *arg){
	void                            *src;
	size_t                            rc;
	vm_pgtbl                     src_pgt;
	char  *test_string = "vmstrlen test";
	size_t                       exc_len;

	src_pgt = hal_refer_kernel_pagetable();
	src = &src_block[0] + TST_VMSTRLEN_OFF;

	exc_len = strlen(test_string);
	strcpy(src, test_string);

	rc = vm_strlen(src_pgt, src);

	if ( rc == exc_len )
		ktest_pass( sp );
	else
		ktest_fail( sp );
}

void
tst_vmstrlen(void){

	ktest_def_test(&tstat_vmstrlen, "vmstrlen1", vmstrlen1, NULL);
	ktest_run(&tstat_vmstrlen);
}


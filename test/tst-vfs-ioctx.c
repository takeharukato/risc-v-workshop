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

#include <kern/vfs-if.h>

#include <kern/ktest.h>

static ktest_stats tstat_vfs_ioctx=KTEST_INITIALIZER;

static void
vfs_ioctx1(struct _ktest_stats *sp, void __unused *arg){
	int rc;
	vfs_ioctx *ioctx1;

	rc = vfs_ioctx_alloc(NULL, &ioctx1);
	if ( rc == 0 ) {

		ktest_pass( sp );
		vfs_ioctx_free(ioctx1);
	} else
		ktest_fail( sp );
}

void
tst_vfs_ioctx(void){

	ktest_def_test(&tstat_vfs_ioctx, "vfs_ioctx", vfs_ioctx1, NULL);
	ktest_run(&tstat_vfs_ioctx);
}


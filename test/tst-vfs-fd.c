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

static ktest_stats tstat_vfs_fd=KTEST_INITIALIZER;

static void
vfs_fd1(struct _ktest_stats *sp, void __unused *arg){
	int rc;

	rc = 0;
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );
}

void
tst_vfs_fd(void){

	ktest_def_test(&tstat_vfs_fd, "vfs_fd", vfs_fd1, NULL);
	ktest_run(&tstat_vfs_fd);
}


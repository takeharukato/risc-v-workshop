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

#include "tst-vfs-tstfs.h"

static ktest_stats tstat_vfs_mount=KTEST_INITIALIZER;

static void
vfs_mount1(struct _ktest_stats *sp, void __unused *arg){
	int                        rc;
	dev_id                    dev;
	dev_id                  minor;

	tst_vfs_tstfs_init();
	ktest_pass( sp );

	minor = 0;
	dev = (dev_id)TST_VFS_TSTFS_DEV_MAJOR << 32|minor;

	rc = vfs_mount(NULL, "/", dev, NULL);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );


	rc = vfs_unmount_rootfs();  /* rootfsのアンマウント */
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	tst_vfs_tstfs_finalize();
}

void
tst_vfs_mount(void){

	ktest_def_test(&tstat_vfs_mount, "vfs_mount", vfs_mount1, NULL);
	ktest_run(&tstat_vfs_mount);
}

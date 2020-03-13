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

static ktest_stats tstat_vfs_fd=KTEST_INITIALIZER;

static void
vfs_fd1(struct _ktest_stats *sp, void __unused *arg){
	int rc;
	dev_id dev;
	dev_id minor;
	vfs_ioctx *parent_ioctx;
	vfs_ioctx    *cur_ioctx;
	vnode                *v;
	//vfs_open_flags          omode;
	char              fname[1024];

	tst_vfs_tstfs_init();
	ktest_pass( sp );

	minor = 0;
	dev = (dev_id)TST_VFS_TSTFS_DEV_MAJOR << 32|minor;
	
	rc = vfs_mount(NULL, "/", dev, TST_VFS_TSTFS_NAME, NULL);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	/*
	 * I/Oコンテキスト生成
	 */
	rc = vfs_ioctx_alloc(NULL, &parent_ioctx);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/*
	 * I/Oコンテキスト生成(親コンテキストを継承したI/Oコンテキストの解放)
	 */
	rc = vfs_ioctx_alloc(parent_ioctx, &cur_ioctx);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	rc = vfs_path_to_dir_vnode(cur_ioctx, "/", 1, &v, fname, 1024);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	vfs_vnode_ref_dec(v);
	vfs_ioctx_free(cur_ioctx);  /* 親コンテキストを継承したI/Oコンテキストの解放 */
	vfs_ioctx_free(parent_ioctx); /* 親コンテキストの解放 */

	vfs_unmount_rootfs();  /* rootfsのアンマウント */

	vfs_unregister_filesystem(TST_VFS_TSTFS_NAME);  /* ファイルシステムの解放 */
}

void
tst_vfs_fd(void){

	ktest_def_test(&tstat_vfs_fd, "vfs_fd", vfs_fd1, NULL);
	ktest_run(&tstat_vfs_fd);
}


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

#define TST_VFS_FD_FNAMELEN (60)

/*
 * open相当の処理
 */
static int
open_fd(vfs_ioctx *cur_ioctx, const char *path, vfs_open_flags omode, file_descriptor **fpp){
	int                          rc;
	size_t                      len;
	vnode                        *v;
	int                          fd;
	char fname[TST_VFS_FD_FNAMELEN];

	if ( path == NULL )
		return -EINVAL;

	len = strlen(path);
	
	rc = vfs_path_to_dir_vnode(cur_ioctx, (char *)path, len, &v,
	    fname, TST_VFS_FD_FNAMELEN);
	if ( rc != 0 )
		goto error_out;

	rc = vfs_fd_alloc(cur_ioctx, v, omode, &fd, fpp);
	if ( rc != 0 )
		goto unref_out;

	vfs_vnode_ref_dec(v);

	return fd;

unref_out:
	vfs_vnode_ref_dec(v);

error_out:
	return rc;
}

static void
vfs_fd1(struct _ktest_stats *sp, void __unused *arg){
	int                        rc;
	dev_id                    dev;
	dev_id                  minor;
	vfs_ioctx       *parent_ioctx;
	vfs_ioctx          *cur_ioctx;
	int                        fd;
	file_descriptor           *fp;
	file_descriptor     *close_fp;

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

	fd = open_fd(cur_ioctx, "/", VFS_O_RDWR, &fp);
	if ( 0 > fd )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	fd = open_fd(cur_ioctx, "/", VFS_O_RDONLY, &fp);
	if ( fd >= 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/*
	 * close相当の処理
	 */
	rc = vfs_fd_get(cur_ioctx, fd, &close_fp);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	rc = vfs_fd_free(cur_ioctx, close_fp);
	if ( rc == -EBUSY )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	rc = vfs_fd_put(close_fp);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

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


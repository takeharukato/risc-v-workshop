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
	vnode                        *v;
	int                          fd;
	char fname[TST_VFS_FD_FNAMELEN];

	if ( path == NULL )
		return -EINVAL;

	rc = vfs_path_to_dir_vnode(cur_ioctx, (char *)path, &v,
	    fname, TST_VFS_FD_FNAMELEN);
	if ( rc != 0 )
		goto error_out;

	rc = vfs_fd_alloc(cur_ioctx, v, omode, &fd, fpp);
	if ( rc != 0 )
		goto unref_out;

	vfs_vnode_ptr_put(v);

	return fd;

unref_out:
	vfs_vnode_ptr_put(v);

error_out:
	return rc;
}
/*
 * close相当の処理
 */
static int
close_fd(vfs_ioctx *cur_ioctx, int fd){
	int                          rc;
	file_descriptor              *f;

	rc = vfs_fd_get(cur_ioctx, fd, &f);
	if ( rc != 0 )
		goto error_out;

	rc = vfs_fd_remove(cur_ioctx, f);
	kassert( rc == -EBUSY );

	rc = vfs_fd_put(f);
	if ( rc != 0 )
		goto error_out;

	return 0;

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
	int                       fd2;
	file_descriptor          *fp2;
	file_descriptor          *efp;

	tst_vfs_tstfs_init();
	ktest_pass( sp );

	minor = 0;
	dev = (dev_id)TST_VFS_TSTFS_DEV_MAJOR << 32|minor;

	rc = vfs_mount_with_fsname(NULL, "/", dev, TST_VFS_TSTFS_NAME, NULL);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/*
	 * I/Oコンテキストエラー処理
	 */
	rc = vfs_ioctx_alloc(NULL, NULL);
	if ( rc == -EINVAL )
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

	/*
	 * FD獲得処理エラー処理系
	 */
	rc = vfs_fd_get(cur_ioctx, -1, &efp);
	if ( rc == -EBADF )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	rc = vfs_fd_get(cur_ioctx, cur_ioctx->ioc_table_size , &efp);
	if ( rc == -EBADF )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	rc = vfs_fd_get(cur_ioctx,  0, &efp);
	if ( rc == -EBADF )
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

	fd2 = open_fd(cur_ioctx, "/", VFS_O_RDONLY, &fp2);
	if ( fd2 >= 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/*
	 * FDテーブルの伸張 (エラーケース)
	 */
	rc = vfs_ioctx_resize_fd_table(cur_ioctx, 0);
	if ( rc == -EINVAL )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	/*
	 * FDテーブルの伸張 (エラーケース)
	 */
	rc = vfs_ioctx_resize_fd_table(cur_ioctx, VFS_MAX_FD_TABLE_SIZE+1);
	if ( rc == -EINVAL )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	/*
	 * FDテーブルの伸張 (使用中のfdが伸縮対象に含まれるケース)
	 */
	rc = vfs_ioctx_resize_fd_table(cur_ioctx, 1);
	if ( rc == -EBUSY )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/*
	 * FDテーブルの伸張
	 */
	rc = vfs_ioctx_resize_fd_table(cur_ioctx, VFS_MAX_FD_TABLE_SIZE);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/*
	 * FDテーブルの伸縮
	 */
	rc = vfs_ioctx_resize_fd_table(cur_ioctx, VFS_DEFAULT_FD_TABLE_SIZE/2);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	rc = close_fd(cur_ioctx, fd);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	vfs_ioctx_free(cur_ioctx);  /* 親コンテキストを継承したI/Oコンテキストの解放 */
	vfs_ioctx_free(parent_ioctx); /* 親コンテキストの解放 */

	vfs_unmount_rootfs();  /* rootfsのアンマウント */

	tst_vfs_tstfs_finalize();  /* ファイルシステムの解放 */
}

void
tst_vfs_fd(void){

	ktest_def_test(&tstat_vfs_fd, "vfs_fd", vfs_fd1, NULL);
	ktest_run(&tstat_vfs_fd);
}

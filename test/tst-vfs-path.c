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

static ktest_stats tstat_vfs_path=KTEST_INITIALIZER;

char path1[VFS_PATH_MAX+1];
char path2[VFS_PATH_MAX+1];
char path3[VFS_PATH_MAX+1];

static void
vfs_path1(struct _ktest_stats *sp, void __unused *arg){
	int rc;

	strcpy(path1, "/usr/bin");
	memset(path2, 0, VFS_PATH_MAX+1);

	rc = vfs_new_path(path1, path2);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	if ( strcmp(path2, "usr/bin/.") == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	strcpy(path1, "//usr/bin");
	memset(path2, 0, VFS_PATH_MAX+1);
	rc = vfs_new_path(path1, path2);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	if ( strcmp(path2, "usr/bin/.") == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	strcpy(path1, "/usr/bin/");
	memset(path2, 0, VFS_PATH_MAX+1);
	rc = vfs_new_path(path1, path2);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	if ( strcmp(path2, "usr/bin/.") == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	strcpy(path1, "/usr/bin//");
	memset(path2, 0, VFS_PATH_MAX+1);
	rc = vfs_new_path(path1, path2);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	if ( strcmp(path2, "usr/bin/.") == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/*
	 * パスが含まれていない
	 */
	memset(path1, 0, VFS_PATH_MAX+1);
	memset(path2, 0, VFS_PATH_MAX+1);
	rc = vfs_new_path(path1, path2);
	if ( rc == -ENOENT )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/*
	 * パスが含まれていない(NULL)
	 */
	memset(path1, 0, VFS_PATH_MAX+1);
	memset(path2, 0, VFS_PATH_MAX+1);
	rc = vfs_new_path(NULL, path2);
	if ( rc == -ENOENT )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/*
	 * パスが含まれていない(先頭の'/'の消去)
	 */
	memset(path1, 0, VFS_PATH_MAX+1);
	memset(path2, 0, VFS_PATH_MAX+1);
	memset(path1, VFS_PATH_DELIM, VFS_PATH_MAX - 1);
	rc = vfs_new_path(path1, path2);
	if ( rc == -ENOENT )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/*
	 * PATH_MAX
	 */
	memset(path1, 0, VFS_PATH_MAX+1);
	memset(path2, 0, VFS_PATH_MAX+1);
	memset(path1, 'a', VFS_PATH_MAX);
	rc = vfs_new_path(path1, path2);
	if ( rc == -ENAMETOOLONG )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/*
	 * PATH_MAX
	 */
	memset(path1, 0, VFS_PATH_MAX+1);
	memset(path2, 0, VFS_PATH_MAX+1);
	memset(path1, 'a', VFS_PATH_MAX-2);
	rc = vfs_new_path(path1, path2);
	if ( rc == -ENAMETOOLONG )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/*
	 * パス結合
	 */
	memset(path1, 0, VFS_PATH_MAX+1);
	memset(path2, 0, VFS_PATH_MAX+1);
	strcpy(path1, "/usr/bin//");
	strcpy(path2, "/home/user/");
	rc = vfs_cat_paths(path1, path2, path3);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	/*
	 * 片側のパスがない
	 */
	memset(path1, 0, VFS_PATH_MAX+1);
	memset(path2, 0, VFS_PATH_MAX+1);
	strcpy(path2, "/home/user/");
	rc = vfs_cat_paths(NULL, path2, path3);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	/*
	 *  片側のパスがない
	 */
	memset(path1, 0, VFS_PATH_MAX+1);
	memset(path2, 0, VFS_PATH_MAX+1);
	strcpy(path1, "/usr/bin//");
	rc = vfs_cat_paths(path1, NULL, path3);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/*
	 *  両方のパスがない
	 */
	memset(path1, 0, VFS_PATH_MAX+1);
	memset(path2, 0, VFS_PATH_MAX+1);
	rc = vfs_cat_paths(NULL, NULL, path3);
	if ( rc == -ENOENT )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/*
	 * PATH_MAX
	 */
	memset(path1, 0, VFS_PATH_MAX+1);
	memset(path2, 0, VFS_PATH_MAX+1);
	memset(path3, 0, VFS_PATH_MAX+1);
	memset(path1, 'a', VFS_PATH_MAX);
	memset(path2, 'b', VFS_PATH_MAX);
	rc = vfs_cat_paths(path1, path2, path3);
	if ( rc == -ENAMETOOLONG )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	memset(path1, 0, VFS_PATH_MAX+1);
	memset(path2, 0, VFS_PATH_MAX+1);
	memset(path3, 0, VFS_PATH_MAX+1);
	memset(path1, 'a', VFS_PATH_MAX);
	rc = vfs_cat_paths(path1, path2, path3);
	if ( rc == -ENAMETOOLONG )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	memset(path1, 0, VFS_PATH_MAX+1);
	memset(path2, 0, VFS_PATH_MAX+1);
	memset(path3, 0, VFS_PATH_MAX+1);
	memset(path2, 'b', VFS_PATH_MAX);
	rc = vfs_cat_paths(path1, path2, path3);
	if ( rc == -ENAMETOOLONG )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	memset(path1, 0, VFS_PATH_MAX+1);
	memset(path2, 0, VFS_PATH_MAX+1);
	memset(path3, 0, VFS_PATH_MAX+1);
	memset(path1, 'a', VFS_PATH_MAX/2-1);
	memset(path2, 'b', VFS_PATH_MAX/2-1);
	rc = vfs_cat_paths(path1, path2, path3);
	if ( rc == -ENAMETOOLONG )
		ktest_pass( sp );
	else
		ktest_fail( sp );
}

void
tst_vfs_path(void){

	ktest_def_test(&tstat_vfs_path, "vfs_path1", vfs_path1, NULL);
	ktest_run(&tstat_vfs_path);
}


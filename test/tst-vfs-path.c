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

char path1[VFS_PATH_MAX];
char path2[VFS_PATH_MAX];

static void
vfs_path1(struct _ktest_stats *sp, void __unused *arg){
	int rc;

	strcpy(path1, "/usr/bin");
	memset(path2, 0, VFS_PATH_MAX);

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
	memset(path2, 0, VFS_PATH_MAX);

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
	memset(path2, 0, VFS_PATH_MAX);

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
	memset(path2, 0, VFS_PATH_MAX);

	rc = vfs_new_path(path1, path2);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	if ( strcmp(path2, "usr/bin/.") == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	memset(path1, 0, VFS_PATH_MAX);
	memset(path2, 0, VFS_PATH_MAX);
	rc = vfs_new_path(path1, path2);
	if ( rc == -ENOENT )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	memset(path1, 0, VFS_PATH_MAX);
	memset(path2, 0, VFS_PATH_MAX);
	rc = vfs_new_path(NULL, path2);
	if ( rc == -ENOENT )
		ktest_pass( sp );
	else
		ktest_fail( sp );
}

void
tst_vfs_path(void){

	ktest_def_test(&tstat_vfs_path, "vfs_path1", vfs_path1, NULL);
	ktest_run(&tstat_vfs_path);
}


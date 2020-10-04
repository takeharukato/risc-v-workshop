/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Simple file system test routines                                  */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>

#include <kern/vfs-if.h>

#include <fs/simplefs/simplefs.h>

#include <kern/ktest.h>

static ktest_stats tstat_simplefs1=KTEST_INITIALIZER;

/**
   テスト用のI/Oコンテキスト
 */
static struct _simplefs_vfs_ioctx{
	vfs_ioctx       *parent;
	vfs_ioctx          *cur;
}tst_ioctx;

static void
simplefs2(struct _ktest_stats *sp, void __unused *arg){
	int rc;
	int fd;

	rc = vfs_opendir(tst_ioctx.cur, "/", VFS_O_RDONLY, &fd);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );


	rc = vfs_closedir(tst_ioctx.cur, fd);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

}
/**
   単純なファイルシステムのテスト
   @param[in] sp  テスト統計情報
   @param[in] arg テスト引数
 */
static void
simplefs1(struct _ktest_stats *sp, void __unused *arg){
	int rc;

	/* ルートファイルシステムをマウントする */
	rc = vfs_mount_with_fsname(NULL, "/", FS_INVALID_DEVID, SIMPLEFS_FSNAME, NULL);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/* 親I/Oコンテキスト
	 */
	rc = vfs_ioctx_alloc(NULL, &tst_ioctx.parent);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/* 子I/Oコンテキスト
	 */
	rc = vfs_ioctx_alloc(NULL, &tst_ioctx.cur);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	simplefs2(sp, arg);

	vfs_unmount_rootfs();  /* rootfsのアンマウント */
}

void
tst_simplefs(void){

	ktest_def_test(&tstat_simplefs1, "simplefs1", simplefs1, NULL);
	ktest_run(&tstat_simplefs1);
}

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
	int             rc;
	int             fd;
	ssize_t      nread;
	vfs_file_stat   st;
	char     buf[1024];
	vfs_dirent      *d;
	int           bpos;
	char        d_type;

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
	/* 通常ファイル作成
	 */
	memset(&st, 0, sizeof(vfs_file_stat));
	st.st_mode = S_IFREG|S_IRWXU|S_IRWXG|S_IRWXO;
	rc = vfs_create(tst_ioctx.cur, "/file1", &st);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/* 生成したファイルのオープン
	 */
	rc = vfs_open(tst_ioctx.cur, "/file1", VFS_O_RDWR, 0, &fd);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/* 生成したファイルのクローズ
	 */
	rc = vfs_close(tst_ioctx.cur, fd);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/* ディレクトリエントリ情報取得
	 */
	rc = vfs_opendir(tst_ioctx.cur, "/", VFS_O_RDONLY, &fd);
	kassert( rc == 0 );
	rc = vfs_getdents(tst_ioctx.cur, fd, &buf[0], 0, 1024, &nread);
	kprintf("%8s %-10s %s %10s %s\n", "I-num", "type", "reclen", "off", "name");
	for (bpos = 0;  nread > bpos; bpos += d->d_reclen) {

		d = (vfs_dirent *) (buf + bpos);
		kprintf("%8ld  ", d->d_ino);
		d_type = *(buf + bpos + d->d_reclen - 1);
		kprintf("%-10s ", (d_type == DT_REG) ?  "regular" :
		    (d_type == DT_DIR) ?  "directory" :
		    (d_type == DT_FIFO) ? "FIFO" :
		    (d_type == DT_SOCK) ? "socket" :
		    (d_type == DT_LNK) ?  "symlink" :
		    (d_type == DT_BLK) ?  "block dev" :
		    (d_type == DT_CHR) ?  "char dev" : "???");
		kprintf("%4d %10lld  %s\n", d->d_reclen,
		    (long long) d->d_off, d->d_name);
	}

	/* ファイルのアンリンク
	 */
	rc = vfs_unlink(tst_ioctx.cur, "/file1");
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	rc = vfs_getdents(tst_ioctx.cur, fd, &buf[0], 0, 1024, &nread);
	kprintf("%8s %-10s %s %10s %s\n", "I-num", "type", "reclen", "off", "name");
	for (bpos = 0;  nread > bpos; bpos += d->d_reclen) {

		d = (vfs_dirent *) (buf + bpos);
		kprintf("%8ld  ", d->d_ino);
		d_type = *(buf + bpos + d->d_reclen - 1);
		kprintf("%-10s ", (d_type == DT_REG) ?  "regular" :
		    (d_type == DT_DIR) ?  "directory" :
		    (d_type == DT_FIFO) ? "FIFO" :
		    (d_type == DT_SOCK) ? "socket" :
		    (d_type == DT_LNK) ?  "symlink" :
		    (d_type == DT_BLK) ?  "block dev" :
		    (d_type == DT_CHR) ?  "char dev" : "???");
		kprintf("%4d %10lld  %s\n", d->d_reclen,
		    (long long) d->d_off, d->d_name);
	}
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
	rc = vfs_mount_with_fsname(NULL, "/", VFS_VSTAT_INVALID_DEVID, SIMPLEFS_FSNAME, NULL);
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

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

#define BUF_SIZE (1024)

/**
   テスト用のI/Oコンテキスト
 */
static struct _simplefs_vfs_ioctx{
	vfs_ioctx       *parent;
	vfs_ioctx          *cur;
}tst_ioctx;


static void __unused
simplefs2(struct _ktest_stats *sp, void __unused *arg){
	int             rc;
	int             fd;
	ssize_t      nread;
	vfs_file_stat   st;
	char buf[BUF_SIZE];
	vfs_dirent      *d;
	int           bpos;
	char        d_type;
	ssize_t   rw_bytes;
	size_t         len;

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

	/* ファイルの書き込み
	 */
	len = strlen("Hello, VFS\n");
	rc = vfs_write(tst_ioctx.cur, fd, "Hello, VFS\n", len + 1, &rw_bytes);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	if ( rw_bytes == ( len + 1 ) )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/* EOFリード
	 */
	rc = vfs_read(tst_ioctx.cur, fd, &buf[0], BUF_SIZE, &rw_bytes);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	if ( rw_bytes == 0 )  /* EOF */
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/* seek処理
	 */
	rc = vfs_lseek(tst_ioctx.cur, fd, 0, VFS_SEEK_WHENCE_SET);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/* ファイル読み取り
	 */
	rc = vfs_read(tst_ioctx.cur, fd, &buf[0], BUF_SIZE, &rw_bytes);
	if ( rw_bytes == ( len + 1 ) ) {

		ktest_pass( sp );
		kprintf("Read: %s", buf);
	} else
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
	rc = vfs_getdents(tst_ioctx.cur, fd, &buf[0], 0, BUF_SIZE, &nread);
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


	/* ディレクトリ作成
	 */
	rc = vfs_mkdir(tst_ioctx.cur, "/dev");
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/* 単純なファイルシステムをマウントする */
	rc = vfs_mount_with_fsname(tst_ioctx.cur, "/dev", VFS_VSTAT_INVALID_DEVID,
	    SIMPLEFS_FSNAME, NULL);
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
	/* 単純なファイルシステムをアンマウントする */
	rc = vfs_unmount(tst_ioctx.cur, "/dev");
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

	/* ディレクトリ削除
	 */
	rc = vfs_rmdir(tst_ioctx.cur, "/dev");
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

	/* 子I/Oコンテキスト解放 */
	vfs_ioctx_free(tst_ioctx.cur);

	/* 親I/Oコンテキスト解放 */
	vfs_ioctx_free(tst_ioctx.parent);

	rc = vfs_unmount_rootfs();  /* rootfsのアンマウント */
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );
}

void
tst_simplefs(void){

	ktest_def_test(&tstat_simplefs1, "simplefs1", simplefs1, NULL);
	ktest_run(&tstat_simplefs1);
}

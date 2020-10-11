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

static simplefs_ioctl_arg ioctl_arg; /* ioctl引数 */

/**
   ディレクトリエントリの表示
   @param[in] cur I/Oコンテキスト
   @param[in] dir 表示対象ディレクトリ
 */
static void
show_ls(vfs_ioctx *cur, char *dir){
	int             rc;
	int          dirfd;
	file_descriptor *f;
	ssize_t      nread;
	char buf[BUF_SIZE];
	int           bpos;
	char        d_type;
	vfs_dirent      *d;

	/* ディレクトリを開く
	 */
	rc = vfs_opendir(cur, dir, VFS_O_RDONLY, &dirfd);
	kassert( rc == 0 );

	/* カーネルファイルディスクリプタ獲得
	 */
	rc = vfs_fd_get(tst_ioctx.cur, dirfd, &f);

	/* ディレクトリエントリの表示
	 */
	rc = vfs_getdents(tst_ioctx.cur, f, &buf[0], 0, BUF_SIZE, &nread);
	kprintf("%8s %-10s %s %10s %s\n", "I-num", "type", "reclen", "off", "name");

	for (bpos = 0, d = (vfs_dirent *) (buf + bpos);
	     nread > bpos; bpos += d->d_reclen) {

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

	vfs_fd_put(f);  /*  ファイルディスクリプタの参照を解放  */

	/* ディレクトリのクローズ
	 */
	rc = vfs_closedir(tst_ioctx.cur, dirfd);
	kassert( rc == 0 );
}

/**
   リードオンリーファイルシステムのテスト
   @param[in] sp  テスト統計情報
   @param[in] arg 引数
 */
static void __unused
simplefs4(struct _ktest_stats *sp, void __unused *arg){
	int               rc;
	int               fd;

	/* ディレクトリ作成
	 */
	rc = vfs_mkdir(tst_ioctx.cur, "/romnt");
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/* 単純なファイルシステムをマウントする */
	rc = vfs_mount_with_fsname(tst_ioctx.cur, "/romnt", VFS_VSTAT_INVALID_DEVID,
	    SIMPLEFS_FSNAME, NULL);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/* テスト用ファイルを作成する */
	rc = vfs_open(tst_ioctx.cur, "/romnt/file3", VFS_O_CREAT|VFS_O_EXCL|VFS_O_RDWR,
	    S_IFREG|S_IRWXU|S_IRWXG|S_IRWXO, &fd);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	/*
	   TODO: ファイルを未クローズの時にunmountが走行するとv-nodeが解放されて
	   ファイルディスクリプタから改造済みのv-nodeが参照されたままになり
	   不正メモリアクセスになる問題を修正すること
	   以下の#if 1を#if 0にすると再現可能
	 */
#if 1
	/* ファイルをクローズする (openでファイルを作成しているので) */
	rc = vfs_close(tst_ioctx.cur, fd);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );
#endif
	/* テスト用ディレクトリを作成する
	 */
	rc = vfs_mkdir(tst_ioctx.cur, "/romnt/testdir1");
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	kprintf("After setup for readonly mount test ls /\n");
	show_ls(tst_ioctx.cur, "/");

	kprintf("After setup for readonly mount test ls /romnt\n");
	show_ls(tst_ioctx.cur, "/romnt");

	/* 単純なファイルシステムをアンマウントする */
	rc = vfs_unmount(tst_ioctx.cur, "/romnt");
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/* 単純なファイルシステムをリードオンリーマウントする */
	rc = vfs_mount_with_fsname(tst_ioctx.cur, "/romnt", VFS_VSTAT_INVALID_DEVID,
	    SIMPLEFS_FSNAME, (void *)VFS_MNT_RDONLY);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/* ディレクトリエントリ情報取得
	 */
	kprintf("After ro mount ls /\n");
	show_ls(tst_ioctx.cur, "/");

	/* 作成したディレクトリ内のディレクトリエントリ情報取得
	 */
	kprintf("After romount ls /romnt\n");
	show_ls(tst_ioctx.cur, "/romnt");

	/* 通常ファイル作成
	 */
	/* ファイル作成オープン (エラー)
	 */
	rc = vfs_open(tst_ioctx.cur, "/romnt/file4", VFS_O_CREAT|VFS_O_EXCL|VFS_O_RDWR,
	    S_IFREG|S_IRWXU|S_IRWXG|S_IRWXO, &fd);
	if ( rc == -EROFS )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	if ( rc == 0 ) {

		/* 生成したファイルのクローズ
		 */
		rc = vfs_close(tst_ioctx.cur, fd);
		if ( rc == 0 )
			ktest_pass( sp );
		else
			ktest_fail( sp );

		/* ファイルのアンリンク
		 */
		rc = vfs_unlink(tst_ioctx.cur, "/romnt/file4");
		if ( rc == 0 )
			ktest_pass( sp );
		else
			ktest_fail( sp );
	}

	/* ファイルのアンリンク
	 */
	rc = vfs_unlink(tst_ioctx.cur, "/romnt/file3");
	if ( rc == -EROFS )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/* ディレクトリ作成
	 */
	rc = vfs_mkdir(tst_ioctx.cur, "/romnt/testdir2");
	if ( rc == -EROFS )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	if ( rc == 0 ) {

		/* 生成したディレクトリの削除
		 */
		rc = vfs_rmdir(tst_ioctx.cur, "/romnt/testdir2");
		if ( rc == 0 )
			ktest_pass( sp );
		else
			ktest_fail( sp );
	}

	/* ディレクトリ削除
	 */
	rc = vfs_rmdir(tst_ioctx.cur, "/romnt/testdir1");
	if ( rc == -EROFS )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/* ディレクトリエントリ情報取得
	 */
	kprintf("Before unmount ls /romnt\n");
	show_ls(tst_ioctx.cur, "/romnt");

	/* 単純なファイルシステムをアンマウントする */
	rc = vfs_unmount(tst_ioctx.cur, "/romnt");
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/* ディレクトリエントリ情報取得
	 */
	kprintf("After unmount /romnt\n");
	show_ls(tst_ioctx.cur, "/");

	kprintf("Before rmdir /romnt ls /\n");
	show_ls(tst_ioctx.cur, "/");

	/* ディレクトリ削除
	 */
	rc = vfs_rmdir(tst_ioctx.cur, "/romnt");
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/* ディレクトリエントリ情報取得
	 */
	kprintf("After remove /romnt ls /\n");
	show_ls(tst_ioctx.cur, "/");
}

/**
   パラメタテスト
   @param[in] sp  テスト統計情報
   @param[in] arg 引数
 */
static void __unused
simplefs3(struct _ktest_stats *sp, void __unused *arg){
	int               rc;
	int            dirfd;
	int               fd;
	file_descriptor   *f;
	vfs_file_stat     st;
	char   buf[BUF_SIZE];
	ssize_t     rw_bytes;
	ssize_t        nread;

	/* createによるディレクトリ作成 (エラー)
	 */
	vfs_init_attr_helper(&st);
	st.st_mode = S_IFDIR|S_IRWXU|S_IRWXG|S_IRWXO;
	rc = vfs_create(tst_ioctx.cur, "/faildir", &st);
	if ( rc == -EISDIR )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/* openによるディレクトリオープン (エラー)
	 */
	rc = vfs_open(tst_ioctx.cur, "/", VFS_O_DIRECTORY|VFS_O_RDWR, 0, &dirfd);
	if ( rc == -EISDIR )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/* openによるディレクトリオープン (エラー)
	 */
	rc = vfs_open(tst_ioctx.cur, "/", VFS_O_RDONLY, 0, &dirfd);
	if ( rc == -EISDIR )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/* openによるディレクトリオープン
	 */
	rc = vfs_open(tst_ioctx.cur, "/", VFS_O_DIRECTORY|VFS_O_RDONLY, 0, &dirfd);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/* カーネルファイルディスクリプタ獲得
	 */
	rc = vfs_fd_get(tst_ioctx.cur, dirfd, &f);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/* 負のバッファ長チェック (エラー)
	 */
	rc = vfs_getdents(tst_ioctx.cur, f, &buf[0], 0, -1, &nread);
	if ( rc == -EINVAL )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	vfs_fd_put(f);  /*  ファイルディスクリプタの参照を解放  */

	/* closeによるディレクトリクローズ
	 */
	rc = vfs_close(tst_ioctx.cur, dirfd);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );


	/* 通常ファイル作成
	 */

	/* 生成したファイルのオープン
	 */
	rc = vfs_open(tst_ioctx.cur, "/file3", VFS_O_CREAT|VFS_O_EXCL|VFS_O_RDWR,
	    S_IFREG|S_IRWXU|S_IRWXG|S_IRWXO, &fd);
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

	if ( rc == 0 ) {

		/* 生成したファイルのオープン (エラー)
		 */
		rc = vfs_open(tst_ioctx.cur, "/file3", VFS_O_CREAT|VFS_O_EXCL|VFS_O_RDWR,
		    S_IFREG|S_IRWXU|S_IRWXG|S_IRWXO, &fd);
		if ( rc == -EEXIST )
			ktest_pass( sp );
		else
			ktest_fail( sp );

		if ( rc == -EEXIST ) {

			/* 生成したファイルのクローズ(エラー)
			 */
			rc = vfs_close(tst_ioctx.cur, fd);
			if ( rc == -EBADF )
				ktest_pass( sp );
			else
				ktest_fail( sp );

			/* ディレクトリのクローズ(エラー)
			 */
			rc = vfs_closedir(tst_ioctx.cur, fd);
			if ( rc == -EBADF )
				ktest_pass( sp );
			else
				ktest_fail( sp );

			/* カーネルファイルディスクリプタ獲得 (エラー)
			 */
			rc = vfs_fd_get(tst_ioctx.cur, fd, &f);
			if ( rc == -EBADF )
				ktest_pass( sp );
			else
				ktest_fail( sp );

		}
	}

	/* 通常ファイルのディレクトリオープン (エラー)
	 */
	rc = vfs_opendir(tst_ioctx.cur, "/file3", VFS_O_RDONLY, &dirfd);
	if ( rc == -ENOTDIR )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/* ファイルのトランケート+追記オープン
	 * トランケートと追記処理の順序の妥当性確認のため両方を設定
	 */
	rc = vfs_open(tst_ioctx.cur, "/file3", VFS_O_RDWR|VFS_O_TRUNC|VFS_O_APPEND,
	    S_IFREG|S_IRWXU|S_IRWXG|S_IRWXO, &fd);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	/* カーネルファイルディスクリプタ獲得
	 */
	rc = vfs_fd_get(tst_ioctx.cur, fd, &f);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	/* 負の読み込み長チェック (エラー)
	 */
	rc = vfs_read(tst_ioctx.cur, f, &buf[0], -1, &rw_bytes);
	if ( rc == -EINVAL )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	vfs_fd_put(f);  /*  ファイルディスクリプタの参照を解放  */

	/* カーネルファイルディスクリプタ獲得
	 */
	rc = vfs_fd_get(tst_ioctx.cur, fd, &f);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	/* 負の書き込み長チェック (エラー)
	 */
	rc = vfs_write(tst_ioctx.cur, f, &buf[0], -1, &rw_bytes);
	if ( rc == -EINVAL )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	vfs_fd_put(f);  /*  ファイルディスクリプタの参照を解放  */

	/* 生成したファイルのクローズ
	 */
	rc = vfs_close(tst_ioctx.cur, fd);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/* ファイルのアンリンク
	 */
	rc = vfs_unlink(tst_ioctx.cur, "/file3");
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

}

/**
   正常系テスト
   @param[in] sp  テスト統計情報
   @param[in] arg 引数
 */
static void __unused
simplefs2(struct _ktest_stats *sp, void __unused *arg){
	int               rc;
	int               fd;
	file_descriptor   *f;
	int            dirfd;
	ssize_t        nread;
	vfs_file_stat     st;
	char   buf[BUF_SIZE];
	vfs_dirent        *d;
	int             bpos;
	char          d_type;
	ssize_t     rw_bytes;
	size_t           len;

	/* ディレクトリオープン
	 */
	rc = vfs_opendir(tst_ioctx.cur, "/", VFS_O_RDONLY, &dirfd);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/* カーネルファイルディスクリプタ獲得
	 */
	rc = vfs_fd_get(tst_ioctx.cur, dirfd, &f);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	/* ディレクトリエントリ取得
	 */
	rc = vfs_getdents(tst_ioctx.cur, f, &buf[0], 0, BUF_SIZE, &nread);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	kprintf("%8s %-10s %s %10s %s\n", "I-num", "type", "reclen", "off", "name");
	for (bpos = 0, d = (vfs_dirent *) (buf + bpos);
	     nread > bpos; bpos += d->d_reclen) {

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

	vfs_fd_put(f);  /*  ファイルディスクリプタの参照を解放  */

	/* ディレクトリクローズ
	 */
	rc = vfs_closedir(tst_ioctx.cur, dirfd);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/* 通常ファイル作成
	 */
	vfs_init_attr_helper(&st);
	st.st_mode = S_IFREG|S_IRWXU|S_IRWXG|S_IRWXO;
	rc = vfs_create(tst_ioctx.cur, "/file1", &st);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/* ディレクトリエントリ情報取得
	 */
	kprintf("After create\n");
	show_ls(tst_ioctx.cur, "/");

	/* 生成したファイルのオープン
	 */
	rc = vfs_open(tst_ioctx.cur, "/file1", VFS_O_RDWR, 0, &fd);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );


	/* カーネルファイルディスクリプタ獲得
	 */
	rc = vfs_fd_get(tst_ioctx.cur, fd, &f);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/* ファイルの書き込み
	 */
	len = strlen("Hello, VFS\n");
	rc = vfs_write(tst_ioctx.cur, f, "Hello, VFS\n", len + 1, &rw_bytes);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	if ( rw_bytes == ( len + 1 ) )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	vfs_fd_put(f);  /*  ファイルディスクリプタの参照を解放  */


	/* カーネルファイルディスクリプタ獲得
	 */
	rc = vfs_fd_get(tst_ioctx.cur, fd, &f);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	/* バッファ書き戻し
	 */
	rc = vfs_fsync(tst_ioctx.cur, f);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	vfs_fd_put(f);  /*  ファイルディスクリプタの参照を解放  */


	/* カーネルファイルディスクリプタ獲得
	 */
	rc = vfs_fd_get(tst_ioctx.cur, fd, &f);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	/* EOFリード
	 */
	rc = vfs_read(tst_ioctx.cur, f, &buf[0], BUF_SIZE, &rw_bytes);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	vfs_fd_put(f);  /*  ファイルディスクリプタの参照を解放  */

	if ( rw_bytes == 0 )  /* EOF */
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/* カーネルファイルディスクリプタ獲得
	 */
	rc = vfs_fd_get(tst_ioctx.cur, fd, &f);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/* seek処理
	 */
	rc = vfs_lseek(tst_ioctx.cur, f, 0, VFS_SEEK_WHENCE_SET);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	vfs_fd_put(f);  /*  ファイルディスクリプタの参照を解放  */


	/* カーネルファイルディスクリプタ獲得
	 */
	rc = vfs_fd_get(tst_ioctx.cur, fd, &f);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	/* ファイル読み取り
	 */
	rc = vfs_read(tst_ioctx.cur, f, &buf[0], BUF_SIZE, &rw_bytes);
	if ( rw_bytes == ( len + 1 ) ) {

		ktest_pass( sp );
		kprintf("Read: %s", buf);
	} else
		ktest_fail( sp );
	vfs_fd_put(f);  /*  ファイルディスクリプタの参照を解放  */


	/* カーネルファイルディスクリプタ獲得
	 */
	rc = vfs_fd_get(tst_ioctx.cur, fd, &f);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	/* ファイルI-node獲得
	 */
	ioctl_arg.inum = SIMPLEFS_INODE_ROOT_INO;
	rc = vfs_ioctl(tst_ioctx.cur, f, SIMPLEFS_IOCTL_CMD_GETINODE, &ioctl_arg,
	    sizeof(simplefs_ioctl_arg));
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	vfs_fd_put(f);  /*  ファイルディスクリプタの参照を解放  */


	/* 名前の変更
	 */
	rc = vfs_rename(tst_ioctx.cur, "/file1", "/file2");
	if ( rc == 0 )
		ktest_pass( sp );
	 else
		ktest_fail( sp );

	/* ディレクトリエントリ情報取得
	 */
	kprintf("After rename\n");
	show_ls(tst_ioctx.cur, "/");

	/* ファイルのアンリンク
	 */
	rc = vfs_unlink(tst_ioctx.cur, "/file2");
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/* ディレクトリエントリ情報取得
	 */
	kprintf("After unlink\n");
	show_ls(tst_ioctx.cur, "/");

	/* 生成したファイルのクローズ
	 */
	rc = vfs_close(tst_ioctx.cur, fd);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/* ディレクトリエントリ情報取得
	 */
	kprintf("After close\n");
	show_ls(tst_ioctx.cur, "/");

	/* ディレクトリ作成
	 */
	rc = vfs_mkdir(tst_ioctx.cur, "/dev");
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/* ディレクトリエントリ情報取得
	 */
	kprintf("After mkdir ls /\n");
	show_ls(tst_ioctx.cur, "/");

	/* 作成したディレクトリ内のディレクトリエントリ情報取得
	 */
	kprintf("After mkdir ls /dev\n");
	show_ls(tst_ioctx.cur, "/dev");

	/* キャラクタデバイスファイル作成
	 */
	vfs_init_attr_helper(&st);
	st.st_rdev = VFS_VSTAT_MAKEDEV(5,0);
	st.st_mode = S_IFCHR|S_IRWXU|S_IRWXG|S_IRWXO;
	rc = vfs_create(tst_ioctx.cur, "/dev/console", &st);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/* デバイス作成したディレクトリ内のディレクトリエントリ情報取得
	 */
	kprintf("After mknod chardev ls /dev\n");
	show_ls(tst_ioctx.cur, "/dev");

	/* 単純なファイルシステムをマウントする */
	rc = vfs_mount_with_fsname(tst_ioctx.cur, "/dev", VFS_VSTAT_INVALID_DEVID,
	    SIMPLEFS_FSNAME, NULL);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/* ディレクトリエントリ情報取得
	 */
	kprintf("After mount\n");
	show_ls(tst_ioctx.cur, "/");

	/* ブロックデバイスファイル作成
	 */
	vfs_init_attr_helper(&st);
	st.st_rdev = VFS_VSTAT_MAKEDEV(259,0);
	st.st_mode = S_IFBLK|S_IRWXU|S_IRWXG|S_IRWXO;
	rc = vfs_create(tst_ioctx.cur, "/dev/vitioblk0", &st);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/* ブロックデバイスファイル情報取得
	 */
	rc = vfs_open(tst_ioctx.cur, "/dev/vitioblk0", VFS_O_RDWR, 0, &fd);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	if ( rc == 0 ) {

		/* カーネルファイルディスクリプタ獲得
		 */
		rc = vfs_fd_get(tst_ioctx.cur, fd, &f);
		if ( rc == 0 )
			ktest_pass( sp );
		else
			ktest_fail( sp );

		vfs_init_attr_helper(&st);
		rc = vfs_getattr(f->f_vn, VFS_VSTAT_MASK_GETATTR, &st);
		if ( rc == 0 )
			ktest_pass( sp );
		else
			ktest_fail( sp );

		vfs_fd_put(f);  /*  ファイルディスクリプタの参照を解放  */
		rc = vfs_close(tst_ioctx.cur, fd);
		if ( rc == 0 )
			ktest_pass( sp );
		else
			ktest_fail( sp );

		/* ブロックデバイスであることを確認
		 */
		if ( S_ISBLK(st.st_mode) )
			ktest_pass( sp );
		else
			ktest_fail( sp );
	}

	/* デバイス作成したディレクトリ内のディレクトリエントリ情報取得
	 */
	kprintf("After mount mknod blkdev ls /dev\n");
	show_ls(tst_ioctx.cur, "/dev");

	/* 単純なファイルシステムをアンマウントする */
	rc = vfs_unmount(tst_ioctx.cur, "/dev");
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/* ディレクトリエントリ情報取得
	 */
	kprintf("After unmount\n");
	show_ls(tst_ioctx.cur, "/");

	/* ディレクトリエントリ情報取得
	 */
	kprintf("Before unlink console ls /dev\n");
	show_ls(tst_ioctx.cur, "/dev");

	/* デバイスファイルのアンリンク
	 */
	rc = vfs_unlink(tst_ioctx.cur, "/dev/console");
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/* ディレクトリエントリ情報取得
	 */
	kprintf("Before rmdir ls /dev\n");
	show_ls(tst_ioctx.cur, "/dev");

	/* ディレクトリ削除
	 */
	rc = vfs_rmdir(tst_ioctx.cur, "/dev");
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/* ディレクトリエントリ情報取得
	 */
	kprintf("After rmdir\n");
	show_ls(tst_ioctx.cur, "/");
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

	simplefs3(sp, arg);

	simplefs4(sp, arg);

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

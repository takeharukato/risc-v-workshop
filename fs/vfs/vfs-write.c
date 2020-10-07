/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  vfs write operations                                              */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>

#include <kern/vfs-if.h>

/**
   ファイルに書き込む
   @param[in] ioctx   I/Oコンテキスト
   @param[in] fd      ユーザファイルディスクリプタ
   @param[in] buf     書き込みデータを格納したカーネル内アドレス
   @param[in] len     書き込むサイズ(単位:バイト)
   @param[out] wrlenp 書き込んだサイズ(単位:バイト)を返却するアドレス
   @retval  0      正常終了
   @retval -EBADF  正当なユーザファイルディスクリプタでない
   @retval -EIO    I/Oエラー
   @retval -ENOSYS writeをサポートしていない
   @retval -EINVAL lenが負
*/
int
vfs_write(vfs_ioctx *ioctx, int fd, const void *buf, ssize_t len, ssize_t *wrlenp){
	file_descriptor *f;
	int             rc;
	ssize_t   wr_bytes;

	if ( 0 > len )
		return -EINVAL;

	/*
	 * ユーザファイルディスクリプタに対応するファイルディスクリプタを取得
	 */
	rc = vfs_fd_get(ioctx, fd, &f); /* ファイルディスクリプタへの参照を得る */
	if ( rc != 0 ) {

		rc = -EBADF;  /*  不正なユーザファイルディスクリプタを指定した */
		goto error_out;
	}

	if ( ( f->f_flags & VFS_FDFLAGS_RWMASK ) == VFS_FDFLAGS_RDONLY ) {

		rc = -EBADF;  /*  書き込み用にオープンしていない */
		goto put_fd_out;
	}

	kassert(f->f_vn != NULL);
	kassert(f->f_vn->v_mount != NULL);
	kassert(f->f_vn->v_mount->m_fs != NULL);
	kassert( is_valid_fs_calls( f->f_vn->v_mount->m_fs->c_calls ) );

	if ( f->f_vn->v_mount->m_mount_flags & VFS_MNT_RDONLY ) {

		rc = -EROFS;  /* 読み取り専用でマウントされている */
		goto put_fd_out;
	}

	/*
	 * ファイルシステム固有のファイル書込み処理を実行
	 */
	if ( f->f_vn->v_mount->m_fs->c_calls->fs_write == NULL ) {

		rc = -ENOSYS;  /*  ハンドラが定義されていない場合は, -ENOSYSを返却して復帰  */
		goto put_fd_out;
	}

	/* ファイルへの書き込みを実施 */
	wr_bytes = f->f_vn->v_mount->m_fs->c_calls->fs_write(
		f->f_vn->v_mount->m_fs_super,
		f->f_vn->v_id, f->f_vn->v_fs_vnode, f->f_private, buf, f->f_pos, len);
	if ( 0 > wr_bytes ) {

		rc = wr_bytes;  /* エラーコードを返却 */
		goto put_fd_out;
	}

	/* 書込み成功時は書き込んだバイト数に応じて,
	 * ファイルポジションを更新する
	 */
	*wrlenp = wr_bytes;    /* 読み取り長を返却 */

	f->f_pos += wr_bytes; /* ファイルポジションを更新 */

	/* 1バイト以上書き込めた場合は, 最終更新時刻を更新する
	 */
	if ( wr_bytes > 0 ) {

		/* 最終更新時刻を更新する  */
		rc = vfs_time_attr_helper(f->f_vn, NULL, VFS_VSTAT_MASK_MTIME);
		if ( rc != 0 )
			goto put_fd_out;  /* 時刻更新に失敗した */
	}

	vfs_fd_put(f);  /*  ファイルディスクリプタの参照を解放  */

	return 0;

put_fd_out:
	vfs_fd_put(f);  /*  ファイルディスクリプタの参照を解放  */

error_out:
	return rc;
}
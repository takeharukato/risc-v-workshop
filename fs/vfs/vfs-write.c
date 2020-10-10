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
   @param[in] fp      カーネルファイルディスクリプタ
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
vfs_write(vfs_ioctx *ioctx, file_descriptor *fp, const void *buf, ssize_t len,
    ssize_t *wrlenp){
	int             rc;
	ssize_t   wr_bytes;

	if ( 0 > len ) {

		rc = -EINVAL;  /* 書き込み長が負 */
		goto error_out;
	}
	if ( ( fp->f_flags & VFS_FDFLAGS_RWMASK ) == VFS_FDFLAGS_RDONLY ) {

		rc = -EBADF;  /*  書き込み用にオープンしていない */
		goto error_out;
	}

	kassert(fp->f_vn != NULL);
	kassert(fp->f_vn->v_mount != NULL);
	kassert(fp->f_vn->v_mount->m_fs != NULL);
	kassert( is_valid_fs_calls( fp->f_vn->v_mount->m_fs->c_calls ) );

	if ( fp->f_vn->v_mount->m_mount_flags & VFS_MNT_RDONLY ) {

		rc = -EROFS;  /* 読み取り専用でマウントされている */
		goto error_out;
	}

	/*
	 * ファイルシステム固有のファイル書込み処理を実行
	 */
	if ( fp->f_vn->v_mount->m_fs->c_calls->fs_write == NULL ) {

		rc = -ENOSYS;  /*  ハンドラが定義されていない場合は, -ENOSYSを返却して復帰  */
		goto error_out;
	}

	/* ファイルへの書き込みを実施 */
	wr_bytes = fp->f_vn->v_mount->m_fs->c_calls->fs_write(
		fp->f_vn->v_mount->m_fs_super,
		fp->f_vn->v_id, fp->f_vn->v_fs_vnode, fp->f_private, buf, fp->f_pos, len);
	if ( 0 > wr_bytes ) {

		rc = wr_bytes;  /* エラーコードを返却 */
		goto error_out;
	}

	/* 書込み成功時は書き込んだバイト数に応じて,
	 * ファイルポジションを更新する
	 */
	*wrlenp = wr_bytes;    /* 読み取り長を返却 */

	fp->f_pos += wr_bytes; /* ファイルポジションを更新 */

	/* 1バイト以上書き込めた場合は, 最終更新時刻を更新する
	 */
	if ( wr_bytes > 0 ) {

		/* 最終更新時刻を更新する  */
		rc = vfs_time_attr_helper(fp->f_vn, NULL, VFS_VSTAT_MASK_MTIME);
		if ( rc != 0 )
			goto error_out;  /* 時刻更新に失敗した */
	}

	return 0;

error_out:
	return rc;
}

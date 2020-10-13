/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  vfs read operations                                               */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>

#include <kern/vfs-if.h>

/**
   ファイルから読み込む
   @param[in] ioctx   I/Oコンテキスト
   @param[in] fp      カーネルファイルディスクリプタ
   @param[in] buf     データ読み込み先カーネル内アドレス
   @param[in] len     読み込むサイズ(単位:バイト)
   @param[out] rdlenp 読み込んだサイズ(単位:バイト)を返却するアドレス
   @retval  0      正常終了
   @retval -EIO    I/Oエラー
   @retval -ENOSYS readをサポートしていない
   @retval -EINVAL lenが負
*/
int
vfs_read(vfs_ioctx *ioctx, file_descriptor *fp, void *buf, ssize_t len, ssize_t *rdlenp){
	int             rc;
	ssize_t   rd_bytes;

	if ( 0 > len ) {

		rc =  -EINVAL; /* 書き込み長が負 */
		goto error_out;
	}

	kassert(fp->f_vn != NULL);
	kassert(fp->f_vn->v_mount != NULL);
	kassert(fp->f_vn->v_mount->m_fs != NULL);
	kassert( is_valid_fs_calls( fp->f_vn->v_mount->m_fs->c_calls ) );

	/*
	 * ファイルシステム固有のファイル読込み処理を実行
	 */
	if ( fp->f_vn->v_mount->m_fs->c_calls->fs_read == NULL ) {

		rc = -ENOSYS;  /*  ハンドラが定義されていない場合は, -ENOSYSを返却して復帰 */
		goto error_out;
	}

	/* ファイルからの読込みを実施 */
	rd_bytes = fp->f_vn->v_mount->m_fs->c_calls->fs_read(
		fp->f_vn->v_mount->m_fs_super, fp->f_vn->v_id, fp->f_vn->v_fs_vnode,
		buf, fp->f_pos, len, fp->f_private);

	if ( 0 > rd_bytes ) {

		rc = rd_bytes;  /* エラーコードを返却 */
		goto error_out;
	}

	/* 読込み成功時は読み込んだバイト数に応じて,
	 * ファイルポジションを更新する
	 */
	if ( rdlenp != NULL )
		*rdlenp = rd_bytes;    /* 読み取り長を返却 */

	fp->f_pos += rd_bytes; /* ファイルポジションを更新 */

	/* 1バイト以上読み取れた場合で, かつ,
	 * アクセス時刻更新不要指示でオープンしておらず, かつ
	 * アクセス時刻更新不要指示でマウントされていない場合は,
	 * 最終アクセス時刻を更新する
	 */
	if ( ( rd_bytes > 0 )
	     && ( !( fp->f_flags & VFS_FDFLAGS_NOATIME ) )
	    && ( !( fp->f_vn->v_mount->m_mount_flags & VFS_MNT_NOATIME ) ) ) {

		/* 最終アクセス時刻を更新する  */
		rc = vfs_time_attr_helper(fp->f_vn, NULL, VFS_VSTAT_MASK_ATIME);
		if ( rc != 0 )
			goto error_out;  /* 時刻更新に失敗した */
	}

	return 0;

error_out:
	return rc;
}

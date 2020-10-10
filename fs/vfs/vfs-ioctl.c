/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  vfs ioctl operation                                               */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>
#include <kern/page-if.h>

#include <kern/vfs-if.h>

/**
   ファイルに対する制御操作を行う
   @param[in] ioctx I/Oコンテキスト
   @param[in] fp    カーネルファイルディスクリプタ
   @param[in] buf   データ読み込み先カーネル内アドレス
   @param[in] len   バッファサイズ(単位:バイト)
   @retval  0      正常終了
   @retval -ENOTTY ファイルディスクリプタがキャラクタ型デバイスを指していない
 */
int
vfs_ioctl(vfs_ioctx *ioctx, file_descriptor *fp, int op, void *buf, size_t len){
	int             rc;

	kassert(fp->f_vn != NULL);
	kassert(fp->f_vn->v_mount != NULL);
	kassert(fp->f_vn->v_mount->m_fs != NULL);
	kassert( is_valid_fs_calls( fp->f_vn->v_mount->m_fs->c_calls ) );

	/* 疑似ファイルシステムのためにキャラクタ型デバイスを
	 * 指していないエラーについては, ファイルシステム固有の
	 * ファイル制御処理で検出する
	 */

	/*
	 * ファイルシステム固有のファイル制御処理を実行
	 */
	if ( fp->f_vn->v_mount->m_fs->c_calls->fs_ioctl == NULL ) {

		rc = -ENOSYS;  /*  ハンドラが定義されていない場合は, -ENOSYSを返却して復帰 */
		goto error_out;
	}

	/* ファイルの制御処理を実施 */
	rc = fp->f_vn->v_mount->m_fs->c_calls->fs_ioctl(
		fp->f_vn->v_mount->m_fs_super,
		fp->f_vn->v_id, fp->f_vn->v_fs_vnode,
		op, buf, len, fp->f_private);

error_out:
	return rc;
}

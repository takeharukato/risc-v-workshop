/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  vfs getdents operations                                           */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>

#include <kern/vfs-if.h>

/**
   指定されたパスのディレクトリエントリ情報を得る
   @param[in]  ioctx  I/Oコンテキスト
   @param[in]  fp     カーネルファイルディスクリプタ
   @param[in]  buf    データ読み込み先カーネル内アドレス
   @param[in]  off    ディレクトリエントリ読み出しオフセット(単位:バイト)
   @param[in]  buflen ディレクトリエントリ情報書き込み先バッファ長(単位:バイト)
   @param[out] rdlenp 書き込んだディレクトリエントリ情報のサイズ(単位:バイト)返却領域
   @retval     0      正常終了
   @retval    -ENOSYS getdentsをサポートしていない
   @retval    -EINVAL バッファ長に負の値を指定した
   @note       下位のファイルシステムの責任でoffの妥当性を確認する
 */
int
vfs_getdents(vfs_ioctx *ioctx, file_descriptor *fp, void *buf, off_t off,
    ssize_t buflen, ssize_t *rdlenp){
	int             rc;
	ssize_t   rd_bytes;

	if ( 0 > buflen ) {

		rc = -EINVAL; /* バッファ長が負 */
		goto error_out;
	}

	kassert(fp->f_vn != NULL);
	kassert(fp->f_vn->v_mount != NULL);
	kassert(fp->f_vn->v_mount->m_fs != NULL);
	kassert( is_valid_fs_calls( fp->f_vn->v_mount->m_fs->c_calls ) );

	rd_bytes = buflen; /* 読み取り長を設定 */

	if ( fp->f_vn->v_mount->m_fs->c_calls->fs_getdents == NULL ) {

		rc = -ENOSYS; /* ハンドラが定義されていない場合は, -ENOSYSを返却して復帰 */
		goto error_out;
	}

	rc = vfs_vnode_lock(fp->f_vn);  /* v-nodeのロックを獲得 */
	kassert( rc != -ENOENT );
	if ( rc != 0 ) /* イベントを受信したまたはメモリ不足 */
		goto error_out;

	/*
	 * ファイルシステム固有のディレクトリエントリ読込み処理を実行
	 */
	rc = fp->f_vn->v_mount->m_fs->c_calls->fs_getdents(
		fp->f_vn->v_mount->m_fs_super,
		fp->f_vn->v_fs_vnode, buf, off, buflen, &rd_bytes);

	vfs_vnode_unlock(fp->f_vn);  /* v-nodeのロックを解放 */

	if ( rc != 0 )
		goto error_out;  /* エラー復帰する */

	if ( rdlenp != NULL )
		*rdlenp = rd_bytes;  /* 読み取り長を返却 */

	return 0;

error_out:
	return rc;

}

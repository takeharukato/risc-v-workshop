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
   @param[in]  fd     ユーザファイルディスクリプタ
   @param[in]  buf    データ読み込み先カーネル内アドレス
   @param[in]  off    ディレクトリエントリ読み出しオフセット(単位:バイト)
   @param[in]  buflen ディレクトリエントリ情報書き込み先バッファ長(単位:バイト)
   @param[out] rdlenp 書き込んだディレクトリエントリ情報のサイズ(単位:バイト)返却領域
   @retval     0      正常終了
   @retval    -ESRCH  ディレクトリエントリが見つからなかった
   @retval    -ENOENT ゾーンが割り当てられていない
   @retval    -E2BIG  ファイルサイズの上限を超えている
   @retval    -EIO    ページキャッシュアクセスに失敗した
   @retval    -EINVAL 不正なスーパブロックを指定した
 */
int
vfs_getdents(vfs_ioctx *ioctx, int fd, void *buf, off_t off,
    ssize_t buflen, ssize_t *rdlenp){
	file_descriptor *f;
	int             rc;
	ssize_t   rd_bytes;

	if ( 0 > buflen )
		return -EINVAL; /* バッファ長が負 */
		/*
	 * ユーザファイルディスクリプタに対応するファイルディスクリプタを取得
	 */
	rc = vfs_fd_get(ioctx, fd, &f); /* ファイルディスクリプタへの参照を得る */
	if ( rc != 0 ) {

		rc = -EBADF;  /*  不正なユーザファイルディスクリプタを指定した */
		goto error_out;
	}

	kassert(f->f_vn != NULL);
	kassert(f->f_vn->v_mount != NULL);
	kassert(f->f_vn->v_mount->m_fs != NULL);
	kassert( is_valid_fs_calls( f->f_vn->v_mount->m_fs->c_calls ) );

	rc = -ENOSYS;  /*  ハンドラが定義されていない場合は, -ENOSYSを返却して復帰  */

	rd_bytes = buflen; /* 読み取り長を設定 */

	/*
	 * ファイルシステム固有のディレクトリエントリ読込み処理を実行
	 */
	if ( f->f_vn->v_mount->m_fs->c_calls->fs_getdents != NULL ) {

		rc = f->f_vn->v_mount->m_fs->c_calls->fs_getdents(
			f->f_vn->v_mount->m_fs_super,
			f->f_vn->v_fs_vnode, buf, off, buflen, &rd_bytes);
		if ( 0 > rc )
			goto put_fd_out;  /* エラー復帰する */
	}

	if ( rdlenp != NULL )
		*rdlenp = rd_bytes;  /* 読み取り長を返却 */

	vfs_fd_put(f);  /*  ファイルディスクリプタの参照を解放  */

	return 0;

put_fd_out:
	vfs_fd_put(f);  /*  ファイルディスクリプタの参照を解放  */

error_out:
	return rc;

}

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
   @param[in] fd    ユーザファイルディスクリプタ
   @param[in] buf   データ読み込み先カーネル内アドレス
   @param[in] len   バッファサイズ(単位:バイト)
   @retval  0      正常終了
   @retval -EBADF  正当なユーザファイルディスクリプタでない
   @retval -ENOTTY ファイルディスクリプタがキャラクタ型デバイスを指していない
 */
int
vfs_ioctl(vfs_ioctx *ioctx, int fd, int op, void *buf, size_t len){
	int             rc;
	file_descriptor *f;
	vfs_file_stat   st;

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


	vfs_init_attr_helper(&st);  /* ファイル属性情報を初期化する */
	/* ファイル種別を取得する */
	rc = vfs_getattr(f->f_vn, VFS_VSTAT_MASK_MODE_FMT, &st);
	if ( rc != 0 )
		goto put_fd_out;  /* ファイル属性獲得に失敗した */

	if ( !S_ISCHR(st.st_mode) ) {

		rc = -ENOTTY;  /* キャラクタ型デバイスを指していない */
		goto put_fd_out;
	}

	/*
	 * ファイルシステム固有のファイル制御処理を実行
	 */
	if ( f->f_vn->v_mount->m_fs->c_calls->fs_ioctl == NULL ) {

		rc = -ENOSYS;  /*  ハンドラが定義されていない場合は, -ENOSYSを返却して復帰 */
		goto put_fd_out;
	}

	/* ファイルの制御処理を実施 */
	rc = f->f_vn->v_mount->m_fs->c_calls->fs_ioctl(
		f->f_vn->v_mount->m_fs_super,
		f->f_vn->v_id, f->f_vn->v_fs_vnode,
		op, buf, len, f->f_private);

	vfs_fd_put(f);  /*  ファイルディスクリプタの参照を解放  */

	return rc;

put_fd_out:
	vfs_fd_put(f);  /*  ファイルディスクリプタの参照を解放  */

error_out:
	return rc;
}

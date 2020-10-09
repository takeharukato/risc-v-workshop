/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  vfs fsync operation                                               */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>
#include <kern/page-if.h>

#include <kern/vfs-if.h>

/**
   ファイルのデータブロックを書き戻す
   @param[in] ioctx I/Oコンテキスト
   @param[in] fd    ユーザファイルディスクリプタ
   @retval  0      正常終了
   @retval -EBADF  正当なユーザファイルディスクリプタでない
   @retval -ENOSYS fsyncをサポートしていない
 */
int
vfs_fsync(vfs_ioctx *ioctx, int fd){
	int             rc;
	file_descriptor *f;

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

	/* ファイルシステム固有のデータブロック書き戻し処理を実施
	 * ファイルシステム固有のデータブロック書き戻し処理がない場合は正常終了する
	 */
	rc = vfs_vnode_fsync(f->f_vn);

	vfs_fd_put(f);  /*  ファイルディスクリプタの参照を解放  */

error_out:
	return rc;
}

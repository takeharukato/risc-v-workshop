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
   ファイルのデータブロック/管理情報を書き戻す
   @param[in] ioctx I/Oコンテキスト
   @param[in] fp    カーネルファイルディスクリプタ
   @retval  0      正常終了
   @retval -EBADF  正当なユーザファイルディスクリプタでない
   @retval -ENOSYS fsyncをサポートしていない
 */
int
vfs_fsync(vfs_ioctx *ioctx, file_descriptor *fp){
	int             rc;

	kassert(fp->f_vn != NULL);
	kassert(fp->f_vn->v_mount != NULL);
	kassert(fp->f_vn->v_mount->m_fs != NULL);
	kassert( is_valid_fs_calls( fp->f_vn->v_mount->m_fs->c_calls ) );

	/* ファイルシステム固有のファイル書き戻し処理を実施
	 * ファイルシステム固有のファイル書き戻し処理がない場合は正常終了する
	 */
	rc = vfs_vnode_fsync(fp->f_vn);

	return rc;
}

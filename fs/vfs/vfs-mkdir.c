/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  vfs mkdir operation                                               */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>
#include <kern/page-if.h>

#include <kern/vfs-if.h>

/**
   ディレクトリを作成する
   @param[in] ioctx I/Oコンテキスト
   @param[in] path  作成するディレクトリのファイルパス
   @retval  0      正常終了
   @retval -EBADF  正当なユーザファイルディスクリプタでない
   @retval -EIO    I/Oエラー
   @retval -ENOMEM メモリ不足
   @retval -ENOSYS mkdirをサポートしていない
 */
int
vfs_mkdir(vfs_ioctx *ioctx, char *path){
	int                rc;
	vnode              *v;
	char        *pathname;
	char        *filename;
	vfs_vnode_id new_vnid;
	size_t       name_len;
	size_t       path_len;

	/*
	 * パス(ディレクトリ)検索時に使用する一時領域を確保
	 */
	name_len = strlen(path);
	filename = kmalloc(name_len + 1, KMALLOC_NORMAL);
	if ( filename == NULL ) {

		rc = -ENOMEM;
		goto error_out;
	}

	path_len = strlen(path);
	pathname = kstrdup(path);
	if ( pathname == NULL ) {

		rc = -ENOMEM;
		goto free_filename_out;
	}

	/*
	 * パス(ディレクトリ)検索
	 */
	rc = vfs_path_to_dir_vnode(ioctx, pathname, path_len + 1, &v, filename, name_len + 1);
	if (rc != 0)
		goto free_pathname_out;

	kassert(v != NULL);
	kassert(v->v_mount != NULL);
	kassert(v->v_mount->m_fs != NULL);
	kassert( is_valid_fs_calls( v->v_mount->m_fs->c_calls ) );

	if ( v->v_mount->m_fs->c_calls->fs_mkdir == NULL ) {

		/* ファイルシステム固有なディレクトリ作成処理がなければ,
		 * -ENOSYSを返却して復帰
		 */
		rc = -ENOSYS;
		goto put_vnode_out;
	}

	/*
	 * ファイルシステム固有なディレクトリ作成処理を実施
	 */
	/* TODO: ディレクトリ生成ユーザ/グループを設定 */
	rc = v->v_mount->m_fs->c_calls->fs_mkdir(v->v_mount->m_fs_super,
	    v->v_id, v->v_fs_vnode, filename, &new_vnid);
	if ( ( rc != 0 ) && ( rc != -EIO ) && ( rc != -ENOSYS ) )
		rc = -EIO;  /*  エラーコードを補正  */

put_vnode_out:
	vfs_vnode_ptr_put(v);  /*  パス検索時に取得したvnodeへの参照を解放  */

free_pathname_out:
	kfree(pathname);  /*  パス(ディレクトリ)検索時に使用した一時領域を解放  */

free_filename_out:
	kfree(filename);  /*  パス(ディレクトリ)検索時に使用した一時領域を解放  */

error_out:
	return rc;
}

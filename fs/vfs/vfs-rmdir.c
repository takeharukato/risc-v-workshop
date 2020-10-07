/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  vfs rmdir operation                                               */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>
#include <kern/page-if.h>

#include <kern/vfs-if.h>

/**
   ディレクトリを削除する
   @param[in] ioctx I/Oコンテキスト
   @param[in] path  削除するディレクトリのファイルパス
   @retval  0      正常終了
   @retval -EBADF  正当なユーザファイルディスクリプタでない
   @retval -EIO    I/Oエラー
   @retval -ENOMEM メモリ不足
   @retval -ENOENT ディレクトリが見つからなかった
   @retval -ENOSYS mkdirをサポートしていない
 */
int
vfs_rmdir(vfs_ioctx *ioctx, char *path){
	int                rc;
	int       getattr_res;
	vnode              *v;
	vnode          *dir_v;
	char        *pathname;
	char        *filename;
	size_t       name_len;
	size_t       path_len;
	vfs_file_stat      st;

	rc = vfs_path_to_vnode(ioctx, path, &v);  /* 削除対象ディレクトリのv-nodeを取得 */
	if ( rc != 0 )
		goto error_out;  /* ディレクトリが見つからなかった */

	/* ファイル種別を確認
	 */
	getattr_res = vfs_getattr(v, VFS_VSTAT_MASK_MODE_FMT, &st);
	if ( getattr_res != 0 ) {

		rc = -ENOENT;    /* 対象ディレクトリの属性情報が獲得できなかった */
		goto put_target_dir_vnode;
	}

	if ( !S_ISDIR(st.st_mode) ) {

		rc = -ENOTDIR;         /*  ディレクトリ以外を削除しようとした */
		goto put_target_dir_vnode;
	}

	/*
	 * パス(ディレクトリ)検索時に使用する一時領域を確保
	 */
	name_len = strlen(path);
	filename = kmalloc(name_len + 1, KMALLOC_NORMAL);
	if ( filename == NULL ) {

		rc = -ENOMEM;
		goto put_target_dir_vnode;
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
	rc = vfs_path_to_dir_vnode(ioctx, pathname, path_len + 1, &dir_v, filename, name_len + 1);
	if (rc != 0)
		goto free_pathname_out;

	kassert(dir_v != NULL);
	kassert(dir_v->v_mount != NULL);
	kassert(dir_v->v_mount->m_fs != NULL);
	kassert( is_valid_fs_calls( dir_v->v_mount->m_fs->c_calls ) );

	rc = -ENOSYS;

	/*
	 * ファイルシステム固有なディレクトリ削除処理を実施
	 */
	if ( dir_v->v_mount->m_fs->c_calls->fs_rmdir == NULL )
		goto put_dir_vnode;

	rc = dir_v->v_mount->m_fs->c_calls->fs_rmdir(dir_v->v_mount->m_fs_super,
						     dir_v->v_id, dir_v->v_fs_vnode, filename);
	if ( ( rc != 0 ) && ( rc != -EIO ) && ( rc != -ENOSYS ) ) {

		rc = -EIO;  /*  エラーコードを補正  */
		goto put_dir_vnode;
	}

	vfs_vnode_ptr_put(dir_v);  /*  パス検索時に取得したvnodeへの参照を解放  */

	vfs_vnode_ptr_put(v);  /*  削除対象ディレクトリのv-nodeへの参照を解放  */

	return 0;

put_dir_vnode:
	vfs_vnode_ptr_put(dir_v);  /*  パス検索時に取得したvnodeへの参照を解放  */

free_pathname_out:
	kfree(pathname);  /*  パス(ディレクトリ)検索時に使用した一時領域を解放  */

free_filename_out:
	kfree(filename);  /*  パス(ディレクトリ)検索時に使用した一時領域を解放  */

put_target_dir_vnode:
	vfs_vnode_ptr_put(v);  /*  削除対象ディレクトリのv-nodeへの参照を解放  */

error_out:
	return rc;
}

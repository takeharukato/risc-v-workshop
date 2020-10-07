/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  vfs unlink operation                                              */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>
#include <kern/page-if.h>

#include <kern/vfs-if.h>

/**
   ファイルを削除する (ファイルのリンク数を減算する)
   @param[in] ioctx I/Oコンテキスト
   @param[in] path  ファイルパス
   @param[in] stat  ファイル作成時の属性情報 (ファイル種別/デバイス番号など)
   @retval  0      正常終了
   @retval -EBADF  正当なユーザファイルディスクリプタでない
   @retval -EIO    I/Oエラー
   @retval -ENOMEM メモリ不足
   @retval -ENOSYS unlinkをサポートしていない
   @retval -EISDIR ディレクトリを削除しようとした
 */
int
vfs_unlink(vfs_ioctx *ioctx, char *path){
	int                   rc;
	vnode            *file_v;
	vfs_file_stat         st;
	vnode             *dir_v;
	char           *pathname;
	char           *filename;
	size_t          name_len;
	size_t          path_len;

	/* 操作対象ファイルのv-nodeを得る
	 */
	rc = vfs_path_to_vnode(ioctx, path, &file_v);
	if ( rc != 0 )
		goto error_out;

	/*  操作対象ファイルの属性情報を得る
	 */
	vfs_init_attr_helper(&st);
	rc = vfs_getattr(file_v, VFS_VSTAT_MASK_MODE_FMT, &st);
	if ( rc != 0 )
		goto filev_put_out;

	if ( S_ISDIR(st.st_mode) ) {

		rc = -EISDIR;  /* ディレクトリを削除しようとした */
		goto filev_put_out;
	}

	/* パス(ファイル名)検索時に使用する一時領域を確保
	 */
	name_len = strlen(path);
	filename = kmalloc(name_len + 1, KMALLOC_NORMAL);
	if ( filename == NULL ) {

		rc = -ENOMEM;
		goto filev_put_out;
	}

	/* パス(ディレクトリ)検索時に使用する一時領域を確保
	 */
	path_len = strlen(path);
	pathname = kstrdup(path);
	if ( pathname == NULL ) {

		rc = -ENOMEM;
		goto free_filename_out;
	}

	/*
	 * パス(ディレクトリ)検索
	 */
	rc = vfs_path_to_dir_vnode(ioctx, pathname, path_len + 1, &dir_v,
	    filename, name_len + 1);
	if (rc != 0)
		goto free_pathname_out;

	kassert(dir_v != NULL);
	kassert(dir_v->v_mount != NULL);
	kassert(dir_v->v_mount->m_fs != NULL);
	kassert( is_valid_fs_calls( dir_v->v_mount->m_fs->c_calls ) );

	if ( dir_v->v_mount->m_fs->c_calls->fs_unlink == NULL ) {

		/* ファイルシステム固有なアンリンク処理がない場合は,
		 * -ENOSYSを返却して復帰する
		 */
		rc = -ENOSYS;
		goto dirv_put_out;
	}

	/*
	 * ファイルシステム固有なアンリンク処理を実施
	 */
	/* TODO: アクセス権確認 */
	rc = dir_v->v_mount->m_fs->c_calls->fs_unlink(dir_v->v_mount->m_fs_super,
	    dir_v->v_id, dir_v->v_fs_vnode, filename);

	/* ファイルの削除に成功した場合は, v-nodeの削除フラグを設定
	 */
	if ( rc == 0 )
		vfs_vnode_ptr_remove(file_v);
	else if ( ( rc != -EIO ) && ( rc != -ENOSYS ) && ( rc != -EISDIR ) )
		rc = -EIO;  /*  エラーコードを補正  */

	/*
	 * v-node/一時領域を解放
	 */
dirv_put_out:
	vfs_vnode_ptr_put(dir_v);  /* パス検索時に取得したディレクトリv-nodeの参照を解放 */

free_pathname_out:
	kfree(pathname);  /*  パス(ディレクトリ)検索時に使用した一時領域を解放  */

free_filename_out:
	kfree(filename);  /*  パス(ファイル名)検索時に使用した一時領域を解放  */

filev_put_out:
	vfs_vnode_ptr_put(file_v);  /*  削除対象ファイルへのvnodeの参照を解放  */

error_out:
	return rc;
}

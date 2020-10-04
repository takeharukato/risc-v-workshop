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
	int                rc;
	vnode              *v;
	char        *pathname;
	char        *filename;
	size_t       name_len;
	size_t       path_len;

	/* パス(ファイル名)検索時に使用する一時領域を確保
	 */
	name_len = strlen(path);
	filename = kmalloc(name_len + 1, KMALLOC_NORMAL);
	if ( filename == NULL ) {

		rc = -ENOMEM;
		goto error_out;
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
	rc = vfs_path_to_dir_vnode(ioctx, pathname, path_len + 1, &v, filename, name_len + 1);
	if (rc != 0)
		goto free_pathname_out;

	kassert(v != NULL);
	kassert(v->v_mount != NULL);
	kassert(v->v_mount->m_fs != NULL);
	kassert( is_valid_fs_calls( v->v_mount->m_fs->c_calls ) );

	rc = -ENOSYS;

	/*
	 * ファイルシステム固有なアンリンク処理を実施
	 */
	if ( v->v_mount->m_fs->c_calls->fs_unlink != NULL ) {

		/* TODO: アクセス権確認 */
		rc = v->v_mount->m_fs->c_calls->fs_unlink(v->v_mount->m_fs_super, 
		    v->v_id, v->v_fs_vnode, filename);
		if ( ( rc != 0 ) && ( rc != -EIO ) && ( rc != -ENOSYS ) && ( rc != -EISDIR ) )
			rc = -EIO;  /*  エラーコードを補正  */
	}
	
	/* TODO: ファイルの属性値を調査し, リンク数が0になっていたら
	 * v-nodeの削除フラグを設定
	 */
	vfs_vnode_ptr_put(v);  /*  パス検索時に取得したvnodeへの参照を解放  */

free_pathname_out:
	kfree(pathname);  /*  パス(ディレクトリ)検索時に使用した一時領域を解放  */

free_filename_out:
	kfree(filename);  /*  パス(ファイル名)検索時に使用した一時領域を解放  */

error_out:
	return rc;
}

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
   @retval  0      正常終了
   @retval -EBADF  正当なユーザファイルディスクリプタでない
   @retval -EIO    I/Oエラー
   @retval -ENOMEM メモリ不足
   @retval -ENOSYS unlinkをサポートしていない
   @retval -EISDIR ディレクトリを削除しようとした
   @retval -EROFS  読み取り専用でマウントされている
 */
int
vfs_unlink(vfs_ioctx *ioctx, char *path){
	int                   rc;
	vnode            *file_v;
	vnode             *dir_v;
	char           *filename;
	size_t          name_len;

	/* 操作対象ファイルのv-nodeを得る
	 */
	rc = vfs_path_to_vnode(ioctx, path, &file_v);
	if ( rc != 0 )
		goto error_out;

	if ( S_ISDIR(file_v->v_mode) ) {

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

	/*
	 * パス(ディレクトリ)検索
	 */
	rc = vfs_path_to_dir_vnode(ioctx, path, &dir_v,
	    filename, name_len + 1);
	if (rc != 0)
		goto free_filename_out;

	kassert(dir_v != NULL);
	kassert(dir_v->v_mount != NULL);
	kassert(dir_v->v_mount->m_fs != NULL);
	kassert( is_valid_fs_calls( dir_v->v_mount->m_fs->c_calls ) );

	if ( dir_v->v_mount->m_mount_flags & VFS_MNT_RDONLY ) {

		rc = -EROFS;  /* 読み取り専用でマウントされている */
		goto dir_v_put_out;
	}

	if ( dir_v->v_mount->m_fs->c_calls->fs_unlink == NULL ) {

		/* ファイルシステム固有なアンリンク処理がない場合は,
		 * -ENOSYSを返却して復帰する
		 */
		rc = -ENOSYS;
		goto dir_v_put_out;
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
	else if ( ( rc != -EIO ) && ( rc != -ENOSYS ) && ( rc != -EISDIR ) ) {

		rc = -EIO;  /*  エラーコードを補正  */
		goto dir_v_put_out;
	}

	/*
	 * v-node/一時領域を解放
	 */

	vfs_vnode_ptr_put(dir_v);  /* パス検索時に取得したディレクトリv-nodeの参照を解放 */
	kfree(filename);  /*  パス(ファイル名)検索時に使用した一時領域を解放  */
	vfs_vnode_ptr_put(file_v);  /*  削除対象ファイルへのvnodeの参照を解放  */

	return 0;

dir_v_put_out:
	vfs_vnode_ptr_put(dir_v);  /* パス検索時に取得したディレクトリv-nodeの参照を解放 */

free_filename_out:
	kfree(filename);  /*  パス(ファイル名)検索時に使用した一時領域を解放  */

filev_put_out:
	vfs_vnode_ptr_put(file_v);  /*  削除対象ファイルへのvnodeの参照を解放  */

error_out:
	return rc;
}

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
   ファイル名を変更する
   @param[in] ioctx     I/Oコンテキスト
   @param[in] old_path  変更前のパス
   @param[in] new_path  変更後のパス
   @retval  0      正常終了
   @retval -EINTR  イベントを受信した
   @retval -EIO    I/Oエラー
   @retval -ENOMEM メモリ不足
   @retval -ENOSYS renameをサポートしていない
   @retval -EROFS  読み取り専用ファイルシステムを更新しようとした
   @retval -EXDEV  同じマウントポイント内にない
 */
int
vfs_rename(vfs_ioctx *ioctx, char *old_path, char *new_path){
	int                       rc;
	vnode             *old_dir_v;
	vnode             *new_dir_v;
	char           *old_filename;
	size_t      old_filename_len;
	char           *new_filename;
	size_t      new_filename_len;
	int                   cmpval;

	/* 変更前のパス格納に必要なサイズを算出 */
	old_filename_len = strlen(old_path);

	/* 変更前のパス(ファイル名)検索時に使用する一時領域を確保
	 */
	old_filename = kmalloc(old_filename_len + 1, KMALLOC_NORMAL);
	if ( old_filename == NULL ) {

		rc = -ENOMEM;  /* メモリ不足 */
		goto error_out;
	}

	/* 変更後のパス格納に必要なサイズを算出 */
	new_filename_len = strlen(new_path);

	/* 変更後のパス(ファイル名)検索時に使用する一時領域を確保
	 */
	new_filename = kmalloc(new_filename_len + 1, KMALLOC_NORMAL);
	if ( new_filename == NULL ) {

		rc = -ENOMEM;  /* メモリ不足 */
		goto free_old_filename_out;
	}

	/* 変更前のパスのディレクトリのv-nodeへの参照を獲得
	 */
	rc = vfs_path_to_dir_vnode(ioctx, old_path, &old_dir_v,
	    old_filename, old_filename_len + 1);
	if ( rc != 0 )
		goto free_new_filename_out;

	/* 変更後のパスのディレクトリのv-nodeへの参照を獲得
	 */
	rc = vfs_path_to_dir_vnode(ioctx, new_path, &new_dir_v,
	    new_filename, new_filename_len + 1);
	if ( rc != 0 )
		goto old_dir_v_put_out;

	if ( vfs_vnode_mnt_cmp(old_dir_v, new_dir_v) != 0 ) {

		rc = -EXDEV;  /* 同じマウントポイント内にない */
		goto new_dir_v_put_out;
	}

	kassert(old_dir_v != NULL);
	kassert(old_dir_v->v_mount != NULL);
	kassert(old_dir_v->v_mount->m_fs != NULL);
	kassert( is_valid_fs_calls( old_dir_v->v_mount->m_fs->c_calls ) );

	if ( old_dir_v->v_mount->m_mount_flags & VFS_MNT_RDONLY ) {

		rc = -EROFS;  /* 読み取り専用でマウントされている */
		goto new_dir_v_put_out;
	}

	if ( old_dir_v->v_mount->m_fs->c_calls->fs_rename == NULL ) {

		/* ファイルシステム固有なリネーム処理がない場合は,
		 * -ENOSYSを返却して復帰する
		 */
		rc = -ENOSYS;
		goto new_dir_v_put_out;
	}

	/*
	 * ディレクトリのv-nodeを小さい順にロックする
	 */
	cmpval = vfs_vnode_cmp(old_dir_v, new_dir_v);
	if ( 0 > cmpval ) {

		/* old_dir_vの方がnew_dir_vより小さい */
		rc = vfs_vnode_lock(old_dir_v);
		kassert( rc != -ENOENT );
		if ( rc != 0 )
			goto new_dir_v_put_out; /* イベントを受信したまたはメモリ不足 */

		rc = vfs_vnode_lock(new_dir_v);
		kassert( rc != -ENOENT );
		if ( rc != 0 ) {

			vfs_vnode_unlock(old_dir_v);
			goto new_dir_v_put_out; /* イベントを受信したまたはメモリ不足 */
		}

		/* ファイルシステム固有のrename処理を呼び出す
		 */
		rc = old_dir_v->v_mount->m_fs->c_calls->fs_rename(
			old_dir_v->v_mount->m_fs_super,
			old_dir_v->v_id, old_dir_v->v_fs_vnode, old_filename,
			new_dir_v->v_id, new_dir_v->v_fs_vnode, new_filename);

		vfs_vnode_unlock(new_dir_v);
		vfs_vnode_unlock(old_dir_v);
	} else if ( cmpval > 0 ) {

		/* old_dir_vの方がnew_dir_vより大きい */
		rc = vfs_vnode_lock(new_dir_v);
		kassert( rc != -ENOENT );
		if ( rc != 0 )
			goto new_dir_v_put_out; /* イベントを受信したまたはメモリ不足 */

		rc = vfs_vnode_lock(old_dir_v);
		kassert( rc != -ENOENT );
		if ( rc != 0 ) {

			vfs_vnode_unlock(new_dir_v);
			goto new_dir_v_put_out; /* イベントを受信したまたはメモリ不足 */
		}

		/* ファイルシステム固有のrename処理を呼び出す
		 */
		rc = old_dir_v->v_mount->m_fs->c_calls->fs_rename(
			old_dir_v->v_mount->m_fs_super,
			old_dir_v->v_id, old_dir_v->v_fs_vnode, old_filename,
			new_dir_v->v_id, new_dir_v->v_fs_vnode, new_filename);

		vfs_vnode_unlock(old_dir_v);
		vfs_vnode_unlock(new_dir_v);
	} else { /* 同一ディレクトリの場合, old_dir_vだけをロックする */

		rc = vfs_vnode_lock(old_dir_v);
		kassert( rc != -ENOENT );
		if ( rc != 0 )
			goto new_dir_v_put_out; /* イベントを受信したまたはメモリ不足 */

		/* ファイルシステム固有のrename処理を呼び出す
		 */
		rc = old_dir_v->v_mount->m_fs->c_calls->fs_rename(
			old_dir_v->v_mount->m_fs_super,
			old_dir_v->v_id, old_dir_v->v_fs_vnode, old_filename,
			old_dir_v->v_id, old_dir_v->v_fs_vnode, new_filename);

		vfs_vnode_unlock(old_dir_v);
	}

	vfs_vnode_ptr_put(new_dir_v);
	vfs_vnode_ptr_put(old_dir_v);
	kfree(new_filename);
	kfree(old_filename);

	return 0;

new_dir_v_put_out:
	vfs_vnode_ptr_put(new_dir_v);

old_dir_v_put_out:
	vfs_vnode_ptr_put(old_dir_v);

free_new_filename_out:
	kfree(new_filename);

free_old_filename_out:
	kfree(old_filename);

error_out:
	return rc;
}

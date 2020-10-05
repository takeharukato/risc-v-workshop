/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  vfs open operations                                               */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>

#include <kern/vfs-if.h>

/*
 * ファイルシステム操作IF
 */

/**
   指定されたパスのディレクトリを開く
   @param[in]  ioctx  I/Oコンテキスト
   @param[in]  path   openするディレクトリのパス
   @param[in]  oflags open時に指定したモード
   @param[out] fdp    ユーザファイルディスクリプタを返却する領域
   @retval  0       正常終了
   @retval -EBADF   不正なユーザファイルディスクリプタを指定した
   @retval -ENOMEM  メモリ不足
   @retval -ENOENT  パスが見つからなかった
   @retval -ENOSPC  ユーザファイルディスクリプタに空きがない
   @retval -ENOTDIR ディレクトリ以外のファイルを開こうとした
   @retval -EIO     I/Oエラーが発生した
 */
int
vfs_opendir(vfs_ioctx *ioctx, char *path, vfs_open_flags oflags, int *fdp) {
	int                    rc;
	int                    fd;
	vnode                  *v;
	vfs_open_flags dir_oflags;

	/*
	 * 指定されたファイルパスのvnodeの参照を取得
	 */
	rc = vfs_path_to_vnode(ioctx, path, &v);
	if (rc != 0) {

		kassert( ( rc == -ENOMEM ) || ( rc == -ENOENT ) || ( rc == -EIO ) );
		goto out;
	}

	if ( !( v->v_mode & VFS_VNODE_MODE_DIR ) ) {

		rc = -ENOTDIR; /* ディレクトリ以外のファイルを開こうとした */
		goto unref_vnode_out;
	}

	dir_oflags = oflags & VFS_O_OPENDIR_MASK; /* ファイルオープンモードを補正 */

	/* vnodeに対するファイルディスクリプタを取得
	 */
	rc = vfs_fd_alloc(ioctx, v, dir_oflags, &fd, NULL);
	if ( rc != 0 )
		goto unref_vnode_out;

	*fdp = fd;  /* ユーザファイルディスクリプタを返却 */

	vfs_vnode_ref_dec(v);  /* パス検索時に獲得したv-nodeの参照を解放  */

	return 0;

unref_vnode_out:
	vfs_vnode_ref_dec(v);  /* パス検索時に獲得したv-nodeの参照を解放  */

out:
	return rc;
}

/**
   指定されたパスのファイルを開く
   @param[in]  ioctx  I/Oコンテキスト
   @param[in]  path   openするファイルのパス
   @param[in]  oflags open時に指定したフラグ値
   @param[in]  omode  open時に指定したファイル種別/アクセス権
   @param[out] fdp    ユーザファイルディスクリプタを返却する領域
   @retval  0      正常終了
   @retval -EBADF  不正なユーザファイルディスクリプタを指定した
   @retval -ENOMEM メモリ不足
   @retval -ENOENT パスが見つからなかった
   @retval -ENOSPC ユーザファイルディスクリプタに空きがない
   @retval -EISDIR ディレクトリを開こうとした
   @retval -EIO    I/Oエラーが発生した
 */
int
vfs_open(vfs_ioctx *ioctx, char *path, vfs_open_flags oflags, vfs_fs_mode omode, int *fdp) {
	int             rc;
	int             fd;
	vnode           *v;
	vfs_file_stat   st;

	if ( oflags & VFS_O_DIRECTORY )
		return vfs_opendir(ioctx, path, oflags, fdp);  /* ディレクトリを開く */

	/*
	 * 指定されたファイルパスのvnodeの参照を取得
	 */
	rc = vfs_path_to_vnode(ioctx, path, &v);

	if ( ( rc != 0 ) && ( ( rc != -ENOENT ) || ( ( oflags & VFS_O_CREAT ) != 0 ) ) ) {

		/* ファイルが存在しない */
		kassert( ( rc == -ENOMEM ) || ( rc == -ENOENT ) || ( rc == -EIO ) );
		goto out;
	}

	if ( ( oflags & VFS_O_CREAT ) != 0 ) {  /* ファイル生成処理 */

		if ( ( rc == 0 ) && ( ( oflags & VFS_O_EXCL ) != 0 ) ) {

			rc = -EEXIST;  /* ファイルが存在する場合はエラーとする */
			goto unref_vnode_out;
		}

		if ( rc == -ENOENT ) { /* ファイルが存在しない場合はファイルを生成する */

			vfs_init_attr_helper(&st);  /* ファイル属性情報を初期化する */
			st.st_mode = omode;  /* ファイルモード */
			rc = vfs_create(ioctx, path, &st); /* ファイルを生成する */
			if ( rc != 0 )
				goto unref_vnode_out;  /* ファイル生成に失敗した */
		}
	}

	if ( oflags & VFS_O_TRUNC ) { /* ファイルサイズを0にする */

		vfs_init_attr_helper(&st);  /* ファイル属性情報を初期化する */
		st.st_size = 0;    /* ファイルサイズを0にする */
		rc = vfs_setattr(v, &st, VFS_VSTAT_MASK_SIZE); /* ファイルサイズのみを設定する */
		if ( rc != 0 )
			goto unref_vnode_out;  /* サイズ更新に失敗した */
	}

	//if ( oflags & VFS_O_APPEND ) /* ファイルポジションを末尾に設定する */

	if ( v->v_mode & VFS_VNODE_MODE_DIR ) {

		rc = -EISDIR; /* ディレクトリを開こうとした */
		goto unref_vnode_out;
	}

	/*
	 * vnodeに対するファイルディスクリプタを取得
	 */
	rc = vfs_fd_alloc(ioctx, v, oflags, &fd, NULL);
	if ( rc != 0 )
		goto unref_vnode_out;

	*fdp = fd;  /* ユーザファイルディスクリプタを返却 */

	vfs_vnode_ref_dec(v);  /* パス検索時に獲得したvnodeの参照を解放  */

	return 0;

unref_vnode_out:
	vfs_vnode_ref_dec(v);  /* パス検索時に獲得したvnodeの参照を解放  */

out:
	return rc;
}

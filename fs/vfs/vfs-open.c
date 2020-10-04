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

/**
   open処理共通関数 (内部関数)
   @param[in]  ioctx I/Oコンテキスト
   @param[in]  v     openするファイルのvnode
   @param[in]  oflags open時に指定したモード
   @param[out] fdp   ユーザファイルディスクリプタを返却する領域
   @retval  0     正常終了
   @retval -EBADF  不正なユーザファイルディスクリプタを指定した
   @retval -ENOSPC ユーザファイルディスクリプタに空きがない
   @retval -EPERM  ディレクトリを書き込みで開こうとした
   @retval -ENOMEM メモリ不足
   @retval -EIO    I/Oエラー
   @retval -ENOSYS open/opendirをサポートしていない
 */
static int 
open_common(vfs_ioctx *ioctx, vnode *v, vfs_open_flags oflags, int *fdp){
	int                     fd;
	vfs_file_private file_priv;
	file_descriptor         *f;
	int                     rc;

	/*
	 * ファイルディスクリプタを獲得する
	 */
	rc = vfs_fd_alloc(ioctx, v, oflags, &fd, &f);
	if ( rc != 0 ) {

		kassert( rc == -ENOMEM );
		goto out;
	}

	kassert( f->f_vn != NULL );  
	kassert( f->f_vn->v_mount != NULL );
	kassert( f->f_vn->v_mount->m_fs != NULL );
	kassert( is_valid_fs_calls( f->f_vn->v_mount->m_fs->c_calls ) );

	if ( ( f->f_vn->v_mode & VFS_VNODE_MODE_DIR )
	    && ( oflags & VFS_O_WRONLY ) ) {

		/* 書き込みでディレクトリを開こうとした */
		rc = -EPERM;
		goto put_fd_out;
	}

	/*
	 * Close On Exec指定フラグの設定
	 */
	if ( oflags & VFS_O_CLOEXEC )
		f->f_flags |=  VFS_FDFLAGS_COE;

	if ( v->v_mount->m_fs->c_calls->fs_open != NULL ) {

		/*
		 * ファイルシステム固有のオープン処理を実施
		 */

		file_priv = NULL;  /* ファイルディスクリプタプライベート情報を初期化 */
		
		rc = v->v_mount->m_fs->c_calls->fs_open(v->v_mount->m_fs_super, 
		    v->v_fs_vnode, oflags, &file_priv);
		if (rc != 0) {
			
			kassert( ( rc == -ENOMEM ) || ( rc == -EIO ) );
			goto put_fd_out;
		}
		
		f->f_private = file_priv;
	} else {
		
		rc = -ENOSYS;
		goto put_fd_out;
	}
	
	*fdp = fd;  /*  ユーザファイルディスクリプタを返却  */
	
	return 0;

put_fd_out:
	/*  vfs_fd_allocで加算したvnodeカウント, ファイルディスクリプタカウントを減算  */
	vfs_fd_put(f);
out:
	return rc;
}

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
	int                   rc;
	int                   fd;
	vnode                 *v;
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

	dir_oflags =  oflags & VFS_O_OPENDIR_MASK; /* ファイルオープンモードを補正 */
	
	/* vnodeに対するファイルディスクリプタを取得
	 */
	rc = open_common(ioctx, v, dir_oflags, &fd);
	if ( rc != 0 ) {

		kassert( ( rc == -ENOMEM ) || ( rc == -ENOENT ) || ( rc == -EIO ) );
		goto unref_vnode_out;
	}

	*fdp = fd;

unref_vnode_out:
	vfs_vnode_ref_dec(v);  /* パス検索時に獲得したvnodeの参照を解放  */

out:
	return rc;
}

/**
   指定されたパスのファイルを開く
   @param[in]  ioctx  自プロセスのI/Oコンテキスト
   @param[in]  path   openするファイルのパス
   @param[in]  oflags open時に指定したフラグ値
   @param[in]  omode  open時に指定したファイル種別/アクセス権
   @param[out] fdp   ユーザファイルディスクリプタを返却する領域
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

			memset(&st, 0, sizeof(vfs_file_stat));
			st.st_mode = omode;  /* ファイルモード */
			rc = vfs_create(ioctx, path, &st); /* ファイルを生成する */
			if ( rc != 0 )
				goto unref_vnode_out;  /* ファイル生成に失敗した */
		}
	}

	//if ( oflags & VFS_O_TRUNC ) /* ファイルサイズを0にする */
	//if ( oflags & VFS_O_APPEND ) /* ファイルポジションを末尾に設定する */
	
	if ( v->v_mode & VFS_VNODE_MODE_DIR ) {

		rc = -EISDIR; /* ディレクトリを開こうとした */
		goto unref_vnode_out;
	}

	/*
	 * vnodeに対するファイルディスクリプタを取得
	 */
	rc = open_common(ioctx, v, oflags, &fd);
	if ( rc != 0 ) {

		kassert( ( rc == -ENOMEM ) || ( rc == -ENOENT ) || ( rc == -EIO ) );
		goto unref_vnode_out;
	}

	*fdp = fd;

unref_vnode_out:
	vfs_vnode_ref_dec(v);  /* パス検索時に獲得したvnodeの参照を解放  */

out:
	return rc;
}

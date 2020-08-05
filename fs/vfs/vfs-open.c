/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  vfs open                                                          */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>

#include <kern/spinlock.h>
#include <kern/wqueue.h>
#include <kern/mutex.h>
#include <kern/vfs-if.h>
#include <kern/page-if.h>

/**
   open処理共通関数 (内部関数)
   @param[in]  cur_ioctx I/Oコンテキスト
   @param[in]  v         openするファイルのvnode
   @param[in]  omode     open時に指定したモード
   @param[out] fdp       ユーザファイルディスクリプタを返却する領域
   @retval  0     正常終了
   @retval -EBADF  不正なユーザファイルディスクリプタを指定した
   @retval -ENOSPC ユーザファイルディスクリプタに空きがない
   @retval -EPERM  ディレクトリを書き込みで開こうとした
   @retval -ENOMEM メモリ不足
   @retval -EIO    I/Oエラー
   @retval -ENOSYS open/opendirをサポートしていない
 */
static int 
open_common(vfs_ioctx *cur_ioctx, vnode *v, vfs_open_flags omode, int *fdp){
	int                     fd;
	vfs_file_private file_priv;
	file_descriptor         *f;
	int                     rc;

	/*
	 * ファイルディスクリプタを獲得する
	 */
	rc = vfs_fd_alloc(cur_ioctx, v, omode, &fd, &f);
	if ( rc != 0 ) {

		kassert( rc == -ENOMEM );
		goto out;
	}

	kassert( f->f_vn != NULL );  
	kassert( f->f_vn->v_mount != NULL );
	kassert( f->f_vn->v_mount->m_fs != NULL );
	kassert( is_valid_fs_calls( f->f_vn->v_mount->m_fs->c_calls ) );

	/*
	 * Close On Exec指定フラグの設定
	 */
	if ( omode & VFS_O_CLOEXEC )
		f->f_flags |=  VFS_FDFLAGS_COE;

	if ( v->v_mount->m_fs->c_calls->fs_open != NULL ) {


		if ( ( f->f_vn->v_mode & VFS_VNODE_MODE_DIR )
		    && ( omode & VFS_O_WRONLY ) ) {

			/* 書き込みでディレクトリを開こうとした */
			rc = -EPERM;
			goto put_fd_out;
		}

		/*
		 * ファイルシステム固有のオープン処理を実施
		 */
		rc = v->v_mount->m_fs->c_calls->fs_open(v->v_mount->m_fs_super, 
		    v->v_fs_vnode, omode, &file_priv);
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
   指定されたパスのファイルを開く
   @param[in]  cur_ioctx 自プロセスのI/Oコンテキスト
   @param[in]  path      openするファイルのパス
   @param[in]  omode     open時に指定したモード
   @param[out] fdp       ユーザファイルディスクリプタを返却する領域
   @retval  0      正常終了
   @retval -EBADF  不正なユーザファイルディスクリプタを指定した
   @retval -ENOMEM メモリ不足
   @retval -ENOENT パスが見つからなかった
   @retval -ENOSPC ユーザファイルディスクリプタに空きがない
   @retval -EPERM  ディレクトリを書き込みで開こうとした
   @retval -EIO    I/Oエラーが発生した
 */
int
vfs_open(vfs_ioctx *cur_ioctx, char *path, vfs_open_flags omode, int *fdp) {
	vnode *v;
	int   rc;
	int   fd;

	/*
	 * 指定されたファイルパスのvnodeの参照を取得
	 */
	rc = vfs_path_to_vnode(cur_ioctx, path, &v);
	if (rc != 0) {

		kassert( ( rc == -ENOMEM ) || ( rc == -ENOENT ) || ( rc == -EIO ) );
		goto out;
	}

	/*
	 * vnodeに対するファイルディスクリプタを取得
	 */
	rc = open_common(cur_ioctx, v, omode, &fd);
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

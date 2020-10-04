/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  vfs close operations                                              */
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
   ディレクトリを開いているユーザファイルディスクリプタのクローズ
   @param[in] ioctx I/Oコンテキスト
   @param[in] fd    ユーザファイルディスクリプタ
   @retval  0       正常終了
   @retval -EBADF   不正なユーザファイルディスクリプタを指定した
   @retval -ENOTDIR ユーザファイルディスクリプタがディレクトリを指している
 */
int
vfs_closedir(vfs_ioctx *ioctx, int fd){
	int             rc;
	file_descriptor *f;

	rc = vfs_fd_get(ioctx, fd, &f); /* ファイルディスクリプタへの参照を得る */
	if ( rc != 0 )
		return -EBADF;  /*  不正なユーザファイルディスクリプタを指定した */

	kassert( f->f_vn != NULL );
	if ( !( f->f_vn->v_mode & VFS_VNODE_MODE_DIR ) ) { /* ディレクトリでない場合 */

		/*
		 * ユーザファイルディスクリプタがディレクトリを指していない
		 */
		rc = -ENOTDIR;
		goto put_fd_out;
	}

	rc = vfs_fd_remove(ioctx, f);  /*  ユーザファイルディスクリプタを解放  */
	kassert( rc == -EBUSY );  /* 本関数内で参照を得ているので最終参照ではない */

	vfs_fd_put(f);  /*  ファイルディスクリプタへの参照を解放する  */

	return 0;

put_fd_out:
	vfs_fd_put(f);  /*  ファイルディスクリプタへの参照を解放する  */

	return rc;
}

/**
   ユーザファイルディスクリプタのクローズ
   @param[in] ioctx I/Oコンテキスト
   @param[in] fd    ユーザファイルディスクリプタ
   @retval  0      正常終了
   @retval -EBADF  不正なユーザファイルディスクリプタを指定した
   @retval -EISDIR ユーザファイルディスクリプタがディレクトリを指している
 */
int
vfs_close(vfs_ioctx *ioctx, int fd){
	int             rc;
	file_descriptor *f;

	rc = vfs_fd_get(ioctx, fd, &f); /* ファイルディスクリプタへの参照を得る */
	if ( rc != 0 )
		return -EBADF;  /*  不正なユーザファイルディスクリプタを指定した */

	kassert( f->f_vn != NULL );
	if ( f->f_vn->v_mode & VFS_VNODE_MODE_DIR ) { /* ディレクトリの場合 */
		
		/*
		 * ユーザファイルディスクリプタがディレクトリを指している
		 */
		rc = -EISDIR;
		goto put_fd_out;
	}

	rc = vfs_fd_remove(ioctx, f);  /*  ユーザファイルディスクリプタを解放  */
	kassert( rc == -EBUSY );  /* 本関数内で参照を得ているので最終参照ではない */

	vfs_fd_put(f);  /*  ファイルディスクリプタへの参照を解放する  */

	return 0;

put_fd_out:
	vfs_fd_put(f);  /*  ファイルディスクリプタへの参照を解放する  */

	return rc;
}

/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  vfs read operations                                               */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>

#include <kern/vfs-if.h>
#include <kern/timer-if.h>

/**
   ファイルから読み込む
   @param[in] ioctx   I/Oコンテキスト
   @param[in] fd      ユーザファイルディスクリプタ
   @param[in] buf     データ読み込み先カーネル内アドレス
   @param[in] len     読み込むサイズ(単位:バイト)
   @param[out] rdlenp 読み込んだサイズ(単位:バイト)を返却するアドレス
   @retval  0      正常終了
   @retval -EBADF  正当なユーザファイルディスクリプタでない
   @retval -EIO    I/Oエラー
   @retval -ENOSYS readをサポートしていない 
   @retval -EINVAL lenが負
*/
int
vfs_read(vfs_ioctx *ioctx, int fd, void *buf, ssize_t len, ssize_t *rdlenp){
	file_descriptor *f;
	vfs_file_stat   st;
	ktimespec       ts;
	int             rc;
	ssize_t   rd_bytes;

	if ( 0 > len )
		return -EINVAL;

	/*
	 * ユーザファイルディスクリプタに対応するファイルディスクリプタを取得
	 */
	rc = vfs_fd_get(ioctx, fd, &f); /* ファイルディスクリプタへの参照を得る */
	if ( rc != 0 ) {

		rc = -EBADF;  /*  不正なユーザファイルディスクリプタを指定した */
		goto error_out;
	}

	kassert(f->f_vn != NULL);  
	kassert(f->f_vn->v_mount != NULL);
	kassert(f->f_vn->v_mount->m_fs != NULL);
	kassert( is_valid_fs_calls( f->f_vn->v_mount->m_fs->c_calls ) );

	rc = -ENOSYS;  /*  ハンドラが定義されていない場合は, -ENOSYSを返却して復帰  */

	rd_bytes = len; /* 読み取り長を設定 */
	
	/*
	 * ファイルシステム固有のファイル読込み処理を実行
	 */
	if ( f->f_vn->v_mount->m_fs->c_calls->fs_read != NULL ) {

		rd_bytes = f->f_vn->v_mount->m_fs->c_calls->fs_read(
			f->f_vn->v_mount->m_fs_super,
			f->f_vn->v_id, f->f_vn->v_fs_vnode, f->f_private, buf, f->f_pos, len);
		if ( 0 > rd_bytes ) {

			rc = rd_bytes;  /* エラーコードを返却 */
			goto put_fd_out;
		}
	}

	/* 読込み成功時は読み込んだバイト数に応じて,
	 * ファイルポジションを更新する
	 */
	if ( rdlenp != NULL )
		*rdlenp = rd_bytes;    /* 読み取り長を返却 */
	f->f_pos += rd_bytes; /* ファイルポジションを更新 */

	if ( ( rd_bytes > 0 ) 
	     && ( !( f->f_flags & VFS_O_NOATIME ) ) ) {  /* アクセス時刻更新が必要な場合 */

		vfs_init_attr_helper(&st);  /* ファイル属性情報を初期化する */
		/* TODO: 時刻更新ヘルパを作成して, アクセス時刻を更新する  */
		tim_walltime_get(&ts);  /* 現在時刻を取得 */
		st.st_atime = ts.tv_sec;    
		/* アクセス時刻のみを設定する */
		rc = vfs_setattr(f->f_vn, &st, VFS_VSTAT_MASK_ATIME);
		if ( rc != 0 )
			goto put_fd_out;  /* 時刻更新に失敗した */
	}

	vfs_fd_put(f);  /*  ファイルディスクリプタの参照を解放  */

	return 0;

put_fd_out:
	vfs_fd_put(f);  /*  ファイルディスクリプタの参照を解放  */
	
error_out:
	return rc;
}

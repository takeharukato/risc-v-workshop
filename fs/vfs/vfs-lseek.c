/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  vfs lseek operation                                               */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>
#include <kern/page-if.h>

#include <kern/vfs-if.h>

/**
   ファイルポジションを更新する
   @param[in] ioctx     I/Oコンテキスト
   @param[in] fd        ユーザファイルディスクリプタ
   @param[in] pos       ファイルポジション
   @param[in] whence    ファイルポジションの意味
   - VFS_SEEK_WHENCE_SET オフセットは pos バイトに設定される。
   - VFS_SEEK_WHENCE_CUR オフセットは現在位置に pos バイトを足した位置になる。
   - VFS_SEEK_WHENCE_END オフセットはファイルのサイズに pos バイトを足した位置になる。
   @retval  0      正常終了
   @retval -EBADF  不正なユーザファイルディスクリプタを指定した
   @retval -EINVAL whenceが不正
*/
int
vfs_lseek(vfs_ioctx *ioctx, int fd, off_t pos, vfs_seek_whence whence){
	int              rc;
	int     getattr_res;
	file_descriptor  *f;
	off_t       new_pos;
	off_t       arg_pos;
	vfs_file_stat    st;

	if ( ( whence != VFS_SEEK_WHENCE_SET )
	     && ( whence != VFS_SEEK_WHENCE_CUR )
	     && ( whence != VFS_SEEK_WHENCE_END )
	     && ( whence != VFS_SEEK_WHENCE_DATA )
	     && ( whence != VFS_SEEK_WHENCE_HOLE ) )
		return -EINVAL;  /*  whenceが不正  */

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

	/* ファイル末尾からのシークやホールへのシークのために,
	 * ファイルサイズ情報を得る
	 */
	if ( ( whence == VFS_SEEK_WHENCE_END ) ||
	     ( whence == VFS_SEEK_WHENCE_HOLE ) ) {

		vfs_init_attr_helper(&st);  /* ファイル属性情報を初期化する */
		/* ファイルサイズを取得する */
		getattr_res = vfs_getattr(f->f_vn, VFS_VSTAT_MASK_SIZE, &st);

		/* ファイルサイズが取れない場合は,
		 * 現在のポジションを仮に設定する
		 */
		if ( getattr_res != 0 )
			new_pos = f->f_pos;
	}

	/*
	 *  引数で指定されたファイルポジションをwhenceの指示に応じて補正し,
	 *  更新後の論理ファイルポジション (ファイル種類に依存しない
	 *  論理的なファイルポジション)をnew_posに保存
	 */
	arg_pos = pos;  /* 引数で指定されたファイルポジションを格納 */

	switch( whence ) {

	case VFS_SEEK_WHENCE_SET:  /* ファイル先頭からのシークの場合 */

		if ( 0 > arg_pos )
			arg_pos = 0;
		new_pos = pos;
		break;

	case VFS_SEEK_WHENCE_CUR:
		if ( ( 0 > arg_pos ) && ( ( -1 * arg_pos ) > f->f_pos ) ) {

			/* 現在位置から前の位置にシークする場合で,
			 * ファイル先頭を超える場合は, ファイルポジションを
			 * ファイルの先頭に設定の上, ファイル先頭をシークするように
			 * 引数を補正する
			 */
			new_pos = 0;  /* ファイルポジションをファイルの先頭に設定 */
			arg_pos = -1 * f->f_pos; /* 現在のファイルポジション分前に戻る */
		} else
			new_pos = f->f_pos + arg_pos; /* 指定された位置に設定 */
		break;

	case VFS_SEEK_WHENCE_END:

		if ( getattr_res == 0 ) {

			/* ファイルサイズ情報を獲得できた場合は,
			 * VFS層でファイルポジションを算出
			 */
			if ( ( 0 > arg_pos )
			     && ( ( -1 * arg_pos ) > st.st_size ) ) {

				/* ファイル末尾から前の位置にシークする場合で,
				 * ファイル先頭を超える場合は, ファイルポジションを
				 * ファイルの先頭に設定の上, ファイル先頭をシークするように
				 * 引数を補正する
				 */
				new_pos = 0;  /* ファイルポジションをファイルの先頭に設定 */
				arg_pos = -1 * st.st_size; /* ファイルサイズ分前に戻る */
			} else
				new_pos = st.st_size + arg_pos; /* 指定された位置に設定 */
		}
		break;

	case VFS_SEEK_WHENCE_DATA:
		/*  指定位置以降の次にデータがある位置をシークする(GNU 拡張)
		 *  引数で渡された位置(pos)を返す。
		 *  posが参照する場所がホールであったとしても,
		 *  連続する0の列のデータで構成されているとみなす。
		 */
		new_pos = arg_pos;
		break;

	case VFS_SEEK_WHENCE_HOLE:
		/*  指定位置以降の次にホールがある位置をシークする(GNU 拡張)
		 *  ファイルサイズ情報を獲得できた場合は,VFS層でファイルの末尾を返す。
		 */
		if ( getattr_res == 0 )
			new_pos = st.st_size;
		break;
	default:
		break;
	}

	kassert( new_pos >= 0 );  /* VFS層の返却値が負にならないことを確認 */

	if ( f->f_vn->v_mount->m_fs->c_calls->fs_seek == NULL )
		goto success;  /* seek処理が定義されていなければVFS層の設定値を返却 */

	/*  ファイルシステム固有のseek処理を実施
	 */
	rc = f->f_vn->v_mount->m_fs->c_calls->fs_seek(f->f_vn->v_mount->m_fs_super,
						      f->f_vn->v_fs_vnode,
						      arg_pos, whence, f->f_private,
						      &new_pos);
	if ( rc != 0 )
		goto put_fd_out;

	if ( 0 > new_pos )
		new_pos = 0;  /*  ファイルポジションが負になる場合は0に補正  */
success:
	f->f_pos = new_pos;  /* ファイルポジションを更新 */

	vfs_fd_put(f);  /*  ファイルディスクリプタの参照を解放  */

	return 0;

put_fd_out:
	vfs_fd_put(f);  /*  ファイルディスクリプタの参照を解放  */

error_out:
	return rc;

}

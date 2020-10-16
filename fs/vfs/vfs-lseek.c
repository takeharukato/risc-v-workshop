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

#include <kern/vfs-if.h>

/**
   ファイルポジションを更新する
   @param[in] ioctx  I/Oコンテキスト
   @param[in] fp     カーネルファイルディスクリプタ
   @param[in] pos    ファイルポジション
   @param[in] whence ファイルポジションの意味
   - VFS_SEEK_WHENCE_SET オフセットは pos バイトに設定される。
   - VFS_SEEK_WHENCE_CUR オフセットは現在位置に pos バイトを足した位置になる。
   - VFS_SEEK_WHENCE_END オフセットはファイルのサイズに pos バイトを足した位置になる。
   @retval  0      正常終了
   @retval -EINVAL whenceが不正
*/
int
vfs_lseek(vfs_ioctx *ioctx, file_descriptor *fp, off_t pos, vfs_seek_whence whence){
	int              rc;
	int             res;
	off_t       new_pos;
	off_t       arg_pos;
	vfs_file_stat    st;

	if ( ( whence != VFS_SEEK_WHENCE_SET )
	     && ( whence != VFS_SEEK_WHENCE_CUR )
	     && ( whence != VFS_SEEK_WHENCE_END )
	     && ( whence != VFS_SEEK_WHENCE_DATA )
	     && ( whence != VFS_SEEK_WHENCE_HOLE ) )
		return -EINVAL;  /*  whenceが不正  */

	kassert(fp->f_vn != NULL);
	kassert(fp->f_vn->v_mount != NULL);
	kassert(fp->f_vn->v_mount->m_fs != NULL);
	kassert( is_valid_fs_calls( fp->f_vn->v_mount->m_fs->c_calls ) );

	/*
	 *  引数で指定されたファイルポジションをwhenceの指示に応じて補正し,
	 *  更新後の論理ファイルポジション (ファイル種類に依存しない
	 *  論理的なファイルポジション)をnew_posに保存
	 */
	arg_pos = pos;  /* 引数で指定されたファイルポジションを格納 */
	new_pos = fp->f_pos;  /* サイズ不明の場合は, 現在位置を指す */

	/* ファイル末尾からのシークやホールへのシークのために,
	 * ファイルサイズ情報を得る
	 */
	vfs_init_attr_helper(&st);  /* ファイル属性情報を初期化する */
	/* ファイルサイズを取得する */
	res = vfs_getattr(fp->f_vn, VFS_VSTAT_MASK_SIZE, &st);

	switch( whence ) {

	case VFS_SEEK_WHENCE_SET:  /* ファイル先頭からのシークの場合 */

		if ( 0 > arg_pos )
			arg_pos = 0;
		new_pos = arg_pos;
		break;

	case VFS_SEEK_WHENCE_CUR:

		if ( ( 0 > arg_pos ) && ( ( -1 * arg_pos ) > fp->f_pos ) ) {

			/* 現在位置から前の位置にシークする場合で,
			 * ファイル先頭を超える場合は, ファイルポジションを
			 * ファイルの先頭に設定の上, ファイル先頭をシークするように
			 * 引数を補正する
			 */
			new_pos = 0;  /* ファイルポジションをファイルの先頭に設定 */
			arg_pos = -1 * fp->f_pos; /* 現在のファイルポジション分前に戻る */
		} else
			new_pos = fp->f_pos + arg_pos; /* 指定された位置に設定 */
		break;

	case VFS_SEEK_WHENCE_END:

		if ( res == 0 ) {

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
		} else
			new_pos = fp->f_pos; /* ファイルサイズ不明時は, 現在の位置を設定 */

		break;

	case VFS_SEEK_WHENCE_DATA:
		/*  指定位置以降の次にデータがある位置をシークする(GNU 拡張)
		 *  引数で渡された位置(pos)を返す。
		 *  posが参照する場所がホールであったとしても,
		 *  連続する0の列のデータで構成されているとみなす。
		 */
		if ( res == 0 ) {

			/* ファイル末尾から前の位置にシークする場合で,
			 * ファイル先頭を超える場合は, ファイルポジションを
			 * ファイルの先頭に設定の上, ファイル先頭をシークするように
			 * 引数を補正する
			 */
			if ( ( 0 > arg_pos ) && ( ( -1 * arg_pos ) > st.st_size ) )
				new_pos = 0;
			else
				new_pos = arg_pos;
		}
		break;

	case VFS_SEEK_WHENCE_HOLE:

		/*  指定位置以降の次にホールがある位置をシークする(GNU 拡張)
		 *  ファイルサイズ情報を獲得できた場合は,VFS層でファイルの末尾を返す。
		 */
		if ( res == 0 )
			new_pos = st.st_size;
		else
			new_pos = fp->f_pos; /* ファイルサイズ不明時は, 現在の位置を設定 */
		break;

	default:
		break;
	}

	kassert( new_pos >= 0 );  /* VFS層の返却値が負にならないことを確認 */

	if ( fp->f_vn->v_mount->m_fs->c_calls->fs_seek == NULL )
		goto success;  /* seek処理が定義されていなければVFS層の設定値を返却 */

	rc = vfs_vnode_lock(fp->f_vn);  /* v-nodeのロックを獲得 */
	kassert( rc != -ENOENT );
	if ( rc != 0 ) /* イベントを受信したまたはメモリ不足 */
		goto error_out;

	/*  ファイルシステム固有のseek処理を実施
	 */
	rc = fp->f_vn->v_mount->m_fs->c_calls->fs_seek(fp->f_vn->v_mount->m_fs_super,
						      fp->f_vn->v_fs_vnode,
						      arg_pos, whence, fp->f_private,
						      &new_pos);

	vfs_vnode_unlock(fp->f_vn);  /* v-nodeのロックを解放 */

	if ( rc != 0 )
		goto error_out;

	if ( 0 > new_pos )
		new_pos = 0;  /*  ファイルポジションが負になる場合は0に補正  */
success:
	fp->f_pos = new_pos;  /* ファイルポジションを更新 */

	return 0;

error_out:
	return rc;
}

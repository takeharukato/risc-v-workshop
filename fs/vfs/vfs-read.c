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
#include <fs/vfs/vfs-internal.h>

/**
   ファイルから読み込む
   @param[in] ioctx   I/Oコンテキスト
   @param[in] fp      カーネルファイルディスクリプタ
   @param[in] buf     データ読み込み先カーネル内アドレス
   @param[in] len     読み込むサイズ(単位:バイト)
   @param[out] rdlenp 読み込んだサイズ(単位:バイト)を返却するアドレス
   @retval  0      正常終了
   @retval -EIO    I/Oエラー
   @retval -EINVAL lenが負
   @retval -ENOENT 削除中のv-nodeを指定した
   @retval -ENOSYS read/getattrをサポートしていない
*/
int
vfs_read(vfs_ioctx *ioctx, file_descriptor *fp, void *buf, ssize_t len, ssize_t *rdlenp){
	int              rc;
	bool            res;
	ssize_t    rd_bytes;
	ssize_t     remains;
	ssize_t       total;
	vfs_file_stat    st;
	vfs_page_cache  *pc;
	size_t        pgsiz;
	off_t        pg_off;
	void          *data;

	if ( 0 > len ) {

		rc = -EINVAL; /* 読み取り長が負 */
		goto error_out;
	}

	kassert(fp->f_vn != NULL);
	kassert(fp->f_vn->v_mount != NULL);
	kassert(fp->f_vn->v_mount->m_fs != NULL);
	kassert( is_valid_fs_calls( fp->f_vn->v_mount->m_fs->c_calls ) );

	res = vfs_vnode_ref_inc(fp->f_vn);  /* v-nodeへの参照を得る */
	kassert(res); /* ファイルディスクリプタからの参照が残っているので参照を獲得可能 */

	/*
	 * ファイルサイズを獲得する
	 */
	vfs_init_attr_helper(&st);
	rc = vfs_getattr(fp->f_vn, VFS_VSTAT_MASK_SIZE, &st);
	if ( rc != 0 )
		goto unref_vnode_out;  /* ファイルサイズ獲得に失敗した */

	kassert( fp->f_pos >= 0 );
	/*
	 *読み込みサイズをファイル長以内のサイズに補正する
	 */
	if ( CHECK_WRAP_AROUND(st.st_size, fp->f_pos, len) )
		remains = MIN( len, st.st_size - fp->f_pos );
	else
		remains = (fp->f_pos >= st.st_size) ? 0 : st.st_size - fp->f_pos;

	/*
	 * ページキャッシュに読み込む
	 */
	for( total = 0; remains > 0; remains -= rd_bytes ){


		rc = vfs_page_cache_get(fp->f_vn->v_pcp, fp->f_pos, &pc);
		if ( rc != 0 )
			goto return_read_byte_out;

		rc = vfs_page_cache_pool_pagesize_get(fp->f_vn->v_pcp, &pgsiz);
		if ( rc != 0 )
			goto put_page_cache_out;

		pg_off = fp->f_pos % pgsiz;  /* ページ内オフセット */
		rd_bytes = MIN(pgsiz - pg_off, remains);  /* 読み取り長 */
		vfs_page_cache_refer_data(pc, &data);    /* データ領域を参照 */
		memmove(buf, data + pg_off, rd_bytes);  /* ページキャッシュの内容を転送 */

		vfs_page_cache_put(pc);  /* ページキャッシュ解放 */

		/* 読込み成功時は読み込んだバイト数に応じて,
		 * ファイルポジションを更新する
		 */
		fp->f_pos += rd_bytes; /* ファイルポジションを更新 */
		total += rd_bytes;     /* 総読み取り長を加算 */
	}

	/* 1バイト以上読み取れた場合で, かつ,
	 * アクセス時刻更新不要指示でオープンしておらず, かつ
	 * アクセス時刻更新不要指示でマウントされていない場合は,
	 * 最終アクセス時刻を更新する
	 */
	if ( ( total > 0 )
	     && ( !( fp->f_flags & VFS_FDFLAGS_NOATIME ) )
	    && ( !( fp->f_vn->v_mount->m_mount_flags & VFS_MNT_NOATIME ) ) ) {

		/* 最終アクセス時刻を更新する  */
		rc = vfs_time_attr_helper(fp->f_vn, NULL, VFS_VSTAT_MASK_ATIME);
		if ( rc != 0 )
			goto return_read_byte_out; /* 時刻更新に失敗した */
	}

	if ( rdlenp != NULL )
		*rdlenp = total;    /* 読み取り長を返却 */

	vfs_vnode_ref_dec(fp->f_vn);  /* v-nodeへの参照を解放する */

	return 0;

put_page_cache_out:
	vfs_page_cache_put(pc);  /* ページキャッシュ解放 */

return_read_byte_out:
	if ( rdlenp != NULL )
		*rdlenp = total;    /* 読み取り長を返却 */

unref_vnode_out:
	vfs_vnode_ref_dec(fp->f_vn);  /* v-nodeへの参照を解放する */

error_out:
	return rc;
}

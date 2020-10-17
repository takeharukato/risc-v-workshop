/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Minix file system I-node operations                               */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>

#include <kern/spinlock.h>
#include <kern/dev-old-pcache.h>

#include <fs/minixfs/minixfs.h>

/**
   MinixV1 ディスクI-node情報のバイトオーダ反転(内部関数)
   @param[in] in_dip  変換元のディスクI-node情報
   @param[in] out_dip 変換先のディスクI-node情報
 */
static void
swap_minixv1_inode(minixv1_inode *in_dip, minixv1_inode *out_dip){
	int i;

	out_dip->i_mode = __bswap16( in_dip->i_mode );
	out_dip->i_uid = __bswap16( in_dip->i_uid );
	out_dip->i_size = __bswap32( in_dip->i_size );
	out_dip->i_mtime = __bswap32( in_dip->i_mtime );
	out_dip->i_gid = in_dip->i_gid;
	out_dip->i_nlinks = in_dip->i_nlinks;

	for( i = 0; MINIX_V1_NR_TZONES > i; ++i) {

		out_dip->i_zone[i] = __bswap16( in_dip->i_zone[i] );
	}
}

/**
   MinixV2 ディスクI-node情報のバイトオーダ反転(内部関数)
   @param[in] in_dip  変換元のディスクI-node情報
   @param[in] out_dip 変換先のディスクI-node情報
 */
static void
swap_minixv2_inode(minixv2_inode *in_dip, minixv2_inode *out_dip){
	int i;

	out_dip->i_mode = __bswap16( in_dip->i_mode );
	out_dip->i_nlinks = __bswap16( in_dip->i_nlinks );
	out_dip->i_uid = __bswap16( in_dip->i_uid );
	out_dip->i_gid = __bswap16( in_dip->i_gid );
	out_dip->i_size = __bswap32( in_dip->i_size );
	out_dip->i_atime = __bswap32( in_dip->i_atime );
	out_dip->i_mtime = __bswap32( in_dip->i_mtime );
	out_dip->i_ctime = __bswap32( in_dip->i_ctime );

	for( i = 0; MINIX_V2_NR_TZONES > i; ++i) {

		out_dip->i_zone[i] = __bswap32( in_dip->i_zone[i] );
	}
}

/**
   MinixのI-nodeを読み書きする
   @param[in]   sbp     Minixスーパブロック情報
   @param[in]   i_num   I-node番号
   @param[in]   rw_flag 読み書き種別
   - MINIX_RW_READING 読み取り
   - MINIX_RW_WRITING 書き込み
   @param[out]  dip     MinixディスクI-node情報
   @retval      0       正常終了
   @retval     -EINVAL  不正なスーパブロック情報を指定した
   @retval     -ENODEV  I-nodeが読み取れなかった
 */
int
minix_rw_disk_inode(minix_super_block *sbp, minix_ino i_num, int rw_flag, minix_inode *dip) {
	int          rc;
	off_t    pc_off;
	off_t       off;
	page_cache  *pc;
	void    *dinode;

	/*
	 * I-nodeが格納されている領域のページオフセットアドレスを算出する
	 */
	pc_off = MINIX_IMAP_BLKNO
		+ MINIX_D_SUPER_BLOCK(sbp, s_imap_blocks)
		+ MINIX_D_SUPER_BLOCK(sbp, s_zmap_blocks); /* ブロック単位でのオフセット */

	/* I-node格納先ブロック(単位: ブロック) */
	/* @note MinixのファイルシステムのI-node番号は1から始まるのに対し,
	 * ビットマップのビットは0から割り当てられるので, Minixファイルシステムの
	 * フォーマット(mkfs)時点で, 0番のI-node/ゾーンビットマップは予約されいている
	 * それに対して, I-nodeテーブルやゾーンは1番のディスクI-node情報や
	 * ゾーンから開始される。従って, ビットマップのビット番号とディレクトリエントリ中の
	 * I-node番号は等しく, ディレクトリエントリ中のI-node番号からI-nodeテーブルの
	 * インデクスへの変換式は,
	 * インデクス = ディレクトリエントリ中のI-node番号 - 1
	 * となる.
	 * 参考: Minixのalloc_zoneのコメント
	 * Note that the routine alloc_bit() returns 1 for the lowest possible
	 * zone, which corresponds to sp->s_firstdatazone.  To convert a value
	 * between the bit number, 'b', used by alloc_bit() and the zone number, 'z',
	 * stored in the inode, use the formula:
	 *     z = b + sp->s_firstdatazone - 1
	 * Alloc_bit() never returns 0, since this is used for NO_BIT (failure).
	 */
	pc_off +=
		(off_t)( (i_num - 1) / ( MINIX_BLOCK_SIZE(sbp) / MINIX_DINODE_SIZE(sbp) ) );
	pc_off = pc_off * MINIX_BLOCK_SIZE(sbp); /* 格納先アドレス ( 単位:バイト )*/
	pc_off += (off_t)( (i_num - 1) * MINIX_DINODE_SIZE(sbp) ); /* I-node配列 */

	/* ページキャッシュを獲得する */
	rc = pagecache_get(sbp->dev, pc_off, &pc);
	if ( rc != 0 ) {

		rc = -ENODEV; /* ページが読み取れなかった */
		goto error_out;
	}

	off = pc_off % pc->pgsiz;   /* ページ内オフセットを算出   */
	dinode = (pc->pc_data + off); /* ディスク上のI-node */

	if ( MINIX_SB_IS_V1(sbp) ){

		/*
		 * MinixV1ファイルシステムの場合
		 */
		if ( rw_flag == MINIX_RW_WRITING ) {


			if ( sbp->swap_needed ) /* バイトオーダ変換して書き込む */
				swap_minixv1_inode((minixv1_inode *)&dip->d_inode,
				    (minixv1_inode *)dinode);
			else
				memmove((void *)dinode, (void *)&dip->d_inode,
				    sizeof(minixv1_inode));
			pagecache_mark_dirty(pc);
		} else {

			if ( sbp->swap_needed ) /* バイトオーダ変換して読み込む */
				swap_minixv1_inode((minixv1_inode *)dinode,
				    (minixv1_inode *)&dip->d_inode);
			else
				memmove((void *)&dip->d_inode, (void *)dinode,
				    sizeof(minixv1_inode));
		}
	} else if ( ( MINIX_SB_IS_V2(sbp) ) || ( MINIX_SB_IS_V3(sbp) ) ) {

		/*
		 * MinixV2, MinixV3ファイルシステムの場合
		 */

		if ( rw_flag == MINIX_RW_WRITING ) {


			if ( sbp->swap_needed ) /* バイトオーダ変換して書き込む */
				swap_minixv2_inode((minixv2_inode *)&dip->d_inode,
				    (minixv2_inode *)dinode);
			else
				memmove((void *)dinode, (void *)&dip->d_inode,
				    sizeof(minixv2_inode));
			pagecache_mark_dirty(pc);
		} else {

			if ( sbp->swap_needed ) /* バイトオーダ変換して読み込む */
				swap_minixv2_inode((minixv2_inode *)dinode,
				    (minixv2_inode *)&dip->d_inode);
			else
				memmove((void *)&dip->d_inode, (void *)dinode,
				    sizeof(minixv2_inode));
		}

	} else {

		rc = -EINVAL;  /* 不正なI-node */
		goto put_pcache_out;
	}

	pagecache_put(pc);  /* ページキャッシュを解放する     */

	return 0;

put_pcache_out:
	pagecache_put(pc);  /* ページキャッシュを解放する     */

error_out:
	return rc;
}

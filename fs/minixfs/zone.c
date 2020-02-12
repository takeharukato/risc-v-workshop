/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Minix file system zone operations                                 */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>

#include <kern/spinlock.h>
#include <kern/dev-pcache.h>

#include <fs/minixfs/minixfs.h>

/**
   インデクス種別
 */
#define MINIX_ZONE_INDEX_NONE    (-1)  /**< 未定義  */
#define MINIX_ZONE_ADDR_NONE     (0)
#define MINIX_ZONE_ADDR_DIRECT   (1)
#define MINIX_ZONE_ADDR_SINGLE   (2)
#define MINIX_ZONE_ADDR_DOUBLE   (3)

/**
   ゾーンをクリアする
   @param[in] sbp    Minixスーパブロック情報
   @param[in] znum   ゾーン番号
   @param[in] offset クリア開始オフセット (単位: バイト)
   @param[in] size   クリアサイズ (単位: バイト)
   @retval    0      正常終了
   @retval   -EIO    ページキャッシュアクセス失敗
 */
int
minix_clear_zone(minix_super_block *sbp, minix_zone znum, off_t offset, off_t size){
	int                      rc; /* 返り値 */
	page_cache              *pc; /* ページキャッシュ */
	size_t                pgsiz; /* ページキャッシュサイズ   */
	obj_cnt_type     first_page; /* 操作開始ページアドレス(単位:バイト)   */
	obj_cnt_type       end_page; /* 操作終了ページアドレス(単位:バイト)   */
	obj_cnt_type       cur_page; /* 操作対象ページアドレス(単位:バイト)   */
	off_t               clr_pos; /* クリア開始アドレス(単位:バイト)   */
	off_t               cur_pos; /* クリア操作アドレス(単位:バイト)   */
	off_t               remains; /* 残クリアサイズ (単位:バイト) */
	off_t               clr_off; /* ページ内オフセット (単位:バイト) */
	off_t               clr_len; /* ページ内クリアサイズ (単位:バイト) */

	rc = pagecache_pagesize(sbp->dev, &pgsiz);  /* ページサイズ取得 */
	kassert( rc == 0 ); /* マウントされているはずなのでデバイスが存在する */

	/* デバイス内でのクリア開始アドレス */
	clr_pos = znum * MINIX_ZONE_SIZE(sbp) + offset;

	/* クリア対象ゾーンの先頭ページアドレスを算出  */
	first_page = truncate_align(clr_pos, pgsiz);
	/* クリア対象ゾーンの最終ページアドレスを算出  */
	end_page = roundup_align(clr_pos + size, pgsiz);
	
	/*
	 * ゾーン内のページを順次クリアする
	 */
	cur_pos = clr_pos;
	remains = size;
	for(cur_page = first_page;
	    ( end_page > cur_page ) && ( remains > 0 ); cur_page += pgsiz) {

		/*  ページキャッシュを読み込み  */
		rc = pagecache_get(sbp->dev, cur_page, &pc);
		if ( rc != 0 ) {

			rc = -EIO;
			goto error_out;
		}

		/*  ページ内クリア */
		clr_off = cur_pos % pgsiz;        /* ページ内オフセット */
		clr_len = MIN(remains, pgsiz - clr_off);  /* クリア長   */
		memset(pc->pc_data+clr_off, 0, clr_len);  /* クリア実行 */

		pagecache_put(pc);  /* ページキャッシュを解放する  */
		
		cur_pos += clr_len;  /* 書き込み先更新 */
		remains -= clr_len;  /* 残量更新 */
	}

	return 0;

error_out:
	return rc;
}

/**
   ゾーンを割り当てる
   @param[in]  sbp    Minixスーパブロック情報
   @param[out] zp     ゾーン番号返却域
   @retval     0      正常終了
   @retval    -ENOSPC 空きゾーンがない
 */
int 
minix_alloc_zone(minix_super_block *sbp, minix_zone *zp){
	int                        rc;
	minix_bitmap_idx bitmap_index;
	minix_zone               znum;

	/* ゾーンビットマップから空きゾーンを割り当てる */
	rc = minix_bitmap_alloc(sbp, ZONE_MAP, &bitmap_index);
	if ( rc != 0 ) {

		kprintf(KERN_ERR "minix_alloc_zone: No space on device %lx\n", sbp->dev);
		return -ENOSPC;
	}

	/* 割り当てたゾーン番号を返却する */
	/* @note Minixのファイルシステムのゾーン番号は1から始まるのに対し, 
	 * ビットマップのビットは0から割り当てられるので, Minixファイルシステムの
	 * フォーマット(mkfs)時点で, 0番のI-node/ゾーンビットマップは予約されいている
	 * それに対して, I-nodeテーブルやゾーンは1番のディスクI-node情報や
	 * ゾーンから開始される。従って, ビットマップのビット番号からゾーン番号の変換式は,
	 * ゾーン番号 = ビット番号 + sbp->s_firstdatazone - 1
	 * となる. 
	 * 参考: Minixのalloc_zoneのコメント
	 * Note that the routine alloc_bit() returns 1 for the lowest possible
	 * zone, which corresponds to sp->s_firstdatazone.  To convert a value
	 * between the bit number, 'b', used by alloc_bit() and the zone number, 'z',
	 * stored in the inode, use the formula:
	 *     z = b + sp->s_firstdatazone - 1
	 * Alloc_bit() never returns 0, since this is used for NO_BIT (failure).
	 */
	znum = MINIX_D_SUPER_BLOCK(sbp,s_firstdatazone) + (minix_zone)bitmap_index - 1;

	*zp = znum;

	return 0;
}

/**
   ゾーンを解放する
   @param[in]  sbp    Minixスーパブロック情報
   @param[in]  znum   ゾーン番号
   @retval     0      正常終了
   @retval    -ENOSPC 空きゾーンがない
 */
void
minix_free_zone(minix_super_block *sbp, minix_zone znum){
	minix_bitmap_idx  bitmap_index;

	/* ゾーン番号の健全性を確認 */
	if ( ( MINIX_D_SUPER_BLOCK(sbp, s_firstdatazone) > znum ) ||
	     ( znum >= MINIX_SB_ZONES_NR(sbp) ) )
		return;

	/* ゾーン内のブロックをクリアする
	 */
	minix_clear_zone(sbp, znum, 0, MINIX_ZONE_SIZE(sbp));

	/*
	 * ビットマップから割り当てを開放する
	 * ビットマップのビット番号からゾーン番号の変換式は,
	 * ゾーン番号 = ビット番号 + sbp->s_firstdatazone - 1
	 * であるので
	 * ビット番号 = ゾーン番号 -  sbp->s_firstdatazone + 1
	 * となる
	 */
	bitmap_index =
		(minix_bitmap_idx)(znum - MINIX_D_SUPER_BLOCK(sbp,s_firstdatazone) + 1);
	minix_bitmap_free(sbp, ZONE_MAP, bitmap_index);  /* ゾーンの割り当てを解放する */

	return;
}

/**
   間接参照ブロックを解析し間接参照ブロックから参照されているゾーンのゾーン番号を得る
   @param[in]  sbp            Minixスーパブロック情報
   @param[in]  ind_blk_znum   間接参照ブロックのゾーン番号
   @param[in]  ind_blk_ind    new_zoneの間接参照ブロック(ゾーン配列)インデクス値
   @param[out] dzonep       参照先データゾーン(または2段目の間接参照ブロック)番号返却領域
   @retval     0      正常終了
   @retval    -EINVAL 不正なスーパブロックを指定した
 */
int
minix_rd_indir(minix_super_block *sbp, minix_zone ind_blk_znum, 
    int ind_blk_ind, minix_zone *dzonep){
	int                      rc; /* 返り値 */
	page_cache              *pc; /* ページキャッシュ */
	size_t                pgsiz; /* ページキャッシュサイズ   */
	obj_cnt_type       mod_page; /* 操作対象ページアドレス(単位:バイト)   */
	off_t               mod_pos; /* クリア開始アドレス(単位:バイト)   */
	off_t               mod_off; /* ページ内オフセット (単位:バイト) */
	minix_zone         ref_zone; /* 参照先ゾーン */

	rc = pagecache_pagesize(sbp->dev, &pgsiz);  /* ページサイズ取得 */
	kassert( rc == 0 ); /* マウントされているはずなのでデバイスが存在する */

	/* インデクスの妥当性確認 */
	kassert( MINIX_ZONE_SIZE(sbp) > ( ind_blk_ind * MINIX_ZONE_NUM_SIZE(sbp) ) );
	
	/* 参照先アドレス */
	mod_pos = ind_blk_znum * MINIX_ZONE_SIZE(sbp) 
		+ ind_blk_ind * MINIX_ZONE_NUM_SIZE(sbp);
	
	/* 参照先アドレスの先頭ページアドレスを算出  */
	mod_page = truncate_align(mod_pos, pgsiz);
	
	/*  ページキャッシュを読み込み  */
	rc = pagecache_get(sbp->dev, mod_page, &pc);
	if ( rc != 0 ) {
		
		rc = -EIO;
		goto error_out;
	}
	
	/* ページ内クリア */
	mod_off = mod_pos % pgsiz;  /* ページ内オフセット */

	if ( MINIX_SB_IS_V2(sbp) ||  MINIX_SB_IS_V3(sbp) ) {

		if ( sbp->swap_needed )
			ref_zone = __bswap32( *(minixv2_zone *)(pc->pc_data + mod_off) );
		else
			ref_zone = *(minixv2_zone *)(pc->pc_data + mod_off);
	} else if ( MINIX_SB_IS_V1(sbp) ) {

		if ( sbp->swap_needed )
			ref_zone = __bswap16( *(minixv1_zone *)(pc->pc_data + mod_off) );
		else
			ref_zone = *(minixv1_zone *)(pc->pc_data + mod_off);
	} else {

		rc = -EINVAL;
		goto put_pcache_out;
	}

	pagecache_put(pc);  /* ページキャッシュを解放する  */

	if ( dzonep != NULL )
		*dzonep = ref_zone;

	return 0;

put_pcache_out:
	pagecache_put(pc);  /* ページキャッシュを解放する  */

error_out:
	return rc;
}

/**
   間接参照ブロックを更新する
   @param[in]  sbp            Minixスーパブロック情報
   @param[in]  ind_blk_znum   間接参照ブロックのゾーン番号
   @param[in]  ind_blk_ind    new_zoneの間接参照ブロック(ゾーン配列)インデクス値
   @param[in]  new_zone       割当て対象データゾーン(または2段目の間接参照ブロック)
   @retval     0      正常終了
   @retval    -EINVAL  不正なスーパブロックを指定した
   @retval    -EIO    ページキャッシュ操作に失敗した
 */
int
minix_wr_indir(minix_super_block *sbp, minix_zone ind_blk_znum, 
    int ind_blk_ind, minix_zone new_zone){
	int                      rc; /* 返り値 */
	page_cache              *pc; /* ページキャッシュ */
	size_t                pgsiz; /* ページキャッシュサイズ   */
	obj_cnt_type       mod_page; /* 操作対象ページアドレス(単位:バイト)   */
	off_t               mod_pos; /* クリア開始アドレス(単位:バイト)   */
	off_t               mod_off; /* ページ内オフセット (単位:バイト) */

	rc = pagecache_pagesize(sbp->dev, &pgsiz);  /* ページサイズ取得 */
	kassert( rc == 0 ); /* マウントされているはずなのでデバイスが存在する */

	/* インデクスの妥当性確認 */
	kassert( MINIX_ZONE_SIZE(sbp) > ( ind_blk_ind * MINIX_ZONE_NUM_SIZE(sbp) ) );
	
	/* 更新対象アドレス */
	mod_pos = ind_blk_znum * MINIX_ZONE_SIZE(sbp)
		+ ind_blk_ind * MINIX_ZONE_NUM_SIZE(sbp);
	
	/* 更新対象の先頭ページアドレスを算出  */
	mod_page = truncate_align(mod_pos, pgsiz);
	
	/*  ページキャッシュを読み込み  */
	rc = pagecache_get(sbp->dev, mod_page, &pc);
	if ( rc != 0 ) {
		
		rc = -EIO;
		goto error_out;
	}
	
	/* ページ内クリア */
	mod_off = mod_pos % pgsiz;  /* ページ内オフセット */

	if ( MINIX_SB_IS_V2(sbp) ||  MINIX_SB_IS_V3(sbp) ) {

		if ( sbp->swap_needed )
			*(minixv2_zone *)(pc->pc_data + mod_off) = __bswap32(new_zone);
		else
			*(minixv2_zone *)(pc->pc_data + mod_off) = new_zone;
	} else if ( MINIX_SB_IS_V1(sbp) ) {

		if ( sbp->swap_needed )
			*(minixv1_zone *)(pc->pc_data + mod_off) = __bswap16(new_zone);
		else
			*(minixv1_zone *)(pc->pc_data + mod_off) = new_zone;
	} else {

		rc = -EINVAL;
		goto put_pcache_out;
	}

	pagecache_put(pc);  /* ページキャッシュを解放する  */
		
	return 0;

put_pcache_out:
	pagecache_put(pc);  /* ページキャッシュを解放する  */

error_out:
	return rc;
}

/**
   間接参照ブロックを割当て指定されたゾーンへの参照をセットする
   @param[in] sbp      Minixスーパブロック情報
   @param[in] index    間接参照ブロックのゾーン配列インデクス
   @param[in] new_zone 間接参照ブロックから参照するゾーン
   @param[in] newblkp  割り当てた間接参照ブロックのゾーン番号返却域
   @retval     0       正常終了
   @retval    -ENOSPC  空きゾーンがない
   @retval    -EINVAL  不正なスーパブロックを指定した
   @retval    -EIO     ページキャッシュ操作に失敗した
 */
int
minix_alloc_indirect_block(minix_super_block *sbp, int index, minix_zone new_zone, 
			   minix_zone *newblkp){
	int                rc;
	minix_zone new_indblk;

	kassert( index >= 0 );
	kassert( MINIX_INDIRECTS(sbp) > index );

	/* 間接参照ブロックを割り当てる
	 */
	rc = minix_alloc_zone(sbp, &new_indblk);
	if ( rc != 0 ) 
		return rc;   /* 割当てに失敗したらエラー復帰する */

	/* ゾーン内のブロックをクリアする
	 */
	minix_clear_zone(sbp, new_indblk, 0, MINIX_ZONE_SIZE(sbp));

	/* 間接参照ブロックのindexの位置にnewzoneへの参照を書き込む
	 */
	rc = minix_wr_indir(sbp, new_indblk, index, new_zone);
	if ( rc != 0 )
		goto free_blk_out;

	return 0;

free_blk_out:
	minix_free_zone(sbp, new_indblk);  /* 割り当てた間接参照ブロックを解放する  */
	return rc;
}

/**
   インデクス値の算出
   @param[in] sbp Minixスーパブロック情報
   @param[in] position ファイル内でのオフセットアドレス(単位:バイト)
   @param[out] typep   ゾーン参照種別返却領域
   @param[out] zidxp   I-node順ファイルアクセス配列のインデクス番号返却領域
   @param[out] idx1stp 間接参照ブロック中のゾーン番号配列のインデクス番号返却領域
   @param[out] idx2ndp 2重間接参照ブロック中のゾーン番号配列のインデクス番号返却領域
   @retval     0       正常終了
   @retval    -E2BIG   ファイルサイズの上限を超えている
 */
int
minix_calc_indexes(minix_super_block *sbp, off_t position, 
		   int *typep, int *zidxp, int *idx1stp, int *idx2ndp){
	int                    rc;  /* 返り値 */
	int            index_type;  /* ゾーン参照種別                            */
	int                zindex;  /* I-node中のi_zone配列のインデクス          */
	int       first_ind_index;  /* 間接参照ブロックのゾーン配列インデクス    */
	int      second_ind_index;  /* 2重間接参照ブロックのゾーン配列インデクス */
	minix_zone       rel_zone;  /* ファイル先頭からの相対ゾーン番号          */

	/*
	 * 値を初期化する
	 */
	index_type = MINIX_ZONE_ADDR_NONE;
	zindex = MINIX_ZONE_INDEX_NONE;
	first_ind_index =  MINIX_ZONE_INDEX_NONE;
	second_ind_index =  MINIX_ZONE_INDEX_NONE;

	/*
	 * ファイル中の相対位置からminixのゾーン番号を算出する
	 */
	/* ファイル先頭からのブロック番号 */
	rel_zone = ( position/MINIX_BLOCK_SIZE(sbp) );
	/* ファイル先頭からのゾーン番号に変換 */
	rel_zone >>= MINIX_D_SUPER_BLOCK(sbp, s_log_zone_size); 

	/*  ゾーン番号からアドレッシング種別を分類し, 直接参照ブロック, 
	 *  単間接ブロックのインデクス
	 *  1段目の2重間接ブロックのインデクス
	 *  2段目の2重間接ブロックのインデクス
	 *  を算出する
	 */
	if ( MINIX_NR_DZONES(sbp) > rel_zone ) {
		
		/*
		 * 直接参照ブロック
		 */
		index_type = MINIX_ZONE_ADDR_DIRECT;
		zindex = (int)rel_zone;
	} else if ( (MINIX_INDIRECTS(sbp) * MINIX_NR_IND_ZONES(sbp) ) >
	    ( rel_zone - MINIX_NR_DZONES(sbp) ) ) {  /* 間接参照ブロック */
		
		rel_zone -= MINIX_NR_DZONES(sbp); /* ゾーンインデクスを更新 */

		/*
		 * 単間接参照ブロック中のインデクスを計算
		 */
		index_type = MINIX_ZONE_ADDR_SINGLE; /* 単間接参照 */
		/* inodeのi_zone配列中の単間接ブロック中のオフセットインデックスを
		 * 算出
		 */
		zindex = rel_zone / MINIX_INDIRECTS(sbp);
		first_ind_index = rel_zone % MINIX_INDIRECTS(sbp);
		kassert( MINIX_NR_IND_ZONES(sbp) > zindex );

		/* 単間接参照ブロック中のzone配列中のインデックスを算出  */
		zindex += MINIX_INDIRECT_ZONE_IDX(sbp); /* i_zone配列のインデクスに変換 */
	} else if ( ( MINIX_INDIRECTS(sbp) * MINIX_INDIRECTS(sbp) * MINIX_NR_DIND_ZONES(sbp) )
	    > ( rel_zone - ( MINIX_NR_DZONES(sbp) 
		    + MINIX_INDIRECTS(sbp) * MINIX_NR_IND_ZONES(sbp) ) ) ) {

		index_type = MINIX_ZONE_ADDR_DOUBLE;  /* 2重間接参照 */
		/* ゾーンインデクスを更新 */
		rel_zone -= MINIX_NR_DZONES(sbp) 
		    + MINIX_INDIRECTS(sbp) * MINIX_NR_IND_ZONES(sbp);
		
		/* inodeのi_zone配列中の2重間接ブロックのオフセット
		 * インデックスとオフセット位置を算出
		 */
		zindex = rel_zone / ( MINIX_INDIRECTS(sbp) * MINIX_INDIRECTS(sbp) );
		rel_zone = rel_zone % ( MINIX_INDIRECTS(sbp) * MINIX_INDIRECTS(sbp) );

		/* 1段目の2重間接参照ブロック中のzone配列中の
		 * インデックスを算出  
		 */
		first_ind_index = rel_zone  / MINIX_INDIRECTS(sbp);
		/* 2段目の2重間接参照ブロック中のzone配列中の
		 * インデックスを算出  
		 */
		second_ind_index = rel_zone % MINIX_INDIRECTS(sbp);
				
		kassert( MINIX_NR_DIND_ZONES(sbp) > zindex );
		kassert( MINIX_INDIRECTS(sbp) > first_ind_index  );
		kassert( MINIX_INDIRECTS(sbp) > second_ind_index );

		/* i_zone配列のインデクスに変換 */
		zindex += MINIX_DBL_INDIRECT_ZONE_IDX(sbp); 
	} else {

		rc = -E2BIG;  /*  最大ファイル長を越えた */
		goto error_out;
	}

	kassert( index_type !=  MINIX_ZONE_ADDR_NONE );
	kassert( MINIX_NR_TZONES(sbp) > zindex  );
	kassert( ( first_ind_index == MINIX_ZONE_INDEX_NONE ) ||
	    ( MINIX_INDIRECTS(sbp) > first_ind_index ) );
	kassert(  ( second_ind_index == MINIX_ZONE_INDEX_NONE ) || 
	    ( MINIX_INDIRECTS(sbp) > second_ind_index ) );

	*typep = index_type;
	*zidxp = zindex;
	*idx1stp = first_ind_index;
	*idx2ndp = second_ind_index;

	return 0;

error_out:
	return rc;
}

/** 
    ファイル中の指定されたオフセット位置にゾーンを割り当てる
    @param[in] dip      MinixディスクI-node情報
    @param[in] position ファイル内でのオフセットアドレス(単位:バイト)
    @param[in] new_zone 割り当てるゾーン
    @retval     0       正常終了
    @retval    -ENOSPC  空きゾーンがない
    @retval    -EINVAL  不正なスーパブロックを指定した
    @retval    -EIO     ページキャッシュ操作に失敗した
    @note      new_zoneはビットマップから獲得済みでなければならない
 */
int
minix_write_mapped_block(minix_inode *dip, off_t position, minix_zone new_zone){
	int                    rc;  /* Return code */
	int            index_type;  /* indexing way to point a data zone */
	int                zindex;  /* index for direct zone in i_zone array in an inode */
	int       first_ind_index;  /* 1st indirect index for direct zone in i_zone array */
	int      second_ind_index;  /* 2nd indirect index for direct zone in i_zone array */
	minix_zone new_1st_indblk;  /* newly allocated zone for 1st indirect block */
	minix_zone new_2nd_indblk;  /* newly allocated zone for 2nd indirect block */
	minix_zone   cur_data_blk;  /* Already allocated zone for data block 
				     *  ( in double indirect block )  
				     */
	minix_zone cur_2nd_indblk;  /* Already allocated zone for 2nd indirect block */

	/*
	 *  ゾーン番号からアドレッシング種別を分類し, 直接参照ブロック, 
	 *  単間接ブロックのインデクス
	 *  1段目の2重間接ブロックのインデクス
	 *  2段目の2重間接ブロックのインデクス
	 *  を算出する
	 */
	rc = minix_calc_indexes(dip->sbp, position, &index_type, 
				&zindex, &first_ind_index, &second_ind_index);
	if ( rc != 0 )
		goto error_out;  /*  E2BIGを返却する  */

	kassert( MINIX_NR_TZONES(dip->sbp) > zindex );

	if ( index_type == MINIX_ZONE_ADDR_DIRECT ) {

		/*
		 * 直接参照ブロック
		 */
		MINIX_D_INODE_SET(dip, i_zone[zindex], new_zone);
		goto success;
	}

	/*
	 * 単間接参照ブロック, 2重間接参照ブロックの割り当て
	 */
	new_1st_indblk = MINIX_NO_ZONE(dip->sbp);
	new_2nd_indblk = MINIX_NO_ZONE(dip->sbp);
	cur_2nd_indblk = MINIX_NO_ZONE(dip->sbp);

	if ( MINIX_D_INODE(dip, i_zone[zindex]) == MINIX_NO_ZONE(dip->sbp) ) {

		/* 単間接参照ブロック, 2重間接ブロックの1段目のブロックが
		 * 未割当ての場合
		 */

		if ( index_type == MINIX_ZONE_ADDR_SINGLE ) {

			/*
			 * 単間接参照ブロックの割当て
			 */
			rc = minix_alloc_indirect_block(dip->sbp, first_ind_index, 
			    new_zone, &new_1st_indblk);
		} else {
			
			/*
			 * 2重間接ブロックの2段目のブロックの割当て
			 */
			kassert( second_ind_index >= 0 );
			rc = minix_alloc_indirect_block(dip->sbp, second_ind_index, 
			    new_zone, &new_2nd_indblk);
			if ( rc != 0 )
				goto error_out; /*  割当に失敗した */

			/*
			 * 2重間接ブロックの1段目のブロックの割当て
			 */
			kassert( new_2nd_indblk != MINIX_NO_ZONE(dip->sbp) );
			rc = minix_alloc_indirect_block(dip->sbp, first_ind_index, 
			    new_2nd_indblk, &new_1st_indblk);
		}
		if ( rc != 0 ) {
			
			/*
			 * 単間接参照ブロック, 2重間接ブロックの1段目のブロックの
			 * 割当てに失敗した
			 */
			if ( index_type == MINIX_ZONE_ADDR_SINGLE ) 
				goto error_out; /* エラー復帰する */
			else {

				/* 2重間接ブロックの場合は, 
				 * 2段目のブロックを開放してから
				 * エラー復帰する 
				 */
				kassert(new_2nd_indblk != MINIX_NO_ZONE(dip->sbp) );
				minix_free_zone(dip->sbp, new_2nd_indblk);
				goto error_out; 
			}
		}
		/* 1段目のブロックへの参照を記録 */
		kassert( new_1st_indblk != MINIX_NO_ZONE(dip->sbp) );
		MINIX_D_INODE_SET(dip, i_zone[zindex], new_1st_indblk); 
	} else {  /* 単間接参照ブロック, 2重間接ブロックの1段目のブロックが割当済み  */

		/* 単間接参照ブロック中のデータブロック/2段目のブロックへの参照を取得 */
		minix_rd_indir(dip->sbp, MINIX_D_INODE(dip, i_zone[zindex]), 
		    first_ind_index, &cur_2nd_indblk);
		if ( index_type == MINIX_ZONE_ADDR_SINGLE ) {

			/*  データブロックへの参照を記録 */
			kassert( cur_2nd_indblk == MINIX_NO_ZONE(dip->sbp) );
			minix_wr_indir(dip->sbp, MINIX_D_INODE(dip, i_zone[zindex]), 
			    first_ind_index, new_zone);			
		} else {  /*  2重間接ブロックの場合  */
			
			if ( cur_2nd_indblk !=  MINIX_NO_ZONE(dip->sbp) ) {
			
				/*
				 *  2重間接ブロックの2段目のブロックが割り当て済みの場合
				 */

				/* 2段目のブロック中のデータブロックへの参照を取得 */
				minix_rd_indir(dip->sbp, cur_2nd_indblk, 
				    second_ind_index, &cur_data_blk);

				/*  データブロックへの参照を記録 */
				kassert( cur_data_blk == MINIX_NO_ZONE(dip->sbp) );
				minix_wr_indir(dip->sbp, cur_2nd_indblk, 
				    second_ind_index, new_zone);
			} else {

				/*  2重間接ブロックの2段目のブロックを割り当てて
				 *  データブロックへの参照を記録
				 */
				rc = minix_alloc_indirect_block(dip->sbp, second_ind_index, 
				    new_zone, &new_2nd_indblk);
				if ( rc != 0 ) {

					/* 割当てに失敗したら
					 * 1段目のブロック中の2段目のブロックへの
					 * 参照を消去
					 */
					minix_wr_indir(dip->sbp, 
					    MINIX_D_INODE(dip, i_zone[zindex]), 
					    first_ind_index, MINIX_NO_ZONE(dip->sbp));
					goto error_out;
				}
			}
		}
	}

success:
	return 0;

error_out:
	return rc;
}


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

	/* ゾーン番号の健全性を確認 */
	kassert( ( znum >= MINIX_D_SUPER_BLOCK(sbp, s_firstdatazone) ) &&
	    ( MINIX_SB_ZONES_NR(sbp) > znum ) );

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
   間接参照ブロックを解析し間接参照ブロックが空であることを確認する
   @param[in]  sbp            Minixスーパブロック情報
   @param[in]  ind_blk_znum   間接参照ブロックのゾーン番号
   @retval     真             間接参照ブロックが空である
   @retval     偽             間接参照ブロックが空でない
 */
bool
minix_is_empty_indir(minix_super_block *sbp, minix_zone ind_blk_znum){
	int                      rc; /* 返り値 */
	int                       i; /* ゾーン配列インデクス                    */
	page_cache              *pc; /* ページキャッシュ                        */
	size_t                pgsiz; /* ページキャッシュサイズ                  */
	obj_cnt_type       mod_page; /* 操作対象ページアドレス(単位:バイト)     */
	obj_cnt_type     first_page; /* 開始ページアドレス(単位:バイト)         */
	obj_cnt_type       end_page; /* 終了ページアドレス(単位:バイト)         */
	off_t               cur_pos; /* 参照先アドレス(単位:バイト)             */
	minix_zone         ref_zone; /* 参照先ゾーン */

	/* ゾーン番号の健全性を確認 */
	kassert( ind_blk_znum >= ( MINIX_D_SUPER_BLOCK(sbp, s_firstdatazone) ) &&
		 ( MINIX_SB_ZONES_NR(sbp) > ind_blk_znum ) );

	rc = pagecache_pagesize(sbp->dev, &pgsiz);  /* ページサイズ取得 */
	kassert( rc == 0 ); /* マウントされているはずなのでデバイスが存在する */

	/* 参照先アドレスを算出 */
	cur_pos = ind_blk_znum * MINIX_ZONE_SIZE(sbp);

	/* 先頭ページアドレスを算出  */
	first_page = truncate_align(cur_pos, pgsiz);

	/* 最終ページアドレスを算出  */
	end_page =  roundup_align(cur_pos + MINIX_ZONE_SIZE(sbp), pgsiz);
	
	/* 参照先アドレス */
	for(mod_page = first_page; end_page > mod_page; mod_page += pgsiz){

		/*  ページキャッシュを読み込み  */
		rc = pagecache_get(sbp->dev, mod_page, &pc);
		if ( rc != 0 ) 
			goto error_out;
		
		for(i = 0; pgsiz / MINIX_ZONE_NUM_SIZE(sbp) > i; ++i) {

			/* 間接参照ブロック中のゾーン番号配列の
			 * 全てのエントリが空であることを確認する
			 */
			if ( MINIX_SB_IS_V2(sbp) || MINIX_SB_IS_V3(sbp) ) {

				if ( sbp->swap_needed )
					ref_zone =
						__bswap32( *(minixv2_zone *)
							   ( pc->pc_data + cur_pos % pgsiz ) );
				else
					ref_zone = *(minixv2_zone *)
						( pc->pc_data + cur_pos % pgsiz );
			} else if ( MINIX_SB_IS_V1(sbp) ) {

				if ( sbp->swap_needed )
					ref_zone =
						__bswap16( *(minixv1_zone *)
							   ( pc->pc_data + cur_pos % pgsiz) );
				else
					ref_zone = *(minixv1_zone *)
						( pc->pc_data + cur_pos % pgsiz );

			}

			if ( ref_zone != MINIX_NO_ZONE(sbp) )
				goto put_pcache_out;  /* 空でないゾーン */
			cur_pos += MINIX_ZONE_NUM_SIZE(sbp); /* 次の要素へ */
		}
		pagecache_put(pc);  /* ページキャッシュを解放する  */
	}

	return true;

put_pcache_out:
	pagecache_put(pc);  /* ページキャッシュを解放する  */

error_out:
	return false;
}

/**
   間接参照ブロックを解析し間接参照ブロックから参照されているゾーンのゾーン番号を得る
   @param[in]  sbp            Minixスーパブロック情報
   @param[in]  ind_blk_znum   間接参照ブロックのゾーン番号
   @param[in]  ind_blk_ind    new_zoneの間接参照ブロック(ゾーン配列)インデクス値
   @param[out] dzonep       参照先データゾーン(または2段目の間接参照ブロック)番号返却領域
   @retval     0      正常終了
   @retval    -EINVAL 不正なスーパブロックを指定した
   @retval    -EIO    ページキャッシュの読み取りに失敗した
 */
int
minix_rd_indir(minix_super_block *sbp, minix_zone ind_blk_znum, 
    int ind_blk_ind, minix_zone *dzonep){
	int                      rc; /* 返り値 */
	page_cache              *pc; /* ページキャッシュ                      */
	size_t                pgsiz; /* ページキャッシュサイズ                */
	obj_cnt_type       mod_page; /* 操作対象ページアドレス(単位:バイト)   */
	off_t               mod_pos; /* クリア開始アドレス(単位:バイト)       */
	off_t               mod_off; /* ページ内オフセット (単位:バイト)      */
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

	*newblkp = new_indblk;

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
   ファイル中の指定されたオフセット位置に対応したゾーン番号を取得する
   @param[in]  sbp      Minixスーパブロック情報
   @param[in]  dip      MinixディスクI-node情報
   @param[in]  position ファイル内でのオフセットアドレス(単位:バイト)
   @param[out] zonep    ゾーン番号返却域
   @retval     0        正常終了
   @retval    -ENOENT   ゾーンが割り当てられていない
   @retval    -E2BIG    ファイルサイズの上限を超えている
   @retval    -EIO    ページキャッシュの読み取りに失敗した
 */
int
minix_read_mapped_block(minix_super_block *sbp, minix_inode *dip, off_t position, minix_zone *zonep){
	int                    rc;  /* 返り値 */
	minix_zone     found_zone;  /* 見つかったゾーン番号                      */
	int            index_type;  /* ゾーン参照種別                            */
	int                zindex;  /* I-node中のi_zone配列のインデクス          */
	int       first_ind_index;  /* 間接参照ブロックのゾーン配列インデクス    */
	int      second_ind_index;  /* 2重間接参照ブロックのゾーン配列インデクス */
	minix_zone cur_1st_indblk;  /* 1段目の間接参照ブロックゾーン番号         */
	minix_zone cur_2nd_indblk;  /* 2段目の間接参照ブロックゾーン番号         */

	/*
	 *  ゾーン番号からアドレッシング種別を分類し, 直接参照ブロック, 
	 *  単間接ブロックのインデクス
	 *  1段目の2重間接ブロックのインデクス
	 *  2段目の2重間接ブロックのインデクス
	 *  を算出する
	 */
	rc = minix_calc_indexes(sbp, position, &index_type, 
				&zindex, &first_ind_index, &second_ind_index);
	if ( rc != 0 )
		goto error_out;  /*  E2BIGを返却する  */
	
	found_zone = MINIX_D_INODE(sbp, dip, i_zone[zindex]);
	if ( found_zone == MINIX_NO_ZONE(sbp) ) {
		
		rc = -ENOENT;  
		goto error_out; /*  ゾーンが未割り当て  */  
	}

	/*
	 * 直接参照ブロック
	 */
	if ( index_type == MINIX_ZONE_ADDR_DIRECT ) 
		goto success;

	/*
	 * 単間接ブロック/2重間接ブロック
	 */
	cur_1st_indblk = found_zone;

	/*  間接参照ブロックからデータブロックまたは2段目の間接参照ブロックを取得  */
	rc = minix_rd_indir(sbp, cur_1st_indblk, first_ind_index, &found_zone);
	kassert( rc != -EINVAL );  /* 不正スーパブロック情報が引き渡されることはない */
	if ( rc != 0 )
		goto error_out;  /* ページキャッシュ読み取りに失敗した */

	if ( found_zone == MINIX_NO_ZONE(sbp) ) {
		
		rc = -ENOENT;  /*  未割り当て  */
		goto error_out;
	}

	/*
	 * 単間接ブロック
	 */
	if ( index_type == MINIX_ZONE_ADDR_SINGLE ) 
		goto success;

	/*
	 * 2重間接ブロックの場合
	 */
	cur_2nd_indblk = found_zone;

	/*  間接参照ブロックからデータブロックを取得  */
	rc = minix_rd_indir(sbp, cur_2nd_indblk, second_ind_index, &found_zone);
	kassert( rc != -EINVAL );  /* 不正スーパブロック情報が引き渡されることはない */
	if ( rc != 0 )
		goto error_out;  /* ページキャッシュ読み取りに失敗した */

	if ( found_zone  == MINIX_NO_ZONE(sbp) ) {
		
		rc = -ENOENT;  /*  未割り当て  */
		goto error_out;
	}

success:
	if ( zonep != NULL )
		*zonep = found_zone;  /* ゾーン番号を返却 */

	return 0;

error_out:
	return rc;
}

/** 
    ファイル中の指定されたオフセット位置にゾーンを割り当てる
    @param[in] sbp      Minixスーパブロック情報
    @param[in] dip      MinixディスクI-node情報
    @param[in] position ファイル内でのオフセットアドレス(単位:バイト)
    @param[in] new_zone 割り当てるゾーン
    @retval     0       正常終了
    @retval    -E2BIG   ファイルサイズの上限を超えている
    @retval    -ENOSPC  空きゾーンがない
    @retval    -EINVAL  不正なスーパブロックを指定した
    @retval    -EIO     ページキャッシュ操作に失敗した
    @note      new_zoneはビットマップから獲得済みでなければならない
 */
int
minix_write_mapped_block(minix_super_block *sbp, minix_inode *dip, off_t position,
    minix_zone new_zone){
	int                    rc;  /* 返り値                                            */
	int            index_type;  /* ゾーン参照種別                                    */
	int                zindex;  /* I-node中のi_zone配列のインデクス                  */
	int       first_ind_index;  /* 間接参照ブロックのゾーン配列インデクス            */
	int      second_ind_index;  /* 2重間接参照ブロックのゾーン配列インデクス         */
	minix_zone   cur_data_blk;  /* 割り当て済みのデータブロック                      */
	minix_zone new_1st_indblk;  /* 新規に割り当てた1段目の間接参照ブロックゾーン番号 */
	minix_zone new_2nd_indblk;  /* 新規に割り当てた2段目の間接参照ブロックゾーン番号 */
	minix_zone cur_1st_indblk;  /* 割り当て済みの1段目の間接参照ブロックゾーン番号   */
	minix_zone cur_2nd_indblk;  /* 割り当て済みの2段目の間接参照ブロックゾーン番号   */

	/*
	 *  ゾーン番号からアドレッシング種別を分類し, 直接参照ブロック, 
	 *  単間接ブロックのインデクス
	 *  1段目の2重間接ブロックのインデクス
	 *  2段目の2重間接ブロックのインデクス
	 *  を算出する
	 */
	rc = minix_calc_indexes(sbp, position, &index_type, 
				&zindex, &first_ind_index, &second_ind_index);
	if ( rc != 0 )
		goto error_out;  /*  E2BIGを返却する  */

	kassert( MINIX_NR_TZONES(sbp) > zindex );

	if ( index_type == MINIX_ZONE_ADDR_DIRECT ) {

		/*
		 * 直接参照ブロック
		 */
		MINIX_D_INODE_SET(sbp, dip, i_zone[zindex], new_zone);
		goto success;
	}

	/*
	 * 単間接参照ブロック, 2重間接参照ブロックの割り当て
	 */
	new_1st_indblk = MINIX_NO_ZONE(sbp);
	new_2nd_indblk = MINIX_NO_ZONE(sbp);
	cur_2nd_indblk = MINIX_NO_ZONE(sbp);

	if ( MINIX_D_INODE(sbp, dip, i_zone[zindex]) == MINIX_NO_ZONE(sbp) ) {

		/* 単間接参照ブロック, 2重間接ブロックの1段目のブロックが
		 * 未割当ての場合
		 */		
		if ( index_type == MINIX_ZONE_ADDR_SINGLE ) {

			/*
			 * 単間接参照データブロックを更新
			 */
			rc = minix_alloc_indirect_block(sbp, first_ind_index, 
			    new_zone, &new_1st_indblk);
		} else {
			
			/*
			 * 2重間接ブロックの2段目のブロックの割当て
			 */
			kassert( second_ind_index >= 0 );
			rc = minix_alloc_indirect_block(sbp, second_ind_index, 
			    new_zone, &new_2nd_indblk);
			if ( rc != 0 )
				goto error_out; /*  割当に失敗した */

			/*
			 * 2重間接ブロックの1段目のブロックの割当て
			 */
			kassert( new_2nd_indblk != MINIX_NO_ZONE(sbp) );
			rc = minix_alloc_indirect_block(sbp, first_ind_index, 
			    new_2nd_indblk, &new_1st_indblk);
		}
		if ( rc != 0 ) {
			
			/*
			 * 単間接参照ブロック, 2重間接ブロックの1段目のブロックの
			 * 割当てに失敗した
			 */
			if ( index_type == MINIX_ZONE_ADDR_SINGLE ) {

				if ( new_zone == MINIX_NO_ZONE(sbp) )
					goto success; /* 正常終了 */

				goto error_out; /* エラー復帰する */
			} else {

				/* 2重間接ブロックの場合は, 
				 * 2段目のブロックを開放してから
				 * エラー復帰する 
				 */
				kassert(new_2nd_indblk != MINIX_NO_ZONE(sbp) );
				minix_free_zone(sbp, new_2nd_indblk);
				goto error_out; 
			}
		}
		/* 1段目のブロックへの参照を更新 */
		kassert( new_1st_indblk != MINIX_NO_ZONE(sbp) );
		MINIX_D_INODE_SET(sbp, dip, i_zone[zindex], new_1st_indblk); 
	} else {  /* 単間接参照ブロック, 2重間接ブロックの1段目のブロックが割当済み  */

		/* 単間接参照ブロック中のデータブロック/2段目のブロックへの参照を取得 */
		rc = minix_rd_indir(sbp, MINIX_D_INODE(sbp, dip, i_zone[zindex]), 
				    first_ind_index, &cur_2nd_indblk);
		/* 不正スーパブロック情報が引き渡されることはない */
		kassert( rc != -EINVAL ); 
		if ( rc != 0 )
			goto error_out;  /* ページキャッシュ読み取りに失敗した */

		if ( index_type == MINIX_ZONE_ADDR_SINGLE ) {
			
			/*  データブロックへの参照を更新 */
			kassert( ( cur_2nd_indblk == MINIX_NO_ZONE(sbp) ) ||
				 ( new_zone == MINIX_NO_ZONE(sbp) ) );
			minix_wr_indir(sbp, MINIX_D_INODE(sbp, dip, i_zone[zindex]), 
				       first_ind_index, new_zone);

			/* ゾーン解放の場合で, かつ, 間接参照ブロックが空なら
			 * 間接参照ブロックを解放する
			 */
			if ( minix_is_empty_indir(sbp, 
						  MINIX_D_INODE(sbp, dip, i_zone[zindex])) ) {

				cur_1st_indblk = MINIX_D_INODE(sbp, dip, i_zone[zindex]);
				MINIX_D_INODE_SET(sbp, dip, i_zone[zindex], MINIX_NO_ZONE(sbp));
				minix_free_zone(sbp, cur_1st_indblk);
			}
		} else {  

			/*
			 * 2重間接ブロックの1段目のブロックが割当済みの場合
			 */
			if ( cur_2nd_indblk !=  MINIX_NO_ZONE(sbp) ) { 
			
				/*
				 *  2重間接ブロックの2段目のブロックが割り当て済みの場合
				 */

				/* 2段目のブロック中のデータブロックへの参照を取得 */
				rc = minix_rd_indir(sbp, cur_2nd_indblk, 
				    second_ind_index, &cur_data_blk);
				/* 不正スーパブロック情報が引き渡されることはない */
				kassert( rc != -EINVAL );
				if ( rc != 0 )
					goto error_out; /* ページキャッシュ読み取り失敗 */

				/*  データブロックへの参照を更新 */
				kassert( ( cur_data_blk == MINIX_NO_ZONE(sbp) ) ||
					 ( new_zone == MINIX_NO_ZONE(sbp) ) );
				minix_wr_indir(sbp, cur_2nd_indblk, 
				    second_ind_index, new_zone);

				/* ゾーン解放の場合で, かつ, 間接参照ブロックが空なら
				 * 2段目の間接参照ブロックを解放する
				 */
				if ( minix_is_empty_indir(sbp, cur_2nd_indblk) ) {

					/* 1段目のブロック中の2段目のブロックへの
					 * 参照を消去
					 */
					cur_1st_indblk = MINIX_D_INODE(sbp, dip, i_zone[zindex]);
					minix_wr_indir(sbp, cur_1st_indblk,
					    first_ind_index, MINIX_NO_ZONE(sbp));
					/* 2段目の間接参照ブロックを解放する */
					minix_free_zone(sbp, cur_2nd_indblk);
					if ( minix_is_empty_indir(sbp, cur_1st_indblk) ) {

						/* 1段目の間接参照ブロックが空になったら
						 * 間接参照ブロックへの参照をクリアして
						 * 間接参照ブロックを解放
						 */
						MINIX_D_INODE_SET(sbp, dip, i_zone[zindex], 
								  MINIX_NO_ZONE(sbp));
						minix_free_zone(sbp, cur_1st_indblk);
					}
				}
			} else {
				/* 2重間接参照ブロックの場合で,
				 * かつ,
				 * 2重間接ブロックの1段目のブロックが割当済みの場合で,
				 * かつ,
				 * 2重間接ブロックの2段目のブロックが割り当てられておらず,
				 * かつ,
				 * データブロックの解放指示だった場合,
				 */
				if ( new_zone == MINIX_NO_ZONE(sbp) ) { 

					/* 1段目のブロック中の2段目のブロックへの
					 * 参照を消去
					 */
					cur_1st_indblk = MINIX_D_INODE(sbp, dip, i_zone[zindex]);
					minix_wr_indir(sbp, cur_1st_indblk, 
					    first_ind_index, MINIX_NO_ZONE(sbp));
					if ( minix_is_empty_indir(sbp,
								  cur_1st_indblk) ) {

						/* 1段目の間接参照ブロックが空になったら
						 * 間接参照ブロックへの参照をクリアして
						 * 間接参照ブロックを解放
						 */
						MINIX_D_INODE_SET(sbp, dip, i_zone[zindex],
								  MINIX_NO_ZONE(sbp));
						minix_free_zone(sbp, cur_1st_indblk);
					}

					goto success;
				}
					
				/*  2重間接ブロックの2段目のブロックを割り当てて
				 *  データブロックへの参照を更新
				 */
				rc = minix_alloc_indirect_block(sbp, second_ind_index, 
				    new_zone, &new_2nd_indblk);
				if ( rc != 0 ) {

					/* 割当てに失敗したら
					 * 1段目のブロック中の2段目のブロックへの
					 * 参照を消去
					 */
					cur_1st_indblk = MINIX_D_INODE(sbp, dip, i_zone[zindex]);
					minix_wr_indir(sbp, cur_1st_indblk,
					    first_ind_index, MINIX_NO_ZONE(sbp));
					if ( minix_is_empty_indir(sbp,
								  cur_1st_indblk) ) {
						
						/* 1段目の間接参照ブロックが空になったら
						 * 間接参照ブロックへの参照をクリアして
						 * 間接参照ブロックを解放
						 */
						MINIX_D_INODE_SET(sbp, dip, i_zone[zindex],
								  MINIX_NO_ZONE(sbp));
						minix_free_zone(sbp, cur_1st_indblk);
					}
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

/**
   MinixのI-nodeを元にデータブロックを読み書きする
   @param[in]   sbp        Minixスーパブロック情報
   @param[in]   i_num      Minix I-node番号
   @param[in]   dip        MinixディスクI-node
   @param[in]   kpage      ファイルストリーム格納先カーネル仮想アドレス
   @param[in]   off        ファイルストリーム開始オフセット
   @param[in]   len        読み書き長(単位: バイト)
   @param[in]   rw_flag    読み書き種別
   - MINIX_RW_READING 読み取り
   - MINIX_RW_WRITING 書き込み
   @param[in]   rwlenp     読み書きした長さ(単位: バイト)
   @retval      0          正常終了
   @retval     -ENOENT     ゾーンが割り当てられていない
   @retval     -E2BIG      ファイルサイズの上限を超えている
   @retval     -EIO        ページキャッシュアクセスに失敗した
   @retval     -ENOSPC     空きゾーンがない
   @retval     -EINVAL     不正なスーパブロックを指定した
   @note TODO: 更新時間更新
 */
int
minix_rw_zone(minix_super_block *sbp, minix_ino i_num, minix_inode *dip, void *kpage,
    off_t off, ssize_t len, int rw_flag, ssize_t *rwlenp){
	int           rc;
	ssize_t  remains;
	ssize_t    total;
	page_cache   *pc;
	off_t    cur_pos;
	off_t     pg_pos;
	off_t     pg_off;
	size_t     pgsiz;
	ssize_t    rwsiz;
	minix_zone  zone;

	if ( ( off > MINIX_D_INODE(sbp, dip, i_size) ) || ( off > ( off + len ) ) ) {

		if ( rwlenp != NULL )
			*rwlenp = 0;
		goto success;  /* EOF */
	}

	rc = pagecache_pagesize(sbp->dev, &pgsiz);  /* ページサイズ取得 */
	kassert( rc == 0 ); /* マウントされているはずなのでデバイスが存在する */

	if ( rw_flag == MINIX_RW_READING )
		total = MIN(len, MINIX_D_INODE(sbp, dip, i_size) - off);  /* 読み込みサイズ */
	else
		total = len;  /* 書き込みサイズ */

	remains = total; /* 残転送サイズ */

	for(cur_pos = off; remains > 0; remains -= rwsiz, cur_pos += rwsiz ){

		rc = minix_read_mapped_block(sbp, dip, cur_pos, &zone);
		if ( rc != 0 ) {
			
			/* ゾーンが割り当てられていない場合は, 
			 * 新たにゾーンを割り当てる 
			 * @note 読取りの場合, 上記で転送サイズをファイルサイズ内に
			 * 収めるように補正しているので, 読取りの場合で, かつ, 
			 * ゾーンが割り当てられていない場合でも, ゼロクリアした
			 * ゾーンを割り当てればよい(パンチホールアクセスケース)
			 */
			rc = minix_alloc_zone(sbp, &zone);  /* データゾーンを割当てる */
			if ( rc != 0 ) 
				goto error_out;

			/* ゾーン内のブロックをクリアする
			 * 本ファイルシステムで使用されたデータブロックは
			 * データブロックの解放時にクリアされるためゼロクリアを
			 * 保証できるが, 新規に割り当てたゾーンのディスクブロックが
			 * 過去に他の用途で使われていた場合そのデータが読めてしまうため
			 * 念のためゾーンをクリアしてからマップする
			 */
			minix_clear_zone(sbp, zone, 0, MINIX_ZONE_SIZE(sbp));

			/* ゾーンをマップする */
			rc = minix_write_mapped_block(sbp, dip, cur_pos, zone);
			if ( rc != 0 ) {  /* 割り当てたゾーンをマップできなかった */

				/* 割当てたデータゾーンを解放する  */
				minix_free_zone(sbp, zone);
				goto error_out;
			}
		}
		
		/* 読み書き開始ページを算出する
		 */
		pg_pos = zone *	( MINIX_BLOCK_SIZE(sbp) 
				  << MINIX_D_SUPER_BLOCK(sbp, s_log_zone_size) );
		pg_pos = truncate_align(pg_pos, pgsiz);  /* ページ先頭アドレス */

		/* ページキャッシュロード */
		rc = pagecache_get(sbp->dev, pg_pos, &pc);
		if ( rc != 0 )
			goto error_out;	

		pg_off = cur_pos % pgsiz;  /* ページキャッシュ内オフセット */
		rwsiz = MIN(remains, pgsiz - pg_off);  /* ページ内の読み書き可能量 */
		if ( rw_flag == MINIX_RW_READING )
			memmove(kpage, pc->pc_data + pg_off, rwsiz);
		else
			memmove(pc->pc_data + pg_off, kpage, rwsiz);

		pagecache_put(pc);  /* ページキャッシュを解放  */	
	}

	if ( total != remains ) {  /* 読み書きを行った場合 */

		if ( i_num != MINIX_NO_INUM(sbp) ) {  /* I-nodeの更新を行う場合 */

			if ( rw_flag == MINIX_RW_WRITING ) {

				/* サイズ更新 */
				if ( ( off + total - remains ) 
				    > ( MINIX_D_INODE(sbp, dip, i_size) ) ) 
					MINIX_D_INODE_SET(sbp, dip, i_size,
					    off + ( total - remains ) ); 

				/* TODO: 更新時刻更新 */
			} else {
			
				/* TODO: 参照時刻更新 */
			}

			/* I-node情報を更新 */
			rc = minix_rw_disk_inode(sbp, i_num, MINIX_RW_WRITING, dip);
			if ( rc != 0 ) 
				goto error_out;	
		}

		if ( rwlenp != NULL )
			*rwlenp = total - remains;  /* 読み書きを行えたサイズを返却 */
	}
success:
	return 0;

error_out:
	return rc;
}

/**
   MinixのI-nodeを元にデータブロックを解放する
   @param[in]   sbp        Minixスーパブロック情報
   @param[in]   i_num      Minix I-node番号
   @param[in]   dip        MinixディスクI-node
   @param[in]   off        解放開始位置のオフセット(単位: バイト)
   @param[in]   len        解放長(単位: バイト)
   @retval      0          正常終了
   @retval     -EINVAL     offに負の値を指定した
   @retval     -EFBIG      ファイル長よりoffの方が大きい
   @note TODO: 更新時間更新
 */
int
minix_unmap_zone(minix_super_block *sbp, minix_ino i_num, minix_inode *dip, off_t off, 
    ssize_t len){
	int                    rc;
	size_t              pgsiz;
	off_t             cur_pos;
	size_t            clr_siz;
	size_t          clr_start;
	minix_zone   cur_rel_zone;
	minix_zone first_rel_zone;
	minix_zone   end_rel_zone;
	minix_zone    remove_zone;

	if ( len == 0 )
		return 0;  /* 削除不要 */

	if ( ( 0 > off ) || ( 0 > len ) )
		return -EINVAL;  /*  オフセットまたは解放長に負の値を設定した  */

	if ( ( off > MINIX_D_INODE(sbp, dip, i_size) ) ||
	    ( off > ( off + len ) ) )
		return -EFBIG;  /* ファイル長よりオフセットの方が大きい */

	 /* ファイルの終端を越える場合は, 削除長をファイル終端に補正する */
	if ( ( off + len ) > MINIX_D_INODE(sbp, dip, i_size) )
		len = MINIX_D_INODE(sbp, dip, i_size) - off;

	rc = pagecache_pagesize(sbp->dev, &pgsiz);  /* ページサイズ取得 */
	kassert( rc == 0 ); /* マウントされているはずなのでデバイスが存在する */

	first_rel_zone = 
		truncate_align(off, MINIX_ZONE_SIZE(sbp))
		/ MINIX_ZONE_SIZE(sbp);    /* 開始ゾーン     */

	end_rel_zone = roundup_align(off + len, MINIX_ZONE_SIZE(sbp))
		/ MINIX_ZONE_SIZE(sbp); /* 終了ゾーン     */

	/* 終了位置がゾーン境界と合っていない場合
	 */
	if ( ( end_rel_zone > first_rel_zone ) 
	    && ( addr_not_aligned(off + len, MINIX_ZONE_SIZE(sbp)) ) )
		--end_rel_zone;  /* 終了ゾーンを減算 */

	cur_pos = off; /* 削除開始位置   */
	cur_rel_zone = first_rel_zone;  /* 削除開始ゾーン */

	/* 開始点がゾーン境界と合っておらず, 後続のゾーンもクリアする場合
	 */
	if ( addr_not_aligned(off, MINIX_ZONE_SIZE(sbp) ) 
	    && ( end_rel_zone > first_rel_zone ) ) {

		/* クリア開始オフセットを算出 */
		clr_start = cur_pos % MINIX_ZONE_SIZE(sbp);
		/* クリアサイズを算出 */
		clr_siz = MINIX_ZONE_SIZE(sbp) - clr_start;

		/* デバイス先頭からのゾーン番号を算出
		 */
		rc = minix_read_mapped_block(sbp, dip, 
		    first_rel_zone * MINIX_ZONE_SIZE(sbp), &remove_zone);

		/* 開始ゾーン内をクリアする */
		minix_clear_zone(sbp, remove_zone, clr_start, clr_siz);

		++cur_rel_zone; /* 次のゾーンから削除を継続する */
		/*  削除位置を更新する  */
		cur_pos = roundup_align(cur_pos, MINIX_ZONE_SIZE(sbp));
	}

	/*
	 * 削除範囲中のゾーンを解放する
	 */
	for( ; end_rel_zone > cur_rel_zone; ++cur_rel_zone ) {
		
		/* デバイス先頭からのゾーン番号を算出
		 */
		rc = minix_read_mapped_block(sbp, dip, 
		    cur_rel_zone * MINIX_ZONE_SIZE(sbp), &remove_zone);

		/* 割当てられているゾーンを解放する
		 */
		if ( rc == 0 ) {

			rc = minix_write_mapped_block(sbp, dip, 
						      cur_rel_zone * MINIX_ZONE_SIZE(sbp), 
						      MINIX_NO_ZONE(sbp));
			if ( rc != 0 )
				continue;  /* 解放できなかったブロックをスキップする */
			minix_free_zone(sbp, remove_zone);
		}
		cur_pos += MINIX_ZONE_SIZE(sbp);  /*  削除位置を更新する  */
	}

	if ( ( off + len ) > cur_pos ) { /* 最終ゾーン内をクリアする */

		/* デバイス先頭からのゾーン番号を算出
		 */
		rc = minix_read_mapped_block(sbp, dip, 
		    end_rel_zone * MINIX_ZONE_SIZE(sbp), &remove_zone);
		if ( rc == 0 ) {

			/* ゾーン内の全データが削除対象となる場合は, 
			 * ゾーン内のブロックを解放する。
			 */
			if ( ( !addr_not_aligned(off, MINIX_ZONE_SIZE(sbp) ) )
			    && ( ( off + len ) == MINIX_D_INODE(sbp, dip, i_size) ) ) {

				/* 割当てられているデータゾーンを解放する
				 */
				/* ゾーンへの参照を解放する */
				rc = minix_write_mapped_block(sbp, dip, 
							      end_rel_zone *
							      MINIX_ZONE_SIZE(sbp),
							      MINIX_NO_ZONE(sbp));
				if ( rc == 0 ) /* ゾーンを解放する */
					minix_free_zone(sbp, remove_zone); 
			} else {

				/* 最終ゾーン内にデータが残っている場合は指定された
				 * 範囲をゼロクリアする
				 */
				minix_clear_zone(sbp, remove_zone, 
				    off % MINIX_ZONE_SIZE(sbp), 
				    (off + len) % MINIX_ZONE_SIZE(sbp) );
				cur_pos = off + len;
			}
		}
	}

	/* ファイル終端までクリアした場合 */
	if ( ( off + len ) == MINIX_D_INODE(sbp, dip, i_size) ) { 

		MINIX_D_INODE_SET(sbp, dip, i_size, off);  /* サイズを更新 */
	}

	/* I-node情報を更新 */
	rc = minix_rw_disk_inode(sbp, i_num, MINIX_RW_WRITING, dip);
	if ( rc != 0 ) 
		goto error_out;	

	return 0;

error_out:
	return rc;
}

/**
   MinixのI-nodeを元にデータブロックを伸長する
   @param[in]   sbp        Minixスーパブロック情報
   @param[in]   i_num      Minix I-node番号
   @param[in]   dip        MinixディスクI-node
   @param[in]   len        伸長長(単位: バイト)
   @retval      0          正常終了
   @retval     -EINVAL     offに負の値を指定した
   @retval     -EFBIG      ファイル長を超えている
   @note TODO: 更新時間更新
 */
int
minix_extend_zone(minix_super_block *sbp, minix_ino i_num, minix_inode *dip, ssize_t len){
	int                    rc;
	int                   res;
	size_t              pgsiz;
	off_t             cur_pos;
	size_t            old_siz;
	size_t            clr_siz;
	size_t          clr_start;
	minix_zone   cur_rel_zone;
	minix_zone first_rel_zone;
	minix_zone   end_rel_zone;
	minix_zone       new_zone;

	if ( len == 0 )
		return 0;  /* 伸長不要 */

	if ( 0 > len )
		return -EINVAL;

	rc = pagecache_pagesize(sbp->dev, &pgsiz);  /* ページサイズ取得 */
	kassert( rc == 0 ); /* マウントされているはずなのでデバイスが存在する */

	old_siz = MINIX_D_INODE(sbp, dip, i_size);  /* 伸長前のサイズ  */

	first_rel_zone = 
		truncate_align(old_siz, MINIX_ZONE_SIZE(sbp))
		/ MINIX_ZONE_SIZE(sbp);    /* 開始ゾーン     */

	end_rel_zone = roundup_align(old_siz + len, MINIX_ZONE_SIZE(sbp))
		/ MINIX_ZONE_SIZE(sbp); /* 終了ゾーン     */

	cur_pos = old_siz;              /* 伸長開始位置   */
	cur_rel_zone = first_rel_zone;  /* 伸長開始ゾーン */

	/* 開始点がゾーン境界と合っておらず, 後続のゾーンがある場合
	 */
	if ( addr_not_aligned(cur_pos, MINIX_ZONE_SIZE(sbp) ) 
	    && ( end_rel_zone > first_rel_zone ) ) {

		/* デバイス先頭からのゾーン番号を算出
		 */
		rc = minix_read_mapped_block(sbp, dip, 
		    first_rel_zone * MINIX_ZONE_SIZE(sbp), &new_zone);
		if ( rc == 0 ) {  /* 既にデータゾーンが割り当てられている場合 */

			/* クリア開始オフセットを算出 */
			clr_start = cur_pos % MINIX_ZONE_SIZE(sbp);

			/* クリアサイズを算出 */
			clr_siz = MINIX_ZONE_SIZE(sbp) - clr_start;

			/* 開始ゾーン内をクリアする */
			minix_clear_zone(sbp, new_zone, clr_start, clr_siz);

			/* 次のゾーンからゾーンを伸長する */
			++cur_rel_zone; 
			cur_pos = roundup_align(cur_pos, MINIX_ZONE_SIZE(sbp));
		}
	}

	/*
	 * 伸長範囲中のゾーンを割り当てる
	 */
	for( ; end_rel_zone > cur_rel_zone; ++cur_rel_zone ) {
		
		/* デバイス先頭からのゾーン番号を算出
		 */
		rc = minix_read_mapped_block(sbp, dip, 
		    cur_rel_zone * MINIX_ZONE_SIZE(sbp), &new_zone);

		/* ゾーンが割り当てられていない場合は, 
		 * 新たにゾーンを割り当てる 
		 * @note 読取りの場合, 上記で転送サイズをファイルサイズ内に
		 * 収めるように補正しているので, 読取りの場合で, かつ, 
		 * ゾーンが割り当てられていない場合でも, ゼロクリアした
		 * ゾーンを割り当てればよい(パンチホールアクセスケース)
		 */
		rc = minix_alloc_zone(sbp, &new_zone);  /* データゾーンを割当てる */
		if ( rc != 0 ) 
			goto unmap_zone_out;

		/* ゾーン内のブロックをクリアする
		 * 本ファイルシステムで使用されたデータブロックは
		 * データブロックの解放時にクリアされるためゼロクリアを
		 * 保証できるが, 新規に割り当てたゾーンのディスクブロックが
		 * 過去に他の用途で使われていた場合そのデータが読めてしまうため
		 * 念のためゾーンをクリアしてからマップする
		 */
		minix_clear_zone(sbp, new_zone, 0, MINIX_ZONE_SIZE(sbp));

		/* ゾーンをマップする */
		rc = minix_write_mapped_block(sbp, dip, cur_pos, new_zone);
		if ( rc != 0 ) {  /* 割り当てたゾーンをマップできなかった */

			/* 割当てたデータゾーンを解放する  */
			minix_free_zone(sbp, new_zone);
			goto unmap_zone_out;
		}

		cur_pos += MINIX_ZONE_SIZE(sbp);   /* ゾーン追加位置を更新 */
		MINIX_D_INODE_SET(sbp, dip, i_size, cur_pos);  /* サイズを更新 */
	}

	if ( MINIX_D_INODE(sbp, dip, i_size) > ( old_siz + len ) ) { 

		/*
		 * 終端がゾーン境界に沿っていない場合
		 */
		cur_pos = old_siz + len;  /* サイズを補正 */
		MINIX_D_INODE_SET(sbp, dip, i_size, cur_pos);  /* サイズを更新 */
	}

	/* I-node情報を更新 */
	rc = minix_rw_disk_inode(sbp, i_num, MINIX_RW_WRITING, dip);
	if ( rc != 0 ) {

		/* 最終ゾーンを削除するようにサイズを更新 */
		MINIX_D_INODE_SET(sbp, dip, i_size, 
				  roundup_align(cur_pos, MINIX_ZONE_SIZE(sbp))); 
		goto unmap_zone_out;
	}
	return 0;

unmap_zone_out:
	/* 伸長したゾーンをアンマップ */
	res = minix_unmap_zone(sbp, i_num, dip, old_siz,
			      MINIX_D_INODE(sbp, dip, i_size) - old_siz);
	kassert( res == 0 );

	/* ゾーン解放に伴って元のサイズに更新されていることを確認  */
	kassert( MINIX_D_INODE(sbp, dip, i_size) == old_siz );  

	return rc;
}

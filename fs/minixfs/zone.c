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
	 * ゾーン番号 = ビット番号 + sp->s_firstdatazone - 1
	 * となる. 
	 * 参考: Minixのalloc_zoneのコメント
	 * Note that the routine alloc_bit() returns 1 for the lowest possible
	 * zone, which corresponds to sp->s_firstdatazone.  To convert a value
	 * between the bit number, 'b', used by alloc_bit() and the zone number, 'z',
	 * stored in the inode, use the formula:
	 *     z = b + sp->s_firstdatazone - 1
	 * Alloc_bit() never returns 0, since this is used for NO_BIT (failure).
	 */

	*zp = MINIX_D_SUPER_BLOCK(sbp,s_firstdatazone) + (minix_zone)bitmap_index - 1;
	return 0;
}
/**
   ゾーンをクリアする
   @param[in] sbp    Minixスーパブロック情報
   @param[in] znum   ゾーン番号
   @retval    0      正常終了
   @retval   -EIO    ページキャッシュアクセス失敗
 */
int
minix_clear_zone(minix_super_block *sbp, minix_zone znum){
	int                      rc;
	page_cache              *pc; /* ページキャッシュ */
	size_t                pgsiz; /* ページキャッシュサイズ   */
	obj_cnt_type  blks_per_page; /* ページあたりのブロック数 */
	obj_cnt_type     first_page; /* 操作開始ページ   */
	obj_cnt_type       end_page; /* 操作終了ページ   */
	obj_cnt_type       cur_page; /* 操作対象ページ   */
	obj_cnt_type      first_blk; /* 操作開始ブロック */
	obj_cnt_type        end_blk; /* 操作終了ブロック */
	obj_cnt_type  __unused  cur_blk; /* 操作対象ブロック */

	rc = pagecache_pagesize(sbp->dev, &pgsiz);  /* ページサイズ取得 */
	kassert( rc == 0 ); /* マウントされているはずなのでデバイスが存在する */

	/* 開始ブロック */
	first_blk = znum << MINIX_D_SUPER_BLOCK(sbp, s_log_zone_size);
	/* 終了ブロック */
	end_blk = first_blk +  (1 << MINIX_D_SUPER_BLOCK(sbp, s_log_zone_size));

	/* ページ当たりのMinixブロック数を算出  */
	blks_per_page = pgsiz / MINIX_BLOCK_SIZE(sbp);
	/* クリア対象ゾーンの先頭ページ番号を算出  */
	first_page = truncate_align(first_blk, blks_per_page);
	/* クリア対象ゾーンの最終ページ番号を算出  */
	end_page = roundup_align(end_blk, blks_per_page);
	
	/*
	 * ゾーン内のブロックを順次クリアする
	 */
	for(cur_page = first_page, cur_blk = first_blk;
	    end_page > cur_page; ++cur_page) {

		/*  ページキャッシュを読み込み  */
		rc = pagecache_get(sbp->dev, cur_page * pgsiz, &pc);
		if ( rc != 0 ) {

			rc = -EIO;
			goto error_out;
		}

		/*  TODO: ブロックをクリア */
		pagecache_put(pc);  /* ページキャッシュを解放する  */
	}

	return 0;

error_out:
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
#if 0
/** Map new zone to a positon in a file
   @param[in]      dev Device id 
   @param[in, out] dip Disk inode
   @param[in]      position Byte offset in a file
   @param[in]      new_zone zone number to be allocated newly
   @note original function 
   int write_map, (struct inode *rip, off_t position, zone_t new_zone)
   rip Memory inode
   position Byte offset in a file
   new_zone zone number to be allocated newly
   @note new_zoneはビットマップ上で割当済みになっていることを前提とする
 */
int
minix_write_mapped_block(minix_super_block *sbp, minix_inode *dip, 
			off_t position, minix_zone new_zone){
	int                    rc;  /* Return code */
	minix_super_block      sb;  /* super block */
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

        /*  FIXME: This should be get_super instead */
	rc = minix_read_super(dev, &sb);
	if ( rc != 0 )
		return ENODEV;
	
	/*
	 *  ゾーン番号からアドレッシング種別を分類し, 直接参照ブロック, 
	 *  単間接ブロックのインデクス
	 *  1段目の2重間接ブロックのインデクス
	 *  2段目の2重間接ブロックのインデクス
	 *  を算出する
	 */
	rc = minix_calc_indexes(&sb, position, &index_type, 
				&zindex, &first_ind_index, &second_ind_index);
	if ( rc != 0 )
		goto out;  /*  E2BIGを返却する  */

	kassert( zindex < MINIX_NR_TZONES );

	if ( index_type == MINIX_ZONE_ADDR_DIRECT ) {

		/*
		 * 直接参照ブロック
		 */
		dip->i_zone[zindex] = new_zone;
		rc = 0;
		goto out;
	}

	/*
	 * 単間接参照ブロック, 2重間接参照ブロックの割り当て
	 */
	new_1st_indblk = MINIX_NO_ZONE;
	new_2nd_indblk = MINIX_NO_ZONE;
	cur_2nd_indblk = MINIX_NO_ZONE;

	if ( dip->i_zone[zindex] == MINIX_NO_ZONE ) {/* 単間接参照ブロック, 
						      * 2重間接ブロックの1段目のブロックが
						      * 未割当ての場合  
						      */

		if ( index_type == MINIX_ZONE_ADDR_SINGLE ) {

			/*
			 * 単間接参照ブロックの割当て
			 */
			rc = minix_alloc_indirect_block(&sb, dev, first_ind_index, 
			    new_zone, &new_1st_indblk);
		} else {
			
			/*
			 * 2重間接ブロックの2段目のブロックの割当て
			 */
			kassert( second_ind_index >= 0 );
			rc = minix_alloc_indirect_block(&sb, dev, second_ind_index, 
			    new_zone, &new_2nd_indblk);
			if ( rc != 0 )
				goto out; /*  割当てに失敗したら
					   * エラー復帰(ENOSPC, ENODEV)する 
					   */

			/*
			 * 2重間接ブロックの1段目のブロックの割当て
			 */
			kassert( new_2nd_indblk != MINIX_NO_ZONE);
			rc = minix_alloc_indirect_block(&sb, dev, first_ind_index, 
			    new_2nd_indblk, &new_1st_indblk);
		}
		if ( rc != 0 ) {
			
			/*
			 * 単間接参照ブロック, 2重間接ブロックの1段目のブロックの
			 * 割当てに失敗した
			 */
			if ( index_type == MINIX_ZONE_ADDR_SINGLE ) 
				goto out; /* エラー復帰(ENOSPC, ENODEV)する */
			else {

				/* 2重間接ブロックの場合は, 
				 * 2段目のブロックを開放してから
				 * エラー復帰(ENOSPC, ENODEV)する 
				 */
				kassert(new_2nd_indblk != MINIX_NO_ZONE);
				minix_free_zone(&sb, dev, new_2nd_indblk);
				goto out; 
			}
		}
		/* 1段目のブロックへの参照を記録 */
		kassert( new_1st_indblk != MINIX_NO_ZONE );
		dip->i_zone[zindex] = new_1st_indblk; 
	} else {  /* 単間接参照ブロック, 2重間接ブロックの1段目のブロックが割当済み  */

		/* 単間接参照ブロック中のデータブロック/2段目のブロックへの参照を取得 */
		minix_rd_indir(&sb, dev, dip->i_zone[zindex], 
		    first_ind_index, &cur_2nd_indblk);
		if ( index_type == MINIX_ZONE_ADDR_SINGLE ) {

			/*  データブロックへの参照を記録 */
			kassert( cur_2nd_indblk == MINIX_NO_ZONE );
			minix_wr_indir(&sb, dev, dip->i_zone[zindex], 
			    first_ind_index, new_zone);			
		} else {  /*  2重間接ブロックの場合  */
			
			if ( cur_2nd_indblk !=  MINIX_NO_ZONE ) {
			
				/*
				 *  2重間接ブロックの2段目のブロックが割り当て済みの場合
				 */

				/* 2段目のブロック中のデータブロックへの参照を取得 */
				minix_rd_indir(&sb, dev, cur_2nd_indblk, 
				    second_ind_index, &cur_data_blk);

				/*  データブロックへの参照を記録 */
				kassert( cur_data_blk == MINIX_NO_ZONE );
				minix_wr_indir(&sb, dev, cur_2nd_indblk, 
				    second_ind_index, new_zone);
			} else {

				/*  2重間接ブロックの2段目のブロックを割り当てて
				 *  データブロックへの参照を記録
				 */
				rc = minix_alloc_indirect_block(&sb, dev, second_ind_index, 
				    new_zone, &new_2nd_indblk);
				if ( rc != 0 ) {

					/* 割当てに失敗したら
					 * 1段目のブロック中の2段目のブロックへの
					 * 参照を消去
					 */
					minix_wr_indir(&sb, dev, dip->i_zone[zindex], 
					    first_ind_index, MINIX_NO_ZONE);
					goto out;
				}
			}
		}
	}
	rc = 0;
out:
	return rc;
}
#endif

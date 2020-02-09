/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Minix file system super block operations                          */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>

#include <kern/spinlock.h>
#include <kern/dev-pcache.h>

#include <fs/minixfs/minixfs.h>

/**
   MinixV3 スーパブロックのバイトオーダ反転
   @param[in]  in_sb 入力スーパブロック
   @param[out] out_sb 出力スーパブロック
 */
static void
swap_minixv3_super_block(minixv3_super_block *in_sb, minixv3_super_block *out_sb){

		out_sb->s_ninodes = __bswap32( in_sb->s_ninodes );
		out_sb->s_imap_blocks = __bswap16( in_sb->s_imap_blocks );
		out_sb->s_zmap_blocks = __bswap16( in_sb->s_zmap_blocks );
		out_sb->s_firstdatazone = __bswap16( in_sb->s_firstdatazone );
		out_sb->s_log_zone_size = __bswap16( in_sb->s_log_zone_size );
		out_sb->s_max_size = __bswap32( in_sb->s_max_size );
		out_sb->s_zones = __bswap32( in_sb->s_zones );
		out_sb->s_magic = __bswap16( in_sb->s_magic );
		out_sb->s_blocksize = __bswap16( in_sb->s_blocksize );
		out_sb->s_disk_version = in_sb->s_disk_version;
}

/**
   MinixV3 スーパブロックを読込む
   @param[in]  dsb ディスク上のスーパブロック
   @param[out] sbp メモリ上のスーパブロック返却域
   @retval     0      正常終了
   @retval    -ENODEV MinixV3スーパブロックではない
 */
static int
read_minixv3_super(void *dsb, minix_super_block *sbp){
	minixv3_super_block      *mv3;
	uint16_t        swapped_magic;

	mv3 = (minixv3_super_block *)dsb; /* MinixV3スーパブロックとしてアクセス */
	swapped_magic = __bswap16( MINIX_V3_SUPER_MAGIC );  /* バイトオーダを反転 */

	if ( ( mv3->s_magic != MINIX_V3_SUPER_MAGIC ) && ( mv3->s_magic != swapped_magic ) )
		goto error_out;
	
	sbp->s_magic = MINIX_V3_SUPER_MAGIC;  /* MinixV3ファイルシステム */
	if  ( mv3->s_magic == swapped_magic )
		sbp->swap_needed = true;  /* 要バイトスワップ */

	/*
	 * スーパブロックの内容をコピーする
	 */
	if ( sbp->swap_needed ) /* バイトオーダ変換してコピーする */
		swap_minixv3_super_block((minixv3_super_block *)&sbp->d_super, 
		    (minixv3_super_block *)dsb);
	else
		memmove((void *)&sbp->d_super, (void *)dsb, 
		    sizeof(minixv3_super_block));
	return 0;

error_out:
	return -ENODEV;
}

/**
   MinixV1/MinixV2 スーパブロックのバイトオーダ反転
   @param[in]  in_sb 入力スーパブロック
   @param[out] out_sb 出力スーパブロック
 */
static void
swap_minixv12_super_block(minixv12_super_block *in_sb, minixv12_super_block *out_sb){

		out_sb->s_ninodes = __bswap16( in_sb->s_ninodes );
		out_sb->s_nzones = __bswap16( in_sb->s_nzones );
		out_sb->s_imap_blocks = __bswap16( in_sb->s_imap_blocks );
		out_sb->s_zmap_blocks = __bswap16( in_sb->s_zmap_blocks );
		out_sb->s_firstdatazone = __bswap16( in_sb->s_firstdatazone );
		out_sb->s_log_zone_size = __bswap16( in_sb->s_log_zone_size );
		out_sb->s_max_size = __bswap32( in_sb->s_max_size );
		out_sb->s_magic = __bswap16( in_sb->s_magic );
		out_sb->s_state = __bswap16( in_sb->s_state );
		out_sb->s_zones = __bswap16( in_sb->s_zones );
}

/**
   MinixV2 スーパブロックを読込む
   @param[in]  dsb ディスク上のスーパブロック
   @param[out] sbp メモリ上のスーパブロック返却域
   @retval     0      正常終了
   @retval    -ENODEV MinixV2スーパブロックではない
 */
static int
read_minixv2_super(void *dsb, minix_super_block *sbp){
	minixv12_super_block      *mv2;
	uint16_t         swapped_magic;

	mv2 = (minixv12_super_block *)dsb; /* MinixV2スーパブロックとしてアクセス */

	swapped_magic = __bswap16( MINIX_V2_SUPER_MAGIC2 ); /* バイトオーダを反転 */
	if ( ( mv2->s_magic == MINIX_V2_SUPER_MAGIC2 ) ||
	    ( mv2->s_magic == swapped_magic ) ){
		
		/* MinixV2 30バイトファイル名
		 */
		sbp->s_magic = MINIX_V2_SUPER_MAGIC2;
		if  ( mv2->s_magic == swapped_magic )
			sbp->swap_needed = true;  /* 要バイトスワップ */
	} else {

		/* MinixV2 14バイトファイル名
		 */
		swapped_magic = __bswap16( MINIX_V2_SUPER_MAGIC ); /* バイトオーダを反転 */
		if ( ( mv2->s_magic == MINIX_V2_SUPER_MAGIC ) ||
		    ( mv2->s_magic == swapped_magic ) ) {

	
			sbp->s_magic = MINIX_V2_SUPER_MAGIC;
			if  ( mv2->s_magic == swapped_magic )
				sbp->swap_needed = true;  /* 要バイトスワップ */
		} else
			goto error_out;  /* MinixV2ファイルシステムではない */
	}

	/*
	 * スーパブロックの内容をコピーする
	 */
	if ( sbp->swap_needed ) /* バイトオーダ変換してコピーする */
		swap_minixv12_super_block((minixv12_super_block *)&sbp->d_super, 
		    (minixv12_super_block *)dsb);
	else
		memmove((void *)&sbp->d_super, (void *)dsb, 
		    sizeof(minixv12_super_block));

	return 0;

error_out:
	return -ENODEV;
}

/**
   MinixV1 スーパブロックを読込む
   @param[in]  dsb ディスク上のスーパブロック
   @param[out] sbp メモリ上のスーパブロック返却域
   @retval     0      正常終了
   @retval    -ENODEV MinixV1スーパブロックではない
 */
static int
read_minixv1_super(void *dsb, minix_super_block *sbp){
	minixv12_super_block      *mv1;
	uint16_t         swapped_magic;

	mv1 = (minixv12_super_block *)dsb; /* MinixV1スーパブロックとしてアクセス */

	swapped_magic = __bswap16( MINIX_V1_SUPER_MAGIC2 ); /* バイトオーダを反転 */
	if ( ( mv1->s_magic == MINIX_V1_SUPER_MAGIC2 ) ||
	    ( mv1->s_magic == swapped_magic ) ){
		
		/* MinixV1 30バイトファイル名
		 */
		sbp->s_magic = MINIX_V1_SUPER_MAGIC2;
		if  ( mv1->s_magic == swapped_magic )
			sbp->swap_needed = true;  /* 要バイトスワップ */
	} else {

		/* MinixV1 14バイトファイル名
		 */
		swapped_magic = __bswap16( MINIX_V1_SUPER_MAGIC ); /* バイトオーダを反転 */
		if ( ( mv1->s_magic == MINIX_V1_SUPER_MAGIC ) ||
		    ( mv1->s_magic == swapped_magic ) ) {

	
			sbp->s_magic = MINIX_V1_SUPER_MAGIC;
			if  ( mv1->s_magic == swapped_magic )
				sbp->swap_needed = true;  /* 要バイトスワップ */
		} else
			goto error_out;  /* MinixV1ファイルシステムではない */
	}

	/*
	 * スーパブロックの内容をコピーする
	 */
	if ( sbp->swap_needed ) /* バイトオーダ変換してコピーする */
		swap_minixv12_super_block((minixv12_super_block *)&sbp->d_super, 
		    (minixv12_super_block *)dsb);
	else
		memmove((void *)&sbp->d_super, (void *)dsb, 
		    sizeof(minixv12_super_block));

	return 0;

error_out:
	return -ENODEV;
}


/** 
    ビットマップ中の空きビットを割り当てる
    @param[in] sbp      スーパブロック情報
    @param[in] map_type 検索対象ビットマップ種別
      INODE_MAP ... I-nodeビットマップ
      ZONE_MAP  ... ゾーンビットマップ
    @param[out] idxp 割り当てたビットのインデクスを返却する領域
    @retval  0       正常終了
    @retval -ENOSPC  空きビットマップがない
 */
int
minix_bitmap_alloc(minix_super_block *sbp, int map_type, minix_bitmap_idx *idxp) {
	int                    rc;  /* Return code */
	size_t              pgsiz;  /* ページキャッシュのページ長 (単位: バイト) */
	size_t  __unused  nr_bits;  /* ビットマップ中のビット数 */
	obj_cnt_type     cur_page;  /* 検索対象ページ */
	obj_cnt_type     nr_pages;  /* 検索ページ数   */
	obj_cnt_type   first_page;  /* 検索開始ページ */
	obj_cnt_type     end_page;  /* 最終検索ページ */
	page_cache            *pc;  /* ページキャッシュ */

	rc = pagecache_pagesize(sbp->dev, &pgsiz);  /* ページサイズ取得 */
	kassert( rc == 0 ); /* マウントされているはずなのでデバイスが存在する */

	if (map_type == INODE_MAP) { /* I-nodeビットマップから空きビットを探す */

		first_page = (MINIX_IMAP_BLKNO * MINIX_BLOCK_SIZE(sbp)) 
			/ pgsiz; /* I-nodeビットマップのデバイス上のページ番号 */
		nr_bits = MINIX_D_SUPER_BLOCK(sbp,s_ninodes) + 1;
		nr_pages = 
			roundup_align(MINIX_D_SUPER_BLOCK(sbp, s_imap_blocks)
			    * MINIX_BLOCK_SIZE(sbp), pgsiz) / pgsiz;
	} else { /* ゾーンビットマップから空きビットを探す */

		/* ゾーンビットマップのデバイス上のページ番号 */
		first_page = ( MINIX_IMAP_BLKNO + MINIX_D_SUPER_BLOCK(sbp, s_imap_blocks) )
		    * MINIX_BLOCK_SIZE(sbp)  / pgsiz;  
		nr_bits = MINIX_SB_ZONES_NR(sbp) - 
			MINIX_D_SUPER_BLOCK(sbp,s_firstdatazone) + 1;
		nr_pages = 
			roundup_align(MINIX_D_SUPER_BLOCK(sbp, s_zmap_blocks)
			    * MINIX_BLOCK_SIZE(sbp), pgsiz) / pgsiz;
	}

	end_page = first_page + nr_pages;

	/*  
	 * Search bitmap blocks
	 */
	for(cur_page = first_page; end_page > cur_page; ++cur_page) {

		/*
		 * Load bitmap
		 */
		rc = pagecache_get(sbp->dev, cur_page * pgsiz, &pc);

#if 0
		bitmap = (minix_bitchunk_t *)&bp->data[0];

		/*
		 * Search a free bit and try to allocate it in the bitmap
		 */
		alloced_num = 0;
		rc = minix_bitmap_alloc_nolock(
			&bitmap[0], &bitmap[BITMAP_CHUNKS(dblk_siz)],
			d_blk, dblk_siz, nr_bits, &alloced_num);

		/* Ideally, we should obtain the inode/zone corresponding to
		 * this index before write this buffer back.
		 */
		if ( rc == 0 ) {

			/*
			 * Write bitmap
			 */
			buffer_cache_mark_dirty(bp);
			buffer_cache_blk_write(bp);  /* Write it immediately */
			buffer_cache_blk_release(bp);

			*idxp = alloced_num;

			return 0;
		}
#endif
		pagecache_put(pc);  /* ページキャッシュを解放する     */
	}
	
	return -ENOSPC;
}

/**
   Minixファイルシステムのスーパブロックを書き戻す
   @param[in] sbp スーパブロック情報
   @retval 0   正常終了
 */
int
minix_write_super(minix_super_block *sbp){
	int                        rc;
	page_cache                *pc;
	vm_vaddr                  off;
	vm_vaddr                sboff;
	void                     *dsb;

	/* デバイスの先頭からのオフセット位置を算出する */
	sboff = MINIX_SUPERBLOCK_BLKNO * MINIX_OLD_BLOCK_SIZE;	

	/* ページキャッシュを獲得する */
	rc = pagecache_get(sbp->dev, sboff, &pc);
	if ( rc != 0 )
		goto error_out;  /* ページキャシュの獲得に失敗した */

	off = sboff % pc->pgsiz;   /* ページ内オフセットを算出   */
	dsb = (pc->pc_data + off); /* ディスク上のスーパブロック */

	/*
	 * スーパブロックの内容をページキャッシュに書き込む
	 */
	if ( MINIX_SB_IS_V3(sbp) ){

		/*
		 * MinixV3ファイルシステムの場合
		 */
		if ( sbp->swap_needed ) /* バイトオーダ変換して書き込む */
			swap_minixv3_super_block((minixv3_super_block *)dsb, 
			    (minixv3_super_block *)&sbp->d_super);
		else
			memmove((void *)dsb, (void *)&sbp->d_super, 
			    sizeof(minixv3_super_block));
	} else if ( ( MINIX_SB_IS_V1(sbp) ) || ( MINIX_SB_IS_V2(sbp) ) ) {

		/*
		 * MinixV1, MinixV2ファイルシステムの場合
		 */
		if ( sbp->swap_needed ) /* バイトオーダ変換して書き込む */
			swap_minixv12_super_block((minixv12_super_block *)dsb, 
			    (minixv12_super_block *)&sbp->d_super);
		else
			memmove((void *)dsb, (void *)&sbp->d_super, 
			    sizeof(minixv12_super_block));
	} else {

		rc = -EINVAL;  /* 不正なスーパブロック */
		goto put_pcache_out;
	}
	
	pagecache_put(pc);  /* ページキャッシュを解放する     */

	return 0;

put_pcache_out:
	pagecache_put(pc);  /* ページキャッシュを解放する */

error_out:
	return rc;
}

/**
   Minixファイルシステムのスーパブロックを読み込む
   @param[in]  dev Minixファイルシステムが記録されたブロックデバイスのデバイスID
   @param[out] sbp 情報返却領域
   @retval 0   正常終了
 */
int
minix_read_super(dev_id dev, minix_super_block *sbp){
	int                        rc;
	page_cache                *pc;
	vm_vaddr                  off;
	vm_vaddr                sboff;
	void                     *dsb;

	kassert( sbp != NULL );

	/* デバイスの先頭からのオフセット位置を算出する */
	sboff = MINIX_SUPERBLOCK_BLKNO * MINIX_OLD_BLOCK_SIZE;	

	/* ページキャッシュを獲得する */
	rc = pagecache_get(dev, sboff, &pc);
	if ( rc != 0 )
		goto error_out;  /* スーパブロックが読み取れなかった */

	off = sboff % pc->pgsiz;  /* ページ内オフセットを算出 */
	dsb = (pc->pc_data + off); /* ディスク上のスーパブロック */

	sbp->s_magic = 0;          /* マジック番号をクリアする */
	sbp->swap_needed = false;  /* スワップ要否をクリアする */
	/*
	 * スーパブロックの検出
	 */
	rc = read_minixv3_super(dsb, sbp);
	if ( rc == 0 ) 
		goto found;  /* MinixV3スーパブロックを検出 */

	rc = read_minixv2_super(dsb, sbp);
	if ( rc == 0 ) 
		goto found;  /* MinixV2スーパブロックを検出 */

	rc = read_minixv1_super(dsb, sbp);
	if ( rc != 0 ) 
		goto put_pcache_out;  /* Minix スーパブロックが見つからなかった */
	
found:
	pagecache_put(pc);  /* ページキャッシュを解放する     */
	sbp->dev = dev;     /* ブロックデバイス番号を記録する */

	return 0;

put_pcache_out:
	pagecache_put(pc);  /* ページキャッシュを解放する */
error_out:
	return rc;
}

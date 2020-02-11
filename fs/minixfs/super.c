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
   MinixV3 スーパブロックのバイトオーダ反転(内部関数)
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
   MinixV3 スーパブロックを読込む(内部関数)
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
   MinixV1/MinixV2 スーパブロックのバイトオーダ反転(内部関数)
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
   MinixV2 スーパブロックを読込む(内部関数)
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
   MinixV1 スーパブロックを読込む(内部関数)
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
   ページキャッシュ中のビットマップからビットを割当てる(内部関数)
   @param[in] sbp      スーパブロック情報
   @param[in] cur_page 検索対象ページ番号 (単位: デバイス先頭からのページ数)
   @param[in] bit_off  検索開始ビット位置 (単位: ビット)
   @param[in] nt_bits  ビットマップ中の総ビット数 (単位: ビット)
   @param[in] map_type 検索対象ビットマップ種別
   INODE_MAP ... I-nodeビットマップ
   ZONE_MAP  ... ゾーンビットマップ
   @param[out] idxp 割り当てたビットのインデクスを返却する領域
*/
static int
minix_bitmap_alloc_nolock(minix_super_block *sbp, obj_cnt_type cur_page, int map_type,
			  minix_bitmap_idx bit_off, minix_bitmap_idx nr_bits, 
			  minix_bitmap_idx *idxp) {
	int                        rc;
	int                         i;
	page_cache                *pc;  /* ページキャッシュ */
	size_t                  pgsiz;  /* ページキャッシュのページ長 (単位: バイト) */
	minixv12_bitchunk     *v12ptr;  /* MinixV1, MinixV2ビットマップチャンクポインタ */
	minixv12_bitchunk      v12val;  /* MinixV1, MinixV2ビットマップチャンク */
	minixv3_bitchunk       *v3ptr;  /* MinixV3ビットマップチャンクポインタ */
	minixv3_bitchunk        v3val;  /* MinixV3ビットマップチャンク */
	void                 *end_ptr;  /* 末尾アドレス */
	minix_bitmap_idx    found_num;  /* 見つかったビット */

	rc = pagecache_pagesize(sbp->dev, &pgsiz);  /* ページサイズ取得 */
	kassert( rc == 0 ); /* マウントされているはずなのでデバイスが存在する */

	/*
	 * Load bitmap
	 */
	rc = pagecache_get(sbp->dev, cur_page * pgsiz, &pc);
	if ( rc != 0 )
		return rc;

	end_ptr = (void *)pc->pc_data + pgsiz;

	if ( MINIX_SB_IS_V3(sbp) ) {

		for( v3ptr = (minixv3_bitchunk *)pc->pc_data; end_ptr > (void *)v3ptr; ++v3ptr) {

			if ( *v3ptr == ~((minixv3_bitchunk)0) )
				continue;  /*  空きビットがない  */

			if ( sbp->swap_needed )
				v3val = __bswap32(*v3ptr);  /* バイトスワップ */
			else
				v3val = *v3ptr;
			
			/*
			 * ビットマップチャンク中のビットを探査する
			 * @note 上記で空きビットの存在を確認しているので必ず見つかる
			 */
			for (i = 0; (v3val & (1 << i)) != 0; ++i); 

			/* found_numはビットマップ領域内でのビット位置を表す
			 */
			found_num = bit_off + 
				( ( (uintptr_t)v3ptr - (uintptr_t)pc->pc_data )
				  / sizeof(minixv3_bitchunk) ) * 
				( sizeof(minixv3_bitchunk) * BITS_PER_BYTE ) + i;

			/* ビットマップの範囲を越えていないことを確認
			*/
			if ( found_num >= nr_bits  )
				goto put_pcache_out;

			v3val |= 1 << i;  /* ビットを使用中に設定 */
			if ( sbp->swap_needed )
				*v3ptr = __bswap32(v3val);  /* バイトスワップ */
			else
				*v3ptr = v3val;
			break;
		} 
	} else { /* MinixV1, MinixV2ファイルシステム */

			kassert( MINIX_SB_IS_V2(sbp) || MINIX_SB_IS_V1(sbp) );

			for( v12ptr = (minixv12_bitchunk *)pc->pc_data; end_ptr > (void *)v12ptr;
			     ++v12ptr) {

				if ( *v12ptr == ~((minixv12_bitchunk)0) )
					continue;  /*  空きビットがない  */
				
				if ( sbp->swap_needed )
					v12val = __bswap16(*v12ptr);  /* バイトスワップ */
				else
					v12val = *v12ptr;
				
				/*
				 * ビットマップチャンク中のビットを探査する
				 * @note 上記で空きビットの存在を確認しているので必ず見つかる
				 */
				for (i = 0; (v12val & (1 << i)) != 0; ++i); 
			
				/* found_numはビットマップ領域内でのビット位置を表す
				 */
				found_num = bit_off + 
					( ( (uintptr_t)v12ptr - (uintptr_t)pc->pc_data )
					  / sizeof(minixv12_bitchunk) ) * 
					( sizeof(minixv12_bitchunk) * BITS_PER_BYTE ) + i;

				/* ビットマップの範囲を越えていないことを確認
				 */
				if ( found_num >= nr_bits  )
					goto put_pcache_out;

				v12val |= 1 << i;  /* ビットを使用中に設定 */
				if ( sbp->swap_needed )
					*v12ptr = __bswap16(v12val);  /* バイトスワップ */
				else
					*v12ptr = v12val;
				break;
			}
	}

	*idxp = found_num;  /* 見つかったビットを返却 */
	goto success;
	

put_pcache_out:
	pagecache_put(pc);  /* ページキャッシュを解放する     */	
	return -ESRCH;

success:
	pagecache_put(pc);  /* ページキャッシュを解放する     */	

	return 0;
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
	int                    rc;  /* 返り値 */
	size_t              pgsiz;  /* ページキャッシュのページ長 (単位: バイト) */
	size_t            nr_bits;  /* ビットマップ中のビット数 */
	obj_cnt_type     cur_page;  /* 検索対象ページ   */
	obj_cnt_type     nr_pages;  /* 検索ページ数     */
	obj_cnt_type   first_page;  /* 検索開始ページ   */
	obj_cnt_type     end_page;  /* 最終検索ページ   */
	minix_bitmap_idx      idx;  /* 割り当てたビット */

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

		rc = minix_bitmap_alloc_nolock(sbp, cur_page, map_type,
		    ( cur_page - first_page ) * BITS_PER_BYTE * pgsiz, 
					       nr_bits, &idx);
		if ( rc != 0 )
			goto error_out;

		*idxp = idx;
		break;
	}

	return 0;

error_out:
	return -ENOSPC;
}

/**
    ビットマップ中の空きビットを割り当てる
    @param[in] sbp      スーパブロック情報
    @param[in] map_type 検索対象ビットマップ種別
      INODE_MAP ... I-nodeビットマップ
      ZONE_MAP  ... ゾーンビットマップ
    @param[in] fbit   解放するビット
    @retval  0       正常終了
    @retval -EINVAL  ビットマップ範囲内にないビットを指定した
    @retval -ENODEV  ページキャッシュを読み込めなかった
 */
int
minix_bitmap_free(minix_super_block *sbp, int map_type, minix_bitmap_idx fbit) {
	int                    rc;  /* 返り値 */
	page_cache            *pc;  /* ページキャッシュ */
	size_t              pgsiz;  /* ページキャッシュのページ長 (単位: バイト) */
	size_t            nr_bits;  /* ビットマップ中のビット数 */
	obj_cnt_type     cur_page;  /* 検索対象ページ   */
	obj_cnt_type     nr_pages;  /* 検索ページ数     */
	obj_cnt_type   first_page;  /* 検索開始ページ   */
	obj_cnt_type     end_page;  /* 最終検索ページ   */
	minix_bitmap_idx  bit_off;  /* ビットマップチャンク中のオフセット */
	minix_bitmap_idx  bit_idx;  /* ビットマップチャンク配列のインデクス */
	minix_bitmap_idx     mask;  /* ビットをセットしたチャンク */
	minixv3_bitchunk   *v3ptr;  /* MinixV3ビットマップチャンクポインタ */
	minixv3_bitchunk    v3val;  /* MinixV3ビットマップチャンク */
	minixv12_bitchunk *v12ptr;  /* MinixV1, MinixV2ビットマップチャンクポインタ */
	minixv12_bitchunk  v12val;  /* MinixV1, MinixV2ビットマップチャンク */

	rc = pagecache_pagesize(sbp->dev, &pgsiz);  /* ページサイズ取得 */
	kassert( rc == 0 ); /* マウントされているはずなのでデバイスが存在する */

	if ( map_type == INODE_MAP ) { /* I-nodeビットマップから空きビットを探す */

		first_page = (MINIX_IMAP_BLKNO * MINIX_BLOCK_SIZE(sbp)) 
			/ pgsiz; /* I-nodeビットマップのデバイス上のページ番号 */
		nr_bits = MINIX_D_SUPER_BLOCK(sbp,s_ninodes) + 1;
		nr_pages = 
			roundup_align(MINIX_D_SUPER_BLOCK(sbp, s_imap_blocks)
			    * MINIX_BLOCK_SIZE(sbp), pgsiz) / pgsiz;
	} else { /* ゾーンビットマップから空きビットを探す */

		/* ゾーンビットマップのデバイス上のページ番号 */
		first_page = ( MINIX_IMAP_BLKNO + MINIX_D_SUPER_BLOCK(sbp, s_imap_blocks) )
		    * MINIX_BLOCK_SIZE(sbp) / pgsiz;  
		nr_bits = MINIX_SB_ZONES_NR(sbp) - 
			MINIX_D_SUPER_BLOCK(sbp,s_firstdatazone) + 1;
		nr_pages = 
			roundup_align(MINIX_D_SUPER_BLOCK(sbp, s_zmap_blocks)
			    * MINIX_BLOCK_SIZE(sbp), pgsiz) / pgsiz;
	}

	cur_page = ( fbit / ( BITS_PER_BYTE * pgsiz ) ) + first_page;
	end_page = first_page + nr_pages;

	if ( ( fbit >= nr_bits ) || ( cur_page >= end_page ) )
		return -EINVAL;  /* 範囲外のビット */

	bit_idx = fbit / ( BITS_PER_BYTE * MINIX_BMAPCHUNK_SIZE(sbp) );
	bit_idx = bit_idx % ( pgsiz / MINIX_BMAPCHUNK_SIZE(sbp) );
	bit_off = fbit % ( BITS_PER_BYTE * MINIX_BMAPCHUNK_SIZE(sbp) );
	
	mask = 1 << bit_off;

	rc = pagecache_get(sbp->dev, cur_page * pgsiz, &pc);
	if ( rc != 0 )
		return -ENODEV;

	if ( MINIX_SB_IS_V3(sbp) ) {

		/*
		 * MinixV3ファイルシステム
		 */
		v3ptr = (minixv3_bitchunk *)pc->pc_data;
		if ( sbp->swap_needed )
			v3val = __bswap32(v3ptr[bit_idx]);
		else
			v3val = v3ptr[bit_idx];

		kassert( v3val & mask );

		v3val &= ~mask;

		if ( sbp->swap_needed )
			v3ptr[bit_idx] = __bswap32(v3val);
		else
			v3ptr[bit_idx] = v3val;
	} else {

		/*
		 * MinixV1, MinixV2ファイルシステム
		 */
		v12ptr = (minixv12_bitchunk *)pc->pc_data;
		if ( sbp->swap_needed )
			v12val = __bswap16(v12ptr[bit_idx]);
		else
			v12val = v12ptr[bit_idx];

		kassert( v12val & mask );

		v12val &= (minixv12_bitchunk)(~mask & (~((minixv12_bitchunk)0)));

		if ( sbp->swap_needed )
			v12ptr[bit_idx] = __bswap16(v12val);
		else
			v12ptr[bit_idx] = v12val;
	}
	
	pagecache_put(pc);  /* ページキャッシュを解放する     */

	return 0;
}

/**
   Minixファイルシステムのスーパブロックを書き戻す
   @param[in] sbp スーパブロック情報
   @retval 0   正常終了
 */
int
minix_write_super(minix_super_block *sbp){
	int          rc;
	page_cache  *pc;
	off_t       off;
	off_t     sboff;
	void       *dsb;

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
	int          rc;
	page_cache  *pc;
	off_t       off;
	off_t     sboff;
	void       *dsb;

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

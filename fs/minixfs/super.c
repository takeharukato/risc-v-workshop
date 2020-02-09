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

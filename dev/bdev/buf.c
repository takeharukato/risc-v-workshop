/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Block buffer Routines                                             */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>

#include <kern/page-if.h>

#include <kern/vfs-if.h>
#include <kern/dev-if.h>

static kmem_cache blkbuf_cache;   /**< ブロックバッファのSLABキャッシュ */

/**
   ブロックバッファを初期化する (内部関数)
   @param[in] buf  ブロックバッファ
 */
static void
init_blkbuf(block_buffer *buf){

	list_init(&buf->b_ent); /* リストエントリの初期化 */
	buf->b_offset = 0;  /* バッファオフセットを初期化する */
	buf->b_len = 0;     /* バッファ長を初期化する */
	buf->b_page = NULL; /* ページキャッシュポインタを初期化する */
}

/**
   ブロックバッファを割り当てる (内部関数)
   @param[out] bufp  ブロックバッファを指し示すポインタのアドレス
   @retval  0       正常終了
   @retval -ENOMEM  メモリ不足
 */
static int
alloc_blkbuf(block_buffer **bufp){
	int            rc;
	block_buffer *buf;

	kassert( bufp != NULL );

	/* ブロックバッファを確保する */
	rc = slab_kmem_cache_alloc(&blkbuf_cache, KMALLOC_NORMAL, (void **)&buf);
	if ( rc != 0 )
		goto error_out;

	init_blkbuf(buf);  /* ブロックバッファを初期化 */

	*bufp = buf;  /* 確保したバッファを返却する */

	return 0;

error_out:
	return rc;
}

/**
   ブロックバッファを解放する (内部関数)
   @param[in] buf   ブロックバッファ
 */
static void
free_blkbuf(block_buffer *buf){

	slab_kmem_cache_free((void *)buf);  /* ブロックバッファを解放する */
	return ;
}

/*
 * IF関数
 */

/**
   ページキャッシュにブロックバッファを割り当てる
   @param[in]  devid    ブロックデバイスのデバイスID
   @param[in]  pc       ページキャッシュ
   @retval  0      正常終了
   @retval -EINVAL デバイスIDが不正
   @retval -ENODEV 指定されたデバイスが見つからなかった
   @retval -ENOENT ページキャッシュが解放中だった
   @retval -ENOMEM メモリ不足
 */
int
block_buffer_map_to_page_cache(dev_id devid, vfs_page_cache *pc){
	int                   rc;
	bdev_entry         *bdev;
	block_buffer        *buf;
	size_t             pgsiz;
	off_t            blk_off;
	obj_cnt_type           i;
	obj_cnt_type     nr_bufs;

	/* ブロックデバイスのページキャッシュであることを確認 */
	kassert( VFS_PCACHE_IS_DEVICE_PAGE(pc) );
	kassert( VFS_PCACHE_IS_BUSY(pc) );  /* ページキャッシュの参照と使用権を獲得済み */

	if ( devid == VFS_VSTAT_INVALID_DEVID )
		return -EINVAL;

	rc = bdev_bdev_entry_get(devid, &bdev);   /* ブロックデバイスエントリへの参照を獲得 */
	if ( rc != 0 )
		goto error_out;

	rc = vfs_page_cache_pagesize_get(pc, &pgsiz);
	if ( rc != 0 )
		goto put_bdev_ent_out;

	kassert( pgsiz >= bdev->bdent_blksiz );
	kassert( !addr_not_aligned(pgsiz, bdev->bdent_blksiz) );

	/* ページ内に割り当てるブロック数を算出する */
	nr_bufs = pgsiz / bdev->bdent_blksiz;

	/* ページキャッシュにブロックを割り当てる
	 */
	for( i = 0, blk_off = 0; nr_bufs > i; ++i, blk_off += bdev->bdent_blksiz ) {

		rc = alloc_blkbuf(&buf);
		if ( rc != 0 )
			goto free_blocks_out;

		buf->b_offset = blk_off;  /* ページ内オフセットを設定 */
		buf->b_len = bdev->bdent_blksiz; /* ブロックサイズを設定 */
		/* ブロックバッファを登録する */
		rc = vfs_page_cache_enqueue_block_buffer(pc, buf);
		kassert( rc == 0 );
	}

	bdev_bdev_entry_put(bdev); /* ブロックデバイスエントリへの参照を解放する */

	return 0;

free_blocks_out:
	block_buffer_unmap_from_page_cache(pc);  /* ブロックバッファを解放する */

put_bdev_ent_out:
	bdev_bdev_entry_put(bdev); /* ブロックデバイスエントリへの参照を解放する */

error_out:
	return rc;
}

/**
   ページキャッシュに割り当てられたブロックバッファを解放する
   @param[in]  pc  ページキャッシュ
 */
void
block_buffer_unmap_from_page_cache(vfs_page_cache *pc){
	int                rc;
	block_buffer *cur_buf;

	/* ブロックデバイスのページキャッシュであることを確認 */
	kassert( VFS_PCACHE_IS_DEVICE_PAGE(pc) );
	kassert( VFS_PCACHE_IS_BUSY(pc) );  /* ページキャッシュの参照と使用権を獲得済み */

	/* ブロックバッファを解放する
	 */
	rc = vfs_page_cache_dequeue_block_buffer(pc, &cur_buf);	/* 先頭のバッファを取り出す */
	while( rc == 0 ) {

		free_blkbuf(cur_buf);  /* バッファを解放する */
		/* 先頭のバッファを取り出す */
		rc = vfs_page_cache_dequeue_block_buffer(pc, &cur_buf);
		if ( rc == -EAGAIN )
			break;  /* キューにバッファがない */
		kassert( rc == 0 );
	}

	return ;
}

/**
   ブロックバッファを獲得し, バッファを含むページキャッシュの使用権を得る
   @param[in]  devid デバイスID
   @param[in]  blkno ブロック番号
   @param[out] bufp  ブロックバッファを指し示すポインタのアドレス
   @retval 0 正常終了
   @retval -EINVAL デバイスIDが不正
   @retval -ENODEV 指定されたデバイスが見つからなかった
   @retval -ENOENT ページキャッシュが解放中だった
 */
int
block_buffer_get(dev_id devid, dev_blkno blkno, block_buffer **bufp){
	int                  rc;
	bdev_entry        *bdev;
	off_t            offset;
	off_t       page_offset;
	size_t           blksiz;
	size_t            pgsiz;
	vfs_page_cache      *pc;
	block_buffer       *buf;

	rc = bdev_bdev_entry_get(devid, &bdev);   /* ブロックデバイスエントリへの参照を獲得 */
	if ( rc != 0 )
		goto error_out;

	rc = bdev_blocksize_get(devid, &blksiz); /* ブロックサイズを取得する */
	if ( rc != 0 )
		goto put_bdev_out;

	offset = blkno * blksiz;  /* ブロックデバイス内でのオフセットアドレスを算出 */

	rc = bdev_page_cache_get(devid, offset, &pc); /* ページキャッシュを獲得する */
	if ( rc != 0 )
		goto put_bdev_out;

	rc = vfs_page_cache_pagesize_get(pc, &pgsiz); /* ページサイズを獲得する */
	if ( rc != 0 )
		goto put_page_cache_out;

	kassert( VFS_PCACHE_BUFSIZE_VALID(blksiz ,pgsiz) ); /* ブロックサイズの正当性確認 */

	page_offset = offset % pgsiz;  /* ページ内のオフセットアドレスを得る */

	/* バッファキャッシュを得る */
	rc = vfs_page_cache_block_buffer_find(pc, page_offset, &buf);
	if ( rc != 0 )
		goto put_page_cache_out;

	if ( bufp != NULL )
		*bufp = buf;  /* ページキャッシュの使用権とバッファを返却 */
	 else
		vfs_page_cache_put(pc);  /* ページキャッシュの使用権を解放する */

	bdev_bdev_entry_put(bdev);  /* ブロックデバイスエントリへの参照を解放 */

	return 0;

put_page_cache_out:
	vfs_page_cache_put(pc);  /* ページキャッシュの使用権を解放する */

put_bdev_out:
	bdev_bdev_entry_put(bdev);  /* ブロックデバイスエントリへの参照を解放 */

error_out:
	return rc;
}

/**
   ブロックバッファを含むページキャッシュの使用権を解放する
   @param[in] buf  ブロックバッファ
 */
void
block_buffer_put(block_buffer *buf){
	int rc;

	rc = vfs_page_cache_put(buf->b_page);  /* ページキャッシュの使用権を解放する */
	kassert( rc == 0 );
}

/**
   二次記憶の内容をブロックバッファに読み込む
   @param[in] buf  ブロックバッファ
   @retval  0      正常終了
   @retval -ENOENT 解放中のページキャッシュだった
 */
int
block_buffer_read(block_buffer *buf){

	return bio_page_read(buf->b_page); /* バッファを含むページキャッシュに読み込む */
}

/**
   ブロックバッファを二次記憶に書き込む
   @param[in] buf  ブロックバッファ
   @retval  0      正常終了
   @retval -ENOENT 解放中のページキャッシュだった
 */
int
block_buffer_write(block_buffer *buf){

	return bio_page_write(buf->b_page); /* バッファを含むページキャッシュを書き込む */
}

/**
   ブロックバッファのデータ領域を参照する
   @param[in]  buf   ブロックバッファ
   @param[out] datap データ領域を指し示すポインタのアドレス
   @retval  0      正常終了
   @retval -ENOENT 解放中のページキャッシュだった
 */
int
block_buffer_refer_data(block_buffer *buf, void **datap){
	int                  rc;
	bool                res;
	void          *buf_data;

	res = vfs_page_cache_ref_inc(buf->b_page);  /* ページキャッシュへの参照を獲得する */
	if ( !res ) {

		rc = -ENOENT;  /* 開放中のページキャッシュ */
		goto error_out;
	}

	/* ブロックデバイスのページキャッシュであることを確認 */
	kassert( VFS_PCACHE_IS_DEVICE_PAGE(buf->b_page) );
	kassert( VFS_PCACHE_IS_BUSY(buf->b_page) );  /* バッファ使用権を獲得済み */

	buf_data = buf->b_page->pc_data + buf->b_offset; /* バッファデータの先頭を参照 */

	if ( datap != NULL )
		*datap = buf_data; /* データ領域を返却する */

	vfs_page_cache_ref_dec(buf->b_page);  /* ページキャッシュへの参照を解放する */

	return 0;

error_out:
	return rc;
}

/**
   ブロックバッファを書き込み済みに設定する
   @param[in] buf   ブロックバッファ
   @retval  0      正常終了
   @retval -ENOENT 解放中のページキャッシュだった
 */
int
block_buffer_mark_dirty(block_buffer *buf){
	int                  rc;
	bool                res;

	res = vfs_page_cache_ref_inc(buf->b_page);  /* ページキャッシュへの参照を獲得する */
	if ( !res ) {

		rc = -ENOENT;  /* 開放中のページキャッシュ */
		goto error_out;
	}

	/* バッファを含むページキャッシュを書き込み済みに設定 */
	vfs_page_cache_mark_dirty(buf->b_page);

	vfs_page_cache_ref_dec(buf->b_page);  /* ページキャッシュへの参照を解放する */

	return 0;

error_out:
	return rc;
}

/**
   ブロックバッファ機構の初期化
 */
void
block_buffer_init(void){
	int rc;

	/* ブロックバッファのキャッシュを初期化する */
	rc = slab_kmem_cache_create(&blkbuf_cache, "block buffer cache",
	    sizeof(block_buffer), SLAB_ALIGN_NONE,  0, KMALLOC_NORMAL, NULL, NULL);
	kassert( rc == 0 );
}

/**
   ブロックバッファ機構の終了
 */
void
block_buffer_finalize(void){

	/* ブロックバッファキャッシュを解放 */
	slab_kmem_cache_destroy(&blkbuf_cache);
}

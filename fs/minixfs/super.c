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
   Minixファイルシステムのスーパブロックを読み込む
   @param[in]  dev Minixファイルシステムが記録されたブロックデバイスのデバイスID
   @param[out] sbp 情報返却領域
   @retval 0   正常終了
 */
int
minix_read_super(dev_id dev, minix_super_block *sbp){
	int         rc;
	page_cache *pc;
	vm_vaddr   off;
	vm_vaddr sboff;

	kassert( sbp != NULL );
	
	/* デバイスの先頭からのオフセット位置を算出する */
	sboff = MINIX_SUPERBLOCK_BLKNO * MINIX_OLD_BLOCK_SIZE;	

	/* ページキャッシュを獲得する */
	rc = pagecache_get(dev, sboff, &pc);
	if ( rc != 0 )
		goto error_out;  /* スーパブロックが読み取れなかった */

	off = sboff % pc->pgsiz;  /* ページ内オフセットを算出 */

	/* スーパブロックの内容をコピーする */
	memmove(&sbp->d_super.v3, (minix_super_block *)(pc->pc_data + off), sizeof(minix_super_block));

	pagecache_put(pc);  /* ページキャッシュを解放する */

	return 0;

error_out:
	return rc;
}

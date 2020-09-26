/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Simple file system data block operations                          */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>

#include <kern/mutex.h>
#include <kern/dev-pcache.h>
#include <kern/vfs-if.h>

#include <fs/simplefs/simplefs.h>

/**
   ファイル中の指定されたオフセット位置に対応したゾーン番号を取得する
   @param[in]  fs_super 単純なファイルシステムのスーパブロック情報
   @param[in]  fs_inode 単純なファイルシステムのI-node情報
   @param[in]  position ファイル内でのオフセットアドレス(単位:バイト)
   @param[out] blkp     ブロック番号返却域
   @retval     0        正常終了
   @retval    -ENOENT   ブロックが割り当てられていない
   @retval    -E2BIG    ファイルサイズの上限を超えている
 */
int
simplefs_read_mapped_block(simplefs_super_block *fs_super, simplefs_inode *fs_inode,
    off_t position, simplefs_blkno *blkp){
	simplefs_blkno blk;
	
	if ( position > fs_inode->i_size )
		return -E2BIG;  /*  ファイルサイズの上限を超えている  */

	/* ブロック番号を算出する */
	blk = truncate_align(position, fs_super->s_blksiz) / fs_super->s_blksiz;
	kassert( SIMPLEFS_IDATA_NR > blk );  /* ブロック番号の範囲を超えないこと */
	
	*blkp = blk;  /* 物理ブロック番号を返却する */

	return 0;
}

/**
    ファイル中の指定されたオフセット位置にブロックを割り当てる
    @param[in] fs_super 単純なファイルシステムのスーパブロック情報
    @param[in] fs_inode 単純なファイルシステムのI-node情報
    @param[in] position ファイル内でのオフセットアドレス(単位:バイト)
    @param[in] new_blk  割り当てる物理ブロック番号
    @retval    0       正常終了
    @retval   -E2BIG   ファイルサイズの上限を超えている
 */
int
simplefs_write_mapped_block(simplefs_super_block *fs_super, simplefs_inode *fs_inode,
    off_t position, simplefs_blkno new_blk){
	simplefs_blkno blk;
	
	if ( position > fs_inode->i_size )
		return -E2BIG;  /*  ファイルサイズの上限を超えている  */
	
	if ( new_blk != SIMPLEFS_SUPER_INVALID_BLOCK ) {

		/*  ブロック番号が一致することを確認する */
		blk = truncate_align(position, fs_super->s_blksiz) / fs_super->s_blksiz;
		kassert( blk == new_blk );
	}
	
	return 0;
}

/**
   単純なファイルシステムのI-nodeを元にデータブロックを解放する
   @param[in]   fs_super   単純なファイルシステムのスーパブロック情報
   @param[in]   fs_vnid    単純なファイルシステムのI-node番号
   @param[in]   fs_inode   単純なファイルシステムのI-node
   @param[in]   off        解放開始位置のオフセット(単位: バイト)
   @param[in]   len        解放長(単位: バイト)
   @retval      0          正常終了
   @retval     -EINVAL     offまたlenに負の値を指定した
   @retval     -EFBIG      ファイル長よりoffの方が大きい
 */
int
simplefs_unmap_block(simplefs_super_block *fs_super, vfs_vnode_id fs_vnid,
    simplefs_inode *fs_inode, off_t off, ssize_t len) {

	if ( ( 0 > off ) || ( 0 > len ) )
		return -EINVAL;  /*  offまたlenに負の値を指定した  */

	if  ( ( off > fs_inode->i_size ) || ( off > ( off + len ) ) )
		return -EFBIG;  /*  ファイル長よりoffの方が大きい  */

	if ( ( off + len ) >= fs_inode->i_size ) {

		
		fs_inode->i_size = off;  /* ファイルサイズを更新 */
	}
	return 0;
}

/**
   単純なファイルシステムのブロックを割り当てる
   @param[in]  fs_super   単純なファイルシステムのスーパブロック情報
   @param[out] block_nump ブロック番号返却域
   @retval     0      正常終了
   @retval    -ENOSPC 空きゾーンがない
 */
int 
simplefs_alloc_block(simplefs_super_block *fs_super, simplefs_blkno *block_nump){

	return 0;
}

/**
   単純なファイルシステムのブロックを解放する
   @param[in]  fs_super  単純なファイルシステムのスーパブロック情報
   @param[in]  fs_inode  単純なファイルシステムのI-node情報
   @param[in]  block_num ブロック番号
 */
void
simplefs_free_block(simplefs_super_block *fs_super, simplefs_inode *fs_inode,
    simplefs_blkno block_num){
	
	simplefs_clear_block(fs_super, fs_inode, block_num,
	    0, fs_super->s_blksiz);  /* ブロック内をクリアする */

	/* メモリファイルシステムの場合は, ブロックを二次記憶に書き戻す
	 * 処理は不要
	 */

	return ;
}


/**
   単純なファイルシステムのブロックをクリアする
   @param[in] fs_super  単純なファイルシステムのスーパブロック情報
   @param[in] fs_inode  単純なファイルシステムのI-node情報
   @param[in] block_num ブロック番号
   @param[in] offset    ブロック内のクリア開始オフセット (単位: バイト)
   @param[in] size      ブロック内のクリアサイズ (単位: バイト)
   @retval    0      正常終了
   @retval   -EINVAL offset/sizeがブロック長を超えている
   @retval   -E2BIG  ファイルサイズの上限を超えている
 */
int
simplefs_clear_block(simplefs_super_block *fs_super, simplefs_inode *fs_inode,
    simplefs_blkno block_num, off_t offset, off_t size){

	if ( offset > fs_super->s_blksiz )
		return -EINVAL;  /* offsetがブロック長を超えている  */

	if ( size > fs_super->s_blksiz )
		return -EINVAL;  /* sizeがブロック長を超えている  */

	if ( ( ( offset + size ) > fs_super->s_blksiz ) ||
	    ( offset >= ( offset + size ) ) )
		return -EINVAL;  /* offset+sizeがブロック長を超えている  */
	
	if ( block_num >= SIMPLEFS_IDATA_NR )
		return -E2BIG;  /*  ファイルサイズの上限を超えている  */

	memset(SIMPLEFS_REFER_DATA(fs_inode, block_num) + offset, 0,
	    size); /* ブロック内をクリアする */

	return 0;
}

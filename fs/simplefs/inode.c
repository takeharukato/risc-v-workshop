/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Simple file system inode operations                               */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>

#include <kern/mutex.h>
#include <kern/dev-pcache.h>
#include <kern/vfs-if.h>
#include <kern/timer-if.h>

#include <fs/simplefs/simplefs.h>

/**
   単純なファイルシステムのディレクトリI-nodeを初期化する(ファイル種別に依らない共通処理)
   @param[in] fs_inode 初期化対象ディレクトリのI-node
 */
static void
simplefs_init_inode_common(simplefs_inode *fs_inode){

	fs_inode->i_mode   = S_IRWXU|S_IRWXG|S_IRWXO; /* アクセス権を設定             */
	fs_inode->i_nlinks = 1;                       /* リンク数を設定               */

	fs_inode->i_uid   = FS_ROOT_UID;              /* ユーザIDを設定               */
	fs_inode->i_gid   = FS_ROOT_GID;              /* グループIDを設定             */
	fs_inode->i_major = 0;                        /* デバイスのメジャー番号を設定 */
	fs_inode->i_minor = 0;                        /* デバイスのマイナー番号を設定 */
	fs_inode->i_size  = 0;                        /* サイズを設定                 */
	fs_inode->i_atime = 0;                        /* アクセス時刻を0クリア        */
	fs_inode->i_mtime = 0;                        /* 更新時刻を0クリア            */
	fs_inode->i_ctime = 0;                        /* 属性更新時刻を0クリア        */	

	/* データブロックを初期化 */
	memset(&fs_inode->i_dblk, 0, sizeof(simplefs_data)*SIMPLEFS_IDATA_NR);
}

/**
   単純なファイルシステムのファイルを伸縮/解放(クリア)する
   @param[in] fs_super スーパブロック情報
   @param[in] fs_vnid  v-node ID
   @param[in] fs_inode 初期化対象ディレクトリのI-node
   @param[in] off      解放開始位置のオフセット(単位: バイト)
   @param[in] len      解放長(単位: バイト)
   @retval  0          正常終了
   @retval -EINVAL     offまたはlenに負の値を指定した
   @retval -EFBIG      ファイル長よりoffの方が大きい
 */
int
simplefs_inode_truncate_down(simplefs_super_block *fs_super, vfs_vnode_id fs_vnid, 
				 simplefs_inode *fs_inode, off_t off, off_t len){
	size_t          blksiz;
	off_t          cur_pos;
	size_t         clr_siz;
	size_t       clr_start;
	size_t     cur_rel_blk;
	size_t   first_rel_blk;
	size_t     end_rel_blk;
	size_t      remove_blk;
	
	if ( len == 0 )
		return 0; 	/* 解放長が0の場合は更新不要 */
	if ( ( 0 > off ) || ( 0 > len ) )
		return -EINVAL; /* オフセットまたは解放長に負の値を設定した  */
	
	if ( ( off > fs_inode->i_size ) || ( off > ( off + len ) ) )
		return -EFBIG; /* ファイル長よりオフセットの方が大きい */

	/* ファイルの終端を越える場合は, 削除長をファイル終端に補正する */
	if ( ( off + len ) > fs_inode->i_size )
		len = fs_inode->i_size - off;

	blksiz = fs_super->s_blksiz; /* ブロックサイズ取得 */
	first_rel_blk = truncate_align(off, blksiz) / blksiz; /* 開始ブロック算出 */
	end_rel_blk = roundup_align(off + len, blksiz) / blksiz;  /* 終了ブロック算出 */

	/* 終了位置がブロック境界と合っていない場合
	 * 終了ブロックを減算
	 */
	if ( ( end_rel_blk > first_rel_blk ) && addr_not_aligned(off + len, blksiz) )
		--end_rel_blk;  /* 終了ブロックを減算 */

	cur_pos = off; /* 削除開始位置をoffに設定 */
	cur_rel_blk = first_rel_blk; /* 削除開始ブロックを算出  */

	/* 開始点がブロック境界と合っておらず, 後続のブロックもクリアする場合
	 */
	if ( addr_not_aligned(off, blksiz) && ( end_rel_blk > first_rel_blk ) ) {

		clr_start = cur_pos % blksiz; /* ブロック中の削除開始オフセットを算出 */
		clr_siz = blksiz - clr_start; /* ブロック中の削除サイズを算出 */

		remove_blk = cur_rel_blk;     /* TODO: 物理ブロック番号を算出 */
		memset(SIMPLEFS_REFER_DATA(fs_inode, remove_blk) + clr_start, 0,
		    clr_siz); /* 開始ブロック内をクリアする */

		++cur_rel_blk; /* 次のブロックから削除を継続する */
		cur_pos = roundup_align(cur_pos, blksiz);   /*  削除位置を更新する  */
	}
	
	/* 削除範囲中のブロックをクリアする
	 */
	for( ; end_rel_blk > cur_rel_blk; ++cur_rel_blk ) {

		/* TODO: ビットマップを参照して有効なブロックであることを確認 */
		remove_blk = cur_rel_blk;    /* TODO: 物理ブロック番号を算出 */
		memset(SIMPLEFS_REFER_DATA(fs_inode, remove_blk), 0,
		    blksiz); /* ブロック内をクリアする */
		/* TODO: 割当てられているブロックを解放する */
		cur_pos += blksiz;  /*  削除位置を更新する  */
	}
	
	if ( ( off + len ) > cur_pos ) { /* 最終ブロック内をクリアする */

		/* TODO: ビットマップを参照して有効なブロックであることを確認 */
		remove_blk = cur_rel_blk;   /* ブロック番号を算出 */

		if ( addr_not_aligned(off, blksiz ) ||
		    ( ( off + len ) != fs_inode->i_size ) ) {

			/* 最終削除位置がブロック境界にそろっていない場合
			 * 最終ブロック内の指定範囲をクリアする
			 */
			/* ブロック中の削除開始オフセットを算出 */
			clr_start = cur_pos % blksiz;
			/* ブロック中の削除サイズを算出 */
			clr_siz = blksiz - clr_start;
			memset(SIMPLEFS_REFER_DATA(fs_inode, remove_blk) + clr_start, 0,
			    clr_siz); /* 最終ブロック内をクリアする */
		} else {  /* 最終ブロックを解放する場合 */

			memset(SIMPLEFS_REFER_DATA(fs_inode, remove_blk), 0,
			    blksiz); /* 最終ブロック内をすべてクリアする */
			/* TODO: 割当てられているブロックを解放する */
		}
	}

	/* ファイル終端までクリアした場合は, ファイルサイズを更新する
	 */	
	if ( ( off + len ) == fs_inode->i_size )
		fs_inode->i_size = off;  /* ファイルサイズを更新 */

	return 0;
}

/**
   単純なファイルシステムのデバイスI-nodeを初期化する
   @param[in] fs_inode 初期化対象ディレクトリのI-node
   @param[in] mode     ファイル種別/アクセス権
   @param[in] major    デバイスのメジャー番号
   @param[in] minor    デバイスのマイナー番号
   @retval  0      正常終了
   @retval -EINVAL デバイス以外のファイルを作ろうとした
 */
int
simplefs_device_inode_init(simplefs_inode *fs_inode, uint16_t mode,
    uint16_t major, uint16_t minor){

	if ( ( !S_ISCHR(mode) ) && ( !S_ISBLK(mode) ) )
		return -EINVAL;  /* デバイス以外のファイルを作ろうとした */

	simplefs_init_inode_common(fs_inode);  /* I-nodeの初期化共通処理 */

	fs_inode->i_mode = mode;   /* ファイル種別, アクセス権を設定  */
	fs_inode->i_major = major; /* デバイスのメジャー番号を設定 */
	fs_inode->i_minor = minor; /* デバイスのマイナー番号を設定 */

	return 0;
}

/**
   単純なファイルシステムの通常ファイル/ディレクトリI-nodeを初期化する
   @param[in] fs_inode 初期化対象ディレクトリのI-node
   @param[in] mode ファイル種別/アクセス権
   @retval  0      正常終了
   @retval -EINVAL 通常ファイル/ディレクトリ以外のファイルを作ろうとした
 */
int
simplefs_inode_init(simplefs_inode *fs_inode, uint16_t mode){

	if ( ( !S_ISREG(mode) ) && ( !S_ISDIR(mode) ) )
		return -EINVAL;  /* 通常ファイル/ディレクトリ以外のファイルを作ろうとした */

	simplefs_init_inode_common(fs_inode);  /* I-nodeの初期化共通処理 */

	fs_inode->i_mode |= mode; /* ファイル種別, アクセス権を設定  */

	if ( S_ISDIR(mode) )  /* ディレクトリの場合  */
		++fs_inode->i_nlinks;        /* 親ディレクトリからの参照分を加算 */

	return 0;
}

/**
   単純なファイルシステムの通常ファイル/デバイスファイルを削除する
   @param[in] fs_super スーパブロック情報
   @param[in] fs_vnid  v-node ID
   @param[in] fs_inode 初期化対象ディレクトリのI-node
   @retval  0      正常終了
 */
int
simplefs_inode_remove(simplefs_super_block *fs_super, vfs_vnode_id fs_vnid,
    simplefs_inode *fs_inode){

	bitops_clr(fs_vnid, &fs_super->s_inode_map); /* I-nodeを解放 */

	simplefs_init_inode_common(fs_inode);  /* I-nodeの情報をクリアする */

	return 0;
}


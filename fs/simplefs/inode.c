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
   @param[in] fs_inode 単純なファイルシステムのI-node情報
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
   単純なファイルシステムのI-nodeを割り当てる
   @param[in] fs_super   スーパブロック情報
   @param[out] fs_vnidp  割り当てたInodeのv-node ID
   @retval  0      正常終了
   @retval -ENOSPC    I-nodeに空きがない
 */
int
simplefs_alloc_inode(simplefs_super_block *fs_super, vfs_vnode_id *fs_vnidp){
	size_t idx;
	
	idx = bitops_ffc(&fs_super->s_inode_map);
	if ( idx == 0 )
		return -ENOSPC;  /*  I-nodeに空きがない  */

	--idx;  /* ビットマップインデックスに変換 */

	bitops_set(idx, &fs_super->s_inode_map); /* I-nodeを確保 */

	return 0;
}

/**
   単純なファイルシステムのI-nodeを解放する
   @param[in] fs_super スーパブロック情報
   @param[in] fs_vnid  v-node ID
   @param[in] fs_inode 単純なファイルシステムのI-node情報
   @retval  0      正常終了
   @retval -E2BIG     I-node番号が大きすぎる
   @retval -ENOENT    未割り当てのI-nodeを指定した
 */
int
simplefs_remove_inode(simplefs_super_block *fs_super, vfs_vnode_id fs_vnid){

	if ( fs_vnid >= SIMPLEFS_INODE_NR )
		return -E2BIG;  /*  I-node番号が大きすぎる  */

	if ( !bitops_isset(fs_vnid, &fs_super->s_inode_map) )
		return -ENOENT;  /*  未割り当てのI-nodeを指定した  */

	bitops_clr(fs_vnid, &fs_super->s_inode_map); /* I-nodeを解放 */

	return 0;
}

/**
   単純なファイルシステムのI-node情報を二次記憶から読み込む
   @param[in]  fs_super  単純なファイルシステムのスーパブロック情報
   @param[in]  fs_vnid   単純なファイルシステムのI-node番号
   @param[out] fs_inodep 単純なファイルシステムのI-node情報を指し示すポインタのアドレス
   @retval    0         正常終了
   @retval   -E2BIG     I-node番号が大きすぎる
   @retval   -ENOENT    未割り当てのI-nodeを指定した
 */
int
simplefs_read_inode(simplefs_super_block *fs_super, vfs_vnode_id fs_vnid, 
    simplefs_inode **fs_inodep){

	kassert( fs_inodep != NULL );
	
	if ( fs_vnid >= SIMPLEFS_INODE_NR )
		return -E2BIG;  /*  I-node番号が大きすぎる  */

	if ( !bitops_isset(fs_vnid, &fs_super->s_inode_map) )
		return -ENOENT;  /*  未割り当てのI-nodeを指定した  */

	*fs_inodep = SIMPLEFS_REFER_INODE(fs_super, fs_vnid);  /* I-nodeを参照する */

	return 0;
}

/**
   単純なファイルシステムのI-node情報を二次記憶に書き込む
   @param[in] fs_super  単純なファイルシステムのスーパブロック情報
   @param[in] fs_vnid   単純なファイルシステムのI-node番号
   @param[in] fs_inode  単純なファイルシステムのI-node情報
   @retval    0         正常終了
   @retval   -E2BIG     I-node番号が大きすぎる
   @retval   -ENOENT    未割り当てのI-nodeを指定した
 */
static int
simplefs_write_inode(simplefs_super_block *fs_super, vfs_vnode_id fs_vnid, 
    simplefs_inode *fs_inode){

	if ( fs_vnid >= SIMPLEFS_INODE_NR )
		return -E2BIG;  /*  I-node番号が大きすぎる  */

	if ( !bitops_isset(fs_vnid, &fs_super->s_inode_map) )
		return -ENOENT;  /*  未割り当てのI-nodeを指定した  */

	return 0;
}

/**
   単純なファイルシステムのファイルを伸長する
   @param[in] fs_super スーパブロック情報
   @param[in] fs_vnid  v-node ID
   @param[in] fs_inode 単純なファイルシステムのI-node情報
   @param[in] len      伸長する長さ(単位: バイト)
   @retval  0          正常終了
   @retval -EINVAL     offまたはlenに負の値を指定した
   @retval -EFBIG      ファイル長よりoffの方が小さい
 */
int
simplefs_inode_truncate_up(simplefs_super_block *fs_super, vfs_vnode_id fs_vnid, 
			     simplefs_inode *fs_inode, off_t len){
	int                       rc;
	int                      res;
	size_t                blksiz;
	off_t                cur_pos;
	size_t               old_siz;
	size_t               clr_siz;
	size_t             clr_start;
	simplefs_blkno   cur_rel_blk;
	simplefs_blkno first_rel_blk;
	simplefs_blkno   end_rel_blk;
	simplefs_blkno       new_blk;

	kassert( len != 0 );  /* 解放長が0の場合の処理は呼び出し元で実施済み */
	
	if ( 0 > len )
		return -EINVAL;      /* 伸長長に負の値を設定した */

	blksiz = fs_super->s_blksiz; /* ブロックサイズ取得       */

	old_siz = fs_inode->i_size;  /* 伸長前のサイズを取得     */


	first_rel_blk = truncate_align(old_siz, blksiz) / blksiz;     /* 開始ブロック算出 */
	end_rel_blk = roundup_align(old_siz + len, blksiz) / blksiz;  /* 終了ブロック算出 */

	cur_pos = old_siz;            /* 伸長対象位置を設定 */
	cur_rel_blk = first_rel_blk;  /* 伸長対象ブロック   */

	/* 開始点がブロック境界と合っておらず, 後続のブロックがある場合
	 */
	if ( addr_not_aligned(cur_pos, blksiz ) && ( end_rel_blk > first_rel_blk ) ) {

		rc = simplefs_read_mapped_block(fs_super, fs_inode,
		    first_rel_blk * blksiz, &new_blk); /* 物理ブロック番号算出 */
		kassert( rc == 0 );

		/* 既にブロックが割り当てられている場合
		 * ブロック内の指定された範囲をクリアする
		 */
		if ( rc == 0 ) {

			/* クリア開始オフセットを算出 */
			clr_start = cur_pos % blksiz;
			
			/* クリアサイズを算出 */
			clr_siz = blksiz - clr_start;
			
			/* 開始ブロック内をクリアする */
			rc = simplefs_clear_block(fs_super, fs_inode, new_blk,
			    clr_start, clr_siz);
			kassert( rc == 0 );
		
			++cur_rel_blk; /* 伸長対象ブロックを更新 */
			cur_pos = roundup_align(cur_pos, blksiz); /* 伸張対象位置を更新 */
		}
	}
	
	/* 伸長範囲中のゾーンを割り当てるループ
	 */
	for( ; end_rel_blk > cur_rel_blk; ++cur_rel_blk ) {

		rc = simplefs_read_mapped_block(fs_super, fs_inode,
		    cur_pos, &new_blk); /* 物理ブロック番号を算出 */
		kassert( rc == 0 );
		if ( rc == 0 ) {

			/* ブロックをクリアする。 */
			rc = simplefs_clear_block(fs_super, fs_inode, new_blk, 0, blksiz);
			kassert( rc == 0 );
		} else {

			/* @note 通常, ブロックが割り当てられていない場合は, 
			 * 新たにブロックを割り当てる。
			 * 割り当てに失敗した場合は, 割り当てたブロックを解放する。
			 * ファイル長のみでファイルを管理する場合は不要
			 */
			rc = simplefs_alloc_block(fs_super, &new_blk);
			if ( rc != 0 )
				goto unmap_block_out; /* 割り当てたブロックを解放する */

			/* ブロックをクリアする。 */
			rc = simplefs_clear_block(fs_super, fs_inode, new_blk, 0, blksiz);
			if ( rc != 0 ) {

				/* 割当てたブロックを解放する  */
				simplefs_free_block(fs_super, fs_inode, new_blk);
				goto unmap_block_out;
			}
			/* ブロックをマップする。
			 * ブロックをマップできなかった場合は, 割り当てたブロックを解放する。
			 */
			rc = simplefs_write_mapped_block(fs_super, fs_inode,
			    cur_pos, new_blk);
			if ( rc != 0 ) {

				/* 割当てたブロックを解放する  */
				simplefs_free_block(fs_super, fs_inode, new_blk);
				goto unmap_block_out;
			}
		}

		cur_pos += blksiz;   /* ブロック追加位置を更新 */
		fs_inode->i_size = cur_pos;  /* ファイルサイズを更新 */
	}

	/* 終端がブロック境界に沿っていない場合, ファイルサイズを伸長範囲に
	 * 収めるように補正する。
	 */
	if ( fs_inode->i_size > ( old_siz + len ) ) { 

		/*
		 * 終端がブロック境界に沿っていない場合
		 */
		cur_pos = old_siz + len;  /* 伸長対象位置を補正 */
		fs_inode->i_size = cur_pos;  /* ファイルサイズを更新 */
	}

	/* 二次記憶上のI-node情報を更新 
	 */
	rc = simplefs_write_inode(fs_super, fs_vnid, fs_inode);
	if ( rc != 0 ) {

		/* 最終ブロックまで削除するようにサイズを更新 */
		fs_inode->i_size = roundup_align(cur_pos, blksiz);
		goto unmap_block_out;
	}

	return 0;

unmap_block_out:
	/* 伸長したゾーンをアンマップ */
	res = simplefs_unmap_block(fs_super, fs_vnid, fs_inode, old_siz,
	    fs_inode->i_size - old_siz);
	kassert( res == 0 );

	/* ゾーン解放に伴って元のサイズに更新されていることを確認  */
	kassert( fs_inode->i_size == old_siz );  

	return rc;
}

/**
   単純なファイルシステムのファイルを伸縮/解放(クリア)する
   @param[in] fs_super スーパブロック情報
   @param[in] fs_vnid  v-node ID
   @param[in] fs_inode 単純なファイルシステムのI-node情報
   @param[in] off      解放開始位置のオフセット(単位: バイト)
   @param[in] len      解放長(単位: バイト)
   @retval  0          正常終了
   @retval -EINVAL     offまたはlenに負の値を指定した
   @retval -EFBIG      ファイル長よりoffの方が大きい
 */
int
simplefs_inode_truncate_down(simplefs_super_block *fs_super, vfs_vnode_id fs_vnid, 
				 simplefs_inode *fs_inode, off_t off, off_t len){
	int                       rc;
	size_t                blksiz;
	off_t                cur_pos;
	size_t               clr_siz;
	size_t             clr_start;
	simplefs_blkno   cur_rel_blk;
	simplefs_blkno first_rel_blk;
	simplefs_blkno   end_rel_blk;
	simplefs_blkno    remove_blk;
	
	kassert( len != 0 );  /* 解放長が0の場合の処理は呼び出し元で実施済み */

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

		rc = simplefs_read_mapped_block(fs_super, fs_inode,
		    cur_rel_blk * fs_inode->i_size, &remove_blk); /* 物理ブロック番号を算出 */
		kassert( ( rc != -EINVAL ) && ( rc != -E2BIG ) );
		if ( rc == 0 ) {  /* ブロックが割り当て済みの場合 */
			
			rc = simplefs_clear_block(fs_super, fs_inode, remove_blk,
			    clr_start, clr_siz);  /* ブロック内をクリアする */
			kassert( rc == 0 );
		}
		
		++cur_rel_blk; /* 次のブロックから削除を継続する */
		cur_pos = roundup_align(cur_pos, blksiz);   /*  削除位置を更新する  */
	}
	
	/* 削除範囲中のブロックをクリアする
	 */
	for( ; end_rel_blk > cur_rel_blk; ++cur_rel_blk ) {

		rc = simplefs_read_mapped_block(fs_super, fs_inode,
		    cur_rel_blk * fs_inode->i_size, &remove_blk); /* 物理ブロック番号を算出 */
		kassert( ( rc != -EINVAL ) && ( rc != -E2BIG ) );
		if ( rc == 0 ) {  /* ブロックが割り当て済みの場合, ブロックを解放する */

			/* ブロックをアンマップする */
			rc = simplefs_unmap_block(fs_super, fs_vnid, fs_inode,
			    cur_rel_blk * blksiz, blksiz);
			if ( rc != 0 ) 
				continue;  /* 無効化できなかったブロックをスキップする */
			
			/* ブロックを解放する */
			simplefs_free_block(fs_super, fs_inode, remove_blk);
		}
		cur_pos += blksiz;  /*  削除位置を更新する  */
	}
	
	if ( ( off + len ) > cur_pos ) { /* 最終ブロック内をクリアする */

		rc = simplefs_read_mapped_block(fs_super, fs_inode,
		    cur_rel_blk * fs_inode->i_size, &remove_blk); /* 物理ブロック番号を算出 */
		kassert( ( rc != -EINVAL ) && ( rc != -E2BIG ) );

		if ( addr_not_aligned(off, blksiz ) ||
		    ( ( off + len ) != fs_inode->i_size ) ) {

			/* 最終削除位置がブロック境界にそろっていない場合
			 * 最終ブロック内の指定範囲をクリアする
			 */
			/* ブロック中の削除開始オフセットを算出 */
			clr_start = cur_pos % blksiz;
			/* ブロック中の削除サイズを算出 */
			clr_siz = blksiz - clr_start;

			rc = simplefs_clear_block(fs_super, fs_inode, remove_blk,
			    clr_start, clr_siz);  /* ブロック内をクリアする */
			kassert( rc == 0 );
		} else { /* 最終ブロック全体を解放する場合 */

			/* ブロックをアンマップする */
			rc = simplefs_unmap_block(fs_super, fs_vnid, fs_inode,
			    cur_rel_blk * blksiz, blksiz);
			/* ブロックを解放する
			 * アンマップできなかったブロックは解放しない
			 */
			if ( rc == 0 ) 
				simplefs_free_block(fs_super, fs_inode, remove_blk);
		}
	}

	/* ファイル終端までクリアした場合は, ファイルサイズを更新する
	 */	
	if ( ( off + len ) == fs_inode->i_size )
		fs_inode->i_size = off;  /* ファイルサイズを更新 */

	return 0;
}

/**
   単純なファイルシステムのファイルサイズを更新する
   @param[in] fs_super スーパブロック情報
   @param[in] fs_vnid  v-node ID
   @param[in] fs_inode 単純なファイルシステムのI-node情報
   @param[in] len      更新後のファイルサイズ(単位: バイト)
   @retval  0          正常終了
   @retval -EINVAL     ファイルサイズが不正
 */
int
simplefs_inode_truncate(simplefs_super_block *fs_super, vfs_vnode_id fs_vnid, 
    simplefs_inode *fs_inode, off_t len){
	int       rc;
	ktimespec ts;
	
	if ( len == 0 )
		return 0;  /* サイズ変更がない場合は即時正常復帰する */
	if ( 0 > len )
		return -EINVAL;  /*  ファイルサイズに負の値を指定した */

	if ( len >= SIMPLEFS_SUPER_MAX_FILESIZE )
		return -EINVAL;   /* 最大ファイル長さを超えている  */
	
	if ( fs_inode->i_size > len )
		rc = simplefs_inode_truncate_down(fs_super, fs_vnid, 
		    fs_inode, len,
		    fs_inode->i_size - len);  /*  ファイルサイズを伸縮する  */
	else
		rc = simplefs_inode_truncate_up(fs_super, fs_vnid, 
		    fs_inode, len - fs_inode->i_size);  /*  ファイルサイズを伸長する  */

	if ( rc != 0 )
		goto error_out;  /* エラー復帰する  */
	/*
	 * ファイル書き込み時刻を更新する
	 */
	tim_walltime_get(&ts);  /* 現在時刻を得る */
	fs_inode->i_mtime = ts.tv_sec;  /* 最終書き込み時刻を更新する */

	rc = simplefs_write_inode(fs_super, fs_vnid, fs_inode); /* I-nodeを書き戻す */
	if ( rc != 0 )
		goto error_out;  /* エラー復帰する  */

	return 0;
	
error_out:
	return rc;
}

/**
   単純なファイルシステムのファイルサイズを更新する
   @param[in] fs_super  スーパブロック情報
   @param[in] fs_vnid   v-node ID
   @param[in] fs_inode  単純なファイルシステムのI-node情報
   @param[in] file_priv ファイルディスクリプタのプライベート情報
   @param[in] buf       読み込みバッファ
   @param[in] pos       ファイル内での読み込み開始オフセット(単位: バイト)
   @param[in] len       読み込み長
   @retval  0           正常終了
 */
int
simplefs_inode_read(simplefs_super_block *fs_super, vfs_vnode_id fs_vnid, 
    simplefs_inode *fs_inode, vfs_file_private file_priv,
    void *buf, off_t pos, ssize_t len){
	
	return 0;
}

/**
   単純なファイルシステムのデバイスI-nodeを初期化する
   @param[in] fs_inode 単純なファイルシステムのI-node情報
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
   @param[in] fs_inode 単純なファイルシステムのI-node情報
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


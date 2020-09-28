/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Simple file system directory entry operations                     */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>

#include <kern/mutex.h>
#include <kern/dev-pcache.h>
#include <kern/vfs-if.h>

#include <fs/simplefs/simplefs.h>

/**
   単純なファイルシステムのディレクトリエントリ中から指定された名前のエントリを検索する
   @param[in]  fs_super     単純なファイルシステムのスーパブロック情報
   @param[in]  fs_dir_inode 単純なファイルシステムのディレクトリを指すI-node
   @param[in]  name         検索対象の名前
   @param[out] fs_vnidp     見つかったエントリに対する単純なファイルシステムのI-node番号
   @retval  0      正常終了
   @retval -ENOTDIR ディレクトリではないI-nodeを指定した
   @retval -ENOENT  指定された名前が見つからなかった
 */
int
simplefs_dirent_lookup(simplefs_super_block *fs_super, simplefs_inode *fs_dir_inode,
    const char *name, simplefs_ino *fs_vnidp){
	int                 rc;
	off_t          cur_pos;
	simplefs_dent  cur_ent;	

	if ( !S_ISDIR(fs_dir_inode->i_mode) )
		return -ENOTDIR;  /*  ディレクトリではないI-nodeを指定した */

	/* ディレクトリエントリを順にたどる
	 */
	for(cur_pos = 0; fs_dir_inode->i_size > cur_pos; cur_pos += sizeof(simplefs_dent)) {

		/* ディレクトリエントリを読込む */
		rc = simplefs_inode_read(fs_super, SIMPLEFS_INODE_INVALID_INO, 
		    fs_dir_inode, NULL, &cur_ent, cur_pos, sizeof(simplefs_dent));

		if ( rc != 0 )
			continue;  /* ディレクトリエントリが読めなかった */

		if ( cur_ent.d_inode == SIMPLEFS_INODE_INVALID_INO )
			continue;  /* 無効なディレクトリエントリだった */
		
		/* ディレクトリエントリの名前が指定された名前と一致した場合 */
		if ( memcmp(name, cur_ent.d_name, SIMPLEFS_DIRSIZ) == 0 ) {

			if ( fs_vnidp != NULL )
				*fs_vnidp = cur_ent.d_inode;  /* I-node番号を返却 */
			goto success;  /* 正常復帰 */
		}
	}

	return -ENOENT;  /* ディレクトリエントリが見つからなかった  */
	
success:
	return 0;
}

/**
   単純なファイルシステムのディレクトリエントリ中から空きエントリを探す
   @param[in]  fs_super     単純なファイルシステムのスーパブロック情報
   @param[in]  fs_dir_inode 単純なファイルシステムのディレクトリを指すI-node
   @param[out] offsetp      空きディレクトリエントリまでのオフセット(単位: バイト)
   @retval  0       正常終了
   @retval -ENOTDIR ディレクトリではないI-nodeを指定した
   @retval -ENOSPC  空きエントリが見つからなかった
 */
int
simplefs_get_free_ent(simplefs_super_block *fs_super, simplefs_inode *fs_dir_inode,
    off_t *offsetp){
	int                 rc;
	off_t          cur_pos;
	simplefs_dent  cur_ent;	

	if ( !S_ISDIR(fs_dir_inode->i_mode) )
		return -ENOTDIR;  /*  ディレクトリではないI-nodeを指定した */

	/* ディレクトリエントリを順にたどる
	 */
	for(cur_pos = 0; fs_dir_inode->i_size > cur_pos; cur_pos += sizeof(simplefs_dent)) {

		/* ディレクトリエントリを読込む */
		rc = simplefs_inode_read(fs_super, SIMPLEFS_INODE_INVALID_INO, 
		    fs_dir_inode, NULL, &cur_ent, cur_pos, sizeof(simplefs_dent));

		if ( rc != 0 )
			continue;  /* ディレクトリエントリが読めなかった */

		if ( cur_ent.d_inode == SIMPLEFS_INODE_INVALID_INO ) {

			/* 空きディレクトリエントリを見つけた
			 */
			if ( offsetp != NULL )
				*offsetp = cur_pos;  /* 空きエントリまでのオフセットを返却 */
			goto success;
		}
	}

	if ( ( fs_dir_inode->i_size > cur_pos )
		&& ( ( fs_dir_inode->i_size - cur_pos ) > sizeof(simplefs_dent) ) ) {

		/* ディレクトリエントリを書き込む余裕がある
		 */
		if ( offsetp != NULL )
			*offsetp = fs_dir_inode->i_size;  /* ファイル末尾を返却 */
		goto success;
	}

	return -ENOSPC;  /* 空きディレクトリエントリが見つからなかった  */
	
success:
	return 0;
}

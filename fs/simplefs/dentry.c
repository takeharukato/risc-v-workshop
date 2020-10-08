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
#include <kern/vfs-if.h>

#include <fs/simplefs/simplefs.h>

/**
   単純なファイルシステムのディレクトリエントリ中から指定された名前のエントリを検索する (内部関数)
   @param[in]  fs_super     単純なファイルシステムのスーパブロック情報
   @param[in]  fs_dir_inode 単純なファイルシステムのディレクトリを指すI-node
   @param[in]  name         検索対象の名前
   @param[out] fs_vnidp     見つかったエントリに対する単純なファイルシステムのI-node番号
   @param[out] entp         見つかったエントリのファイル内オフセット位置 (単位: バイト)
   @retval  0      正常終了
   @retval -ENOTDIR ディレクトリではないI-nodeを指定した
   @retval -ENOENT  指定された名前が見つからなかった
 */
static int
simplefs_lookup_dirent_full(simplefs_super_block *fs_super, simplefs_inode *fs_dir_inode,
    const char *name, simplefs_ino *fs_vnidp, off_t *entp){
	int                 rc;
	off_t          cur_pos;
	simplefs_dent  cur_ent;
	size_t          cmplen;

	if ( !S_ISDIR(fs_dir_inode->i_mode) )
		return -ENOTDIR;  /*  ディレクトリではないI-nodeを指定した */

	cmplen = MIN(strlen(name) + 1,SIMPLEFS_DIRSIZ); /* ヌル文字を含めた長さで比較する */

	/* ディレクトリエントリを順にたどる
	 */
	for(cur_pos = 0; fs_dir_inode->i_size > cur_pos; cur_pos += sizeof(simplefs_dent)) {

		/* ディレクトリエントリを読込む */
		rc = simplefs_inode_read(fs_super, SIMPLEFS_INODE_INVALID_INO,
		    fs_dir_inode, NULL, &cur_ent, cur_pos, sizeof(simplefs_dent));

		if ( rc != sizeof(simplefs_dent) )
			continue;  /* ディレクトリエントリが読めなかった */

		if ( cur_ent.d_inode == SIMPLEFS_INODE_INVALID_INO )
			continue;  /* 無効なディレクトリエントリだった */

		/* ディレクトリエントリの名前が指定された名前と一致した場合 */
		if ( memcmp(name, cur_ent.d_name, cmplen) == 0 ) {

			if ( fs_vnidp != NULL )
				*fs_vnidp = cur_ent.d_inode;  /* I-node番号を返却 */
			if ( entp != NULL )
				*entp = cur_pos;  /* ディレクトリエントリのオフセットを返却 */
			goto success;  /* 正常復帰 */
		}
	}

	return -ENOENT;  /* ディレクトリエントリが見つからなかった  */

success:
	return 0;
}

/**
   単純なファイルシステムのディレクトリエントリ中から空きエントリを探す (内部関数)
   @param[in]  fs_super     単純なファイルシステムのスーパブロック情報
   @param[in]  fs_dir_inode 単純なファイルシステムのディレクトリを指すI-node
   @param[out] offsetp      空きディレクトリエントリまでのオフセット(単位: バイト)
   @retval  0       正常終了
   @retval -ENOTDIR ディレクトリではないI-nodeを指定した
   @retval -ENOSPC  空きエントリが見つからなかった
 */
static int
simplefs_get_free_dirent(simplefs_super_block *fs_super, simplefs_inode *fs_dir_inode,
    off_t *offsetp){
	ssize_t          rdlen;
	off_t          cur_pos;
	simplefs_dent  cur_ent;

	if ( !S_ISDIR(fs_dir_inode->i_mode) )
		return -ENOTDIR;  /*  ディレクトリではないI-nodeを指定した */

	/* ディレクトリエントリを順にたどる
	 */
	for(cur_pos = 0; fs_dir_inode->i_size > cur_pos; cur_pos += sizeof(simplefs_dent) ) {

		/* ディレクトリエントリを読込む */
		rdlen = simplefs_inode_read(fs_super, SIMPLEFS_INODE_INVALID_INO,
		    fs_dir_inode, NULL, &cur_ent, cur_pos, sizeof(simplefs_dent));

		if ( rdlen !=  sizeof(simplefs_dent) )
			continue;  /* ディレクトリエントリが読めなかった */

		if ( cur_ent.d_inode == SIMPLEFS_INODE_INVALID_INO ) {

			/* 空きディレクトリエントリを見つけた
			 */
			if ( offsetp != NULL )
				*offsetp = cur_pos;  /* 空きエントリまでのオフセットを返却 */
			goto success;
		}
	}

	if ( ( SIMPLEFS_SUPER_MAX_FILESIZE > cur_pos )
		&& ( ( SIMPLEFS_SUPER_MAX_FILESIZE - cur_pos ) > sizeof(simplefs_dent) ) ) {

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

/**
   単純なファイルシステムのディレクトリエントリ中にディレクトリエントリを追加する
   @param[in]  fs_super     単純なファイルシステムのスーパブロック情報
   @param[in]  fs_dir_vnid  単純なファイルシステムのディレクトリのI-node番号
   @param[in]  fs_dir_inode 単純なファイルシステムのディレクトリを指すI-node
   @param[in]  fs_vnid      作成するエントリの単純なファイルシステムのI-node番号
   @param[in]  name         作成するエントリの名前
   @retval  0       正常終了
   @retval -ENOTDIR ディレクトリではないI-nodeを指定した
   @retval -EEXIST  同じ名前のディレクトリエントリがすでに存在する
   @retval -ENOSPC  空きエントリが見つからなかった
   @retval -EIO     ディレクトリエントリの書き込みに失敗した
 */
int
simplefs_dirent_add(simplefs_super_block *fs_super, simplefs_ino fs_dir_vnid,
    simplefs_inode *fs_dir_inode, simplefs_ino fs_vnid, const char *name){
	int                 rc;
	ssize_t          wrlen;
	off_t          new_pos;
	simplefs_dent  new_ent;

	if ( !S_ISDIR(fs_dir_inode->i_mode) )
		return -ENOTDIR;  /*  ディレクトリではないI-nodeを指定した */

	rc = simplefs_lookup_dirent_full(fs_super, fs_dir_inode, name, NULL, NULL);
	if ( rc != -ENOENT ) {

		kassert( rc != -ENOTDIR );  /* ディレクトリを引き渡していることを確認 */
		rc = -EEXIST; /*  同じ名前のディレクトリエントリがすでに存在する */
		goto error_out;
	}

	/* ディレクトリエントリ追加位置を探す
	 */
	rc = simplefs_get_free_dirent(fs_super, fs_dir_inode, &new_pos);
	if ( rc != 0 ) {

		kassert( rc != -ENOTDIR );  /* ディレクトリを引き渡していることを確認 */
		rc = -ENOSPC;  /* 空きディレクトリエントリがない */
		goto error_out;
	}

	/* ディレクトリエントリを作成
	 */
	new_ent.d_inode = fs_vnid;  /*  I-node番号  */
	memcpy(&new_ent.d_name[0], name, SIMPLEFS_DIRSIZ);  /*  名前  */

	/* ディレクトリエントリを書き込む
	 */
	wrlen = simplefs_inode_write(fs_super, fs_dir_vnid,
	    fs_dir_inode, NULL, &new_ent, new_pos, sizeof(simplefs_dent));
	if ( wrlen != sizeof(simplefs_dent) ) {

		rc = -EIO;  /* 書き込みに失敗した */
		goto error_out;
	}

	return 0;

error_out:
	return rc;  /* ディレクトリエントリ作成に失敗した  */
}

/**
   単純なファイルシステムのディレクトリエントリ中にディレクトリエントリを削除する
   @param[in]  fs_super     単純なファイルシステムのスーパブロック情報
   @param[in]  fs_dir_vnid  単純なファイルシステムのディレクトリのI-node番号
   @param[in]  fs_dir_inode 単純なファイルシステムのディレクトリを指すI-node
   @param[in]  name         削除するエントリの名前
   @retval  0       正常終了
   @retval -ENOTDIR ディレクトリではないI-nodeを指定した
   @retval -ENOENT  指定された名前のエントリが存在しない
   @retval -EIO     ディレクトリエントリの書き込みに失敗した
 */
int
simplefs_dirent_del(simplefs_super_block *fs_super, simplefs_ino fs_dir_vnid,
    simplefs_inode *fs_dir_inode, const char *name){
	int                 rc;
	ssize_t          wrlen;
	off_t          ent_pos;
	simplefs_dent  new_ent;

	if ( !S_ISDIR(fs_dir_inode->i_mode) )
		return -ENOTDIR;  /*  ディレクトリではないI-nodeを指定した */

	/*  名前をキーにディレクトリエントリを検索する */
	rc = simplefs_lookup_dirent_full(fs_super, fs_dir_inode, name, NULL, &ent_pos);
	if ( rc != 0 ) {

		kassert( rc != -ENOTDIR );  /* ディレクトリを引き渡していることを確認 */
		rc = -ENOENT; /*  指定された名前のディレクトリエントリが存在しない */
		goto error_out;
	}

	/* ディレクトリエントリを削除
	 */
	new_ent.d_inode = SIMPLEFS_INODE_INVALID_INO;    /*  無効I-node番号  */
	memset(&new_ent.d_name[0], 0, SIMPLEFS_DIRSIZ);  /*  名前を削除      */

	/* ディレクトリエントリを書き込む
	 */
	wrlen = simplefs_inode_write(fs_super, fs_dir_vnid,
	    fs_dir_inode, NULL, &new_ent, ent_pos, sizeof(simplefs_dent));
	if ( wrlen != sizeof(simplefs_dent) ) {

		rc = -EIO;  /* 書き込みに失敗した */
		goto error_out;
	}

	return 0;

error_out:
	return rc;  /* ディレクトリエントリ更新に失敗した  */
}

/**
   単純なファイルシステムのディレクトリエントリを検索し, 指定された名前に対応するI-node番号を返却する
   @param[in]  fs_super     単純なファイルシステムのスーパブロック情報
   @param[in]  fs_dir_inode 単純なファイルシステムのディレクトリを指すI-node
   @param[in]  name         作成するエントリの名前
   @param[out] fs_vnidp     単純なファイルシステムのI-node番号返却領域
   @retval  0       正常終了
   @retval -ENOTDIR ディレクトリではないI-nodeを指定した
   @retval -ENOENT  指定された名前のエントリが存在しない
 */
int
simplefs_dirent_lookup(simplefs_super_block *fs_super, simplefs_inode *fs_dir_inode,
    const char *name, simplefs_ino *fs_vnidp){
	int                 rc;
	simplefs_ino      inum;

	if ( !S_ISDIR(fs_dir_inode->i_mode) )
		return -ENOTDIR;  /*  ディレクトリではないI-nodeを指定した */

	/* 指定された名前のディレクトリエントリを検索する
	 */
	rc = simplefs_lookup_dirent_full(fs_super, fs_dir_inode, name, &inum, NULL);
	if ( rc != 0 ) {

		kassert( rc != -ENOTDIR );  /* ディレクトリを引き渡していることを確認 */
		rc = -ENOENT; /*  指定された名前のディレクトリエントリが存在しない */
		goto error_out;
	}

	if ( fs_vnidp != NULL )
		*fs_vnidp = inum;  /* I-node番号を返却  */

	return 0;

error_out:
	return rc;
}

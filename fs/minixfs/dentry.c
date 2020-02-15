/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Minix file system directory entry operations                      */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>

#include <kern/spinlock.h>
#include <kern/dev-pcache.h>

#include <fs/minixfs/minixfs.h>

/**
   ディレクトリエントリ中の数値情報をバイトスワップして, 出力先にコピーする
   @param[in]  sbp    メモリ上のスーパブロック情報
   @param[in]  in_de  入力ディレクトリエントリ
   @param[out] out_de 出力ディレクトリエントリ
*/
static void
swap_diskdent(minix_super_block *sbp, minix_dentry *in_de, minix_dentry *out_de){

	/* I-node番号を取り出す
	 */
	if ( MINIX_SB_IS_V3(sbp) ) {

		out_de->d_dentry.v3.inode = *(minixv3_ino *)in_de;		
		if ( sbp->swap_needed )
			out_de->d_dentry.v3.inode = __bswap32( *(minixv3_ino *)in_de );
	} else {

		out_de->d_dentry.v12.inode = *(minixv2_ino *)in_de;
		if ( sbp->swap_needed )
		        out_de->d_dentry.v12.inode =
				__bswap16( *(minixv2_ino *)in_de );
	}

	/* ファイル名領域をコピーする */
	memmove((void *)out_de + MINIX_D_DIRNAME_OFFSET(sbp),
	    (void *)in_de + MINIX_D_DIRNAME_OFFSET(sbp), MINIX_D_DIRSIZ(sbp));
}
/**
   ディレクトリエントリを埋める
   @param[in]  sbp    メモリ上のスーパブロック情報
   @param[in]  name   ファイル名
   @param[in]  inum   I-node番号
   @param[out] out_de 出力ディレクトリエントリ
*/
static void
fill_minix_dent(minix_super_block *sbp, const char *name, minix_ino inum, 
    minix_dentry *out_de){
	size_t namelen;

	namelen = strlen(name);  /* ファイル名長を得る */

	/* I-node番号を設定 */
	if ( MINIX_SB_IS_V3(sbp) ) 
		out_de->d_dentry.v3.inode = inum;
	else
		out_de->d_dentry.v12.inode = inum;

	/* ファイル名をコピーする */
	memmove((void *)out_de + MINIX_D_DIRNAME_OFFSET(sbp),
	    name, MIN(MINIX_D_DIRSIZ(sbp), namelen)); /* ファイル名をコピー */
	memset((void *)out_de + MINIX_D_DIRNAME_OFFSET(sbp) + namelen,
	    0, MINIX_D_DIRSIZ(sbp) - namelen); /* 余り領域を0で埋める */
}
/**
   ディレクトリエントリ中から所定の名前を持ったエントリを検索する(内部関数)
   @param[in]  sbp    メモリ上のスーパブロック情報
   @param[in]  dirip  検索するディレクトリのI-node情報
   @param[in]  name   検索キーとなる名前
   @param[out] dep    ディレクトリ返却域 (ネイティブエンディアンに変換して返却)
   @param[out] offp   ディレクトリエントリのオフセット位置(単位:バイト)返却域
   @retval     0      正常終了
   @retval    -ESRCH  ディレクトリエントリが見つからなかった
   @retval    -ENOENT ゾーンが割り当てられていない
   @retval    -E2BIG  ファイルサイズの上限を超えている
   @retval    -EIO    ページキャッシュアクセスに失敗した
   @retval    -ENOSPC 空きゾーンがない
   @retval    -EINVAL 不正なスーパブロックを指定した
 */
int
lookup_dentry_by_name(minix_super_block *sbp, minix_inode *dirip, const char *name, 
    minix_dentry *dep, off_t *offp){
	int                rc;  /* 返り値                                        */
	minix_dentry    d_ent;  /* ディスク上のディレクトリエントリ              */
	minix_dentry    m_ent;  /* メモリ上のディレクトリエントリ                */
	obj_cnt_type      pos;  /* ディレクトリエントリ読み取り位置(単位:個)     */
	obj_cnt_type  nr_ents;  /* ディレクトリエントリ総数(単位:個)             */
	size_t          rdlen;  /* 読み込んだディレクトリエントリ長(単位:バイト) */

	/* @note diripがディレクトリのI-nodeであることは呼び出し元で確認
	 */
	nr_ents = MINIX_D_INODE(dirip, i_size) / MINIX_D_DENT_SIZE(sbp);

	for(pos = 0; nr_ents > pos; ++pos) {

		rc = minix_rw_zone(MINIX_NO_INUM(sbp), dirip, 
		    &d_ent, pos*MINIX_D_DENT_SIZE(sbp), MINIX_D_DENT_SIZE(sbp), 
		    MINIX_RW_READING, &rdlen);

		if ( rc != 0 )
			continue; /* パンチホールが存在しうるので読み飛ばす */

		if ( rdlen != MINIX_D_DENT_SIZE(sbp) )
			break;  /* ディレクトリエントリの終了 */

		swap_diskdent(sbp, &d_ent, &m_ent); /* ネイティブエンディアンに変換 */

		if ( strncmp(name, 
			(void *)&m_ent + MINIX_D_DIRNAME_OFFSET(sbp),
			MINIX_D_DIRSIZ(sbp)) == 0 ) { /* ディレクトリエントリが見つかった */

			/* ディレクトリエントリの内容をコピー */
			if ( dep != NULL )
				memmove(dep, &m_ent, sizeof(minix_dentry));

			/* オフセット位置を返却 */
			if ( offp != NULL )
				*offp = pos*MINIX_D_DENT_SIZE(sbp);

			return 0;
		}
	}

	return -ESRCH;
}

/**
   ディレクトリエントリ中から所定の名前を持ったエントリを検索する
   @param[in]  sbp    メモリ上のスーパブロック情報
   @param[in]  dirip  検索するディレクトリのI-node情報
   @param[in]  name   検索キーとなる名前
   @param[out] de     ディレクトリエントリ返却領域
   @retval     0      正常終了
   @retval    -ESRCH  ディレクトリエントリが見つからなかった
   @retval    -ENOENT ゾーンが割り当てられていない
   @retval    -E2BIG  ファイルサイズの上限を超えている
   @retval    -EIO    ページキャッシュアクセスに失敗した
   @retval    -ENOSPC 空きゾーンがない
   @retval    -EINVAL 不正なスーパブロックを指定した
 */
int
minix_lookup_dentry_by_name(minix_super_block *sbp, minix_inode *dirip, const char *name, 
    minix_dentry *de){
	
	/* ディレクトリエントリを検索し, その内容を返却する */
	return lookup_dentry_by_name(sbp, dirip, name, de, NULL);
}

/**
   ディレクトリエントリにエントリを追加する
   @param[in]  sbp      メモリ上のスーパブロック情報
   @param[in]  dir_inum 追加対象ディレクトリのI-node番号
   @param[in]  dirip    追加対象ディレクトリのI-node情報
   @param[in]  name     ファイル名
   @param[in]  inum     I-node番号
   @retval     0      正常終了
   @retval    -EEXIST ディレクトリエントリが見つからなかった
   @retval    -ENOENT ゾーンが割り当てられていない
   @retval    -E2BIG  ファイルサイズの上限を超えている
   @retval    -EIO    ページキャッシュアクセスに失敗した
   @retval    -ENOSPC 空きゾーンがない
   @retval    -EINVAL 不正なスーパブロックを指定した
 */
int
minix_add_dentry(minix_super_block *sbp, minix_ino dir_inum, minix_inode *dirip, 
    const char *name, minix_ino inum){
	int                rc;  /* 返り値                                         */
	minix_dentry    d_ent;  /* ディスク上のディレクトリエントリ               */
	minix_dentry    m_ent;  /* ネイティブエンディアンでのディレクトリエントリ */
	minix_dentry  new_ent;  /* 書き込みディレクトリエントリ                   */
	obj_cnt_type      pos;  /* ディレクトリエントリ読み取り位置(単位:個)      */
	obj_cnt_type  nr_ents;  /* ディレクトリエントリ総数(単位:個)              */
	size_t          rdlen;  /* 読み込んだディレクトリエントリ長(単位:バイト)  */
	size_t          wrlen;  /* 書き込んだディレクトリエントリ長(単位:バイト)  */

	/* @note diripがディレクトリのI-nodeであることは呼び出し元で確認
	 */

	rc = lookup_dentry_by_name(sbp, dirip, name, &m_ent, NULL);
	if ( rc == 0 ) {

		if ( MINIX_D_DENT_INUM(sbp, &m_ent) == inum )
			goto success; /* I-node番号が一致する場合は, 正常終了 */

		rc = -EEXIST;  /* エントリがすでに存在する */
		goto error_out;
	}

	/* 空きエントリを探す
	 */
	nr_ents = MINIX_D_INODE(dirip, i_size) / MINIX_D_DENT_SIZE(sbp);
	for(pos = 0; nr_ents > pos; ++pos) {

		rc = minix_rw_zone(MINIX_NO_INUM(sbp), dirip, 
		    &d_ent, pos*MINIX_D_DENT_SIZE(sbp), MINIX_D_DENT_SIZE(sbp), 
		    MINIX_RW_READING, &rdlen);

		if ( rc != 0 )
			continue; /* パンチホールが存在しうるので読み飛ばす */

		if ( rdlen != MINIX_D_DENT_SIZE(sbp) )
			break;  /* ディレクトリエントリの終了 */

		swap_diskdent(sbp, &d_ent, &m_ent); /* ネイティブエンディアンに変換 */
		if ( MINIX_D_DENT_INUM(sbp, &m_ent) == MINIX_NO_INUM(sbp) )
			goto write_dent; /* ディレクトリエントリを書き込む */
	}

	kassert(pos == nr_ents);  /* ディレクトリエントリの末尾に追記する */

write_dent:
	fill_minix_dent(sbp, name, inum, &new_ent); /* ディレクトリエントリを生成 */
	/* ディスク上のディレクトリエントリに変換 */
	swap_diskdent(sbp, &new_ent, &d_ent);
	rc = minix_rw_zone(dir_inum, dirip, 
	    &d_ent, pos*MINIX_D_DENT_SIZE(sbp), MINIX_D_DENT_SIZE(sbp), 
	    MINIX_RW_WRITING, &wrlen);
	if ( rc != 0 )
		goto error_out;  /* ディレクトリエントリの書き込みに失敗した場合 */

	if ( wrlen != MINIX_D_DENT_SIZE(sbp) ) { /* 書き込みが途中で失敗した場合 */

		/* 書き込んだ内容をクリアする */
		rc = minix_unmap_zone(dir_inum, dirip, 
		    pos*MINIX_D_DENT_SIZE(sbp), MINIX_D_DENT_SIZE(sbp));
		if ( rc != 0 )
			goto error_out;
	}

success:
	return 0;

error_out:
	return rc;
}

/**
   ディレクトリエントリからエントリを削除する
   @param[in]  sbp    メモリ上のスーパブロック情報
   @param[in]  dir_inum 追加対象ディレクトリのI-node番号
   @param[in]  dirip  検索するディレクトリのI-node情報
   @param[in]  name   ファイル名
   @param[out] inump  削除したディレクトリエントリのI-node番号返却域
   @retval     0      正常終了
   @retval    -ESRCH  ディレクトリエントリが見つからなかった
   @retval    -ENOENT ゾーンが割り当てられていない
   @retval    -E2BIG  ファイルサイズの上限を超えている
   @retval    -EIO    ページキャッシュアクセスに失敗した
   @retval    -ENOSPC 空きゾーンがない
   @retval    -EINVAL 不正なスーパブロックを指定した
 */
int
minix_del_dentry(minix_super_block *sbp, minix_ino dir_inum, minix_inode *dirip, 
    const char *name, minix_ino *inump){
	int             rc; /* 返り値                                         */
	minix_dentry m_ent; /* ネイティブエンディアンでのディレクトリエントリ */
	off_t          off; /* ファイル内でのオフセット位置(単位:バイト)      */

	/* @note diripがディレクトリのI-nodeであることは呼び出し元で確認
	 */
	rc = lookup_dentry_by_name(sbp, dirip, name, &m_ent, &off);
	if ( rc != 0 ) 
		goto error_out; /* ディレクトリエントリが見つからなかった */

	/* ディレクトリエントリを消去する */
	rc = minix_unmap_zone(dir_inum, dirip, off, MINIX_D_DENT_SIZE(sbp));
	if ( rc != 0 )
		goto error_out;

	if ( inump != NULL )
		*inump = MINIX_D_DENT_INUM(sbp, &m_ent); /* I-node番号を返却 */

	return 0;

error_out:
	return rc;
}

/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Simple file system super block operations                          */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>

#include <kern/mutex.h>
#include <kern/dev-pcache.h>
#include <kern/vfs-if.h>
#include <fs/simplefs/simplefs.h>

static simplefs_table g_simplefs_tbl=__SIMPLEFS_TABLE_INITIALIZER(&g_simplefs_tbl);


/**
   単純なファイルシステムのディレクトリI-nodeを初期化する(ファイル種別に依らない共通処理)
   @param[in] fs_inode 初期化対象ディレクトリのI-node
 */
void
simplefs_init_inode_common(simplefs_inode *fs_inode){
	/* 単純なファイルシステム全体のロックを獲得済み */
	kassert( mutex_locked_by_self(&g_simplefs_tbl.mtx) );

	fs_inode->i_mode  = S_IRWXU|S_IRWXG|S_IRWXO; /* アクセス権を設定   */
	fs_inode->i_nlinks = 1;                      /* リンク数を設定     */

	fs_inode->i_uid = FS_ROOT_UID;               /* ユーザIDを設定     */
	fs_inode->i_gid = FS_ROOT_GID;               /* グループIDを設定   */
	fs_inode->i_major = 0;                       /* デバイスのメジャー番号を設定 */
	fs_inode->i_minor = 0;                       /* デバイスのマイナー番号を設定 */
	fs_inode->i_size = 0;                        /* サイズを設定       */
	fs_inode->i_atime = 0;                       /* TODO: walltimeを代入する */
	fs_inode->i_mtime = 0;                       /* TODO: walltimeを代入する */
	fs_inode->i_ctime = 0;                       /* TODO: walltimeを代入する */	

	/* データブロックを初期化 */
	memset(&fs_inode->i_dblk, 0, sizeof(simplefs_data)*SIMPLEFS_IDATA_NR);
}

/**
   単純なファイルシステムのデバイスI-nodeを初期化する
   @param[in] fs_inode 初期化対象ディレクトリのI-node
   @param[in] mode     ファイル種別/アクセス権
   @param[in] major    デバイスのメジャー番号
   @param[in] minor    デバイスのマイナー番号
 */
int
simplefs_device_inode_init(simplefs_inode *fs_inode, uint16_t mode,
    uint16_t major, uint16_t minor){

	/* 単純なファイルシステム全体のロックを獲得済み */
	kassert( mutex_locked_by_self(&g_simplefs_tbl.mtx) );

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
   @param[in] mode     ファイル種別/アクセス権
 */
int
simplefs_inode_init(simplefs_inode *fs_inode, uint16_t mode){

	/* 単純なファイルシステム全体のロックを獲得済み */
	kassert( mutex_locked_by_self(&g_simplefs_tbl.mtx) );

	if ( ( !S_ISREG(mode) ) && ( !S_ISDIR(mode) ) )
		return -EINVAL;  /* 通常ファイル/ディレクトリ以外のファイルを作ろうとした */

	simplefs_init_inode_common(fs_inode);  /* I-nodeの初期化共通処理 */

	fs_inode->i_mode |= mode; /* ファイル種別, アクセス権を設定  */

	if ( S_ISDIR(mode) )  /* ディレクトリの場合  */
		++fs_inode->i_nlinks;        /* 親ディレクトリからの参照分を加算 */

	return 0;
}


/**
   単純なファイルシステムのスーパブロック情報を初期化する
   @param[in] super 単純なファイルシステムのスーパブロック情報
 */
static void
simplefs_init_super(simplefs_super_block *super){
	int i;

	/* 単純なファイルシステム全体のロックを獲得済み */
	kassert( mutex_locked_by_self(&g_simplefs_tbl.mtx) );

	super->s_private = NULL;  /* プライベート情報を初期化  */
	bitops_zero(&super->s_inode_map);	/* I-nodeマップをクリア */
	/* 予約I-nodeを使用済みに設定 */
	bitops_set(SIMPLEFS_INODE_RESERVED_INO, &super->s_inode_map); 
	/* ルートI-nodeを使用済みに設定 */
	bitops_set(SIMPLEFS_INODE_ROOT_INO, &super->s_inode_map); 

	/* I-nodeをゼロ初期化する */
	for(i = 0; SIMPLEFS_INODE_NR > i; ++i) 
		memset(&super->s_inode[i], 0, sizeof(simplefs_inode));

	/* TODO: ルートディレクトリのディレクトリエントリを作成する  */
	
	super->s_state = SIMPLEFS_SUPER_INITIALIZED;  /* 初期化済みに設定 */

	return;
}

/**
   単純なファイルシステムのスーパブロックを読み込む
   @param[in]  dev シンプルファイルシステムが記録されたブロックデバイスのデバイスID
   @param[out] sbp 情報返却領域
   @retval 0   正常終了
 */
int
simplefs_read_super(dev_id dev, simplefs_super_block *sbp){

	return 0;
}
/**
   単純なファイルシステムの初期化
 */
int
simplefs_init(void){
	int            i;
	
	/* 単純なファイルシステム全体をロックする  */
	mutex_lock(&g_simplefs_tbl.mtx);
	for( i = 0; SIMPLEFS_SUPER_NR > i; ++i) 
		simplefs_init_super(&g_simplefs_tbl.super_blocks[i]);

	/* 単純なファイルシステム全体に対するロックを解放する  */
	mutex_unlock(&g_simplefs_tbl.mtx);

	return 0;
}

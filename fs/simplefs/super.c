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
#include <kern/vfs-if.h>

#include <fs/simplefs/simplefs.h>

/** スーパーブロック表 */
static simplefs_table g_simplefs_tbl=__SIMPLEFS_TABLE_INITIALIZER(&g_simplefs_tbl);

/**
   単純なファイルシステムのスーパブロック情報を初期化する
   @param[in] fs_super 単純なファイルシステムのスーパブロック情報
 */
static void
simplefs_init_super(simplefs_super_block *fs_super){
	int                         rc;
	int                          i;
	simplefs_inode *root_dir_inode;
	
	mutex_init(&fs_super->mtx);  /*  排他用mutexを初期化する */

	fs_super->s_blksiz = SIMPLEFS_SUPER_BLOCK_SIZE;  /* ブロック長を初期化  */
	fs_super->s_private = NULL;  /* プライベート情報を初期化  */
	bitops_zero(&fs_super->s_inode_map);	/* I-nodeマップをクリア */
	/* 予約I-nodeを使用済みに設定 */
	bitops_set(SIMPLEFS_INODE_RESERVED_INO, &fs_super->s_inode_map); 
	/* ルートI-nodeを使用済みに設定 */
	bitops_set(SIMPLEFS_INODE_ROOT_INO, &fs_super->s_inode_map); 

	/* I-nodeをゼロ初期化する */
	for(i = 0; SIMPLEFS_INODE_NR > i; ++i) 
		memset(&fs_super->s_inode[i], 0, sizeof(simplefs_inode));

	/* ルートディレクトリのディレクトリエントリを作成する
	 */
	/* ルートディレクトリのI-nodeを参照する */
	rc = simplefs_refer_inode(fs_super, SIMPLEFS_INODE_ROOT_INO, &root_dir_inode);
	kassert( rc == 0 );

	/* ルートディレクトリのI-nodeをディレクトリとして初期化する */
	rc = simplefs_inode_init(root_dir_inode,
	    S_IFDIR|
	    VFS_VNODE_MODE_ACS_IRWXU|VFS_VNODE_MODE_ACS_IRWXG|VFS_VNODE_MODE_ACS_IRWXO);
	kassert( rc == 0 );

	/* ルートディレクトリの".", ".."エントリを作成する */
	rc = simplefs_dirent_add(fs_super, SIMPLEFS_INODE_ROOT_INO, root_dir_inode,
	    SIMPLEFS_INODE_ROOT_INO, ".");
	kassert( rc == 0 );
	rc = simplefs_dirent_add(fs_super, SIMPLEFS_INODE_ROOT_INO, root_dir_inode,
	    SIMPLEFS_INODE_ROOT_INO, "..");
	kassert( rc == 0 );

	fs_super->s_state = SIMPLEFS_SUPER_INITIALIZED;  /* 初期化済みに設定 */
	
	return;
}

/**
   単純なファイルシステムの空きスーパブロック情報を取得する
   @param[out] fs_superp 単純なファイルシステムのスーパブロック情報を指し示すポインタのアドレス
   @retval  0      空きスーパブロックが見つかった
   @retval -ENOENT 空きスーパブロックがない
   @note LO: 単純なファイルシステム全体をロック, スーパブロック情報のロックの順に獲得する
 */
int
simplefs_get_super(simplefs_super_block **fs_superp){
	int                       i;
	simplefs_super_block *super;
	
	/* 単純なファイルシステム全体をロックする  */
	mutex_lock(&g_simplefs_tbl.mtx);

	for( i = 0; SIMPLEFS_SUPER_NR > i; ++i) {

		super = &g_simplefs_tbl.super_blocks[i];  /* スーパブロック情報を参照 */

		mutex_lock(&super->mtx);  /* スーパブロック情報のロックを獲得 */
		if ( super->s_state == SIMPLEFS_SUPER_INITIALIZED ) {
			
			mutex_unlock(&super->mtx);  /* スーパブロック情報のロックを解放 */
			goto success;  /* 空きスーパブロックが見つかった */
		}
		mutex_unlock(&super->mtx);  /* スーパブロック情報のロックを解放 */
	}

	/* 単純なファイルシステム全体に対するロックを解放する  */
	mutex_unlock(&g_simplefs_tbl.mtx);
	return -ENOENT;

success:
	/* 単純なファイルシステム全体に対するロックを解放する  */
	mutex_unlock(&g_simplefs_tbl.mtx);
	if ( fs_superp != NULL )
		*fs_superp = super;  /* スーパブロック情報を返却する */

	return 0;
}

/**
   単純なファイルシステムの初期化
   @retval 0 正常終了
   @retval -ENOMEM   メモリ不足
 */
int
simplefs_init(void){
	int    i;
	int   rc;
	
	/* 単純なファイルシステム全体をロックする  */
	mutex_lock(&g_simplefs_tbl.mtx);

	for( i = 0; SIMPLEFS_SUPER_NR > i; ++i) 
		simplefs_init_super(&g_simplefs_tbl.super_blocks[i]); /* スーパブロックを初期化 */

	/* 単純なファイルシステム全体に対するロックを解放する  */
	mutex_unlock(&g_simplefs_tbl.mtx);

	rc = simplefs_register_filesystem();  /* ファイルシステムを登録する */
	kassert( ( rc == 0 ) || ( rc == -ENOMEM ) );

	return rc;
}

/**
   単純なファイルシステムの終了
 */
void
simplefs_finalize(void){

	simplefs_unregister_filesystem();  /* ファイルシステムの登録を抹消する */
	return ;
}

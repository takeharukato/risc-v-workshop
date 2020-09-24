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
#include <kern/timer-if.h>

#include <fs/simplefs/simplefs.h>

static simplefs_table g_simplefs_tbl=__SIMPLEFS_TABLE_INITIALIZER(&g_simplefs_tbl);

/**
   単純なファイルシステムのスーパブロック情報を初期化する
   @param[in] fs_super 単純なファイルシステムのスーパブロック情報
 */
static void
simplefs_init_super(simplefs_super_block *fs_super){
	int i;

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

	/* TODO: ルートディレクトリのディレクトリエントリを作成する  */
	
	fs_super->s_state = SIMPLEFS_SUPER_INITIALIZED;  /* 初期化済みに設定 */

	return;
}

/**
   単純なファイルシステムのスーパブロックを読み込む
   @param[in]  dev シンプルファイルシステムが記録されたブロックデバイスのデバイスID
   @param[out] fs_super 情報返却領域
   @retval 0   正常終了
 */
int
simplefs_read_super(dev_id dev, simplefs_super_block *fs_super){

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

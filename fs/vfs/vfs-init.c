/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Virtual File System Initialization                                */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>

#include <kern/vfs-if.h>

#include <fs/vfs/vfs-internal.h>
#include <kern/dev-if.h>

/**
   ファイルシステムの初期化
 */
void
vfs_init(void){

	vfs_init_filesystem_table();  /* ファイルシステムテーブルの初期化 */
	vfs_init_mount_table();       /* マウントテーブルの初期化         */
	vfs_init_ioctx();             /* I/Oコンテキストテーブルの初期化  */

#if 0 /* TODO: ページプール作成時に有効化 */

	/* 初期化順序に制限はないが, ブロックデバイスのページキャッシュ中に
	 * ブロックバッファを含むため,ブロックバッファの初期化を先に初期化する
	 */
	block_buffer_init();          /* ブロックバッファの初期化 */

	vfs_init_pageio();            /* ページI/O機構の初期化            */
#endif
}

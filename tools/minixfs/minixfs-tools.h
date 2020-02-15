/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  minix file system tools                                           */
/*                                                                    */
/**********************************************************************/
#if !defined(TOOLS_MINIXFS_MINIXFS_TOOLS_H)
#define TOOLS_MINIXFS_MINIXFS_TOOLS_H

#include <stdint.h>

/** デフォルトではバージョン3のファイルシステムを生成 */
#define MKFS_MINIXFS_DEFAULT_VERSION (3)
/** デフォルトでは8MiBのファイルシステムを生成 */
#define MKFS_MINIXFS_DEFAULT_SIZE_MB (8)

#define MKFS_MINIXFS_FSIMG_NAME_FMT "minixfs-v%d-fsimg.img"  /**< イメージファイル名 */

#define MKFS_MINIXFS_ROOT_INO        (1)  /**< ルートディレクトリ I-node番号 */

/**
   疑似ページキャッシュ
 */
typedef struct _page_cache{
	dev_id                       bdevid;  /**< デバイスID                           */
	size_t                        pgsiz;  /**< ページサイズ                   */
	off_t                        offset;  /**< オフセット (単位:バイト)             */
	void                       *pc_data;  /**< ページキャッシュデータへのポインタ   */
}page_cache;

/**
   ファイルシステムイメージ情報
 */
typedef struct _fs_image{
	char           *file;  /**< ファイル名               */
	size_t          size;  /**< ファイルサイズ           */
	void           *addr;  /**< ファイルマップ先アドレス */
}fs_image;
int fsimg_create(char *_filename, size_t _imgsiz);
int fsimg_pagecache_init(char *_filename);
#endif  /*  TOOLS_MINIXFS_MINIXFS_TOOLS_H  */

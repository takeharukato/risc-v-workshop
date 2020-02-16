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
#include <fs/minixfs/minixfs.h>

#define MKFS_MINIX_PATH_DELIMITER    '/'  /**< デリミタはスラッシュ */

#define MKFS_MINIX_FS_DEVID          (1)  /**< 疑似デバイスID */

/** デフォルトではバージョン3のファイルシステムを生成 */
#define MKFS_MINIXFS_DEFAULT_VERSION (3)
/** デフォルトでは8MiBのファイルシステムを生成 */
#define MKFS_MINIXFS_DEFAULT_SIZE_MB (8)

/** デフォルトでは1024ファイル */
#define MKFS_MINIXFS_DEFAULT_NR_INODES (4096)

#define MKFS_MINIXFS_FSIMG_NAME_FMT "minixfs-v%d-fsimg.img"  /**< イメージファイル名 */

#define MKFS_MINIXFS_ROOT_INO        (1)  /**< ルートディレクトリ I-node番号 */
#define MKFS_MINIXFS_ROOT_INO_LINKS  (2)  /**< ルートディレクトリ の参照数("."と"..") */
#define MKFS_MINIXFS_ROOT_INO_UID    (2)  /**< daemonのユーザID */
#define MKFS_MINIXFS_ROOT_INO_GID    (2)  /**< daemonのグループID */
#define MKFS_MINIXFS_ROOT_INO_MODE   (0040777)  /**< ルートディレクトリのi_mode */
#define MKFS_MINIXFS_REGFILE_MODE    (0100777)  /**< 通常ファイルのi_mode */

/**                                  
   ファイルシステムイメージ情報
 */
typedef struct _fs_image{
	char                     *file;  /**< ファイル名               */
	size_t                    size;  /**< ファイルサイズ           */
	void                     *addr;  /**< ファイルマップ先アドレス */
	struct _minix_super_block  msb;  /**< スーパブロック情報       */
	struct _minix_inode       root;  /**< ルートI-node情報         */
}fs_image;
int fsimg_create(char *filename, int _version, int _nr_inodes, size_t _imgsiz, 
    fs_image **_handlep);
int fsimg_pagecache_init(char *_filename, fs_image **_handlep);
int create_superblock(struct _fs_image *img, int _version, int _nr_inodes);
int path_get_basename(char *_path, char **_dir, char **_name);
int path_to_minix_inode(struct _minix_super_block *_sbp, struct _minix_inode *_root, 
    char *_path, struct _minix_inode *_outv);
int create_regular_file(struct _fs_image *_handle, minix_ino _dir_inum,
    struct _minix_inode *_dirip, const char *_name, uint32_t _mode, minix_ino *_new_inum);
#endif  /*  TOOLS_MINIXFS_MINIXFS_TOOLS_H  */

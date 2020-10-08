/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Virtual file system directory entry definitions                   */
/*                                                                    */
/**********************************************************************/
#if !defined(_FS_VFS_VFS_DIRENT_H)
#define  _FS_VFS_VFS_DIRENT_H

#if !defined(ASM_FILE)

#include <klib/freestanding.h>
#include <klib/container.h>

#include <kern/kern-types.h>
#include <fs/vfs/vfs-types.h>

/**
   VFSディレクトリエントリ情報
   (Linuxのlinux_dirent 構造体相当)
   @note d_nameの先頭アドレスからヌルターミネートされた文字列が続き,
   最後の1バイトにd_typeエントリが入る
 */
typedef struct _vfs_dirent{
	vfs_vnode_id  d_ino;  /* vnode番号 */
	off_t         d_off;  /* 次のディレクトリエントリのオフセット位置(単位:バイト) */
	uint16_t   d_reclen;  /* エントリ長(単位: バイト)  */
	char      d_name[1];  /* ファイル名 (ヌル終端を含む文字列の先頭) */
}vfs_dirent;

/**
   ディレクトリエントリのファイル名へのオフセット位置
 */
#define VFS_DIRENT_NAME_OFF (offset_of(struct _vfs_dirent, d_name))

/**
   ディレクトリエントリのd_typeのオフセット位置を算出する
   @param[in] _namelen ファイル名の文字列長(ヌル終端を含まない文字列の長さ)
 */
#define VFS_DIRENT_DENT_TYPE_OFF(_namelen) \
	((size_t)( (offset_of(struct _vfs_dirent, d_name)) + (_namelen) + 1 ))

/**
   ディレクトリエントリのサイズを算出する
   @param[in] _namelen ファイル名の文字列長(ヌル終端を含まない文字列の長さ)
 */
#define VFS_DIRENT_DENT_SIZE(_namelen) \
	( VFS_DIRENT_DENT_TYPE_OFF(_namelen) + 1 )
/**
   ディレクトリエントリのd_typeメンバの値
 */
enum {
	DT_UNKNOWN = 0,  /**< ファイルタイプが不明                  */
	DT_FIFO = 1,     /**< 名前付きパイプ                        */
	DT_CHR = 2,      /**< キャラクタ型デバイス                  */
	DT_DIR = 4,      /**< ディレクトリ                          */
	DT_BLK = 6,      /**< ブロックデバイス                      */
	DT_REG = 8,      /**< 通常のファイル                        */
	DT_LNK = 10,     /**< シンボリックリンク                    */
	DT_SOCK = 12,    /**< UNIXドメインソケット                  */
	DT_WHT = 14      /**< Unionファイルシステムのホワイトアウト */
};

#endif  /* !ASM_FILE */
#endif  /*  _FS_VFS_VFS_DIRENT_H  */

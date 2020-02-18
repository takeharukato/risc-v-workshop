/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Virtual file system constant values                               */
/*                                                                    */
/**********************************************************************/
#if !defined(_FS_VFS_VFS_CONSTS_H)
#define  _FS_VFS_VFS_CONSTS_H 

#if !defined(ASM_FILE)
#include <klib/freestanding.h>
#include <klib/misc.h>

#define VFS_PATH_MAX     ULONGLONG_C(1024)  /**< パス長        */
#define VFS_FSNAME_MAX   (256)              /**< ファイルシステム名(ヌル終端含む) */
#define VFS_PATH_DELIM   '/'                /**< パスデリミタ  */
#define MAX_FD_TABLE_SIZE       (2048)  /**<  最大ファイルディスクリプタテーブルエントリ数  */
/**<  デフォルトファイルディスクリプタテーブルエントリ数 */
#define DEFAULT_FD_TABLE_SIZE   (128)   

#endif  /*  !ASM_FILE */
#endif  /*  _FS_VFS_VFS_CONSTS_H  */

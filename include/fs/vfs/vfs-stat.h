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

#include <klib/misc.h>

/** ファイル種別
 */
#define VFS_VNODE_MODE_NONE     (UINT32_C(0000000)) /**<  ファイル種別未設定    */
#define VFS_VNODE_MODE_SOCK     (UINT32_C(0140000)) /**<  ソケット              */
#define VFS_VNODE_MODE_FLINK    (UINT32_C(0120000)) /**<  シンボリックリンク    */
#define VFS_VNODE_MODE_REG      (UINT32_C(0100000)) /**<  通常ファイル          */
#define VFS_VNODE_MODE_BLK      (UINT32_C(0060000)) /**<  ブロックデバイス      */
#define VFS_VNODE_MODE_DIR      (UINT32_C(0040000)) /**<  ディレクトリ          */
#define VFS_VNODE_MODE_CHR      (UINT32_C(0020000)) /**<  キャラクタ型デバイス  */
#define VFS_VNODE_MODE_FIFO     (UINT32_C(0010000)) /**<  名前付きPIPE          */
#define VFS_VNODE_MODE_IFMT     (UINT32_C(0170000)) /**<  ファイル種別マスク    */

#define VFS_VNODE_MODE_ACS_IRWXU  (UINT32_C(00700)) /**< ファイル所有者用ビットマスク      */
#define VFS_VNODE_MODE_ACS_IRUSR  (UINT32_C(00400)) /**< 所有者の読み込み許可              */
#define VFS_VNODE_MODE_ACS_IWUSR  (UINT32_C(00200)) /**< 所有者の書き込み許可              */
#define VFS_VNODE_MODE_ACS_IXUSR  (UINT32_C(00100)) /**< 所有者の実行許可                  */
#define VFS_VNODE_MODE_ACS_IRWXG  (UINT32_C(00070)) /**< グループ用ビットマスク            */
#define VFS_VNODE_MODE_ACS_IRGRP  (UINT32_C(00040)) /**< グループの読み込み許可            */
#define VFS_VNODE_MODE_ACS_IWGRP  (UINT32_C(00020)) /**< グループの書き込み許可            */
#define VFS_VNODE_MODE_ACS_IXGRP  (UINT32_C(00010)) /**< グループの実行許可                */
#define VFS_VNODE_MODE_ACS_IRWXO  (UINT32_C(00007)) /**< 他人 (others) 用ビットマスク      */
#define VFS_VNODE_MODE_ACS_IROTH  (UINT32_C(00004)) /**< 他人の読み込み許可                */
#define VFS_VNODE_MODE_ACS_IWOTH  (UINT32_C(00002)) /**< 他人の書き込み許可                */
#define VFS_VNODE_MODE_ACS_IXOTH  (UINT32_C(00001)) /**< 他人の実行許可                    */
#define VFS_VNODE_MODE_ACS_ISUID  (UINT32_C(04000)) /**< set-user-ID bit                   */
#define VFS_VNODE_MODE_ACS_ISGID  (UINT32_C(02000)) /**< set-group-ID bit                  */
#define VFS_VNODE_MODE_ACS_ISVTX  (UINT32_C(01000)) /**< スティッキー・ビット              */
#define VFS_VNODE_MODE_ACS_ISMSK  (UINT32_C(07000)) /**< user/group/スティッキーbit マスク */

/** アクセスビットマスク  */
#define VFS_VNODE_MODE_ACS_MASK						\
	( VFS_VNODE_MODE_ACS_IRWXU |					\
	  VFS_VNODE_MODE_ACS_IRWXG |					\
	  VFS_VNODE_MODE_ACS_IRWXO |					\
	  VFS_VNODE_MODE_ACS_ISMSK )

#define S_IFMT  (VFS_VNODE_MODE_IFMT)    /* ファイル種別を示すビット領域を表すビットマスク */
#define S_IFREG (VFS_VNODE_MODE_REG)     /* レギュラーファイル                             */
#define S_IFBLK (VFS_VNODE_MODE_BLK)     /* block special                                  */
#define S_IFDIR (VFS_VNODE_MODE_DIR)  	 /* directory */
#define S_IFCHR (VFS_VNODE_MODE_CHR)	 /* character special */
#define S_IFIFO (VFS_VNODE_MODE_FIFO)	    /* this is a FIFO */
#define S_ISUID (VFS_VNODE_MODE_ACS_ISUID)  /* set user id on execution */
#define S_ISGID (VFS_VNODE_MODE_ACS_ISGID)  /* set group id on execution */
				/* next is reserved for future use */
#define S_ISVTX (VFS_VNODE_MODE_ACS_ISVTX)	/* save swapped text even after use */

/* POSIX masks for st_mode. */
#define S_IRWXU   (UINT32_C(00700))		/* owner:  rwx------ */
#define S_IRUSR   (UINT32_C(00400))		/* owner:  r-------- */
#define S_IWUSR   (UINT32_C(00200))		/* owner:  -w------- */
#define S_IXUSR   (UINT32_C(00100))		/* owner:  --x------ */

#define S_IRWXG   (UINT32_C(00070))		/* group:  ---rwx--- */
#define S_IRGRP   (UINT32_C(00040))		/* group:  ---r----- */
#define S_IWGRP   (UINT32_C(00020))		/* group:  ----w---- */
#define S_IXGRP   (UINT32_C(00010))		/* group:  -----x--- */

#define S_IRWXO   (UINT32_C(00007))		/* others: ------rwx */
#define S_IROTH   (UINT32_C(00004))		/* others: ------r-- */ 
#define S_IWOTH   (UINT32_C(00002))		/* others: -------w- */
#define S_IXOTH   (UINT32_C(00001))		/* others: --------x */


#endif  /*  !ASM_FILE  */
#endif /*  _FS_VFS_VFS_DIRENT_H   */

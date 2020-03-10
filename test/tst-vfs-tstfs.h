/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  file system for test                                              */
/*                                                                    */
/**********************************************************************/
#if !defined(_KERN_TST_VFS_TSTFS_H)
#define _KERN_TST_VFS_TSTFS_H

#include <klib/freestanding.h>
#include <kern/kern-types.h>

#define TST_VFS_TSTFS_MAGIC      (0xfeedfeed)
#define TST_VFS_TSTFS_ROOT_VNID  (2)
#define TST_VFS_TSTFS_DEV_MAJOR  (0xfffffffe)
#define TST_VFS_TSTFS_NAME       "tstfs"
#define TST_VFS_TSTFS_FNAME_LEN   (60)
#define TST_VFS_NR_INODES        (128)
#define TST_VFS_TSTFS_SSTATE_NONE (0)
typedef uint32_t tst_vfs_tstfs_ino;
/**
   テスト用ファイルシステムディレクトリエントリ
 */
typedef struct _tst_vfs_tstfs_dent{
	RB_ENTRY(_tst_vfs_tstfs_dent)   ent; /**< I-node中ディレクトリエントリへのリンク */
	tst_vfs_tstfs_ino               ino; /**< I-node番号                             */
	char  name[TST_VFS_TSTFS_FNAME_LEN]; /**< ファイル名(ヌルターミネート有り)       */
}tst_vfs_tstfs_dent;

/**
   テスト用ファイルシステムデータページ
 */
typedef struct _tst_vfs_tstfs_dpage{
	RB_ENTRY(_tst_vfs_tstfs_dpage) ent; /**< I-node中のデータページ管理情報のリンク */
	off_t                        index; /**< オフセットページアドレス               */
	void                         *page; /**< データページ                           */
}tst_vfs_tstfs_dpage;

/**
   テスト用ファイルシステムI-node
 */
typedef struct _tst_vfs_tstfs_inode{
	struct _mutex                   mtx; /**< 排他用ミューテックス */
	tst_vfs_tstfs_ino             i_ino;  /**< I-node番号                       */
	uint16_t                     i_mode;  /**< ファイル種別/保護属性            */
	uint16_t                   i_nlinks;  /**< ファイルのリンク数 (単位:個)     */
	uint32_t                     i_size;  /**< ファイルサイズ (単位:バイト)     */
	RB_ENTRY(_tst_vfs_tstfs_inode)  ent; /**< スーパブロック中のI-node情報のリンク */
	RB_HEAD(_tst_vfs_tstfs_dpage_tree, _tst_vfs_tstfs_dpage) dpages;  /**< データページ  */
	/** ディレクトリエントリ  */
	RB_HEAD(_tst_vfs_tstfs_dent_tree, _tst_vfs_tstfs_dent) dents;  
}tst_vfs_tstfs_inode;

/**
   テスト用ファイルシステムスーパブロック情報
 */
typedef struct _tst_vfs_tstfs_super{
	struct _mutex        mtx; /**< 排他用ミューテックス */
	dev_id           s_devid;  /**< デバイスID                     */
	uint32_t         s_magic;  /**< スーパブロックのマジック番号   */
	uint32_t         s_state;  /**< マウント状態                   */
	BITMAP_TYPE(, uint64_t, TST_VFS_NR_INODES)  s_inode_map;  /**< I-nodeマップ */
	RB_ENTRY(_tst_vfs_tstfs_super) ent; /**< ファイルシステムDBへのリンク */
	RB_HEAD(_tst_vfs_tstfs_inode_tree, _tst_vfs_tstfs_inode) s_inodes; /**< I-node */
}tst_vfs_tstfs_super;

/**
   テスト用ファイルシステムDB
 */
typedef struct _tst_vfs_tstfs_db{
	struct _mutex                                         mtx; /**< 排他用ミューテックス */
	RB_HEAD(_tst_vfs_tstfs_super_tree, _tst_vfs_tstfs_super) supers; /**< スーパブロック */
}tst_vfs_tstfs_db;

#endif  /*  _KERN_TST_VFS_TSTFS_H  */

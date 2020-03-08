/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  file system for test                                              */
/*                                                                    */
/**********************************************************************/
#if !defined(_KERN_TST_VFS_TSTFS_H)
#define _KERN_TST_VFS_TSTFS_H

#define TST_VFS_TSTFS_MAGIC (0xfeedfeed)
#define TST_VFS_TSTFS_VN_MAX    (5)
#define TST_VFS_TSTFS_ROOT_VNID (2)
#define TST_VFS_TSTFS_DEV_VNID  (3)
#define TST_VFS_TSTFS_DEV       (4)
#define TST_VFS_TSTFS_NAME      "tstfs"
#define TST_VFS_TSTFS_FNAME_LEN (60)
#define TST_VFS_NR_INODES       (128)

/**
   テスト用ファイルシステムディレクトリエントリ
 */
typedef struct _tst_vfs_tstfs_dent{
	RB_ENTRY(_tst_vfs_tstfs_dent)   ent; /**< I-node中ディレクトリエントリへのリンク */
	uint32_t                        ino; /**< I-node番号                             */
	char  name[TST_VFS_TSTFS_FNAME_LEN]; /**< ファイル名(ヌルターミネート有り)       */
}tst_vfs_tstfs_dent;

/**
   テスト用ファイルシステムデータページ
 */
typedef struct _tst_vfs_tstfs_data{
	RB_ENTRY(_tst_vfs_tstfs_data) ent; /**< I-node中のデータページ管理情報のリンク */
	uintptr_t                  offset; /**< ファイル内オフセットアドレス           */
	void                        *page; /**< データページ                           */
}tst_vfs_tstfs_data;

/**
   テスト用ファイルシステムI-node
 */
typedef struct _tst_vfs_tstfs_inode{
	uint16_t                     i_mode;  /**< ファイル種別/保護属性            */
	uint16_t                   i_nlinks;  /**< ファイルのリンク数 (単位:個)     */
	uint16_t                      i_uid;  /**< ファイル所有者のユーザID         */
	uint16_t                      i_gid;  /**< ファイル所有者のグループID       */
	uint32_t                     i_size;  /**< ファイルサイズ (単位:バイト)     */
	uint32_t                    i_atime;  /**< 最終アクセス時刻 (単位:UNIX時刻) */
	uint32_t                    i_mtime;  /**< 最終更新時刻 (単位:UNIX時刻)     */
	uint32_t                    i_ctime;  /**< 最終属性更新時刻 (単位:UNIX時刻) */
	RB_HEAD(_tst_vfs_tstfs_data_tree, _tst_vfs_tstfs_data) datas;  /**< データページ  */
	/** ディレクトリエントリ  */
	RB_HEAD(_tst_vfs_tstfs_dent_tree, _tst_vfs_tstfs_dent) dents;  
}tst_vfs_tstfs_inode;

/**
   テスト用ファイルシステムスーパブロック情報
 */
typedef struct _tst_vfs_tstfs_super{
	uint32_t         s_magic;  /**< スーパブロックのマジック番号   */
	uint32_t         s_state;  /**< マウント状態                   */
	BITMAP_TYPE(, uint64_t, TST_VFS_NR_INODES)  inode_map;  /**< I-nodeマップ */
	RB_HEAD(_tst_vfs_tstfs_inode_tree, _tst_vfs_tstfs_inode) s_inodes; /**< I-node */
}tst_vfs_tstfs_super;

#endif  /*  _KERN_TST_VFS_TSTFS_H  */

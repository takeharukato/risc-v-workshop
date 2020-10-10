/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  simple file system definitions                                    */
/*                                                                    */
/**********************************************************************/
#if !defined(_FS_SIMPLEFS_SIMPLEFS_H)
#define _FS_SIMPLEFS_SIMPLEFS_H

#if !defined(ASM_FILE)

#include <klib/freestanding.h>
#include <klib/bitops.h>
#include <kern/kern-types.h>
#include <kern/mutex.h>

#define SIMPLEFS_FSNAME      "simplefs" /**< ファイルシステム名 */
/** 最大マウントポイント数 (単位:個) */
#define SIMPLEFS_SUPER_NR      (2)
/** 最大ファイル数 (単位:個) */
#define SIMPLEFS_INODE_NR     (64)
/**< ファイル中の最大データブロック数 (単位:個)  */
#define SIMPLEFS_IDATA_NR    (128)
/** NULL終端を含まない最大ファイル名長 (単位:バイト) */
#define SIMPLEFS_DIRSIZ       (60)
/** データブロック長 (単位:バイト) */
#define SIMPLEFS_SUPER_BLOCK_SIZE     (UINT64_C(4096))
/** 最大ファイルサイズ (単位:バイト) */
#define SIMPLEFS_SUPER_MAX_FILESIZE   ( SIMPLEFS_SUPER_BLOCK_SIZE * SIMPLEFS_IDATA_NR )

#define SIMPLEFS_SUPER_INVALID_BLOCK  ((UINT64_C(1)<<32) - 1)   /**< 無効ブロック       */

#define SIMPLEFS_SUPER_UNINITIALIZED  (0x0)   /**< 未初期化           */
#define SIMPLEFS_SUPER_INITIALIZED    (0x1)   /**< 初期化済み         */
#define SIMPLEFS_SUPER_MOUNTED        (0x2)   /**< マウントされている */

#define SIMPLEFS_INODE_RESERVED_INO   (0x0)   /**< 予約I-node番号     */
#define SIMPLEFS_INODE_INVALID_INO   \
	SIMPLEFS_INODE_RESERVED_INO           /**< 無効I-node番号     */
#define SIMPLEFS_INODE_ROOT_INO       (0x1)   /**< ルートI-node番号   */

/** ファイルモードを抽出マスク */
#define SIMPLEFS_INODE_MODE_MASK     ((UINT32_C(1) << 16) - 1)

#define SIMPLEFS_IOCTL_CMD_GETINODE  (0x1)    /**< i-node取得 */

typedef uint32_t          simplefs_ino;   /**< I-node番号    */
typedef uint32_t        simplefs_blkno;   /**< ブロック番号  */
typedef void *   simplefs_file_private;   /**< プライベート情報  */

/* ディレクトリエントリ中のファイル名までのオフセット (単位: バイト) */
#define SIMPLEFS_D_DIRNAME_OFFSET             (sizeof(simplefs_ino))

/**
   単純ファイルシステムのデータエントリ
 */
typedef struct _simplefs_data{
	uint64_t data[SIMPLEFS_SUPER_BLOCK_SIZE/8];  /**< 1ページ分 */
}simplefs_data;

/**
   単純ファイルシステムのディレクトリエントリ
 */
typedef struct _simplefs_dent{
	simplefs_ino            d_inode;  /**< I-node番号                                  */
	char    d_name[SIMPLEFS_DIRSIZ];  /**< ファイル名 (NULL終端なし, 60バイト) */
}simplefs_dent;

/**
   単純ファイルシステムのファイル管理情報
 */
typedef struct _simplefs_inode{
	uint16_t                     i_mode;  /**< ファイル種別/保護属性            */
	uint16_t                   i_nlinks;  /**< ファイルのリンク数 (単位:個)     */
	uint16_t                      i_uid;  /**< ファイル所有者のユーザID         */
	uint16_t                      i_gid;  /**< ファイル所有者のグループID       */
	uint16_t                    i_major;  /**< デバイスのメジャー番号           */
	uint16_t                    i_minor;  /**< デバイスのマイナー番号           */
	uint32_t                     i_size;  /**< ファイルサイズ (単位:バイト)     */
	uint32_t                    i_atime;  /**< 最終アクセス時刻 (単位:UNIX時刻) */
	uint32_t                    i_mtime;  /**< 最終更新時刻 (単位:UNIX時刻)     */
	uint32_t                    i_ctime;  /**< 最終属性更新時刻 (単位:UNIX時刻) */
	union{
		/* データブロック */
		struct _simplefs_data i_data[SIMPLEFS_IDATA_NR];
		/* ディレクトリエントリ */
		struct _simplefs_dent i_dent[SIMPLEFS_SUPER_BLOCK_SIZE/64];
	}i_dblk; /**< データブロック */
}simplefs_inode;

/**
   単純ファイルシステムのボリューム管理情報 (スーパブロック情報)
 */
typedef struct _simplefs_super_block{
	struct _mutex                                      mtx;  /**< 排他用mutex    */
	off_t                                         s_blksiz; /**< ブロック長           */
	uint64_t                                       s_state; /**< スーパブロックの状態 */
	void                                        *s_private; /**< プライベート情報     */
	BITMAP_TYPE(, uint64_t, SIMPLEFS_INODE_NR) s_inode_map; /**< I-nodeマップ         */
	struct _simplefs_inode      s_inode[SIMPLEFS_INODE_NR]; /**< I-node               */
}simplefs_super_block;

/**
   単純なファイルシステムのioctl引数
 */
typedef struct _simplefs_ioctl_arg{
	simplefs_ino             inum;  /**< I-node番号 */
	struct _simplefs_inode  inode;  /**< I-node情報 */
}simplefs_ioctl_arg;

/**
   単純なファイルシステム管理用大域データ
 */
typedef struct _simplefs_table{
	struct _mutex                                            mtx;  /**< 排他用mutex    */
	struct _simplefs_super_block super_blocks[SIMPLEFS_SUPER_NR];  /**< スーパブロック */
}simplefs_table;

/**
   単純ファイルシステム管理用大域データ初期化子
   @param _tablep 単純ファイルシステム管理用大域データへのポインタ
 */
#define __SIMPLEFS_TABLE_INITIALIZER(_tablep)		\
	{						\
	.mtx = __MUTEX_INITIALIZER(&((_tablep)->mtx)),	\
	}

int simplefs_alloc_inode(struct _simplefs_super_block *_fs_super, simplefs_ino *_fs_vnidp);
int simplefs_inode_remove(struct _simplefs_super_block *_fs_super, simplefs_ino _fs_vnid);
int simplefs_refer_inode(struct _simplefs_super_block *_fs_super, simplefs_ino _fs_vnid,
    struct _simplefs_inode **_fs_inodep);

int simplefs_device_inode_init(struct _simplefs_inode *_fs_inode, uint16_t _mode,
    uint16_t _major, uint16_t _minor);
int simplefs_inode_init(struct _simplefs_inode *_fs_inode, uint16_t _mode);

int simplefs_read_mapped_block(struct _simplefs_super_block *_fs_super,
    struct _simplefs_inode *_fs_inode, off_t _position, simplefs_blkno *_blkp);
int simplefs_write_mapped_block(struct _simplefs_super_block *_fs_super,
    struct _simplefs_inode *_fs_inode, off_t _position, simplefs_blkno _new_blk);
int simplefs_unmap_block(struct _simplefs_super_block *_fs_super, simplefs_ino _fs_vnid,
    struct _simplefs_inode *_fs_inode, off_t _off, ssize_t _len);

ssize_t simplefs_inode_read(struct _simplefs_super_block *_fs_super, simplefs_ino _fs_vnid,
    struct _simplefs_inode *_fs_inode, simplefs_file_private _file_priv,
    void *_buf, off_t _pos, ssize_t _len);
ssize_t simplefs_inode_write(struct _simplefs_super_block *_fs_super, simplefs_ino _fs_vnid,
    struct _simplefs_inode *_fs_inode, simplefs_file_private _file_priv,
    const void *_buf, off_t _pos, ssize_t _len);

int simplefs_inode_truncate(struct _simplefs_super_block *_fs_super,
    simplefs_ino _fs_vnid, struct _simplefs_inode *_fs_inode, off_t _len);
int simplefs_alloc_block(struct _simplefs_super_block *_fs_super,
    simplefs_blkno *_block_nump);
void simplefs_free_block(struct _simplefs_super_block *_fs_super, simplefs_inode *_fs_inode,
    simplefs_blkno _block_num);
int simplefs_clear_block(struct _simplefs_super_block *_fs_super,
    struct _simplefs_inode *_fs_inode, simplefs_blkno _block_num, off_t _offset, off_t _size);
int simplefs_read_block(struct _simplefs_super_block *_fs_super,
    struct _simplefs_inode *_fs_inode, void *_buf, simplefs_blkno _block_num,
    off_t _offset, off_t _size);
int simplefs_write_block(struct _simplefs_super_block *_fs_super,
    struct _simplefs_inode *_fs_inode, void *_buf, simplefs_blkno _block_num,
    off_t _offset, off_t _size);

int simplefs_dirent_add(struct _simplefs_super_block *fs_super,
    simplefs_ino _fs_dir_vnid, struct _simplefs_inode *fs_dir_inode,
    simplefs_ino _fs_vnid, const char *_name);
int simplefs_dirent_del(struct _simplefs_super_block *_fs_super,
    simplefs_ino _fs_dir_vnid, struct _simplefs_inode *_fs_dir_inode,
    const char *_name);
int simplefs_dirent_lookup(struct _simplefs_super_block *_fs_super,
    struct _simplefs_inode *_fs_dir_inode, const char *_name, simplefs_ino *_fs_vnidp);

int simplefs_get_super(struct _simplefs_super_block **_fs_superp);

int simplefs_register_filesystem(void);
int simplefs_unregister_filesystem(void);
int simplefs_init(void);
#endif  /*  !ASM_FILE  */
#endif  /*  _FS_SIMPLEFS_SIMPLEFS_H   */

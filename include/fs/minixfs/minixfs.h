/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Minix3 file system definitions                                    */
/*                                                                    */
/**********************************************************************/
#if !defined(FS_MINIXFS_MINIXFS_H)
#define FS_MINIXFS_MINIXFS_H

#if !defined(ASM_FILE)

#include <klib/freestanding.h>
#include <kern/kern-types.h>
#include <klib/klib-consts.h>
#include <klib/compiler.h>

/** MinixV1,V2, V3スーパブロック取得時のブロックサイズ */
#define MINIX_OLD_BLOCK_SIZE  (1024) 
#define MINIX_V1_BLOCK_SIZE   (MINIX_OLD_BLOCK_SIZE)
#define MINIX_V2_BLOCK_SIZE   (MINIX_OLD_BLOCK_SIZE)

#define MINIX_SUPERBLOCK_BLKNO (1)  /**< スーパブロックのブロック番号 (1KiB目) */
#define MINIX_IMAP_BLKNO       (2)  /**< I-nodeブロックのブロック番号          */

/**
   スーパブロックのバージョン
 */
#define MINIX_V1_SUPER_MAGIC   (0x137F)       /* minix V1                     */
#define MINIX_V1_SUPER_MAGIC2  (0x138F)       /* minix V1 30 バイトファイル名 */
#define MINIX_V2_SUPER_MAGIC   (0x2468)       /* minix V2 14 バイトファイル名 */
#define MINIX_V2_SUPER_MAGIC2  (0x2478)       /* minix V2 30 バイトファイル名 */
#define MINIX_V3_SUPER_MAGIC   (0x4d5a)       /* minix V3 60 バイトファイル名 */

#define MINIX_V1_DIRSIZ        (14)  /**< MinixV1でのファイル名長(14バイト) */
#define MINIX_V1_DIRSIZ2       (30)  /**< MinixV1でのファイル名長(30バイト) */
#define MINIX_V2_DIRSIZ        (14)  /**< MinixV2でのファイル名長(14バイト) */
#define MINIX_V2_DIRSIZ2       (30)  /**< MinixV2でのファイル名長(30バイト) */
#define MINIX_V3_DIRSIZ	       (60)  /**< MinixV3でのファイル名長(60バイト) */

#define MINIX_V1_NR_TZONES     (9)  /**< MinixV1でのゾーン配列総数        */
#define MINIX_V1_NR_DZONES     (7)  /**< MinixV1での直接参照ブロック数    */
#define MINIX_V1_NR_IND_ZONES  (1)  /**< MinixV1での間接参照ブロック数    */
#define MINIX_V1_NR_DIND_ZONES (1)  /**< MinixV1での2重間接参照ブロック数 */
/** MinixV1 I-nodeでの最初の間接参照ブロックインデクス */
#define MINIX_V1_INDIRECT_ZONE_IDX      ( MINIX_V1_NR_DZONES )
/** MinixV1 I-nodeでの最初の2重間接参照ブロックインデクス */
#define MINIX_V1_DBL_INDIRECT_ZONE_IDX 	( MINIX_V1_INDIRECT_ZONE_IDX + 1 ) 

#define MINIX_V2_NR_TZONES     (10) /**< MinixV2でのゾーン配列総数        */
#define MINIX_V2_NR_DZONES     (7)  /**< MinixV2での直接参照ブロック数    */
#define MINIX_V2_NR_IND_ZONES  (1)  /**< MinixV2での間接参照ブロック数    */
#define MINIX_V2_NR_DIND_ZONES (1)  /**< MinixV2での2重間接参照ブロック数 */
/** MinixV2 I-nodeでの最初の間接参照ブロックインデクス */
#define MINIX_V2_INDIRECT_ZONE_IDX      ( MINIX_V2_NR_DZONES)       
/** MinixV2 I-nodeでの最初の2重間接参照ブロックインデクス */
#define MINIX_V2_DBL_INDIRECT_ZONE_IDX	( MINIX_V2_INDIRECT_ZONE_IDX + 1 )

/** MinixV3でのゾーン配列総数     */
#define MINIX_V3_NR_TZONES     (MINIX_V2_NR_TZONES)
/** MinixV3での直接参照ブロック数 */
#define MINIX_V3_NR_DZONES     (MINIX_V2_NR_DZONES)
/** MinixV3での間接参照ブロック数 */
#define MINIX_V3_NR_IND_ZONES  (MINIX_V2_NR_IND_ZONES)
/** MinixV3での2重間接参照ブロック数 */
#define MINIX_V3_NR_DIND_ZONES (MINIX_V2_NR_DIND_ZONES)
/** MinixV3 I-nodeでの最初の間接参照ブロックインデクス */
#define MINIX_V3_INDIRECT_ZONE_IDX      ( MINIX_V3_NR_DZONES)       
/** MinixV3 I-nodeでの最初の2重間接参照ブロックインデクス */
#define MINIX_V3_DBL_INDIRECT_ZONE_IDX	( MINIX_V3_INDIRECT_ZONE_IDX + 1 )

/**
   ビットマップ操作識別用マクロ
 */
#define INODE_MAP              (0)      /**< I-node ビットマップ操作 */
#define ZONE_MAP               (1)      /**< ゾーンビットマップ操作  */

/**
   I-node操作の種別
 */
#define MINIX_INODE_READING    (0)      /**< I-node読取り */
#define MINIX_INODE_WRITING    (1)      /**< I-node書込み */

/**
   I-node番号/ゾーン番号型
 */
typedef uint16_t      minixv1_ino;  /**< MinixV1でのI-node番号型 */
typedef minixv1_ino   minixv2_ino;  /**< MinixV2でのI-node番号型 */
typedef uint32_t      minixv3_ino;  /**< MinixV3でのI-node番号型 */
typedef uint32_t        minix_ino;  /**< メモリ中のI-node番号    */
typedef uint16_t     minixv1_zone;  /**< MinixV1でのゾーン番号型 */
typedef uint32_t     minixv2_zone;  /**< MinixV2でのゾーン番号型 */
typedef minixv2_zone minixv3_zone;  /**< MinixV3でのゾーン番号型 */
typedef uint32_t       minix_zone;  /**< メモリ中のゾーン番号    */
/**
   ビットマップチャンク
 */
typedef uint16_t minixv12_bitchunk;  /**< MinixV1, MinixV2 ビットマップチャンク */
typedef uint32_t  minixv3_bitchunk;  /**< MinixV3 ビットマップチャンク          */
typedef uint32_t  minix_bitmap_idx;  /**< メモリ中のビットマップインデクス      */

/**
   MinixV1/V2 のスーパブロック
 */
typedef struct _minixv12_super_block {
	uint16_t       s_ninodes;  /**< 総I-node数(ファイル数 単位:個) */
	uint16_t        s_nzones;  /**< 総ゾーン数(ファイル数 単位:個) */
	uint16_t   s_imap_blocks;  /**< I-nodeビットマップブロックの数
				    * (単位:個, ブロック長: 1KiB)
				    */
	uint16_t   s_zmap_blocks;  /**< ゾーンビットマップブロックの数
				    * (単位:個, ブロック長: 1KiB)
				    */
	uint16_t s_firstdatazone;  /**< 最初のゾーン(データブロック)の開始ブロック位置
				    * (単位:ブロック番号, ブロック長: 1KiB)
				    */
	uint16_t s_log_zone_size;  /**< ゾーン長のブロックオーダ
				    * (単位:シフト値, ブロック長: 1KiB)
				    * バイト単位でのゾーン長: 1024 << s_log_zone_size
				    */
	uint32_t      s_max_size;  /**< 最大ファイルサイズ(単位: バイト) */
	uint16_t         s_magic;  /**< スーパブロックのマジック番号     */
	uint16_t         s_state;  /**< マウント状態                     */
	uint32_t         s_zones;  /**< 総ゾーン数(単位:個, MinixV2用)   */
} __packed minixv12_super_block;

/**
   MinixV3のスーパブロック
 */
typedef struct _minixv3_super_block {
	uint32_t       s_ninodes;  /**< 総I-node数(ファイル数 単位:個) */
	uint16_t          s_pad0;  /**< パディング領域                 */
	uint16_t   s_imap_blocks;  /**< I-nodeビットマップブロックの数
				    * (単位:個, ブロック長: s_blocksize)
				    */
	uint16_t   s_zmap_blocks;  /**< ゾーンビットマップブロックの数
				    * (単位:個, ブロック長: s_blocksize)
				    */
	uint16_t s_firstdatazone;  /**< 最初のゾーン(データブロック)の開始ブロック位置
				    * (単位:個, ブロック長: s_blocksize)
				    */
	uint16_t s_log_zone_size;  /**< ゾーン長のブロックオーダ
				    * (単位:シフト値, ブロック長: 1KiB)
				    * バイト単位でのゾーン長: 1024 << s_log_zone_size
				    */
	uint16_t          s_pad1;  /**< パディング領域                          */
	uint32_t      s_max_size;  /**< 最大ファイルサイズ(単位: バイト)        */
	uint32_t         s_zones;  /**< 総ゾーン数(単位:個)                     */
	uint16_t         s_magic;  /**< スーパブロックのマジック番号            */
	uint16_t          s_pad2;  /**< パディング領域                          */
	uint16_t     s_blocksize;  /**< ファイルシステムブロック長(単位:バイト) */
	uint8_t   s_disk_version;  /**< ファイルシステムレイアウトバージョン    */
} __packed minixv3_super_block;

/**
   メモリ上のMinixスーパブロック
 */
typedef struct _minix_super_block {
	uint16_t         s_magic;  /**< スーパブロックマジック番号                         */
	bool         swap_needed;  /**< ビッグエンディアンでフォーマットされている場合は真 */
	dev_id               dev;  /**< デバイスID                                         */
	uint32_t         s_state;  /**< マウント状態などのフラグ                           */
	union  _d_super{
		minixv12_super_block v12;  /**< MinixV1 V2 スーパブロック */
		minixv3_super_block  v3;   /**< MinixV3 スーパブロック    */
	}d_super;
}minix_super_block;

/**
   MinixV1 ディスク I-node
 */
typedef struct _minixv1_inode {	
	uint16_t                     i_mode;  /** ファイル種別/保護属性        */
	uint16_t                      i_uid;  /** ファイル所有者のユーザID     */
	uint32_t                     i_size;  /** ファイルサイズ (単位:バイト) */
	uint32_t                    i_mtime;  /** 最終更新時刻 (単位:UNIX時刻) */
	uint8_t                       i_gid;  /** ファイル所有者のグループID   */
	uint8_t                    i_nlinks;  /** ファイルのリンク数 (単位:個) */
	uint16_t i_zone[MINIX_V1_NR_TZONES];  /** 順ファイル構成ブロック番号   */
} __packed minixv1_inode;

/**
   MinixV2 ディスク I-node
 */
typedef struct _minixv2_inode {
	uint16_t                     i_mode;  /** ファイル種別/保護属性            */
	uint16_t                   i_nlinks;  /** ファイルのリンク数 (単位:個)     */
	uint16_t                      i_uid;  /** ファイル所有者のユーザID         */
	uint16_t                      i_gid;  /** ファイル所有者のグループID       */
	uint32_t                     i_size;  /** ファイルサイズ (単位:バイト)     */
	uint32_t                    i_atime;  /** 最終アクセス時刻 (単位:UNIX時刻) */
	uint32_t                    i_mtime;  /** 最終更新時刻 (単位:UNIX時刻)     */
	uint32_t                    i_ctime;  /** 最終属性更新時刻 (単位:UNIX時刻) */
	uint32_t i_zone[MINIX_V2_NR_TZONES];  /** 順ファイル構成ブロック番号       */
} __packed minixv2_inode;

/**
   MinixV3 ディスク I-node
 */
typedef struct _minixv2_inode minixv3_inode;

/**
   メモリ上のMinix I-node
 */
typedef struct _minix_inode {
	minix_super_block     *sbp;  /**< Minixスーパブロック情報   */
	union  _d_inode{
		minixv1_inode   v1;  /**< MinixV1 I-node            */
		minixv2_inode  v23;  /**< MinixV2 V3 I-node         */
	}d_inode;
}minix_inode;

/**
   MinixV1 ディレクトリエントリ
 */
typedef struct _minixv1_dentry {
	minixv1_ino inode;  /**< I-node番号                                  */
	char      name[0];  /**< ファイル名 (NULLターミネートなし, 14バイト) */
} __packed minixv1_dentry ;

/**
   MinixV2 ディレクトリエントリ
   @note nameメンバは14または30バイト (ファイルフォーマットの版数に依存)
 */
typedef minixv1_dentry minixv2_dentry;

/**
   MinixV3 ディレクトリエントリ
 */
typedef struct _minix3_dentry {
	minixv3_ino inode;  /**< I-node番号                                  */
	char      name[0];  /**< ファイル名 (NULLターミネートなし, 60バイト) */
} __packed minixv3_dentry;

/**
   MinixV1のスーパブロックを格納していることを確認
   @param[in] _sbp メモリ中のスーパブロック情報
 */
#define MINIX_SB_IS_V1(_sbp) \
	( ( ((_sbp)->s_magic) == MINIX_V1_SUPER_MAGIC ) ||	\
	    ( ((_sbp)->s_magic) == MINIX_V1_SUPER_MAGIC2 ) )
/**
   MinixV2のスーパブロックを格納していることを確認
   @param[in] _sbp メモリ中のスーパブロック情報
 */
#define MINIX_SB_IS_V2(_sbp) \
	( ( ((_sbp)->s_magic) == MINIX_V2_SUPER_MAGIC ) ||	\
	  ( ((_sbp)->s_magic) == MINIX_V2_SUPER_MAGIC2 ) )

/**
   MinixV3のスーパブロックを格納していることを確認
   @param[in] _sbp メモリ中のスーパブロック情報
 */
#define MINIX_SB_IS_V3(_sbp) \
	( ((_sbp)->s_magic) == MINIX_V3_SUPER_MAGIC )

/**
   メモリ中のMinixディスクスーパブロックからディスクスーパブロック情報メンバを得る
   @param[in] _sbp メモリ中のスーパブロック情報へのポインタ
   @param[in] _m   メンバ名
 */
#define MINIX_D_SUPER_BLOCK(_sbp, _m)					\
	( MINIX_SB_IS_V3((_sbp)) ? (((minixv3_super_block *)(&((_sbp)->d_super)))->_m) : \
	    (((minixv12_super_block *)(&((_sbp)->d_super)))->_m) )
/**
   総ゾーン数を算出する
   @param[in] _sbp メモリ中のスーパブロック情報
 */
#define MINIX_NR_TZONES(_sbp) \
	( MINIX_SB_IS_V3((_sbp)) ? (MINIX_V3_NR_TZONES) : \
	  ( MINIX_SB_IS_V2((_sbp)) ? (MINIX_V2_NR_TZONES) : (MINIX_V1_NR_TZONES) ) )

/**
   直接参照ゾーン数を算出する
   @param[in] _sbp メモリ中のスーパブロック情報
 */
#define MINIX_NR_DZONES(_sbp) \
	( MINIX_SB_IS_V3((_sbp)) ? (MINIX_V3_NR_DZONES) : \
	  ( MINIX_SB_IS_V2((_sbp)) ? (MINIX_V2_NR_DZONES) : (MINIX_V1_NR_DZONES) ) )

/**
   間接参照参照ゾーン数を算出する
   @param[in] _sbp メモリ中のスーパブロック情報
 */      
#define MINIX_NR_IND_ZONES(_sbp) \
	( MINIX_SB_IS_V3((_sbp)) ? (MINIX_V3_NR_IND_ZONES) : \
	  ( MINIX_SB_IS_V2((_sbp)) ? (MINIX_V2_NR_IND_ZONES) : (MINIX_V1_NR_IND_ZONES) ) )

/**
   2重間接参照参照ゾーン数を算出する
   @param[in] _sbp メモリ中のスーパブロック情報
 */      
#define MINIX_NR_DIND_ZONES(_sbp) \
	( MINIX_SB_IS_V3((_sbp)) ? (MINIX_V3_NR_DIND_ZONES) : \
	  ( MINIX_SB_IS_V2((_sbp)) ? (MINIX_V2_NR_DIND_ZONES) : (MINIX_V1_NR_DIND_ZONES) ) )

/**
   ファイルシステムブロックサイズを得る (単位:バイト)
   @param[in] _sbp メモリ中のスーパブロック情報
 */      
#define MINIX_BLOCK_SIZE(_sbp) \
	( MINIX_SB_IS_V3((_sbp)) ? ((_sbp)->d_super.v3.s_blocksize) :	\
	    ( MINIX_SB_IS_V2((_sbp)) ? (MINIX_V2_BLOCK_SIZE) : (MINIX_V1_BLOCK_SIZE) ) )

/**
   ゾーン番号のバイト長を得る (単位:バイト)
   @param[in] _sbp メモリ中のスーパブロック情報
 */      
#define MINIX_ZONE_NUM_SIZE(_sbp) \
	( MINIX_SB_IS_V3((_sbp)) ? (sizeof(minixv3_zone)) :	\
	    ( MINIX_SB_IS_V2((_sbp)) ? (sizeof(minixv2_zone)) : (sizeof(minixv1_zone)) ) )

/**
   ディスクI-nodeのサイズを得る (単位:バイト)
   @param[in] _sbp メモリ中のスーパブロック情報
 */      
#define MINIX_DINODE_SIZE(_sbp) \
	( MINIX_SB_IS_V3((_sbp)) ? (sizeof(minixv3_inode)) :	\
	    ( MINIX_SB_IS_V2((_sbp)) ? (sizeof(minixv2_inode)) : (sizeof(minixv1_inode)) ) )

/**
   ビットマップチャンクのサイズを得る (単位:バイト)
   @param[in] _sbp メモリ中のスーパブロック情報
   @note 使用しない?
 */      
#define MINIX_BMAPCHUNK_SIZE(_sbp)				    \
	( MINIX_SB_IS_V3((_sbp)) ? (sizeof(minixv3_bitchunk)) :	    \
	  ( MINIX_SB_IS_V2((_sbp)) ? (sizeof(minixv12_bitchunk)) :  \
	    (sizeof(minixv12_bitchunk)) ) )

/**
   ゾーンのサイズを得る
   @param[in] _sbp メモリ中のスーパブロック情報
 */      
#define MINIX_ZONE_SIZE(_sbp) \
	(MINIX_BLOCK_SIZE(_sbp) << MINIX_D_SUPER_BLOCK(_sbp, s_log_zone_size))

/**
   間接参照ゾーンから参照されるゾーンの数を得る
   @param[in] _sbp メモリ中のスーパブロック情報
 */      
#define MINIX_INDIRECTS(_sbp)	\
	( MINIX_ZONE_SIZE((_sbp)) / MINIX_ZONE_NUM_SIZE((_sbp)) )

/**
   間接参照ゾーンの最初のインデクスを得る
   @param[in] _sbp メモリ中のスーパブロック情報
 */      
#define MINIX_INDIRECT_ZONE_IDX(_sbp)				      \
	( MINIX_SB_IS_V3((_sbp)) ? (MINIX_V3_INDIRECT_ZONE_IDX) :     \
	    ( MINIX_SB_IS_V2((_sbp)) ? (MINIX_V2_INDIRECT_ZONE_IDX) : \
		(MINIX_V1_INDIRECT_ZONE_IDX) ) )

/**
   2重間接参照ゾーンの最初のインデクスを得る
   @param[in] _sbp メモリ中のスーパブロック情報
 */      
#define MINIX_DBL_INDIRECT_ZONE_IDX(_sbp)				      \
	( MINIX_SB_IS_V3((_sbp)) ? (MINIX_V3_DBL_INDIRECT_ZONE_IDX) :     \
	    ( MINIX_SB_IS_V2((_sbp)) ? (MINIX_V2_DBL_INDIRECT_ZONE_IDX) : \
		(MINIX_V1_DBL_INDIRECT_ZONE_IDX) ) )
	
/**
   ゾーン番号型のサイズを得る(単位:バイト)
   @param[in] _sbp メモリ中のスーパブロック情報
 */      
#define MINIX_ZONE_NUM_SIZE(_sbp) \
	( MINIX_SB_IS_V3((_sbp)) ? (sizeof(minixv3_zone)) :	\
	    ( MINIX_SB_IS_V2((_sbp)) ? (sizeof(minixv2_zone)) : (sizeof(minixv1_zone)) ) )

/**
   スーパブロック中の総ゾーン数を得る
   @param[in] _sbp メモリ中のスーパブロック情報
 */
#define MINIX_SB_ZONES_NR(_sbp)     \
	( ( MINIX_SB_IS_V3((_sbp)) || MINIX_SB_IS_V2((_sbp)) )	   \
	    ? MINIX_D_SUPER_BLOCK((_sbp),s_zones) :		   \
	    ((_sbp)->d_super.v12.s_nzones) )

/**
   無効ゾーン番号を得る
   @param[in] _sbp メモリ中のスーパブロック情報
 */
#define MINIX_NO_ZONE(_sbp)				\
	( MINIX_SB_IS_V3((_sbp) ) ? ((minixv3_zone)0) :	\
	    ( MINIX_SB_IS_V2((_sbp)) ? ((minixv2_zone)0) : ((minixv1_zone)0) ) )
	
/**
   I-nodeテーブルブロック中に含まれるI-node情報の数を算出する
   @param[in] _sbp メモリ中のスーパブロック情報
 */      
#define MINIX_INODES_PER_BLOCK(_sbp) \
	( MINIX_BLOCK_SIZE((_sbp) ) / MINIX_DINODE_SIZE(_sbp))

/**
   メモリ中のMinixディスクI-nodeからI-nodeのメンバ変数の値を得る
   @param[in] _inodep メモリ中のI-node情報へのポインタ
   @param[in] _m   メンバ名
 */
#define MINIX_D_INODE(_inodep, _m)					\
	( MINIX_SB_IS_V1((_inodep)->sbp )				\
	    ? (((minixv1_inode *)(&((_inodep)->d_inode)))->_m)		\
	    : (((minixv2_inode *)(&((_inodep)->d_inode)))->_m) )

/**
   メモリ中のMinixディスクI-nodeからI-nodeのメンバ変数の値を更新する
   @param[in] _inodep メモリ中のI-node情報へのポインタ
   @param[in] _m   メンバ名
   @param[in] _v   設定値
 */
#define MINIX_D_INODE_SET(_inodep, _m, _v) do{				         \
		if ( MINIX_SB_IS_V1((_inodep)->sbp) )			         \
			(((minixv1_inode *)(&((_inodep)->d_inode)))->_m) = (_v); \
		else							         \
			(((minixv2_inode *)(&((_inodep)->d_inode)))->_m) = (_v); \
	}while(0)

/**
   メモリ中のMinixディスクI-nodeから最終アクセス時刻を得る
   @param[in] _inodep メモリ中のI-node情報へのポインタ
 */
#define MINIX_D_INODE_ATIME(_inodep)					\
	( MINIX_SB_IS_V1((_inodep)->sbp )				\
	    ? (((minixv1_inode *)(&((_inodep)->d_inode)))->i_mtime)	\
	    : (((minixv2_inode *)(&((_inodep)->d_inode)))->i_atime) )

/**
   メモリ中のMinixディスクI-nodeの最終アクセス時刻を更新する
   @param[in] _inodep メモリ中のI-node情報へのポインタ
   @param[in] _v   設定値
 */
#define MINIX_D_INODE_ATIME_SET(_inodep, _v) do{			  \
	if ( !MINIX_SB_IS_V1((_inodep)->sbp) )			          \
		(minixv2_inode *)(&((_inodep)->d_inode))->i_atime = (_v); \
	}while(0)

/**
   メモリ中のMinixディスクI-nodeから属性更新時刻を得る
   @param[in] _inodep メモリ中のI-node情報へのポインタ
 */
#define MINIX_D_INODE_CTIME(_inodep)					\
	( MINIX_SB_IS_V1((_inodep)->sbp )				\
	    ? (((minixv1_inode *)(&((_inodep)->d_inode)))->i_mtime)	\
	    : (((minixv2_inode *)(&((_inodep)->d_inode)))->i_ctime) )

/**
   メモリ中のMinixディスクI-nodeの最終属性更新時刻を更新する
   @param[in] _inodep メモリ中のI-node情報へのポインタ
   @param[in] _v   設定値
 */
#define MINIX_D_INODE_CTIME_SET(_inodep, _v) do{			  \
	if ( !MINIX_SB_IS_V1((_inodep)->sbp) )			          \
		(minixv2_inode *)(&((_inodep)->d_inode))->i_atime = (_v); \
	}while(0)
#endif  /*  !ASM_FILE  */
#endif  /*  FS_MINIXFS_MINIXFS_H   */


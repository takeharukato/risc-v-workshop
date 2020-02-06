/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  MinixV3 file system definitions                                   */
/*                                                                    */
/**********************************************************************/
#if !defined(MINIXFS_H)
#define MINIXFS_H

#include <klib/klib-consts.h>

#define MINIX_SUPERBLOCK_BLKNO (1)            /**< スーパブロックのブロック番号 */
#define MINIX_IMAP_BLKNO       (2)            /**< I-nodeビットマップのブロック番号 */

#define MINIX_V3_SUPER_MAGIC   (0x4d5a)       /* minix V3 60 byte file name  */

#define MINIX_V2_DIRSIZ        (14)
#define MINIX_V2_DIRSIZ2       (30)
#define MINIX_V3_DIRSIZ	       (60)

#define MINIX_V2_NR_DZONES     (7)	/* # direct zone numbers in a V2 inode */
#define MINIX_V2_NR_TZONES     (10)	/* total # zone numbers in a V2 inode */
#define MINIX_V2_NR_IND_ZONES  (1)
#define MINIX_V2_NR_DIND_ZONES (1)

#define MINIX_V2_INDIRECT_ZONE_IDX \
	( MINIX_V2_NR_DZONES)       /* First indirect zone in V2 inode */
#define MINIX_V2_DBL_INDIRECT_ZONE_IDX					\
	( MINIX_V2_INDIRECT_ZONE_IDX + 1)  /* First double indirect zone in V2 inode */

#define MINIX_NR_DZONES        (MINIX_V2_NR_DZONES)
#define MINIX_NR_TZONES        (MINIX_V2_NR_TZONES)
#define MINIX_NR_IND_ZONES     (MINIX_V2_NR_IND_ZONES)
#define MINIX_NR_DIND_ZONES    (MINIX_V2_NR_DIND_ZONES)

#define INODE_MAP              (0)      /* operating on the inode bit map */
#define ZONE_MAP               (1)      /* operating on the zone bit map */

#if !defined(ASM_FILE)

#include <klib/freestanding.h>
#include <klib/compiler.h>

typedef uint32_t minixv3_ino;
typedef uint32_t minixv23_zone;

typedef struct _minixv3_super_block {
	uint32_t s_ninodes;
	uint16_t s_pad0;
	uint16_t s_imap_blocks;
	uint16_t s_zmap_blocks;
	uint16_t s_firstdatazone;
	uint16_t s_log_zone_size;
	uint16_t s_pad1;
	uint32_t s_max_size;
	uint32_t s_zones;
	uint16_t s_magic;
	uint16_t s_pad2;
	uint16_t s_blocksize;
	uint8_t  s_disk_version;
} __packed minixv3_super_block;

typedef struct _minixv23_inode {
	uint16_t i_mode;
	uint16_t i_nlinks;
	uint16_t i_uid;
	uint16_t i_gid;
	uint32_t i_size;
	uint32_t i_atime;
	uint32_t i_mtime;
	uint32_t i_ctime;
	uint32_t i_zone[MINIX_V2_NR_TZONES];
} __packed minixv23_inode;

typedef struct _minix3_dentry {
	minixv3_ino inode;
	char     name[0];
} __packed minixv3_dentry;

typedef uint32_t             minixv3_bitchunk;
typedef minixv3_super_block minix_super_block;
typedef minixv3_dentry           minix_dentry;
typedef minixv3_ino                 minix_ino;
typedef minixv23_inode            minix_inode;
typedef minixv3_bitchunk     minix_bitchunk_t;
typedef minixv23_zone              minix_zone;

#define MINIX_V2_ZONE_NUM_SIZE (sizeof(minixv2_zone))  /* # bytes in V2 zone  */
#define MINIX_V2_INDIRECTS     \
	(MINIX_BLOCK_SIZE/MINIX_V2_ZONE_NUM_SIZE)  /* # zones/indir block in V2 */
#define MINIX_INDIRECTS         MINIX_V2_INDIRECTS
#define MINIX_INDIRECT_ZONE_IDX     (MINIX_V2_INDIRECT_ZONE_IDX)
#define MINIX_DBL_INDIRECT_ZONE_IDX (MINIX_V2_DBL_INDIRECT_ZONE_IDX)

#define MINIX_BLOCK_SIZE(sbp) (((minix_super_block *)(sbp))->s_blocksize)
#define MINIX_ZONE_SIZE(sbp)   \
	(MINIX_BLOCK_SIZE(sbp)  << (((minix_super_block *)(sbp))->s_log_zone_size))
#define MINIX_INODES_PER_BLOCK(sbp) ( MINIX_BLOCK_SIZE(sbp) / sizeof(minix_inode))
#define MINIX_BMAP_PER_BLOCK(sbp)   (MINIX_BLOCK_SIZE(sbp)*BITS_PER_BYTE))

#define MINIX_NO_ZONE          ((minix_zone) 0)	/* absence of a zone number */

#endif  /*  !ASM_FILE  */
#endif  /*  MINIXFS_H  */

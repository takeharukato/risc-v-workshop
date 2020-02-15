/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  test routine                                                      */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>
#include <fs/minixfs/minixfs.h>
#include <kern/fs-fsimg.h>

#include <kern/ktest.h>

static ktest_stats tstat_minixfs=KTEST_INITIALIZER;

int minix_calc_indexes(minix_super_block *_sbp, off_t _position, int *_typep, int *_zidxp,
    int *_idx1stp, int *_idx2ndp);

#define ROOT_INO  (1)
#define TST_INO   (2)
#define TST_BIG   (3)
#define TST_BUFSIZ (1024)
#define TST_WRPTN  (0x5a)
#define TST_INIPTN (0xde)

static char rwbuf[TST_BUFSIZ];
static char  vbuf[TST_BUFSIZ];

static int
write_test(minix_ino ino, minix_inode *dip, off_t off, size_t size, size_t *wrp){
	int       rc;
	int      vrc;
	size_t rdlen;
	size_t wrlen;

	memset(&vbuf[0], TST_INIPTN, TST_BUFSIZ);
	memset(&rwbuf[0], TST_WRPTN, TST_BUFSIZ);

	rc = minix_rw_zone(ino, dip, &rwbuf[0], off, size, MINIX_RW_WRITING, &wrlen);

	if ( rc == 0 ) {

		vrc = minix_rw_zone(ino, dip, &vbuf[0], off, 
				    MIN(wrlen, TST_BUFSIZ), MINIX_RW_READING, &rdlen);
		if ( vrc != 0 )
			return vrc;

		if ( memcmp(&rwbuf[0], &vbuf[0], MIN(wrlen, TST_BUFSIZ)) != 0 ) 
			return -ENXIO;
	}

	if ( wrp != NULL )
		*wrp = wrlen;

	return rc;
}


static void
minixfs1(struct _ktest_stats *sp, void __unused *arg){
	int               rc;
	minix_super_block sb;
	minix_bitmap_idx idx;
	minix_inode     din1;
	minix_inode     din2;
	minix_inode     din3;
	minix_ino        ino;
	int             type;
	int             didx;
	int             idx1;
	int             idx2;
	size_t           len;
	size_t         rdlen;
	size_t         wrlen;
	size_t         dtlen;
	minix_dentry     ent;
	off_t            pos;
	size_t    min_direct;
	size_t    max_direct;
	size_t       min_ind;
	size_t       max_ind;
	size_t      min_dind;
	size_t      max_dind;
	off_t        old_siz;

	rc = minix_read_super(FS_FSIMG_DEVID, &sb);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	rc = minix_write_super(&sb);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	rc = minix_bitmap_alloc(&sb, INODE_MAP, &idx);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	kprintf("Allocated I-node: %d\n", idx);
	rc = minix_bitmap_free(&sb, INODE_MAP, idx);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	rc = minix_bitmap_alloc(&sb, ZONE_MAP, &idx);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	kprintf("Allocated Zone: %d\n", idx);
	rc = minix_bitmap_free(&sb, ZONE_MAP, idx);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );


	rc = minix_rw_disk_inode(&sb, ROOT_INO, MINIX_RW_READING, &din1);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	ino = TST_INO;
	rc = minix_rw_disk_inode(&sb, ino, MINIX_RW_READING, &din2);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	kprintf("inode[%d](i_mode, i_uid, i_size, i_gid, i_nlinks, "
	    "i_atime, i_mtime, i_mtime)\n"
	    "=(0%lo, %d, %ld, %d, %d, %ld, %ld, %ld)\n", ino,
	    MINIX_D_INODE(&din2, i_mode),
	    MINIX_D_INODE(&din2, i_uid),
	    MINIX_D_INODE(&din2, i_size),
	    MINIX_D_INODE(&din2, i_gid),
	    MINIX_D_INODE(&din2, i_nlinks),
	    MINIX_D_INODE_ATIME(&din2),
	    MINIX_D_INODE(&din2, i_mtime),
	    MINIX_D_INODE_CTIME(&din2));
	MINIX_D_INODE_SET(&din2, i_uid, 0);
	rc = minix_rw_disk_inode(&sb, ino, MINIX_RW_WRITING, &din2);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	rc = minix_rw_disk_inode(&sb, ino, MINIX_RW_READING, &din2);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	if ( MINIX_D_INODE(&din2, i_uid) == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/*
	 * Zoneインデクス計算
	 */
	min_direct = 0 * MINIX_ZONE_SIZE(&sb);
	max_direct = MINIX_NR_DZONES(&sb) * MINIX_ZONE_SIZE(&sb) - 1;
	min_ind = max_direct + 1;
	max_ind = ( MINIX_NR_DZONES(&sb) +
		( MINIX_NR_IND_ZONES(&sb) * MINIX_INDIRECTS(&sb) ) )
	    * MINIX_ZONE_SIZE(&sb) - 1;
	min_dind = max_ind + 1;
	max_dind = ( MINIX_NR_DZONES(&sb) +
		( MINIX_NR_IND_ZONES(&sb) * MINIX_INDIRECTS(&sb) ) + 
		( MINIX_NR_DIND_ZONES(&sb) * MINIX_INDIRECTS(&sb) * MINIX_INDIRECTS(&sb) ) )
	    * MINIX_ZONE_SIZE(&sb) - 1;

	/* 直接参照ゾーン最小値 */
	rc = minix_calc_indexes(&sb, min_direct, &type, &didx, &idx1, &idx2);
	if ( ( rc == 0 ) && ( type == MINIX_ZONE_ADDR_DIRECT ) )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	/* 直接参照ゾーン最大値 */
	rc = minix_calc_indexes(&sb, max_direct, &type, &didx, &idx1, &idx2);
	if ( ( rc == 0 ) && ( type == MINIX_ZONE_ADDR_DIRECT ) )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	/* 単間接参照ゾーン最小値 */
	rc = minix_calc_indexes(&sb, min_ind, &type, &didx, &idx1, &idx2);
	if ( ( rc == 0 ) && ( type == MINIX_ZONE_ADDR_SINGLE ) )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	/* 単間接参照ゾーン最大値 */
	rc = minix_calc_indexes(&sb,max_ind, &type, &didx, &idx1, &idx2);
	if ( ( rc == 0 ) && ( type == MINIX_ZONE_ADDR_SINGLE ) )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	/* 2重間接参照ゾーン最小値 */
	rc = minix_calc_indexes(&sb, min_dind, &type, &didx, &idx1, &idx2);
	if ( ( rc == 0 ) && ( type == MINIX_ZONE_ADDR_DOUBLE ) )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	/* 2重間接参照ゾーン最大値 */
	rc = minix_calc_indexes(&sb, max_dind, &type, &didx, &idx1, &idx2);
	if ( ( rc == 0 ) && ( type == MINIX_ZONE_ADDR_DOUBLE ) )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/* レンジ外 */
	rc = minix_calc_indexes(&sb, max_dind + 1, &type, &didx, &idx1, &idx2);
	if ( rc == -E2BIG )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	/*
	 * ディレクトリエントリ読み出し
	 */
	len = MINIX_D_INODE(&din1, i_size);
	pos = 0;
	while( len > 0 ) {

		rc = minix_rw_zone(1, &din1, &ent, pos, MINIX_D_DENT_SIZE(&sb), 
		    MINIX_RW_READING, &rdlen);
		if ( rc != 0 )
			break;
		len -= MINIX_D_DENT_SIZE(&sb);
		pos += MINIX_D_DENT_SIZE(&sb);
	}

	memset(&rwbuf[0], 0, TST_BUFSIZ);
	rc = minix_rw_zone(ino, &din2, &rwbuf[0], 0, TST_BUFSIZ, MINIX_RW_READING, &rdlen);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	rwbuf[MIN(MINIX_D_INODE(&din2, i_size), TST_BUFSIZ - 1)]='\0';
	kprintf("Read:%s", rwbuf);

	memset(&rwbuf[0], 0, TST_BUFSIZ);
	dtlen = strlen("Hello RISC-V\n");
	strcpy(&rwbuf[0], "Hello RISC-V\n");
	rwbuf[dtlen]='\0';
	rc = minix_rw_zone(ino, &din2, &rwbuf[0], 0, dtlen + 1,MINIX_RW_WRITING, &wrlen);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	memset(&rwbuf[0], 0, TST_BUFSIZ);
	rc = minix_rw_zone(ino, &din2, &rwbuf[0], 0, TST_BUFSIZ, MINIX_RW_READING, &rdlen);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	rwbuf[MIN(MINIX_D_INODE(&din2, i_size), TST_BUFSIZ - 1)]='\0';
	kprintf("Read:%s", rwbuf);

	/*
	 * ゾーンの開放
	 */
	rc = minix_unmap_zone(ino, &din2, MINIX_D_INODE(&din2, i_size)/2,
	    MINIX_D_INODE(&din2, i_size));
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	rc = minix_unmap_zone(ino, &din2, 0, MINIX_D_INODE(&din2, i_size));
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	
	/* 2重間接ブロック
	 */
	rc = minix_rw_disk_inode(&sb, TST_BIG, MINIX_RW_READING, &din3);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	/*
	 * ゾーンの開放
	 */
	rc = minix_unmap_zone(ino, &din3, 0, MINIX_D_INODE(&din3, i_size));
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	if ( MINIX_D_INODE(&din3, i_size) == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	/*
	 * ゾーン書き込み
	 */
	rc = minix_unmap_zone(TST_INO, &din3, 0, MINIX_D_INODE(&din3, i_size));
	kassert( rc == 0 );
	rc = write_test( TST_INO, &din3, 0, 
			 min_direct+1,
			 &wrlen);
	if  ( ( rc == 0 ) && ( wrlen == MINIX_D_INODE(&din3, i_size) ) )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	rc = minix_unmap_zone(TST_INO, &din3, 0, MINIX_D_INODE(&din3, i_size));
	kassert( rc == 0 );
	rc = write_test( TST_INO, &din3, 0, max_direct+1, &wrlen);
	if ( ( rc == 0 ) && ( wrlen == MINIX_D_INODE(&din3, i_size) ) )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/* 単間接参照ブロック */
	rc = minix_unmap_zone(TST_INO, &din3, 0, MINIX_D_INODE(&din3, i_size));
	kassert( rc == 0 );
	rc = write_test( TST_INO, &din3, 0, min_ind+1, &wrlen);
	if ( ( rc == 0 ) && ( wrlen == MINIX_D_INODE(&din3, i_size) ) )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	rc = minix_unmap_zone(TST_INO, &din3, 0, MINIX_D_INODE(&din3, i_size));
	kassert( rc == 0 );
	rc = write_test( TST_INO, &din3, 0, max_ind+1, &wrlen);
	if ( ( rc == 0 ) && ( wrlen == MINIX_D_INODE(&din3, i_size) ) )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/* 2重間接参照ブロック */
	rc = minix_unmap_zone(TST_INO, &din3, 0, MINIX_D_INODE(&din3, i_size));
	kassert( rc == 0 );
	rc = write_test( TST_INO, &din3, 0, min_dind+1, &wrlen);
	if ( ( rc == 0 ) && ( wrlen == MINIX_D_INODE(&din3, i_size) ) )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/*
	 * サイズ縮小
	 */
	/* データ作成 */
	rc = minix_unmap_zone(TST_INO, &din3, 0, MINIX_D_INODE(&din3, i_size));
	kassert( rc == 0 );
	rc = write_test( TST_INO, &din3, 0, min_ind+1, &wrlen);
	if ( ( rc == 0 ) && ( wrlen == MINIX_D_INODE(&din3, i_size) ) )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	/* 開始: ゾーン境界, ファイル先頭 
	   終了: ゾーン境界, ファイル末尾残
	 */
	old_siz = MINIX_D_INODE(&din3, i_size);
	rc = minix_unmap_zone(TST_INO, &din3, 0, 
			      truncate_align(MINIX_D_INODE(&din3, i_size),
					     MINIX_ZONE_SIZE(&sb))
			      - MINIX_ZONE_SIZE(&sb));
	if ( ( rc == 0 ) && ( old_siz == MINIX_D_INODE(&din3, i_size) ) )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/* データ作成 */
	rc = minix_unmap_zone(TST_INO, &din3, 0, MINIX_D_INODE(&din3, i_size));
	kassert( rc == 0 );
	rc = write_test( TST_INO, &din3, 0, min_ind + 1, &wrlen);
	if ( ( rc == 0 ) && ( wrlen == MINIX_D_INODE(&din3, i_size) ) )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/* 開始: ゾーン境界, ファイル先頭 
	   終了: ゾーン非境界, ファイル末尾残
	 */
	old_siz = MINIX_D_INODE(&din3, i_size);
	rc = minix_unmap_zone(TST_INO, &din3, 0, 
	    truncate_align(MINIX_D_INODE(&din3, i_size), MINIX_ZONE_SIZE(&sb))
			      - MINIX_ZONE_SIZE(&sb)/2);
	if ( ( rc == 0 ) && ( old_siz == MINIX_D_INODE(&din3, i_size) ) )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	
	/* データ作成 */
	rc = minix_unmap_zone(TST_INO, &din3, 0, MINIX_D_INODE(&din3, i_size));
	kassert( rc == 0 );
	rc = write_test( TST_INO, &din3, 0, 
	    roundup_align(min_ind + 1, MINIX_ZONE_SIZE(&sb))
	    - MINIX_ZONE_SIZE(&sb) / 2, &wrlen);
	if ( ( rc == 0 ) && ( wrlen == MINIX_D_INODE(&din3, i_size) ) )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	/* 開始: ゾーン境界, ファイル先頭 
	   終了: ゾーン非境界, ファイル末尾まで消去
	 */
	old_siz = MINIX_D_INODE(&din3, i_size);
	rc = minix_unmap_zone(TST_INO, &din3, 0, MINIX_D_INODE(&din3, i_size) );
	if ( ( rc == 0 ) && ( MINIX_D_INODE(&din3, i_size) == 0 ) )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/* データ作成 */
	rc = minix_unmap_zone(TST_INO, &din3, 0, MINIX_D_INODE(&din3, i_size));
	kassert( rc == 0 );
	rc = write_test( TST_INO, &din3, 0, min_ind+1, &wrlen);
	if ( ( rc == 0 ) && ( wrlen == MINIX_D_INODE(&din3, i_size) ) )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	/* 開始: ゾーン非境界, ファイル先頭から1/2ゾーン
	   終了: ゾーン境界, ファイル末尾残
	 */
	old_siz = MINIX_D_INODE(&din3, i_size);
	rc = minix_unmap_zone(TST_INO, &din3, MINIX_ZONE_SIZE(&sb)/2, 
			      truncate_align(MINIX_D_INODE(&din3, i_size),
					     MINIX_ZONE_SIZE(&sb))
			      - MINIX_ZONE_SIZE(&sb));
	if ( ( rc == 0 ) && ( old_siz == MINIX_D_INODE(&din3, i_size) ) )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/* データ作成 */
	rc = minix_unmap_zone(TST_INO, &din3, 0, MINIX_D_INODE(&din3, i_size));
	kassert( rc == 0 );
	rc = write_test( TST_INO, &din3, 0, min_ind + 1, &wrlen);
	if ( ( rc == 0 ) && ( wrlen == MINIX_D_INODE(&din3, i_size) ) )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/* 開始: ゾーン非境界, ファイル先頭から1/2ゾーン
	   終了: ゾーン非境界, ファイル末尾残
	 */
	old_siz = MINIX_D_INODE(&din3, i_size);
	rc = minix_unmap_zone(TST_INO, &din3, MINIX_ZONE_SIZE(&sb)/2,
	    truncate_align(MINIX_D_INODE(&din3, i_size), MINIX_ZONE_SIZE(&sb))
			      - MINIX_ZONE_SIZE(&sb)/2);
	if ( ( rc == 0 ) && ( old_siz == MINIX_D_INODE(&din3, i_size) ) )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	
	/* データ作成 */
	rc = minix_unmap_zone(TST_INO, &din3, 0, MINIX_D_INODE(&din3, i_size));
	kassert( rc == 0 );
	rc = write_test( TST_INO, &din3, 0, 
	    roundup_align(min_ind + 1, MINIX_ZONE_SIZE(&sb))
	    - MINIX_ZONE_SIZE(&sb) / 2, &wrlen);
	if ( ( rc == 0 ) && ( wrlen == MINIX_D_INODE(&din3, i_size) ) )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	/* 開始: ゾーン非境界, ファイル先頭から1/2ゾーン
	   終了: ゾーン非境界, ファイル末尾まで消去
	 */
	old_siz = MINIX_D_INODE(&din3, i_size);
	rc = minix_unmap_zone(TST_INO, &din3, MINIX_ZONE_SIZE(&sb)/2,
	    MINIX_D_INODE(&din3, i_size) );
	if ( ( rc == 0 ) && ( MINIX_D_INODE(&din3, i_size) == MINIX_ZONE_SIZE(&sb)/2 ) )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/*
	 * 2ゾーンからなるファイルの場合(中間のゾーン解放を行わないケース)
	 */
	/* データ作成 */
	rc = minix_unmap_zone(TST_INO, &din3, 0, MINIX_D_INODE(&din3, i_size));
	kassert( rc == 0 );
	rc = write_test( TST_INO, &din3, 0, MINIX_ZONE_SIZE(&sb)*2, &wrlen);
	if ( ( rc == 0 ) && ( wrlen == MINIX_D_INODE(&din3, i_size) ) )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	/* 開始: ゾーン非境界, ファイル先頭から1/2ゾーン
	   終了: ゾーン非境界, ファイル末尾-1/2ゾーン
	 */
	old_siz = MINIX_D_INODE(&din3, i_size);
	rc = minix_unmap_zone(TST_INO, &din3, MINIX_ZONE_SIZE(&sb)/2,
	    MINIX_ZONE_SIZE(&sb) );
	if ( ( rc == 0 ) && ( MINIX_D_INODE(&din3, i_size) == old_siz ) )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/* データ作成 */
	rc = minix_unmap_zone(TST_INO, &din3, 0, MINIX_D_INODE(&din3, i_size));
	kassert( rc == 0 );
	rc = write_test( TST_INO, &din3, 0, MINIX_ZONE_SIZE(&sb)*2, &wrlen);
	if ( ( rc == 0 ) && ( wrlen == MINIX_D_INODE(&din3, i_size) ) )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	/* 開始: ゾーン非境界, ファイル先頭から1/2ゾーン
	   終了: ゾーン境界, ファイル末尾まで削除
	 */
	old_siz = MINIX_D_INODE(&din3, i_size);
	rc = minix_unmap_zone(TST_INO, &din3, MINIX_ZONE_SIZE(&sb)/2,
	    MINIX_D_INODE(&din3, i_size) - MINIX_ZONE_SIZE(&sb)/2 );
	if ( ( rc == 0 ) && ( MINIX_D_INODE(&din3, i_size) == MINIX_ZONE_SIZE(&sb)/2 ) )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/*
	 * 1ゾーンからなるファイルの場合(中間のゾーン解放を行わないケース)
	 */
	/* データ作成 */
	rc = minix_unmap_zone(TST_INO, &din3, 0, MINIX_D_INODE(&din3, i_size));
	kassert( rc == 0 );
	rc = write_test( TST_INO, &din3, 0, MINIX_ZONE_SIZE(&sb), &wrlen);
	if ( ( rc == 0 ) && ( wrlen == MINIX_D_INODE(&din3, i_size) ) )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	/* 開始: ゾーン非境界, ファイル先頭から1/2ゾーン
	   終了: ゾーン境界, ファイル末尾
	 */
	old_siz = MINIX_D_INODE(&din3, i_size);
	rc = minix_unmap_zone(TST_INO, &din3, MINIX_ZONE_SIZE(&sb)/2,
	    MINIX_ZONE_SIZE(&sb)/2 );
	if ( ( rc == 0 ) && ( MINIX_D_INODE(&din3, i_size) == MINIX_ZONE_SIZE(&sb)/2 ) )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/* データ作成 */
	rc = minix_unmap_zone(TST_INO, &din3, 0, MINIX_D_INODE(&din3, i_size));
	kassert( rc == 0 );
	rc = write_test( TST_INO, &din3, 0, MINIX_ZONE_SIZE(&sb), &wrlen);
	if ( ( rc == 0 ) && ( wrlen == MINIX_D_INODE(&din3, i_size) ) )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	/* 開始: ゾーン境界, ファイル先頭
	   終了: ゾーン非境界, ファイル末尾-1/2ゾーン(1/2ゾーン残留)
	 */
	old_siz = MINIX_D_INODE(&din3, i_size);
	rc = minix_unmap_zone(TST_INO, &din3, 0, MINIX_ZONE_SIZE(&sb)/2 );
	if ( ( rc == 0 ) && ( MINIX_D_INODE(&din3, i_size) == old_siz ) )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/* データ作成 */
	rc = minix_unmap_zone(TST_INO, &din3, 0, MINIX_D_INODE(&din3, i_size));
	kassert( rc == 0 );
	rc = write_test( TST_INO, &din3, 0, MINIX_ZONE_SIZE(&sb), &wrlen);
	if ( ( rc == 0 ) && ( wrlen == MINIX_D_INODE(&din3, i_size) ) )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	/* 開始: ゾーン非境界, ファイル先頭+1/4ゾーン
	   終了: ゾーン非境界, ファイル末尾-1/4ゾーン (ファイルの末尾1/4ゾーン残留)
	 */
	old_siz = MINIX_D_INODE(&din3, i_size);
	rc = minix_unmap_zone(TST_INO, &din3, MINIX_ZONE_SIZE(&sb)/4, 
	    MINIX_ZONE_SIZE(&sb)/2 );
	if ( ( rc == 0 ) && ( MINIX_D_INODE(&din3, i_size) == old_siz ) )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/* 
	 * ディレクトリエントリ
	 */
	/* 検索 */
	rc = minix_lookup_dentry_by_name(&sb, &din1, "hello.txt", &ent);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	rc = minix_bitmap_alloc(&sb, INODE_MAP, &idx);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	kprintf("Allocated new I-node num: %d\n", idx);
	if ( rc == 0 ) {

		rc = minix_add_dentry(&sb, ROOT_INO, &din1, "hello.txt", idx);
		if ( rc == -EEXIST )
			ktest_pass( sp );
		else
			ktest_fail( sp );

		rc = minix_add_dentry(&sb, ROOT_INO, &din1, "newfile.txt", idx);
		if ( rc == 0 )
			ktest_pass( sp );
		else
			ktest_fail( sp );
		
		rc = minix_del_dentry(&sb, ROOT_INO, &din1, "no-file", &idx);
		if ( rc != 0 )
			ktest_pass( sp );
		else
			ktest_fail( sp );

		rc = minix_del_dentry(&sb, ROOT_INO, &din1, "newfile.txt", &idx);
		if ( rc == 0 )
			ktest_pass( sp );
		else
			ktest_fail( sp );

		kprintf("free I-node num: %d\n", idx);
		rc = minix_bitmap_free(&sb, INODE_MAP, idx);
		if ( rc == 0 )
			ktest_pass( sp );
		else
			ktest_fail( sp );
	}
}

void
tst_minixfs(void){

	ktest_def_test(&tstat_minixfs, "minixfs1", minixfs1, NULL);
	ktest_run(&tstat_minixfs);
}


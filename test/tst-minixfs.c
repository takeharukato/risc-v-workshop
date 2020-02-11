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

static void
minixfs1(struct _ktest_stats *sp, void __unused *arg){
	int               rc;
	minix_super_block sb;
	minix_bitmap_idx idx;
	minix_inode     din1;
	int             type;
	int             didx;
	int             idx1;
	int             idx2;
	
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

	rc = minix_bitmap_free(&sb, INODE_MAP, idx);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	rc = minix_rw_disk_inode(&sb, 1, MINIX_INODE_READING, &din1);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	MINIX_D_INODE_SET(&din1, i_uid, 0);
	rc = minix_rw_disk_inode(&sb, 1, MINIX_INODE_WRITING, &din1);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	rc = minix_rw_disk_inode(&sb, 1, MINIX_INODE_READING, &din1);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	if ( MINIX_D_INODE(&din1, i_uid) == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/*
	 * Zoneインデクス計算
	 */
	/* 直接参照ゾーン最小値 */
	rc = minix_calc_indexes(&sb, 0 * MINIX_ZONE_SIZE(&sb), 
	    &type, &didx, &idx1, &idx2);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	/* 直接参照ゾーン最大値 */
	rc = minix_calc_indexes(&sb, ( MINIX_NR_DZONES(&sb) - 1 ) * MINIX_ZONE_SIZE(&sb),
	    &type, &didx, &idx1, &idx2);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	/* 単間接参照ゾーン最小値 */
	rc = minix_calc_indexes(&sb, MINIX_NR_DZONES(&sb) * MINIX_ZONE_SIZE(&sb),
	    &type, &didx, &idx1, &idx2);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	/* 単間接参照ゾーン最大値 */
	rc = minix_calc_indexes(&sb,( MINIX_NR_DZONES(&sb) +
		( MINIX_NR_IND_ZONES(&sb) * MINIX_INDIRECTS(&sb) ) )
	    * MINIX_ZONE_SIZE(&sb) - 1,
	    &type, &didx, &idx1, &idx2);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	/* 2重間接参照ゾーン最小値 */
	rc = minix_calc_indexes(&sb,( MINIX_NR_DZONES(&sb) +
		( MINIX_NR_IND_ZONES(&sb) * MINIX_INDIRECTS(&sb) ) )
	    * MINIX_ZONE_SIZE(&sb),
	    &type, &didx, &idx1, &idx2);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	/* 2重間接参照ゾーン最大値 */
	rc = minix_calc_indexes(&sb, ( MINIX_NR_DZONES(&sb) +
		( MINIX_NR_IND_ZONES(&sb) * MINIX_INDIRECTS(&sb) ) + 
		( MINIX_NR_DIND_ZONES(&sb) * MINIX_INDIRECTS(&sb) * MINIX_INDIRECTS(&sb) ) )
	    * MINIX_ZONE_SIZE(&sb) - 1,
	    &type, &didx, &idx1, &idx2);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/* レンジ外 */
	rc = minix_calc_indexes(&sb, ( MINIX_NR_DZONES(&sb) +
		( MINIX_NR_IND_ZONES(&sb) * MINIX_INDIRECTS(&sb) ) + 
		( MINIX_NR_DIND_ZONES(&sb) * MINIX_INDIRECTS(&sb) * MINIX_INDIRECTS(&sb) ) )
	    * MINIX_ZONE_SIZE(&sb), &type, &didx, &idx1, &idx2);
	if ( rc == -E2BIG )
		ktest_pass( sp );
	else
		ktest_fail( sp );
}

void
tst_minixfs(void){

	ktest_def_test(&tstat_minixfs, "minixfs1", minixfs1, NULL);
	ktest_run(&tstat_minixfs);
}


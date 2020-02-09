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

int minix_read_super(dev_id _dev, minix_super_block *_sbp);
int minix_write_super(minix_super_block *_sbp);
int minix_bitmap_alloc(minix_super_block *_sbp, int map_type, minix_bitmap_idx *_idxp);
static void
minixfs1(struct _ktest_stats *sp, void __unused *arg){
	int rc;
	minix_super_block sb;
	minix_bitmap_idx idx;

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
}

void
tst_minixfs(void){

	ktest_def_test(&tstat_minixfs, "minixfs1", minixfs1, NULL);
	ktest_run(&tstat_minixfs);
}


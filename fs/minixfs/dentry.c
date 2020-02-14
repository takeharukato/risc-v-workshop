/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Minix file system directory entry operations                      */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>

#include <kern/spinlock.h>
#include <kern/dev-pcache.h>

#include <fs/minixfs/minixfs.h>

int
minix_lookup_dentry_by_name(minix_inode *dirip, const char *name, 
    minix_dentry *de){
	int                rc;  /* 返り値                                        */
	minix_dentry      ent;  /* ディレクトリエントリ                          */
	obj_cnt_type      pos;  /* ディレクトリエントリ読み取り位置(単位:個)     */
	obj_cnt_type  nr_ents;  /* ディレクトリエントリ総数(単位:個)             */
	size_t          rdlen;  /* 読み込んだディレクトリエントリ長(単位:バイト) */
	minixv2_ino  v12_inum;  /* MinixV1/V2でのI-node番号                      */
	minixv3_ino   v3_inum;  /* MinixV3でのI-node番号                         */
	void           *fname;  /* ファイル名へのポインタ                        */

	/* TODO: ディレクトリであることを確認 */

	nr_ents = MINIX_D_INODE(dirip, i_size) / MINIX_D_DENT_SIZE(dirip->sbp);

	for(pos = 0; nr_ents > pos; ++pos) {

		rc = minix_rw_zone(MINIX_NO_INUM(dirip->sbp), dirip, 
		    &ent, pos*MINIX_D_DENT_SIZE(dirip->sbp), MINIX_D_DENT_SIZE(dirip->sbp), 
		    MINIX_RW_READING, &rdlen);
		if ( ( rc != 0 ) || ( rdlen != MINIX_D_DENT_SIZE(dirip->sbp) ) )
			break;

		fname = (void *)&ent + MINIX_D_DIRNAME_OFFSET(dirip->sbp);
		if ( strncmp(name, fname, MINIX_D_DIRSIZ(dirip->sbp)) == 0 ) {

			/* 一致するエントリが見つかった */
			if ( MINIX_SB_IS_V3(dirip->sbp) ) {

				if ( dirip->sbp->swap_needed )
					v3_inum = __bswap32( *(minixv3_ino *)&ent );
				else
					v3_inum = *(minixv3_ino *)&ent;
				memmove((void *)de, &v3_inum, sizeof(minixv3_ino));
			} else {

				if ( dirip->sbp->swap_needed )
					v12_inum = __bswap32( *(minixv2_ino *)&ent );
				else
					v12_inum = *(minixv2_ino *)&ent;
				memmove((void *)de, &v12_inum, sizeof(minixv2_ino));
			}
			memmove((void *)de + MINIX_D_DIRNAME_OFFSET(dirip->sbp),
			    fname, MINIX_D_DIRSIZ(dirip->sbp));
			return 0;
		}
	}

	return -ENOENT;
}

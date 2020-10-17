/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Minix super block operations for an image file                    */
/*                                                                    */
/**********************************************************************/

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <klib/freestanding.h>
#include <kern/kern-consts.h>
#include <klib/klib-consts.h>
#include <klib/misc.h>
#include <klib/align.h>
#include <klib/compiler.h>
#include <kern/kern-types.h>
#include <kern/page-macros.h>

#include <kern/dev-old-pcache.h>

#include <fs/minixfs/minixfs.h>

#include <minixfs-tools.h>
#include <utils.h>

/**
   パスをディレクトリ部とファイル名部に分離する
 */
int
path_get_basename(char *path, char **dir, char **name){
	char *delim_pos;

	delim_pos = strrchr((const char *)path, MKFS_MINIX_PATH_DELIMITER);
	if ( delim_pos == NULL ) {

		*dir = NULL;  /* ディレクトリ部は存在しない */
		*name = path; /* ファイル名を返却           */
	} else {

		*delim_pos++ = '\0';
		*dir = path;  /* ディレクトリ部を返却 */
		*name = delim_pos; /* ファイル名を返却 */
	}

	return 0;
}
/**
    指定されたパスのMinix I-nodeを獲得する
    @param[in] sbp    Minix スーパブロック情報
    @param[in] path   検索対象のパス文字列
    @param[out] outv  見つかったMinix I-nodeの返却領域
    @retval  0        正常終了
    @retval  -ENOENT  パスが見つからなかった
    @retval  -EIO     パス検索時にI/Oエラーが発生した
 */
int
path_to_minix_inode(minix_super_block *sbp, minix_inode *root, char *path, minix_inode *outv){
	char              *p;
	char         *next_p;
	minix_inode  *curr_v;
	minix_inode       vp;
	minix_dentry    dent;
	int               rc;

	p = path;

	curr_v = root;  /*  ルートディレクトリから検索を開始  */
	if ( *p == '/' )  /* 絶対パス指定  */
		for( p += 1 ; *p == '/'; ++p);  /*  連続した'/'を飛ばす  */

	/*
	 * パスの探索
	 */
	for(;;) {

		if (*p == '\0') { /* パスの終端に達した */

			rc = 0;
			break;
		}

		/*
		 * 文字列の終端またはパスデリミタを検索
		 */
		for(next_p = p + 1;
		    ( *next_p != '\0' ) && ( *next_p != '/' );
		    ++next_p);

		if ( *next_p == '/' ) {

			*next_p = '\0'; /* 文字列を終端 */

			/*  連続した'/'を飛ばす  */
			for( next_p += 1; *next_p == '/'; ++next_p);
		}

		/*
		 * @note pは, パス上の1エレメントを指している
		 */

		/* エレメントに対応するディレクトリエントリを取得 */
		rc = minix_lookup_dentry_by_name(sbp, curr_v, p, &dent);
		if (rc != 0) {

			if ( ( rc != -EIO ) && ( rc != -ENOENT ) )
				rc = -EIO;  /*  IFを満たさないエラーは-EIOに変換  */

			goto error_out;
		}

		/* ディレクトリエントリに格納されているI-node番号をキーにI-nodeを取得 */
		rc = minix_rw_disk_inode(sbp, MINIX_D_DENT_INUM(sbp, &dent),
		    MINIX_RW_READING, &vp);
		if (rc != 0)
			goto error_out;

		/*
		 * 次の要素を参照
		 */
		p = next_p;
		curr_v = &vp;
	}

	if ( outv != NULL )
		memmove(outv, curr_v, sizeof(minix_inode));

	return 0;

error_out:
	return rc;
}

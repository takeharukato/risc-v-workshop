/* -*- mode: C; coding:utf-8 -*- */
/************************************************************************/
/*  OS kernel sample                                                    */
/*  Copyright 2019 Takeharu KATO                                        */
/*                                                                      */
/*  VFS path operation                                                  */
/*                                                                      */
/************************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>

#include <kern/spinlock.h>
#include <kern/wqueue.h>
#include <kern/mutex.h>

#include <kern/vfs-if.h>

/**
   パスの終端が/だったら/.に変換して返却する
   @param[in]  path 入力パス
   @param[out] conv 変換後のパス
   @retval  0             正常終了
   @retval -ENOENT        パスが含まれていない
   @retval -ENAMETOOLONG  パス名が長すぎる
 */
int
vfs_new_path(const char *path, char *conv){
	size_t path_len;
	size_t conv_len;
	const char  *sp;

	if ( path == NULL )
		return -ENOENT;  /* パスが含まれていない */

	path_len = strlen(path);
	if ( path_len == 0 )
		return -ENOENT;  /* パスが含まれていない */

	if ( path_len >= VFS_PATH_MAX )
		return -ENAMETOOLONG;  /* パス長が長すぎる */
	
	sp = path;
	/*
	 * 先頭の'/'を読み飛ばす
	 */
	while( *sp == VFS_PATH_DELIM )
		++sp;
	if ( *sp == '\0' )
		return -ENOENT;  /* パスが含まれていない */

	conv_len = strlen(sp);
	while( sp[conv_len - 1] == VFS_PATH_DELIM )
		--conv_len;

	if ( ( conv_len + 2 ) >= VFS_PATH_MAX )
		return -ENAMETOOLONG;  /* パス長が長すぎる */

	strncpy(conv, sp, conv_len);
	conv_len += 2; /* "/."分を追加する */
	conv[conv_len - 2]=VFS_PATH_DELIM;
	conv[conv_len - 1]='.';
	conv[conv_len]='\0';

	return 0;
}
/**
   パスを結合する
 */
int
vfs_cat_paths(char *a, char *b, char *result){
    char        *p;
    size_t     len;

    len = strlen(a) + strlen(b) + 2;

    if ( len >= VFS_PATH_MAX ) 
	    return -ENOMEM;
    
    snprintf(result, VFS_PATH_MAX, "%s/%s", a, b);

    return 0;
}

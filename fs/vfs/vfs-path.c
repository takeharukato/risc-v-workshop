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
   @param[in]  path1入力パス1
   @param[in]  path2入力パス2
   @param[out] conv 変換後のパス
   @retval  0             正常終了
   @retval -ENOENT        パスが含まれていない
   @retval -ENAMETOOLONG  パス名が長すぎる
 */
int
vfs_cat_paths(char *path1, char *path2, char *conv){
	size_t    len1;
	size_t    len2;
	size_t     len;
	char       *ep;
	char       *sp;
	
	len1 = strlen(path1);
	len2 = strlen(path2);

	if ( ( len1 == 0 ) && ( len2 == 0 ) )
		return -ENOENT;  /* 両者にパスが含まれていない */

	/*
	 *一つ目のパスの終端のパスデリミタを取り除く
	 */
	if ( len1 > 0 ) {
		
		ep = &path1[len1 - 1];
		while( *ep == VFS_PATH_DELIM ) {
			
			*ep-- = '\0';
			--len1;
		}
	}
	
	/*
	 *二つ目のパスの先頭のパスデリミタを取り除く
	 */
	if ( len2 > 0 ) {
		
		sp = path2;
		while( *sp == VFS_PATH_DELIM ) {
			
			++sp;
			--len2;
		}
	}

	/*
	 * 文字列間にパスデリミタを付与して結合する
	 */
	len = len1 + len2  + 2;
	if ( len >= VFS_PATH_MAX ) 
		return -ENAMETOOLONG;

	ksnprintf(conv, VFS_PATH_MAX, "%s%c%s", path1, VFS_PATH_DELIM, sp);
	
	return 0;
}

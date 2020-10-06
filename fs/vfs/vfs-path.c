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
   指定されたパスのvnodeの参照を獲得する(実処理関数)
   @param[in] ioctx  パス検索に使用するI/Oコンテキスト
   @param[in] path   検索対象のパス文字列
   @param[out] outv  見つかったvnodeを指し示すポインタのアドレス
   @retval   0       正常終了
   @retval  -ENOENT  パスが見つからなかった
   @retval  -EIO     パス検索時にI/Oエラーが発生した
 */
static int
path_to_vnode(vfs_ioctx *ioctx, char *path, vnode **outv){
	char            *p;
	char       *next_p;
	vnode      *curr_v;
	vnode      *next_v;
	vfs_vnode_id  vnid;
	int             rc;

	kassert( ioctx->ioc_root != NULL );

	p = path;

	if ( *p == VFS_PATH_DELIM ) { /* 絶対パス指定  */

		for( p += 1 ; *p == VFS_PATH_DELIM; ++p);  /*  連続した'/'を飛ばす  */

		curr_v = ioctx->ioc_root;  /*  現在のルートディレクトリから検索を開始  */
		kassert( curr_v != NULL );

		vfs_vnode_ref_inc(curr_v);  /* 現在のv-nodeへの参照を加算 */
	} else { /* 相対パス指定  */

		mutex_lock(&ioctx->ioc_mtx);

		curr_v = ioctx->ioc_cwd;    /*  現在のディレクトリから検索を開始  */
		kassert( curr_v != NULL );

		vfs_vnode_ref_inc(curr_v);  /* 現在のv-nodeへの参照を加算 */
		mutex_unlock(&ioctx->ioc_mtx);
	}

	/*
	 * パスの探索
	 */
	for(;;) {

		if ( *p == '\0' ) { /* パスの終端に達した */

			rc = 0;
			break;
		}

		/*
		 * 文字列の終端またはパスデリミタを検索
		 */
		for(next_p = p + 1;
		    ( *next_p != '\0' ) && ( *next_p != VFS_PATH_DELIM );
		    ++next_p);

		if ( *next_p == VFS_PATH_DELIM ) {

			*next_p = '\0'; /* 文字列を終端 */

			/*  連続した'/'を飛ばす  */
			for( next_p += 1; *next_p == VFS_PATH_DELIM; ++next_p);
		}

		/*
		 * @note pは, パス上の1エレメントを指している
		 */

		if ( (strcmp("..", p) == 0 ) && ( curr_v->v_mount->m_root == curr_v ) ) {

			/* ボリューム内のルートディレクトリより
			 * 上位のディレクトリへ移動する場合
			 */
			if ( curr_v->v_mount->m_mount_point != NULL ) {

				/*  マウントポイントであれば, マウント元のFSの
				 *  vnodeに切り替え
				 */
				next_v = curr_v->v_mount->m_mount_point;
				vfs_vnode_ref_inc(next_v);
				vfs_vnode_ref_dec(curr_v);
				curr_v = next_v;
			}
		}

		/* 下位のFSにパス解析を移譲し, エレメントに対応するv-node番号を取得
		 */
		kassert( curr_v->v_mount != NULL );
		kassert( curr_v->v_mount->m_fs != NULL );
		kassert( curr_v->v_mount->m_fs->c_calls != NULL );
		kassert( is_valid_fs_calls( curr_v->v_mount->m_fs->c_calls ) );
		rc = curr_v->v_mount->m_fs->c_calls->fs_lookup(curr_v->v_mount->m_fs_super,
							 curr_v->v_fs_vnode, p, &vnid);
		if (rc != 0) {

			if ( ( rc != -EIO ) && ( rc != -ENOENT ) )
				rc = -EIO;  /*  IFを満たさないエラーは-EIOに変換  */

			vfs_vnode_ref_dec(curr_v);
			goto error_out;
		}

		/*
		 * マウントポイントID, v-node IDをキーにv-nodeの参照を獲得
		 */
		rc = vfs_vnode_get(curr_v->v_mount->m_id, vnid, &next_v);
		if (rc != 0) {

			kprintf(KERN_ERR "path_to_vnode: could not lookup vnode"
			    " (fsid 0x%x vnid 0x%Lx)\n",
			    (unsigned)curr_v->v_mount->m_id, (unsigned long long)vnid);
			vfs_vnode_ref_dec(curr_v);
			goto error_out;
		}

		/*  vnodeの参照を開放  */
		vfs_vnode_ref_dec(curr_v);

		/*
		 * 次の要素を参照
		 */
		p = next_p;
		curr_v = next_v;

		if ( curr_v->v_mount_on != NULL ) {

			/* マウントポイントだった場合は,
			 * マウント先のファイルシステム上の
			 * root v-nodeを参照
			 */
			next_v = curr_v->v_mount_on->m_root;
			vfs_vnode_ref_inc(next_v);
			vfs_vnode_ref_dec(curr_v);
			curr_v = next_v;
		}
	}

	*outv = curr_v;

error_out:
	return rc;
}

/**
   指定されたディレクトリのvnodeの参照を獲得する (実処理関数)
   @param[in]  ioctx    パス検索に使用するI/Oコンテキスト
   @param[in]  path     検索対象のパス文字列
   @param[in]  pathlen  pathが指し示す領域の長さ
   @param[out] outv     見つかったvnodeを指し示すポインタのアドレス
   @param[out] filename パス中のファイル名部分を返却する領域のアドレス
   @param[in]  fnamelen filenameが指し示す領域の長さ
   @retval  0        正常終了
   @retval  -ENOENT   パスが見つからなかった
   @retval  -EIO      パス検索時にI/Oエラーが発生した
*/
static int
path_to_dir_vnode(vfs_ioctx *ioctx, char *path, size_t pathlen, vnode **outv,
    char *filename, size_t fnamelen){
	char *p;

	kassert( pathlen > 0 );
	kassert( fnamelen > 0 );

	p = strrchr(path, '/');
	if ( p == NULL ) {

		/* 引数pathが'/'を全く含まない場合 (例: "afile")は,
		 * ./afile を指定されたとみなして, filenameには
		 * "afile"をコピーして返却し, 検索パスには"."を引き渡す
		 */
		strncpy(filename, path, fnamelen);
		filename[fnamelen - 1] = '\0';

		strncpy(path, ".", pathlen);
		filename[pathlen - 1] = '\0';
	} else {

		/* ディレクトリのvnodeを獲得する
		 * 末尾の/の後にカレントディレクトリを表す.だけを
		 * 含むエレメントを作成して文字列を終端させることで
		 * ディレクトリのvnodeを獲得する
		 * e.g., path: "/xxx/yyy"の場合なら"/xxx/."に置き換えて,
		 *             "/xxx/"のvnodeを獲得させる
		 *       pathが'/'で終わる場合はpathを修正せずに検索パスに引き渡す
		 * filenameには, path中の/の後に続く文字列(ファイル部)をコピーして返却する
		 */
		strncpy(filename, p+1, fnamelen);
		filename[fnamelen - 1] = '\0';

		if( p[1] != '\0' ){  /* pathが'/'で終わっていない場合 */

			p[1] = '.';
			p[2] = '\0';
		}
	}

	/*
	 * パス中のディレクトリ部分(親ディレクトリ)のvnodeを得る
	 */
	return path_to_vnode(ioctx, path, outv);
}

/**
   指定されたパスのvnodeの参照を獲得する
   @param[in]  ioctx   パス検索に使用するI/Oコンテキスト
   @param[in]  path    検索対象のパス文字列
   @param[out] outv    見つかったvnodeを指し示すポインタのアドレス
   @retval  0        正常終了
   @retval  -ENOENT   パスが見つからなかった
   @retval  -EIO      パス検索時にI/Oエラーが発生した
 */
int
vfs_path_to_vnode(vfs_ioctx *ioctx, char *path, vnode **outv) {

	/*
	 * パスのvnodeを得る
	 */
	return path_to_vnode(ioctx, path, outv);
}

/**
   指定されたディレクトリのvnodeの参照を獲得する
   @param[in]  ioctx    パス検索に使用するI/Oコンテキスト
   @param[in]  path     検索対象のパス文字列
   @param[in]  pathlen  検索対象のパス文字列領域の長さ
   @param[out] outv     見つかったvnodeを指し示すポインタのアドレス
   @param[out] filename パス中のファイル名部分を返却する領域のアドレス
   @param[in]  fnamelen filenameが指し示す領域の長さ
   @retval  0        正常終了
   @retval  -ENOENT   パスが見つからなかった
   @retval  -EIO      パス検索時にI/Oエラーが発生した
   @note parenti相当のIF
 */
int
vfs_path_to_dir_vnode(vfs_ioctx *ioctx, char *path, size_t pathlen, vnode **outv,
    char *filename, size_t fnamelen) {

	/*
	 * ファイルの親ディレクトリのvnodeを得る
	 */
	return path_to_dir_vnode(ioctx, path, pathlen, outv, filename, fnamelen);
}

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
	 * 先頭の連続した'/'を消去する
	 */
	while( *sp == VFS_PATH_DELIM )
		++sp;
	if ( *sp == '\0' )
		return -ENOENT;  /* パスが含まれていない */

	/*
	 * 末尾の連続した'/'を消去する
	 */
	conv_len = strlen(sp);
	while( ( conv_len > 0 ) && ( sp[conv_len - 1] == VFS_PATH_DELIM ) )
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
   @param[in]  path1 入力パス1
   @param[in]  path2 入力パス2
   @param[out] conv  変換後のパス
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

	len1 = len2 = 0;

	if ( path1 != NULL )
		len1 = strlen(path1);

	if ( path2 != NULL )
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
	len = len1 + len2;

	if ( len >= VFS_PATH_MAX )
		return -ENAMETOOLONG;

	if ( len1 == 0 ) { /* 二つ目のパスを返却 */

		strcpy(conv, path2);
		return 0;
	}

	if ( len2 == 0 ) { /* 一つ目のパスを返却 */

		strcpy(conv, path1);
		return 0;
	}

	len += 2;  /* パスデリミタとヌル終端分の長さを追加 */
	if ( len >= VFS_PATH_MAX )
		return -ENAMETOOLONG;

	ksnprintf(conv, VFS_PATH_MAX, "%s%c%s", path1, VFS_PATH_DELIM, sp);

	return 0;
}

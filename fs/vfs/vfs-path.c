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

#include <kern/page-if.h>

#include <kern/vfs-if.h>

#include <fs/vfs/vfs-internal.h>

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
	char     *copypath;
	vfs_vnode_id  vnid;
	int             rc;

	kassert( ioctx->ioc_root != NULL );

	copypath = strdup(path);  /* 引数で渡されたパスの複製を得る */
	if ( copypath == NULL ) {

		rc = -ENOMEM;  /* メモリ不足 */
		goto error_out;
	}

	p = copypath;

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

		kassert( !vfs_vnode_locked_by_self(curr_v) );  /* 自己再入しないことを確認 */

		/* ファイル名検索中にディレクトリエントリを書き換えられないように
		 * v-nodeのロックを獲得
		 */
		rc = vfs_vnode_lock(curr_v);
		kassert( rc != -ENOENT );
		if ( rc != 0 ) /* イベントを受信したまたはメモリ不足 */
			goto free_copypath_out;

		rc = curr_v->v_mount->m_fs->c_calls->fs_lookup(curr_v->v_mount->m_fs_super,
							 curr_v->v_fs_vnode, p, &vnid);
		if (rc != 0) {

			if ( ( rc != -EIO ) && ( rc != -ENOENT ) )
				rc = -EIO;  /*  IFを満たさないエラーは-EIOに変換  */

			vfs_vnode_unlock(curr_v);  /* v-nodeのロックを解放 */
			vfs_vnode_ref_dec(curr_v); /*  vnodeの参照を開放  */
			goto free_copypath_out;
		}

		vfs_vnode_unlock(curr_v);  /* v-nodeのロックを解放 */

		/*
		 * マウントポイントID, v-node IDをキーにv-nodeの参照を獲得
		 */
		rc = vfs_vnode_get(curr_v->v_mount->m_id, vnid, &next_v);
		if (rc != 0) {

			kprintf(KERN_ERR "path_to_vnode: could not lookup vnode"
			    " (fsid 0x%x vnid 0x%Lx)\n",
			    (unsigned)curr_v->v_mount->m_id, (unsigned long long)vnid);

			vfs_vnode_ref_dec(curr_v); /*  vnodeの参照を開放  */
			goto free_copypath_out;
		}

		vfs_vnode_ref_dec(curr_v);  /*  vnodeの参照を開放  */

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

free_copypath_out:
	kfree(copypath);  /* 一時領域を解放する */

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
   @retval  -ENOMEM  メモリ不足
   @retval  -ENOENT  パスが見つからなかった
   @retval  -EIO     パス検索時にI/Oエラーが発生した
*/
static int
path_to_dir_vnode(vfs_ioctx *ioctx, char *path, size_t pathlen, vnode **outv,
    char *filename, size_t fnamelen){
	int         rc;
	char        *p;

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
	rc = path_to_vnode(ioctx, path, outv);

	return rc;
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
   @param[out] outv     見つかったvnodeを指し示すポインタのアドレス
   @param[out] filename パス中のファイル名部分を返却する領域のアドレス
   @param[in]  fnamelen filenameが指し示す領域の長さ
   @retval  0        正常終了
   @retval  -ENOENT   パスが見つからなかった
   @retval  -EIO      パス検索時にI/Oエラーが発生した
   @note parenti相当のIF
 */
int
vfs_path_to_dir_vnode(vfs_ioctx *ioctx, char *path, vnode **outv,
    char *filename, size_t fnamelen) {
	int         rc;
	char *copypath;
	size_t pathlen;

	pathlen = strlen(path) + 1;  /* ヌル文字を含む領域の長さ */
	copypath = strdup(path);  /* 引数で渡されたパスの複製を得る */
	if ( copypath == NULL ) {

		rc = -ENOMEM;  /* メモリ不足 */
		goto error_out;
	}

	/*
	 * ファイルの親ディレクトリのvnodeを得る
	 */
	rc = path_to_dir_vnode(ioctx, copypath, pathlen, outv, filename, fnamelen);
	if ( rc != 0 )
		goto free_copypath_out;

	kfree(copypath);  /* 一時領域を開放する */

	return 0;

free_copypath_out:
	kfree(copypath);  /* 一時領域を開放する */

error_out:
	return rc;
}

/**
   パス中の"../","./"を解釈したパス名を構成する
   @param[in]  cur_abspath   現在のパス
   @param[out] new_pathp  変換後のパスを指し示すポインタのアドレス
   @retval  0             正常終了
   @retval -ENOENT        パスが絶対パスでない
 */
int
vfs_path_resolve_dotdirs(char *cur_abspath, char **new_pathp){
	int          rc;
	char       *inp;
	char      *outp;
	char    *outbuf;
	char       inch;
	size_t   outlen;

	if ( new_pathp == NULL )
		return 0; /* パスは返却しないが正常終了する */

	if ( cur_abspath == NULL ) {  /* パスが指定されていない */

		rc = -EINVAL;
		goto error_out;
	}

	/* cur_abspathが絶対パス指定でない場合(パスデリミタから始まらない場合)は
	 * 引数エラーとする
	 */
	if ( cur_abspath[0] != VFS_PATH_DELIM ) {

		rc = -EINVAL;
		goto error_out;
	}

	/* 出力バッファを用意する
	 */
	outlen = strlen(cur_abspath);
	outbuf = kmalloc(outlen + 1, KMALLOC_NORMAL);
	if ( outbuf == NULL ) {

		rc = -ENOMEM;
		goto error_out;
	}

	inp = cur_abspath; /* 入力位置を設定 */
	outp = outbuf;  /* 出力位置を設定 */

	/* 出力バッファの先頭にルートディレクトリのパスデリミタを配置,
	 * ルートディレクトリの次の位置に入出力位置を設定
	 */
	*outp++ = *inp++;
	kassert( *outbuf == VFS_PATH_DELIM );

	for( ; ; ) {

		inch = *inp++;  /* 入力ポインタ位置の文字を読み取り, 入力位置を進める */
		if ( inch == '\0' )
			break; /* 入力ポインタ位置が文字列終端だったら抜ける */

		/* 出力ポインタがパスデリミタの直後で, かつ,
		 * 入力ポインタ位置が"./"だった場合,
		 * 入力ポインタを2つ進める
		 */
		if ( ( *( outp - 1 ) == VFS_PATH_DELIM )
		    && ( inch == '.' )
		    && ( *inp == VFS_PATH_DELIM ) ) {

			inp += 1;  /* "./"の'/'を読み飛ばす */
			continue;  /* 読み取りを継続する */
		}

		/* 出力ポインタがパスデリミタの直後で, かつ,
		 * 入力ポインタ位置が"../"だった場合, 入力ポインタを3つ進め,
		 * 出力ポインタ位置が出力バッファの先頭(ルートディレクトリのパスデリミタ)で
		 * なければ出力ポインタの前の/を検索し, その次の位置に出力ポインタを配置する
		 * 出力バッファ中の1つ上のディレクトリを指し示す
		 */
		if ( ( *( outp - 1 ) == VFS_PATH_DELIM )
		    && ( inch == '.' ) && ( *inp == '.' )
		     && ( ( *(inp + 1) == VFS_PATH_DELIM ) ||
			  ( *(inp + 1) == '\0' ) ) ) {

			if ( *(inp + 1) == VFS_PATH_DELIM )
				inp += 2;  /* "../"の"./"を読み飛ばす */
			else
				inp += 1;  /* ".."の2文字目の"."を読み飛ばす */
			/* ルートディレクトリを指していなければ
			 * 出力先を一つ上のディレクトリ位置まで戻す
			 */
			if ( ( outp - 1 ) > outbuf ) {


				outp -= 2; /* 一つ上の上のディレクトリに移る */
				while( ( outp != outbuf ) && ( *outp != VFS_PATH_DELIM ) )
					--outp;  /* パスデリミタの位置まで戻す */

				/* 先頭にパスデリミタをセットしているのでパスデリミタを
				 * 指しているはず
				 */
				kassert( *outp == VFS_PATH_DELIM );
				++outp;  /* デリミタの次の位置から書き出しを行う */
			}
			continue;  /* 読み取りを継続する */
		}

		if ( inch == VFS_PATH_DELIM ) { /* パスデリミタだった場合 */

			/* 後続の連続したパスデリミタを読み飛ばす */
			while( *inp == VFS_PATH_DELIM )
				++inp; /* デリミタを飛ばす */

			 /* 直前がデリミタである場合は出力しない
			  * (e.g., "/bin/..//"の場合, "../を処理した時点では,
			  * "/"の直後を指しているため, ここでデリミタを出力することで
			  * 連続したデリミタが出力されてしまうことを避けるため。
			  */
			if ( *( outp - 1 ) == VFS_PATH_DELIM )
				continue;
		}
		*outp++ = inch;  /* 出力バッファに文字を書き込む */
	}

	*outp = '\0';  /* ヌルターミネートする */

	kassert( new_pathp != NULL );

	*new_pathp = strdup(outbuf);  /* 文字列を複製する */

	kfree(outbuf);  /* 一時領域を開放する */

	return 0;

error_out:
	return rc;
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
   @param[in]  path1  入力パス1
   @param[in]  path2  入力パス2
   @param[out] convp  変換後のパスを指し示すポインタのアドレス
   @retval  0             正常終了
   @retval -ENOENT        パスが含まれていない
   @retval -ENOMEM        メモリ不足
   @retval -ENAMETOOLONG  パス名が長すぎる
 */
int
vfs_paths_cat(char *path1, char *path2, char **convp){
	int           rc;
	size_t      len1;
	size_t      len2;
	size_t       len;
	char  *path1_str;
	char  *path2_str;
	char         *ep;
	char         *sp;
	char       *conv;
	char    *dupconv;

	len1 = len2 = 0;

	if ( path1 != NULL )
		len1 = strlen(path1);

	if ( path2 != NULL )
		len2 = strlen(path2);

	if ( ( len1 == 0 ) && ( len2 == 0 ) )
		return -ENOENT;  /* 両者にパスが含まれていない */

	if ( convp == NULL )
		return 0; /* パスを返却しないで正常終了する */

	/* 第1引数の文字列を複製
	 */
	if ( len1 > 0 ) {

		path1_str = strdup(path1);
		if ( path1_str == NULL ) {

			rc = -ENOMEM;  /* メモリ不足 */
			goto error_out;
		}
	}

	/* 第2引数の文字列を複製
	 */
	if ( len2 > 0 ) {

		path2_str = strdup(path2);
		if ( path2_str == NULL ) {

			rc = -ENOMEM;  /* メモリ不足 */
			goto free_path1_str_out;
		}
	}
	/*
	 *一つ目のパスの終端のパスデリミタを取り除く
	 */
	if ( len1 > 0 ) {

		ep = &path1_str[len1 - 1];
		/* 先頭のパスデリミタは残して後続のパスデリミタを取り除く */
		while( ( len1 > 1 ) && ( *ep == VFS_PATH_DELIM ) ) {

			*ep-- = '\0';
			--len1;
		}
	}

	/*
	 *二つ目のパスの先頭のパスデリミタを取り除く
	 */
	if ( len2 > 0 ) {

		sp = path2_str;
		while( *sp == VFS_PATH_DELIM ) {

			++sp;
			--len2;
		}
	}

	/*
	 * 文字列間にパスデリミタを付与して結合する
	 */
	if ( ( ( len1 > 0 ) && ( path1_str[len1 - 1] == VFS_PATH_DELIM ) )
		|| ( ( len1 == 0 ) || ( len2 == 0 ) ) )
		len = len1 + len2 + 1; /* 文字列長の合計にヌル文字分を追加 */
	else
		len = len1 + len2 + 2; /* 文字列長の合計にヌル文字とパスデリミタ分を追加 */

	if ( len >= VFS_PATH_MAX ) {

		rc = -ENAMETOOLONG;  /* パス名が長すぎる */
		goto free_path2_str_out;
	}

	conv = kmalloc(len, KMALLOC_NORMAL);  /* 返却先バッファを獲得 */
	if ( conv == NULL ) {

		rc = -ENOMEM; /* メモリ不足 */
		goto free_path2_str_out;
	}

	if ( len1 == 0 ) { /* 二つ目のパスを返却 */

		strcpy(conv, path2_str);
		goto duplicate_conv;
	}

	if ( len2 == 0 ) { /* 一つ目のパスを返却 */

		strcpy(conv, path1_str);
		goto duplicate_conv;
	}

	/* パスをデリミタで接続 */
	if ( ( len1 > 0 ) && ( path1_str[len1 - 1] == VFS_PATH_DELIM ) )
		ksnprintf(conv, VFS_PATH_MAX, "%s%s", path1, sp);
	else
		ksnprintf(conv, VFS_PATH_MAX, "%s%c%s", path1, VFS_PATH_DELIM, sp);

duplicate_conv:
	kassert( convp != NULL );

	dupconv = strdup(conv);  /* 文字列を複製 */
	if ( dupconv == NULL ) {

		rc = -ENOMEM;  /* メモリ不足 */
		goto free_conv_out;
	}

	*convp = dupconv;  /* 結果を返却 */

	return 0;

free_conv_out:
	kfree(conv);        /* 変換用のバッファを解放     */

free_path2_str_out:
	if ( len2 > 0 )
		kfree(path2_str);   /* 第2引数文字列の複製を解放 */

free_path1_str_out:
	if ( len1 > 0 )
		kfree(path1_str);   /* 第1引数文字列の複製を解放 */

error_out:
	return rc;
}

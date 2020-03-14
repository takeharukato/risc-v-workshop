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

#include <kern/page-if.h>
#include <kern/vfs-if.h>

#include "tst-vfs-tstfs.h"

static kmem_cache tstfs_super; /**< super blockのSLABキャッシュ */
static kmem_cache tstfs_inode; /**< I-nodeのSLABキャッシュ */
static kmem_cache tstfs_dent;  /**< ディレクトリエントリのSLABキャッシュ */
static kmem_cache tstfs_dpage; /**< データページのSLABキャッシュ */

static tst_vfs_tstfs_db g_tstfs_db={.initialized = false,	\
				    .mtx = __MUTEX_INITIALIZER(&g_tstfs_db.mtx), \
				    .supers = RB_INITIALIZER(&g_tstfs_db.supers), \
};

/** スーパブロックRB木
 */
static int _tst_vfs_tstfs_super_cmp(struct _tst_vfs_tstfs_super *_key, 
				  struct _tst_vfs_tstfs_super *_ent);
RB_GENERATE_STATIC(_tst_vfs_tstfs_super_tree, _tst_vfs_tstfs_super, ent, 
		   _tst_vfs_tstfs_super_cmp);

/** I-nodeRB木
 */
static int _tst_vfs_tstfs_inode_cmp(struct _tst_vfs_tstfs_inode *_key, 
				  struct _tst_vfs_tstfs_inode *_ent);
RB_GENERATE_STATIC(_tst_vfs_tstfs_inode_tree, _tst_vfs_tstfs_inode, ent, 
		   _tst_vfs_tstfs_inode_cmp);

/** ディレクトリエントリRB木
 */
static int _tst_vfs_tstfs_dent_cmp(struct _tst_vfs_tstfs_dent *_key, 
				  struct _tst_vfs_tstfs_dent *_ent);
RB_GENERATE_STATIC(_tst_vfs_tstfs_dent_tree, _tst_vfs_tstfs_dent, ent, 
		   _tst_vfs_tstfs_dent_cmp);

/** データページRB木
 */
static int _tst_vfs_tstfs_dpage_cmp(struct _tst_vfs_tstfs_dpage *_key, 
				  struct _tst_vfs_tstfs_dpage *_ent);
RB_GENERATE_STATIC(_tst_vfs_tstfs_dpage_tree, _tst_vfs_tstfs_dpage, ent, 
		   _tst_vfs_tstfs_dpage_cmp);

/**
   スーパブロック比較関数
   @param[in] key 比較対象1
   @param[in] ent RB木内の各エントリ
   @retval 正  keyが entより前にある
   @retval 負  keyが entより後にある
   @retval 0   keyが entに等しい
 */
static int
_tst_vfs_tstfs_super_cmp(struct _tst_vfs_tstfs_super *key, 
				    struct _tst_vfs_tstfs_super *ent){	

	if ( key->s_devid < ent->s_devid )
		return 1;

	if ( ent->s_devid < key->s_devid )
		return -1;

	return 0;	
}

/**
   I-node比較関数
   @param[in] key 比較対象1
   @param[in] ent RB木内の各エントリ
   @retval 正  keyが entより前にある
   @retval 負  keyが entより後にある
   @retval 0   keyが entに等しい
 */
static int
_tst_vfs_tstfs_inode_cmp(struct _tst_vfs_tstfs_inode *key, 
			 struct _tst_vfs_tstfs_inode *ent){	

	if ( key->i_ino < ent->i_ino )
		return 1;

	if ( ent->i_ino < key->i_ino )
		return -1;

	return 0;	
}

/**
   ディレクトリエントリ比較関数
   @param[in] key 比較対象1
   @param[in] ent RB木内の各エントリ
   @retval 正  keyが entより前にある
   @retval 負  keyが entより後にある
   @retval 0   keyが entに等しい
 */
static int
_tst_vfs_tstfs_dent_cmp(struct _tst_vfs_tstfs_dent *key, 
			struct _tst_vfs_tstfs_dent *ent){	

	if ( strncmp(key->name, ent->name, TST_VFS_TSTFS_FNAME_LEN) > 0 )
		return 1;

	if ( strncmp(key->name, ent->name, TST_VFS_TSTFS_FNAME_LEN) < 0 )
		return -1;

	return 0;	
}

/**
   データページ比較関数
   @param[in] key 比較対象1
   @param[in] ent RB木内の各エントリ
   @retval 正  keyが entより前にある
   @retval 負  keyが entより後にある
   @retval 0   keyが entに等しい
 */
static int
_tst_vfs_tstfs_dpage_cmp(struct _tst_vfs_tstfs_dpage *key, 
			struct _tst_vfs_tstfs_dpage *ent){	

	if ( key->index < ent->index )
		return 1;

	if ( ent->index < key->index )
		return -1;

	return 0;	
}

/**
   スーパブロック情報を検索する
   @param[in] devid   デバイス番号
   @param[out] superp スーパブロック情報返却領域
   @retval  0      正常終了
   @retval -ENOENT スーパブロック情報が見つからなかった
 */
static int
tst_vfs_tstfs_superblock_find_nolock(dev_id devid, tst_vfs_tstfs_super **superp){
	int                     rc;
	tst_vfs_tstfs_super *super;
	tst_vfs_tstfs_super    key;

	key.s_devid = devid;

	super = RB_FIND(_tst_vfs_tstfs_super_tree, &g_tstfs_db.supers, &key);

	if ( super == NULL ) {

		rc = -ENOENT;
		goto error_out;
	}

	if ( superp != NULL )
		*superp = super;

	return 0;

error_out:
	return rc;
}

/**
   データページを検索する
   @param[in]  inode  通常ファイルのI-node情報
   @param[in]  offset ファイル先頭からのオフセットアドレス(単位:バイト)
   @param[out] dpagep データページ情報返却領域
   @retval  0      正常終了
   @retval -ENOENT データページが見つからなかった
 */
static int
tst_vfs_tstfs_dpage_find_nolock(tst_vfs_tstfs_inode *inode, off_t offset, 
    tst_vfs_tstfs_dpage **dpagep){
	tst_vfs_tstfs_dpage *dpage;
	tst_vfs_tstfs_dpage    key;

	key.index = offset / PAGE_SIZE;
	dpage = RB_FIND(_tst_vfs_tstfs_dpage_tree, &inode->dpages, &key);
	if ( dpage == NULL )
		return -ENOENT;

	if ( dpagep != NULL )
		*dpagep = dpage;

	return 0;
}

/**
   ディレクトリエントリを検索する
   @param[in]  inode  ディレクトリファイルのI-node情報
   @param[in]  name   ファイル名
   @param[out] dentp  ディレクトリエントリ情報返却領域
   @retval  0      正常終了
   @retval -ENOENT ディレクトリエントリが見つからなかった
 */
static int
tst_vfs_tstfs_dent_find_nolock(tst_vfs_tstfs_inode *inode, const char *name, 
    tst_vfs_tstfs_dent **dentp){
	tst_vfs_tstfs_dent   *dent;
	tst_vfs_tstfs_dent     key;

	strncpy(key.name, name, TST_VFS_TSTFS_FNAME_LEN);
	key.name[TST_VFS_TSTFS_FNAME_LEN - 1]='\0';

	dent = RB_FIND(_tst_vfs_tstfs_dent_tree, &inode->dents, &key);
	if ( dent == NULL )
		return -ENOENT;

	if ( dentp != NULL )
		*dentp = dent;

	return 0;
}

/**
   I-nodeを検索する
   @param[in]  super  スーパブロック情報
   @param[in]  ino    I-node番号
   @param[out] inodep I-node情報返却領域
   @retval  0      正常終了
   @retval -ENOENT I-nodeが見つからなかった
 */
static int
tst_vfs_tstfs_inode_find_nolock(tst_vfs_tstfs_super *super, tst_vfs_tstfs_ino ino, 
    tst_vfs_tstfs_inode **inodep){
	tst_vfs_tstfs_inode *inode;
	tst_vfs_tstfs_inode    key;
	
	key.i_ino = ino;
	inode = RB_FIND(_tst_vfs_tstfs_inode_tree, &super->s_inodes, &key);
	if ( inode == NULL )
		return -ENOENT;

	if ( inodep != NULL )
		*inodep = inode;

	return 0;
}
/**
   ディレクトリエントリを追加する
   @param[in]  inode  ディレクトリファイルのI-node情報
   @param[in]  ino    追加するファイルのI-node番号
   @param[in]  name   追加するファイルのファイル名
   @retval  0      正常終了
   @retval -EBUSY  ディレクトリエントリ内に同じ名前のファイルが存在する
 */
static int
tst_vfs_tstfs_dent_add_nolock(tst_vfs_tstfs_inode *inode, tst_vfs_tstfs_ino ino, 
    const char *name){
	int                     rc;
	tst_vfs_tstfs_dent   *dent;
	tst_vfs_tstfs_dent    *res;

	rc = slab_kmem_cache_alloc(&tstfs_dent, KMALLOC_NORMAL, (void **)&dent);
	if ( rc != 0 ) 
		return rc;

	dent->ino = ino;
	strncpy(dent->name, name, TST_VFS_TSTFS_FNAME_LEN);
	dent->name[TST_VFS_TSTFS_FNAME_LEN - 1] = '\0';

	res = RB_INSERT(_tst_vfs_tstfs_dent_tree, &inode->dents, dent);
	if ( res != NULL ) {

		rc = -EBUSY;
		goto free_dent_out;
	}

	return 0;

free_dent_out:
	slab_kmem_cache_free((void *)dent);  /* ディレクトリエントリを解放    */	
	return rc;
}

/**
   ディレクトリエントリを削除する
   @param[in]  inode  ディレクトリファイルのI-node情報
   @param[in]  name   削除するファイルのファイル名
   @retval  0      正常終了
   @retval -ENOENT ディレクトリエントリ内に指定された名前のファイルが存在しない
 */
static int
tst_vfs_tstfs_dent_del_nolock(tst_vfs_tstfs_inode *inode, const char *name){
	int                     rc;
	tst_vfs_tstfs_dent     key;
	tst_vfs_tstfs_dent   *dent;
	tst_vfs_tstfs_dent    *res;

	strncpy(key.name, name, TST_VFS_TSTFS_FNAME_LEN);
	key.name[TST_VFS_TSTFS_FNAME_LEN - 1] = '\0';

	rc = tst_vfs_tstfs_dent_find_nolock(inode, name, &dent);
	if ( rc != 0 )
		return  -ENOENT;

	res = RB_REMOVE(_tst_vfs_tstfs_dent_tree, &inode->dents, dent);
	kassert( res != NULL );
	kassert( inode->i_size >= sizeof(tst_vfs_tstfs_dent) );
	
	inode->i_size -= sizeof(tst_vfs_tstfs_dent);

	slab_kmem_cache_free((void *)dent);  /* ディレクトリエントリを解放  */

	return 0;
}

/**
   データページを削除する
   @param[in]  inode  通常ファイルのI-node情報
   @param[in]  offset ファイル先頭からのオフセットアドレス(単位:バイト)
   @retval  0      正常終了
   @retval -ENOENT 指定されたオフセットのデータページが存在しない
 */
static int
tst_vfs_tstfs_dpage_free_nolock(tst_vfs_tstfs_inode *inode, off_t offset){
	int                     rc;
	tst_vfs_tstfs_dpage *dpage;
	tst_vfs_tstfs_dpage   *res;

	rc = tst_vfs_tstfs_dpage_find_nolock(inode, offset, &dpage);
	if ( rc != 0 ) 
		return  -ENOENT;

	res = RB_REMOVE(_tst_vfs_tstfs_dpage_tree, &inode->dpages, dpage);
	kassert( res != NULL );

	slab_kmem_cache_free((void *)dpage);  /* データページを解放  */

	return 0;
}

/**
   I-nodeを割り当てる
   @param[in]  super  スーパブロック情報
   @param[out] inodep I-node情報返却領域
   @retval  0      正常終了
   @retval -ENOSPC 空きI-nodeがない
 */
static int
tst_vfs_tstfs_inode_alloc_nolock(tst_vfs_tstfs_super *super, tst_vfs_tstfs_inode **inodep){
	int                     rc;
	tst_vfs_tstfs_inode *inode;
	tst_vfs_tstfs_inode   *res;
	tst_vfs_tstfs_ino  new_ino;

	rc = slab_kmem_cache_alloc(&tstfs_inode, KMALLOC_NORMAL, (void **)&inode);
	if ( rc != 0 ) 
		goto error_out;

	new_ino = bitops_ffc(&super->s_inode_map);
	if ( new_ino == 0 ) {

		rc = -ENOSPC;
		goto error_out;
	}

	mutex_init(&inode->mtx);
	inode->i_ino = new_ino - 1;
	inode->i_mode = VFS_VNODE_MODE_NONE;
	inode->i_nlinks = 1;
	inode->i_size = 0;
	RB_INIT(&inode->dents);
	RB_INIT(&inode->dpages);

	res = RB_INSERT(_tst_vfs_tstfs_inode_tree, &super->s_inodes, inode);
	kassert( res == NULL );

	if ( inodep != NULL )
		*inodep = inode;

	return 0;

error_out:
	return rc;
}

/**
   I-nodeを解放する
   @param[in]  super  スーパブロック情報
   @param[in]  inode  I-node情報
   @retval  0      正常終了
   @retval -ENOENT I-nodeが見つからなかった
 */
static int
tst_vfs_tstfs_inode_free_nolock(tst_vfs_tstfs_super *super, tst_vfs_tstfs_inode *inode){
	int                                       rc;
	tst_vfs_tstfs_inode                     *res;
	tst_vfs_tstfs_dent         *dent, *next_dent;
	off_t                                    off;

	res = RB_REMOVE(_tst_vfs_tstfs_inode_tree, &super->s_inodes, inode);
	if ( res == NULL ) {

		rc = -ENOENT;
		goto error_out;
	}
	bitops_clr(inode->i_ino, &super->s_inode_map);

	if ( ( inode->i_mode & S_IFDIR ) ) {

		/*  ループ内で削除処理を行うのでRB_FOREACH_SAFEを使用  */
		RB_FOREACH_SAFE(dent, _tst_vfs_tstfs_dent_tree, &inode->dents, next_dent) {
			
			rc = tst_vfs_tstfs_dent_del_nolock(inode, dent->name);
			kassert( rc == 0 );
		}
	} else {

		for(off = 0; inode->i_size > off; off += PAGE_SIZE) 
			tst_vfs_tstfs_dpage_free_nolock(inode, off/PAGE_SIZE);
	}

	slab_kmem_cache_free((void *)inode);  /* I-node を解放    */

	return 0;

error_out:
	return rc;
}

/**
   ファイルシステムをマウントする
   @param[in] fs_super  スーパブロック情報
   @param[in] id        マウントポイントID
   @param[in] dev       デバイスID
   @param[in] args      引数情報
   @param[in] root_vnid マウントポイントのv-node ID返却領域
   @retval  0      正常終了
   @retval -ENOENT 指定されたデバイスが見つからない
   @retval -ENOMEM メモリ不足
 */
static int
tst_vfs_tstfs_mount(vfs_fs_super *fs_super, vfs_mnt_id id, dev_id dev, void *args, 
		 vfs_vnode_id *root_vnid){
	int                     rc;
	uint32_t             major;
	tst_vfs_tstfs_super *super;

	major = dev >> 32;
	
	if ( major != TST_VFS_TSTFS_DEV_MAJOR )
		return -ENOENT;

	rc = tst_vfs_tstfs_superblock_alloc(dev, &super);
	if ( rc != 0 )
		return rc;

	*fs_super = (vfs_fs_super)super;
	*root_vnid = (vfs_vnode_id)TST_VFS_TSTFS_ROOT_VNID;

	return 0;
}

/**
   ファイルシステムをアンマウントする
   @param[in] fs_super  スーパブロック情報
   @retval    0         正常終了
 */
static int
tst_vfs_tstfs_unmount(vfs_fs_super fs_super){
	tst_vfs_tstfs_super *super;

	super = (tst_vfs_tstfs_super *)fs_super;
	kassert( super->s_magic == TST_VFS_TSTFS_MAGIC );

	tst_vfs_tstfs_superblock_free(super);

	return 0;
}

/**
   ファイルシステム固有v-node情報を獲得する
   @param[in]  fs_super  スーパブロック情報
   @param[in]  id        v-node ID
   @param[out] modep     v-node のファイル種別
   @param[out] v         ファイルシステム固有v-node情報返却領域
   @retval    0          正常終了
   @retval   -EINVAL     v-node IDが不正
   @retval   -ENOENT     v-node IDに対応するI-nodeが見つからなかった
 */
static int
tst_vfs_tstfs_getvnode(vfs_fs_super fs_super, vfs_vnode_id id, vfs_fs_mode *modep,
    vfs_fs_vnode *v){
	int                     rc;
	tst_vfs_tstfs_super *super;
	tst_vfs_tstfs_ino      ino;
	tst_vfs_tstfs_inode *inode;

	if ( ( 0 > id) || ( id > TST_VFS_NR_INODES ) )
		return -EINVAL;

	super=(tst_vfs_tstfs_super *)fs_super;
	ino = (tst_vfs_tstfs_ino)id; 

	mutex_lock(&super->mtx);
	rc = tst_vfs_tstfs_inode_find_nolock(super, ino, &inode);
	mutex_unlock(&super->mtx);
	if ( rc != 0 )
		return rc;

	*v = (vfs_fs_vnode)inode;
	*modep = inode->i_mode;

	return 0;
}

/**
   ファイルシステム固有v-node情報を返却する
   @param[in]  fs_super  スーパブロック情報
   @param[in]  id        v-node ID
   @param[out] modep     v-node のファイル種別
   @param[out] v         ファイルシステム固有v-node情報返却領域
   @retval    0          正常終了
   @retval   -EINVAL     v-node IDが不正
   @retval   -ENOENT     v-node IDに対応するI-nodeが見つからなかった
 */
static int
tst_vfs_tstfs_putvnode(vfs_fs_super __unused fs_super, vfs_fs_vnode __unused v){

	return 0;
}

/**
   ディレクトリエントリ中のファイル名からv-node IDを取得する
   @param[in]  fs_super  スーパブロック情報
   @param[in]  dir       ディレクトリファイルのファイルシステム固有v-node
   @param[in]  name      ファイル名
   @param[out] idp       v-node番号情報返却領域
   @retval    0          正常終了
   @retval   -EINVAL     スーパブロック情報が不正またはdirがディレクトリを指していない
   @retval   -ENOENT     ファイルが見つからなかった
 */
static int
tst_vfs_tstfs_lookup(vfs_fs_super fs_super, vfs_fs_vnode dir,
		 const char *name, vfs_vnode_id *idp){
	int                     rc;
	tst_vfs_tstfs_super *super;
	tst_vfs_tstfs_inode    *dv;
	tst_vfs_tstfs_dent   *dent;

	super = (tst_vfs_tstfs_super *)fs_super;
	if ( super->s_magic != TST_VFS_TSTFS_MAGIC )
		return -EINVAL;
	
	dv = (tst_vfs_tstfs_inode *)dir;
	if ( !( dv->i_mode & S_IFDIR ) )
		return -EINVAL;

	mutex_lock(&dv->mtx);

	rc = tst_vfs_tstfs_dent_find_nolock(dv, name, &dent);
	if ( rc != 0 )
		goto unlock_out;

	mutex_unlock(&dv->mtx);

	*idp = dent->ino;

	return 0;

unlock_out:
	mutex_unlock(&dv->mtx);
	return rc;
}

/**
   ファイルポジションを更新する
   @param[in]  fs_super  スーパブロック情報
   @param[in]  v         ファイルシステム固有v-node情報
   @param[in]  file_priv ファイルディスクリプタプライベート情報
   @param[in]  pos       変更するオフセット位置(単位:バイト)
   @param[out] new_posp  変更後オフセット位置(単位:バイト)返却領域
   @param[in]  whence    オフセット位置指定
   @retval    0          正常終了
 */
static int
tst_vfs_tstfs_seek(vfs_fs_super fs_super, vfs_fs_vnode v, vfs_file_private file_priv, 
	       off_t pos, off_t *new_posp, int whence){

	return 0;
}

static fs_calls tst_vfs_tstfs_calls={
	.fs_mount = tst_vfs_tstfs_mount,
	.fs_unmount = tst_vfs_tstfs_unmount,
	.fs_getvnode = tst_vfs_tstfs_getvnode,
	.fs_putvnode = tst_vfs_tstfs_putvnode,
	.fs_lookup = tst_vfs_tstfs_lookup,
	.fs_seek = tst_vfs_tstfs_seek,
};

/**
   データページを解放する
   @param[in]  inode  通常ファイルのI-node情報
   @param[in]  offset ファイル先頭からのオフセットアドレス(単位:バイト)
   @retval  0      正常終了
   @retval -EBUSY  指定されたオフセットのデータページが存在する
 */
int
tst_vfs_tstfs_dpage_free(tst_vfs_tstfs_inode *inode, off_t offset){
	int        rc;
	off_t   index;

	index = offset / PAGE_SIZE;

	mutex_lock(&inode->mtx);
	rc = tst_vfs_tstfs_dpage_free_nolock(inode, index);
	mutex_unlock(&inode->mtx);

	return rc;
}

/**
   データページを割り当てる
   @param[in]  inode  通常ファイルのI-node情報
   @param[in]  offset ファイル先頭からのオフセットアドレス(単位:バイト)
   @retval  0      正常終了
   @retval -EBUSY  指定されたオフセットのデータページが存在する
 */
int
tst_vfs_tstfs_dpage_alloc(tst_vfs_tstfs_inode *inode, off_t offset, 
    tst_vfs_tstfs_dpage **dpagep){
	int                     rc;
	tst_vfs_tstfs_dpage *dpage;
	tst_vfs_tstfs_dpage   *res;

	mutex_lock(&inode->mtx);

	rc = slab_kmem_cache_alloc(&tstfs_dpage, KMALLOC_NORMAL, (void **)&dpage);
	if ( rc != 0 ) 
		goto unlock_out;

	rc = pgif_get_free_page(&dpage->page, KMALLOC_NORMAL, PAGE_USAGE_PCACHE);
	if ( rc != 0 )
		goto free_dpage_out;

	dpage->index = offset / PAGE_SIZE;
	res = RB_INSERT(_tst_vfs_tstfs_dpage_tree, &inode->dpages, dpage);
	if ( res != NULL ) {

		rc = -EBUSY;
		goto free_cache_page_out;
	}

	mutex_unlock(&inode->mtx);

	return 0;

free_cache_page_out:
	pgif_free_page(dpage->page);

free_dpage_out:
	slab_kmem_cache_free((void *)dpage);  /* データページを解放    */	

unlock_out:
	mutex_unlock(&inode->mtx);
	return rc;
}

/**
   ディレクトリエントリを追加する
   @param[in]  inode  ディレクトリファイルのI-node情報
   @param[in]  ino    追加するファイルのI-node番号
   @param[in]  name   追加するファイルのファイル名
   @retval  0      正常終了
   @retval -EBUSY  ディレクトリエントリ内に同じ名前のファイルが存在する
 */
int
tst_vfs_tstfs_dent_add(tst_vfs_tstfs_inode *inode, tst_vfs_tstfs_ino ino, const char *name){
	int  rc;

	mutex_lock(&inode->mtx);
	rc = tst_vfs_tstfs_dent_add_nolock(inode, ino, name);
	mutex_unlock(&inode->mtx);

	return rc;
}

/**
   ディレクトリエントリを削除する
   @param[in]  inode  ディレクトリファイルのI-node情報
   @param[in]  name   削除するファイルのファイル名
   @retval  0      正常終了
   @retval -ENOENT ディレクトリエントリ内に指定された名前のファイルが存在しない
 */
int
tst_vfs_tstfs_dent_del(tst_vfs_tstfs_inode *inode, const char *name){
	int                     rc;

	mutex_lock(&inode->mtx);
	rc = tst_vfs_tstfs_dent_del_nolock(inode, name);
	mutex_unlock(&inode->mtx);

	return rc;
}

int
tst_vfs_tstfs_superblock_find(dev_id devid, tst_vfs_tstfs_super **superp){
	int rc;

	mutex_lock(&g_tstfs_db.mtx);
	rc = tst_vfs_tstfs_superblock_find_nolock(devid, superp);
	mutex_unlock(&g_tstfs_db.mtx);

	return rc;
}

int
tst_vfs_tstfs_inode_alloc(tst_vfs_tstfs_super *super, tst_vfs_tstfs_inode **inodep){
	int  rc;

	mutex_lock(&super->mtx);

	rc = tst_vfs_tstfs_inode_alloc_nolock(super, inodep);
	if ( rc != 0 ) 
		goto unlock_out;


	mutex_unlock(&super->mtx);

	return 0;

unlock_out:
	mutex_unlock(&super->mtx);
	return rc;
}

/**
   I-nodeを解放する
   @param[in]  super  スーパブロック情報
   @param[in]  inode  I-node情報
   @retval  0      正常終了
   @retval -ENOENT I-nodeが見つからなかった
 */
int
tst_vfs_tstfs_inode_free(tst_vfs_tstfs_super *super, tst_vfs_tstfs_inode *inode){
	int  rc;

	mutex_lock(&super->mtx);

	rc = tst_vfs_tstfs_inode_free_nolock(super, inode);
	if ( rc != 0 ) 
		goto unlock_out;

	mutex_unlock(&super->mtx);

	return 0;

unlock_out:
	mutex_unlock(&super->mtx);
	return rc;
}
/**
   ファイルシステム固有スーパブロック情報を割り当てる
   @param[in]  devid  デバイスID
   @param[out] superp スーパブロック情報返却領域
   @retval     0      正常終了
 */
int
tst_vfs_tstfs_superblock_alloc(dev_id devid, tst_vfs_tstfs_super **superp){
	int                          rc;
	int                           i;
	tst_vfs_tstfs_super      *super;
	tst_vfs_tstfs_super        *res;
	tst_vfs_tstfs_inode  *dir_inode;

	rc = slab_kmem_cache_alloc(&tstfs_super, KMALLOC_NORMAL, (void **)&super);
	if ( rc != 0 )
		goto unlock_out;

	mutex_init(&super->mtx);
	super->s_devid = devid;
	super->s_magic = TST_VFS_TSTFS_MAGIC;
	super->s_state = TST_VFS_TSTFS_SSTATE_NONE;
	RB_INIT(&super->s_inodes);
	bitops_zero(&super->s_inode_map);
	for(i = 0; TST_VFS_TSTFS_ROOT_VNID > i; ++i) 
		bitops_set(i, &super->s_inode_map);

	/* ルートI-nodeの割当て, ディレクトリエントリの生成 
	 */
	rc = tst_vfs_tstfs_inode_alloc(super, &dir_inode); /* ルートI-node割り当て */
	kassert(dir_inode->i_ino == TST_VFS_TSTFS_ROOT_VNID);
	/*
	 * ".", ".." エントリの追加
	 */
	rc = tst_vfs_tstfs_dent_add(dir_inode, VFS_VNODE_MODE_DIR, ".");
	kassert( rc == 0 );

	rc = tst_vfs_tstfs_dent_add(dir_inode, VFS_VNODE_MODE_DIR, "..");
	kassert( rc == 0 );

	dir_inode->i_mode = VFS_VNODE_MODE_DIR; /* ディレクトリに設定 */
	dir_inode->i_nlinks = 2; /* ディレクトリなのでリンク数を2に初期化 */
	/* "."と".."エントリのサイズを加算 */
	dir_inode->i_size += sizeof(tst_vfs_tstfs_dent) * 2; 
	mutex_lock(&g_tstfs_db.mtx);

	res = RB_INSERT(_tst_vfs_tstfs_super_tree, &g_tstfs_db.supers, super);
	if ( res != NULL ) {

		rc = -EBUSY;
		goto free_super_out;
	}

	mutex_unlock(&g_tstfs_db.mtx);
	if ( superp != NULL )
		*superp = super;

	return 0;

free_super_out:
	slab_kmem_cache_free((void *)super);  /* super blockを解放    */

unlock_out:
	mutex_unlock(&g_tstfs_db.mtx);
	return rc;
}
void
tst_vfs_tstfs_superblock_free(tst_vfs_tstfs_super *super){
	int                      rc;
	tst_vfs_tstfs_inode  *inode;
	tst_vfs_tstfs_inode   *next;

	mutex_lock(&super->mtx);
	/*  ループ内で削除処理を行うのでRB_FOREACH_SAFEを使用  */
	RB_FOREACH_SAFE(inode, _tst_vfs_tstfs_inode_tree, &super->s_inodes, next) {

		mutex_lock(&inode->mtx);

		rc = tst_vfs_tstfs_inode_free_nolock(super, inode);
		kassert( rc == 0 );

		mutex_unlock(&inode->mtx);
	}
	mutex_unlock(&super->mtx);
	slab_kmem_cache_free((void *)super);  /* super blockを解放    */
}


void
tst_vfs_tstfs_init(void){
	int rc;

	if ( !g_tstfs_db.initialized ) {

		/* テスト用ファイルシステムsuperblock用SLABキャッシュの初期化 */

		rc = slab_kmem_cache_create(&tstfs_super, "test file system superblock", 
		    sizeof(tst_vfs_tstfs_super), SLAB_ALIGN_NONE, 0, KMALLOC_NORMAL,
		    NULL, NULL);
		kassert( rc == 0 );

		/* テスト用ファイルシステムI-node用SLABキャッシュの初期化 */
		rc = slab_kmem_cache_create(&tstfs_inode, "test file system I-node", 
		    sizeof(tst_vfs_tstfs_inode), SLAB_ALIGN_NONE,  0, KMALLOC_NORMAL,
		    NULL, NULL);
		kassert( rc == 0 );

		/* テスト用ファイルシステムディレクトリエントリ用SLABキャッシュの初期化 */
		rc = slab_kmem_cache_create(&tstfs_dent, "test file system directory entries",
		    sizeof(tst_vfs_tstfs_dent), SLAB_ALIGN_NONE,  0, KMALLOC_NORMAL,
		    NULL, NULL);
		kassert( rc == 0 );

		/* テスト用ファイルシステムデータページ用SLABキャッシュの初期化 */
		rc = slab_kmem_cache_create(&tstfs_dpage, "test file system data pages", 
		    sizeof(tst_vfs_tstfs_dpage), SLAB_ALIGN_NONE,  0, KMALLOC_NORMAL,
		    NULL, NULL);
		kassert( rc == 0 );
		rc = vfs_register_filesystem(TST_VFS_TSTFS_NAME, &tst_vfs_tstfs_calls);
		kassert( rc == 0 );
		g_tstfs_db.initialized = true;
	}
}

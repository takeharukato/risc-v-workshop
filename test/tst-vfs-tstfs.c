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

static tst_vfs_tstfs_db g_tstfs_db={.mtx = __MUTEX_INITIALIZER(&g_tstfs_db.mtx),\
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

int
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

int
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

int
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

int
tst_vfs_tstfs_inode_find_nolock(tst_vfs_tstfs_super *super, tst_vfs_tstfs_ino ino, 
    tst_vfs_tstfs_inode **inodep){
	tst_vfs_tstfs_inode *inode;
	tst_vfs_tstfs_inode    key;

	inode = RB_FIND(_tst_vfs_tstfs_inode_tree, &super->s_inodes, &key);
	if ( inode == NULL )
		return -ENOENT;

	if ( inodep != NULL )
		*inodep = inode;

	return 0;
}

int
tst_vfs_tstfs_dent_add(tst_vfs_tstfs_inode *inode, tst_vfs_tstfs_ino ino, const char *name){
	int                     rc;
	tst_vfs_tstfs_dent   *dent;
	tst_vfs_tstfs_dent    *res;

	mutex_lock(&inode->mtx);

	rc = slab_kmem_cache_alloc(&tstfs_dent, KMALLOC_NORMAL, (void **)&dent);
	if ( rc != 0 ) 
		goto unlock_out;

	dent->ino = ino;
	strncpy(dent->name, name, TST_VFS_TSTFS_FNAME_LEN);
	dent->name[TST_VFS_TSTFS_FNAME_LEN - 1] = '\0';

	res = RB_INSERT(_tst_vfs_tstfs_dent_tree, &inode->dents, dent);
	if ( res != NULL ) {

		rc = -EBUSY;
		goto free_dent_out;
	}

	mutex_unlock(&inode->mtx);

	return 0;

free_dent_out:
	slab_kmem_cache_free((void *)dent);  /* ディレクトリエントリを解放    */	

unlock_out:
	mutex_unlock(&inode->mtx);
	return rc;
}

int
tst_vfs_tstfs_dent_del_nolock(tst_vfs_tstfs_inode *inode, const char *name){
	tst_vfs_tstfs_dent     key;
	tst_vfs_tstfs_dent   *dent;

	strncpy(key.name, name, TST_VFS_TSTFS_FNAME_LEN);
	key.name[TST_VFS_TSTFS_FNAME_LEN - 1] = '\0';

	dent = RB_REMOVE(_tst_vfs_tstfs_dent_tree, &inode->dents, &key);
	if ( dent == NULL ) 
		return  -ENOENT;

	slab_kmem_cache_free((void *)dent);  /* ディレクトリエントリを解放  */

	return 0;
}

int
tst_vfs_tstfs_dpage_free_nolock(tst_vfs_tstfs_inode *inode, off_t index){
	tst_vfs_tstfs_dpage    key;
	tst_vfs_tstfs_dpage *dpage;

	key.index = index;

	dpage = RB_REMOVE(_tst_vfs_tstfs_dpage_tree, &inode->dpages, &key);
	if ( dpage == NULL ) 
		return  -ENOENT;

	slab_kmem_cache_free((void *)dpage);  /* データページを解放  */

	return 0;
}

int
tst_vfs_tstfs_dpage_alloc(tst_vfs_tstfs_inode *inode, off_t index, 
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

	dpage->index = index;
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

int
tst_vfs_tstfs_dpage_free(tst_vfs_tstfs_inode *inode, off_t index){
	int                     rc;

	mutex_lock(&inode->mtx);
	rc = tst_vfs_tstfs_dpage_free_nolock(inode, index);
	mutex_unlock(&inode->mtx);

	return rc;
}

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
	int                     rc;
	tst_vfs_tstfs_inode *inode;
	tst_vfs_tstfs_inode   *res;
	tst_vfs_tstfs_ino  new_ino;

	mutex_lock(&super->mtx);

	rc = slab_kmem_cache_alloc(&tstfs_inode, KMALLOC_NORMAL, (void **)&inode);
	if ( rc != 0 ) 
		goto unlock_out;

	new_ino = bitops_ffs(&super->s_inode_map);

	mutex_init(&inode->mtx);
	inode->i_ino = new_ino;
	inode->i_mode = VFS_VNODE_MODE_NONE;
	inode->i_nlinks = 1;
	inode->i_size = 0;

	res = RB_INSERT(_tst_vfs_tstfs_inode_tree, &super->s_inodes, inode);
	kassert( res == NULL );

	if ( inodep != NULL )
		*inodep = inode;

	mutex_unlock(&super->mtx);

	return 0;

unlock_out:
	mutex_unlock(&super->mtx);
	return rc;
}

int
tst_vfs_tstfs_inode_free_nolock(tst_vfs_tstfs_super *super, tst_vfs_tstfs_inode *inode){
	int                                       rc;
	tst_vfs_tstfs_inode                     *res;
	tst_vfs_tstfs_dent         *dent, *next_dent;
	off_t                                    off;

	res = RB_REMOVE(_tst_vfs_tstfs_inode_tree, &super->s_inodes, inode);
	if ( res == NULL ) {

		rc = -ENOENT;
		goto unlock_out;
	}
	bitops_clr(inode->i_ino, &super->s_inode_map);

	mutex_lock(&inode->mtx);

	/*  ループ内で削除処理を行うのでRB_FOREACH_SAFEを使用  */
	RB_FOREACH_SAFE(dent, _tst_vfs_tstfs_dent_tree, &inode->dents, next_dent) {

		rc = tst_vfs_tstfs_dent_del_nolock(inode, dent->name);
		kassert( rc == 0 );
	}

	for(off = 0; inode->i_size > off; off += PAGE_SIZE) 
		tst_vfs_tstfs_dpage_free_nolock(inode, off/PAGE_SIZE);

	mutex_unlock(&inode->mtx);

	slab_kmem_cache_free((void *)inode);  /* I-node を解放    */

	return 0;

unlock_out:
	mutex_unlock(&super->mtx);
	return rc;
}

int
tst_vfs_tstfs_superblock_alloc(dev_id devid){
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
	dir_inode->i_mode = VFS_VNODE_MODE_DIR;

	/*
	 * ".", ".." エントリの追加
	 */
	rc = tst_vfs_tstfs_dent_add(dir_inode, VFS_VNODE_MODE_DIR, ".");
	kassert( rc == 0 );

	rc = tst_vfs_tstfs_dent_add(dir_inode, VFS_VNODE_MODE_DIR, "..");
	kassert( rc == 0 );

	mutex_lock(&g_tstfs_db.mtx);

	rc = tst_vfs_tstfs_superblock_find_nolock(devid, NULL);
	res = RB_INSERT(_tst_vfs_tstfs_super_tree, &g_tstfs_db.supers, super);
	if ( res != NULL ) {

		rc = -EBUSY;
		goto free_super_out;
	}

	mutex_unlock(&g_tstfs_db.mtx);
	
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

		rc = tst_vfs_tstfs_inode_free_nolock(super, inode);
		kassert( rc == 0 );
	}
	mutex_unlock(&super->mtx);
	slab_kmem_cache_free((void *)super);  /* super blockを解放    */
}


void
tst_vfs_tstfs_init(void){
	int rc;

	/* テスト用ファイルシステムsuperblock用SLABキャッシュの初期化 */
	rc = slab_kmem_cache_create(&tstfs_super, "test file system superblock", 
	    sizeof(tst_vfs_tstfs_super), SLAB_ALIGN_NONE,  0, KMALLOC_NORMAL, NULL, NULL);
	kassert( rc == 0 );

	/* テスト用ファイルシステムI-node用SLABキャッシュの初期化 */
	rc = slab_kmem_cache_create(&tstfs_inode, "test file system I-node", 
	    sizeof(tst_vfs_tstfs_inode), SLAB_ALIGN_NONE,  0, KMALLOC_NORMAL, NULL, NULL);
	kassert( rc == 0 );

	/* テスト用ファイルシステムディレクトリエントリ用SLABキャッシュの初期化 */
	rc = slab_kmem_cache_create(&tstfs_dent, "test file system directory entries", 
	    sizeof(tst_vfs_tstfs_dent), SLAB_ALIGN_NONE,  0, KMALLOC_NORMAL, NULL, NULL);
	kassert( rc == 0 );
	
	/* テスト用ファイルシステムデータページ用SLABキャッシュの初期化 */
	rc = slab_kmem_cache_create(&tstfs_dpage, "test file system data pages", 
	    sizeof(tst_vfs_tstfs_dpage), SLAB_ALIGN_NONE,  0, KMALLOC_NORMAL, NULL, NULL);
	kassert( rc == 0 );
}

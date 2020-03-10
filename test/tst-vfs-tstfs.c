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

int
tst_vfs_tstfs_superblock_find(dev_id devid, tst_vfs_tstfs_super **superp){
	int rc;

	mutex_lock(&g_tstfs_db.mtx);
	rc = tst_vfs_tstfs_superblock_find_nolock(devid, superp);
	mutex_unlock(&g_tstfs_db.mtx);

	return rc;
}

int
tst_vfs_tstfs_superblock_alloc(dev_id devid){
	int rc;
	int                      i;
	tst_vfs_tstfs_super *super;
	tst_vfs_tstfs_super   *res;

	rc = slab_kmem_cache_alloc(&tstfs_super, KMALLOC_NORMAL, (void **)&super);
	if ( rc != 0 )
		goto unlock_out;

	super->s_devid = devid;
	super->s_magic = TST_VFS_TSTFS_MAGIC;
	super->s_state = TST_VFS_TSTFS_SSTATE_NONE;
	RB_INIT(&super->s_inodes);
	bitops_zero(&super->s_inode_map);
	for(i = 0; TST_VFS_TSTFS_ROOT_VNID > i; ++i) 
		bitops_set(i, &super->s_inode_map);

	bitops_set(TST_VFS_TSTFS_ROOT_VNID, &super->s_inode_map);
	/* TODO: ルートI-nodeの割当て, ディレクトリエントリの生成 */

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

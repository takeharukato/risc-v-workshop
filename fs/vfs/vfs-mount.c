/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Mount Table Routines                                              */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>

#include <kern/spinlock.h>
#include <kern/wqueue.h>
#include <kern/mutex.h>
#include <kern/vfs-if.h>
#include <kern/page-if.h>

/** グローバルマウントテーブル */
static mount_table   g_mnttbl = __MNTTBL_INITIALIZER(&g_mnttbl);
static kmem_cache fs_mount_cache; /**< マウントポイントのSLABキャッシュ */
static kmem_cache    vnode_cache; /**< v-nodeのSLABキャッシュ */
/**
   マウントポイント比較処理
 */
static int _fs_mount_cmp(struct _fs_mount *_key, struct _fs_mount *_ent);
RB_GENERATE_STATIC(_fs_mount_tree, _fs_mount, m_ent, _fs_mount_cmp);

/**
   v-node比較処理
 */
static int _vnode_cmp(struct _vnode *_key, struct _vnode *_ent);
RB_GENERATE_STATIC(_vnode_tree, _vnode, v_vntbl_ent, _vnode_cmp);

/** 
    マウントポイント比較関数
    @param[in] key 比較対象マウントポイント
    @param[in] ent マウントポイントテーブル内の各エントリ
    @retval 正  keyのm_idが entのm_idより前にある
    @retval 負  keyのm_idが entのm_idより後にある
    @retval 0   keyのm_idが entのm_idに等しい
 */
static int 
_fs_mount_cmp(struct _fs_mount *key, struct _fs_mount *ent){
	
	if ( key->m_id < ent->m_id )
		return 1;

	if ( key->m_id > ent->m_id )
		return -1;

	return 0;
}

/** 
    v-node比較関数
    @param[in] key 比較対象v-node
    @param[in] ent マウントポイント内の各v-nodeエントリ
    @retval 正  keyのm_id, v_idが entのm_id, v_idより前にある
    @retval 負  keyのm_id, v_idが entのm_id, v_idより後にある
    @retval 0   keyのm_id, v_idが entのm_id, v_idに等しい
 */
static int 
_vnode_cmp(struct _vnode *key, struct _vnode *ent){
	
	if ( key->v_mount->m_id < ent->v_mount->m_id )
		return 1;

	if ( key->v_mount->m_id > ent->v_mount->m_id )
		return -1;

	if ( key->v_id < ent->v_id )
		return 1;

	if ( key->v_id > ent->v_id )
		return -1;
	
	return 0;
}

/**
   マウント情報の初期化 (内部関数)
   @param[in] mount 初期化するマウント情報
   @param[in] path  マウントポイントパス
   @param[in] fs    ファイルシステム情報
   @retval  0      正常終了
   @retval -ENOMEM メモリ不足
*/
static int
init_mount(fs_mount *mount, char *path, fs_container *fs) {
	int        rc;

	memset(mount, 0, sizeof(fs_mount));  /*  ゼロクリア */

	mutex_init(&mount->m_mtx);       /*  マウントテーブル排他用mutexの初期化     */
	refcnt_init(&mount->m_refs);     /*  参照カウンタの初期化                    */
	mount->m_id = VFS_INVALID_MNTID; /*  マウントIDの初期化                      */
	mount->m_dev = FS_INVALID_DEVID; /*  デバイスIDの初期化                      */
	RB_INIT(&mount->m_head);         /*  マウントテーブル内のv-nodeリストの初期化 */
	mount->m_fs_super = NULL;        /*  ファイルシステム固有情報の初期化        */
	mount->m_fs = fs;                /*  ファイルシステム情報の初期化            */
	mount->m_mnttbl = NULL;          /*  登録先マウントテーブルの初期化          */
	mount->m_root = NULL;            /*  ルートv-nodeの初期化                     */
	mount->m_mount_point = NULL;     /*  マウントポイント文字列の初期化          */
	mount->m_mount_flags = VFS_MNT_MNTFLAGS_NONE;  /*  マウントフラグの初期化           */
	mount->m_mount_path = kstrdup(path);    /*  マウントポイントパスの複製       */

	rc = -ENOMEM;
	if ( mount->m_mount_path == NULL )
		goto error_out;

	return 0;

error_out:
	return rc;
}

/**
   マウント情報を割り当てる (内部関数)
   @param[in] fstbl   ファイルシステムテーブル
   @param[in] mnttbl  マウントテーブル
   @param[in] path    マウントポイントパス文字列
   @param[in] fs_name ファイルシステム名
   @param[out] mntp   マウント情報を指すポインタのアドレス
   @retval  0      正常終了
   @retval -ENOMEM メモリ不足
   @retval -ENOENT ファイルシステムが見つからなかった
 */
static int __unused
alloc_new_fsmount(char *path, const char *fs_name, fs_mount **mntp){
	fs_mount         *new_mount;
	fs_container            *fs;
	int                  rc = 0;

	/*
	 * 指定されたファイルシステムのファイルシステム情報を探索
	 */
	vfs_fs_get(fs_name, &fs);
	if ( fs == NULL ) {  /*  ファイルシステム情報が見つからなかった */

		rc = -ENOENT;
		goto error_out;
	}

	rc = slab_kmem_cache_alloc(&fs_mount_cache, KMALLOC_NORMAL,
	    (void **)&new_mount);
	if ( rc != 0 ) {

		rc = -ENOMEM;
		goto put_fs_out;  /* メモリ不足 */
	}

	rc = init_mount(new_mount, path, fs); /* マウント情報の初期化 */
	if ( rc != 0 ) {

		kassert( rc == -ENOMEM );
		goto free_mount_out;
	}

	kassert( is_valid_fs_calls( new_mount->m_fs->c_calls ) );

	*mntp = new_mount;  /*  マウント情報を返却  */

	return 0;

free_mount_out:
	slab_kmem_cache_free(new_mount); /* プロセス情報を解放する */	

put_fs_out:
	vfs_fs_put(fs);  /* ファイルシステムへの参照を返却する */

error_out:
	return rc;
}

/**
   マウント情報を解放する (内部関数)
   @param[in] mount マウント情報
   @pre ファイルシステムへの参照を獲得済みであること
*/
static void 
free_fsmount(fs_mount *mount) {

	kassert( mount->m_fs != NULL );

	vfs_fs_put(mount->m_fs);      /*  ファイルシステムへの参照を解放  */
	/*  対象のマウント情報を待ち合わせているスレッドを起床 */
	mutex_destroy(&mount->m_mtx); 
	kfree(mount->m_mount_path);  /*  マウントポイント文字列を解放  */
	slab_kmem_cache_free(mount); /*  マウント情報を解放  */
}
/**
   マウントIDを割り当てる
   @param[out] m_idp   マウントID返却領域
   @retval     0       正常終了
   @retval    -ENOSPC  マウントIDに空きがない
 */
static int
alloc_new_mntid_nolock(vfs_mnt_id *idp){
	vfs_mnt_id  new_id;
	fs_mount  *cur_mnt;
	fs_mount       key;

	for(new_id = g_mnttbl.mt_last_id + 1; new_id != g_mnttbl.mt_last_id; ++new_id) {

		if ( new_id == VFS_INVALID_MNTID )
			continue;  /* 無効なIDを飛ばす */

		key.m_id = new_id;  /* 検索対象マウントID */
		/* マウント情報を検索 */
		cur_mnt = RB_FIND(_fs_mount_tree, &g_mnttbl.mt_head, &key); 
		if ( cur_mnt == NULL ) { /* 空きIDを検出 */

			*idp = new_id; /* IDを返却 */
			g_mnttbl.mt_last_id = new_id; /* 最後に検出したIDを記録 */
			return 0;
		}
	}

	return -ENOSPC;
}

/**
   マウントIDを解放する
   @param[in] id   マウントID返却領域
   @retval     0       正常終了
   @retval    -EBUSY   対象のマウントポイントが登録されている
 */
static int
free_mntid_nolock(vfs_mnt_id id){
	fs_mount  *cur_mnt;
	fs_mount       key;

	kassert( id != VFS_INVALID_MNTID ); /* 有効なマウントポイントIDを指定する */

	key.m_id = id;  /* 検索対象マウントID */
	cur_mnt = RB_FIND(_fs_mount_tree, &g_mnttbl.mt_head, &key); 
	if ( cur_mnt != NULL )   /* 登録済みのマウント情報 */
		return -EBUSY;

	/* 最後に割り当てたIDと等しい場合は次回の検索IDを再設定する */
	if ( g_mnttbl.mt_last_id == id )
		g_mnttbl.mt_last_id = id - 1; 
	
	return 0;
}

/**
    マウント情報をマウントテーブルに追加する (内部関数)
    @param[in] mount  マウント情報
    @retval    0      正常終了
    @retval   -ENOSPC マウントIDに空きがない
 */
static __unused int
add_fsmount_to_mnttbl(fs_mount *mount) {
	int             rc;
	vfs_mnt_id  new_id;
	fs_mount  *cur_mnt;

	mutex_lock(&g_mnttbl.mt_mtx);
	rc = alloc_new_mntid_nolock(&new_id); /* マウントIDの割り当て */
	if ( rc != 0 ) 
		goto unlock_out;

	mount->m_id = new_id;
	cur_mnt = RB_INSERT(_fs_mount_tree, &g_mnttbl.mt_head, mount); 
	kassert( cur_mnt == NULL );  /* マウント情報の多重登録 */

	mutex_unlock(&g_mnttbl.mt_mtx);

	return 0;

unlock_out:
	mutex_unlock(&g_mnttbl.mt_mtx);

	return rc;
}

/**
   マウント情報をマウントテーブルから削除する (内部関数)
   @param[in] mount  マウント情報
 */
static void
remove_fs_mount_from_mnttbl_nolock(fs_mount *mount) {
	fs_mount  *cur_mnt;

	/* マウント情報を削除  */
	cur_mnt = RB_REMOVE(_fs_mount_tree, &g_mnttbl.mt_head, mount); 
	kassert( cur_mnt != NULL );  /* マウント情報の多重解放 */

	free_mntid_nolock(mount->m_id);          /* マウントIDを解放      */
	mount->m_id = VFS_INVALID_MNTID;         /* 無効マウントIDを設定  */
}

/** 
    マウント情報にv-nodeを追加 (実処理関数)
    @param[in] mount  v-node格納先ボリュームのマウント情報
    @param[in] v      追加するv-node
    @retval    0      正常終了
    @retval    -EBUSY  アンマウント中
 */
static int
add_vnode_to_mount_nolock(fs_mount *mount, vnode *v){
	vnode *cur_v;

	if ( mount->m_mount_flags & VFS_MNT_UNMOUNTING ) /* アンマウント中は登録不能 */
		return -EBUSY;

	cur_v = RB_INSERT(_vnode_tree, &mount->m_head, v); 
	kassert( cur_v == NULL );  /* v-nodeの多重登録 */
	v->v_mount = mount;

	vfs_fs_mount_ref_inc(mount);  /* v-nodeからの参照を加算 */

	return 0;
}

/**
   v-nodeを使用中にする (内部関数)
   @param[in] v 操作対象のv-node
 */
static void
mark_busy_vnode_nolock(vnode *v) {

	v->v_flags |= VFS_VFLAGS_BUSY;      /*  使用中に設定する */	
}

/**
   v-nodeの使用中フラグを落とす (内部関数)
   @param[in] v 操作対象のv-node
 */
static void
unmark_busy_vnode_nolock(vnode *v) {

	v->v_flags &= ~VFS_VFLAGS_BUSY;      /*  使用中に設定する */	
	if ( !wque_is_empty(&v->v_waiters) )   /*  v-nodeを待っているスレッドを起床 */
		wque_wakeup(&v->v_waiters, WQUE_RELEASED); 
}

/**
   v-nodeをロード済みにする (内部関数)
   @param[in] v 操作対象のv-node
 */
static __unused void
mark_valid_vnode_nolock(vnode *v) {

	v->v_flags |= VFS_VFLAGS_VALID;      /*  ロード済みに設定する */	
}

/**
   v-nodeのロード済みフラグを落とす (内部関数)
   @param[in] v 操作対象のv-node
 */
static __unused void
unmark_valid_vnode_nolock(vnode *v) {

	v->v_flags &= ~VFS_VFLAGS_VALID;      /*  有効ビットを落とす */	
}

/**
   v-nodeがロード済みであることを確認する (内部関数)
   @param[in] v 操作対象のv-node
 */
static int
is_valid_vnode_nolock(vnode *v) {
	
	return ( v->v_flags & ( VFS_VFLAGS_VALID | VFS_VFLAGS_DIRTY ) );  
}

/**
   v-nodeを更新済みにする (内部関数)
   @param[in] v 操作対象のv-node
 */
static __unused void
mark_dirty_vnode_nolock(vnode *v) {

	v->v_flags |= VFS_VFLAGS_DIRTY;      /*  更新済みに設定する */	
}

/**
   v-nodeの更新済みフラグを落とす (内部関数)
   @param[in] v 操作対象のv-node
 */
static __unused void
unmark_dirty_vnode_nolock(vnode *v) {

	v->v_flags &= ~VFS_VFLAGS_DIRTY;      /*  更新済みビットを落とす */	
}

/**
   v-nodeが更新されていることを確認する (内部関数)
   @param[in] v 操作対象のv-node
 */
static __unused int
is_dirty_vnode_nolock(vnode *v) {
	
	return ( v->v_flags & VFS_VFLAGS_DIRTY );  
}

/**
   v-nodeが使用中であることを確認する (内部関数)
   @param[in] v 操作対象のv-node
 */
static __unused int
is_busy_vnode_nolock(vnode *v) {
	
	return ( v->v_flags & VFS_VFLAGS_BUSY );  
}

/**
   v-nodeの削除フラグをセットする (内部関数)
   @param[in] v 操作対象のv-node
 */
static __unused void
mark_delete_vnode_nolock(vnode *v) {

	v->v_flags |= VFS_VFLAGS_DELETE;      /*  Close on Execに設定する */	
}

/**
   v-nodeの削除フラグをクリアする (内部関数)
   @param[in] v 操作対象のv-node
 */
static __unused void
unmark_delete_vnode_nolock(vnode *v) {

	v->v_flags &= ~VFS_VFLAGS_DELETE;      /*  Close on Execビットを落とす */	
}

/**
   削除対象v-nodeであることを確認する (内部関数)
   @param[in] v 操作対象のv-node
 */
static __unused int
is_deleted_vnode_nolock(vnode *v) {
	
	return ( v->v_flags & VFS_VFLAGS_DELETE );  
}

/**
   v-nodeの初期化
   @param[in] v 初期化対象のv-node
 */
static void
init_vnode(vnode *v){

	/*
	 * メンバを初期化
	 */
	mutex_init(&v->v_mtx);               /*  状態更新用mutexの初期化                    */
	refcnt_init(&v->v_refs);             /*  参照カウンタの初期化                       */
	v->v_fs_vnode = NULL;                /*  ファイルシステム固有v-nodeの初期化         */
	wque_init_wait_queue(&v->v_waiters); /*  v-nodeウエイトキューの初期化               */
	v->v_id = VFS_INVALID_VNID;          /*  vnidの初期化                               */
	v->v_mount = NULL;                   /*  マウント情報の初期化                       */
	v->v_mode = VFS_VNODE_MODE_NONE;     /*  ファイルモードの初期化                     */
	mark_busy_vnode_nolock(v);           /*  使用中に設定する                           */ 

	return ;
}

/**
   v-nodeを新規に生成する (内部関数)
   @param[in] mnt マウントポイント情報
   @return 新設したv-node
   @return NULL メモリ不足
 */
static vnode *
alloc_new_vnode(fs_mount *mnt){
	int           rc;
	vnode *new_vnode;

	rc = slab_kmem_cache_alloc(&vnode_cache, KMALLOC_NORMAL,
	    (void **)&new_vnode);
	if ( rc != 0 ) {

		rc = -ENOMEM;
		goto error_out;  /* メモリ不足 */
	}

	init_vnode(new_vnode);   /* v-nodeを初期化  */

	new_vnode->v_mount = mnt;  /* マウントポイント情報を設定 */

	return new_vnode;  /* 獲得したv-nodeを返却 */

error_out:
	return NULL;
}

/**
   参照されていないv-nodeを開放する (実処理関数)
   @param[in] v 操作対象のv-node
   @pre v-nodeテーブルに登録済みのv-nodeであること
   @pre マウントテーブルに登録済みのv-nodeであること
   @pre v-nodeを参照しているスレッドが他にいないこと
 */
static __unused void
release_vnode(vnode *v){

	kassert( v->v_mount != NULL );  /* マウントポイントに登録済み */
	kassert( v->v_mount->m_fs != NULL );
	kassert( is_valid_fs_calls( v->v_mount->m_fs->c_calls ) );
	kassert( refcnt_read(&v->v_refs) == 0 );

	/* 
	 * TODO: vm_cacheを実装した場合は, 本関数の最初にvm_cacheを開放すること
	 */

	/*
	 * ファイルシステム固有のremove/putvnode操作を呼出し
	 *  実ファイルシステム上のv-node(disk inode)を削除する場合 
	 *    fs_removevnodeを呼び出す
	 *  実ファイルシステム上のv-node(disk inode)をv-nodeの内容で更新する場合, または, 
	 *    fs_removevnodeが定義されていない場合fs_putvnodeを呼び出す
	 */
	if ( ( v->v_flags & VFS_VFLAGS_DELETE ) &&
	    ( v->v_mount->m_fs->c_calls->fs_removevnode != NULL ) )
		v->v_mount->m_fs->c_calls->fs_removevnode(v->v_mount->m_fs_super, v->v_fs_vnode);
	else 
		v->v_mount->m_fs->c_calls->fs_putvnode(v->v_mount->m_fs_super, v->v_fs_vnode);

	vfs_vnode_ref_dec(v);  /* v-nodeを解放する */
}

/**
   vnodeのロックを試みる (内部関数)
   @param[in] v 操作対象のvnode
   @retval  0      正常終了
   @retval -EBUSY  他スレッドがロックを獲得中
 */
static int
trylock_vnode(vnode *v) {

	if ( is_busy_vnode_nolock(v) )
		return -EBUSY;

	mark_busy_vnode_nolock(v);   /*  使用中に更新  */

	return 0;
}

/**
   mntid, vnidをキーとしてv-nodeを検索する (実処理関数) 
   @param[in] mnt   マウントポイント情報
   @param[in] vnid  v-node ID
   @param[out] vpp  v-nodeを指し示すポインタのアドレス
   @retval  0       正常終了
   @return -EINVAL  不正なマウントポイントIDを指定した
   @return -ENOENT  指定されたfsid, vnidに対応するv-nodeが見つからなかった
 */
static int
find_vnode(fs_mount *mnt, vfs_vnode_id vnid, vnode **outv){
	int             rc;
	vnode           *v;
	vnode        v_key;
	wque_reason reason;

	v_key.v_id = vnid;  /* 検索対象v-node ID */
	for( ; ; ) {

		mutex_lock(&mnt->m_mtx);  /* マウントポイントのロックを獲得 */

		/*
		 * 指定されたvnode IDに対応するvnodeを指定されたマウントポイントから検索
		 */
		v = RB_FIND(_vnode_tree, &mnt->m_head, &v_key);
		if ( v == NULL ) {  /* v-nodeが登録されていない */
			
			/*
			 * v-nodeを割当て, 実ファイルシステムからディスクI-nodeを読み込む
			 */
			v = alloc_new_vnode(mnt);
			if ( v == NULL ) {

				rc = -ENOMEM;
				goto unlock_out;
			}

			/*
			 * ファイルシステム固有のvnode情報を割り当てる
			 */
			rc = v->v_mount->m_fs->c_calls->fs_getvnode(v->v_mount->m_fs_super, 
								    vnid, &v->v_fs_vnode);

			if ( rc != 0 )
				goto unlock_out;  /* ディスクI-nodeの読み取りに失敗した */

			if ( v->v_fs_vnode == NULL ) {
		
				/*  下位のファイルシステムがv-nodeを割り当てられなかった */
				rc = -ENOENT;
				goto unlock_out;
			}

			v->v_id = vnid;      /* v-node IDを設定 */
			mark_valid_vnode_nolock(v); /* v-nodeをロード済みに設定する */

			rc = add_vnode_to_mount_nolock(v->v_mount, v); /* v-nodeを登録する */

			unmark_busy_vnode_nolock(v);  /* v-nodeのロックを解除(BUSYを解除)  */
			if ( rc != 0 )
				goto unlock_out;  /* v-nodeを登録できなかった */
		}

		kassert( v != NULL );
		kassert( is_valid_vnode_nolock(v) );

		if ( !is_busy_vnode_nolock(v) ) {
		
			/* 他に対象のvnodeを更新しているスレッドいなければ
			 * 見つかったvnodeを返却する
			 */
			if ( outv != NULL ) {

				*outv = v;             /* v-nodeを返却               */
				/* マウントポイントのロックを解放 */
				mutex_unlock(&mnt->m_mtx);
				break;
			}
		}

		/*  vnodeの更新完了を待ち合わせる */
		reason = wque_wait_on_event_with_mutex(&v->v_waiters, &mnt->m_mtx);
		if ( reason == WQUE_DESTROYED ) {

			rc = -ENOENT;   /* v-nodeが見つからなかった */
			goto unlock_out;
		}

		mutex_unlock(&mnt->m_mtx);  /* マウントポイントのロックを解放 */
	}

	return 0;

unlock_out:
	mutex_unlock(&mnt->m_mtx);  /* マウントポイントのロックを解放 */

	return rc;
}

/*
 * 外部IF
 */

/**
   v-nodeの参照カウンタをインクリメントする
   @param[in] v  v-node情報
   @retval    真 v-nodeの参照を獲得できた
   @retval    偽 v-nodeの参照を獲得できなかった
 */
bool
vfs_vnode_ref_inc(vnode *v) {

	/* v-node解放中でなければ, 利用カウンタを加算し, 加算前の値を返す  
	 */
	return ( refcnt_inc_if_valid(&v->v_refs) != 0 ); 
}

/**
   v-nodeの参照カウンタをデクリメントする
   @param[in] v  v-node情報
   @retval    真 v-nodeの最終参照者だった
   @retval    偽 v-nodeの最終参照者でない
 */
bool
vfs_vnode_ref_dec(vnode *v){
	bool     res;
	vnode *res_v;

	kassert( v->v_mount != NULL ); /* v-nodeテーブルに登録されていることを確認 */

	/*  マウントポイントの参照カウンタをさげる  */
	res = refcnt_dec_and_mutex_lock(&v->v_refs, &v->v_mount->m_mtx);
	if ( res ) { /* マウントポイントの最終参照者だった場合 */
		
		/* v-nodeをv-nodeテーブルから外す */
		res_v = RB_REMOVE(_vnode_tree, &v->v_mount->m_head, v); 
		kassert( res_v != NULL );  /* v-nodeの多重解放 */

		mutex_unlock(&v->v_mount->m_mtx);  /* v-nodeテーブルをアンロック  */
		
		vfs_fs_mount_ref_dec(v->v_mount);  /* v-nodeからの参照を減算 */
		v->v_mount = NULL;

		/*
		 *  v-nodeを解放する
		 */
		mutex_destroy(&v->v_mtx);    /* 待ちスレッドを起床  */
		wque_wakeup(&v->v_waiters, WQUE_DESTROYED); /* v-node待ちスレッドを起床 */
		slab_kmem_cache_free(v);  /*  v-nodeを開放  */
	}

	return res;
}

/**
   mntid, vnidをキーとしてv-nodeを検索し, 参照を得る
   @param[in] mntid マウントポイントID
   @param[in] vnid  v-node ID
   @param[out] vpp v-nodeを指し示すポインタのアドレス
   @retval  0       正常終了
   @return -EINVAL  不正なマウントポイントIDを指定した
   @return -ENOENT  指定されたfsid, vnidに対応するv-nodeが見つからなかった
 */
int
vfs_vnode_get(vfs_mnt_id mntid, vfs_vnode_id vnid, vnode **outv){
	int             rc;
	bool           res;
	fs_mount      *mnt;
	vnode           *v;

	rc = vfs_fs_mount_get(mntid, &mnt); /* マウントポイントの参照獲得 */
	if ( rc != 0 ) {
		
		rc = -EINVAL;
		goto error_out;
	}

	rc = find_vnode(mnt, vnid, &v);  /* v-nodeを取得する */
	if ( rc != 0 )
		goto put_mount_out;

	/* 
	 * 見つかったvnodeを返却する
	 */
	if ( outv != NULL ) {

		res = vfs_vnode_ref_inc(v);  /* 参照を獲得する */
		if ( !res )
			goto put_mount_out;		
		*outv = v;   /* v-nodeを返却 */
	}

	vfs_fs_mount_put(mnt);  /* マウントポイントの参照解放 */

	return 0;

put_mount_out:
	vfs_fs_mount_put(mnt);  /* マウントポイントの参照解放 */

error_out:
	return rc;
}

/**
   mntid, vnidをキーとしてv-nodeを検索し, 参照を解放する
   @param[in] mntid マウントポイントID
   @param[in] vnid  v-node ID
   @retval  0       正常終了
   @retval -EAGAIN  最終参照者ではなかった
   @return -EINVAL  不正なマウントポイントIDを指定した
   @return -ENOENT  指定されたfsid, vnidに対応するv-nodeが見つからなかった
 */
int
vfs_vnode_put(vfs_mnt_id mntid, vfs_vnode_id vnid){
	int             rc;
	bool           res;
	fs_mount      *mnt;
	vnode           *v;

	rc = vfs_fs_mount_get(mntid, &mnt); /* マウントポイントの参照獲得 */
	if ( rc != 0 ) {
		
		rc = -EINVAL;
		goto error_out;
	}

	rc = find_vnode(mnt, vnid, &v);  /* v-nodeを取得する */
	if ( rc != 0 )
		goto put_mount_out;

	/* 
	 * 見つかったv-nodeを返却する
	 */
	res = vfs_vnode_ref_dec(v);  /* 参照を獲得する             */
	
	vfs_fs_mount_put(mnt);       /* マウントポイントの参照解放 */

	if ( res )
		return 0;  /* 最終参照者だった       */

	return -EAGAIN;    /* 最終参照者ではなかった */

put_mount_out:
	vfs_fs_mount_put(mnt);  /* マウントポイントの参照解放 */

error_out:
	return rc;
}

/**
   vnodeをロックする
   @param[in] v 操作対象のvnode
   @retval  0       正常終了
   @retval -ENOENT  vnodeが破棄された
 */
int
vfs_vnode_lock(vnode *v) {
	int             rc;
	wque_reason reason;

	do {
		mutex_lock(&v->v_mount->m_mtx);  /* マウントポイントのロックを獲得 */

		rc = trylock_vnode(v);  /*  vnodeのロックを試みる  */

		if ( rc != 0 ) {
			
			kassert( rc == -EBUSY );

			/*  他のスレッドがvnodeを使用中の場合は
			 *  vnodeの開放を待ち合わせる 
			 */
			reason = wque_wait_on_event_with_mutex(&v->v_waiters, 
			    &v->v_mount->m_mtx);
			if ( reason == WQUE_DESTROYED ) 
				goto unlock_out;
		}

		mutex_unlock(&v->v_mount->m_mtx); /* マウントポイントのロックを解放 */
	}while( rc != 0 );

	return 0;

unlock_out:
	mutex_unlock(&v->v_mount->m_mtx);  /* マウントポイントのロックを解放 */

	return -ENOENT;
}

/**
   vnodeをアンロックする
   @param[in] v 操作対象のvnode
 */
void
vfs_vnode_unlock(vnode *v) {

	/* マウントポイントの参照獲得 
	 * TODO: vnode生成時にマウントポイントの参照を加算, 解放時に減算
	 */

	mutex_lock(&v->v_mount->m_mtx);  /* マウントポイントのロックを獲得 */

	kassert( is_busy_vnode_nolock(v) );  /* ロック済みv-nodeであることを確認 */

	unmark_busy_vnode_nolock(v);   /*  未使用に更新  */
	
	mutex_unlock(&v->v_mount->m_mtx); /* マウントポイントのロックを解放 */

	return ;
}

/**
   マウントポイント情報への参照を得る
   @param[in]  mntid   マウントID
   @param[out] mountp  マウントポイント情報を指し示すポインタのアドレス
   @retval    0      正常終了
   @retval   -ENOENT マウントIDに対応するマウントポイント情報が見つからなかった
 */
int
vfs_fs_mount_get(vfs_mnt_id mntid, fs_mount **mountp) {
	int           rc;
	bool         res;
	fs_mount    *mnt;
	fs_mount   m_key;

	m_key.m_id = mntid;  /* 検索対象マウントID */

	mutex_lock(&g_mnttbl.mt_mtx);  /* マウントテーブルをロックする */

	/* マウント情報を検索 */
	mnt = RB_FIND(_fs_mount_tree, &g_mnttbl.mt_head, &m_key); 
	if ( mnt == NULL ) {

		rc = -ENOENT;  /* マウントポイント情報が見つからなかった */
		goto unlock_out;
	}

	if ( mountp != NULL ) {

		res = vfs_fs_mount_ref_inc(mnt);  /* 操作用に参照を加算 */
		kassert( res );
		*mountp = mnt;  /* マウントポイントを返却 */
	}

	mutex_unlock(&g_mnttbl.mt_mtx);   /* マウントテーブルをアンロックする */

	return 0;

unlock_out:
	mutex_unlock(&g_mnttbl.mt_mtx);   /* マウントテーブルをアンロックする */
	return rc;
}

/**
   マウントポイント情報への参照を返却する
   @param[in] mount  マウントポイント情報
 */
void
vfs_fs_mount_put(fs_mount *mount) {

	vfs_fs_mount_ref_dec(mount);  /* 操作用の参照を減算 */

	return ;
}

/**
   マウントポイントの参照カウンタをインクリメントする
   @param[in] mount  マウント情報
   @retval    真 マウントポイントの参照を獲得できた
   @retval    偽 マウントポイントの参照を獲得できなかった
 */
bool
vfs_fs_mount_ref_inc(fs_mount *mount) {

	/* アンマウント中でなければ, 利用カウンタを加算し, 加算前の値を返す  
	 */
	return ( refcnt_inc_if_valid(&mount->m_refs) != 0 ); 
}

/**
   マウントポイントの参照カウンタをデクリメントする
   @param[in] mount  マウント情報
   @retval    真 マウントポイントの最終参照者だった
   @retval    偽 マウントポイントの最終参照者でない
 */
bool
vfs_fs_mount_ref_dec(fs_mount *mount){
	bool res;

	/*  マウントポイントの参照カウンタをさげる  */
	res = refcnt_dec_and_mutex_lock(&mount->m_refs, &g_mnttbl.mt_mtx);
	if ( res ) { /* マウントポイントの最終参照者だった場合 */

		/* マウントポイントをマウントテーブルから外す */
		remove_fs_mount_from_mnttbl_nolock(mount);
		free_fsmount(mount); /* マウントポイントを解放する */
		mutex_unlock(&g_mnttbl.mt_mtx);  /*  マウントテーブルをアンロック  */
	}

	return res;
}

/**
   マウントテーブルの初期化
 */
void
vfs_init_mount_table(void){
	int rc;
	
	/* マウントポイント用SLABキャッシュの初期化 */
	rc = slab_kmem_cache_create(&fs_mount_cache, "vfs mount point", 
	    sizeof(fs_mount), SLAB_ALIGN_NONE,  0, KMALLOC_NORMAL, NULL, NULL);
	kassert( rc == 0 );
	
	/* v-node用SLABキャッシュの初期化 */
	rc = slab_kmem_cache_create(&vnode_cache, "vfs v-node", 
	    sizeof(vnode), SLAB_ALIGN_NONE,  0, KMALLOC_NORMAL, NULL, NULL);
	kassert( rc == 0 );
}

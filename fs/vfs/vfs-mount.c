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

#include <kern/mutex.h>
#include <kern/page-if.h>
#include <kern/thr-if.h>

#include <kern/vfs-if.h>

#include <fs/vfs/vfs-internal.h>

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
   マウントID比較関数 (内部関数)
   @param[in] m1 マウント情報
   @param[in] m2 マウント情報
   @retval 正  m1のm_idが entのm_idより後にある (m1->m_id > m2->m_id )
   @retval 負  m1のm_idが entのm_idより前にある (m1->m_id < m2->m_id )
   @retval 0   m1のm_idが entのm_idに等しい     (m1->m_id == m2->m_id )
 */
static int
compare_mount_id_common(struct _fs_mount *m1, struct _fs_mount *m2){

	if ( m1->m_id > m2->m_id )
		return 1;

	if ( m1->m_id < m2->m_id )
		return -1;

	return 0;
}

/**
   vnode ID比較関数 (内部関数)
   @param[in] v1 v-node
   @param[in] v2 v-node
   @retval 正  v1のv_idが entのv_idより後にある (v1->v_id > v2->v_id )
   @retval 負  v1のv_idが entのv_idより前にある (v1->v_id < v2->v_id )
   @retval 0   v1のv_idが entのv_idに等しい     (v1->v_id == v2->v_id )
 */
static int
compare_vnode_id_common(struct _vnode *v1, struct _vnode *v2){

	if ( v1->v_id > v2->v_id )
		return 1;

	if ( v1->v_id < v2->v_id )
		return -1;

	return 0;
}
/**
    マウントポイント比較関数
    @param[in] key 比較対象マウントポイント
    @param[in] ent マウントポイントテーブル内の各エントリ
    @retval 正  keyのm_idが entのm_idより後にある (key->m_id > ent->m_id )
    @retval 負  keyのm_idが entのm_idより前にある (key->m_id < ent->m_id )
    @retval 0   keyのm_idが entのm_idに等しい     (key->m_id == ent->m_id )
 */
static int
_fs_mount_cmp(struct _fs_mount *key, struct _fs_mount *ent){

	return compare_mount_id_common(key, ent);
}

/**
    v-node比較関数
    @param[in] key 比較対象v-node
    @param[in] ent マウントポイント内の各v-nodeエントリ
    @retval 正  keyのm_id, v_idが entのm_id, v_idより後にある
                (key->v_mount->m_id > ent->v_mount->m_id
		または, key->v_mount->m_id == ent->v_mount->m_id, key->v_id > ent->v_id)
    @retval 負  keyのm_id, v_idが entのm_id, v_idより前にある
                (key->v_mount->m_id < ent->v_mount->m_id
		または, key->v_mount->m_id == ent->v_mount->m_id, key->v_id < ent->v_id)
    @retval 0   keyのm_id, v_idが entのm_id, v_idに等しい
                (key->v_mount->m_id == ent->v_mount->m_id, かつ, key->v_id == ent->v_id )
 */
static int
_vnode_cmp(struct _vnode *key, struct _vnode *ent){

	if ( key->v_mount->m_id > ent->v_mount->m_id )
		return 1;

	if ( key->v_mount->m_id < ent->v_mount->m_id )
		return -1;

	if ( key->v_id > ent->v_id )
		return 1;

	if ( key->v_id < ent->v_id )
		return -1;

	return 0;
}

/**
   v-node mutexを獲得せずにv-nodeのフラグ値を設定する (内部関数)
   @param[in] v     操作対象のv-node
   @param[in] flags 設定するフラグ値
 */
static void
mark_vnode_flag_nolock(vnode *v, vfs_vnode_flags flags){

	v->v_flags |= flags;      /*  フラグをセットする */
	if ( !wque_is_empty(&v->v_waiters) )   /*  v-nodeを待っているスレッドを起床 */
		wque_wakeup(&v->v_waiters, WQUE_RELEASED);
}

/**
   v-node mutexを獲得せずにv-nodeのフラグをクリアする (内部関数)
   @param[in] v 操作対象のv-node
   @param[in] flags 設定するフラグ値
 */
static void
unmark_vnode_flag_nolock(vnode *v, vfs_vnode_flags flags) {

	v->v_flags &= ~flags;      /*  フラグをクリアする */
	if ( !wque_is_empty(&v->v_waiters) )   /*  v-nodeを待っているスレッドを起床 */
		wque_wakeup(&v->v_waiters, WQUE_RELEASED);
}

/**
   v-nodeのフラグ値を確認する (内部関数)
   @param[in] v 操作対象のv-node
   @param[in] flags 確認するフラグ値
   @retval    真 フラグのいずれかがセットされている
   @retval    偽 フラグのいずれもがセットされていない
 */
static bool
check_vnode_flags_nolock(vnode *v, vfs_vnode_flags flags) {

	return ( ( v->v_flags & flags ) != 0 );
}

/**
   v-node mutexを獲得せずにv-nodeを使用中に設定する (内部関数)
   @param[in] v 操作対象のv-node
 */
static void
mark_vnode_busy_nolock(vnode *v) {
	vfs_vnode_flags old_flags;

	old_flags = v->v_flags; /* 更新前のフラグ値を獲得 */

	v->v_flags |= VFS_VFLAGS_BUSY;      /*  フラグをセットする */

	kassert( v->v_locked_by == NULL );  /*  ロック獲得者がいないことを確認 */
	v->v_locked_by = ti_get_current_thread();  /* 自スレッドをロック獲得スレッドに設定 */

	/* フラグ更新を通知 */
	if ( !( old_flags & VFS_VFLAGS_BUSY )
	    && ( !wque_is_empty(&v->v_waiters) ) )   /*  v-nodeを待っているスレッドを起床 */
		wque_wakeup(&v->v_waiters, WQUE_RELEASED);
}

/**
   v-node mutexを獲得せずにv-nodeの使用中フラグ(VFS_VFLAGS_BUSY)をクリアする (内部関数)
   @param[in] v 操作対象のv-node
 */
static void
unmark_vnode_busy_nolock(vnode *v) {
	vfs_vnode_flags old_flags;

	old_flags = v->v_flags; /* 更新前のフラグ値を獲得 */

	v->v_flags &= ~VFS_VFLAGS_BUSY;      /*  フラグをクリアする */

	kassert( v->v_locked_by != NULL );  /* ロック獲得者がいることを確認 */
	v->v_locked_by = NULL;              /* ロック獲得スレッドをクリア   */

	/* フラグ更新を通知 */
	if  ( ( old_flags & VFS_VFLAGS_BUSY )
	    && ( !wque_is_empty(&v->v_waiters) ) ) /*  v-nodeを待っているスレッドを起床 */
		wque_wakeup(&v->v_waiters, WQUE_RELEASED);
}

/**
   v-nodeのフラグ値を設定する (内部関数)
   @param[in] v     操作対象のv-node
   @param[in] flags 設定するフラグ値
   @retval    真    フラグを設定できた
   @retval    偽    フラグを設定できなかった
   @note LO: v-nodeロック, ウエイトキューロックの順にロックを獲得する
 */
static bool
mark_vnode_flag(vnode *v, vfs_vnode_flags flags){
	bool          res;

	res = vfs_vnode_ref_inc(v);  /* フラグ操作用に参照を獲得する */
	if ( !res )
		return false; /* すでに削除中のv-nodeだった */

	mutex_lock(&v->v_mtx);  /* v-nodeのロックを獲得 */

	if ( check_vnode_flags_nolock(v, flags) )
		goto unlock_out;  /* フラグ設定済み */

	mark_vnode_flag_nolock(v, flags);  /* フラグを設定 */
	if ( !wque_is_empty(&v->v_waiters) )   /*  v-nodeを待っているスレッドを起床 */
		wque_wakeup(&v->v_waiters, WQUE_RELEASED);

unlock_out:
	mutex_unlock(&v->v_mtx);  /* v-nodeのロックを解放 */

	res = vfs_vnode_ref_dec(v);  /* 参照を解放 */

	return !res;      /* 最終参照でなければフラグをセットできている  */
}

/**
   v-nodeのフラグをクリアする (内部関数)
   @param[in] v 操作対象のv-node
   @param[in] flags クリアするフラグ値
   @retval    真    フラグを設定できた
   @retval    偽    フラグを設定できなかった
 */
static bool
unmark_vnode_flag(vnode *v, vfs_vnode_flags flags) {
	bool res;

	res = vfs_vnode_ref_inc(v);  /* フラグ操作用に参照を獲得する */
	if ( !res )
		return false; /* すでに削除中のv-nodeだった */

	mutex_lock(&v->v_mtx);  /* v-nodeのロックを獲得 */

	unmark_vnode_flag_nolock(v, flags);  /* フラグをクリアする */

	mutex_unlock(&v->v_mtx);  /* v-nodeのロックを解放 */

	res = vfs_vnode_ref_dec(v);  /* 参照を解放 */

	return !res;      /* 最終参照ではなければフラグをセットできている  */
}

/**
   v-nodeを使用中に設定する (内部関数)
   @param[in] v     操作対象のv-node
   @retval    真    フラグを設定できた
   @retval    偽    フラグを設定できなかった
   @note LO: v-nodeロック, ウエイトキューロックの順にロックを獲得する
 */
static bool
mark_vnode_busy(vnode *v){
	bool          res;

	res = vfs_vnode_ref_inc(v);  /* フラグ操作用に参照を獲得する */
	if ( !res )
		return false; /* すでに削除中のv-nodeだった */

	mutex_lock(&v->v_mtx);  /* v-nodeのロックを獲得 */

	if ( check_vnode_flags_nolock(v, VFS_VFLAGS_BUSY) )
		goto unlock_out;  /* フラグ設定済み */

	mark_vnode_busy_nolock(v);  /* 使用中に設定 */

unlock_out:
	mutex_unlock(&v->v_mtx);  /* v-nodeのロックを解放 */

	res = vfs_vnode_ref_dec(v);  /* 参照を解放 */

	return !res;      /* 最終参照でなければフラグをセットできている  */
}

/**
   v-nodeの使用中フラグをクリアする (内部関数)
   @param[in] v 操作対象のv-node
   @retval    真    フラグを設定できた
   @retval    偽    フラグを設定できなかった
 */
static bool
unmark_vnode_busy(vnode *v) {
	bool res;

	res = vfs_vnode_ref_inc(v);  /* フラグ操作用に参照を獲得する */
	if ( !res )
		return false; /* すでに削除中のv-nodeだった */

	mutex_lock(&v->v_mtx);  /* v-nodeのロックを獲得 */

	unmark_vnode_busy_nolock(v); /* 使用中フラグをクリアする */

	mutex_unlock(&v->v_mtx);  /* v-nodeのロックを解放 */

	res = vfs_vnode_ref_dec(v);  /* 参照を解放 */

	return !res;      /* 最終参照ではなければフラグをセットできている  */
}

/**
   自スレッドがv-nodeをロックしていることを確認する (内部関数)
   @param[in] v 操作対象のv-node
   @retval    真     自スレッドがv-nodeをロックしている
   @retval    偽     自スレッドがv-nodeをロックしていない
 */
static bool
check_vnode_locked_by_self_nolock(vnode *v) {
	bool  rc;

	/* ロック済みv-nodeで, 自スレッドがロックを所持している場合は, 真を返す */
	if ( ( check_vnode_flags_nolock(v, VFS_VFLAGS_BUSY) )
	    && ( v->v_locked_by == ti_get_current_thread() ) )
		rc = true;
	else
		rc = false;

	return rc;
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
	mount->m_dev = VFS_VSTAT_INVALID_DEVID; /*  デバイスIDの初期化               */
	RB_INIT(&mount->m_head);         /*  マウントテーブル内のv-nodeリストの初期化 */
	mount->m_fs_super = NULL;        /*  ファイルシステム固有情報の初期化        */
	mount->m_fs = fs;                /*  ファイルシステム情報の初期化            */
	mount->m_mnttbl = NULL;          /*  登録先マウントテーブルの初期化          */
	mount->m_root = NULL;            /*  ルートv-nodeの初期化                    */
	mount->m_mount_point = NULL;     /*  マウントポイントvnodeの初期化           */
	mount->m_mount_flags = VFS_MNT_MNTFLAGS_NONE;  /*  マウントフラグの初期化    */
	mount->m_mount_path = strdup(path);     /*  マウントポイントパスの複製       */

	if ( mount->m_mount_path == NULL ) {

		rc = -ENOMEM;  /* メモリ不足 */
		goto error_out;
	}

	vfs_fs_ref_inc(mount->m_fs);  /* ファイルシステムへの参照を加算 */

	return 0;

error_out:
	return rc;
}

/**
   マウント情報を割り当てる (内部関数)
   @param[in] path    マウントポイントパス文字列
   @param[in] fs_name ファイルシステム名
   @param[out] mntp   マウント情報を指すポインタのアドレス
   @retval  0      正常終了
   @retval -ENOMEM メモリ不足
   @retval -ENOENT ファイルシステムが見つからなかった
 */
static int
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

	vfs_fs_put(fs);  /* ファイルシステムへの参照を返却する */

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

	vfs_fs_ref_dec(mount->m_fs);   /*  ファイルシステムへの参照を解放  */

	/*  対象のマウント情報を待ち合わせているスレッドを起床 */
	mutex_destroy(&mount->m_mtx);
	kfree(mount->m_mount_path);  /*  マウントポイント文字列を解放  */
	slab_kmem_cache_free(mount); /*  マウント情報を解放  */
}
/**
   マウントIDを割り当てる
   @param[out] idp     マウントID返却領域
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
    @note マウントテーブルロックを獲得後に呼び出すこと
 */
static int
add_fsmount_to_mnttbl_nolock(fs_mount *mount) {
	int             rc;
	vfs_mnt_id  new_id;
	fs_mount  *cur_mnt;

	rc = alloc_new_mntid_nolock(&new_id); /* マウントIDの割り当て */
	if ( rc != 0 )
		goto error_out;

	mount->m_id = new_id;
	cur_mnt = RB_INSERT(_fs_mount_tree, &g_mnttbl.mt_head, mount);
	kassert( cur_mnt == NULL );  /* マウント情報の多重登録 */

	return 0;

error_out:
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
   ロックをとらずにマウントポイントの参照カウンタをデクリメントする
   @param[in] mount  マウント情報
   @retval    真 マウントポイントの最終参照者だった
   @retval    偽 マウントポイントの最終参照者でない
 */
static bool
dec_fs_mount_ref_nolock(fs_mount *mount){
	bool res;

	/*  マウントポイントの参照カウンタをさげる  */
	res = refcnt_dec_and_test(&mount->m_refs);
	if ( res ) { /* マウントポイントの最終参照者だった場合 */

		/* マウントポイントをマウントテーブルから外す */
		remove_fs_mount_from_mnttbl_nolock(mount);
		free_fsmount(mount); /* マウントポイントを解放する */
	}

	return res;
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
    マウント情報からv-nodeを削除 (実処理関数)
    @param[in] v      削除するv-node
    @retval    0      正常終了
 */
static void
del_vnode_from_mount_nolock(vnode *v){
	vnode *res_v;

	/* v-nodeをマウントポイントのv-nodeテーブルから外す */
	res_v = RB_REMOVE(_vnode_tree, &v->v_mount->m_head, v);
	kassert( res_v != NULL );  /* v-nodeの多重解放 */

	dec_fs_mount_ref_nolock(v->v_mount);  /* v-nodeからの参照を減算 */
	return ;
}

/**
   マウントポイント情報への参照を得る(実処理関数)
   @param[in]  mntid   マウントID
   @param[out] mountp  マウントポイント情報を指し示すポインタのアドレス
   @retval    0      正常終了
   @retval   -ENOENT マウントIDに対応するマウントポイント情報が見つからなかった
 */
static int
get_fs_mount_nolock(vfs_mnt_id mntid, fs_mount **mountp) {
	bool         res;
	fs_mount    *mnt;
	fs_mount   m_key;

	m_key.m_id = mntid;  /* 検索対象マウントID */

	/* マウント情報を検索 */
	mnt = RB_FIND(_fs_mount_tree, &g_mnttbl.mt_head, &m_key);
	if ( mnt == NULL )
		return -ENOENT; /* マウントポイント情報が見つからなかった */

	if ( mountp != NULL ) {

		res = vfs_fs_mount_ref_inc(mnt);  /* 操作用に参照を加算 */
		kassert( res );
		*mountp = mnt;  /* マウントポイントを返却 */
	}

	return 0;
}

/**
   v-nodeの削除を予約する (内部関数)
   @param[in] v  操作対象のv-node
   @retval    真 v-nodeの最終参照者だった
   @retval    偽 v-nodeの参照が残っている
 */
static bool
mark_vnode_delete_common(vnode *v){
	bool     res;

	res = vfs_vnode_ref_inc(v);  /* フラグ操作用に参照を獲得する */
	if ( !res )
		return true; /* すでに削除中のv-nodeだった */

	mutex_lock(&v->v_mount->m_mtx);    /*  マウントポイントロックを獲得     */
	mutex_lock(&v->v_mtx);  /* v-nodeのロックを獲得 */

	/* 未削除v-nodeの場合は, マウントポイントから外す */
	if ( !check_vnode_flags_nolock(v, VFS_VFLAGS_DELETE) ) {

		del_vnode_from_mount_nolock(v);  /* v-nodeをマウントポイント情報から削除 */
		res = vfs_vnode_ref_dec(v);  /* マウント情報からの参照を解放する */
		kassert( !res );

		/* ディスクI-nodeの削除予約をかける */
		mark_vnode_flag_nolock(v, VFS_VFLAGS_DELETE);
		if ( !wque_is_empty(&v->v_waiters) )   /*  v-nodeを待っているスレッドを起床 */
			wque_wakeup(&v->v_waiters, WQUE_RELEASED);
	}

	mutex_unlock(&v->v_mtx);  /* v-nodeのロックを解放 */
	mutex_unlock(&v->v_mount->m_mtx);    /*  マウントポイントロックを解放     */

	res = vfs_vnode_ref_dec(v);  /* 削除操作用の参照を解放する */

	return res;
}

/**
   v-nodeの初期化
   @param[in] v 初期化対象のv-node
   @note alloc_new_vnode内部のv-node初期化処理部分をメンテナンスの観点から分離した関数.
   マウントポイント情報の設定を行う必要があるので, alloc_new_vnodeのみから使用する.
 */
static void
init_vnode(vnode *v){

	/*
	 * メンバを初期化
	 */
	mutex_init(&v->v_mtx);               /*  状態更新用mutexの初期化                    */
	/* マウント情報からの参照分を設定して初期化する */
	refcnt_init(&v->v_refs);             /*  参照カウンタの初期化                       */
	v->v_fduse = 0;                      /*  ファイルディスクリプタからの利用数初期化   */
	v->v_fs_vnode = NULL;                /*  ファイルシステム固有v-nodeの初期化         */
	wque_init_wait_queue(&v->v_waiters); /*  v-nodeウエイトキューの初期化               */
	v->v_id = VFS_INVALID_VNID;          /*  vnidの初期化                               */
	v->v_mount = NULL;                   /*  マウント情報の初期化                       */
	v->v_mount_on = NULL;                /*  マウント先ボリュームのマウント情報を初期化 */
	v->v_locked_by = NULL;               /*  ロック獲得スレッドを初期化                 */
	v->v_mode = VFS_VNODE_MODE_NONE;     /*  ファイルモードの初期化                     */
	v->v_flags = VFS_VFLAGS_FREE;        /*  未使用v-nodeとして初期化                   */
	mark_vnode_busy_nolock(v);           /*  使用中に設定する */
	/*  ファイルシステム固有v-node未読み込み    */
	unmark_vnode_flag_nolock(v, VFS_VFLAGS_VALID);

	return ;
}

/**
   v-nodeを新規に生成する (内部関数)
   @param[in] mnt マウントポイント情報
   @return 新設したv-node
   @return NULL メモリ不足
   @note  マウント情報からの参照を付与したv-nodeを生成する
   生成したv-nodeをマウント情報へ登録する必要があるので, find_vnodeのみで使用する
   find_vnode内部の処理をメンテナンスの観点から分離した関数.
 */
static vnode *
alloc_new_vnode(fs_mount *mnt){
	int           rc;
	vnode *new_vnode;

	rc = slab_kmem_cache_alloc(&vnode_cache, KMALLOC_NORMAL,
	    (void **)&new_vnode);
	if ( rc != 0 )
		goto error_out;  /* メモリ不足 */

	init_vnode(new_vnode);   /* v-nodeを初期化  */

	new_vnode->v_mount = mnt;  /* マウントポイント情報を設定 */

	return new_vnode;  /* 獲得したv-nodeを返却 */

error_out:
	return NULL;
}

/**
   v-nodeを解放する (内部関数)
   @param[in] v  v-node情報
 */
static void
release_vnode(vnode *v){

	/*
	 * TODO:
	 * vm_cache(v-nodeに関連付けられたデータページキャッシュキュー)機能実装時に
	 * 本関数の最初でvm_cacheを開放すること
	 */

	/*
	 * ファイルシステム固有のremove/putvnode操作を呼出し
	 *  実ファイルシステム上のv-node(disk I-node)を削除する場合
	 *    fs_removevnodeを呼び出す
	 *  実ファイルシステム上のv-node(disk I-node)をv-nodeの内容で更新する場合, または,
	 *    fs_removevnodeが定義されていない場合fs_putvnodeを呼び出す
	 */
	if ( check_vnode_flags_nolock(v, VFS_VFLAGS_DELETE)
	    && ( v->v_mount->m_fs->c_calls->fs_removevnode != NULL ) )
		v->v_mount->m_fs->c_calls->fs_removevnode(v->v_mount->m_fs_super,
		    v->v_id, v->v_fs_vnode);
	else
		v->v_mount->m_fs->c_calls->fs_putvnode(v->v_mount->m_fs_super,
		    v->v_id, v->v_fs_vnode);

	if ( !check_vnode_flags_nolock(v, VFS_VFLAGS_DELETE) )
		del_vnode_from_mount_nolock(v);  /* v-nodeをマウントポイント情報から削除 */

	v->v_mount = NULL;
	/*
	 *  v-nodeを解放する
	 */
	mutex_destroy(&v->v_mtx);    /* 待ちスレッドを起床  */
	wque_wakeup(&v->v_waiters, WQUE_DESTROYED); /* v-node待ちスレッドを起床 */
	slab_kmem_cache_free(v);  /*  v-nodeを開放  */
}

/**
   v-nodeのロックを試みる (内部関数)
   @param[in] v 操作対象のv-node
   @retval  0      正常終了
   @retval -EBUSY  他スレッドがロックを獲得中
   @pre マウントポイントのロックを獲得してから呼び出す
 */
static int
trylock_vnode(vnode *v) {
	bool res;

	if ( check_vnode_flags_nolock(v, VFS_VFLAGS_BUSY) )
		return -EBUSY; /* 他スレッドがロックを獲得中 */

	res = mark_vnode_busy(v);   /*  使用中に設定する */

	kassert( res );  /* trylock_vnode呼び出し前に参照を得ているのでフラグを設定可能 */

	return 0;
}

/**
   mntid, vnidをキーとしてv-nodeを検索する (実処理関数)
   @param[in]  mnt   マウントポイント情報
   @param[in]  vnid  v-node ID
   @param[out] outv  v-nodeを指し示すポインタのアドレス
   @retval  0        正常終了
   @retval -EINVAL   不正なマウントポイントIDを指定した
   @retval -ENOMEM   メモリ不足
   @retval -ENOENT   ディスクI-node読み取りに失敗した
   @retval -EINTR    v-node待ち合わせ中にイベントを受信した
   @retval -EBUSY    アンマウント中のボリュームだった
   @note LO: マウントポイントロック, v-nodeロックの順にロックを獲得する
   @note 本関数で確保したv-nodeはマウント情報からの参照(参照カウント1)がついた状態で
   初期化されるので, 呼び出し元でマウント情報に登録すること
 */
static int
find_vnode(fs_mount *mnt, vfs_vnode_id vnid, vnode **outv){
	int             rc;
	vnode           *v;
	vnode        v_key;
	vfs_fs_mode   mode;
	wque_reason reason;

	v_key.v_id = vnid;    /* 検索対象v-node ID */
	v_key.v_mount = mnt;  /* 検索対象v-nodeのマウントポイント情報 */
	for( ; ; ) {

		mutex_lock(&mnt->m_mtx);  /* マウントポイントのロックを獲得 */

		/*
		 * 指定されたv-node IDに対応するv-nodeを指定されたマウントポイントから検索
		 */
		v = RB_FIND(_vnode_tree, &mnt->m_head, &v_key);
		if ( v == NULL ) {  /* v-nodeが登録されていない */

			/*
			 * v-nodeを割当て, 実ファイルシステムからディスクI-nodeを読み込む
			 */
			v = alloc_new_vnode(mnt);
			if ( v == NULL ) {

				rc = -ENOMEM;  /* メモリ不足 */
				goto unlock_out;
			}

			/*
			 * ファイルシステム固有のv-node情報を割り当てる
			 */
			rc = v->v_mount->m_fs->c_calls->fs_getvnode(v->v_mount->m_fs_super,
			    vnid, &mode, &v->v_fs_vnode);

			if ( ( rc != 0 ) || ( v->v_fs_vnode == NULL ) ) {

				rc = -ENOENT;  /* ディスクI-nodeの読み取りに失敗した */
				goto unlock_out;
			}

			v->v_id = vnid;      /* v-node IDを設定 */
			v->v_mode = mode;    /* v-nodeのファイルモードを設定 */

			/* v-nodeをロード済みに設定する */
			mark_vnode_flag_nolock(v, VFS_VFLAGS_VALID);

			rc = add_vnode_to_mount_nolock(v->v_mount, v); /* v-nodeを登録する */

			unmark_vnode_busy_nolock(v);  /* v-nodeのロックを解除 */
			if ( rc != 0 )
				goto unlock_out;  /* v-nodeを登録できなかった */
		}

		kassert( v != NULL );
		kassert( check_vnode_flags_nolock(v, VFS_VFLAGS_VALID) );

		if ( !check_vnode_flags_nolock(v, VFS_VFLAGS_BUSY) ) {

			/* 他に対象のv-nodeを更新しているスレッドがいなければ
			 * 見つかったv-nodeを返却する
			 */
			if ( outv != NULL ) {

				*outv = v;             /* v-nodeを返却               */
				/* マウントポイントのロックを解放 */
				mutex_unlock(&mnt->m_mtx);
			}
			break;
		}

		/*  v-nodeの更新完了を待ち合わせる */
		reason = wque_wait_on_event_with_mutex(&v->v_waiters, &mnt->m_mtx);
		if ( reason == WQUE_DELIVEV ) {

			rc = -EINTR;   /* イベントを受信した */
			goto unlock_out;
		}

		mutex_unlock(&mnt->m_mtx);  /* マウントポイントのロックを解放 */
	}

	return 0;

unlock_out:
	mutex_unlock(&mnt->m_mtx);  /* マウントポイントのロックを解放 */

	return rc;
}

/**
   ロックをとらずにv-nodeの参照カウンタをデクリメントする(内部関数)
   @param[in] v  v-node情報
   @retval    真 v-nodeの最終参照者だった
   @retval    偽 v-nodeの最終参照者でない
   @pre マウントポイントロックを獲得してから呼び出すこと
 */
static bool
dec_vnode_ref_nolock(vnode *v){
	bool     res;

	kassert( v->v_mount != NULL ); /* v-nodeテーブルに登録されていることを確認 */

	/* v-nodeの参照カウンタをデクリメントする */
	res = refcnt_dec_and_test(&v->v_refs);
	if ( res )  /* v-nodeの最終参照者だった場合 */
		release_vnode(v); /* v-nodeを解放する */

	return res;
}

/**
   v-nodeに関連付けられたデータを書き戻す (内部関数)
   @param[in] v  v-node情報
   @retval    0      正常終了
 */
static int
fsync_vnode_common(vnode *v){
	int        rc;

	kassert( v->v_mount != NULL);
	kassert( v->v_mount->m_fs != NULL);
	kassert( is_valid_fs_calls( v->v_mount->m_fs->c_calls ) );

	rc = 0;  /*  正常終了 */

	/* ファイルシステム固有のv-node書き戻し処理がない場合は
	 * v-nodeのみ書き戻す (v-nodeのロックは呼び出し元で得ているか,
	 * ロック不可条件下(アンマウント中)で呼ばれる
	 */
	if ( v->v_mount->m_fs->c_calls->fs_fsync != NULL )
		rc = v->v_mount->m_fs->c_calls->fs_fsync(v->v_mount->m_fs_super,
		    v->v_fs_vnode);

	return rc;
}

/**
   マウントポイント中のv-nodeを書き出してロックする (内部関数)
   @param[in] mount マウント情報
   @retval  0     正常終了
   @retval -EBUSY 使用中のv-nodeがマウントテーブル中に残留している
   @pre    マウントテーブルロックを獲得して呼び出すこと
   @note LO: マウントポイントロック, v-nodeロックの順にロックを獲得する
 */
static int
sync_and_lock_vnodes(fs_mount *mount){
	int           rc;
	vnode         *v;

	/* 他のスレッドがファイル操作を行わないようにv-nodeテーブルロックを獲得
	 */
	mutex_lock(&mount->m_mtx);    /*  マウントポイントロックを獲得     */

	/* アンマウント可否によらずボリューム中の更新中でないファイルの内容を
	 * 書き出し, ファイルシステムの損傷を最小化する
	 */
	RB_FOREACH(v, _vnode_tree, &mount->m_head){

		mutex_lock(&v->v_mtx);    /* v-nodeのロックを獲得 */
		fsync_vnode_common(v);    /* データブロックを書き戻す */

		/* 未書き出しのv-nodeを書き戻す
		 */
		if ( check_vnode_flags_nolock(v, VFS_VFLAGS_DIRTY) ) {

			rc = v->v_mount->m_fs->c_calls->fs_putvnode(v->v_mount->m_fs_super,
			    v->v_id, v->v_fs_vnode);
			if ( rc == 0 )
				unmark_vnode_flag_nolock(v, VFS_VFLAGS_DIRTY);
		}
		mutex_unlock(&v->v_mtx);  /* v-nodeのロックを解放 */
	}

	/*
	 * メタ情報を書き戻す
	 */
	if ( mount->m_fs->c_calls->fs_sync != NULL )
		mount->m_fs->c_calls->fs_sync(mount->m_fs_super);

	/*
	 * ボリューム中に使用中のvnodeが含まれていないことを確認する
	 */
	RB_FOREACH(v, _vnode_tree, &mount->m_head){

		mutex_lock(&v->v_mtx);  /* v-nodeのロックを獲得 */
		if ( ( check_vnode_flags_nolock(v, VFS_VFLAGS_BUSY) ) ||
		     ( (v != mount->m_root)
		       && ( ( refcnt_read(&v->v_refs) > 1 ) || ( v->v_fduse > 0 ) ) ) ||
		     ( (v == mount->m_root) && ( refcnt_read(&v->v_refs) > 3 ) ) ) {

			     /* root v-node でない場合で参照中の場合または
			      * root v-node の場合で参照数が3より大きい場合は,
			      * エラー復帰する
			      *
			      * v-nodeテーブルからの参照で+1, 上記の
			      * vfs_path_to_vnodeで+1, マウント時に+1されているので
			      * この時点でroot v-nodeの参照カウンタは3でなければ
			      * アンマウントできない
			      */
			     rc = -EBUSY;
			     mutex_unlock(&v->v_mtx);  /* v-nodeのロックを解放 */
			     goto unlock_out;
		}
		mutex_unlock(&v->v_mtx);  /* v-nodeのロックを解放 */
	}

	/*
	 * ボリューム中の全vnodeの情報をロックし, v-node更新を禁止する
	 */
	RB_FOREACH(v, _vnode_tree, &mount->m_head){

		mutex_lock(&v->v_mtx);  /* v-nodeのロックを獲得 */
		kassert( v->v_mount != NULL);
		kassert( v->v_mount->m_fs != NULL);
		kassert( is_valid_fs_calls( v->v_mount->m_fs->c_calls ) );

		if ( v != mount->m_root ) /* ルートv-node以外をロックする */
			mark_vnode_busy_nolock(v);  /* v-nodeをロックする */

		mutex_unlock(&v->v_mtx);  /* v-nodeのロックを解放 */
	}

	/* v-nodeの追加を避けるためにv-nodeテーブルロック中でフラグを更新し
	 * アンマウント処理中に遷移
	 */
	mount->m_mount_flags |= VFS_MNT_UNMOUNTING;
	mutex_unlock(&mount->m_mtx);  /* v-nodeテーブルをアンロック */

	return 0;

unlock_out:
	mutex_unlock(&mount->m_mtx);  /* v-nodeテーブルをアンロック */

	return rc;
}

/**
   マウントポイント情報中のv-nodeを解放する (内部関数)
   @param[in] mount マウントポイント情報
   @note LO: マウントポイントロック, v-nodeロックの順にロックを獲得する
 */
static void
free_vnodes_in_fs_mount(fs_mount *mount){
	vnode         *v;

	/* アンマウント処理中であることを確認 */
	kassert( mount->m_mount_flags & VFS_MNT_UNMOUNTING );

	/* 他のスレッドがファイル操作を行わないようにv-nodeテーブルロックを獲得
	 */
	mutex_lock(&mount->m_mtx);    /*  v-nodeテーブルをロック     */

	/*
	 * ボリューム中の全v-nodeを解放する
	 */
	RB_FOREACH(v, _vnode_tree, &mount->m_head){

		if ( v == mount->m_root )  /* root v-nodeを除いて解放する */
			continue;

		mutex_lock(&v->v_mtx);  /* v-nodeのロックを獲得 */

		kassert( v->v_mount != NULL);
		kassert( v->v_mount->m_fs != NULL);
		kassert( is_valid_fs_calls( v->v_mount->m_fs->c_calls ) );
		kassert( check_vnode_flags_nolock(v, VFS_VFLAGS_BUSY) );
		kassert( refcnt_read(&v->v_refs) == 1 ); /* 解放可能なはず */

		mutex_unlock(&v->v_mtx);  /* v-nodeのロックを解放 */
		vfs_vnode_ref_dec(v);  /* v-nodeの参照を解放 ( v-nodeを解放 ) */
	}

	mutex_unlock(&mount->m_mtx);  /* v-nodeテーブルをアンロック */

	return ;
}

/**
   mntid, vnidをキーとしてv-nodeを検索し, 参照を得る(内部関数)
   @param[in]  mnt  マウントポイント情報
   @param[in]  vnid v-node ID
   @param[out] outv v-nodeを指し示すポインタのアドレス
   @retval  0       正常終了
   @retval -EINVAL  不正なマウントポイントIDを指定した
   @retval -ENOMEM  メモリ不足
   @retval -ENOENT  ディスクI-node読み取りに失敗した
   @retval -EINTR   v-node待ち合わせ中にイベントを受信した
   @retval -EBUSY   アンマウント中のボリュームだった
   @note   vfs_mountからマウントテーブルのロックを獲得した状態で呼び出すget_vnode関数
   @note   本関数は, vnodeを操作するスレッドからの参照を付与してv-nodeを返却する
 */
static int
get_vnode(fs_mount *mnt, vfs_vnode_id vnid, vnode **outv){
	int             rc;
	bool           res;
	vnode           *v;

	for( ; ; ) {

		rc = find_vnode(mnt, vnid, &v);  /* v-nodeを検索する */
		if ( rc != 0 )
			goto error_out;

		/*
		 * 見つかったv-nodeを返却する
		 */
		if ( outv != NULL ) {

			/* vnodeを操作するスレッドからの参照を付与する */
			res = vfs_vnode_ref_inc(v);
			if ( !res )
				continue;  /* v-node破棄に伴い, v-nodeを再検索する */
			*outv = v;   /* v-nodeを返却 */
			break;
		}
	}

	return 0;

error_out:
	return rc;
}

/**
   ファイルシステムのアンマウント (内部関数)
   @param[in] root_vnode アンマウントするボリュームのroot v-node
   @retval  0      正常終了
   @retval -ENOENT 指定されたパスが見つからなかった
   @retval -EINVAL マウントポイントではないパスを指定した
   @retval -EBUSY  ボリューム内のファイルが使用中
   @note LO: マウントテーブルロック, マウントポイントロック,
   v-nodeロックの順にロックを獲得する
 */
static int
unmount_common(vnode *root_vnode){
	fs_mount  *mount;
	int           rc;

	mutex_lock(&g_mnttbl.mt_mtx);  /* マウントテーブルをロック  */

	rc = get_fs_mount_nolock(root_vnode->v_mount->m_id,
				 &mount); /* マウントポイント情報の参照獲得 */
	if ( rc != 0 ) {

		rc = -ENOENT;   /* マウントポイント情報の参照が得られなかった */
		mutex_unlock(&g_mnttbl.mt_mtx);  /* マウントテーブルをアンロック  */
		goto error_out;
	}

	if ( mount == NULL ) {  /* マウントポイントが見つからなかった */

		rc = -ENOENT;   /* マウントポイントの参照が得られなかった */
		mutex_unlock(&g_mnttbl.mt_mtx);  /* マウントテーブルをアンロック  */
		goto error_out;
	}

	kassert( mount->m_fs != NULL );
	kassert( is_valid_fs_calls( mount->m_fs->c_calls ) );

	/* 対象ボリュームのルートv-nodeと指定されたv-nodeが
	 *  異なる場合は, 引数異常で復帰する
	 */
	if ( mount->m_root != root_vnode ) {

		rc = -EINVAL;
		goto unref_mount_in_lock_out;  /*  アンマウント不能  */
	}

	/* ボリューム中のv-nodeを書き出し,
	 * v-nodeをロックする
	 */
	rc = sync_and_lock_vnodes(mount);
	if ( rc != 0 )
		goto unref_mount_in_lock_out;  /*  アンマウント不能  */

	/*
	 * ファイルシステム固有なアンマウント処理を実施
	 */
	if ( mount->m_fs->c_calls->fs_unmount != NULL ) {

		mount->m_fs->c_calls->fs_unmount(mount->m_fs_super);
	}
	/*
	 * root v-node以外のv-nodeを解放
	 */
	free_vnodes_in_fs_mount(mount);

	vfs_vnode_ref_dec( mount->m_root ); /*  root v-nodeの参照を解放する */
	if ( mount->m_mount_point != NULL ) {

		/* マウントポイントv-nodeからの参照を除去 */
		mount->m_mount_point->v_mount_on = NULL;
		/*  マウントポイントディレクトリのv-nodeへの参照を解放する */
		vfs_vnode_ref_dec( mount->m_mount_point );
		mount->m_mount_point = NULL;  /* マウントポイントv-nodeを無効化 */
	}
	/*
	 * マウント情報の登録を抹消
	 */
	dec_fs_mount_ref_nolock(mount);

	mutex_unlock(&g_mnttbl.mt_mtx);  /* マウントテーブルをアンロック  */

	vfs_fs_mount_put(mount);  /* マウントポイントの参照解放 */

	return 0;

unref_mount_in_lock_out:
	/* vfs_fs_mount_put中でマウントテーブルロックを獲得して参照を解放するので
	 * マウントテーブルのロックを先に解放する
	 */
	mutex_unlock(&g_mnttbl.mt_mtx);  /* マウントテーブルをアンロック  */
	vfs_fs_mount_put(mount);  /* マウントポイントの参照解放 */
error_out:
	return rc;
}

/**
   mntid, vnidをキーとしてv-nodeを検索し削除フラグをセットする (内部関数)
   @param[in] mntid マウントポイントID
   @param[in] vnid  v-node ID
   @retval  0       正常終了
   @retval -EAGAIN  最終参照者ではなかった
   @retval -EINVAL  不正なマウントポイントIDを指定した
   @retval -ENOMEM  メモリ不足
   @retval -ENOENT  ディスクI-node読み取りに失敗した
   @retval -EINTR   v-node待ち合わせ中にイベントを受信した
   @retval -EBUSY   アンマウント中のボリュームだった
 */
static int
remove_vnode(vfs_mnt_id mntid, vfs_vnode_id vnid){
	int             rc;
	bool           res;
	fs_mount      *mnt;
	vnode           *v;

	rc = vfs_fs_mount_get(mntid, &mnt); /* マウントポイントの参照獲得 */
	if ( rc != 0 ) {

		rc = -EINVAL;
		goto error_out;
	}

	rc = find_vnode(mnt, vnid, &v);  /* 指定されたv-nodeを検索する */
	if ( rc != 0 )
		goto put_mount_out;

	res = mark_vnode_delete_common(v); /* ディスクI-nodeの削除予約をかける */

	vfs_fs_mount_put(mnt);       /* マウントポイントの参照解放 */
	if ( res )
		return 0;  /* 最終参照者だった       */

	return -EAGAIN;    /* 最終参照者ではなかった */

put_mount_out:
	vfs_fs_mount_put(mnt);  /* マウントポイントの参照解放 */

error_out:
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

	/* v-node解放中でなければ, 利用カウンタを加算
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

	kassert( v->v_mount != NULL ); /* v-nodeテーブルに登録されていることを確認 */

	res = dec_vnode_ref_nolock(v); /* 参照カウンタをデクリメントする */
	return res;
}

/**
   v-nodeのファイルディスクリプタからの利用数を加算する
   @param[in] v  v-node情報
   @retval    真 v-nodeのファイルディスクリプタからの利用数を加算した
   @retval    偽 v-nodeのファイルディスクリプタからの利用数を加算できなかった
 */
bool
vfs_vnode_fduse_inc(vnode *v) {
	bool           res;

	mutex_lock(&v->v_mtx);  /* v-nodeのロックを獲得 */

	res = vfs_vnode_ref_inc(v);  /* v-nodeの参照数を加算 */
	if ( !res )
		goto unlock_out;

	++v->v_fduse;  /* ファイルディスクリプタからの利用数を加算 */

	mutex_unlock(&v->v_mtx);  /* v-nodeのロックを解放 */

	return true;

unlock_out:
	mutex_unlock(&v->v_mtx);  /* v-nodeのロックを解放 */
	return false;
}

/**
   v-nodeのファイルディスクリプタ利用数を減算する
   @param[in] v  v-node情報
   @retval    真 v-nodeの最終参照者だった
   @retval    偽 v-nodeの最終参照者でない
 */
bool
vfs_vnode_fduse_dec(vnode *v){
	bool           res;

	mutex_lock(&v->v_mtx);  /* v-nodeのロックを獲得 */

	--v->v_fduse;  /* ファイルディスクリプタからの利用数を加算 */
	res = dec_vnode_ref_nolock(v); /* 参照カウンタをデクリメントする */

	mutex_unlock(&v->v_mtx);  /* v-nodeのロックを解放 */

	return res;
}

/**
   mntid, vnidをキーとしてv-nodeを検索し, 参照を得る
   @param[in] mntid マウントポイントID
   @param[in]  vnid v-node ID
   @param[out] outv v-nodeを指し示すポインタのアドレス
   @retval  0       正常終了
   @retval -EINVAL  不正なマウントポイントIDを指定した
   @retval -ENOMEM  メモリ不足
   @retval -ENOENT  ディスクI-node読み取りに失敗した
   @retval -EINTR   v-node待ち合わせ中にイベントを受信した
   @retval -EBUSY   アンマウント中のボリュームだった
 */
int
vfs_vnode_get(vfs_mnt_id mntid, vfs_vnode_id vnid, vnode **outv){
	int             rc;
	fs_mount      *mnt;
	vnode           *v;

	rc = vfs_fs_mount_get(mntid, &mnt); /* マウントポイントの参照獲得 */
	if ( rc != 0 ) {

		rc = -EINVAL;
		goto error_out;
	}

	rc = get_vnode(mnt, vnid, &v);  /* v-nodeの参照を得る */
	if ( rc != 0 )
		goto put_mount_out;  /* v-nodeが見つからなかった */

	vfs_fs_mount_put(mnt);  /* マウントポイントの参照解放 */

	if ( outv != NULL )
		*outv = v;  /* v-nodeを返却 */

	return 0;

put_mount_out:
	vfs_fs_mount_put(mnt);  /* マウントポイントの参照解放 */

error_out:
	return rc;
}

/**
   v-nodeキャッシュからv-nodeを除去する
   @param[in] v     操作対象のv-node
   @retval  0       正常終了
   @retval -EBUSY   アンマウント中のボリュームだった
 */
int
vfs_vnode_ptr_put(vnode *v){
	int             rc;
	bool           res;

	kassert( v->v_mount != NULL );

	res = vfs_fs_mount_ref_inc(v->v_mount);  /* マウントテーブル操作用に参照を加算 */
	kassert( res );

	/*
	 * v-nodeの参照を解放する
	 */
	res = vfs_vnode_ref_inc(v);  /* 参照解放処理用に参照を獲得する */
	if ( !res ) {

		rc = 0; /* すでに削除中のv-nodeだった */
		goto put_mount_out;
	}

	/* マウント情報からの参照を解放してv-nodeを参照するスレッドがいなくなった時点で
	 * マウント情報内のv-nodeキャッシュから外せるようにする
	 */
	res = vfs_vnode_ref_dec(v);
	kassert( !res );      /* 最終参照ではないはず  */

	res = vfs_vnode_ref_dec(v);  /* 参照解放処理用の参照を解放する */

	/* @note 削除予約済みv-nodeの場合, 削除予約フラグを設定した時点で
	 * マウントテーブルへの参照を減算しており, かつ,
	 * 上記のv-node参照解放処理の延長でマウントテーブル操作用の参照を
	 * 解放済みであるため, 本関数でマウントテーブル操作用の参照を解放する必要はない。
	 */
	if ( !res ) {

		vfs_fs_mount_ref_dec(v->v_mount);  /* マウントポイントの参照解放 */
		return -EAGAIN;    /* 最終参照者ではなかった */
	}

	return 0;  /* 最終参照者だった       */

put_mount_out:
	vfs_fs_mount_ref_dec(v->v_mount);  /* マウントポイントの参照解放 */
	return rc;
}

/**
   v-nodeの削除フラグをセットする
   @param[in] v      操作対象のv-node
   @retval    0      v-nodeをマウント情報から削除した
   @retval   -ENOENT 操作対象のv-nodeの参照を獲得できなかった
   @retval   -EAGAIN v-nodeの最終参照者ではなかった
 */
int
vfs_vnode_ptr_remove(vnode *v){
	int             rc;
	bool           res;

	/*
	 * v-nodeを削除する
	 */
	res = vfs_vnode_ref_inc(v);  /* 削除予約処理用に参照を獲得する */
	if ( !res ) {

		rc = -ENOENT; /* すでに解放中のv-nodeだった */
		goto error_out;
	}

	/* v-nodeの削除予約をかける */
	rc = remove_vnode(v->v_mount->m_id, v->v_id);

	vfs_vnode_ref_dec(v);  /* 削除予約処理用の参照を解放する */

	if ( !res )
		return -EAGAIN;  /* v-nodeの最終参照者ではなかった */

	return 0;

error_out:
	return rc;
}

/**
   v-nodeを更新済みにする
   @param[in] v 操作対象のv-node
 */
void
vfs_mark_dirty_vnode(vnode *v) {
	bool res;

	res = mark_vnode_flag(v, VFS_VFLAGS_DIRTY); /* ダーティフラグをセットする */
	kassert( res );
}

/**
   v-nodeの更新済みフラグを落とす
   @param[in] v 操作対象のv-node
 */
void
vfs_unmark_dirty_vnode(vnode *v) {
	bool res;

	res = unmark_vnode_flag(v, VFS_VFLAGS_DIRTY);  /* ダーティフラグを落とす */
	kassert( res );
}

/**
   v-nodeが更新されていることを確認する
   @param[in] v  操作対象のv-node
   @retval    真 更新済みのv-node
   @retval    偽 更新済みのv-nodeでない
 */
bool
vfs_is_dirty_vnode(vnode *v) {
	bool res;

	res = vfs_vnode_ref_inc(v);  /* 参照を獲得する */
	if ( !res )
		return false;  /* 削除中のv-node */

	mutex_lock(&v->v_mtx);

	res = check_vnode_flags_nolock(v, VFS_VFLAGS_DIRTY);  /* ダーティフラグを確認する */

	mutex_unlock(&v->v_mtx);

	vfs_vnode_ref_dec(v);  /* 参照を解放する */

	return res;
}

/**
   v-nodeをロックする
   @param[in] v 操作対象のv-node
   @retval  0       正常終了
   @retval -ENOENT  削除中のv-nodeをロックしようとした
   @retval -EINTR   v-node待ち合わせ中にイベントを受信した
   @retval -ENOMEM  メモリ不足
   @note LO: マウントポイントロック, v-nodeロックの順にロックを獲得する
 */
int
vfs_vnode_lock(vnode *v) {
	int             rc;
	bool           res;
	wque_reason reason;

	res = vfs_vnode_ref_inc(v);  /* 参照を獲得する */
	if ( !res )
		return -ENOENT;  /* 削除中のv-node */

	do {
		mutex_lock(&v->v_mount->m_mtx);  /* マウントポイントのロックを獲得 */

		rc = trylock_vnode(v);  /*  v-nodeのロックを試みる  */

		if ( rc != 0 ) {

			kassert( rc == -EBUSY );

			/*  他のスレッドがv-nodeを使用中の場合は
			 *  v-nodeの開放を待ち合わせる
			 */
			reason = wque_wait_on_event_with_mutex(&v->v_waiters,
			    &v->v_mount->m_mtx);
			if ( reason == WQUE_DELIVEV ) {

				rc = -EINTR;   /* イベントを受信した */
				goto unlock_out;
			}
		}

		mutex_unlock(&v->v_mount->m_mtx); /* マウントポイントのロックを解放 */
	}while( rc != 0 );

	vfs_vnode_ref_dec(v);  /* 参照を解放する */

	return 0;

unlock_out:
	mutex_unlock(&v->v_mount->m_mtx);  /* マウントポイントのロックを解放 */
	vfs_vnode_ref_dec(v);  /* 参照を解放する */

	return rc;
}

/**
   v-nodeをアンロックする
   @param[in] v 操作対象のv-node
   @note LO: マウントポイントロック, v-nodeロックの順にロックを獲得する
 */
void
vfs_vnode_unlock(vnode *v) {
	bool res;

	/* マウントポイントの参照獲得
	 */

	res = vfs_vnode_ref_inc(v);  /* 参照を獲得する */
	if ( !res )
		return;  /* 削除中のv-node */

	mutex_lock(&v->v_mount->m_mtx);  /* マウントポイントのロックを獲得 */

	/* ロック済みv-nodeであることを確認 */
	kassert( check_vnode_flags_nolock(v, VFS_VFLAGS_BUSY) );

	res = unmark_vnode_busy(v); /* v-nodeのロックを解除する */
	kassert( res );

	mutex_unlock(&v->v_mount->m_mtx); /* マウントポイントのロックを解放 */

	vfs_vnode_ref_dec(v);  /* 参照を解放する */

	return ;
}

/**
   自スレッドがv-nodeをロックしていることを確認する
   @param[in] v 操作対象のv-node
   @retval    真     自スレッドがv-nodeをロックしている
   @retval    偽     自スレッドがv-nodeをロックしていない
   @note LO: マウントポイントロック, v-nodeロックの順にロックを獲得する
 */
bool
vfs_vnode_locked_by_self(vnode *v) {
	bool  rc;
	bool res;

	/* マウントポイントの参照獲得
	 */

	res = vfs_vnode_ref_inc(v);  /* 参照を獲得する */
	if ( !res )
		return false;  /* 削除中のv-node */

	mutex_lock(&v->v_mount->m_mtx);  /* マウントポイントのロックを獲得 */

	/* 自スレッドがロックを獲得していることを確認 */
	rc = check_vnode_locked_by_self_nolock(v);

	mutex_unlock(&v->v_mount->m_mtx); /* マウントポイントのロックを解放 */

	vfs_vnode_ref_dec(v);  /* 参照を解放する */

	return rc;
}

/**
   v-nodeに関連付けられたデータを書き戻す
   @param[in] v  v-node情報
   @retval    0      正常終了
   @retval   -ENOENT 解放中のv-nodeを指定した
   @retval   -EINTR  v-node待ち合わせ中にイベントを受信した
   @retval   -ENOMEM メモリ不足
 */
int
vfs_vnode_fsync(vnode *v){
	int        rc;
	int   sync_rc;
	bool      res;

	res = vfs_vnode_ref_inc(v); /* v-nodeへの参照を獲得 */
	if ( !res ) {

		rc = -ENOENT; /* すでに解放中のv-nodeだった */
		goto error_out;
	}

	if ( ( v->v_mount != NULL )
	    && ( v->v_mount->m_mount_flags & VFS_MNT_RDONLY ) ) {

		rc = -EROFS;  /* 読み取り専用でマウントされている */
		goto vnode_dec_out;
	}

	rc = vfs_vnode_lock(v);  /* v-nodeの更新処理ロックを獲得 */
	if ( rc != 0 )
		goto vnode_dec_out;

	sync_rc = fsync_vnode_common(v);  /* データブロックを書き戻す */
	if ( ( sync_rc == 0 ) && vfs_is_dirty_vnode(v) ) {

		sync_rc = v->v_mount->m_fs->c_calls->fs_putvnode(v->v_mount->m_fs_super,
		    v->v_id, v->v_fs_vnode);  /* v-nodeを書き戻す */
		if ( sync_rc == 0 )
			vfs_unmark_dirty_vnode(v);  /* v-node書き出し済み */
	}
	vfs_vnode_unlock(v);  /* v-nodeの更新処理ロックを解放 */

	vfs_vnode_ref_dec(v);	 /* v-nodeへの参照を解放 */

	return sync_rc;

vnode_dec_out:
	vfs_vnode_ref_dec(v);	 /* v-nodeへの参照を解放 */

error_out:
	return rc;
}

/**
   v-nodeのマウントIDを比較する
   @param[in] v1  v-node情報1
   @param[in] v2  v-node情報2
   @retval   -1   v1のマウントIDがv2のマウント IDより小さい
   @retval    1   v1のマウントIDがv2のマウント IDより大きい
   @retval    0   v1のマウントIDとv2のマウントIDが等しい
*/
int
vfs_vnode_mnt_cmp(vnode *v1, vnode *v2){
	int   rc;
	bool res;

	res = vfs_vnode_ref_inc(v1); /* v-node v1への参照を獲得 */
	kassert( res );

	res = vfs_vnode_ref_inc(v2); /* v-node v2への参照を獲得 */
	kassert( res );

	/* マウントIDを比較する */
	rc = compare_mount_id_common(v1->v_mount, v2->v_mount);

	vfs_vnode_ref_dec(v2);  /* v-node v2への参照を解放 */
	vfs_vnode_ref_dec(v1);  /* v-node v1への参照を解放 */
	return rc;
}

/**
   v-nodeを比較する
   @param[in] v1  v-node情報1
   @param[in] v2  v-node情報2
   @retval   正   vnode v1のマウントID, v-node IDがv2のマウントID, v-node IDより大きい
   @retval   負   vnode v1のマウントID, v-node IDがv2のマウントID, v-node IDより小さい
   @retval    0   vnode v1のマウントID, v-node IDがv2のマウントID, v-node IDと等しい
*/
int
vfs_vnode_cmp(vnode *v1, vnode *v2){
	int   rc;
	bool res;

	res = vfs_vnode_ref_inc(v1); /* v-node v1への参照を獲得 */
	kassert( res );

	res = vfs_vnode_ref_inc(v2); /* v-node v2への参照を獲得 */
	kassert( res );

	kassert(v1->v_mount != NULL);  /* 登録済みv-nodeであることを確認 */
	kassert(v2->v_mount != NULL);  /* 登録済みv-nodeであることを確認 */

	rc = vfs_vnode_mnt_cmp(v1, v2);  /* マウント IDを比較する */
	if ( rc != 0 )
		goto cmp_out;  /* マウントIDの大小を返却 */

	/* v-node IDを比較する */
	rc = compare_vnode_id_common(v1, v2);

cmp_out:
	vfs_vnode_ref_dec(v2);  /* v-node v2への参照を解放 */
	vfs_vnode_ref_dec(v1);  /* v-node v1への参照を解放 */

	return rc;
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
   マウントポイント情報への参照を得る
   @param[in]  mntid   マウントID
   @param[out] mountp  マウントポイント情報を指し示すポインタのアドレス
   @retval    0      正常終了
   @retval   -ENOENT マウントIDに対応するマウントポイント情報が見つからなかった
 */
int
vfs_fs_mount_get(vfs_mnt_id mntid, fs_mount **mountp) {
	int           rc;
	fs_mount    *mnt;

	mutex_lock(&g_mnttbl.mt_mtx);  /* マウントテーブルをロックする */

	rc = get_fs_mount_nolock(mntid, &mnt); 	/* マウント情報を検索 */
	if ( rc != 0 ) {

		rc = -ENOENT;  /* マウントポイント情報が見つからなかった */
		goto unlock_out;
	}

	if ( mountp != NULL )
		*mountp = mnt;  /* マウントポイントを返却 */

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
   システムルートディレクトリv-nodeへの参照を得る
   @param[out] rootp     システムルートディレクトリv-nodeを指し示すポインタのアドレス
   @retval  0      正常終了
   @retval -ENOMEM メモリ不足
   @retval -ENOENT 指定された名前のファイルシステムまたはパスが見つからなかった
   @retval -EBUSY  既に対象ボリュームのルートマウントポイントになっている
   @retval -ENODEV システムルートディレクトリが存在しない
   @note LO: マウントテーブルロック, マウントポイントロック, v-nodeロックの順に
   ロックを獲得する
*/
int
vfs_fs_mount_system_root_vnode_get(vnode **rootp){
	int   rc;
	bool res;

	kassert( rootp != NULL );

	mutex_lock(&g_mnttbl.mt_mtx);

	if ( g_mnttbl.mt_root == NULL ) {

		rc = -ENODEV;  /* システムルートディレクトリが存在しない */
		goto unlock_out;
	}

	res = vfs_vnode_ref_inc(g_mnttbl.mt_root); /* 参照をインクリメント */
	if ( !res ) {

		rc = -ENODEV;  /* システムルートディレクトリのアンマウント中 */
		goto unlock_out;
	}

	*rootp = g_mnttbl.mt_root;  /* システムルートディレクトリのv-nodeを返却 */
	mutex_unlock(&g_mnttbl.mt_mtx);

	return 0;

unlock_out:
	mutex_unlock(&g_mnttbl.mt_mtx);
	return rc;
}

/**
   ファイルシステムをファイルシステム名を指定してマウントする
   @param[in] ioctxp   I/Oコンテキスト
   @param[in] path     マウント先のパス名
   @param[in] dev      マウントするデバイス
   @param[in] fs_name  ファイルシステム名
   @param[in] args     マウントオプション
   @retval  0      正常終了
   @retval -ENOMEM メモリ不足
   @retval -ENOENT 指定された名前のファイルシステムまたはパスが見つからなかった
   @retval -EINVAL ファイルシステムのマウントに失敗した(オプションが不正?)
   引数で指定されたファイルパスがディレクトリを指していない
   @retval -EIO     ファイルシステムマウント時にI/Oエラーが発生した
   @retval -EBUSY   既に対象ボリュームのルートマウントポイントになっている
   @retval -ENODEV  下位のファイルシステムがルートv-nodeを設定しなかった
   @retval -ENOTDIR ディレクトリでないパスを指定した
   @note LO: マウントテーブルロック, マウントポイントロック, v-nodeロックの順に
   ロックを獲得する
*/
int
vfs_mount_with_fsname(vfs_ioctx *ioctxp, char *path, dev_id dev, const char *fs_name,
	  void *args){
	int                       rc;
	fs_mount              *mount;
	vnode         *covered_vnode;
	vfs_vnode_id         root_id;
	void               *mnt_args;
	vfs_mnt_flags  new_mnt_flags;

	kassert( path != NULL );

	mutex_lock(&g_mnttbl.mt_mtx);

	if ( ( ( g_mnttbl.mt_root == NULL ) || ( ioctxp == NULL ) )
	    && ( strcmp(path, "/") != 0 ) ) {

		/* ルートディレクトリがマウントされていないため
		 * パス検索不能
		 */
		rc = -ENOENT;
		goto unlock_out;
	}

	covered_vnode = NULL;  /* マウントディレクトリのv-nodeポインタを初期化 */

	if ( g_mnttbl.mt_root != NULL ) {

		/* 通常のマウント処理
		 * ルートディレクトリをアンマウント時には, 競合が発生するスレッドはいない.
		 * ルートディレクトリマウント済みの場合はロックを外しても,
		 * 本処理以外のスレッドがルートディレクトリをアンマウントすることはできない
		 */
		mutex_unlock(&g_mnttbl.mt_mtx);  /* パス検索のためロックを解放する */
		/* マウントポイントディレクトリの参照を得る
		 * マウントが正常終了した場合, 本関数で得た参照は,
		 * アンマウント時まで解放しない.
		 * これによって, マウント前に他のスレッドが参照を得ていた場合でも
		 * マウントポイントディレクトリのv-nodeが解放されないようにする。
		 */
		rc = vfs_path_to_vnode(ioctxp, path, &covered_vnode);
		if ( rc != 0 ) {

			/* マウントポイントディレクトリのパス検索に失敗した */
			if ( ( rc != -ENOMEM ) && ( rc != -EIO ) )
				rc = -EINVAL;
			goto error_out;
		}

		mutex_lock(&g_mnttbl.mt_mtx);   /* マウント処理のためロックを獲得する */
	}

	/*
	 * マウント情報割当て
	 */
	rc = alloc_new_fsmount(path, fs_name, &mount);
	if ( rc != 0 )
		goto unlock_out;

	kassert( is_valid_fs_calls( mount->m_fs->c_calls ) );

	mnt_args = NULL;  /* ルートマウント時はマウントオプションにNULLを引き渡す */
	if ( g_mnttbl.mt_root != NULL ) {

		/*
		 * 通常のマウント処理
		 */
		if ( covered_vnode == NULL ) {

			/* マウントポイントのパスが見つからなかった */
			rc = -EINVAL;
			goto free_mount_out;
		}

		/* 対象のvnodeがディレクトリでない場合, 引数異常で復帰 */
		if ( !S_ISDIR(covered_vnode->v_mode) ) {

			/* マウントポイントのv-nodeの参照を解放  */
			dec_vnode_ref_nolock(covered_vnode);
			rc = -EINVAL;
			goto free_mount_out;
		}

		/* システムルートディレクトリでなく, かつ,
		 * マウント対象ボリュームのルートになっている場合は,
		 * 多重ルートマウントエラーで復帰
		 */
		if ( ( covered_vnode != g_mnttbl.mt_root ) &&
		     ( covered_vnode->v_mount->m_root == covered_vnode ) ){

			/* マウントディレクトリへのv-nodeの参照を解放  */
			dec_vnode_ref_nolock(covered_vnode);
			rc = -EBUSY;
			goto free_mount_out;
		}

		mount->m_mount_point = covered_vnode; /* マウントポイントとなるv-nodeを記録 */
		/* マウントポイントのv-nodeにマウント情報を設定 */
		covered_vnode->v_mount_on = mount;
		mnt_args = args;  /* マウントオプションを引き渡す */
	} else {

		/* ルートマウント時は, マウントポイントをNULLに設定 */
		mount->m_mount_point = NULL;
	}

	root_id = VFS_DEFAULT_ROOT_VNID;  /* デフォルトのroot v-node IDを仮に設定 */

	/*
	 * ファイルシステム固有のマウント処理を実施
	 */
	if ( mount->m_fs->c_calls->fs_mount != NULL ) {

		new_mnt_flags = VFS_MNT_MNTFLAGS_NONE;  /* マウントフラグ値をクリア */
		rc = mount->m_fs->c_calls->fs_mount(mount->m_id,
		    dev, mnt_args, &mount->m_fs_super, &new_mnt_flags, &root_id);
		if ( rc != 0 ) {

			if ( ( rc != -ENOMEM ) && ( rc != -EIO ) )
				rc = -EINVAL;  /*  マウントオプション不正  */
			goto unref_covers_vnode_out;
		}

		/* 下位のファイルシステムで設定可能なフラグのみを取得 */
		new_mnt_flags &= VFS_MNT_FSLAYER_MASK;
		mount->m_mount_flags |= new_mnt_flags;  /* マウントフラグを設定 */
	}

	/*
	 * root v-nodeへの参照を獲得
	 */
	rc = get_vnode(mount, root_id, &mount->m_root);
	if ( rc != 0 )
		goto unmount_out;

	if ( mount->m_root == NULL ) {

		/* 下位のファイルシステムがルートv-nodeを設定しなかった */
		rc = -ENODEV;
		goto unmount_out;
	}

	if ( !S_ISDIR(mount->m_root->v_mode) ) {

		rc = -ENOTDIR; /* ディレクトリでないv-nodeを指定した */
		goto put_root_vnode_out;
	}

	mount->m_dev = dev; /* デバイスIDを設定 */

	/* マウント情報をマウントテーブルに登録し,
	 * マウントIDを割当てる
	 */
	add_fsmount_to_mnttbl_nolock(mount);

	if ( mount->m_mount_point == NULL ) {

		/* ルートマウント時は, マウントテーブルのルートディレクトリを更新 */
		kassert( g_mnttbl.mt_root == NULL );
		g_mnttbl.mt_root = mount->m_root;
		g_mnttbl.mt_root_mntid = mount->m_id;
	}

	mutex_unlock(&g_mnttbl.mt_mtx);

	return 0;

put_root_vnode_out:
	dec_vnode_ref_nolock(mount->m_root);

unmount_out:  /*  ファイルシステム固有のアンマウント処理を実施  */
	if ( mount->m_fs->c_calls->fs_unmount != NULL )
		mount->m_fs->c_calls->fs_unmount(mount->m_fs_super);

unref_covers_vnode_out: /* 通常マウント時はマウントポイントの参照を解放  */
	if ( mount->m_mount_point != NULL )  /* マウントポイントの参照を解放  */
		dec_vnode_ref_nolock(mount->m_mount_point);

free_mount_out:  /*  マウント情報を解放  */
	free_fsmount(mount);

unlock_out:
	mutex_unlock(&g_mnttbl.mt_mtx);

error_out:
	return rc;
}

/**
   ルートファイルシステムのアンマウント
   @retval  0      正常終了
   @retval -ENOENT 指定されたパスが見つからなかった
   @retval -EINVAL マウントポイントではないパスを指定した
   @retval -EBUSY  ボリューム内のファイルが使用中
*/
int
vfs_unmount_rootfs(void){
	fs_mount  *mount;
	int           rc;
	bool         res;

	 /* マウント情報への参照獲得 */
	rc = get_fs_mount_nolock(g_mnttbl.mt_root_mntid, &mount);
	if ( rc != 0 ) {

		rc = -ENOENT;  /* マウントポイント情報が見つからなかった */
		goto error_out;
	}

	mutex_lock(&g_mnttbl.mt_mtx); /* マウントテーブルをロック  */

	if ( mount->m_root != g_mnttbl.mt_root ) {  /*  マウントポイントではない  */

		rc = -EINVAL;
		goto unlock_out;
	}

	/*  アンマウント前に参照を獲得し,
	 *  参照カウンタを3以上にして
	 *  unmount_commonを呼び出す
	 *  条件を整える
	 */
	vfs_vnode_ref_inc(g_mnttbl.mt_root);

	mutex_unlock(&g_mnttbl.mt_mtx); /* マウントテーブルをアンロック  */

	rc = unmount_common(g_mnttbl.mt_root);  /* アンマウント処理を実行  */

	vfs_vnode_ref_dec(g_mnttbl.mt_root); /*  v-node削除用に参照を解放する */

	/* アンマウント失敗(上記の参照解放によりマウント操作用参照を解放済み) */
	if ( rc != 0 )
		goto put_mount_out;

	res = vfs_vnode_ref_dec(g_mnttbl.mt_root); /*  参照を解放する */
	if ( res ) {  /* ルートディレクトリ情報を再初期化 */

		g_mnttbl.mt_root = NULL;
		g_mnttbl.mt_root_mntid = VFS_INVALID_MNTID;
	}

	vfs_fs_mount_put(mount);  /* マウントポイントの参照を解放する */

	return 0;

unlock_out:
	mutex_unlock(&g_mnttbl.mt_mtx); /* マウントテーブルをアンロック  */
put_mount_out:
	vfs_fs_mount_put(mount);  /* マウントポイントの参照を解放する */
error_out:
	return rc;
}
/**
   ファイルシステムのアンマウント
   @param[in] ioctxp   I/Oコンテキスト
   @param[in] path     マウント先のパス名
   @retval  0      正常終了
   @retval -ENOENT 指定されたパスが見つからなかった
   @retval -EINVAL マウントポイントではないパスを指定した
   @retval -EBUSY  ボリューム内のファイルが使用中
 */
int
vfs_unmount(vfs_ioctx *ioctxp, char *path){
	vnode         *v;
	int           rc;

	/*
	 * 指示されたパスのvnodeを獲得する
	 */
	rc = vfs_path_to_vnode(ioctxp, path, &v);
	if ( rc != 0 ) {

		rc = -ENOENT;
		goto out;
	}

	/*
	 * ボリュームをアンマウントする
	 */
	rc = unmount_common(v);
	if ( rc != 0 )
		goto unref_out;

	vfs_vnode_ref_dec(v);   /* v-node削除用に参照を解放する */

	/* vfs_path_to_vnodeで獲得した参照を解放
	 * @note マウント前に対象のディレクトリにアクセスしていたプロセスが
	 * 存在する可能性があるので, 以下の参照解放後にv-nodeの参照数が0になるとは
	 * 限らない。
	 */
	vfs_vnode_ref_dec(v);

	return 0;

unref_out:
	vfs_vnode_ref_dec(v);   /* vfs_path_to_vnodeで獲得した参照を解放  */

out:
	return rc;
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

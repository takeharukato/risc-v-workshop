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

#include <kern/vfs-if.h>

#include <kern/ktest.h>
#define TST_VFS_MOUNT_MAGIC (0xfeedfeed)
#define TST_VFS_MOUNT_VN_MAX (5)
#define TST_VFS_MOUNT_ROOT_VNID (2)
#define TST_VFS_MOUNT_DEV_VNID  (3)
#define TST_VFS_MOUNT_DEV       (4)

static ktest_stats tstat_vfs_mount=KTEST_INITIALIZER;

typedef struct _tst_vfs_mount_super{
	uint32_t       s_ninodes;  /**< 総I-node数(ファイル数 単位:個) */
	uint32_t         s_magic;  /**< スーパブロックのマジック番号     */
	uint32_t         s_state;  /**< マウント状態                     */
}tst_vfs_mount_super;

typedef struct _tst_vfs_mount_vnode{
	uint16_t                     i_mode;  /**< ファイル種別/保護属性            */
	uint16_t                   i_nlinks;  /**< ファイルのリンク数 (単位:個)     */
	uint16_t                      i_uid;  /**< ファイル所有者のユーザID         */
	uint16_t                      i_gid;  /**< ファイル所有者のグループID       */
	uint32_t                     i_size;  /**< ファイルサイズ (単位:バイト)     */
	uint32_t                    i_atime;  /**< 最終アクセス時刻 (単位:UNIX時刻) */
	uint32_t                    i_mtime;  /**< 最終更新時刻 (単位:UNIX時刻)     */
	uint32_t                    i_ctime;  /**< 最終属性更新時刻 (単位:UNIX時刻) */
	uint32_t                     i_vnid;  /**< v-node ID                        */
}tst_vfs_mount_vnode;

static tst_vfs_mount_super g_super;
static tst_vfs_mount_vnode vn_array[TST_VFS_MOUNT_VN_MAX];

static int
tst_vfs_fs_mount(vfs_fs_super *fs_super, vfs_mnt_id id, dev_id dev, void *args, 
		 vfs_vnode_id *root_vnid){

	if ( dev != TST_VFS_MOUNT_DEV )
		return -ENOENT;

	*fs_super = (vfs_fs_super)&g_super;
	*root_vnid = (vfs_vnode_id)TST_VFS_MOUNT_ROOT_VNID;

	ktest_pass( &tstat_vfs_mount );

	return 0;
}

static int
tst_vfs_fs_unmount(vfs_fs_super fs_super){
	tst_vfs_mount_super *super;

	super = (tst_vfs_mount_super *)fs_super;
	if ( super->s_magic == TST_VFS_MOUNT_MAGIC )
		ktest_pass( &tstat_vfs_mount );
	else
		ktest_fail( &tstat_vfs_mount );

	return 0;
}

static void
tst_vfs_mount_init_vnodes(void){
	int i;

	g_super.s_magic = TST_VFS_MOUNT_MAGIC;
	g_super.s_ninodes = TST_VFS_MOUNT_VN_MAX;
	for(i = 0; TST_VFS_MOUNT_VN_MAX > i; ++i) {

		vn_array[i].i_vnid = i;
	}
}
static int
tst_vfs_mount_getvnode(vfs_fs_super fs_priv, vfs_vnode_id id, vfs_fs_vnode *v){
	
	if ( ( 0 > id) || ( id >= TST_VFS_MOUNT_VN_MAX ) )
		return -EINVAL;

	*v = (vfs_fs_vnode)&vn_array[id];

	return 0;
}
static int
tst_vfs_mount_putvnode(vfs_fs_super fs_priv, vfs_fs_vnode v){
	tst_vfs_mount_vnode    *vn;
	
	vn = (tst_vfs_mount_vnode *)v;
	if ( ( 0 > vn->i_vnid ) || ( vn->i_vnid >= TST_VFS_MOUNT_VN_MAX ) )
		return -EINVAL;

	return 0;
}
static int
tst_vfs_mount_lookup(vfs_fs_super fs_super, vfs_fs_vnode dir,
		 const char *name, vfs_vnode_id *id, vfs_fs_mode *modep){
	tst_vfs_mount_super *super;
	tst_vfs_mount_vnode    *dv;

	super = (tst_vfs_mount_super *)fs_super;
	if ( super->s_magic != TST_VFS_MOUNT_MAGIC )
		return -EINVAL;
	
	dv = (tst_vfs_mount_vnode *)dir;
	if ( dv->i_vnid != TST_VFS_MOUNT_ROOT_VNID )
		return -EINVAL;
	if ( ( strcmp(name, ".") == 0 ) || ( strcmp(name, "..") == 0 ) ) 
		*id = TST_VFS_MOUNT_ROOT_VNID;
	else if ( strcmp(name, "dev") == 0 )
		*id = TST_VFS_MOUNT_DEV_VNID;
	else
		return -ENOENT;

	return 0;
}
static int
tst_vfs_mount_seek(vfs_fs_super fs_priv, vfs_fs_vnode v, vfs_file_private file_priv, 
	       off_t pos, off_t *new_posp, int whence){

	return 0;
}
static fs_calls tst_vfs_mount_calls={
	.fs_mount = tst_vfs_fs_mount,
	.fs_unmount = tst_vfs_fs_unmount,
	.fs_getvnode = tst_vfs_mount_getvnode,
	.fs_putvnode = tst_vfs_mount_putvnode,
	.fs_lookup = tst_vfs_mount_lookup,
	.fs_seek = tst_vfs_mount_seek,
};

static void
vfs_mount1(struct _ktest_stats *sp, void __unused *arg){
	int rc;

	tst_vfs_mount_init_vnodes();

	rc = vfs_register_filesystem("tst_vfs_mount", &tst_vfs_mount_calls);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	rc = vfs_mount(NULL, "/", TST_VFS_MOUNT_DEV, "tst_vfs_mount", NULL);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	vfs_unmount_rootfs();  /* rootfsのアンマウント */
}

void
tst_vfs_mount(void){

	ktest_def_test(&tstat_vfs_mount, "vfs_mount", vfs_mount1, NULL);
	ktest_run(&tstat_vfs_mount);
}


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

static ktest_stats tstat_vfs_fstbl=KTEST_INITIALIZER;

static int
tst_vfs_fstbl_getvnode(vfs_fs_super fs_priv, vfs_vnode_id id, vfs_fs_mode *modep,
    vfs_fs_vnode *v){

	return 0;
}
static int
tst_vfs_fstbl_putvnode(vfs_fs_super fs_priv, vfs_fs_vnode v){

	return 0;
}
static int
tst_vfs_fstbl_lookup(vfs_fs_super fs_priv, vfs_fs_vnode dir,
		 const char *name, vfs_vnode_id *id){

	return 0;
}
static int
tst_vfs_fstbl_seek(vfs_fs_super fs_priv, vfs_fs_vnode v, vfs_file_private file_priv, 
	       off_t pos, off_t *new_posp, int whence){

	return 0;
}
static 	fs_calls tst_vfs_fstbl_calls={
		.fs_getvnode = tst_vfs_fstbl_getvnode,
		.fs_putvnode = tst_vfs_fstbl_putvnode,
		.fs_lookup = tst_vfs_fstbl_lookup,
		.fs_seek = tst_vfs_fstbl_seek,
};

static 	fs_calls bad_calls={
		.fs_getvnode = tst_vfs_fstbl_getvnode,
		.fs_putvnode = tst_vfs_fstbl_putvnode,
		.fs_lookup = tst_vfs_fstbl_lookup,
		.fs_seek = NULL,
};

static void
vfs_fstbl1(struct _ktest_stats *sp, void __unused *arg){
	int                  rc;
	fs_container *container;

	rc = vfs_register_filesystem("bad", &bad_calls);
	if ( rc == -EINVAL )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	rc = vfs_register_filesystem("tst_vfs_fstbl", &tst_vfs_fstbl_calls);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	rc = vfs_fs_get("bad", &container);
	if ( rc == -ENOENT )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	rc = vfs_fs_get("tst_vfs_fstbl", &container);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	vfs_fs_put(container);
	rc = vfs_unregister_filesystem(NULL);
	if ( rc == -EINVAL )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	rc = vfs_unregister_filesystem("tst_vfs_fstbl");
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	rc = vfs_unregister_filesystem("tst_vfs_fstbl");
	if ( rc == -ENOENT )
		ktest_pass( sp );
	else
		ktest_fail( sp );

}

void
tst_vfs_fstbl(void){

	ktest_def_test(&tstat_vfs_fstbl, "vfs_fstbl", vfs_fstbl1, NULL);
	ktest_run(&tstat_vfs_fstbl);
}


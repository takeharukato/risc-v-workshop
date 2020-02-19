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

int dummy_getvnode(vfs_fs_private fs_priv, vnode_id id, vfs_fs_vnode *v){

	return 0;
}
int dummy_putvnode(vfs_fs_private fs_priv, vfs_fs_vnode v){

	return 0;
}
int dummy_lookup(vfs_fs_private fs_priv, vfs_fs_vnode dir,
		 const char *name, vnode_id *id, fs_mode *modep){

	return 0;
}
int dummy_seek(vfs_fs_private fs_priv, vfs_fs_vnode v, vfs_file_private file_priv, 
	       off_t pos, off_t *new_posp, int whence){

	return 0;
}
static 	fs_calls dummy_calls={
		.fs_getvnode = dummy_getvnode,
		.fs_putvnode = dummy_putvnode,
		.fs_lookup = dummy_lookup,
		.fs_seek = dummy_seek,
};

static 	fs_calls bad_calls={
		.fs_getvnode = dummy_getvnode,
		.fs_putvnode = dummy_putvnode,
		.fs_lookup = dummy_lookup,
		.fs_seek = NULL,
};

static void
vfs_fstbl1(struct _ktest_stats *sp, void __unused *arg){
	int rc;
	bool res;
	fs_container *container;

	rc = vfs_register_filesystem("bad", &bad_calls);
	if ( rc == -EINVAL )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	rc = vfs_register_filesystem("dummy", &dummy_calls);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	rc = vfs_fs_get("bad", &container);
	if ( rc == -ENOENT )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	rc = vfs_fs_get("dummy", &container);
	if ( rc == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	res = vfs_fs_ref_inc(container);
	if ( res ) 
		ktest_pass( sp );
	else
		ktest_fail( sp );

	vfs_fs_put(container);
	res = vfs_fs_ref_dec(container);
	if ( res )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	
}

void
tst_vfs_fstbl(void){

	ktest_def_test(&tstat_vfs_fstbl, "vfs_fstbl", vfs_fstbl1, NULL);
	ktest_run(&tstat_vfs_fstbl);
}


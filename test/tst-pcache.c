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
#include <kern/fs-fsimg.h>
#include <kern/dev-pcache.h>

#include <kern/ktest.h>

static ktest_stats tstat_pcache=KTEST_INITIALIZER;

static void
pcache1(struct _ktest_stats *sp, void __unused *arg){
	int         rc;
	off_t      off;
	size_t  fssize;
	page_cache *pc;

	fssize=(uintptr_t)&_fsimg_end - (uintptr_t)&_fsimg_start;
	for(off = 0; fssize > off; off += PAGE_SIZE) {

		rc = pagecache_get(FS_FSIMG_DEVID, off,  &pc);
		if ( rc == 0 )
			ktest_pass( sp );
		else
			ktest_fail( sp );

		if ( PCACHE_IS_VALID(pc) )
			ktest_pass( sp );
		else
			ktest_fail( sp );
		
		pagecache_mark_dirty(pc);
		if ( PCACHE_IS_DIRTY(pc) )
			ktest_pass( sp );
		else
			ktest_fail( sp );
		pagecache_write(pc);
		if ( PCACHE_IS_CLEAN(pc) )
			ktest_pass( sp );
		else
			ktest_fail( sp );

		pagecache_put(pc);
	}
}

void
tst_pcache(void){

	ktest_def_test(&tstat_pcache, "pcache1", pcache1, NULL);
	ktest_run(&tstat_pcache);
}


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
#include <kern/dev-if.h>

#include <kern/ktest.h>

static ktest_stats tstat_bdev=KTEST_INITIALIZER;

static void
bdev1(struct _ktest_stats *sp, void __unused *arg){

}

void
tst_bdev(void){

	ktest_def_test(&tstat_bdev, "bdev1", bdev1, NULL);
	ktest_run(&tstat_bdev);
}

/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  test routine                                                      */
/*                                                                    */
/**********************************************************************/

#include <kern/kern-common.h>
#include <kern/ktest.h>
#include <kern/spinlock.h>

static ktest_stats tstat_spinlock=KTEST_INITIALIZER;

static spinlock g_lock=__SPINLOCK_INITIALIZER;

static void
spinlock1(ktest_stats *statp, void __unused *arg){
	spinlock lock;
	intrflags iflags;

	if ( g_lock.locked == 0 )
		ktest_pass(statp);
	else
		ktest_fail(statp);

	spinlock_init(&lock);
	if ( lock.locked == 0 )
		ktest_pass(statp);
	else
		ktest_fail(statp);

	spinlock_lock(&lock);
	if ( spinlock_locked_by_self(&lock) )
		ktest_pass(statp);
	else
		ktest_fail(statp);
	spinlock_unlock(&lock);
	if ( !spinlock_locked_by_self(&lock) )
		ktest_pass(statp);
	else
		ktest_fail(statp);

	spinlock_lock_disable_intr(&lock, &iflags);
	if ( spinlock_locked_by_self(&lock) )
		ktest_pass(statp);
	else
		ktest_fail(statp);

	spinlock_unlock_restore_intr(&lock, &iflags);
	if ( !spinlock_locked_by_self(&lock) )
		ktest_pass(statp);
	else
		ktest_fail(statp);

	spinlock_raw_lock_disable_intr(&lock, &iflags);
	if ( spinlock_locked_by_self(&lock) )
		ktest_pass(statp);
	else
		ktest_fail(statp);
	
	spinlock_raw_unlock_restore_intr(&lock, &iflags);
	if ( !spinlock_locked_by_self(&lock) )
		ktest_pass(statp);
	else
		ktest_fail(statp);

	return ;
}

void
tst_spinlock(void){

	ktest_def_test(&tstat_spinlock, "spinlock1", spinlock1, NULL);
	ktest_run(&tstat_spinlock);
}

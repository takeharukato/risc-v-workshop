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

#include <klib/atomic.h>

#define ATOMIC_VAL1  (0x12345678)
#define ATOMIC_VAL2  (0xffffffff)
#define ATOMIC_VAL3  (0xafafafaf)
#define ATOMIC_VAL4  (0xf0f0f0f0)
#define ATOMIC_VAL5  (0xfafafafa)
#define ATOMIC_VAL6  (0xfeedfeed)

static ktest_stats tstat_atomic=KTEST_INITIALIZER;

static void
atomic1(struct _ktest_stats *sp, void __unused *arg){
	atomic dest;
	atomic_val inc, v1, v2, res;
	void *p1, *p2, *pr;
	bool  rc;

	atomic_set(&dest, ATOMIC_VAL1);
	inc=1;
	kprintf("atomic add(%ld,%ld)", atomic_read(&dest), inc);
	res = atomic_add_fetch(&dest, inc);
	kprintf("=%ld\n", res);
	kprintf("now: %ld\n", atomic_read(&dest));
	if ( ( atomic_read(&dest) == (atomic_val)(ATOMIC_VAL1 + 1) ) &&
	    ( res == (atomic_val)(ATOMIC_VAL1) ) )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	atomic_set(&dest, ATOMIC_VAL2);
	inc=ATOMIC_VAL3;
	kprintf("atomic and(%lx,%lx)", atomic_read(&dest), inc);
	res = atomic_and_fetch(&dest, inc);
	kprintf("=%lx\n", res);
	kprintf("now: %lx\n", atomic_read(&dest));
	if ( ( atomic_read(&dest) == (atomic_val)( ATOMIC_VAL2 & ATOMIC_VAL3 ) ) &&
	    ( res == (atomic_val)(ATOMIC_VAL2) ) )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	atomic_set(&dest, ATOMIC_VAL4);
	inc=ATOMIC_VAL3;
	kprintf("atomic or(%lx,%lx)", atomic_read(&dest), inc);
	res = atomic_or_fetch(&dest, inc);
	kprintf("=%lx\n", res);
	kprintf("now: %lx\n", atomic_read(&dest));
	if ( ( atomic_read(&dest) == (atomic_val)( ATOMIC_VAL3 | ATOMIC_VAL4 ) ) &&
	    ( res == (atomic_val)(ATOMIC_VAL4) ) )
		ktest_pass( sp );

	atomic_set(&dest, ATOMIC_VAL5 );
	inc=ATOMIC_VAL3;
	kprintf("atomic xor(%lx,%lx)", atomic_read(&dest), inc);
	res = atomic_xor_fetch(&dest, inc);
	kprintf("=%lx\n", res);
	kprintf("now: %lx\n", atomic_read(&dest));
	if ( ( atomic_read(&dest) == (atomic_val)( ATOMIC_VAL5 ^ ATOMIC_VAL3 ) ) &&
		( res == (atomic_val)(ATOMIC_VAL5) ) )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	atomic_set(&dest, ATOMIC_VAL1);
	inc=1;
	kprintf("atomic sub(%ld,%ld)", atomic_read(&dest), inc);
	res = atomic_sub_fetch(&dest, inc);
	kprintf("=%ld\n", res);
	kprintf("now: %ld\n", atomic_read(&dest));
	if ( ( atomic_read(&dest) == (atomic_val)(ATOMIC_VAL1 - inc) ) &&
	    ( res == (atomic_val)(ATOMIC_VAL1) ) )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	atomic_set(&dest, 2);
	inc=1;
	kprintf("atomic sub_test1(%lx,%lx)", atomic_read(&dest), inc);
	rc = atomic_sub_and_test(&dest,1, &res);
	kprintf("=%s\n", (rc)?("True"):("False"));
	if ( ( atomic_read(&dest) == 1 ) && (!rc) )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	kprintf("atomic sub_test2(%lx,%lx)", atomic_read(&dest), inc);
	rc = atomic_sub_and_test(&dest,1, &res);
	kprintf("=%s\n", (rc)?("True"):("False"));
	if ( ( atomic_read(&dest) == 0 ) && ( rc ) )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	atomic_set(&dest, 1);
	inc=1;
	kprintf("atomic cmpxchg(%lx,%lx)", atomic_read(&dest), inc);
	kprintf("=%lx\n", atomic_cmpxchg_fetch(&dest, 1, 0));
	if ( atomic_read(&dest) == 0 )
		ktest_pass( sp );

	p1 = NULL;
	p2 = (void *)ATOMIC_VAL6;
	kprintf("atomic cmpxchg ptr(p1,p2) = (%p,%p)\n", p1, p2);
	pr = atomic_cmpxchg_ptr_fetch(&p1, NULL, p2);
	kprintf("atomic cmpxchg ptr(pr,p1,p2) = (%p,%p,%p)\n", pr, p1, p2);
	if ( ( pr == NULL ) && ( p1 == p2 ) )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	atomic_set(&dest, 1);
	v1 = 1;
	v2 = 2;
	kprintf("atomic try cmpxchg(v1,v2) = (%lx,%lx)\n", v1, v2);
	rc = atomic_try_cmpxchg_fetch(&dest, &v1, v2);
	kprintf("atomic try cmpxchg(rc,dest,v1,v2) = (%s,%lx,%lx,%lx)\n",
	    (rc)?("True"):("False"), atomic_read(&dest), v1, v2);
	if ( ( rc == true ) && ( atomic_read(&dest) == v2 ) )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	atomic_set(&dest, ATOMIC_VAL1);
	inc=1;
	kprintf("atomic add_return(%ld,%ld)", atomic_read(&dest), inc);
	res = atomic_add_return(&dest, inc);
	kprintf("=%ld\n", res);
	kprintf("now: %ld\n", atomic_read(&dest));
	if ( ( atomic_read(&dest) == (atomic_val)(ATOMIC_VAL1 + 1) ) &&
	    ( atomic_read(&dest) == res ) )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	atomic_set(&dest, ATOMIC_VAL1);
	inc=1;
	kprintf("atomic sub_return(%ld,%ld)", atomic_read(&dest), inc);
	res = atomic_sub_return(&dest, inc);
	kprintf("=%ld\n", res);
	kprintf("now: %ld\n", atomic_read(&dest));
	if ( ( atomic_read(&dest) == (atomic_val)(ATOMIC_VAL1 - 1) ) &&
	    ( atomic_read(&dest) == res ) )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	atomic_set(&dest, ATOMIC_VAL2);
	inc=ATOMIC_VAL3;
	kprintf("atomic and_return(%lx,%lx)", atomic_read(&dest), inc);
	res = atomic_and_return(&dest, inc);
	kprintf("=%lx\n", res);
	kprintf("now: %lx\n", atomic_read(&dest));
	if ( ( atomic_read(&dest) == (atomic_val)(ATOMIC_VAL3 & ATOMIC_VAL3 ) ) &&
	    ( atomic_read(&dest) == res ) )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	atomic_set(&dest, ATOMIC_VAL4);
	inc=ATOMIC_VAL3;
	kprintf("atomic or_return(%lx,%lx)", atomic_read(&dest), inc);
	kprintf("=%lx\n", atomic_or_return(&dest, inc));
	kprintf("now: %lx\n", atomic_read(&dest));
	if ( ( atomic_read(&dest) == (atomic_val)(ATOMIC_VAL4 | ATOMIC_VAL3 ) ) &&
	    ( atomic_read(&dest) == res ) )
		ktest_pass( sp );

	atomic_set(&dest, ATOMIC_VAL5);
	inc=ATOMIC_VAL3;
	kprintf("atomic xor_return(%lx,%lx)", atomic_read(&dest), inc);
	res = atomic_xor_return(&dest, inc);
	kprintf("=%lx\n", res);
	kprintf("now: %lx\n", atomic_read(&dest));
	if ( ( atomic_read(&dest) == (atomic_val)( ATOMIC_VAL3 ^ ATOMIC_VAL5 ) ) &&
	    ( atomic_read(&dest) == res ) )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	atomic_set(&dest, 0);
	inc=1;
	kprintf("atomic add_fetch_unless(%ld,0, %ld)", atomic_read(&dest), inc);
	kprintf("=%ld ", atomic_add_fetch_unless(&dest, 0, inc));
	kprintf("dest=%ld\n", atomic_read(&dest));
	if ( atomic_read(&dest) == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	kprintf("atomic add_fetch_unless(%ld,1,%ld)", atomic_read(&dest), inc);
	kprintf("=%ld ",  atomic_add_fetch_unless(&dest, 1, inc));
	kprintf("dest=%ld\n", atomic_read(&dest));
	if ( atomic_read(&dest) == 1 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	atomic_set(&dest, 1);
	inc=1;
	kprintf("atomic sub_fetch_unless(%ld,1,%ld)", atomic_read(&dest), inc);
	kprintf("=%ld ", atomic_sub_fetch_unless(&dest,1,inc));
	kprintf("dest=%ld\n", atomic_read(&dest));
	if ( atomic_read(&dest) == 1 )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	kprintf("atomic sub_fetch_unless(%ld,0,%ld)", atomic_read(&dest), inc);
	kprintf("=%ld ", atomic_sub_fetch_unless(&dest,0,inc));
	kprintf("dest=%ld\n", atomic_read(&dest));
	if ( atomic_read(&dest) == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );
}

void
tst_atomic(void){

	ktest_def_test(&tstat_atomic, "atomic1", atomic1, NULL);
	ktest_run(&tstat_atomic);
}

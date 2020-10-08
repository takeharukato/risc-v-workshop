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

#include <klib/atomic64.h>

#define ATOMIC_VAL1  (0x12345678)
#define ATOMIC_VAL2  (0xffffffff)
#define ATOMIC_VAL3  (0xafafafaf)
#define ATOMIC_VAL4  (0xf0f0f0f0)
#define ATOMIC_VAL5  (0xfafafafa)
#define ATOMIC_VAL6  (0xfeedfeed)

#define ATOMIC64_VAL1  (0x1234567890abcdef)
#define ATOMIC64_VAL2  (0xffffffffffffffff)
#define ATOMIC64_VAL3  (0xafafafafafafafaf)
#define ATOMIC64_VAL4  (0xf0f0f0f0f0f0f0f0)
#define ATOMIC64_VAL5  (0xfafafafafafafafa)
#define ATOMIC64_VAL6  (0xfeedfeedfeedfeed)

static ktest_stats tstat_atomic64=KTEST_INITIALIZER;

static void
atomic64_test1(struct _ktest_stats *sp, void __unused *arg){
	atomic64 dest;
	atomic64_val inc, v1, v2, res;
	void *p1, *p2, *pr;
	bool  rc;

	atomic64_set(&dest, ATOMIC64_VAL1);
	inc=1;
	kprintf("64bit atomic add(%qd,%qd)", atomic64_read(&dest), inc);
	res = atomic64_add_fetch(&dest, inc);
	kprintf("=%qd\n", res);
	kprintf("now: %qd\n", atomic64_read(&dest));
	if ( ( atomic64_read(&dest) == (atomic64_val)(ATOMIC64_VAL1 + 1) ) &&
	    ( res == (atomic64_val)(ATOMIC64_VAL1) ) )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	atomic64_set(&dest, ATOMIC64_VAL2);
	inc=ATOMIC64_VAL3;
	kprintf("64bit atomic and(%qx,%qx)", atomic64_read(&dest), inc);
	res = atomic64_and_fetch(&dest, inc);
	kprintf("=%qx\n", res);
	kprintf("now: %qx\n", atomic64_read(&dest));
	if ( ( atomic64_read(&dest) == (atomic64_val)( ATOMIC64_VAL2 & ATOMIC64_VAL3 ) ) &&
	    ( res == (atomic64_val)(ATOMIC64_VAL2) ) )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	atomic64_set(&dest, ATOMIC64_VAL4);
	inc=ATOMIC64_VAL3;
	kprintf("64bit atomic or(%qx,%qx)", atomic64_read(&dest), inc);
	res = atomic64_or_fetch(&dest, inc);
	kprintf("=%qx\n", res);
	kprintf("now: %qx\n", atomic64_read(&dest));
	if ( ( atomic64_read(&dest) == (atomic64_val)( ATOMIC64_VAL3 | ATOMIC64_VAL4 ) ) &&
	    ( res == (atomic64_val)(ATOMIC64_VAL4) ) )
		ktest_pass( sp );

	atomic64_set(&dest, ATOMIC64_VAL5 );
	inc=ATOMIC64_VAL3;
	kprintf("64bit atomic xor(%qx,%qx)", atomic64_read(&dest), inc);
	res = atomic64_xor_fetch(&dest, inc);
	kprintf("=%qx\n", res);
	kprintf("now: %qx\n", atomic64_read(&dest));
	if ( ( atomic64_read(&dest) == (atomic64_val)( ATOMIC64_VAL5 ^ ATOMIC64_VAL3 ) ) &&
		( res == (atomic64_val)(ATOMIC64_VAL5) ) )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	atomic64_set(&dest, ATOMIC64_VAL1);
	inc=1;
	kprintf("64bit atomic sub(%qd,%qd)", atomic64_read(&dest), inc);
	res = atomic64_sub_fetch(&dest, inc);
	kprintf("=%qd\n", res);
	kprintf("now: %qd\n", atomic64_read(&dest));
	if ( ( atomic64_read(&dest) == (atomic64_val)(ATOMIC64_VAL1 - inc) ) &&
	    ( res == (atomic64_val)(ATOMIC64_VAL1) ) )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	atomic64_set(&dest, 2);
	inc=1;
	kprintf("64bit atomic sub_test1(%qx,%qx)", atomic64_read(&dest), inc);
	rc = atomic64_sub_and_test(&dest,1, &res);
	kprintf("=%s\n", (rc)?("True"):("False"));
	if ( ( atomic64_read(&dest) == 1 ) && (!rc) )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	kprintf("64bit atomic sub_test2(%qx,%qx)", atomic64_read(&dest), inc);
	rc = atomic64_sub_and_test(&dest,1, &res);
	kprintf("=%s\n", (rc)?("True"):("False"));
	if ( ( atomic64_read(&dest) == 0 ) && ( rc ) )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	atomic64_set(&dest, 1);
	inc=1;
	kprintf("64bit atomic cmpxchg(%qx,%qx)", atomic64_read(&dest), inc);
	kprintf("=%qx\n", atomic64_cmpxchg_fetch(&dest, 1, 0));
	if ( atomic64_read(&dest) == 0 )
		ktest_pass( sp );

	p1 = NULL;
	p2 = (void *)ATOMIC64_VAL6;
	kprintf("64bit atomic cmpxchg ptr(p1,p2) = (%p,%p)\n", p1, p2);
	pr = atomic64_cmpxchg_ptr_fetch(&p1, NULL, p2);
	kprintf("64bit atomic cmpxchg ptr(pr,p1,p2) = (%p,%p,%p)\n", pr, p1, p2);
	if ( ( pr == NULL ) && ( p1 == p2 ) )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	atomic64_set(&dest, 1);
	v1 = 1;
	v2 = 2;
	kprintf("64bit atomic try cmpxchg(v1,v2) = (%qx,%qx)\n", v1, v2);
	rc = atomic64_try_cmpxchg_fetch(&dest, &v1, v2);
	kprintf("64bit atomic try cmpxchg(rc,dest,v1,v2) = (%s,%qx,%qx,%qx)\n",
	    (rc)?("True"):("False"), atomic64_read(&dest), v1, v2);
	if ( ( rc == true ) && ( atomic64_read(&dest) == v2 ) )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	atomic64_set(&dest, ATOMIC64_VAL1);
	inc=1;
	kprintf("64bit atomic add_return(%qd,%qd)", atomic64_read(&dest), inc);
	res = atomic64_add_return(&dest, inc);
	kprintf("=%qd\n", res);
	kprintf("now: %qd\n", atomic64_read(&dest));
	if ( ( atomic64_read(&dest) == (atomic64_val)(ATOMIC64_VAL1 + 1) ) &&
	    ( atomic64_read(&dest) == res ) )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	atomic64_set(&dest, ATOMIC64_VAL1);
	inc=1;
	kprintf("64bit atomic sub_return(%qd,%qd)", atomic64_read(&dest), inc);
	res = atomic64_sub_return(&dest, inc);
	kprintf("=%qd\n", res);
	kprintf("now: %qd\n", atomic64_read(&dest));
	if ( ( atomic64_read(&dest) == (atomic64_val)(ATOMIC64_VAL1 - 1) ) &&
	    ( atomic64_read(&dest) == res ) )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	atomic64_set(&dest, ATOMIC64_VAL2);
	inc=ATOMIC64_VAL3;
	kprintf("64bit atomic and_return(%qx,%qx)", atomic64_read(&dest), inc);
	res = atomic64_and_return(&dest, inc);
	kprintf("=%qx\n", res);
	kprintf("now: %qx\n", atomic64_read(&dest));
	if ( ( atomic64_read(&dest) == (atomic64_val)(ATOMIC64_VAL3 & ATOMIC64_VAL3 ) ) &&
	    ( atomic64_read(&dest) == res ) )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	atomic64_set(&dest, ATOMIC64_VAL4);
	inc=ATOMIC64_VAL3;
	kprintf("64bit atomic or_return(%qx,%qx)", atomic64_read(&dest), inc);
	kprintf("=%qx\n", atomic64_or_return(&dest, inc));
	kprintf("now: %qx\n", atomic64_read(&dest));
	if ( ( atomic64_read(&dest) == (atomic64_val)(ATOMIC64_VAL4 | ATOMIC64_VAL3 ) ) &&
	    ( atomic64_read(&dest) == res ) )
		ktest_pass( sp );

	atomic64_set(&dest, ATOMIC64_VAL5);
	inc=ATOMIC64_VAL3;
	kprintf("64bit atomic xor_return(%qx,%qx)", atomic64_read(&dest), inc);
	res = atomic64_xor_return(&dest, inc);
	kprintf("=%qx\n", res);
	kprintf("now: %qx\n", atomic64_read(&dest));
	if ( ( atomic64_read(&dest) == (atomic64_val)( ATOMIC64_VAL3 ^ ATOMIC64_VAL5 ) ) &&
	    ( atomic64_read(&dest) == res ) )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	atomic64_set(&dest, 0);
	inc=1;
	kprintf("64bit atomic add_fetch_unless(%qd,0, %qd)", atomic64_read(&dest), inc);
	kprintf("=%qd ", atomic64_add_fetch_unless(&dest, 0, inc));
	kprintf("dest=%qd\n", atomic64_read(&dest));
	if ( atomic64_read(&dest) == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	kprintf("64bit atomic add_fetch_unless(%qd,1,%qd)", atomic64_read(&dest), inc);
	kprintf("=%qd ",  atomic64_add_fetch_unless(&dest, 1, inc));
	kprintf("dest=%qd\n", atomic64_read(&dest));
	if ( atomic64_read(&dest) == 1 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	atomic64_set(&dest, 1);
	inc=1;
	kprintf("64bit atomic sub_fetch_unless(%qd,1,%qd)", atomic64_read(&dest), inc);
	kprintf("=%qd ", atomic64_sub_fetch_unless(&dest,1,inc));
	kprintf("dest=%qd\n", atomic64_read(&dest));
	if ( atomic64_read(&dest) == 1 )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	kprintf("64bit atomic sub_fetch_unless(%qd,0,%qd)", atomic64_read(&dest), inc);
	kprintf("=%qd ", atomic64_sub_fetch_unless(&dest,0,inc));
	kprintf("dest=%qd\n", atomic64_read(&dest));
	if ( atomic64_read(&dest) == 0 )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	return ;
}

void
tst_atomic64(void){

	ktest_def_test(&tstat_atomic64, "atomic64-1", atomic64_test1, NULL);
	ktest_run(&tstat_atomic64);
}

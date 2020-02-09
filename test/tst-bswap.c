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

#include <kern/ktest.h>

static ktest_stats tstat_bswap=KTEST_INITIALIZER;

#define TST_BSWAP_V64_NATIVE (0x1122334455667788)
#define TST_BSWAP_V64_SWAP   (0x8877665544332211)
#define TST_BSWAP_V32_NATIVE (0x11223344)
#define TST_BSWAP_V32_SWAP   (0x44332211)
#define TST_BSWAP_V16_NATIVE (0x1122)
#define TST_BSWAP_V16_SWAP   (0x2211)

static void
bswap1(struct _ktest_stats *sp, void __unused *arg){
	bool res;
	uint64_t v64;
	uint32_t v32;
	uint16_t v16;
	uint64_t r64;
	uint32_t r32;
	uint16_t r16;

	res = __host_is_little_endian();
#if ( defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__)	\
    && ( __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__ ) ) || defined(__LITTLE_ENDIAN__)
	if ( res )
		ktest_pass( sp );
	else
		ktest_fail( sp );
#elif ( defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__)	\
    && ( __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__ ) ) || defined(__BIG_ENDIAN__)
	if ( !res )
		ktest_pass( sp );
	else
		ktest_fail( sp );
#endif  /* __LITTLE_ENDIAN__ */

	v64 = TST_BSWAP_V64_NATIVE;
	v32 = TST_BSWAP_V32_NATIVE;
	v16 = TST_BSWAP_V16_NATIVE;

	r64 = __bswap64(v64);
	r32 = __bswap32(v32);
	r16 = __bswap16(v16);

	if ( r64 == TST_BSWAP_V64_SWAP )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	if ( r32 == TST_BSWAP_V32_SWAP )
		ktest_pass( sp );
	else
		ktest_fail( sp );
	
	if ( r16 == TST_BSWAP_V16_SWAP )
		ktest_pass( sp );
	else
		ktest_fail( sp );
}

void
tst_bswap(void){

	ktest_def_test(&tstat_bswap, "bswap1", bswap1, NULL);
	ktest_run(&tstat_bswap);
}


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

#define TST_MEMSET1_BUF_NR   (4)
#define TST_CH0            ('a')
#define TST_NR               (8)

static ktest_stats tstat_memset=KTEST_INITIALIZER;

static void
memset1(struct _ktest_stats *sp, void __unused *arg){
	uint8_t           tst_ch[TST_NR];
	uint64_t buf[TST_MEMSET1_BUF_NR];
	uint8_t                     *cp8;
	size_t                       len;
	uint32_t                    i, j;

	for( i = 0; TST_NR > i; ++i)
		tst_ch[i] = TST_CH0 + i;

	len = sizeof(uint64_t) * TST_MEMSET1_BUF_NR;
	/* メモリを0でフィル */
	for(i = 0; sizeof(uint64_t) * TST_MEMSET1_BUF_NR > i; ++i)
		*((uint8_t *)&buf[0] + i)= 0;

	/* メモリをチェック */
	for(i = 0, cp8 = (uint8_t*)&buf[0];
	    sizeof(uint64_t) * TST_MEMSET1_BUF_NR > i; ++i)
		if ( *(cp8 + i) == 0 )
			ktest_pass( sp );
		else
			ktest_fail( sp );

	/* 8バイト境界から1バイトづつずらしてmemsetを行い,
	 * フィルされた内容を確認する
	 */
	len = sizeof(uint64_t) * TST_MEMSET1_BUF_NR - sizeof(uint64_t);
	for( j = 0; TST_NR > j; ++j ) {

		cp8 = (uint8_t*)&buf[0] + j;     /* 開始位置をセット */
		memset(cp8, tst_ch[j], len - j); /* memset実行       */

		/* メモリをチェック */
		for(i = 0, cp8 = (uint8_t*)&buf[0] + j; (len - j) > i; ++i)
			if ( *cp8 == tst_ch[j] )
				ktest_pass( sp );
			else
				ktest_fail( sp );
		if ( *((uint8_t *)&buf[0] + len ) == 0 )
			ktest_pass( sp );
		else
			ktest_fail( sp );
		/* メモリを0でフィル */
		for(i = 0; sizeof(uint64_t) * TST_MEMSET1_BUF_NR > i; ++i)
			*((uint8_t *)&buf[0] + i)= 0;
	}
}

void
tst_memset(void){

	ktest_def_test(&tstat_memset, "memset1", memset1, NULL);
	ktest_run(&tstat_memset);
}


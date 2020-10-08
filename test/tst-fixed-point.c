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

static ktest_stats tstat_fixedpoint=KTEST_INITIALIZER;

static void
fixedpoint1(struct _ktest_stats *sp, void __unused *arg){
	fpa32       fp;
	fpa32   fp_min;
	fpa32   fp_max;
	fpa32      fp1;

	/*
	 * 除算
	 */
	fp_min = fixed_point_div(fixed_point_from_int(1), fixed_point_from_int(16));
	for( fp = fixed_point_from_int(1); fixed_point_cmp_near(fp, fp_min) > 0;
	     fp = fixed_point_div(fp, fixed_point_from_int(2)) )
		kprintf("div fp[%d.%d] > fp_min[%d.%d]\n",
		    fixed_point_to_int_zero(fp), fixed_point_to_frac(fp),
			fixed_point_to_int_zero(fp_min), fixed_point_to_frac(fp_min));
	if ( fp == fp_min )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/*
	 * 乗算
	 */
	fp_min = fixed_point_div(fixed_point_from_int(1), fixed_point_from_int(16));
	fp_max = fixed_point_div(fixed_point_from_int(1), fixed_point_from_int(2));
	for( fp = fp_min; fixed_point_cmp_near(fp_max, fp) > 0;
	     fp = fixed_point_mul(fp, fixed_point_from_int(2)) )
		kprintf("mul fp[%d.%d] < fp_max[%d.%d]\n",
		    fixed_point_to_int_zero(fp), fixed_point_to_frac(fp),
			fixed_point_to_int_zero(fp_max), fixed_point_to_frac(fp_max));
	if ( fp == fp_max )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	fp1 = fixed_point_div(fixed_point_from_int(-1), fixed_point_from_int(16));
	kprintf("negative num [%d,%d] Near:%d Zero:%d\n",
	    fixed_point_to_int_zero(fp1), fixed_point_to_frac(fp1),
	    fixed_point_to_int_near(fp1), fixed_point_to_int_zero(fp1));
	if ( fixed_point_to_int_near(fp1) !=  fixed_point_to_int_zero(fp1) )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/*
	 * 加算
	 */
	fp_min = fixed_point_div(fixed_point_from_int(1), fixed_point_from_int(16));
	fp_max = fixed_point_add(fp_min, 5);
	for( fp = fp_min; fixed_point_cmp_near(fp_max, fp) > 0;
	     fp = fixed_point_add(fp, 2) )
		kprintf("add fp[%d.%d] < fp_max[%d.%d]\n",
		    fixed_point_to_int_zero(fp), fixed_point_to_frac(fp),
			fixed_point_to_int_zero(fp_max), fixed_point_to_frac(fp_max));
	if ( fp >= fp_max )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	/*
	 * 減算
	 */
	fp_min = fixed_point_div(fixed_point_from_int(1), fixed_point_from_int(16));
	fp_max = fixed_point_add(fp_min, 5);
	for( fp = fp_max; fixed_point_cmp_near(fp, fp_min) > 0;
	     fp = fixed_point_sub(fp, 1) )
		kprintf("sub fp[%d.%d] > fp_min[%d.%d]\n",
		    fixed_point_to_int_zero(fp), fixed_point_to_frac(fp),
			fixed_point_to_int_zero(fp_min), fixed_point_to_frac(fp_min));
	if ( fp <= fp_min )
		ktest_pass( sp );
	else
		ktest_fail( sp );

	return ;
}

void
tst_fixedpoint(void){

	ktest_def_test(&tstat_fixedpoint, "fixedpoint1", fixedpoint1, NULL);
	ktest_run(&tstat_fixedpoint);
}


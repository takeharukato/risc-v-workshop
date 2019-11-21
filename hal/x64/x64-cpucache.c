/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Pseudo cpu cache routines                                       */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>

/**
   L1 データキャッシュラインサイズを返却する
   @return データキャッシュラインサイズ (単位:バイト)
 */
size_t
hal_get_cpu_l1_dcache_linesize(void){

	return 64; /* 64 byte */
}
/**
   L1 データキャッシュカラーリング数を返却する
   @return データキャッシュカラーリング数 (単位:個)
 */
obj_cnt_type
hal_get_cpu_l1_dcache_color_num(void){

	return KIB_TO_BYTE(32) / ( (8) * (64) ); /* 32KiB 8way 64 byte linesize */
}

/**
   L1 データキャッシュサイズを返却する
   @return データキャッシュサイズ (単位:バイト)
 */
size_t
hal_get_cpu_dcache_size(void){

	return KIB_TO_BYTE(32);
}

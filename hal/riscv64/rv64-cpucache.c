/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  RISC-V64 cpu cache routines                                       */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>
#include <kern/vm.h>
#include <hal/riscv64.h>
#include <hal/rv64-platform.h>

/**
   L1 データキャッシュラインサイズを返却する
   @return データキャッシュラインサイズ (単位:バイト)
 */
size_t
hal_get_cpu_l1_dcache_linesize(void){

	return RV64_L1_DCACHE_LINESIZE;
}
/**
   L1 データキャッシュカラーリング数を返却する
   @return データキャッシュカラーリング数 (単位:個)
 */
obj_cnt_type
hal_get_cpu_l1_dcache_color_num(void){

	return RV64_L1_DCACHE_COLOR_NUM;
}
/**
   L1 データキャッシュサイズを返却する
   @return データキャッシュサイズ (単位:バイト)
 */
size_t
hal_get_cpu_dcache_size(void){

	return RV64_L1_DCACHE_SIZE;
}

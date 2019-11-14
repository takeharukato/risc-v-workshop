/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  cpu level interrupt control                                       */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>

#include <kern/cpuintr.h>

/**
    CPUレベルの割込み禁止状態を保存するカーネル共通部用IF
    @param[out] iflags CPUレベルの割込み禁止状態保存領域のアドレス
    @note TODO: プラットフォーム管理のIFを呼ぶように修正
 */
void
krn_cpu_save_and_disable_interrupt(intrflags *iflags){

	hal_cpu_save_interrupt(iflags);
	hal_cpu_disable_interrupt();
}

/**
    CPUレベルの割込み禁止状態を復元する
    @param[in] iflags CPUレベルの割込み禁止状態保存領域のアドレス
    @note TODO: プラットフォーム管理のIFを呼ぶように修正
 */
void
krn_cpu_restore_interrupt(intrflags *iflags){

	hal_cpu_restore_interrupt(iflags);
}

/**
    CPUレベルで割込みを禁止する
    @note TODO: プラットフォーム管理のIFを呼ぶように修正
 */
void
krn_cpu_disable_interrupt(void){

	hal_cpu_disable_interrupt();
}

/**
    CPUレベルで割込みを許可する
    @note TODO: プラットフォーム管理のIFを呼ぶように修正
 */
void
krn_cpu_enable_interrupt(void){

	hal_cpu_enable_interrupt();
}

/**
    CPUレベルで割込みが禁止されていることを確認する
    @retval 真     CPUレベルで割込みが禁止されている
    @retval 偽     CPUレベルで割込みが禁止されていない
    @note TODO: プラットフォーム管理のIFを呼ぶように修正
 */
bool
krn_cpu_interrupt_disabled(void){

	return hal_cpu_interrupt_disabled();
}

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

#include <hal/hal-cpuintr.h>

/**
    RFLAGSを検査しCPUレベルで割込みが禁止されていることを確認する
    @param[in] iflags CPUレベルの割込み禁止状態保存領域のアドレス 
    @retval 真     CPUレベルで割込みが禁止されている
    @retval 偽     CPUレベルで割込みが禁止されていない
    @note 割込みフラグの扱い(1の時割込み許可, 0の時禁止)については, 
    Intel 64 and IA-32 Architectures Software Developer’s Manual, 
    Combined Volumes: 1, 2A, 2B, 2C, 3A, 3B and 3C 
    3.4.3.3 System Flags and IOPL Field参照
 */
bool
hal_intrflags_interrupt_disabled(intrflags *iflags){

	/* IFフラグが立っていないことを確認する  */
	return !( *iflags & x64_rflags_val(X64_RFLAGS_IF) );
}

#if defined(CONFIG_HAL)
/**
    CPUレベルの割込み禁止状態を保存する
    @param[out] iflags CPUレベルの割込み禁止状態保存領域のアドレス
 */
void
hal_cpu_save_interrupt(intrflags *iflags){


	x64_cpu_save_flags(iflags);
}

/**
    CPUレベルの割込み禁止状態を復元する
    @param[in] iflags CPUレベルの割込み禁止状態保存領域のアドレス
 */
void
hal_cpu_restore_interrupt(intrflags *iflags){


	x64_cpu_restore_flags(iflags);	
}

/**
    CPUレベルで割込みを禁止する
 */
void
hal_cpu_disable_interrupt(void){

	x64_cpu_disable_interrupt();
}

/**
    CPUレベルで割込みを許可する
 */
void
hal_cpu_enable_interrupt(void){

	x64_cpu_enable_interrupt();
}

/**
    CPUレベルで割込みが禁止されていることを確認する
    @retval 真     CPUレベルで割込みが禁止されている
    @retval 偽     CPUレベルで割込みが禁止されていない
 */
bool
hal_cpu_interrupt_disabled(void){
	intrflags iflags;

	x64_cpu_save_flags(&iflags);

	return hal_intrflags_interrupt_disabled(&iflags);
}

#else
/**
    CPUレベルの割込み禁止状態を保存する
    @param[out] iflags CPUレベルの割込み禁止状態保存領域のアドレス
 */
void
hal_cpu_save_interrupt(intrflags __unused *iflags){

}

/**
    CPUレベルの割込み禁止状態を復元する
    @param[in] iflags CPUレベルの割込み禁止状態保存領域のアドレス
 */
void
hal_cpu_restore_interrupt(intrflags __unused *iflags){

}

/**
    CPUレベルで割込みを禁止する
 */
void
hal_cpu_disable_interrupt(void){

}

/**
    CPUレベルで割込みを許可する
 */
void
hal_cpu_enable_interrupt(void){

}

/**
    CPUレベルで割込みが禁止されていることを確認する
    @retval 真     CPUレベルで割込みが禁止されている
    @retval 偽     CPUレベルで割込みが禁止されていない
    @note 割込みフラグの扱い(1の時割込み許可, 0の時禁止)については, 
    Intel 64 and IA-32 Architectures Software Developer’s Manual, 
    Combined Volumes: 1, 2A, 2B, 2C, 3A, 3B and 3C 
    3.4.3.3 System Flags and IOPL Field参照
 */
bool
hal_cpu_interrupt_disabled(void){

	return false;
}
#endif  /*  CONFIG_HAL  */

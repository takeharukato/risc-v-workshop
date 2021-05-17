/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  RISC-V64 cpu interrupt operations                                 */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>

#include <hal/riscv64.h>

/**
   割込みを禁止する (内部関数)
 */
static void
rv64_cpu_disable_interrupt(void){

	/* SSTATUS_SIE ビットを落とし, スーパーバイザへの割込みを禁止する */
	rv64_write_sstatus( rv64_read_sstatus() & ~SSTATUS_SIE );
}
/**
   割込みを許可する (内部関数)
 */
static void
rv64_cpu_enable_interrupt(void){

	/* SSTATUS_SIE ビットをセットし, スーパーバイザへの割込みを許可する         */
	rv64_write_sstatus( rv64_read_sstatus() | SSTATUS_SIE );
}

/**
   割込み状態を復元する  (内部関数)
   @param[in] iflags 割り込み状態フラグ格納先アドレス
 */
static void
rv64_cpu_restore_flags(intrflags *iflags){
	uint64_t cur;

	cur = rv64_read_sstatus(); /* 現在のSupervisor statusを取得する */
	cur &= ~SSTATUS_SIE;       /* SIEビットをクリアする */
	/* 保存されたSupervisor statusのSIEビットを設定する */
	rv64_write_sstatus( cur | ( *iflags & SSTATUS_SIE ) );
}

/**
   割込み状態を保存する
   @param[in] iflags 割り込み状態フラグ格納先アドレス
 */
static void
rv64_cpu_save_flags(intrflags *iflags){

	*iflags = rv64_read_sstatus(); /* 現在のSupervisor statusを格納する */
}

/**
    Supervisor statusを検査しCPUレベルで割込みが禁止されていることを確認する
    @param[in] iflags CPUレベルの割込み禁止状態保存領域のアドレス
    @retval 真     CPUレベルで割込みが禁止されている
    @retval 偽     CPUレベルで割込みが禁止されていない
 */
bool
hal_intrflags_interrupt_disabled(intrflags *iflags){

	/* SIEビットが立っていないことを確認する  */
	return ( ( *iflags & SSTATUS_SIE ) == 0 );
}

#if defined(CONFIG_HAL)
/**
    CPUレベルの割込み禁止状態を保存する
    @param[out] iflags CPUレベルの割込み禁止状態保存領域のアドレス
 */
void
hal_cpu_save_interrupt(intrflags *iflags){

	rv64_cpu_save_flags(iflags);
}

/**
    CPUレベルの割込み禁止状態を復元する
    @param[in] iflags CPUレベルの割込み禁止状態保存領域のアドレス
 */
void
hal_cpu_restore_interrupt(intrflags *iflags){


	rv64_cpu_restore_flags(iflags);
}

/**
    CPUレベルで割込みを禁止する
 */
void
hal_cpu_disable_interrupt(void){

	rv64_cpu_disable_interrupt();
}

/**
    CPUレベルで割込みを許可する
 */
void
hal_cpu_enable_interrupt(void){

	rv64_cpu_enable_interrupt();
}

/**
    CPUレベルで割込みが禁止されていることを確認する
    @retval 真     CPUレベルで割込みが禁止されている
    @retval 偽     CPUレベルで割込みが禁止されていない
 */
bool
hal_cpu_interrupt_disabled(void){
	intrflags iflags;

	rv64_cpu_save_flags(&iflags);

	return hal_intrflags_interrupt_disabled(&iflags);
}

#else
/**
    CPUレベルの割込み禁止状態を保存する (CONFIG_HAL無効時)
    @param[out] iflags CPUレベルの割込み禁止状態保存領域のアドレス
 */
void
hal_cpu_save_interrupt(intrflags __unused *iflags){

}

/**
    CPUレベルの割込み禁止状態を復元する (CONFIG_HAL無効時)
    @param[in] iflags CPUレベルの割込み禁止状態保存領域のアドレス
 */
void
hal_cpu_restore_interrupt(intrflags __unused *iflags){

}

/**
    CPUレベルで割込みを禁止する (CONFIG_HAL無効時)
 */
void
hal_cpu_disable_interrupt(void){

}

/**
    CPUレベルで割込みを許可する (CONFIG_HAL無効時)
 */
void
hal_cpu_enable_interrupt(void){

}

/**
    CPUレベルで割込みが禁止されていることを確認する (CONFIG_HAL無効時)
    @retval 真     CPUレベルで割込みが禁止されている
    @retval 偽     CPUレベルで割込みが禁止されていない
 */
bool
hal_cpu_interrupt_disabled(void){

	return false;
}
#endif  /*  CONFIG_HAL  */

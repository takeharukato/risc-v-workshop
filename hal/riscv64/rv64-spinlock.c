/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  RISC-V64 spinlock routines                                        */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>

#include <kern/spinlock.h>

extern uint32_t rv64_xchg(volatile uint32_t *_addr, uint32_t _newval);

/**
   スピンロックの実装部
   @param[in] lock 獲得対象のスピンロック
 */
void 
hal_spinlock_lock(spinlock *lock) {
	
	while(rv64_xchg(&lock->locked, 1) != 0);
}

/**
   スピンアンロックの実装部
   @param[in] lock 解放対象のスピンロック
 */
void 
hal_spinlock_unlock(spinlock *lock) {

	rv64_xchg(&lock->locked, 0);
}

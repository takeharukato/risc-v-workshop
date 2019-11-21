/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Spinlock routines                                                 */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>

#include <kern/spinlock.h>

#if defined(CONFIG_SMP)
extern uint32_t x64_xchg(volatile uint32_t *_addr, uint32_t _newval);
extern void x64_pause(void);
/** スピンロックの実装部
    @param[in] lock 獲得対象のスピンロック
 */
void 
hal_spinlock_lock(spinlock *lock) {
	
	while(x64_xchg(&lock->locked, 1) != 0)
		x64_pause();
}

/** スピンアンロックの実装部
    @param[in] lock 解放対象のスピンロック
 */
void 
hal_spinlock_unlock(spinlock *lock) {

	x64_xchg(&lock->locked, 0);
}

#else  /*  !CONFIG_SMP  */
/** スピンロックの実装部(ユニプロセッサ版)
    @param[in] lock 獲得対象のスピンロック
 */
void 
hal_spinlock_lock(spinlock *lock) {

	lock->locked = 1;
}

/** スピンアンロックの実装部(ユニプロセッサ版)
    @param[in] lock 解放対象のスピンロック
 */
void 
hal_spinlock_unlock(spinlock *lock) {

	lock->locked = 0;
}
#endif  /*  CONFIG_SMP  */

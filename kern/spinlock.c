/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  spinlock routines                                                 */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>

#include <klib/stack.h>

#include <kern/spinlock.h>
#include <kern/thr-if.h>

/**
   spinlock獲得時のトレース情報を記録する(HALレイヤからのコールバック関数)
   @param[in] depth   呼び出しの深さ
   @param[in] bpref   ベースポインタ
   @param[in] caller  呼び出し元アドレス
   @param[in] next_bp 次のベースポインタ返却領域 
   @param[in] argp    spinlockのアドレス
   @retval  0 正常終了
   @retval -ENOSPC 呼び出しネストが深すぎる
 */
static int
_trace_spinlock(int depth, uintptr_t __unused *bpref, void *caller, 
		void __unused *next_bp, void  *argp){
	spinlock *lock;
	
	if ( depth >= SPINLOCK_BT_DEPTH )
		return -ENOSPC;  /* 呼び出しネストが深すぎる */
	
	lock = (spinlock *)argp; /* ロック変数を参照 */
	lock->backtrace[depth] = caller;  /*  呼び出し元を記録する  */

	return 0;
}

/**
   spinlock獲得時のトレース情報を記録する
   @param[in] lock    spinlockのアドレス
 */
static void
fill_spinlock_trace(spinlock *lock) {
	void *bp;

	get_base_pointer(&bp);  /*  現在のベースポインタを取得  */

	/* バックトレース情報をクリア */
	memset(&lock->backtrace[0], 0, sizeof(void *) * SPINLOCK_BT_DEPTH );

	/* バックトレース情報を埋める */
	hal_backtrace(_trace_spinlock, bp, (void *)lock);
}

#if defined(CONFIG_CHECK_SPINLOCKS)
/**
   スピンロックを多重に獲得していることを確認する
   @param[in] lock 確認対象のスピンロック
   @retval true  ロックを自分自身で獲得している    
   @retval false ロックを自分自身で獲得していない
 */
static bool
check_recursive_locked(spinlock *lock) {

	/*  自スレッドがロックを獲得していて, ロックネストが1以上であることを確認  */
	return ( spinlock_locked_by_self(lock) && ( lock->depth > 0 ) );
}

/**
   スピンロックを自スレッドが保持していることを確認する
    @param[in] lock 確認対象のスピンロック    
    @retval true  ロックを自分自身で獲得している    
    @retval false ロックを自分自身で獲得していない
 */
bool 
spinlock_locked_by_self(spinlock *lock) {

	/* ロック獲得済みで, かつ, オーナが自スレッドであることを確認
	 * CPU初期化処理～アイドルスレッド起動前まではthr_get_current_thread()がNULLで
	 * 呼び出されるのでその場合はオーナスレッドの確認をせずロック済みなら通す
	 */
	return ( ( lock->locked != 0 ) &&
	    ( ( thr_get_current_thread() == NULL ) 
		|| ( lock->owner == thr_get_current_thread() ) ) ); 
}
#else

/**
   スピンロックを多重に獲得していることを確認する (CONFIG_CHECK_SPINLOCKS無効時)
   @param[in] lock 確認対象のスピンロック
   @retval true  ロックを自分自身で獲得している    
   @retval false ロックを自分自身で獲得していない
 */
static bool
check_recursive_locked(spinlock *lock) {

	/*  自スレッドがロックを獲得していないとみなす  */
	return false;
}

/**
   スピンロックを自スレッドが保持していることを確認する (CONFIG_CHECK_SPINLOCKS無効時)
    @param[in] lock 確認対象のスピンロック    
    @retval true  ロックを自分自身で獲得している    
    @retval false ロックを自分自身で獲得していない
 */
bool 
spinlock_locked_by_self(spinlock __unused *lock) {

	/* ロック獲得済みでないことを確認 */
	return ( lock->locked != 0 ); 
}
#endif  /*  CONFIG_CHECK_SPINLOCKS  */

/**
   スピンロックを初期化する
   @param[in] lock 初期化対象のスピンロック
 */
void 
spinlock_init(spinlock *lock){

	lock->locked = 0;                  /* ロック未獲得に設定         */
	lock->type = SPINLOCK_TYPE_NORMAL; /* 再入禁止ロックとして初期化 */
	lock->depth = 0;                   /* ロック深度をクリア         */
	lock->owner = NULL;                /* オーナをクリア             */
	memset(&lock->backtrace[0], 0, 
	    sizeof(void *) * SPINLOCK_BT_DEPTH); /* バックトレース情報クリア */
}

/**
   プリエンプションに影響せずスピンロックを獲得する
   @param[in] lock 獲得対象のスピンロック    
 */
void 
spinlock_raw_lock(spinlock *lock) {

	/*  多重ロック獲得でないことを確認  */
	kassert( ( lock->type & SPINLOCK_TYPE_RECURSIVE )
	    || ( !check_recursive_locked(lock) ) );

	hal_spinlock_lock(lock);    /*  スピンロック獲得                  */
	fill_spinlock_trace(lock);  /*  スピンロック獲得シーケンスを記録  */
	++lock->depth;              /*  ロック深度を加算                  */

	ti_inc_preempt();           /* プリエンプション禁止 */
}

/**
   プリエンプションに影響せずスピンロックを解放する
   @param[in] lock 獲得対象のスピンロック    
 */
void 
spinlock_raw_unlock(spinlock *lock){

	/*  多重ロック解放でないことを確認  */
	kassert( ( lock->type & SPINLOCK_TYPE_RECURSIVE ) || ( lock->depth == 1) );

#if defined(CONFIG_CHECK_SPINLOCKS)
	lock->owner = NULL;  /*  ロック獲得スレッドをクリア */
#endif  /*  CONFIG_CHECK_SPINLOCKS  */

	--lock->depth;             /* ロック深度を減算       */
	hal_spinlock_unlock(lock); /* スピンロック解放       */
	ti_dec_preempt();          /* プリエンプション許可   */
}

/**
   スピンロックを獲得する
   @param[in] lock 獲得対象のスピンロック    
 */
void 
spinlock_lock(spinlock *lock) {

	spinlock_raw_lock(lock); /* ロック獲得             */
}

/**
   スピンロックを解放する
   @param[in] lock 解放対象のスピンロック    
 */
void
spinlock_unlock(spinlock *lock) {

	spinlock_raw_unlock(lock); /* ロック解放             */
}

/**
   プリエンプションに影響せず割込禁止付きでスピンロックを獲得する
   @param[in] lock 獲得対象のスピンロック    
   @param[in] flags 割込状態保存先アドレス
*/
void
spinlock_raw_lock_disable_intr(spinlock  *lock, intrflags *flags) {

	krn_cpu_save_and_disable_interrupt(flags);  /* 割込み禁止                           */
	spinlock_raw_lock(lock);                    /* プリエンプション操作なしのロック獲得 */
}

/**
   プリエンプションに影響せずスピンロックを解放し, 割込状態を元に戻す
   @param[in] lock 獲得対象のスピンロック    
   @param[in] flags 割込状態保存先アドレス
 */
void
spinlock_raw_unlock_restore_intr(spinlock *lock, intrflags *flags) {

	spinlock_raw_unlock(lock);        /* プリエンプション操作なしのロック解放 */
	krn_cpu_restore_interrupt(flags); /* 割込み状態復元                       */
}

/**
   割込禁止付きでスピンロックを獲得する
   @param[in] lock 獲得対象のスピンロック    
   @param[in] iflags 割込状態保存先アドレス
 */
void
spinlock_lock_disable_intr(spinlock *lock, intrflags *iflags) {
	
	krn_cpu_save_and_disable_interrupt(iflags); /* 割り込み禁止 */
	spinlock_lock(lock);                        /* ロック獲得   */
}

/**
   スピンロックを解放し, 割込状態を元に戻す
   @param[in] lock 獲得対象のスピンロック    
   @param[in] iflags 割込状態保存先アドレス
 */
void
spinlock_unlock_restore_intr(spinlock *lock, intrflags *iflags) {

	spinlock_unlock(lock);              /* ロック解放     */
	krn_cpu_restore_interrupt(iflags);  /* 割込み状態復元 */
}

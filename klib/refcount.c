/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Reference counter routines                                        */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>

/**
   参照カウンタを減算し, 減算後の参照カウンタの状態と値を返却する(内部関数)
   @param[in]  counterp 参照カウンタ
   @param[in]  v        減算値 
   @param[out] newp     更新後の値を返却する領域
   @retval     真       参照カウンタが0になった
   @retval     偽       参照カウンタが0以上
 */
static bool
sub_and_test_refcounter(refcnt *counterp, refcnt_val v, refcnt_val *newp){
	refcnt_val new;
	bool        rc;

	rc = atomic_sub_and_test(&counterp->counter, (atomic_val)v, (atomic_val *)&new);
	kassert( new >= 0 );  /*  減算後に負にならないことを確認する  */
	
	if ( newp != NULL )
		*newp = new;  /*  更新後の値を返却する  */

	return rc;
}

/**
   初期値を指定して参照カウンタを初期化する
   @param[in] counterp  参照カウンタ
   @param[in] val       設定値
 */
void
refcnt_init_with_value(refcnt *counterp, refcnt_val val){
	
	/* 各メンバを初期化する
	 */
	atomic_set_fetch(&counterp->counter, val);
}
/**
   参照カウンタの初期化
   @param[in] counterp  参照カウンタ
 */
void
refcnt_init(refcnt *counterp){
	
	refcnt_init_with_value(counterp, REFCNT_INITIAL_VAL);
}

/**
   参照カウンタに値を設定する
   @param[in] counterp  参照カウンタ
   @param[in] val       設定値
   @note 参照カウンタは事前に初期化されていなければならない
 */
void
refcnt_set(refcnt *counterp, refcnt_val val){

	atomic_set(&counterp->counter, (atomic_val)val);
}

/**
   参照カウンタの値を参照する
   @param[in] counterp  参照カウンタ
   @return    参照カウンタの値
 */
refcnt_val
refcnt_read(refcnt *counterp){

	return (refcnt_val)atomic_read(&counterp->counter);
}

/**
   参照カウンタがゼロでない場合, 参照カウンタに加算する
   @param[in] counterp 参照カウンタ
   @param[in] v        加算値 
   @return    更新前の参照カウンタ値
   @note 参照カウンタは通常1に初期化されるため, 解放済みのカウンタを
   操作しないようにしつつ加算操作を行う
 */
refcnt_val 
refcnt_add_if_valid(refcnt *counterp, refcnt_val v){
	refcnt_val ret;
	
	ret = atomic_add_fetch_unless(&counterp->counter, 0, v);
	kassert( ret > 0 );

	return ret;
}

/**
   参照カウンタに加算する
   @param[in] counterp 参照カウンタ
   @param[in] v        加算値 
   @return    更新前の参照カウンタ値
 */
refcnt_val 
refcnt_add(refcnt *counterp, refcnt_val v){
	refcnt_val ret;
	
	ret = atomic_add_fetch(&counterp->counter, v);

	return ret;
}

/**
   参照カウンタをインクリメントする
   @param[in] counterp 参照カウンタ
   @return    更新前の参照カウンタ値
 */
refcnt_val 
refcnt_inc(refcnt *counterp){

	return refcnt_add(counterp, 1);
}

/**
   参照カウンタがゼロでない場合, 参照カウンタをインクリメントする
   @param[in] counterp 参照カウンタ
   @return    更新前の参照カウンタ値
   @note 参照カウンタは通常1に初期化されるため, 解放済みのカウンタを
   操作しないようにしつつ加算操作を行う
 */
refcnt_val 
refcnt_inc_if_valid(refcnt *counterp){
	refcnt_val ret;
	
	ret = atomic_add_fetch_unless(&counterp->counter, 0, 1);

	return ret;
}

/**
   参照カウンタを減算し, 減算後の参照カウンタの状態を返却する
   @param[in] counterp 参照カウンタ
   @param[in] v        減算値 
   @retval    真       参照カウンタが0になった
   @retval    偽       参照カウンタが0以上
 */
bool
refcnt_sub_and_test(refcnt *counterp, refcnt_val v) {

	return sub_and_test_refcounter(counterp, v, NULL);
}

/**
   参照カウンタをデクリメントし, 操作後の参照カウンタの状態を返却する
   @param[in] counterp 参照カウンタ
   @retval    真       参照カウンタが0になった
   @retval    偽       参照カウンタが0以上
 */
bool
refcnt_dec_and_test(refcnt *counterp) {
	
	return sub_and_test_refcounter(counterp, 1, NULL);
}

/**
   参照カウンタをデクリメントし, 操作後の参照カウンタの値を返却する
   @param[in] counterp 参照カウンタ
   @return 操作後のカウンタ値
 */
refcnt_val 
refcnt_dec(refcnt *counterp){
	refcnt_val new;

	sub_and_test_refcounter(counterp, 1, &new);

	return new;
}

/**
   参照カウンタがデクリメントし0になった場合は, 指定されたロックを獲得する
   参照カウンタが0にならなければそのまま復帰する
   @param[in] counterp 参照カウンタ
   @param[in] lock     獲得するspinlock
   @retval    真       参照カウンタが0になった
   @retval    偽       参照カウンタが0以上
 */
bool
refcnt_dec_and_lock(refcnt *counterp, spinlock *lock){

	spinlock_lock(lock);

	if ( refcnt_dec_and_test(counterp) )
		return true; /* カウンタが0になったらロックを保持したまま抜ける  */


	spinlock_unlock(lock);   /* カウンタが0でないのでロックを解放する  */
	return false;
}

/**
   参照カウンタがデクリメントし0になった場合は, 割込み禁止で指定されたロックを獲得する
   参照カウンタが0にならなければそのまま復帰する
   @param[in] counterp 参照カウンタ
   @param[in] lock     獲得するspinlock
   @param[in] iflags   割込み状態フラグ
   @retval    真       参照カウンタが0になった
   @retval    偽       参照カウンタが0以上
 */
bool
refcnt_dec_and_lock_disable_intr(refcnt *counterp, spinlock *lock, intrflags *iflags){


	spinlock_lock_disable_intr(lock, iflags);
	if ( refcnt_dec_and_test(counterp) )
		return true; /* カウンタが0になったらロックを保持したまま抜ける  */

	spinlock_unlock_restore_intr(lock, iflags);  /* カウンタが0でないのでロックを解放する  */
	return false;
}

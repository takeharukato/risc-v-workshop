/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  kernel level atomic operation routines                            */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>

#include <klib/atomic.h>

/**
	整数をアトミックに加え, 更新前の値を返却する
	@param[out] val 更新する変数のアドレス
	@param[in] incr 加算する整数
	@return 更新前の値
*/
atomic_val
atomic_add_fetch(atomic *val, atomic_val incr){
	
	return hal_atomic_add_fetch(val, incr);
}

/**
	整数をアトミックに減算し, 更新前の値を返却する
	@param[out] val 更新する変数のアドレス
	@param[in] decr 減算する整数
	@return 更新前の値
*/
atomic_val
atomic_sub_fetch(atomic *val, atomic_val decr){

	return hal_atomic_add_fetch(val, -decr);
}

/**
	整数間の論理積をアトミックに算出し, 更新前の値を返却する
	@param[out] val 更新する変数のアドレス
	@param[in]  v   論理積をとる整数
	@return 更新前の値
*/
atomic_val
atomic_and_fetch(atomic *val, atomic_val v){

	return hal_atomic_and_fetch(val, v);
}

/**
	整数間の論理和をアトミックに算出し, 更新前の値を返却する
	@param[out] val 更新する変数のアドレス
	@param[in]  v   論理和をとる整数
	@return 更新前の値
*/
atomic_val
atomic_or_fetch(atomic *val, atomic_val v){

	return hal_atomic_or_fetch(val, v);
}

/**
	整数間の排他的論理和をアトミックに算出し, 更新前の値を返却する
	@param[out] val 更新する変数のアドレス
	@param[in]  v   排他的論理和をとる整数
	@return 更新前の値
*/
atomic_val
atomic_xor_fetch(atomic *val, atomic_val v){

	return hal_atomic_xor_fetch(val, v);
}

/**
	整数をアトミックに代入し, 更新前の値を返却する
	@param[out] val    更新する変数のアドレス
	@param[in]  set_to 設定する値
	@return 更新前の値
*/
atomic_val
atomic_set_fetch(atomic *val, atomic_val set_to){
	
	return hal_atomic_set_fetch(val, set_to);
}

/**
	valがoldに等しい場合は, newをアトミックに代入し, 更新前の値を返却する
	@param[out] val      更新する変数のアドレス
	@param[in]  old      更新前の期待値
	@param[in]  new      更新する値
	@return 更新前の値
*/
atomic_val 
atomic_cmpxchg_fetch(atomic *val, atomic_val old, atomic_val new){
	
	return hal_atomic_cmpxchg_fetch(val, old, new);
}

/**
	ポインタ変数がoldに等しい場合は, newをアトミックに代入し, 更新前の値を返却する
	@param[out] valp     更新するポインタ変数のアドレス
	@param[in]  old      更新前の期待値
	@param[in]  new      更新する値
	@return 更新前の値
*/
void *
atomic_cmpxchg_ptr_fetch(void **valp, void *old, void *new){

	return hal_atomic_cmpxchg_ptr_fetch(valp, old, new);	
}

/**
	valがoldpが指し示す先の値に等しい場合は, newをvalにアトミックに代入する
	valとoldpが指し示す先の値が異なる場合は, oldpの指し示す先をvalの値に更新する
	@param[out]    val      更新する変数のアドレス
	@param[in,out] oldp     更新前の期待値の格納領域
	@param[in]     new      更新する値
	@retval 真     値を更新した
	@retval 偽     値を更新しなかった
*/
bool
atomic_try_cmpxchg_fetch(atomic *val, atomic_val *oldp, atomic_val new){
	atomic_val r, old;
	
	old = *oldp;
	r = hal_atomic_cmpxchg_fetch(val, old, new);
	if ( r != old )
		*oldp = r;
	
	return ( r == old );
}

/**
	整数をアトミックに加算し, 更新後の値を返却する
	@param[out] val 更新する変数のアドレス
	@param[in] incr 加算する整数
	@return 更新後の値
*/
atomic_val
atomic_add_return(atomic *val, atomic_val incr){

	return hal_atomic_add_fetch(val, incr) + incr;
}

/**
	整数をアトミックに減算し, 更新後の値を返却する
	@param[out] val 更新する変数のアドレス
	@param[in] decr 減算する整数
	@return 更新後の値
*/
atomic_val
atomic_sub_return(atomic *val, atomic_val decr){

	return hal_atomic_add_fetch(val, -decr) - decr;
}

/**
	整数間の論理積をアトミックに算出し, 更新後の値を返却する
	@param[out] val 更新する変数のアドレス
	@param[in]  v   論理積をとる整数
	@return 更新後の値
*/
atomic_val
atomic_and_return(atomic *val, atomic_val v){

	return (hal_atomic_and_fetch(val, v) & v);
}

/**
	整数間の論理和をアトミックに算出し, 更新後の値を返却する
	@param[out] val 更新する変数のアドレス
	@param[in]  v   論理和をとる整数
	@return 更新後の値
*/
atomic_val
atomic_or_return(atomic *val, atomic_val v){

	return (hal_atomic_or_fetch(val, v) | v );
}

/**
	整数間の排他的論理和をアトミックに算出し, 更新後の値を返却する
	@param[out] val 更新する変数のアドレス
	@param[in]  v   排他的論理和をとる整数
	@return 更新後の値
*/
atomic_val
atomic_xor_return(atomic *val, atomic_val v){

	return ( hal_atomic_xor_fetch(val, v) ^ v );
}

/**
   整数をアトミックに減算し, 減算後の状態と値を返却する
   @param[out] val      更新する変数のアドレス
   @param[in]  decr     減算値 
   @param[out] newp     更新後の値を返却する領域
   @retval     真       参照カウンタが0になった
   @retval     偽       参照カウンタが0以上
 */
bool
atomic_sub_and_test(atomic *val, atomic_val decr, atomic_val *newp){
	atomic_val new;

	new = atomic_sub_return(val, decr);  /* カウンタを減算し減算後の値を得る */
	kassert( new >= 0 );  /*  減算後に負にならないことを確認する  */
	
	if ( newp != NULL )
		*newp = new;  /*  更新後の値を返却する  */

	if ( new == 0 ) 
		return true;  /*  更新後に0になった */

	kassert( new > 0 );

	return false;
}

/**
   アトミック変数の値を参照する
   @param[in] val 参照する変数のアドレス
   @return アトミック変数の値
*/
atomic_val
atomic_read(atomic *val){

	/* 参照後の競合は避けえないので単なる整数の参照でも許容されうるが,
	 * 念のためアトミックに0を加算し, 加算前の値を得ることで値を参照  
	 */
	return atomic_add_fetch(val, 0);
}

/**
   アトミック変数に値を設定する
   @param[in] val 設定する変数のアドレス
   @param[in] new 設定する値
*/
void
atomic_set(atomic *val, atomic_val new){

	atomic_set_fetch(val, new);
	return;
}

/**
   変数の内容がunexpectedでなければ整数をアトミックに加え, 更新前の値を返却する
   @param[out] val 更新する変数のアドレス
   @param[in] unexpected 更新不可能条件となる変数の値 
   @param[in] incr 加算する整数
   @return 更新前の値
 */
atomic_val
atomic_add_fetch_unless(atomic *val, atomic_val unexpected, atomic_val incr){
	atomic_val cur = atomic_read(val);

	do {
		if ( cur == unexpected )
			break;
	} while (!atomic_try_cmpxchg_fetch(val, &cur, cur + incr));

	return cur;
}

/**
   変数の内容がunexpectedでなければ整数をアトミックに減算, 更新前の値を返却する
   @param[out] val 更新する変数のアドレス
   @param[in] unexpected 更新不可能条件となる変数の値 
   @param[in] decr 減算する整数
   @return 更新前の値
 */
atomic_val
atomic_sub_fetch_unless(atomic *val, atomic_val unexpected, atomic_val decr){
	atomic_val cur = atomic_read(val);

	do {
		if ( cur == unexpected )
			break;
	} while (!atomic_try_cmpxchg_fetch(val, &cur, cur - decr));

	return cur;
}

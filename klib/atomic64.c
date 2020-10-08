/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  kernel level 64bit atomic operation routines                      */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>

#include <klib/atomic64.h>
#include <hal/hal-atomic64.h>

/**
	64ビット整数をアトミックに加え, 更新前の値を返却する
	@param[out] val 更新する変数のアドレス
	@param[in] incr 加算する64ビット整数
	@return 更新前の値
*/
atomic64_val
atomic64_add_fetch(atomic64 *val, atomic64_val incr){

	return hal_atomic64_add_fetch(val, incr);
}

/**
	64ビット整数をアトミックに減算し, 更新前の値を返却する
	@param[out] val 更新する変数のアドレス
	@param[in] decr 減算する64ビット整数
	@return 更新前の値
*/
atomic64_val
atomic64_sub_fetch(atomic64 *val, atomic64_val decr){

	return hal_atomic64_add_fetch(val, -decr);
}

/**
	64ビット整数間の論理積をアトミックに算出し, 更新前の値を返却する
	@param[out] val 更新する変数のアドレス
	@param[in]  v   論理積をとる64ビット整数
	@return 更新前の値
*/
atomic64_val
atomic64_and_fetch(atomic64 *val, atomic64_val v){

	return hal_atomic64_and_fetch(val, v);
}

/**
	64ビット整数間の論理和をアトミックに算出し, 更新前の値を返却する
	@param[out] val 更新する変数のアドレス
	@param[in]  v   論理和をとる64ビット整数
	@return 更新前の値
*/
atomic64_val
atomic64_or_fetch(atomic64 *val, atomic64_val v){

	return hal_atomic64_or_fetch(val, v);
}

/**
	64ビット整数間の排他的論理和をアトミックに算出し, 更新前の値を返却する
	@param[out] val 更新する変数のアドレス
	@param[in]  v   排他的論理和をとる64ビット整数
	@return 更新前の値
*/
atomic64_val
atomic64_xor_fetch(atomic64 *val, atomic64_val v){

	return hal_atomic64_xor_fetch(val, v);
}

/**
	64ビット整数をアトミックに代入し, 更新前の値を返却する
	@param[out] val    更新する変数のアドレス
	@param[in]  set_to 設定する値
	@return 更新前の値
*/
atomic64_val
atomic64_set_fetch(atomic64 *val, atomic64_val set_to){

	return hal_atomic64_set_fetch(val, set_to);
}

/**
	valがoldに等しい場合は, newをアトミックに代入し, 更新前の値を返却する
	@param[out] val      更新する変数のアドレス
	@param[in]  old      更新前の期待値
	@param[in]  new      更新する値
	@return 更新前の値
*/
atomic64_val
atomic64_cmpxchg_fetch(atomic64 *val, atomic64_val old, atomic64_val new){

	return hal_atomic64_cmpxchg_fetch(val, old, new);
}

/**
	ポインタ変数がoldに等しい場合は, newをアトミックに代入し, 更新前の値を返却する
	@param[out] valp     更新するポインタ変数のアドレス
	@param[in]  old      更新前の期待値
	@param[in]  new      更新する値
	@return 更新前の値
*/
void *
atomic64_cmpxchg_ptr_fetch(void **valp, void *old, void *new){

	return hal_atomic64_cmpxchg_ptr_fetch(valp, old, new);
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
atomic64_try_cmpxchg_fetch(atomic64 *val, atomic64_val *oldp, atomic64_val new){
	atomic64_val r, old;

	old = *oldp;
	r = hal_atomic64_cmpxchg_fetch(val, old, new);
	if ( r != old )
		*oldp = r;

	return ( r == old );
}

/**
	64ビット整数をアトミックに加算し, 更新後の値を返却する
	@param[out] val 更新する変数のアドレス
	@param[in] incr 加算する64ビット整数
	@return 更新後の値
*/
atomic64_val
atomic64_add_return(atomic64 *val, atomic64_val incr){

	return hal_atomic64_add_fetch(val, incr) + incr;
}

/**
	64ビット整数をアトミックに減算し, 更新後の値を返却する
	@param[out] val 更新する変数のアドレス
	@param[in] decr 減算する64ビット整数
	@return 更新後の値
*/
atomic64_val
atomic64_sub_return(atomic64 *val, atomic64_val decr){

	return hal_atomic64_add_fetch(val, -decr) - decr;
}

/**
	64ビット整数間の論理積をアトミックに算出し, 更新後の値を返却する
	@param[out] val 更新する変数のアドレス
	@param[in]  v   論理積をとる64ビット整数
	@return 更新後の値
*/
atomic64_val
atomic64_and_return(atomic64 *val, atomic64_val v){

	return (hal_atomic64_and_fetch(val, v) & v);
}

/**
	64ビット整数間の論理和をアトミックに算出し, 更新後の値を返却する
	@param[out] val 更新する変数のアドレス
	@param[in]  v   論理和をとる64ビット整数
	@return 更新後の値
*/
atomic64_val
atomic64_or_return(atomic64 *val, atomic64_val v){

	return (hal_atomic64_or_fetch(val, v) | v );
}

/**
	64ビット整数間の排他的論理和をアトミックに算出し, 更新後の値を返却する
	@param[out] val 更新する変数のアドレス
	@param[in]  v   排他的論理和をとる64ビット整数
	@return 更新後の値
*/
atomic64_val
atomic64_xor_return(atomic64 *val, atomic64_val v){

	return ( hal_atomic64_xor_fetch(val, v) ^ v );
}

/**
   64ビット整数をアトミックに減算し, 減算後の状態と値を返却する
   @param[out] val      更新する変数のアドレス
   @param[in]  decr     減算値
   @param[out] newp     更新後の値を返却する領域
   @retval     真       参照カウンタが0になった
   @retval     偽       参照カウンタが0以上
 */
bool
atomic64_sub_and_test(atomic64 *val, atomic64_val decr, atomic64_val *newp){
	atomic64_val new;

	new = atomic64_sub_return(val, decr);  /* カウンタを減算し減算後の値を得る */
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
atomic64_val
atomic64_read(atomic64 *val){

	/* 参照後の競合は避けえないので単なる64ビット整数の参照でも許容されうるが,
	 * 念のためアトミックに0を加算し, 加算前の値を得ることで値を参照
	 */
	return atomic64_add_fetch(val, 0);
}

/**
   アトミック変数に値を設定する
   @param[in] val 設定する変数のアドレス
   @param[in] new 設定する値
*/
void
atomic64_set(atomic64 *val, atomic64_val new){

	atomic64_set_fetch(val, new);
	return;
}

/**
   変数の内容がunexpectedでなければ64ビット整数をアトミックに加え, 更新前の値を返却する
   @param[out] val 更新する変数のアドレス
   @param[in] unexpected 更新不可能条件となる変数の値
   @param[in] incr 加算する64ビット整数
   @return 更新前の値
 */
atomic64_val
atomic64_add_fetch_unless(atomic64 *val, atomic64_val unexpected, atomic64_val incr){
	atomic64_val cur = atomic64_read(val);

	do {
		if ( cur == unexpected )
			break;
	} while (!atomic64_try_cmpxchg_fetch(val, &cur, cur + incr));

	return cur;
}

/**
   変数の内容がunexpectedでなければ64ビット整数をアトミックに減算, 更新前の値を返却する
   @param[out] val 更新する変数のアドレス
   @param[in] unexpected 更新不可能条件となる変数の値
   @param[in] decr 減算する64ビット整数
   @return 更新前の値
 */
atomic64_val
atomic64_sub_fetch_unless(atomic64 *val, atomic64_val unexpected, atomic64_val decr){
	atomic64_val cur = atomic64_read(val);

	do {
		if ( cur == unexpected )
			break;
	} while (!atomic64_try_cmpxchg_fetch(val, &cur, cur - decr));

	return cur;
}

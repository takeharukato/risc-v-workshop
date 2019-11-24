/* -*- mode: gas; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  x64 cpu atomic operations                                         */
/*                                                                    */
/**********************************************************************/
#define ASM_FILE   1

.code64

.global hal_atomic_add_fetch, hal_atomic_and_fetch
.global hal_atomic_or_fetch, hal_atomic_xor_fetch
.global hal_atomic_set_fetch, hal_atomic_cmpxchg_fetch, hal_atomic_cmpxchg_ptr_fetch

/* @note CMPXCHG r/m32, r32 Compare EAX with r/m32. If equal, ZF is set and r32 is
	 loaded into r/m32. Else, clear ZF and load r/m32 into EAX.
 */

/**
	整数をアトミックに加え, 更新前の値を返却する
	@param[out] rdi 更新する変数のアドレス
	@param[in] esi 加算する整数
	@return 更新前の値
	@note atomic_val hal_atomic_add_fetch(atomic *val, atomic_val incr)相当
*/
hal_atomic_add_fetch:
	lock
	xadd	%esi,(%rdi)	/*  ret=*val, *val += incr */
	mov	%esi, %eax	/*  return ret  */
	ret

/**
	整数間の論理積を算出し, 更新前の値を返却する
	@param[out] rdi 更新する変数のアドレス
	@param[in] esi 論理積をとる整数
	@return 更新前の値
	@note atomic_val hal_atomic_and_fetch(atomic *val, atomic_val incr)相当
*/
hal_atomic_and_fetch:
1:	
	mov	(%rdi),%eax	/* ret=*val */
	mov	%esi,%ecx	/* tmp=incr */
	and	%eax,%ecx	/* tmp &= *val */

	lock
	cmpxchg	%ecx,(%rdi)	/* *val = tmp if *val == ret */

	jnz	1b

	ret			/* return ret */
	
/**
	整数間の論理和を算出し, 更新前の値を返却する
	@param[out] rdi 更新する変数のアドレス
	@param[in] esi 論理和をとる整数
	@return 更新前の値
	@note atomic_val hal_atomic_or_fetch(atomic *val, atomic_val incr)相当
*/
hal_atomic_or_fetch:
1:	
	mov	(%rdi),%eax	/* ret=*val */
	mov	%esi,%ecx	/* tmp=incr */
	or	%eax,%ecx	/* tmp = ret | incr */

	lock
	cmpxchg	%ecx,(%rdi)	/* *val = tmp if *val == ret */

	jnz	1b

	ret			/* return ret */

/**
	整数間の排他的論理和を算出し, 更新前の値を返却する
	@param[out] rdi 更新する変数のアドレス
	@param[in] esi 排他的論理和をとる整数
	@return 更新前の値
	@note atomic_val hal_atomic_xor_fetch(atomic *val, atomic_val incr)相当
*/
hal_atomic_xor_fetch:
1:	
	mov	(%rdi),%eax	/* ret=*val */
	mov	%esi,%ecx	/* tmp=incr */
	xor	%eax,%ecx	/* tmp = ret ^ incr */

	lock
	cmpxchg	%ecx,(%rdi)	/* *val = tmp if *val == ret */

	jnz	1b

	ret			/* return ret */
	
	
/**
	整数をアトミックに代入し, 代入前の値を返す
	@param[out] rdi 代入される変数のアドレス
	@param[in] esi 代入する整数
	@return 更新前の値
	@note atomic_val hal_atomic_set_fetch(atomic *val, atomic_val new)相当
*/
hal_atomic_set_fetch:
	xchg	%esi,(%rdi) /* *val = new */
	mov	%edi,%eax   /* ret=*val */
	ret		    /* return ret */

/**
	更新前の整数値がoldに一致したら整数newをアトミックに代入し, 代入前の値を返す
	@param[out] rdi 代入される変数のアドレス
	@param[in] esi 更新前の値
	@param[in] edx 更新後の値
	@return 更新前の値
	@note atomic_val hal_atomic_cmpxchg_fetch(atomic *val, atomic_val old, atomic_val new)相当
*/
hal_atomic_cmpxchg_fetch:
	mov	%esi, %eax	/*  tmp = old  */

	lock
	cmpxchg	%edx, (%rdi)    /* *val = new if *val == tmp  */

	ret
	
/**
	更新前のポインタ値がoldに一致したらnewをアトミックに代入し, 代入前の値を返す
	@param[out] rdi 代入される変数のアドレス
	@param[in] rsi 更新前のポインタ値
	@param[in] rdx 更新後のポインタ値
	@return 更新前の値
	@note void *hal_atomic_cmpxchg_ptr_fetch(void **valp, void *old, void *new)相当
	64bit CPUなので, hal_atomic64_cmpxchg_ptr_fetchを呼び出すことで実現
*/
hal_atomic_cmpxchg_ptr_fetch:
	call	hal_atomic64_cmpxchg_ptr_fetch
	ret
	


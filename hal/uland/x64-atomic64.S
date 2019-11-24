/* -*- mode: gas; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  x64 64bit atomic operations                                       */
/*                                                                    */
/**********************************************************************/
#define ASM_FILE   1

.code64

.global hal_atomic64_add_fetch, hal_atomic64_and_fetch
.global hal_atomic64_or_fetch, hal_atomic64_xor_fetch
.global hal_atomic64_set_fetch, hal_atomic64_cmpxchg_fetch, hal_atomic64_cmpxchg_ptr_fetch

/* @note CMPXCHG r/m64, r64 Compare RAX with r/m64. If equal, ZF is set and r64 is
	 loaded into r/m64. Else, clear ZF and load r/m64 into RAX.
 */

/**
	整数をアトミックに加え, 更新前の値を返却する
	@param[out] rdi 更新する変数のアドレス
	@param[in] esi 加算する整数
	@return 更新前の値
	@note atomic64_val hal_atomic64_add_fetch(atomic *val, atomic64_val incr)相当
*/
hal_atomic64_add_fetch:
	lock
	xaddq	%rsi,(%rdi)	/*  ret=*val, *val += incr */
	movq	%rsi, %rax	/*  return ret  */
	ret

/**
	整数間の論理積を算出し, 更新前の値を返却する
	@param[out] rdi 更新する変数のアドレス
	@param[in] esi 論理積をとる整数
	@return 更新前の値
	@note atomic64_val hal_atomic64_and_fetch(atomic *val, atomic64_val incr)相当
*/
hal_atomic64_and_fetch:
1:	
	movq	(%rdi),%rax	/* ret=*val */
	movq	%rsi,%rcx	/* tmp=incr */
	andq	%rax,%rcx	/* tmp &= *val */

	lock
	cmpxchgq %rcx,(%rdi)	/* *val = tmp if *val == ret */

	jnz	1b

	ret			/* return ret */
	
/**
	整数間の論理和を算出し, 更新前の値を返却する
	@param[out] rdi 更新する変数のアドレス
	@param[in] esi 論理和をとる整数
	@return 更新前の値
	@note atomic64_val hal_atomic64_or_fetch(atomic *val, atomic64_val incr)相当
*/
hal_atomic64_or_fetch:
1:	
	movq	(%rdi),%rax	/* ret=*val */
	movq	%rsi,%rcx	/* tmp=incr */
	orq	%rax,%rcx	/* tmp = ret | incr */

	lock
	cmpxchgq %rcx,(%rdi)	/* *val = tmp if *val == ret */

	jnz	1b

	ret			/* return ret */

/**
	整数間の排他的論理和を算出し, 更新前の値を返却する
	@param[out] rdi 更新する変数のアドレス
	@param[in] esi 排他的論理和をとる整数
	@return 更新前の値
	@note atomic64_val hal_atomic64_xor_fetch(atomic *val, atomic64_val incr)相当
*/
hal_atomic64_xor_fetch:
1:	
	movq	 (%rdi),%rax	/* ret=*val */
	movq	 %rsi,%rcx	/* tmp=incr */
	xorq	 %rax,%rcx	/* tmp = ret ^ incr */

	lock
	cmpxchgq %rcx,(%rdi)	/* *val = tmp if *val == ret */

	jnz	1b

	ret			/* return ret */
	
	
/**
	整数をアトミックに代入し, 代入前の値を返す
	@param[out] rdi 代入される変数のアドレス
	@param[in] esi 代入する整数
	@return 更新前の値
	@note int hal_atomic64_set_fetch(atomic *val, int new)相当
*/
hal_atomic64_set_fetch:
	xchgq	%rsi,(%rdi) /* *val = new */
	movq	%rdi,%rax   /* ret=*val */
	ret		    /* return ret */

/**
	更新前の整数値がoldに一致したら整数newをアトミックに代入し, 代入前の値を返す
	@param[out] rdi 代入される変数のアドレス
	@param[in] esi 更新前の値
	@param[in] edx 更新後の値
	@return 更新前の値
	@note atomic64_val hal_atomic64_cmpxchg_fetch(atomic *val, atomic64_val old, atomic64_val new)相当
*/
hal_atomic64_cmpxchg_fetch:
	movq	 %rsi, %rax	/*  tmp = old  */

	lock
	cmpxchgq %rdx, (%rdi)    /* *val = new if *val == tmp  */

	ret
	
/**
	更新前のポインタ値がoldに一致したらnewをアトミックに代入し, 代入前の値を返す
	@param[out] rdi 代入される変数のアドレス
	@param[in] rsi 更新前のポインタ値
	@param[in] rdx 更新後のポインタ値
	@return 更新前の値
	@note void *hal_atomic64_cmpxchg_ptr_fetch(void **valp, void *old, void *new)相当
*/
hal_atomic64_cmpxchg_ptr_fetch:
	movq	 %rsi, %rax	/*  tmp = old  */

	lock
	cmpxchgq %rdx, (%rdi)    /* *valp = new if *valp == tmp  */

	ret
	


/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Statistics counter routines                                       */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>

#include <klib/statcnt.h>

/**
   統計情報の値に整数を設定する
   @param[out] cnt 更新する統計情報値のアドレス
   @param[in] val 設定する整数
*/
void
statcnt_set(stat_cnt *cnt, stat_cnt_val val){

	atomic64_set(&cnt->counter, val);  /*  統計情報値を設定する  */
}

/**
   統計情報の値を獲得する
   @param[in] cnt 更新する統計情報値のアドレス
   @return 統計情報値
*/
stat_cnt_val
statcnt_read(stat_cnt *cnt){

	return atomic64_read(&cnt->counter); /*  統計情報値を読みだす  */
}

/**
   統計情報の値に整数をアトミックに加え, 更新後の値を返却する
   @param[out] cnt 更新する統計情報値のアドレス
   @param[in] val 加算する整数
   @return 更新後の値
*/
stat_cnt_val 
statcnt_add(stat_cnt *cnt, stat_cnt_val val){

	/** 統計情報値に値を加算する
	 */
	return atomic64_add_return(&cnt->counter, val);
}

/**
   統計情報の値から整数をアトミックに減算し, 更新後の値を返却する
   @param[out] cnt 更新する統計情報値のアドレス
   @param[in] val 減算する整数
   @return 更新後の値
*/
stat_cnt_val
statcnt_sub(stat_cnt *cnt, stat_cnt_val val){

	/** 統計情報値から値を減算する
	 */
	return atomic64_sub_return(&cnt->counter, val);
}

/**
   統計情報の値に整数をアトミックにインクリメントし, 更新後の値を返却する
   @param[out] cnt 更新する統計情報値のアドレス
   @return 更新後の値
*/
stat_cnt_val 
statcnt_inc(stat_cnt *cnt){

	/** 統計情報値に1加算する
	 */
	return statcnt_add(cnt, 1);
}

/**
   統計情報の値に整数をアトミックにデクリメントし, 更新後の値を返却する
   @param[out] cnt 更新する統計情報値のアドレス
   @return 更新後の値
*/
stat_cnt_val 
statcnt_dec(stat_cnt *cnt){

	/** 統計情報値から1減算する
	 */
	return statcnt_sub(cnt, 1);
}

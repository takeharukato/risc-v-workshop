/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Epoch time functions                                              */
/*                                                                    */
/*  These functions are derived from musl libc.                       */
/*                                                                    */
/**********************************************************************/

/* musl as a whole is licensed under the following standard MIT license: */
/* ---------------------------------------------------------------------- */
/* Copyright 2005-2014 Rich Felker, et al.                                */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/* ---------------------------------------------------------------------- */

#include <klib/freestanding.h>
#include <kern/kern-common.h>

#include <klib/epoch_time.h>

/** グレゴリオ暦 2000年01月01日0時0分0秒 までの秒数 */
#define MILLENNIUM_SECS ( INT64_C(946684800) )

/** 非閏年の一年の経過日数 (365日) */
#define DAYS_PER_YEAR  ( INT64_C(365) )

/** 閏年の一年の経過日数 (366日) */
#define DAYS_PER_LEAP_YEAR  ( INT64_C(366) )

/** 年間の月数 */
#define MONS_PER_YEAR       ( INT64_C(12) )

/** 3月始まりでの月をグレゴリオ暦の月に変換するための加算数 */
#define MAR2FEB_OFFSET_MON     ( INT64_C(2) )

/** 3月始まりでの月の最大値 */
#define MAR2FEB_MAX_MON        ( MONS_PER_YEAR - MAR2FEB_OFFSET_MON )

/** 1分間の経過秒数 (60秒) */
#define SECS_PER_MIN  ( INT64_C(60) )

/** 1時間の経過秒数 (3600秒) */
#define SECS_PER_HOUR  ( INT64_C(60) * SECS_PER_MIN )

/** 1時間の経過分数 (60分) */
#define MINS_PER_HOUR  ( INT64_C(60) )

/** 1日の経過秒数 (86400秒) */
#define SECS_PER_DAY  ( INT64_C(24) * SECS_PER_HOUR )

/** 非閏年の一年の経過秒数 (31536000秒) */
#define SECS_PER_YEAR  ( DAYS_PER_YEAR * SECS_PER_DAY )

/** 非閏年の一年の経過秒数 (31622400秒) */
#define SECS_PER_LEAP_YEAR  ( DAYS_PER_LEAP_YEAR * SECS_PER_DAY )

/**
 * 1週間の日数 (7日)
 */
#define DAYS_PER_WEEK        ( INT64_C(7) )

/**
 *  閏年の2月の日数
 */
#define MDAYS_IN_FEB_LEAP    ( INT64_C(29) )

/**
 *  非閏年の2月の日数
 */
#define MDAYS_IN_FEB         ( INT64_C(28) )

/**
 *  1月の日数
 */
#define MDAYS_IN_JAN         ( INT64_C(31) )

/**
   100年毎の年数
 */
#define YEARS_PER_CENTURY    ( INT64_C(100) )

/**
   400年ごとに一度閏年になる100で割り切れ, かつ, 4で割り切れる年が来る
 */
#define LEAP_YEAR_PER_400Y ( INT64_C(4) * YEARS_PER_CENTURY )

/**
   100年ごとに閏年にならない4で割り切れる年が来る
 */
#define NON_LEAP_YEAR_PER_100Y ( INT64_C(1) * YEARS_PER_CENTURY )

/**
   4ごとに閏年が来る
 */
#define LEAP_YEAR_PER_4Y      ( INT64_C(4) )

/**
   4年ごとに閏年が来る(剰余計算用に非負整数で定義)
 */
#define LEAP_YEAR_PER_4Y_FOR_MOD ( UINT64_C(4) )

/** 400年ごとの閏年の回数 (97回)
 *  100年ごとに4で割り切れるが閏年にならない年があり, 400年で割り切れる年は閏年として
 *  扱う。
 *
 *   400年/4年(閏年になる年) - 400年/100年(100年に１回閏年にならない年) 
 *      + 1年(400年に一回閏年になる年) 
 *  = 100 - 4 + 1 = 97回
*/
#define LEAP_YEARS_PER_400Y  ( INT64_C(97) )

/** 100年ごとの閏年の回数 (24回)
 *   100年ごとに4で割り切れるが閏年にならない年があるので
 *
 *   100年/4年(閏年になる年) - 1年(100年に１回閏年にならない年)  = 25 -1 = 24回
 */
#define LEAP_YEARS_PER_100Y  ( INT64_C(24) )

/** 4年ごとの閏年の回数 (1回)
 */
#define LEAP_YEARS_PER_4Y  ( INT64_C(1) )

/** 400年間の日数 閏日97回(LEAP_YEARS_PER_400Y)を足す (146097日) */
#define DAYS_PER_400Y ( DAYS_PER_YEAR*LEAP_YEAR_PER_400Y + LEAP_YEARS_PER_400Y )

/** 100年間の日数 閏日24回(LEAP_YEARS_PER_100Y)を足す (36524日) */
#define DAYS_PER_100Y ( DAYS_PER_YEAR*NON_LEAP_YEAR_PER_100Y + LEAP_YEARS_PER_100Y )

/** 4年間の日数 閏日1回(LEAP_YEARS_PER_4Y)を足す (1461日) */
#define DAYS_PER_4Y   ( DAYS_PER_YEAR*LEAP_YEAR_PER_4Y + INT64_C(1) )

/* 基準時間(2000年03月01日 00:00:00 水曜日)
   400で割り切れる年(100で割り切れるが閏年になる年)の閏日(2月29日)の翌日 00:00:00
 */
#define LEAPOCH ( MILLENNIUM_SECS + ( SECS_PER_DAY * (MDAYS_IN_JAN + MDAYS_IN_FEB_LEAP) ) )

/**
 * 基準時間(グレゴリオ暦 2000年03月01日水曜日)の曜日
 * mktime(3) (日曜日を0とした経過日数)
 */
#define LEAPOCH_WDAY         ( INT64_C(3) )

/**
   UNIXエポックタイムの最初の閏年(1972年)からUNIX時間最終年(2038年)までの
   tm構造体のtm_year値の符号なし64bit整数表現
   @note tm_yearに負が与えられた場合の比較を正しく行えるように符号なし整数で定義
*/
#define UNIXTIME_END_OF_TM_YEAR_FROM_FIRST_LEAP_YEAR   ( UINT64_C(136) )

/**
   UNIXエポックタイムから最初の閏年(1972年)までの年数(2年)
*/
#define UNIXEAPOCH_OFFSET_TO_FIRST_LEAP_YEAR   ( UINT64_C(2) )

/**
   UNIXエポックタイムの年をtm構造体のtm_yearで表した値
*/
#define UNIXEAPOCH_YEAR_IN_TM_YEAR   ( INT64_C(70) )

/**
   UNIXエポックタイムの直前の閏年(1968年)のtm構造体のtm_year値
   @note tm_yearに負が与えられた場合の比較を正しく行えるように符号なし整数で定義
*/
#define UNIXTIME_LAST_LEAP_YEAR_IN_TM_YEAR  ( UINT64_C(68) )

/**
   各月の初日までの経過日数(非閏年)
 */
enum _days_through_month{
	DAYS_THR_JAN=0,   /**<   1月1日までの経過日数 */
	DAYS_THR_FEB=31,  /**<   2月1日までの経過日数 */
	DAYS_THR_MAR=59,  /**<   3月1日までの経過日数 */
	DAYS_THR_APR=90,  /**<   4月1日までの経過日数 */
	DAYS_THR_MAY=120, /**<   5月1日までの経過日数 */
	DAYS_THR_JUN=151, /**<   6月1日までの経過日数 */
	DAYS_THR_JUL=181, /**<   7月1日までの経過日数 */
	DAYS_THR_AUG=212, /**<   8月1日までの経過日数 */
	DAYS_THR_SEP=243, /**<   9月1日までの経過日数 */
	DAYS_THR_OCT=273, /**<  10月1日までの経過日数 */
	DAYS_THR_NOV=304, /**<  11月1日までの経過日数 */
	DAYS_THR_DEC=334, /**<  12月1日までの経過日数 */
};

/**
   各月の初日までの経過秒数(非閏年)
 */
static const int64_t secs_through_month[] = {
	DAYS_THR_JAN, DAYS_THR_FEB*SECS_PER_DAY, 
	DAYS_THR_MAR*SECS_PER_DAY, DAYS_THR_APR*SECS_PER_DAY, 
	DAYS_THR_MAY*SECS_PER_DAY, DAYS_THR_JUN*SECS_PER_DAY, 
	DAYS_THR_JUL*SECS_PER_DAY, DAYS_THR_AUG*SECS_PER_DAY, 
	DAYS_THR_SEP*SECS_PER_DAY, DAYS_THR_OCT*SECS_PER_DAY, 
	DAYS_THR_NOV*SECS_PER_DAY, DAYS_THR_DEC*SECS_PER_DAY
};

/**
   3月から開始した各月の日数(閏年)
 */
static const char mar2feb_days_in_month[] = {31,30,31,30,31,31,30,31,30,31,31,29};

/**
   エポックタイムをカーネル内タイム構造体に変換する (内部関数)
   @param[in]  t  エポックタイム( 1970年1月1日00:00:00からの経過秒数 (UNIX時間) )
   @param[out] tm カーネル内タイム構造体出力先カーネル仮想アドレス
   @retval  0       正常終了
   @retval -EINVAL  年数がint型整数に収まらない
*/
static int
__secs_to_tm(int64_t t, kernel_tm *tm){
	int64_t             days, secs, years;
	int64_t    remdays, remsecs, remyears;
	int64_t qc_cycles, c_cycles, q_cycles;
	int64_t                        months;
	int64_t              wday, yday, leap;

	/* カーネル内タイム構造体中のtm_yearメンバ(int型)の範囲に収まらない年数を
	 * 引き渡した場合引数異常を返す 
	 */
	if ( ( t < INT_MIN * SECS_PER_LEAP_YEAR ) || 
	    ( t > INT_MAX * SECS_PER_LEAP_YEAR ) )
		return -EINVAL;

	secs = t - LEAPOCH; /* 基準時間(2000年03月01日 00:00:00 水曜日)からの経過秒数算出 */
	
	/*
	 * 日数算出
	 */
	days = secs / SECS_PER_DAY;  /*  秒を日数に変換 */
	remsecs = secs % SECS_PER_DAY; /* 一日の中での経過秒数を算出  */
	if ( remsecs < 0 ) {  /* 経過秒数が負の場合, 前日00:00:00からの経過秒数に変換 */

		remsecs += SECS_PER_DAY;     /*  前日中での経過秒数を算出  */
		days--;  /*  前日の経過秒数に変換したため経過日数を1日減算 */
	}

	/*
	 * 曜日の算出 (日曜日を0とし土曜日を6として算出)
	 */
	/* 基準日(水曜日=3)からの経過日数を7曜で割った余りを算出 */
	wday = ( LEAPOCH_WDAY + days ) % DAYS_PER_WEEK;
	if (wday < 0) /*  経過秒数が負の場合, 前週での曜日を算出 */
		wday += DAYS_PER_WEEK;  /* 前週での曜日を算出 */
	
	/*
	 * 最初の100年を0とした経過年数(年数-100年)を算出
	 */

	/* 400年周期での周期回数を算出
	 */
	qc_cycles = days / DAYS_PER_400Y; /* 400年周期の周期回数を算出 */
	remdays = days % DAYS_PER_400Y; /* 400年内での残り日数を算出 */
	if (remdays < 0) { /* 周期回数が負の場合, 前の400年周期内での経過日数を算出 */

		remdays += DAYS_PER_400Y; /* 400年周期内での経過日数を算出 */
		qc_cycles--; /* 前の400年周期内の経過日数を算出したので経過周期を1周期減算 */
	}

	/* 100年周期での周期回数を算出
	 */
	c_cycles = remdays / DAYS_PER_100Y; /* 100年周期の周期回数を算出 */
	/* 400年間の日数は146097日なので, 100年間の日数(36524日) * 4周期( = 146096日)を
	 * 1回含みうる。4回目の100年周期にあたる場合, 周期回数を減算することで周期0を
	 * 400年周期に該当しない年として扱えるようにする。
	 */
	if ( c_cycles == 4 ) /* 4回目の100年周期にあたる場合 */
		c_cycles--; /* 100年周期の周期回数を1周期減算し, 非閏日の2重加算を避ける */
	remdays -= c_cycles * DAYS_PER_100Y;  /* 100年内での経過日数を算出 */

	/* 4年周期での周期回数を算出
	 */
	q_cycles = remdays / DAYS_PER_4Y;  /* 4年周期の周期回数を算出 */
	/* 100年間の日数は36524日なので, 300年未満の場合, 
	 * 4年間の日数(1461日) * 25周期 ( = 36525日)を含みうる
	 * ちょうど25周期目にあたる場合は周期回数を減算することで周期0を
	 * 100年周期に該当しない年として扱えるようにする。
	 */
	if ( q_cycles == 25 ) 
		q_cycles--;  /*  4年周期の25周期目にあたる場合は閏年扱いにする */
	remdays -= q_cycles * DAYS_PER_4Y; /* 4年内での経過日数を算出 */

	remyears = remdays / DAYS_PER_YEAR;  /* 4年内での年数を算出 */
	/* 4年周期の4年目は非閏年扱いにすることで4年周期の0周期を閏年
	 * として扱えるようにする
	 */
	if ( remyears == 4 ) 
		remyears--;  /* 4年周期の4年目は非閏年扱いにする */
	remdays -= remyears * DAYS_PER_YEAR;  /*  年内での経過日数を算出 */

	/* 4年周期にあたる年で, かつ, 400年周期にあたるか100年周期にあたらなければ
	 * 閏日を加算する。
	 */
	if ( ( remyears == 0 ) && ( ( q_cycles != 0 ) || ( c_cycles == 0 ) ) )
		leap = 1;  /*  閏日を1日加算    */
	else
		leap = 0;  /*  閏日の加算は不要 */

	/* 年内での経過日数を算出する
	 * 3月始まりで計算しているため,  残日数に1月, 2月分(平年日数と必要に
	 * 応じて閏日を加算)する。
	 */
	yday = remdays + MDAYS_IN_JAN + MDAYS_IN_FEB + leap;
	if ( yday >= DAYS_PER_YEAR + leap )
		yday -= DAYS_PER_YEAR+leap; /* 次年に入った場合, 次年での経過日数を算出 */

	/*
	 * 年数を算出
	 */
	years = remyears /* 4年周期内年数 */
		+ INT64_C(4) * q_cycles  /* 4年周期回数 */
		+ YEARS_PER_CENTURY * c_cycles /* 100年周期回数 */
		+ INT64_C(4) * YEARS_PER_CENTURY * qc_cycles; /* 400年周期回数 */

	/*
	 * 月度内経過日数を算出
	 */
	for (months=0; remdays >= mar2feb_days_in_month[months]; ++months)
		remdays -= mar2feb_days_in_month[months];

	/* 
	 * 月を算出
	 */
	if ( months >= MAR2FEB_MAX_MON ) {  /* 次の年にまたがった場合 */

		months -= MONS_PER_YEAR; /* 次の年での経過月数を算出 */
		years++; /* 次の年での経過月数に変換したため経過年数を加算 */
	}

	/* カーネル内タイム構造体のtm_year(int型)に収まらない場合は,
	 * -ERANGEを返却する
	 */
	if ( ( ( years + NON_LEAP_YEAR_PER_100Y ) > INT_MAX ) ||
	    ( ( years + NON_LEAP_YEAR_PER_100Y ) < INT_MIN ) )
		return -ERANGE;

	/* 
	 * カーネル内タイム構造体の値を設定する
	 */
	tm->tm_year = years + YEARS_PER_CENTURY;  /* 年数 (100年目からの経過年に100加算)   */
	tm->tm_mon = months + MAR2FEB_OFFSET_MON; /* 月 (1から始まる3月始まりの月に2加算)  */
	tm->tm_mday = remdays + 1;               /* 月度内経過日数(0日から始まるので1加算) */
	tm->tm_wday = wday;                      /* 曜日 */
	tm->tm_yday = yday;                      /* 年内経過日数 */

	/* 一日内での経過秒数から, 時間, 分, 秒を算出
	 */
	tm->tm_hour = remsecs / SECS_PER_HOUR;                   /* 時間 */
	tm->tm_min = ( remsecs / SECS_PER_MIN ) % MINS_PER_HOUR; /* 分   */
	tm->tm_sec = remsecs % SECS_PER_MIN;                     /* 秒   */

	return 0;  /* 正常終了 */
}

/**
   経過月数を秒に変換する (内部関数)
   @param[in] month 経過月数
   @param[in] is_leap 閏年における経過月数の場合は真を非閏年の場合は偽を引き渡す
   @return 月数を秒に変換した値 (単位: 秒)
*/
static int64_t
 __month_to_secs(int64_t month, int is_leap){
	int64_t t;

	t = secs_through_month[month];   /* 月の初日までの経過秒数を算出 */
	if ( is_leap && ( month >= 2 ) ) /* 閏年で2月以降の場合は, 閏日分の秒数を加算 */
		t += SECS_PER_DAY;       /* 閏日分の秒数を加算 */

	return t;  /* 経過秒数を返却する */
}

/**
   年数を秒に変換する (内部関数)
   @param[in]  year    カーネル内タイム構造体のtm_yearメンバの値 (単位:年)
   @param[out] is_leap 閏年判定結果返却領域
   - 閏年の場合は1を引き渡された領域に設定
   - 非閏年の場合は0を引き渡された領域に設定
   @return 年数を秒に変換した値 (単位: 秒)
*/
static int64_t 
__year_to_secs(int64_t year, int *is_leap){
	int64_t                              y;
	int64_t  cycles, centuries, leaps, rem;
	int                  local_is_leap_var;

	/* 処理を単純化するためにUNIX時間最初の閏年(1972年)からの経過時間を
	 * 基準として最終UNIX時間以前の年(2038年以前の年)であることを
	 * 確認する
	 */
	if ( ( year - UNIXEAPOCH_OFFSET_TO_FIRST_LEAP_YEAR ) 
	    <= UNIXTIME_END_OF_TM_YEAR_FROM_FIRST_LEAP_YEAR ) {

		/* 最終UNIX時間(2038年01月19日 03:14:07)の年(2038年)以前の
		 * 年数を処理する(32bit time_tで扱える範囲を扱う)
		 */

		/* UNIX紀元年以前の最後の閏年(1968年)からの経過年数を算出する */
		y = year - UNIXTIME_LAST_LEAP_YEAR_IN_TM_YEAR; 
		leaps = y / LEAP_YEAR_PER_4Y; /* 経過年数中の閏年の回数を算出 */
		if ( ( y % LEAP_YEAR_PER_4Y ) == 0 ) { /* 4年で割り切れる年の場合 */

			leaps--; /* 閏年の回数を1回減算 */
			if ( is_leap != NULL )
				*is_leap = 1;  /* 指定された年を閏年として返却 */
		} else if ( is_leap != NULL )
			*is_leap = 0; /* 指定された年を非閏年として返却 */

		/* UNIX時間の紀元年(1970年)からの経過時間に年単位での経過秒数を
		 * 掛け合わせ, 閏年の発生回数分の経過秒数を足すことで秒数を返却
		 */
		return ( SECS_PER_YEAR * ( year - UNIXEAPOCH_YEAR_IN_TM_YEAR) )
			+ ( SECS_PER_DAY * leaps );
	}

	/* 閏年返却領域がNULLの場合は, 閏年返却領域がNULL, 非NULLの
	 * 双方の場合に後続の処理論理を共通化するために, ローカル変数領域を
	 * 指すようにする
	 */
	if ( is_leap == NULL ) {

		local_is_leap_var = 0;        /* 非閏年に設定       */
		is_leap = &local_is_leap_var; /* ローカル変数を参照 */
	}

	/* 100年以上の年数が指定されているので100年目を起点とした年数に
	 * 変換した上で, 400年単位での周期回数を算出する
	 */
	cycles = ( year - YEARS_PER_CENTURY ) / LEAP_YEAR_PER_400Y;

	/* 400年内での経過年数を算出する
	 */
	rem = (year - YEARS_PER_CENTURY ) % LEAP_YEAR_PER_400Y;
	if ( rem < 0 ) {  /* 過去の年数の場合  */

		cycles--;  /* 周期回数を減算する */
		rem += LEAP_YEAR_PER_400Y; /* 400年周期開始点からの経過年数を算出する */
	}

	if ( rem == 0 ) {  /* 400年周期と一致する年数の場合 */

		*is_leap = 1;  /*  400年に一度訪れる100年で割り切れる閏年として扱う */
		centuries = 0; /*  100年周期で非閏年ではない */
		leaps = 0;     /*  4年, 100年周期内の閏年分の秒数を足す必要はない */
	} else {

		if ( rem >= INT64_C(2) * YEARS_PER_CENTURY ) {
			
			/* 400年内の経過年数が200年以上ある場合
			 */
			if (rem >= INT64_C(3) * YEARS_PER_CENTURY) {

				/* 400年内の経過年数が300年以上ある場合
				 */
				centuries = 3;  /* 100年周期が3回ある */
				 /* 100年内の経過年数算出 */
				rem -= INT64_C(3) * YEARS_PER_CENTURY;
			} else {

				/* 400年内の経過年数が200年以上ある場合
				 */
				centuries = 2; /* 100年周期が2回ある */
				 /* 100年内の経過年数算出 */
				rem -= INT64_C(2) * YEARS_PER_CENTURY;
			}
		} else {

			/* 400年内の経過年数が100年以上ある場合
			 */
			if (rem >= YEARS_PER_CENTURY) {

				centuries = 1;  /* 100年周期が1回ある */
				/* 100年内の経過年数算出 */
				rem -= YEARS_PER_CENTURY;
			} else
				centuries = 0; /* 100年以内の経過年数 */
		}

		if ( rem == 0 ) {  /* 100年周期と一致する年数の場合 */

			*is_leap = 0;  /* 4で割り切れるが閏年でない年として扱う    */
			leaps = 0;     /*  4年周期内の閏年分の秒数を足す必要はない */
		} else { /* 100年周期内の閏年の発生回数を算出する */

			/*  閏年の発生回数を算出 */
			leaps = rem / LEAP_YEAR_PER_4Y_FOR_MOD;
			rem %= LEAP_YEAR_PER_4Y_FOR_MOD; /* 4年内の経過年数を算出 */
			if ( rem == 0 )
				*is_leap = 1; /* 4年に一度訪れる閏年として扱う */
			else
				*is_leap = 0; /* 非閏年として扱う */
		}
	}

	/*
	 * 閏年の発生回数を算出
	 */
	leaps +=  LEAP_YEARS_PER_400Y * cycles  /* 400年周期での閏年発生回数 */
		+ LEAP_YEARS_PER_100Y*centuries /* 100年周期での閏年発生回数 */
		- *is_leap; /* 指定された年が閏年の場合は重複して加算しないように1減算 */
	/*
	 * 経過秒数を返却
	 */
	return ( year - YEARS_PER_CENTURY ) * SECS_PER_YEAR /* 年数 * 1年あたりの秒数 */
		+ leaps * SECS_PER_DAY   /* 閏日分の秒数 */
		+ MILLENNIUM_SECS  /* 基準時間(2000年03月01日00:00:00)までの経過秒数 */
		+ SECS_PER_DAY; /* 月の初日分の経過秒数 */
}

/**
   カーネル内タイム構造体をエポックタイム ( 1970年1月1日00:00:00からの経過秒数 (UNIX時間) )に
   変換する (内部関数)
   @param[in]  tm カーネル内タイム構造体のカーネル仮想アドレス
   @return エポックタイム ( 1970年1月1日00:00:00からの経過秒数 (UNIX時間) )
*/
static int64_t
__tm_to_secs(const kernel_tm *tm){
	int      is_leap;
	int64_t     year;
	int64_t    month;
	int64_t      adj;
	int64_t        t;

	year = tm->tm_year;  /* 年 */
	month = tm->tm_mon;  /* 月 */

	if ( ( month >= MONS_PER_YEAR ) || ( month < 0 ) ) { 

		/* 1年に収まらない月を指定した場合
		 * 年と月度を補正する (mktime(3)では, 月を0から11までの範囲で指定する)
		 */
		adj = month / MONS_PER_YEAR;  /* 経過年数を算出   */
		month %= MONS_PER_YEAR;       /* 年内の月度を算出 */
		if (month < 0) { /* 負の月度を指定した場合 */

			adj--;  /*  経過年数を減算 */
			month += MONS_PER_YEAR; /* 前年開始時点からの経過月度を算出 */
		}

		year += adj; /* 月度で指定された経過年数分を加算 */
	}

	t = __year_to_secs(year, &is_leap);   /* 年を秒に変換                      */
	t += __month_to_secs(month, is_leap); /* 経過月分の秒数を加算              */
	t += SECS_PER_DAY * (tm->tm_mday-1);  /* 月度内経過日数分の秒数を加算      */
	t += SECS_PER_HOUR * tm->tm_hour;     /* 1日内での経過時間分の秒数を加算   */
	t += SECS_PER_MIN * tm->tm_min;       /* 1時間内での経過分数分の秒数を加算 */
	t += tm->tm_sec;                      /* 1分内での経過秒数を加算           */

	return t;  /*  変換した秒数(エポックタイム)を返却 */
}

/**
   カーネル内タイム構造体をエポックタイム ( 1970年1月1日00:00:00からの経過秒数 (UNIX時間) )に
   変換する
   @param[out] tmv カーネル内タイム構造体のカーネル仮想アドレス(正規化した値を返却)
   @param[out] etp エポックタイム ( 1970年1月1日00:00:00からの経過秒数 (UNIX時間) )返却領域
*/
int
clock_mk_epoch_time(kernel_tm *tmv, epoch_time *etp){
	kernel_tm new={0};
	int64_t t;

	t = __tm_to_secs(tmv);  /* 指定された日付から64bitエポックタイムを算出 */
	if ( (epoch_time)t != t )
		goto error;  /* epoch_time型に収まらない(32bit epoch_timeの場合) */

	if ( __secs_to_tm(t, &new) < 0 )
		goto error;  /* kernel_tmのメンバ型(int)に収まらない */

	/*
	 * 変換値を返却
	 */
	*tmv = new;  /* 正規化したカーネル内タイム構造体を返却 */
	*etp = t;    /* エポックタイムを返却 */

	return 0;    /* 正常終了 */

error:
	return -ERANGE;  /* レンジ外エラー */
}

/**
   エポックタイム ( 1970年1月1日00:00:00からの経過秒数 (UNIX時間) )をカーネル内タイム構造体に
   変換する
   @param[in]  et  エポックタイム ( 1970年1月1日00:00:00からの経過秒数 (UNIX時間) )
   @param[out] tmv カーネル内タイム構造体返却先のカーネル仮想アドレス
*/
int
clock_epoch_time_to_tm(epoch_time et, kernel_tm *tmv){
	int        rc;
	kernel_tm new;

	kassert( tmv != NULL );  /* 返却先を指定していることを確認する */

	memset(&new, 0, sizeof(kernel_tm)); /* カーネル内タイマ構造体をゼロ初期化 */

	rc = __secs_to_tm(et, &new);  /* エポックタイムをカーネル内タイマ構造体に変換 */
	if ( rc < 0 )
		return -ERANGE;  /* エラーの場合はレンジ外エラーを返却 */

	/* 変換した値を指定された領域にコピーする
	 */
	memmove(tmv, &new, sizeof(kernel_tm));  

	return 0;  /*  正常終了 */
}

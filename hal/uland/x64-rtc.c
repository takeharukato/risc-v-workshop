/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Pseudo RTC operations                                             */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>

#include <kern/timer.h>

#include <time.h>

/**
   Real Time Clock (RTC)から時刻を読み出す
   @param[out] ktsp 時刻返却領域
   @retval   0      正常終了
   @retval  -ENODEV RTCを読み出せなかった
 */
int
hal_read_rtc(ktimespec *ktsp) {
	int             rc;
	struct timespec ts;

	rc = clock_gettime(CLOCK_REALTIME, &ts);  /* 時刻の取得 */
	kassert( rc == 0 );
	
	ktsp->tv_sec = ts.tv_sec;   /* 秒 */
	ktsp->tv_nsec = ts.tv_nsec; /* ナノ秒 */

	return rc;
}

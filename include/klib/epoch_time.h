/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Epoch time relevant definitions                                   */
/*                                                                    */
/**********************************************************************/
#if !defined(_KLIB_EPOCH_TIME_H)

#define _KLIB_EPOCH_TIME_H

#include <klib/freestanding.h>

#if defined(_BSD_SOURCE) || defined(_GNU_SOURCE)
#define __tm_gmtoff tm_gmtoff
#define __tm_zone tm_zone
#endif  /*  _BSD_SOURCE || _GNU_SOURCE  */

typedef struct _kernel_tm {
	int tm_sec;
	int tm_min;
	int tm_hour;
	int tm_mday;
	int tm_mon;
	int tm_year;
	int tm_wday;
	int tm_yday;
	int tm_isdst;
	long __tm_gmtoff;
	const char *__tm_zone;
}kernel_tm;

int clock_mk_epoch_time(struct _kernel_tm *_tmv, epoch_time *_etp);
int clock_epoch_time_to_tm(epoch_time _et, struct _kernel_tm *_tmv);
#endif  /*  _KLIB_EPOCH_TIME_H  */

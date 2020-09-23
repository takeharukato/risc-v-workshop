/* -*- mode: c; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  RISC-V64 Android Goldfish RTC on QEMU virt                        */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>

#include <kern/irq-if.h>
#include <kern/timer.h>

#include <hal/rv64-platform.h>
#include <hal/hal-traps.h>

/**
   QEMU Android Goldfish RTC レジスタアクセス
   @param[in] _reg レジスタオフセット
   @return レジスタの仮想アドレス
 */
#define RV64_RTC_REG(_reg) \
	((volatile uint32_t *)(RV64_RTC + (_reg)))

/**
   Real time clockから時刻を読み出す
   @param[out] ktsp 時刻返却領域
   @retval   0 正常終了
   @retval 非0 RTCを読み出せなかった
 */
int
hal_read_rtc(ktimespec *ktsp) {
	uint32_t low, high1, high2;
	uint64_t val;
	
	do{
		high1 = *RV64_RTC_REG(RV64_RTC_TIME_HIGH_REG); /* 上位32ビットを詠み込む */
		low = *RV64_RTC_REG(RV64_RTC_TIME_LOW_REG);    /* 下位32ビットを詠み込む */
		high2 = *RV64_RTC_REG(RV64_RTC_TIME_HIGH_REG); /* 上位32ビットを詠み込む */
	} while( high2 != high1 );  /* 上位32ビットの値に変化があったら読み直す */

	val = ((uint64_t)high1) << 32 | low;  /* 64ビット長の値に変換する */

	if ( ktsp != NULL ) {

		/* 時刻返却領域がNULLでなければ, 値をtimespecに変換して返却する
		 * Android Goldfish RTCは, エポック時間からの経過ナノ秒を64ビット長の
		 * 符号なし整数として返却するので, 秒とナノ秒に分解して返却する
		 */
		ktsp->tv_sec = val / TIMER_NS_PER_SEC;  /* エポック時間からの経過秒     */
		ktsp->tv_nsec = val % TIMER_NS_PER_SEC; /* エポック時間からの経過ナノ秒 */
	}

	return 0;
}

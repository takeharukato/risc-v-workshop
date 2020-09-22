/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  RISC-V64 preparation                                              */
/*                                                                    */
/**********************************************************************/
#if !defined(_HAL_RV64_PREPARE_H)
#define  _HAL_RV64_PREPARE_H 

#include <klib/freestanding.h>

#include <kern/kern-types.h>
#include <kern/spinlock.h>


/**
   初期化処理用データ構造
 */
typedef struct _rv64_prepare_info{
	spinlock         lock;  /**< 初期化処理排他用ロック               */
	spinlock console_lock;  /**< 初期化処理時のコンソールロック       */
	cpu_id       bsp_hart;  /**< Boot Service Processor (BSP)のhartid */
	cpu_id      boot_cpus;  /**< 起動済みCPU数                        */
}rv64_prepare_info;

/**
     初期化処理用データ構造初期化子
 */
#define __RV64_PREPARE_INFO_INITIALIZER			 \
	{						 \
		.lock = __SPINLOCK_INITIALIZER,		 \
		.console_lock  = __SPINLOCK_INITIALIZER, \
		.bsp_hart = 0,			         \
		.boot_cpus = 0,                          \
	}

void rv64_prepare(reg_type _hartid);
#endif  /*  _HAL_RV64_PREPARE_H  */

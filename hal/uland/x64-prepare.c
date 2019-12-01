/* -*- mode: c; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  RISC-V preparation routines                                       */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>
#include <kern/kern-if.h>
#include <kern/spinlock.h>
#include <kern/page-if.h>
#include <kern/vm-if.h>
/**
   カーネル初期化後のアーキ固有初期化処理
 */
void
hal_platform_init(void){

}

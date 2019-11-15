/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  cpu level interrupt control                                       */
/*                                                                    */
/**********************************************************************/
#if !defined(_KERN_PGTBL_H)
#define  _KERN_PGTBL_H 

#include <klib/freestanding.h>
#include <kern/kern-types.h>
typedef struct _pgtbl{
	void *pgtbl_base;  /* ページテーブルベース */
}pgtbl;
#endif  /*  _KERN_PGTBL_H   */

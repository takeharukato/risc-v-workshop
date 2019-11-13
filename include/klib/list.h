/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*   list relevant definitions                                        */
/*                                                                    */
/**********************************************************************/
#if !defined(_KERN_LIST_H)
#define _KERN_LIST_H

#include <klib/freestanding.h>

#include <klib/container.h>

/** リスト構造体
 */
typedef struct _list{
	struct _list *prev;       /**<  前の要素へのポインタ  */
	struct _list *next;       /**<  後の要素へのポインタ  */
}list;

void list_del(struct _list *_node);
void list_init(struct _list *_node);
int  list_not_linked(struct _list *_node);
#endif  /*  _KERN_LIST_H  */

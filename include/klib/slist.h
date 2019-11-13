/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*   single list relevant definitions                                 */
/*                                                                    */
/**********************************************************************/
#if !defined(_KERN_SLIST_H)
#define _KERN_SLIST_H

#include <klib/freestanding.h>

#include <klib/container.h>

/** 単方向リストノード構造体
 */
typedef struct _slist_node{
	struct _slist_node *next;       /**<  後の要素へのポインタ  */
}slist_node;

/** 単方向リストのヘッド
 */
typedef struct _slist_head{
	struct _slist_node *next;       /**<  後の要素へのポインタ  */
}slist_head;

/**
   単方向リストヘッドの初期化子
   @param[in] _head 単方向リストヘッドのポインタ
 */
#define __SLIST_HEAD_INITIALIZER(_head)  \
	{			    \
		.next = NULL,	    \
	}

/**
    単方向リストの中を順に探索するマクロ
    @param[in] _itr    イテレータ (単方向リストノードのポインタの名前)
    @param[in] _sh     単方向リストヘッドへのポインタ
 */
#define slist_for_each(_itr, _sh)					\
	for((_itr) = slist_ref_top((struct _slist_head *)(_sh));	\
	    (_itr) != NULL;						\
	    (_itr) = (_itr)->next) 

/**
    単方向リストの中を順に探索するマクロ
    @param[in] _itr  イテレータ (単方向リストノードのポインタの名前)
    @param[in] _sh   単方向リストヘッドへのポインタ
    @param[in] _np   次の要素を指し示しておくポインタ
 */
#define slist_for_each_safe(_itr, _sh, _np)				\
	for((_itr) = slist_ref_top((struct _slist_head *)(_sh)),	\
		    (_np) = ((_itr) != NULL) ? (_itr)->next : NULL;	\
	    (_itr) != NULL;						\
	    (_itr) = (_np),			\
		    (_np) = ((_itr) != NULL) ? (_itr)->next : NULL) 

void slist_init_head(struct _slist_head *_head);
struct _slist_node *slist_ref_top(struct _slist_head *head);
struct _slist_node *slist_get_top(struct _slist_head *head);
void slist_add(struct _slist_head *_head, struct _slist_node *_node);
void slist_del(struct _slist_head *_head, struct _slist_node *_node);
bool slist_is_empty(struct _slist_head *_head);
void slist_init_node(struct _slist_node *_node);
int  slist_not_linked(struct _slist_node *_node);
#endif  /*  _KERN_LIST_H  */

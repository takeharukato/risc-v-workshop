/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  queue operation definitions                                       */
/*                                                                    */
/**********************************************************************/
#if !defined(_KERN_QUEUE_H)
#define _KERN_QUEUE_H

#include <klib/freestanding.h>

#include <klib/container.h>
#include <klib/list.h>

#define QUEUE_ADD_ASCENDING  (0)  /**< 昇順でキューにつなぐ  */
#define QUEUE_ADD_DESCENDING (1)  /**< 降順でキューにつなぐ  */

/** キュー構造体
 */
typedef struct _queue{
	struct _list *prev;       /*<  最後尾の要素へのポインタ  */
	struct _list *next;       /*<  先頭要素へのポインタ      */
}queue;

/**
   キュー構造体の初期化子
   @param[in] _que キューへのポインタ
 */
#define __QUEUE_INITIALIZER(_que)			\
	{						\
		.prev = (struct _list *)(_que),		\
		.next = (struct _list *)(_que),		\
	}

/**
    キューの中を順に探索するマクロ
    @param[in] _itr    イテレータ (list構造体のポインタの名前)
    @param[in] _que    キューへのポインタ
 */
#define queue_for_each(_itr, _que)				   \
	for((_itr) = queue_ref_top((struct _queue *)(_que));	   \
	    (_itr) != ((struct _list *)(_que));			   \
	    (_itr) = (_itr)->next) 

/**
   キューの中を順に探索するマクロ (ループ内でのキューの変更がある場合)
    @param[in] _itr  イテレータ (list構造体のポインタの名前)
    @param[in] _que  キューへのポインタ
    @param[in] _np   次の要素を指し示しておくポインタ
 */
#define queue_for_each_safe(_itr, _que, _np)				\
	for((_itr) = queue_ref_top((struct _queue *)(_que)), (_np) = (_itr)->next; \
	    (_itr) != ((struct _list *)(_que));				\
	    (_itr) = (_np), (_np) = (_itr)->next ) 

/**
   キューの中を逆順に探索するマクロ
    @param[in] _itr    イテレータ (list構造体のポインタの名前)
    @param[in] _que   キューへのポインタ
 */
#define queue_reverse_for_each(_itr, _que)		      \
	for((_itr) = queue_ref_last((struct _queue *)(_que)); \
	    (_itr) != (struct _list *)(_que);		      \
	    (_itr) = (_itr)->prev ) 

/**
   キューの中を逆順に探索するマクロ (ループ内でのキューの変更がある場合)
    @param[in] _itr  イテレータ (list構造体のポインタの名前)
    @param[in] _que  キューへのポインタ
    @param[in] _np   次の要素を指し示しておくポインタ
 */
#define queue_reverse_for_each_safe(_itr, _que, _np)			\
	for((_itr) = queue_ref_last((struct _queue *)(_que)), (_np) = (_itr)->prev; \
	    (_itr) != (struct _list *)(_que);				\
	    (_itr) = (_np), (_np) = (_itr)->prev ) 

/**
    リストをソートしてつなぐマクロ
    @param[in] _que     キューへのポインタ
    @param[in] _type    リストを埋め込んだデータ構造への型
    @param[in] _member  埋め込んだデータ構造のリストメンバの名前
    @param[in] _nodep   追加するリストノード
    @param[in] _cmp     比較関数へのポインタ
    @param[in] _how     順序指定（昇順または降順)
 */
#define queue_add_sort(_que, _type, _member, _nodep, _cmp, _how) do{	     \
		struct _list   *_lp;					     \
		struct _list *_next;					     \
		_type    *_elem_ref;					     \
		_type    *_node_ref;					     \
									     \
		(_node_ref) =						     \
			container_of(_nodep, _type, _member);		     \
									     \
		_lp = queue_ref_top((struct _queue *)(_que));		     \
		_next = _lp->next;					     \
		do{							     \
									     \
			(_elem_ref) =				 	     \
				container_of(_lp, _type, _member);	     \
									     \
			if ( ( (_next) == (struct _list *)(_que) )    ||     \
			    ( ( (_how) == QUEUE_ADD_ASCENDING ) &&	     \
				( _cmp((_node_ref), (_elem_ref)) < 0 ) ) ||  \
			    ( ( (_how) == QUEUE_ADD_DESCENDING ) &&	     \
				( _cmp((_node_ref), (_elem_ref)) > 0 ) ) ) { \
									     \
				queue_add_before((_lp),(_nodep));	     \
				break;					     \
			}						     \
									     \
			_lp = _next;					     \
			_next = _lp->next;				     \
		}while( _lp != (struct _list *)(_que) );		     \
	}while(0)

/**
    リストを埋め込んだデータ構造を順にたどって要素を見つけ出すマクロ
    @param[in] _que     キューへのポインタ
    @param[in] _type    リストを埋め込んだデータ構造への型
    @param[in] _member  埋め込んだデータ構造のリストメンバの名前
    @param[in] _keyp    比較するキー(リストを埋め込んだデータ構造)へのポインタ
    @param[in] _cmp     比較関数へのポインタ
    @param[out] _elemp  見つけた要素(リストを埋め込んだデータ構造)を指し示すポインタのアドレス
 */
#define queue_find_element(_que, _type, _member, _keyp, _cmp, _elemp) do{ \
		struct _list *_lp;					\
		_type  *_elem_ref;					\
									\
		*((_type **)(_elemp)) = NULL;				\
									\
		queue_for_each((_lp), (_que)) {				\
									\
			(_elem_ref) =					\
				container_of(_lp, _type, _member);	\
									\
			if ( _cmp((_keyp), (_elem_ref)) == 0 ) {	\
									\
				*((_type **)(_elemp)) =	(_elem_ref);	\
				break;					\
			}						\
		}							\
	}while(0)

/**
   リストを埋め込んだデータ構造を逆順にたどって要素を見つけ出すマクロ
    @param[in] _que     キューへのポインタ
    @param[in] _type    リストを埋め込んだデータ構造への型
    @param[in] _member  埋め込んだデータ構造のリストメンバの名前
    @param[in] _keyp    比較するキー(リストを埋め込んだデータ構造)へのポインタ
    @param[in] _cmp     比較関数へのポインタ
    @param[out] _elemp  見つけた要素(リストを埋め込んだデータ構造)を指し示すポインタのアドレス
 */
#define queue_reverse_find_element(_que, _type, _member, _keyp, _cmp, _elemp) do{ \
		struct _list *_lp;					\
		_type  *_elem_ref;					\
									\
		*((_type **)(_elemp)) = NULL;				\
									\
		queue_reverse_for_each((_lp), (_que)) {			\
									\
			(_elem_ref) =					\
				container_of(_lp, _type, _member);	\
									\
			if ( _cmp((_keyp), (_elem_ref)) == 0 ) {	\
									\
				*((_type **)(_elemp)) =	(_elem_ref);	\
				break;					\
			}						\
		}							\
	}while(0)

void queue_add(struct _queue *_head, struct _list *_node);
void queue_add_top(struct _queue *_head, struct _list *_node);
void queue_add_before(struct _list *_target, struct _list *_node);
void queue_add_after(list *_target, list *_node);
bool queue_del(struct _queue *_head, struct _list *_node);
struct _list *queue_ref_top(struct _queue *_head);
struct _list *queue_get_top(struct _queue *_head);
struct _list *queue_ref_last(struct _queue *_head);
struct _list *queue_get_last(struct _queue *_head);
void queue_rotate(struct _queue *_head);
void queue_reverse_rotate(struct _queue *_head);
bool queue_is_empty(struct _queue *_head);
void queue_init(struct _queue *_head);
#endif  /*  _KERN_QUEUE_H  */

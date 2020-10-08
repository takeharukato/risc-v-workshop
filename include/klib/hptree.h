/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Heap tree relevant definitions                                    */
/*                                                                    */
/**********************************************************************/
#if !defined(_KLIB_HPTREE_H)
#define  _KLIB_HPTREE_H

#include <klib/freestanding.h>

#define HPTREE_TYPE_MIN  (0)  /*< 最小値取得用ヒープ  */
#define HPTREE_TYPE_MAX  (1)  /*< 最大値取得用ヒープ  */

/** ヒープノード
 */
typedef struct _heap_node {
	struct _heap_node *parent;  /*< 親ノードへのポインタ  */
	struct _heap_node   *left;  /*< 左ノードへのポインタ  */
	struct _heap_node  *right;  /*< 右ノードへのポインタ  */
}heap_node;

/** ヒープツリー
 */
typedef struct _heap_tree {
	struct _heap_node *root; /*< 根ノード       */
	int               count; /*< ヒープの要素数 */
	int           heap_type; /*< ヒープのタイプ */
	int (*compare)(void *_d1, void *_d2);  /*< 比較関数へのポインタ */
	void (*delete_notifier)(void *_d);  /*< 削除通知関数へのポインタ */
	/*< 表示関数へのポインタ */
	void (*show_handler)(struct _heap_tree *_h, struct _heap_node *_n, void *_d);
	uintptr_t        offset; /*< 埋め込みデータ構造へのオフセット  */
}heap_tree;

/**
   ヒープノードを埋め込んだ構造体のアドレスをヒープノードメンバの
   アドレスから算出する
   @param[in] p   ヒープノードメンバのアドレス
   @param[in] off ヒープノードメンバのオフセット
 */
#define hptree_container(p, off) \
	( (void *)( ( (void *)(p) ) - ( (void *)(off) ) ) )

/**
   ヒープノードメンバの構造体中のオフセットアドレスを求める
   @param[in] t 構造体の型
   @param[in] m 構造体のヒープノードメンバ名
 */
#define hptree_offset(t, m) \
	( (uintptr_t)offset_of(t, m) )

int hptree_create(int _heap_type, uintptr_t offset,
    int (*compare)(void *_d1, void *_d2), void (*delete_notifier)(void *d),
    void (*show_handler)(struct _heap_tree *_h, struct _heap_node *_n, void *_d),
		  struct _heap_tree **_hptp);
void hptree_destroy_heap(struct _heap_tree **_hp);
void hptree_delete_node(struct _heap_tree *_h, struct _heap_node *_n);
void hptree_insert_node(struct _heap_tree *_h, struct _heap_node *_n);
int hptree_get_root(struct _heap_tree *_h, struct _heap_node **_np);
void hptree_show(struct _heap_tree *_h, struct _heap_node *_n);
#endif  /*  _KLIB_HPTREE_H   */

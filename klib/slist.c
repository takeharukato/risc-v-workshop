/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  single list routines                                              */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>

#include <klib/slist.h>

/**
   単方向リスト中の指定されたノードを指しているノードを参照する(内部関数)
   @param[in]  head 操作対象リストヘッド
   @param[in]  node 探査対象リストノード
   @param[out] refp 指定されたノードを指しているノードのアドレスを返却する領域
   @retval  0      正常終了
   @retval -ENOENT 指定されたノードがリスト中にない
 */
static int
find_slist_node_referer(slist_head *head, slist_node *node, slist_node **refp){
	slist_node *ref;

	/*
	 * 指定されたノードを指しているノードを探索する
	 */
	for(ref = (slist_node *)head; ref->next != NULL; ref = ref->next) {

		if ( ref->next == node ) {  /*  ノードが見つかった  */

			*refp = ref;
			return 0;
		}
	}

	return -ENOENT;  /*  指定されたノードを指している要素がなかった  */
}

/**
   単方向リストのリストヘッドを初期化する
   @param[in] head 捜査対象リストヘッド
 */
void
slist_init_head(slist_head *head){

	head->next = NULL;
}

/**
   単方向リストの先頭にノードを追加する
   @param[in] head 追加先リストヘッド
   @param[in] node リストノード
 */
void
slist_add(slist_head *head, slist_node *node){

	node->next = head->next;  /*  現在の先頭ノードを指す  */
	head->next = node; /* 追加したノードを先頭にする  */
}

/**
   単方向リストからノードを削除する
   @param[in] head 操作対象リストヘッド
   @param[in] node 削除するリストノード
 */
void
slist_del(slist_head *head, slist_node *node){
	int          rc;
	slist_node *ref;

	/*
	 * 削除対象ノードを指しているノードを探索する
	 */
	rc = find_slist_node_referer(head, node, &ref);
	kassert( rc == 0 );

	ref->next = node->next;  /*  削除するノードの次のノードを指す  */
	slist_init_node(node); /*  未接続ノードに設定する  */
}

/**
   単方向リストが空であることを確認する
   @param[in] head 操作対象リストヘッド
 */
bool
slist_is_empty(slist_head *head){

	return head->next == NULL;
}

/**
   単方向リストの最初の要素を参照する
   @param[in] head 捜査対象リストヘッド
 */
slist_node *
slist_ref_top(slist_head *head){

	return head->next;
}

/**
   単方向リストの最初の要素を取得する
   @param[in] head 捜査対象リストヘッド
 */
slist_node *
slist_get_top(slist_head *head){
	slist_node *node;

	if ( slist_is_empty(head) )
		return NULL;

	node = slist_ref_top(head); /*  先頭ノードを参照  */
	slist_del(head, node);  /*  先頭ノードをリストから削除  */

	return node;
}

/**
   単方向リストノードを初期化する
   @param[in] node 操作対象リストノード
 */
void
slist_init_node(slist_node *node){

	node->next = NULL;
}
/**
   単方向リストノードが未接続であることを確認する
   @param[in] node 操作対象リストノード
 */
int
slist_not_linked(slist_node *node){

	return node->next == NULL;
}

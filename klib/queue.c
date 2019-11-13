/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  queue routines                                                    */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>

#include <klib/list.h>
#include <klib/queue.h>

/**
   リストノードをキューの末尾に追加する
   @param[in] head 追加先キュー
   @param[in] node リストノード
 */
void
queue_add(queue *head, list *node) {

	node->next = (list *)head;
	node->prev = head->prev;
	head->prev->next = node;
	head->prev = node;
}

/**
   リストノードをキューの先頭に追加する
   @param[in] head 追加先キュー
   @param[in] node リストノード
 */
void
queue_add_top(queue *head, list *node) {

	node->next = head->next;
	node->prev = (list *)head;
	head->next->prev = node;
	head->next = node;
}

/**
    リストノードを指定したノードの直前につなぐ
    @param[in] target 追加先ノード
    @param[in] node   追加するノード
 */
void
queue_add_before(list *target, list *node) {

	node->prev = target->prev;
	node->next = target;
	target->prev->next = node;
	target->prev = node;
}

/**
    リストノードを指定したノードの直後につなぐ
    @param[in] target 追加先ノード
    @param[in] node   追加するノード
 */
void
queue_add_after(list *target, list *node) {

	node->next = target->next;
	node->prev = target;
	target->next->prev = node;
	target->next = node;
}

/**
   キューからノードを切り離す
    @param[in] head 操作対象キュー
    @param[in] node 切り離すノード
    @retval 真 キューが空になった
    @retval 偽 操作後のキューが空でない
 */
bool
queue_del(queue *head, list *node) {

	list_del(node);
	return queue_is_empty(head);
}

/**
   キューの先頭ノードを参照する
    @param[in] head 調査対象キュー
    @retval 先頭リストノードのアドレス
    @note 参照するだけでリストノードの取り外しは行わない
 */
list *
queue_ref_top(queue *head) {
	
	return head->next;
}

/**
   キューの先頭ノードを取り出す
    @param[in] head 操作対象キュー
    @retval 先頭リストノードのアドレス
 */
list *
queue_get_top(queue *head) {
	list *top;

	top = queue_ref_top(head);
	list_del(top);

	return top;
}

/**
   キューの末尾ノードを参照する
    @param[in] head 調査対象キュー
    @retval 末尾のリストノードのアドレス
    @note 参照するだけでリストノードの取り外しは行わない
 */
list *
queue_ref_last(queue *head) {
	
	return head->prev;
}

/**
   キューの末尾ノードを取り出す
    @param[in] head 操作対象キュー
    @retval 末尾のリストノードのアドレス
 */
list *
queue_get_last(queue *head) {
	list *last;

	last = queue_ref_last(head);
	list_del(last);

	return last;
}

/**
   キューを回転する
    @param[in] head 操作対象キュー
 */
void
queue_rotate(queue *head) {

	queue_add(head, queue_get_top(head));
}

/**
   キューを逆回転する
    @param[in] head 操作対象キュー
 */
void
queue_reverse_rotate(queue *head){

	queue_add_top(head, queue_get_last(head));
}

/**
   キューが空であることを確認する
    @param[in] head 調査対象キュー
 */
bool
queue_is_empty(queue *head) {
	
	return  (head->prev == (list *)head) && (head->next == (list *)head);
}

/**
   キューを初期化する
    @param[in] head 操作対象キュー
 */
void 
queue_init(queue *head) {

	head->prev = head->next = (list *)head;	
}

/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  hash relevant definitions                                         */
/*                                                                    */
/**********************************************************************/
#if !defined(_KLIB_HASH_H)
#define  _KLIB_HASH_H 

#include <klib/freestanding.h>
#include <klib/container.h>
#include <klib/splay.h>

struct _hash_table;

/** ハッシュテーブルの要素(データ構造中に埋め込んで使用する)
 */
typedef struct _hash_link{
	void                 *key;  /*<  テーブル登録時に使用したキー       */
	struct _hash_table   *tbl;  /*<  登録先のハッシュテーブルのアドレス */
	SPLAY_ENTRY(_hash_link) link;  /*<  ハッシュテーブルのリンク情報    */
}hash_link;

/** ハッシュテーブル
 */
typedef struct _hash_table {
	SPLAY_HEAD(hash_table_entries, _hash_link) *table;  /*<  ハッシュテーブル  */
	int64_t      link_offset;  /*< ハッシュに格納されたデータ構造の
				    *  hash_link構造体へのオフセット
				    * (単位:バイト)
				    */
	int           table_size;  /*< ハッシュテーブル長(単位:配列エントリ数)
				    */
	int            num_elems;  /*< ハッシュテーブル内に含まれる
				    *  データ構造の数(単位:個)
				    */
	int (*compare_func)(void *key, void *e);  /*<  keyとeに含まれる
						   *   値情報との比較関数
						   */
	int (*hash_func)(void *key, int size);    /*<  ハッシュ値生成関数  */
	void (*key_destroy_func)(void *_key);     /*<  キー開放関数    */
	void (*value_destroy_func)(void *_value); /*<  値開放関数      */
}hash_table;

/**
   埋め込みデータ構造中のハッシュリンクのリンク情報のオフセットを得る
   @param[in] t データ構造の型
   @param[in] m データ構造中のhash_link型データの名称
*/
#define hash_link_offset(t, m)                 \
	( (uintptr_t)( offset_of(t,m) ) )

/**
   埋め込みデータ構造中のハッシュリンクのリンク情報のアドレスを得る
   @param[in] tbl ハッシュテーブルのデータ構造のアドレス
   @param[in] v   ハッシュテーブル中のデータ構造のアドレス
 */
#define hash_link_offset_from_value(tbl, v)			\
	( (struct _hash_link *)( (void *)(v) + ( (tbl)->link_offset ) ) )

/**
   埋め込みデータ構造中のハッシュリンクのリンク情報のリンクエントリアドレス
 */
#define HASH_LINK_ENTRY_OFFSET	\
	((uintptr_t)offset_of(struct _hash_link, link))

/**
   ハッシュリンクデータ構造中のリンク情報のアドレスから
   ハッシュリンクデータ構造のアドレスを得る
   @param[in] tbl ハッシュテーブルのデータ構造のアドレス
   @param[in] lp  ハッシュリンクデータ構造中のリンク情報のアドレス
*/
#define hash_link_entry(tbl, lp)			\
	((struct _hash_link *)( (void *)(lp) - (int64_t)(tbl)->link_offset))

/**
    ハッシュリンクデータ構造中のリンク情報のアドレスから埋め込みデータ構造の
    アドレスを得る
    @param[in] tbl ハッシュテーブルのデータ構造のアドレス
    @param[in] lp  ハッシュリンクデータ構造中のリンク情報のアドレス
 */
#define hash_link_container_of(tbl, lp)			\
	( (void *)hash_link_entry(tbl, lp) - HASH_LINK_ENTRY_OFFSET )

void hash_for_each(void *_htbl, int (*_callback)(void *_htbl, void *_k, void *_v));
void hash_for_each_safe(void *_htbl, int (*callback)(void *_htbl, void *k, void *v));
int hash_lookup(void *_htbl, void *_key, void **_pp);
int hash_remove(void *_htbl, void *_key);
int hash_insert(void *_htbl, void *_key, void *_val);
void hash_destroy(struct _hash_table *_t);
int hash_create(int table_size, int64_t _link_offset,
		struct _hash_table **_tablep,
		int compare_func(void *_key, void *_e),
		int hash_func(void *_key, int _size),
		void (*key_destroy_func)(void *_key), 
		void (*value_destroy_func)(void *_value));
#endif  /*  _KLIB_HASH_H   */

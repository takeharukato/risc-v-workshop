/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  kernel level hash routines                                        */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>

#include <page/page-if.h>

#include <klib/hash.h>

/**
   ハッシュテーブルエントリの比較関数
   @param[in] e1 比較対象のエントリ1
   @param[in] e2 比較対象のエントリ2
 */
static int 
hash_entry_cmp(struct _hash_link *e1, struct _hash_link *e2) {
	
	kassert( e1 != NULL );
	kassert( e2 != NULL );
	kassert( e1->tbl != NULL );
	kassert( e1->tbl->compare_func != NULL );

	return e1->tbl->compare_func(e1->key, e2->key);
}

SPLAY_GENERATE_STATIC(hash_table_entries, _hash_link, link, hash_entry_cmp);

/**
   ハッシュテーブルのすべてのキー, 値についてコールバックを実行する
   @param[in] htbl         操作対象のハッシュテーブル
   @param[in] callback     コールバック関数
 */
void
hash_for_each(void *htbl, int (*callback)(void *_t, void *_k, void *_v)){
	struct _hash_table *t = htbl;
	hash_link              *elem;
	void                  *value;
	int                    i, rc;

	kassert( htbl != NULL );
	kassert( t->compare_func != NULL );
	kassert( t->hash_func != NULL );
	kassert( callback != NULL );

	for(i = 0; t->table_size>i; ++i) {

		SPLAY_FOREACH(elem, hash_table_entries, &t->table[i]) {

			value = hash_link_container_of(t, &elem->link);
			rc = callback( htbl, elem->key, value );
			if ( rc != 0 )
				break;
		}
	}
}

/**
   ハッシュテーブルのすべてのキー, 値についてリンクを外しうるコールバックを実行する
   @param[in] htbl         操作対象のハッシュテーブル
   @param[in] callback     コールバック関数
 */
void
hash_for_each_safe(void *htbl, int (*callback)(void *_t, void *_k, void *_v)){
	struct _hash_table *t = htbl;
	hash_link              *elem;
	void                  *value;
	int                    i, rc;
	hash_link              *next;

	kassert( htbl != NULL );
	kassert( t->compare_func != NULL );
	kassert( t->hash_func != NULL );
	kassert( callback != NULL );

	for(i = 0; t->table_size>i; ++i) {

		SPLAY_FOREACH_SAFE(elem, hash_table_entries, &t->table[i], next) {

			value = hash_link_container_of(t, &elem->link);
			rc = callback( htbl, elem->key, value );
			if ( rc != 0 )
				break;
		}
	}
}

/**
   ハッシュテーブルから指定されたキーを持った要素を検索する
   @param[in] htbl    操作対象のハッシュテーブル
   @param[in] key     検索対象のキー
   @param[out] pp     見つけた要素へのポインタを返却する領域
   @retval  0         要素を見つけた
   @retval -ENOENT;   要素を見つけられなかった
 */
int
hash_lookup(void *htbl, void *key, void **pp){
	struct _hash_table *t = htbl;
	hash_link        find, *elem;
	int                     hash;
	
	kassert( htbl != NULL );
	kassert( t->compare_func != NULL );
	kassert( t->hash_func != NULL );

	hash = t->hash_func(key, t->table_size);
	kassert( ( 0 <= hash ) && ( hash < t->table_size  ) );

	find.tbl = t;
	find.key = key;
	elem = SPLAY_FIND(hash_table_entries, &t->table[hash], &find);
	if ( elem == NULL )
		return -ENOENT;

	if ( pp != NULL )
		*pp = hash_link_container_of(t, &elem->link);

	return 0;
}

/**
   ハッシュテーブルから要素を削除する
   @param[in] htbl 操作対象のハッシュテーブル
   @param[in] key        削除に使用するキー
   @retval  0            正常削除
   @retval -ENOENT;      削除対象のエントリが見つからなかった
 */
int
hash_remove(void *htbl, void *key){
	struct _hash_table *t = htbl;
	hash_link        find, *elem;
	void                      *v;
	int                     hash;

	kassert( htbl != NULL );
	kassert( t->hash_func != NULL );

	hash = t->hash_func(key, t->table_size);
	kassert( ( 0 <= hash ) && ( hash < t->table_size  ) );

	find.tbl = t;
	find.key = key;
	elem = SPLAY_FIND(hash_table_entries, &t->table[hash], &find);
	if ( elem == NULL )
		return -ENOENT;

	SPLAY_REMOVE(hash_table_entries, &t->table[hash], elem);
	v = hash_link_container_of(t, &elem->link);

	if ( t->key_destroy_func != NULL )
		t->key_destroy_func( key );

	if ( t->value_destroy_func != NULL )
		t->value_destroy_func( v );

	--t->num_elems;

	return 0;
}

/**
   ハッシュテーブルに要素を追加する
   @param[in] htbl       追加対象のハッシュテーブル
   @param[in] key        追加に使用するキー
   @param[in] val        追加対象の要素
   @retval  0            正常追加
   @retval -EEXIST       すでにキーが登録済み
 */
int
hash_insert(void *htbl, void *key, void *val) {
	struct _hash_table *t = htbl;
	struct _hash_link      *elem;
	int                 rc, hash;

	kassert( htbl != NULL );
	kassert( t->hash_func != NULL );
	kassert( t->compare_func != NULL );
	kassert( val != NULL );
		

	rc = hash_lookup(htbl, key, NULL);
	if ( rc == 0 )
		return -EEXIST;
		

	hash = t->hash_func(key, t->table_size);
	kassert( ( 0 <= hash ) && ( hash < t->table_size  ) );

	elem = hash_link_offset_from_value(t, val);
	elem->key = key;
	elem->tbl = t;

	SPLAY_INSERT(hash_table_entries, &t->table[hash], elem);
	++t->num_elems;

	return 0;
}

/**
   ハッシュテーブルを破棄する
   @param[in] htbl 破棄対象のハッシュテーブル
 */
void
hash_destroy(struct _hash_table *htbl){

	kassert( htbl->num_elems == 0 );

	if ( htbl->table != NULL ) {

		kfree(htbl->table);
		htbl->table = NULL;
	}

	kfree(htbl);
}

/**
   ハッシュテーブルを生成する
   @param[in] table_size  テーブルサイズ(ハッシュ配列のエントリ数)
   @param[in] link_offset ハッシュの要素内のlist構造へのオフセット
   @param[in] tablep      初期化したハッシュテーブルのアドレスを格納するポインタ変数のアドレス
   @param[in] compare_func キーの比較関数へのポインタ
   @param[in] hash_func    キーからハッシュ値（ハッシュテーブルのインデクス)を算出する関数へのポインタ
   @param[in] key_destroy_func キー開放関数へのポインタ
   @param[in] value_destroy_func 値開放関数へのポインタ
   @retval   0       成功
   @retval  -ENOMEM  メモリがない
*/
int
hash_create(int table_size, int64_t link_offset,
	    struct _hash_table **tablep,
	    int (*compare_func)(void *_key, void *_e),
	    int (*hash_func)(void *_key, int _size),
	    void (*key_destroy_func)(void *_key), 
	    void (*value_destroy_func)(void *_value)){
	struct _hash_table *t;
	int i;

	kassert( tablep != NULL );
	kassert( compare_func != NULL );
	kassert( hash_func != NULL );

	t = (struct _hash_table *)kmalloc( sizeof(struct _hash_table), KMALLOC_NORMAL);
	if ( t == NULL ) 
		return -ENOMEM;

	t->table = (struct hash_table_entries *)kmalloc( sizeof(struct hash_table_entries) * table_size, KMALLOC_NORMAL);
	if ( t->table == NULL )
		goto no_mem_out;

	for( i = 0; table_size > i ; ++i)
		SPLAY_INIT( &t->table[i] );

	t->table_size = table_size;
	t->link_offset = link_offset;
	t->num_elems = 0;
	t->compare_func = compare_func;
	t->hash_func = hash_func;
	t->key_destroy_func = key_destroy_func;
	t->value_destroy_func = value_destroy_func;

	*tablep = t;

	return 0;

no_mem_out:
	kfree(t);
	return -ENOMEM;
}


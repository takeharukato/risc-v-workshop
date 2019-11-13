/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Heap tree routines                                                */
/*                                                                    */
/*  The following codes are derived from the book "Data Structures    */
/*  and Algorithms Made Easy."                                        */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>

#include <page/page-if.h>

#include <klib/hptree.h>

/**
    ヒープ性を保持するためにノードを葉ノードの方向に移動する
    @param[in] h ヒープツリー
    @param[in] n 操作対象ノード
 */
static void 
percolate_down(heap_tree *h, heap_node *n) {
	heap_node *l,*r, *m, *cl, *cr;
	void *ld, *rd, *nd, *md;

	kassert( ( h->heap_type == HPTREE_TYPE_MIN ) ||
	    ( h->heap_type == HPTREE_TYPE_MAX ) );
	/*
	 * 操作対象ノードの左右の子を覚えておく
	 */
	l = n->left;
	r = n->right;

	m = n;	/* 最小/最大ノードを操作対象ノードと仮定する */

	ld = hptree_container(l, h->offset);
	rd = hptree_container(r, h->offset);
	nd = hptree_container(n, h->offset);
	md = hptree_container(m, h->offset);

	if ( h->heap_type == HPTREE_TYPE_MIN ) {
		
		/*  操作対象ノードの左右のノードを調べ, 最も値が小さいノードを算出  */
		if ( ( l != NULL ) && ( h->compare(ld, nd) <= 0 ) ) {

			m = l;
			md = hptree_container(m, h->offset);
		}

		if ( ( r != NULL ) && ( h->compare(rd, md) <= 0 ) ) {

			m = r;
			md = hptree_container(m, h->offset);
		}
	} else {

		/*  操作対象ノードの左右のノードを調べ, 最も値が大きいノードを算出  */
		if ( ( l != NULL ) && ( h->compare(ld, nd) >= 0 ) ) {

			m = l;
			md = hptree_container(m, h->offset);
		}

		if ( ( r != NULL ) && ( h->compare(rd, md) >= 0 ) ) {

			m = r;
			md = hptree_container(m, h->offset);
		}
	}

	/*
	 * 操作対象ノードが最小/最大ノードではない場合ノードを移動する
	 */
	if ( m != n ) {
		
		/*
		 * 最小/最大ノードの左右の子を記憶しておく
		 */
		cl = m->left;
		cr = m->right;

		/*  最小/最大ノードの親ノードを操作対象ノードの親に更新  */
		m->parent = n->parent;  
		if ( m == l ) {  /* 操作対象ノードの左の子ノードと入れ替える場合  */
		
			/*
			 * 最小/最大ノードの左の子ノードに操作対象ノードを接続 
			 * 最小/最大ノードの右の子ノードに操作対象ノードの右の子を接続
			 * 操作対象ノードの右の子の親を更新
			 */
			m->left = n;
			m->right = n->right;
			if ( n->right != NULL ) 
				n->right->parent = m;
		}

		if ( m == r ) {  /* 操作対象ノードの右の子ノードと入れ替える場合  */
		
			/*
			 * 最小/最大ノードの左の子ノードに操作対象ノードの左の子を接続
			 * 最小/最大ノードの右の子ノードに操作対象ノードを接続 
			 * 操作対象ノードの左の子の親を更新
			 */
			m->left = n->left;
			m->right = n;
			if ( n->left != NULL )
				n->left->parent = m;
		}

		/*
		 * 操作対象ノードの親ノードから最小/最大ノードへ接続する
		 */
		if ( (n->parent != NULL ) && ( n->parent->left == n ) ) 
			n->parent->left = m;  /* 親の左の子ノードの場合 */
		if ( (n->parent != NULL ) && ( n->parent->right == n ) )
			n->parent->right = m; /* 親の右の子ノードの場合 */

		/*
		 * 操作対象ノードの親, 左右の子ノードへのリンクを
		 * 最小/最大ノードを親とし, 最小/最大ノードの左右の子ノードへのリンクを
		 * 指すように更新
		 * また, 左右の子ノードから操作対象ノードへのリンク（親へのリンク)を更新
		 */
		n->parent = m;
		n->left = cl;
		n->right = cr;
		if ( cl != NULL )
			cl->parent = n;
		if ( cr != NULL )
			cr->parent = n;
		/*
		 * ヒープ内の最小/最大ノード(根ノード)を更新
		 */
		if ( m->parent == NULL )
			h->root = m;

		percolate_down(h, n);  /*  さらに葉の方のノードに対して移動する  */
	}

	if ( m->parent == NULL )
		h->root = m; /* 根ノードの更新 */

}

/**
   ヒープ性を保持するためにノードを上方向に移動する
   @param[in] h ヒープツリー
   @param[in] n 操作対象ノード
 */
static void 
percolate_up(heap_tree *h, heap_node *n) {
	heap_node *p, *pl, *pr, *pp;
	void *nd, *pd;

	for(p = n->parent; p != NULL ; p = n->parent){
		
		/*
		 * ヒープ性を満たしたら抜ける
		 */
		nd = hptree_container(n, h->offset);
		pd = hptree_container(p, h->offset);

		if ( ( h->heap_type == HPTREE_TYPE_MIN ) && ( h->compare(pd, nd) <= 0 ) )
			break;

		if ( ( h->heap_type == HPTREE_TYPE_MAX ) && ( h->compare(pd, nd) >= 0 ) )
			break;
		
		/*
		 * 親ノードの親ポインタ, 左の子, 右の子を記憶しておく
		 */
		pp = p->parent;
		pl = p->left;
		pr = p->right;
		
		/*
		 * 親ノードの親のリンクを再設定
		 */
		if ( pp != NULL ) {
			
			/* 親ノードの親の左,または, 右のノードを
			 * 入れ替え対象ノードに差し替える 
			 */
			if ( pp->left == p )  
				pp->left = n;
			else if ( pp->right == p )
				pp->right = n;
		}

		/*
		 * 親ノードのリンクを更新する
		 */
		p->left = n->left;   /* 入れ替え対象ノードの左の子を指すように更新 */
		p->right = n->right; /* 入れ替え対象ノードの右の子を指すように更新 */
		/* 入れ替え対象ノードの左の子ノードがある場合はその親ノードへのリンクを更新 */
		if ( n->left != NULL ) 
			n->left->parent = p;
		/* 入れ替え対象ノードの右の子ノードがある場合はその親ノードへのリンクを更新 */
		if ( n->right != NULL )
			n->right->parent = p;
		p->parent = n;  /*  親ノードの親を入れ替え対象ノードに更新 */

		/*
		 * 入れ替え対象ノードのリンクの更新
		 */
		if ( pl == n ) {  /* 親ノードの左の子ノードだった場合  */
			
			n->left = p;  /* 左の子ノードから親を指すように更新  */
			n->right = pr;  /* 親の右の子ノードを指すように右子ノードを更新  */
			if ( pr != NULL )
				pr->parent = n; /* 親の右の子ノードの親ノードリンクを更新  */
		} else {   /* 親ノードの右の子ノードだった場合  */
			
			kassert( pr == n );
			n->left = pl;  /* 親の左の子ノードを指すように左子ノードを更新  */
			n->right = p;  /* 右の子ノードから親ノードを指すように更新  */
			if ( pl != NULL )
				pl->parent = n; /* 親の左の子ノードの親ノードリンクを更新  */
		}
		n->parent = pp;  /*  親ノードへのリンクを親ノードの親に更新 */
		
	}
	
	if ( n->parent == NULL )
		h->root = n;  /* ルートノードを更新 */
}

/**
   サブツリー内の葉ノードを探索する
   @param[in] h ヒープツリー
   @param[in] n 探索開始ノード
   @param[out] np 葉ノードを指すポインタのアドレス
 */
static void
find_last_node_in_subtree(heap_tree *h, heap_node *n, heap_node **np) {
	heap_node *m, *p;
	void *pld, *prd, *pd;

	kassert( ( h->heap_type == HPTREE_TYPE_MIN ) ||
	    ( h->heap_type == HPTREE_TYPE_MAX ) );

	for(p = n; ( p->left != NULL ) || ( p->right != NULL ); p=m) {

		m = p;  /* 探索対象ノードを最小/最大ノードと仮定する */

		pld = hptree_container(p->left, h->offset);
		prd = hptree_container(p->right, h->offset);
		pd = hptree_container(p, h->offset);
	
		if ( h->heap_type == HPTREE_TYPE_MIN ) {
			
			/*  操作対象ノードの左右のノードを調べ, 最も値が大きいノードを算出  */
			if ( ( p->left != NULL ) && ( h->compare(pld, pd) >= 0 ) )
				m = p->left;

			if ( ( p->right != NULL ) && ( h->compare(prd, pd) >= 0 ) )
				m = p->right;
		} else {

			/*  操作対象ノードの左右のノードを調べ, 最も値が小さいノードを算出  */
			if ( ( p->left != NULL ) && ( h->compare(pld, pd) <= 0 ) )
				m = p->left;

			if ( ( p->right != NULL ) && ( h->compare(pld, pd) <= 0 ) )
				m = p->right;
		}
	}

	*np = p;  /*  葉ノードを返却  */

	return;
}

/**
   ヒープツリーの根ノードを参照する
   @param[in] h ヒープツリー
   @param[out] np 根ノードを指すポインタのアドレス
   @retval 0      正常終了
   @retval ENOENT ツリーが空
 */
static int
refer_heap_tree_root(heap_tree *h, heap_node **np){

	if ( h->root == NULL )
		return ENOENT;

	*np = h->root;

	return 0;
}

/**
   ヒープツリーからノードを削除する
   @param[in] h ヒープツリー
   @param[in] n 削除対象ノード
 */
static void 
delete_heap_node(heap_tree *h, heap_node *n) {
	heap_node *last, *lp;

	find_last_node_in_subtree(h, n, &last); /*  葉ノードを探す */
	kassert( ( last->left == NULL ) && ( last->right == NULL ) );

	/* 葉ノードのリンクを切り離す */ 
	lp = last->parent;
	if ( ( lp != NULL ) && ( lp->left == last ) )
		lp->left = NULL;
	if ( ( lp != NULL ) && ( lp->right == last ) )
		lp->right = NULL;

	/* 葉ノードを削除位置に配置する */ 
	last->parent = n->parent;
	last->left = n->left;
	last->right = n->right;
	if ( ( n->parent != NULL ) && ( n->parent->left == n )  )
		n->parent->left = last;
	else if ( ( n->parent != NULL ) && ( n->parent->right == n )  )
		n->parent->right = last;
	if ( n->left != NULL )
		n->left->parent = last;
	if ( n->right != NULL )
		n->right->parent = last;

	percolate_down(h, last);  /*  ヒープ性の再確保  */

	/* 削除対象ノードを切り離す */
	n->parent = NULL;
	n->left = NULL;
	n->right = NULL;

	--h->count;  /*  ヒープ内のノード数を減算  */
	if ( h->count == 0 )
		h->root = NULL;
}

/**
   ヒープツリーを生成する
   @param[in]  type ヒープツリーの種類(最小値探査か最大値探査か)
   @param[in]  offset ヒープノードを埋め込んでいるデータ構造内でのオフセット
   @param[in]  compare 値比較関数へのポインタ
   @param[in]  delete_notifier 削除通知関数へのポインタ
   @param[in]  show_handler 表示関数へのポインタ
   @param[out] hptp ヒープツリーを指すポインタのアドレス
   @retval 0      正常終了    
   @retval EINVAL ヒープ種別が不正
   @retval ENOMEM メモリ不足
 */
int
hptree_create(int type, uintptr_t offset, int (*compare)(void *_d1, void *_d2), 
    void (*delete_notifier)(void *d), 
    void (*show_handler)(heap_tree *_h, heap_node *_n, void *_d), 
    heap_tree **hptp) {
	heap_tree  *h;

	if ( ( type != HPTREE_TYPE_MIN ) && ( type != HPTREE_TYPE_MAX ) )
		return EINVAL;

	kassert( compare != NULL );

	/*
	 * ヒープツリー用のデータ構造を獲得
	 */
	h = (heap_tree *)kmalloc(sizeof(heap_tree), KMALLOC_NORMAL);
	if ( h == NULL ) {
		
		kprintf(KERN_CRI, "Can not allocate memory for a heap.");
		return ENOMEM;
	}
	memset(h, 0, sizeof(heap_tree));

	/*
	 * 初期値を設定
	 */
	h->heap_type = type;
	h->count = 0;
	h->root = NULL;
	h->compare = compare;
	h->delete_notifier = delete_notifier;
	h->show_handler = show_handler;
	h->offset = offset;
	*hptp = h;

	return 0;
}

/**
   ヒープツリーを破棄する
   @param[in] hp ヒープツリーへのポインタのアドレス
   @note ツリー内のノードの開放は行わない
 */
void
hptree_destroy_heap(heap_tree **hp) {
	heap_tree *h;

	h = *hp;
	while( h->count > 0 )   /*  ヒープが空になるまで ルートノードを削除  */
		hptree_delete_node(h, h->root);

	kfree(h);
	*hp = NULL;
}

/**
   ヒープツリーからノードを削除する
   @param[in] h ヒープツリー
   @param[in] n 削除対象ノード
 */
void 
hptree_delete_node(heap_tree *h, heap_node *n) {
	void *nd;

	nd = hptree_container(n, h->offset);
	delete_heap_node(h, n);
	if ( h->delete_notifier != NULL )
		h->delete_notifier(nd);
}

/**
   ヒープツリーにノードを追加する
   @param[in] h ヒープツリー
   @param[in] n 削除対象ノード
 */
void 
hptree_insert_node(heap_tree *h, heap_node *n){
	heap_node *tmp, *p;

	/*
	 * 葉ノードを探索
	 */
	for(tmp = h->root, p = tmp; tmp != NULL; ){

		p = tmp;

		if ( ( tmp->left == NULL ) || ( tmp->right == NULL ) )
			break;

		if ( tmp->left != NULL )
			tmp = tmp->left;
		else
			tmp = tmp->right;
	}

	if ( p != NULL ) {  /*  葉ノードの空いている方の子に指定されたノードを接続 */

		if ( p->left == NULL )
			p->left = n;
		else
			p->right = n;
	}

	/*
	 * 追加したノードの親ノードを更新し, 左右の子をNULLに設定
	 */
	n->parent = p;
	n->left = NULL;
	n->right = NULL;

	percolate_up(h, n);  /*  ヒープ性の確保  */
	++h->count;  /*  ヒープ内のノード数を加算  */
}

/**
   ヒープツリーの根ノードを取り出す
   @param[in] h ヒープツリー
   @param[out] np 根ノードを指すポインタのアドレス
   @retval 0      正常終了
   @retval ENOENT ツリーが空
 */
int
hptree_get_root(heap_tree *h, heap_node **np){
	int       rc;
	heap_node *n;

	rc = refer_heap_tree_root(h, &n);
	if ( rc == 0 ) {  

		/* ツリーが空でなければ, 
		 * 根ノードを切り離して
		 * 返却する
		 */
		delete_heap_node(h, n);
		*np = n;
	}

	return rc;
}

/**
   ヒープツリーを表示する
   @param[in] h ヒープツリー
   @param[in] n 探索対象ノード
 */
void
hptree_show(heap_tree *h, heap_node *n){
	void *nd;

	if ( n->left != NULL ) /* 左の子ノードがある場合, 左の子ノードを指定し再帰呼出し */
		hptree_show(h, n->left);
	
	if ( n->right != NULL ) /* 右の子ノードがある場合, 右の子ノードを指定し再帰呼出し */
		hptree_show(h, n->right);

	nd = hptree_container(n, h->offset);

	if ( h->show_handler != NULL )
		h->show_handler(h, n, nd);

	return;
}

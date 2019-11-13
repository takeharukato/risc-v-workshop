/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  kernel level bitmap operation routines                            */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>

/**
   64ビットの下位側マスクを返却する
   @param[in] _len マスク長 (単位: ビット)
   @return 64ビットの下位側マスク
 */
#define BITOPS_LOWER_MASK64(_len)		\
	calc_align_mask( ULONGLONG_C(1) << (_len) )

/**
   32ビットの下位側ビットマスクを返却する
   @param[in] _len マスク長 (単位: ビット)
   @return 32ビットの下位側ビットマスク
 */
#define BITOPS_LOWER_MASK32(_len)		\
	calc_align_mask( UINT_C(1) << (_len) )

/**
   16ビットの下位側ビットマスクを返却する
   @param[in] _len マスク長 (単位: ビット)
   @return 16ビットの下位側ビットマスク
   @note 16ビットもint型で扱えるためBITOPS_LOWER_MASK32の別名定義にしている
 */
#define BITOPS_LOWER_MASK16(_len)		\
	BITOPS_LOWER_MASK32((_len))

/**
   64ビットの上位側マスクを返却する
   @param[in] _len マスク長 (単位: ビット)
   @return 64ビットの上位側マスク
 */
#define BITOPS_UPPER_MASK64(_len)		\
	( ( BITOPS_LOWER_MASK64( (_len) ) ) << ( 64 - (_len) ) )

/**
   32ビットの上位側ビットマスクを返却する
   @param[in] _len マスク長 (単位: ビット)
   @return 32ビットの上位側ビットマスク
 */
#define BITOPS_UPPER_MASK32(_len)		\
	( ( BITOPS_LOWER_MASK32( (_len) ) ) << ( 32 - (_len) ) )

/**
   16ビットの上位側ビットマスクを返却する
   @param[in] _len マスク長 (単位: ビット)
   @return 16ビットの上位側ビットマスク
 */
#define BITOPS_UPPER_MASK16(_len)		\
	( ( BITOPS_LOWER_MASK16( (_len) ) ) << ( 16 - (_len) ) )

/*
 * NetBSDの bitops.h の実装を参考に作成
 */

/**
   16bit長のビットマップから最初にセットされている(1になっている)ビット位置を探す
   @param[in] n 調査対象の16bit値
   @return 1から数えた最初にセットされているビット位置、見つからなかった場合は, 0を返す 
 */
int
bitops_ffs16(uint16_t n){
	int v;
	int b;

	if ( n == 0 )
		return 0;  /* 見つからなかった場合は, 0を返す */

	v = 1;  /*  1ビット目にあることを仮定  */

	/*
	 * 分割統治法でビット位置を検索する
	 */
	for(b = 8; b > 0; b >>= 1) {

		/*
		 * 上位bビット側/下位bビット側の範囲を検索
		 */
		if ( (n & BITOPS_LOWER_MASK16(b) ) == 0 ) {

			n >>= b; /*  上位bビットから探す          */
			v += b;  /*  検出ビット位置をbビット加算  */
		}
	}

	return v;  /*  検出位置を返却  */
}

/**
   32bit長のビットマップから最初にセットされている(1になっている)ビット位置を探す
   @param[in] n 調査対象の32bit値
   @return 1から数えた最初にセットされているビット位置、見つからなかった場合は, 0を返す 
 */
int
bitops_ffs32(uint32_t n){
	int v;
	int b;

	if ( n == 0 )
		return 0;  /* 見つからなかった場合は, 0を返す */

	v = 1;  /*  1ビット目にあることを仮定  */

	/*
	 * 分割統治法でビット位置を検索する
	 */
	for(b = 16; b > 0; b >>= 1) {

		/*
		 * 上位bビット側/下位bビット側の範囲を検索
		 */
		if ( (n & BITOPS_LOWER_MASK32(b) ) == 0 ) {

			n >>= b; /*  上位bビットから探す          */
			v += b;  /*  検出ビット位置をbビット加算  */
		}
	}

	return v;  /*  検出位置を返却  */
}

/**
   64bit長のビットマップから最初にセットされている(1になっている)ビット位置を探す
   @param[in] n 調査対象の64bit値
   @return 1から数えた最初にセットされているビット位置、見つからなかった場合は, 0を返す 
 */
int
bitops_ffs64(uint64_t n){
	int v;
	int b;

	if ( n == 0 )
		return 0;    /* 見つからなかった場合は, 0を返す */

	v = 1;  /*  1ビット目にあることを想定  */

	/*
	 * 分割統治法でビット位置を検索する
	 */
	for(b = 32; b > 0; b >>= 1) {

		/*
		 * 上位bビット側/下位bビット側の範囲を検索
		 */
		if ( (n & BITOPS_LOWER_MASK64(b) ) == 0 ) {

			n >>= b; /*  上位bビットから探す          */
			v += b;  /*  検出ビット位置をbビット加算  */
		}
	}

	return v;  /*  検出位置を返却  */
}

/**
   16bit長のビットマップから最後にセットされている(1になっている)ビット位置を探す
   @param[in] n 調査対象の16bit値
   @return 1から数えた最後にセットされているビット位置、見つからなかった場合は, 0を返す 
 */
int
bitops_fls16(uint16_t n){
	int v;
	int b;

	if ( n == 0 )
		return 0;     /* 見つからなかった場合は, 0を返す */

	v = 16;  /*  最上位ビットにあることを仮定  */

	/*
	 * 分割統治法でビット位置を検索する
	 */
	for(b = 8; b > 0; b >>= 1) {

		/*
		 * 上位bビット側/下位bビット側の範囲を検索
		 */
		if ( (n & BITOPS_UPPER_MASK16(b) ) == 0 ) {

			n <<= b; /*  下位bビットから探す          */
			v -= b;  /*  検出ビット位置をbビット減算  */
		}
	}

	return v;  /*  検出位置を返却  */
}

/**
   32bit長のビットマップから最後にセットされている(1になっている)ビット位置を探す
   @param[in] n 調査対象の32bit値
   @return 1から数えた最後にセットされているビット位置、見つからなかった場合は, 0を返す 
 */
int
bitops_fls32(uint32_t n){
	int v;
	int b;

	if ( n == 0 )
		return 0;     /* 見つからなかった場合は, 0を返す */

	v = 32;  /*  最上位ビットにあることを仮定  */

	/*
	 * 分割統治法でビット位置を検索する
	 */
	for(b = 16; b > 0; b >>= 1) {

		/*
		 * 上位bビット側/下位bビット側の範囲を検索
		 */
		if ( (n & BITOPS_UPPER_MASK32(b) ) == 0 ) {

			n <<= b; /*  下位bビットから探す          */
			v -= b;  /*  検出ビット位置をbビット減算  */
		}
	}

	return v;  /*  検出位置を返却  */
}

/**
   64bit長のビットマップから最後にセットされている(1になっている)ビット位置を探す
   @param[in] n 調査対象の64bit値
   @return 1から数えた最後にセットされているビット位置、見つからなかった場合は, 0を返す 
 */
int
bitops_fls64(uint64_t n){
	int v;
	int b;

	if ( n == 0)
		return 0;

	v = 64;  /*  最上位ビットにあることを仮定  */

	/*
	 * 分割統治法でビット位置を検索する
	 */
	for(b = 32; b > 0; b >>= 1) {

		/*
		 * 上位bビット側/下位bビット側の範囲を検索
		 */
		if ( (n & BITOPS_UPPER_MASK64(b) ) == 0 ) {

			n <<= b; /*  下位bビットから探す          */
			v -= b;  /*  検出ビット位置をbビット減算  */
		}
	}

	return v;
}

/**
   2つのビットマップが等しいことを確認する
   @param[in] v1 ビットマップ配列の先頭アドレス1
   @param[in] v2 ビットマップ配列の先頭アドレス2
   @param[in] v1_size ビットマップ配列1のサイズ
   @param[in] v2_size ビットマップ配列2のサイズ
   @param[in] elm_size  ビットマップ配列の要素サイズ
   @retval 真 2つのビットマップが等しい
   @retval 偽 2つのビットマップが異なる
 */
bool
__bitops_equal(void *v1, void *v2, size_t v1_size, size_t v2_size, size_t elm_size){
	size_t i;
					
	if ( v1_size != v2_size )
		return false;  /*  配列長が異なる */

	for (i = 0; (v1_size / elm_size ) > i; ++i) {  

		/* 
		 * ビットマップ配列の各要素を比較する
		 */
		if ( elm_size == sizeof(uint64_t) ) {   /*  64bitビットマップ配列の場合  */

			if ( ((uint64_t *)v1)[i] != ((uint64_t *)v2)[i] )
				return false;  /*  異なる要素を検出した  */
		} else if ( elm_size == sizeof(uint32_t) ) { /* 32bitビットマップ配列の場合 */

			if ( ((uint32_t *)v1)[i] != ((uint32_t *)v2)[i] )
				return false;  /*  異なる要素を検出した  */
		} else if ( elm_size == sizeof(uint16_t) ) { /* 16bitビットマップ配列の場合 */

			if ( ((uint16_t *)v1)[i] != ((uint16_t *)v2)[i] )
				return false;  /*  異なる要素を検出した  */
		} else
			kassert_no_reach();  /*  不正なビットマップ要素サイズ  */
	}

	return true;  /*  2つのビットマップが等しい  */
}

/**
   ビットマップ中で最初にセットされているビット位置を返す
   @param[in] v ビットマップ配列の先頭アドレス
   @param[in] v_size ビットマップ配列のサイズ
   @param[in] elm_size  ビットマップ配列の要素サイズ
   @return - 1から数えて最初にセットされているビットの位置
           - 全ビットが0の場合は, 0を返す
 */
uint64_t
__bitops_ffs(void *v, size_t v_size, size_t elm_size){
	unsigned int i;
	int        off;
	uint64_t   pos;
	
	pos = 0;  /*  見つからなかった場合を仮定する */
	for (i = 0; (v_size / elm_size ) > i; ++i) {  

		if ( elm_size == sizeof(uint64_t) )  /*  64bitビットマップ配列の場合  */
			off = bitops_ffs64(((uint64_t *)v)[i]);
		else if ( elm_size == sizeof(uint32_t) ) /*  32bitビットマップ配列の場合  */
			off = bitops_ffs32(((uint32_t *)v)[i]);
		else if ( elm_size == sizeof(uint16_t) )  /*  16bitビットマップ配列の場合  */
			off = bitops_ffs16(((uint16_t *)v)[i]);
		else
			kassert_no_reach();  /*  不正なビットマップ要素サイズ  */

		if ( off != 0 ) {  /*  セットビット検出  */

			pos = BITS_PER_BYTE * elm_size * i + off;  /* ビット位置を算出 */
			break;
		}
	}

	return pos; /* ビット位置を返却 */
}

/**
   ビットマップ中で最後にセットされているビット位置を返す
   @param[in] v ビットマップ配列の先頭アドレス
   @param[in] v_size ビットマップ配列のサイズ
   @param[in] elm_size  ビットマップ配列の要素サイズ
   @return - 1から数えて最後にセットされているビットの位置
           - 全ビットが0の場合は, 0を返す
 */
uint64_t
__bitops_fls(void *v, size_t v_size, size_t elm_size){
	unsigned int i;
	int        idx;
	int        off;
	uint64_t   pos;
	
	pos = 0;  /*  見つからなかった場合を仮定する */
	for (i = 0; (v_size / elm_size ) > i; ++i) {  

		idx = (v_size / elm_size ) - i - 1;  /*  配列の末尾から検索  */
		if ( elm_size == sizeof(uint64_t) ) /*  64bitビットマップ配列の場合  */
			off = bitops_fls64(((uint64_t *)v)[idx]);
		else if ( elm_size == sizeof(uint32_t) ) /*  32bitビットマップ配列の場合  */
			off = bitops_fls32(((uint32_t *)v)[idx]);
		else if ( elm_size == sizeof(uint16_t) ) /*  16bitビットマップ配列の場合  */
			off = bitops_fls16(((uint16_t *)v)[idx]);
		else
			kassert_no_reach();  /*  不正なビットマップ要素サイズ  */

		if ( off != 0 ) {  /*  セットビット検出  */

			pos = BITS_PER_BYTE * elm_size * idx + off;  /* ビット位置を算出 */
			break;
		}
	}

	return pos; /* ビット位置を返却 */
}

/**
   ビットマップ中で最初にクリアされているビット位置を返す
   @param[in] v ビットマップ配列の先頭アドレス
   @param[in] v_size ビットマップ配列のサイズ
   @param[in] elm_size  ビットマップ配列の要素サイズ
   @return - 1から数えて最初にクリアされているビットの位置
           - 全ビットが1の場合は, 0を返す
 */
uint64_t
__bitops_ffc(void *v, size_t v_size, size_t elm_size){
	unsigned int i;
	int        off;
	uint64_t   pos;
	
	pos = 0;  /*  見つからなかった場合を仮定する */
	for (i = 0; (v_size / elm_size ) > i; ++i) {  

		if ( elm_size == sizeof(uint64_t) )  /*  64bitビットマップ配列の場合  */
			off = bitops_ffs64( ~(((uint64_t *)v)[i]) );
		else if ( elm_size == sizeof(uint32_t) ) /*  32bitビットマップ配列の場合  */
			off = bitops_ffs32( ~(((uint32_t *)v)[i]) );
		else if ( elm_size == sizeof(uint16_t) )  /*  16bitビットマップ配列の場合  */
			off = bitops_ffs16( ~(((uint16_t *)v)[i]) );
		else
			kassert_no_reach();  /*  不正なビットマップ要素サイズ  */

		if ( off != 0 ) {  /*  セットビット検出  */

			pos = BITS_PER_BYTE * elm_size * i + off;  /* ビット位置を算出 */
			break;
		}
	}

	return pos; /* ビット位置を返却 */
}

/**
   ビットマップ中で最後にクリアされているビット位置を返す
   @param[in] v ビットマップ配列の先頭アドレス
   @param[in] v_size ビットマップ配列のサイズ
   @param[in] elm_size  ビットマップ配列の要素サイズ
   @return - 1から数えて最後にクリアされているビットの位置
           - 全ビットが1の場合は, 0を返す
 */
uint64_t
__bitops_flc(void *v, size_t v_size, size_t elm_size){
	unsigned int i;
	int        idx;
	int        off;
	uint64_t   pos;
	
	pos = 0;  /*  見つからなかった場合を仮定する */
	for (i = 0; (v_size / elm_size ) > i; ++i) {  

		idx = (v_size / elm_size ) - i - 1;  /*  配列の末尾から検索  */
		if ( elm_size == sizeof(uint64_t) ) /*  64bitビットマップ配列の場合  */
			off = bitops_fls64( ~(((uint64_t *)v)[idx]) );
		else if ( elm_size == sizeof(uint32_t) ) /*  32bitビットマップ配列の場合  */
			off = bitops_fls32( ~(((uint32_t *)v)[idx]) );
		else if ( elm_size == sizeof(uint16_t) ) /*  16bitビットマップ配列の場合  */
			off = bitops_fls16( ~(((uint16_t *)v)[idx]) );
		else
			kassert_no_reach();  /*  不正なビットマップ要素サイズ  */

		if ( off != 0 ) {  /*  セットビット検出  */

			pos = BITS_PER_BYTE * elm_size * idx + off;  /* ビット位置を算出 */
			break;
		}
	}

	return pos; /* ビット位置を返却 */
}

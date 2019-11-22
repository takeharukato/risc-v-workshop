/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  memset routine                                                    */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <klib/string.h>

/**
   sで示されるメモリ領域の先頭からnバイトをcで埋める。
    @param[in] s 書き込み先メモリの先頭アドレス
    @param[in] c 書き込む値(charとして扱われる)
    @param[in] n 書き込むバイト数
    @note 8バイトまたは4バイト境界から始まるアドレスについては, 可能な限り
    8バイトまたは4バイト単位で埋め, 余りバイトはバイト単位で埋める。
    OS内の典型的なデータ構造は, 8バイトまたは4バイト境界にそろっているので
    典型的な場合に効果があり極力単純化した実装を採用している。
 */
void *
memset(void *s, int c, size_t n){
	uint64_t   val;
	uint8_t   *cp8;
	uint32_t *cp32;
	uint64_t *cp64;
	uint32_t   c32;
	uint64_t   c64;
	size_t     len;

	len = n;  /* コピー長を記憶する */

	/*  32bit, 64bit長のバイトcで埋められた領域を作成する  */
	val = c & 0xff;  /* バイトcを符号なし64bit整数に変換  */
	/*  32bitの領域をバイトcで埋める  */
	c32 = (uint32_t)((val << 24) | (val << 16) | (val << 8) | val);
	/*  64bitの領域をバイトcで埋める  */
	c64 = ( ( (uint64_t)c32 ) << 32 ) | (uint64_t)c32;

	cp8 = (uint8_t *)s;  /* 残りバイトの先頭を指す */

	if ( ( ( (uintptr_t)cp8 & ( sizeof(uint64_t) - 1) ) == 0 ) 
	    && ( len >= sizeof(uint64_t) ) ) {
		
		/* 8バイト境界で始まり, 残転送量が8バイト以上ある場合は, 8バイト単位で
		 * 領域を埋める
		 */
retry64:
		for( cp64 = (uint64_t *)cp8; len >= sizeof(uint64_t); len -= sizeof(uint64_t)) 
			*cp64++ = c64;

		cp8 = (uint8_t *)cp64;  /* 残りバイトの先頭を指す */
	}


	if ( ( ( (uintptr_t)cp8 & ( sizeof(uint32_t) - 1) ) == 0 ) 
	     && ( len >= sizeof(uint32_t) ) ) {

		/* 4バイト境界で始まり, 残転送量が4バイト以上ある場合は, 4バイト単位で
		 * 領域を埋める
		 */
retry32:
		for( cp32 = (uint32_t *)cp8; len >= sizeof(uint32_t); len -= sizeof(uint32_t)) {

			*cp32++ = c32;		
			cp8 = (uint8_t *)cp32; /* 残りバイトの先頭を指す */
			if ( ( ( (uintptr_t)cp8 & ( sizeof(uint64_t) - 1) ) == 0 ) 
			     && ( len >= sizeof(uint64_t) ) )
				goto retry64; /* 8バイト境界に到達したら8バイト単位フィルする */
		}
	}

	/* バイト単位でフィルする */
	while ( len-- > 0 ) {

		*cp8++ = (uint8_t)(c & 0xff);
	
		if ( ( ( (uintptr_t)cp8 & ( sizeof(uint64_t) - 1) ) == 0 ) 
		     && ( len >= sizeof(uint64_t) ) )
			goto retry64;  /* 8バイト境界に到達したら, 8バイト単位でフィルをする */

		if ( ( ( (uintptr_t)cp8 & ( sizeof(uint32_t) - 1) ) == 0 ) 
		     && ( len >= sizeof(uint32_t) ) )
			goto retry32;  /* 4バイト境界に到達したら, 4バイト単位でフィルする */
	}
	return s;
}

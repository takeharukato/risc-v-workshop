/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Simple minix file system image generator                          */
/*                                                                    */
/**********************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <klib/freestanding.h>
#include <klib/klib-consts.h>
#include <klib/misc.h>
#include <klib/align.h>
#include <kern/kern-types.h>
#include <kern/page-macros.h>
#include <fs/minixfs/minixfs.h>
#include <kern/fs-fsimg.h>

#include <string.h>

#include <utils.h>

/** デフォルトではバージョン3のファイルシステムを生成 */
#define MKFS_MINIXFS_DEFAULT_VERSION (3)
/** デフォルトでは8MiBのファイルシステムを生成 */
#define MKFS_MINIXFS_DEFAULT_SIZE_MB (8)

#define MKFS_MINIXFS_FSIMG_NAME_FMT "minixfs-v%d-fsimg.img"

#define MKFS_MINIXFS_ROOT_INO        (1)

/**
   ヘルプを表示する
   @param[in] cmd コマンド名
 */
void
show_help(char *cmd){

	printf("%s [-h] [-v version] [-s size] fsimg-file\n", cmd);
	exit(0);
}

int
main(int argc, char *argv[]){
	int              rc;
	int             opt;
	int       nr_inodes;
	int           param;
	char     *imagefile;
	char     buff[1024];
	int         version;
	int64_t     imgsize;
	
	version = MKFS_MINIXFS_DEFAULT_VERSION;               /* バージョン     */
	imgsize = MIB_TO_BYTE(MKFS_MINIXFS_DEFAULT_SIZE_MB);  /* ファイルサイズ */
	nr_inodes = PAGE_SIZE*BITS_PER_BYTE;

	opt = getopt(argc, argv, "hi:s:v:");
	while ( opt != -1 ) {

		switch (opt) {
		case 'h':
			show_help(argv[0]);
			break;
		case 'i':
			param = str2int(optarg, &rc);
			if ( rc != 0 )
				show_help(argv[0]);
			if ( param > MKFS_MINIXFS_ROOT_INO )
				nr_inodes = param;
			break;
		case 's':
			imgsize = str2int64(optarg, &rc);
			if ( rc != 0 )
				show_help(argv[0]);
			imgsize = MIB_TO_BYTE(imgsize);
			break;
		case 'v':
			version = str2int(optarg, &rc);
			if ( rc != 0 )
				show_help(argv[0]);
			break;
		default:
			show_help(argv[0]);
			break;
		}
	}
	if ( ( 1 > version) || ( version > 3 ) ) {

		fprintf(stderr, "Invalid version: %d\n", version);
		exit(1);
	}

	if ( argc > optind ) 
		imagefile=strdup(argv[optind]);
	else {

		snprintf(buff, 1024, MKFS_MINIXFS_FSIMG_NAME_FMT, version);
		imagefile=strdup(buff);
	}
	if ( imagefile == NULL ) {

		fprintf(stderr, "No memory\n");
		exit(1);
	}

	if ( version == 3 ) {
		
		nr_inodes = roundup_align(nr_inodes, PAGE_SIZE*BITS_PER_BYTE);
	} else {

		nr_inodes = roundup_align(nr_inodes, MINIX_OLD_BLOCK_SIZE*BITS_PER_BYTE);
	}

	printf("Image file: %s\n", imagefile);
	printf("nr_inodes : %d\n", nr_inodes);
	printf("version   : %d\n", version);
	printf("size=%lld\n", (long long)imgsize);

	return 0;
}



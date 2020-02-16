/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Copy a file into a Minix file system image                        */
/*                                                                    */
/**********************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include <klib/freestanding.h>
#include <klib/klib-consts.h>
#include <klib/misc.h>
#include <klib/align.h>
#include <kern/kern-types.h>
#include <kern/page-macros.h>
#include <fs/minixfs/minixfs.h>

#include <string.h>

#include <minixfs-tools.h>
#include <utils.h>

/**
   ヘルプを表示する
   @param[in] cmd コマンド名
 */
void
show_help(char *cmd){

	printf("%s [-h] host-file-name fsimg-file\n", cmd);
	printf("\t-h show this help.\n");
	printf("\t Minix file system version.\n");
	printf("\thost-file-name the filepath of the file on the host machine.\n");
	printf("\tfsimg-file the filepath of the file system image file.\n");
	exit(0);
}
int
copy_regular_file_from_host_to_image(fs_image *handle, char *hostpath, char *imagepath){
	int                    fd;
	int                   rc;
	ssize_t            rdlen;
	ssize_t            wrlen;
	off_t                off;
	struct stat fstat_result;
	uint8_t      buf[BUFSIZ];
	char                *dir;
	char               *name;
	char            *tmppath;
	minix_inode     dirinode;
	minix_inode     newinode;
	minix_ino       new_inum;
	minix_ino       del_inum;

	tmppath = strdup(hostpath);
	if ( tmppath == NULL ) {

		rc = -ENOMEM;
		goto error_out;
	}
	rc = path_get_basename(tmppath, &dir, &name);
	if ( rc != 0 )
		goto free_tmppath_out;

	rc = create_regular_file(handle, name, MKFS_MINIXFS_REGFILE_MODE, &new_inum);
	if ( rc != 0 )
		goto free_tmppath_out;

	fd = safe_open(hostpath, 0);
	if ( 0 > fd )
		goto free_newfile_out;

	rc = fstat(fd, &fstat_result); 	  /*  ファイル記述子が示す対象の属性を得る。 */
	if ( 0 > rc ) {

		rc = -errno;
		goto close_fd_out;
	}
	minix_rw_disk_inode(&handle->msb, new_inum, MINIX_RW_READING, &newinode);
	rdlen = read(fd, &buf[0], BUFSIZ);
	for(off = 0; rdlen > 0; ) {
		
		rc = minix_rw_zone(&handle->msb, new_inum, &newinode, &buf[0], off, 
		    rdlen, MINIX_RW_WRITING, &wrlen);
		off += wrlen;
		rdlen = read(fd, &buf[0], BUFSIZ);
	}
	minix_rw_disk_inode(&handle->msb, new_inum, MINIX_RW_WRITING, &newinode);

	close(fd);

	free(tmppath);

	return 0;

close_fd_out:
	close(fd);

free_newfile_out:
	rc = minix_del_dentry(&handle->msb, MKFS_MINIXFS_ROOT_INO, 
				      &dirinode, name, &del_inum);
	minix_bitmap_free(&handle->msb, MKFS_MINIXFS_ROOT_INO, del_inum);
	
free_tmppath_out:
	free(tmppath);

error_out:
	return -errno;
}
int
main(int argc, char *argv[]){
	int              rc;
	int             opt;
	char     *imagefile;
	char      *hostfile;
	char       *imgpath;
	fs_image    *handle;


	opt = getopt(argc, argv, "hp:");
	while ( opt != -1 ) {

		switch (opt) {
		case 'h':
			show_help(argv[0]);
			break;
		case 'p':
			imgpath = strdup(optarg);
			if ( imgpath == NULL ) {

				fprintf(stderr, "No memory\n");
				exit(1);
			}
			break;
		default:
			show_help(argv[0]);
			break;
		}
		opt = getopt(argc, argv, "hp:");
	}

	if ( (optind + 1) >= argc ) {

		show_help(argv[0]);
		exit(1);
	}

	hostfile=strdup(argv[optind++]);
	imagefile=strdup(argv[optind++]);

	if ( ( hostfile == NULL ) || ( imagefile == NULL ) ) {

		fprintf(stderr, "No memory\n");
		exit(1);
	}

	printf("host file: %s\n", hostfile);
	printf("Image file: %s\n", imagefile);

	rc = fsimg_pagecache_init(imagefile, &handle);
	if ( rc != 0 ) 
		exit(1);
	rc = copy_regular_file_from_host_to_image(handle, hostfile, imagefile);
	if ( rc != 0 ) 
		exit(1);

	return 0;
}

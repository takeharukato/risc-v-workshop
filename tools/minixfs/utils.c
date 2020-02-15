/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Utility functions                                                 */
/*                                                                    */
/**********************************************************************/
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>

#include <utils.h>


static int
safe_open_common(int link_is_ok, const char *pathname, int flags){
	int rc;
	int fd;
	struct stat lstat_result, fstat_result;

	rc = lstat(pathname, &lstat_result);  /* パス名が示す対象の属性を得る。 */
	if (rc < 0) {

		rc = -errno;
		goto error_out;
	}
 
	if ( (!S_ISREG(lstat_result.st_mode)) &&
	     ( !(link_is_ok) || !S_ISLNK(lstat_result.st_mode) ) ) {

		/* 通常ファイルでなければエラーにする。 */
		rc = -errno;
		goto error_out;
	}
 
	fd = open(pathname, O_RDWR);

	if ( fd < 0 ) {

		rc = -errno;
		goto error_out;
	}

	rc = fstat(fd, &fstat_result); 	  /*  ファイル記述子が示す対象の属性を得る。 */
	if ( rc < 0 ) {

		rc = -errno;
		goto close_fd_out;
	}

	if (lstat_result.st_ino != fstat_result.st_ino ||
	    lstat_result.st_dev != fstat_result.st_dev) {

		/* lstat()結果とfstat()結果が指す対象は同一か調べる。
		 * iノード番号とデバイス番号を照合する。
		 * 一致しなければ、fdをクローズしてエラー終了。
		 */
		goto close_fd_out;
	}

	return fd;

close_fd_out:
	    close(fd);	    
error_out:
	    return rc;
}

int
safe_open(const char *pathname, int flags){

	return safe_open_common(1, pathname, flags);
}

int
safe_open_nolink(const char *pathname, int flags){

	return safe_open_common(0, pathname, flags);
}

static long long
str2int_common(const char *str, long long lim, int base, int *rcp) {
	char *endptr;
	long long val;
	int old_error;
	
	old_error = errno;
	errno = 0; 
	val = strtoll(str, &endptr, base);

	if ((errno == ERANGE && (val == LONG_MAX || val == LONG_MIN))
	    || (errno != 0 && val == 0) ) {

		goto error_out;
           }

	if (val > lim) {

		errno = ERANGE;
		goto error_out;
	}

	errno = old_error;

	
	return (long long)val;

error_out:
	if (rcp != NULL)
		*rcp = 0;
	return -1;
}

int
str2int(const char *str, int *rcp) {

	return (int)str2int_common(str, INT_MAX, 10, rcp);
}

unsigned int
str2uint(const char *str, int *rcp) {

	return (int)str2int_common(str, INT_MAX, 10, rcp);
}

int32_t 
str2int32(const char *str, int *rcp) {

	if (sizeof(long) == 4)
		return str2int_common(str, LONG_MAX, 10, rcp);

	if (sizeof(short) == 4)
		return str2int_common(str, SHRT_MAX, 10, rcp);

	return str2int_common(str, LLONG_MAX, 10, rcp);
}

uint32_t
str2uint32(const char *str, int *rcp) {

	if (sizeof(long) == 4)
		return str2int_common(str, ULONG_MAX, 10, rcp);

	if (sizeof(short) == 4)
		return str2int_common(str, USHRT_MAX, 10, rcp);

	return str2int_common(str, ULLONG_MAX, 10, rcp);
}

int64_t
str2int64(const char *str, int *rcp) {

	if (sizeof(long) == 8)
		return str2int_common(str, LONG_MAX, 10, rcp);


	return str2int_common(str, LLONG_MAX, 10, rcp);
}

uint64_t
str2uint64(const char *str, int *rcp) {

	if (sizeof(long) == 8)
		return str2int_common(str, ULONG_MAX, 10, rcp);

	return str2int_common(str, ULLONG_MAX, 10, rcp);
}


/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  String operation definitions                                      */
/*                                                                    */
/**********************************************************************/
#if !defined(_KLIB_STRING_H)
#define _KLIB_STRING_H

#include <klib/freestanding.h>

/*
 * コピー関数 memcpy, memmove, strcpy, strncpy
 */
void *memcpy(void *_dest, const void *_src, size_t _count);
void *memmove(void *_dest, const void *_src, size_t _count);
char *strcpy(char *_dest, char const *_src);
char *strncpy(char *_dest, char const *_src, size_t _count);
char *strtok_r(char *_str, const char *_delim, char **_saveptr);
/*
 * 連結関数 strcat, strncat
 */
char *strcat(char *_dest, char const *_src);
char *strncat(char *_dest, char const *_src, size_t _n);

/*
 * 比較関数 memcmp, strcmp, strncmp, strcasecmp, strncasecmp
 */
int memcmp(const void *m1, const void *_m2, size_t _count);
int strcmp(char const *_s1, char const *_s2);
int strncmp(const char *_s1, const char *_s2, size_t _n);
int strcasecmp(char const *_s1, char const *_s2);
int strncasecmp(const char *_s1, const char *_s2, size_t _n);

/*
 * 探索関数 memchr, strchr,strrchr,strstr,strspn,strcspn
 */
void *memchr(const void *_s, int _c, size_t _count);
char *strchr(const char *_s, int _c);
char *strrchr(const char *_s, int _c);
char *strstr(const char *_haystack, const char *_needle);
size_t strspn(const char *_s, const char *_accept);
size_t strcspn(const char *_s, const char *_reject);

/*
 * その他   memset, strlen, strnlen
 */
void *memset(void *_s, int _c, size_t _n);
size_t strlen(char const *_s);
size_t strnlen(char const *_s, size_t _count);

/*
 * 複製関数 strdup
 */
char *kstrdup(const char *_src);

#endif  /*  _KLIB_STRING_H  */

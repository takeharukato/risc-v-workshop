# -*- mode: makefile-gmake; coding:utf-8 -*-
#
-include ${top}/.config
TARGET_CPUS=riscv64 uland
HAL_DIRS=${TARGET_CPUS}
HAL_INC_DIRS=$(addprefix hal-,${TARGET_CPUS})

CPU ?= `uname -m|sed -e "s/x86-64/x64/" -e "s/amd64/x64/" -e "s/x86_64/x64/" -e "s/i?86/ia32/"`
HOST_CC ?= gcc
CC=${CROSS_COMPILE}${HOST_CC}
AR=${CROSS_COMPILE}ar
LD=${CROSS_COMPILE}ld
RANLIB=${CROSS_COMPILE}ranlib
STRIP=${CROSS_COMPILE}strip
OBJCOPY=${CROSS_COMPILE}objcopy
OBJDUMP=${CROSS_COMPILE}objdump
NM=${CROSS_COMPILE}nm
SED=sed
SORT=sort
GTAGS=gtags
CP=cp
RM=rm -f
LN=ln
ARFLAGS=rvc
INC_FLAGS = -I${top}/include
WARN_FLAGS=-Wall -Werror
OPT_FLAGS=-O0 -ggdb3
ifeq ($(CONFIG_PROFILE),y)
PROF_OPT_FLAGS=-fprofile-arcs -ftest-coverage -coverage 
else
PROF_OPT_FLAGS=
endif
# Disable PIE when possible (for Ubuntu 16.10)
ifneq ($(shell $(CC) -dumpspecs 2>/dev/null | grep -e '[^f]no-pie'),)
PIC_OPT_FLAGS=-fno-pie -no-pie -fno-PIC
endif
ifneq ($(shell $(CC) -dumpspecs 2>/dev/null | grep -e '[^f]nopie'),)
PIC_OPT_FLAGS=-fno-pie -nopie -fno-PIC
endif
CFLAGS=${OPT_FLAGS} ${WARN_FLAGS} ${PIC_OPT_FLAGS} ${PROF_OPT_FLAGS} $(shell echo ${CONFIG_HAL_CFLAGS}) $(shell echo ${CONFIG_HAL_OPT_FLAGS}) ${INC_FLAGS}
# CFLAGS for asm-offset.c
ASM_OFFSET_CFLAGS = $(shell echo ${CFLAGS}|sed -e 's@-ggdb[0-9]*@@g')

LDFLAGS=

GIT=git
ARCHIVE_NAME=risc-v-workshop
FSIMG_FILE=${top}/fsimg.img

QEMU ?= qemu-system-x86_64
QEMU_KERNEL ?= ${top}/kernel.elf
QEMU_OPT ?= -m ${CONFIG_HAL_MEMORY_SIZE_MB} -nographic -serial mon:stdio
QEMU_DBG_OPT := -S -gdb tcp::1234

OBJ_CLEAN_FILES=*.o *.d
GTAGS_CLEAN_FILES=GPATH GRTAGS GSYMS GTAGS
TEX_CLEAN_FILES=*.aux *.bbl *.blg *.dvi *.log *.toc \
	*.ptb *.tod *.fls *.fdb-latexmk *.lof
GCOV_CLEAN_FILES=*.gcda *.gcno *.gcov

CLEAN_FILES=${OBJ_CLEAN_FILES} ${GCOV_CLEAN_FILES}  ${TEX_CLEAN_FILES} ${GTAGS_CLEAN_FILES}
DIST_CLEAN_FILES=\\\#* *~ .*.sw* .config* ..config.* *.log *.tar.gz *.tgz

.PHONY: clean distclean hal tools gtags dist

.SUFFIXES: .o .c .S .d

.c.o:
	${CC} ${CFLAGS} -c -o $@ $<

.S.o:
	${CC} ${CFLAGS} -c -o $@ $<

.c.d: 
	${CC} -E ${CFLAGS} -MM -MF"$(patsubst %.c,%.d, $<)" $< 

.S.d: 
	${CC} -E ${CFLAGS} -MM -MF"$(patsubst %.S,%.d, $<)" $< 




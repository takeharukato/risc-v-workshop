# -*- mode: makefile-gmake; coding:utf-8 -*-
#
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
OPT_FLAGS=-O0 -g -Wall
PIC_OPT_FLAGS=-fno-pie -fno-pic -fno-PIC
CFLAGS=${OPT_FLAGS} ${PIC_OPT_FLAGS} $(shell echo ${CONFIG_HAL_CFLAGS}) $(shell echo ${CONFIG_HAL_OPT_FLAGS}) ${INC_FLAGS}
# Disable PIE when possible (for Ubuntu 16.10)
ifneq ($(shell $(CC) -dumpspecs 2>/dev/null | grep -e '[^f]no-pie'),)
CFLAGS += -fno-pie -no-pie
endif
ifneq ($(shell $(CC) -dumpspecs 2>/dev/null | grep -e '[^f]nopie'),)
CFLAGS += -fno-pie -nopie
endif
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

.PHONY: clean distclean hal tools gtags dist

.SUFFIXES: .o .c .S

.c.o:
	${CC} ${CFLAGS} -c -o $@ $<

.S.o:
	${CC} ${CFLAGS} -c -o $@ $<

-include ${top}/.config


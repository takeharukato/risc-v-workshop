# -*- mode: makefile-gmake; coding:utf-8 -*-
#
#  Yet Another Teachable Operating System
#  Copyright 2016 Takeharu KATO
#
top=..
include ${top}/Makefile.inc
local-dirs=scripts styles templates

all: gen-doxygen

#
#Doxygen文書とエラーログをトップディレクトリに生成
#実際の処理は, ${top}/docs/doxygen/scripts/gen-doxygen.shで実施
#
gen-doxygen: clean-doxygen
	@${top}/doxygen/scripts/gen-doxygen.sh \
		-s ${top} -n ${DOXYGEN_ARCHIVE_NAME}
	@if [ -f ${DOXYGEN_ARCHIVE_NAME}-doxygen-doc.tar.gz ]; then 	\
		mv ${DOXYGEN_ARCHIVE_NAME}-doxygen-doc.tar.gz ${top}; \
	fi;
	@if [ -f ${DOXYGEN_ARCHIVE_NAME}-doxygen-error.log ]; then 	\
		mv  ${DOXYGEN_ARCHIVE_NAME}-doxygen-error.log ${top}; 	\
	fi;

clean: clean-doxygen
	for dir in ${local-dirs} ; do \
	${MAKE} -C $${dir} $@;\
	done
	${RM} ${CLEAN_FILES}

distclean: clean
	for dir in ${local-dirs} ; do \
	${MAKE} -C $${dir} $@;\
	done
	${RM}  ${DIST_CLEAN_FILES}

#
#Doxygen関連ファイルの削除
#
clean-doxygen:
	${RM} *-doxygen-doc*.tar.gz *-doxygen-doc-*.log \
		*-doxygen-error.log *.aux *.bbl *.blg *.dvi *.log *.toc \
		*.ptb *.tod *.fls *.fdb-latexmk *.lof


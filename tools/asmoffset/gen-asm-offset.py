#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# This file is a part of the sample kernel for RISC-V
# Copyright 2019 Takeharu KATO.
#

import sys, codecs, os, re, datetime
import ConfigParser
import csv
import shutil
import subprocess
import datetime
import tempfile
from optparse import OptionParser
from StringIO import StringIO

#
#リダイレクト時にUTF-8の日本語を含む文字を受け入れるための設定
#
sys.stdin  = codecs.getwriter('utf_8')(sys.stdin)
sys.stdout = codecs.getwriter('utf_8')(sys.stdout)
sys.stderr = codecs.getwriter('utf_8')(sys.stderr)

u"""
gen-asm-offset.py [options]
usage: gen-asm-offset.py -h -v 

options:
  --version                          版数情報表示
  -h, --help                         ヘルプ表示
  -v, --verbose                      冗長モードに設定
  -c, --config                       コンフィグファイルを設定
  -i INFILE,  --infile=inputfile     入力ファイル名
  -o OUTFILE, --outfile=outputfile   出力先ファイル名
"""

parser = OptionParser(usage="%prog [options]", version="%prog 1.0")
parser.add_option("-v", "--verbose", action="store_true", dest="verbose",
                  help="set verbose mode")
parser.add_option("-i", "--infile", action="store", type="string", dest="infile", help="path for input-file")
parser.add_option("-o", "--outfile", action="store", type="string", dest="outfile", help="path for output-file")


class genoffset:
    def __init__(self):
        self.cmdname = 'genoffset'
        self.infile = None
        self.outfile = None
        self.verbose = False
        self.msg_log = []
        self.war_log = []
        self.err_log = []
        self.dbg_log = []
        (self.options, self.args) = parser.parse_args()
        self.readConfig()
    def inf_msg(self, msg):
        self.msg_log.append(msg)
    def war_msg(self, msg):
        self.war_log.append(msg)
    def err_msg(self, msg):
        self.err_log.append(msg)
    def dbg_msg(self, msg):
        if self.getVerbose():
            self.dbg_log.append(msg)
    def setVerbose(self):
        self.verbose = True
    def getVerbose(self):
        return self.verbose;
    def getInfile(self):
        return self.infile
    def getOutfile(self):
        return self.outfile
    def outputHead(self, fh):
        fh.write("""/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Structure offset definitions                                      */
/*  Note: This file is generated automatically, don't edit by hand.   */ 
/*                                                                    */
/**********************************************************************/

#if !defined(_KLIB_ASM_OFFSET_H)
#define _KLIB_ASM_OFFSET_H

""")
    #
    # asm-offセットヘッダ生成関数
    #
    # asm-offset-helper.hのマクロが
    #
    # .ascii "@ASM_OFFSET@Cマクロ名[空白]マクロ定義値[空白]マクロ定義値算出式"
    #
    # というインラインアセンブラ文を出力する
    #
    # 例:.ascii "@ASM_OFFSET@TI_SIZE $32 sizeof(struct _thread_info)"
    #
    # 本関数は, @ASM_OFFSET@を含む行を取り出し以下を行い
    # Cプリプロセッサのマクロ定義を出力する
    #
    # 1) 行の先頭と末尾の空白/改行を削除
    #    例:\s*.ascii "@ASM_OFFSET@TI_SIZE $32 sizeof(struct _thread_info)"\s*
    #     =>.ascii "@ASM_OFFSET@TI_SIZE $32 sizeof(struct _thread_info)"
    #
    # 2) ".ascii空白の繰り返し"部分を削除
    #    例:.ascii "@ASM_OFFSET@TI_SIZE $32 sizeof(struct _thread_info)"
    #     =>"@ASM_OFFSET@TI_SIZE $32 sizeof(struct _thread_info)"
    #
    # 3) @ASM_OFFSET@を削除
    #    例:"@ASM_OFFSET@TI_SIZE $32 sizeof(struct _thread_info)"
    #     =>"TI_SIZE $32 sizeof(struct _thread_info)"
    #
    # 4) 前後の"を削除
    #    例:"TI_SIZE $32 sizeof(struct _thread_info)"
    #     =>TI_SIZE $32 sizeof(struct _thread_info)
    #
    # 5) 残る文字列を空白で区切って分割し, マクロ名, 
    #    定義値内に$があればを削除し, 定義値算出式文字列(コメント)を取り出し,
    #    Cプリプロセサ定義を出力する
    #    例:TI_SIZE $32 sizeof(struct _thread_info)
    #     =>#define TI_SIZE (32)  /*  sizeof(struct _thread_info)  */
    # 
    def genAsmOffset(self):

        try:
            fh = open(self.outfile, 'w')
        except IOError:
            print '"%s" cannot be opened.' % self.outfile
        else:
            try:
                f = open(self.infile, 'r')
            except IOError:
                print '"%s" cannot be opened.' % self.infile
            else:
                self.outputHead(fh)
                for line in f:                  
                    if (re.search(r'@ASM_OFFSET@', line)):

                        # 1-1) 行の先頭と末尾の空白を削除
                        line = line.lstrip()
                        line = line.rstrip()

                        # 1-2) 行の先頭と末尾の改行を削除
                        line = line.rstrip('\r')
                        line = line.rstrip('\n')

                        # 2) ".ascii空白の繰り返し"部分を削除,
                        line = re.sub(".ascii\s+","",line)

                        # 3) @ASM_OFFSET@を削除
                        line = line.replace("@ASM_OFFSET@","")

                        # 4) 前後の"を削除
                        line = line.lstrip('"')
                        line = line.rstrip('"')

                        # 5-1) 残る文字列を空白で区切って分割
                        # defs[0] マクロ名
                        # defs[1] 定義値(x64の場合, $文字含む)
                        # defs[2] 定義値算出式文字列(コメント)
                        defs = re.split('\s+', line ,2)

                        # 5-2) Cプリプロセサ定義を出力する
                        # #define マクロ名 ($を取り除いた定義値) 
                        #                     /*< 定義値算出式 */
                        # というマクロ定義が出力される
                        fh.write("#define %s (%s) /*< %s */\n" % 
                                 (defs[0], re.sub('\$*', '', 
                                                  defs[1]), defs[2]))
                fh.write("#endif  /*  _KLIB_ASM_OFFSET_H   */\n")
                f.close()
            fh.close() 
        return

    def readConfig(self):
        if self.options.verbose:
            self.setVerbose()

        if self.options.infile and os.path.exists(self.options.infile) and os.path.isfile(self.options.infile):
            self.infile = self.options.infile
        else:
            self.infile="asm-offset.s"

        if self.options.outfile:
            self.outfile = self.options.outfile
        else:
            self.outfile = "asm-offset.h"
            
if __name__ == '__main__':

    obj = genoffset()
    obj.readConfig()
    obj.genAsmOffset()



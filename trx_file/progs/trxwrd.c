#ifdef _MSC_VER
/*NOTE -- Must name code segmen _TRX_TEXT*/
#include <bios.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <trx_str.h>
#include <trx_dt.h>
#else
#include <a_string.h>
#include <a_malloc.h>
#include <a_stdio.h>
#include <a_keys.h>
#include <a_dt.h>
#endif

#include <trx_file.h>

extern int _trx_errLine;

static char hdr[]="TRXWRD -- v1.00 Copyright 1991 by David McKenzie";

#undef _TESTING
#undef MAX_RECS

#define NUMTREES 1
#define SIZEXTRA 0

static int siztxtbuf=4096;
static int siznode=1024;
static int sizcache=256;
static int sizkey=255;
static int sizrec=2;
static int sizlink=2;
static int minPreWrite=1;
static UINT initflg=0;
static UINT mode=0;

#define SIZ_WORDBUF 30

static TRX_NO trx;
static FILE *fp;
static ulong rec,dup,wrd;
static byte wordbuf[SIZ_WORDBUF+2];
static byte trxname[TRX_MAX_PATH];
static flag trx_opened;

#ifdef MAX_RECS
static ulong max_recs;
#endif

static void eprint(nptr format,...)
{
  va_list marker;

  va_start(marker,format);
  vfprintf(stderr,format,marker);
  va_end(marker);
}

static int key_abort(void)
{
#ifdef _MSC_VER
  while(_bios_keybrd(_KEYBRD_READY))
   if((_bios_keybrd(_KEYBRD_READ)&0xFF)==0x1B) return 1;
#else
  if(cio_EscPressed()) return 1;
#endif
  return 0;
}

#define _IFCN(e) trxfcn(e,__LINE__)

static void trxfcn(int e,int line)
{
  if(e) {
#ifdef _MSC_VER
    eprint("\nAborted - Line %d: %Fs.\n",line,trx_Errstr(e));
#else
    eprint("\nAborted - Line %d: %ls.\n",line,trx_Errstr(e));
#endif
    if(e==TRX_ErrFormat) eprint("Program line: %d.\n",_trx_errLine);
    if(trx_opened) trx_Close(trx);
    exit(1);
  }
}

#ifdef _TESTING
static apfcn_v ScanText(void)
{
#define NUM_PASSES 2
#define PASS_PERCENT 10
#define DUP_PERCENT 10
#define DUP_FACTOR 100

  int c;
  nptr p;
  UINT e;
  int passes=0;

  rec=dup=wrd=0L;

  dt_RandSeed=3L;

_newpass:
  ++passes;
  while(TRUE) {
    if(key_abort()) goto _abt1;
    p=wordbuf;
    while((c=getc(fp))!=-1 && c!='\n') {
      if(c!='\r' && p<wordbuf+SIZ_WORDBUF-1) *++p=c;
    }
    if(c==-1) break;
    if((p-wordbuf)>=2) {
      wrd++;
      if((dt_Rand()%100)<PASS_PERCENT) continue;
      p[1]=0;
      *wordbuf=p-wordbuf;
#ifdef MAX_RECS
      if(max_recs && dup+rec>=max_recs-1) {
        if(max_recs && dup+rec>=max_recs) goto _abt0;
      }
#endif
      _IFCN(trx_InsertKey(trx,&passes,wordbuf));
      if((++rec%100)==0L) eprint("%7lu\b\b\b\b\b\b\b",rec);
      if((e=(dt_Rand()%100))<DUP_PERCENT) {
        e*=DUP_FACTOR;
        do {
#ifdef MAX_RECS
          if(max_recs && dup+rec>=max_recs-1) {
            if(max_recs && dup+rec>=max_recs) goto _abt0;
          }
#endif
          _IFCN(trx_InsertKey(trx,&passes,wordbuf));
          dup++;
        }
        while(e--);
      }
    }
  }
  eprint("Pass%2d -- Keys:%7lu  Dups:%7lu  Words scanned:%7lu\n",
    passes,rec,dup,wrd);
  if(passes==NUM_PASSES) return;
  e=fseek(fp,0L,SEEK_SET);
  goto _newpass;

_abt0:
  eprint("\nNext key to add: %s",wordbuf+1);
_abt1:
  eprint("\nAborted.\n");
  trx_Close(trx);
  exit(1);
}
#else
static apfcn_v ScanText(void)
{

  int c;
  nptr p;

  rec=0L;

  eprint("Adding keys: ");

  while(TRUE) {
    p=wordbuf;
    while((c=getc(fp))!=-1 && c!='\n') {
      if(c!='\r' && p<wordbuf+SIZ_WORDBUF-1) *++p=c;
    }
    if(c==-1) break;
    if((p-wordbuf)>=2) {
      *wordbuf=p-wordbuf;
      _IFCN(trx_InsertKey(trx,&rec,wordbuf));
      if((++rec%100)==0L) {
        eprint("%7lu\b\b\b\b\b\b\b",rec);
        if(key_abort()) {
          eprint("%7lu\nAborted.\n",rec);
          goto _end;
        }
      }
    }
  }
  eprint("%7lu\nCompleted.\n",rec);

_end:
  _IFCN(trx_Close(trx));
  exit(0);
}
#endif
/*==============================================================*/
void main(int argc,nptr *argv)
{
  nptr p;
  int e;

#ifndef _MSC_VER
  arcinit();
#endif

  eprint("\n%s",hdr);

  while(--argc) {
#ifdef _MSC_VER
    trx_Strupr(p=*++argv);
#else
    strupr(p=*++argv);
#endif

    if(*p=='-') {
      e=atoi(p+2);
      switch(p[1]) {
#ifdef MAX_RECS
        case 'X' : max_recs=atol(p+2); break;
#endif
        case 'F' : mode|=TRX_AutoFlush; break;
        case 'P' : minPreWrite=e; break;
        case 'K' : sizkey=e; break;
        case 'R' : sizrec=e; break;
        case 'L' : sizlink=e; break;
        case 'C' : sizcache=e; break;
        case 'N' : siznode=e; break;
        case 'T' : siztxtbuf=e; break;
        case 'U' : initflg |= TRX_KeyUnique;
      }
      continue;
    }
    if(*trxname) {
      *trxname=0;
      break;
    }
    else
#ifdef _MSC_VER
    trx_Stcext(trxname,p,"TXT",TRX_MAX_PATH);
#else
    stccext(trxname,p,"TXT",TRX_MAX_PATH);
#endif
  }

  if(!*trxname) {
    eprint("\n\nTRXWRD <filename[.TXT]> [-n<siznode>] "
      "[-c<sizcache>] [-t<siztxtbuf>]\n"
      "     [-r<sizrec>] [-k<sizkey>] [-l<sizlink] [-p<minPW] [-Unique] [-Flush]\n"
      "\nCreates <filename>.TRX from filename[.TXT] (one key per line).\n"
      "Defaults: siznode=%u, sizcache=%u, siztxtbuf=%u, sizrec=%u, sizkey=%u,\n"
      "          sizlink=%u, minPW=%u -Unique=%u, -Flush=%u.\n",
      siznode,sizcache,siztxtbuf,sizrec,sizkey,sizlink,minPreWrite,
      (initflg!=0),(mode!=0)
    );
    exit(0);
  }

  if((fp=fopen(trxname,"rb"))==NULL || setvbuf(fp,0,_IOFBF,siztxtbuf)) {
    eprint("\n\nCannot open file: %s\n",trxname);
    exit(1);
  }
#ifdef _MSC_VER
  trx_Stccc(trxname,trx_Stpnam(trxname));
  *trx_Stpext(trxname)=0;
#else
  _stccc(trxname,_stpname(trxname));
  *_stpext(trxname)=0;
#endif

  if(siznode==512) {
    if(sizcache<0) sizcache=512;
    if(sizkey==255) {
      sizkey=0;
      initflg|=TRX_KeyMaxSize;
    }
  }
  _IFCN(trx_Create(&trx,trxname,mode,NUMTREES,SIZEXTRA,siznode,sizlink));
  if(minPreWrite!=1) _IFCN(trx_SetMinPreWrite(trx,minPreWrite));
  _IFCN(trx_InitTree(trx,sizrec,sizkey,initflg));
  trx_opened=TRUE;
  _IFCN(trx_AllocCache(trx,2,sizcache));
  eprint("\n\nCreating %s -- Node size: %u  Buffers: %u\n",
    trxname,siznode,csh_NumBuffers(trx_Cache(trx)));
  ScanText();
  exit(0);
}

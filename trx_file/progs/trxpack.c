/*TRXPACK.C -- Create a packed TRX file from an unpacked TRX file*/

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <malloc.h>
#include <stdio.h>
#include <ctype.h>
#include <dos_io.h>
#include <trx_file.h>

#define _DBUG

static char hdr[]="TRXPACK -- v2.0 Copyright 2001 by David McKenzie";

static int sizcache=64;

#define SIZ_KEYBUF 256

static TRX_NO trxold,trx;
static long rec;
static byte keybuf[SIZ_KEYBUF];
static byte trxoldname[TRX_MAX_PATH],trxname[TRX_MAX_PATH];
static flag trx_opened,old_opened;

static void eprint(PBYTE format,...)
{
  va_list marker;
  va_start(marker,format);
  vfprintf(stderr,format,marker);
  va_end(marker);
}

#ifdef _DBUG           
#define _NFCN(e) if(e) trxfcn(trxname,__LINE__,e)
#define _OFCN(e) if(e) trxfcn(trxoldname,__LINE__,e)
static void trxfcn(byte *name,int line,int e)
{
    eprint("\nAborted - %s Line %d: %Fs.\n",name,line,trx_Errstr(e));
    if(e==TRX_ErrFormat)
     eprint("Library function line: %d\n",_trx_errLine);
    if(trx_opened) trx_CloseDel(trx);
    if(old_opened) trx_Close(trxold);
    exit(1);
}
#else
#define _NFCN(e) if(e) trxfcn(trxname,e)
#define _OFCN(e) if(e) trxfcn(trxoldname,e)
static void trxfcn(byte *name,int e)
{
  if(e) {
    eprint("\nAborted - %s: %Fs.\n",name,trx_Errstr(e));
    if(trx_opened) trx_CloseDel(trx);
    if(old_opened) trx_Close(trxold);
    exit(1);
  }
}
#endif

static apfcn_v rshow(void)
{
  eprint("%7lu\b\b\b\b\b\b\b",rec);
}

static apfcn_v CopyTree(void)
{
  int e;
  UINT sizrec;
  PBYTE recptr=NULL;

  if((e=trx_SizKey(trxold))==0) return;
  _NFCN(trx_InitTree(trx,
    sizrec=trx_SizRec(trxold),e,trx_InitTreeFlags(trxold)));
  if(sizrec && (recptr=malloc(sizrec))==0) return;

  trx_SetDescend(trx,TRUE);

  eprint("\nTree%4u - Keys added:",(trx&0xFF)+1);
  rec=0L;
  e=trx_Last(trxold);

  while(!e) {
    _OFCN(trx_Get(trxold,recptr,sizrec,keybuf,SIZ_KEYBUF));
    _NFCN(trx_InsertKey(trx,recptr,keybuf));
    e=trx_Prev(trxold);
    if((++rec%100L)==0L) {
      rshow();
	  while(_kbhit()) if(_getch()==0x1B) {
        eprint("\nAborted.\n");
        trx_Close(trxold);
        trx_Close(trx);
        exit(1);
      }
    }
  }
  rshow();
  if(e!=TRX_ErrEOF) _OFCN(e);
  free(recptr);
}

/*==============================================================*/
int main(int argc,char **argv)
{
  PBYTE p;
  int e;

  eprint("\n%s\n",hdr);

  while(--argc) {
    p=*++argv;
    if(*p=='-') {
      switch(toupper(p[1])) {
        case 'C' : sizcache=atoi(p+2); break;
      }
      continue;
    }
    if(*trxoldname) {
      if(!*trxname) {
        if(!strcmp(trxoldname,p)) break;
        strncpy(trxname,p,TRX_MAX_PATH);
      }
    }
    else strncpy(trxoldname,p,TRX_MAX_PATH);
  }

  if(!*trxname) {
    eprint("\nUsage: TRXPACK <oldname[.TRX]> <newname[.TRX]>\n"
      "Creates packed index, <newname>[.TRX], from oldname[.TRX].\n");
    exit(0);
  }

  _OFCN(trx_Open(&trxold,trxoldname,TRX_ReadOnly));
  _OFCN(trx_AllocCache(trxold,sizcache,TRUE));
  old_opened=TRUE;
  
  _NFCN(trx_CreateInd(&trx,trxname,TRX_ReadWrite,trx_FileDef(trxold)));
  trx_opened=TRUE;
  
  memcpy(trx_ExtraPtr(trx),trx_ExtraPtr(trxold),trx_SizExtra(trx));
  trx_MarkExtra(trx);
  
  _NFCN(trx_SetMinPreWrite(trx,0));

  CopyTree();
  for(e=trx_NumTrees(trx);e>1;e--) {
    trx++;
    trxold++;
    CopyTree();
  }
  
  eprint("\nPack completed - Old size: %lu x %ub  "
    "New size: %lu x %ub.\n",
     trx_NumFileNodes(trxold),trx_SizNode(trxold),
     trx_NumFileNodes(trx),trx_SizNode(trx));
     
  _NFCN(trx_Close(trx));
  trx_Close(trxold);
  exit(0);
}

/*NOTE -- Must name code segmen _TRX_TEXT*/

#include <bios.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <trx_str.h>
#include <trx_file.h>

static char hdr[]="TRXTST -- v1.00 Copyright 1991 by David McKenzie";

static int siznode=1024;
static int sizcache=256;
static int sizkey=255;

#define SIZ_WORDBUF 30

static TRX_NO trx;
static word initflg;
static byte wordbuf[SIZ_WORDBUF];
static byte trxname[TRX_MAX_PATH];
static flag trx_opened,deleting;

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
    eprint("\nAborted - Line %d: %Fs.\n",line,trx_Errstr(e));
    if(e==TRX_ErrFormat) eprint("Program line: %d.\n",_trx_errLine);
    if(trx_opened) trx_Close(trx);
    exit(1);
  }
}

static apfcn_v ScanText(void)
{
  word rec;
  int e;

  rec=0;
  trx_Stccp(wordbuf,"ABC");
  while(TRUE) {
    if(key_abort()) {
      eprint("\nAborted.\n");
      trx_Close(trx);
      exit(1);
    }
    rec++;
    if(trx_InsertKey(trx,&rec,wordbuf)) {
        if(trx_errno!=TRX_ErrDup) _IFCN(trx_errno);
    }
    if((rec%100)==0) {
      eprint("%6u\b\b\b\b\b\b",rec);
      if(rec==1000) break;
    }
    if(rec==250) trx_Stccp(wordbuf,"EFG");
    else if(rec==500) trx_Stccp(wordbuf,"FGH12");
    else if(rec==750) trx_Stccp(wordbuf,"FG12345");
  }
  rec=0;
  e=trx_Find(trx,"\3EFG");
  while(!e) {
    rec++;
    e=trx_FindNext(trx,"\3EFG");
  }
  eprint("\nMatches scanned (250 expected): %d\n",rec);
  if(!deleting) return;
  eprint("Deleting 1000 keys..");
  _IFCN(trx_First(trx));
  for(e=1000;e--;) {
    _IFCN(trx_DeleteKey(trx));
    if(e) _IFCN(trx_Next(trx));
  }
}

/*==============================================================*/
void main(int argc,nptr *argv)
{
  nptr p;
  int e;

  /*arcinit();*/
  eprint("\n%s",hdr);

  while(--argc) {
    trx_Strupr(p=*++argv);
    if(*p=='-') {
      e=atoi(p+2);
      switch(p[1]) {
        case 'D' : deleting=TRUE; break;
        case 'K' : sizkey=e; break;
        case 'C' : sizcache=e; break;
        case 'N' : siznode=e; break;
        case 'U' : initflg |= TRX_KeyUnique;
      }
      continue;
    }
    if(*trxname) {
      *trxname=0;
      break;
    }
    else trx_Stcext(trxname,p,"TRX",TRX_MAX_PATH);
  }

  if(!*trxname) {
    eprint("\n\nTRXTST <name[.TRX]> [-n<siznode>] "
      "[-c<sizcache>] [-k<sizkey>] [-Uniq] [-Del]\n"
      "Defaults are siznode=1024, sizcache=256, sizkey=255.\n");
    exit(0);
  }

  if(sizkey==255) {
    sizkey=0;
    initflg|=TRX_KeyMaxSize;
  }
  _IFCN(trx_Create(&trx,trxname,0,1,0,siznode,2));
  _IFCN(trx_InitTree(trx,2,sizkey,initflg));
  trx_opened=TRUE;
  _IFCN(trx_AllocCache(trx,32,sizcache));
  eprint("\n\nCreating %s -- Node size: %u  Buffers: %u\n",
    trxname,siznode,csh_NumBuffers(trx_Cache(trx)));
  ScanText();
  _IFCN(trx_Close(trx));
  eprint("\nCompleted.\n");
  exit(0);
}

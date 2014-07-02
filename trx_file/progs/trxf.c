#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>
#include <dos_io.h>
#include <time.h>
#include <trx_str.h>
#include <trx_file.h>
#include <dbf_file.h>

extern int _trx_errLine;

static char hdr[]="TRXF -- v2.00 Copyright 1997 by David McKenzie";

#define NUMTREES 1
#define SIZEXTRA 0

static int siznode=1024;
static int sizdbfbuf=1024;
static int sizcache=128;
static int sizkey=255;
static int sizrec=4;
static int sizlink=4;
static int minPreWrite=0;
static UINT initflg=0;
static UINT mode=TRX_ReadWrite;

#define SIZ_WORDBUF 256

static TRX_NO trx;
static DBF_NO dbf;
static int fldlen;
static ulong lastrec,rec,siztrx;
static PBYTE recptr,fldptr;
static byte wordbuf[SIZ_WORDBUF];
static byte trxname[TRX_MAX_PATH];
static byte fldname[12];
static byte dbfname[TRX_MAX_PATH];
static flag dbf_opened,trx_opened,inclTrail,exclDel;

static void eprint(PBYTE format,...)
{
  va_list marker;

  va_start(marker,format);
  vfprintf(stderr,format,marker);
  va_end(marker);
}

static int key_abort(void)
{
  while(_kbhit()) if(_getch()==0x1B) 	return 1;
  return 0;
}

#define _IFCN(e) trxfcn(e,__LINE__)
#define _DFCN(e) dbffcn(e,__LINE__)

static void closexit(void)
{
  if(trx_opened) trx_Close(trx);
  if(dbf_opened) dbf_Close(dbf);
  exit(1);
}

static void trxfcn(int e,int line)
{
  if(e) {
    eprint("\nAborted - Line %d: %s.\n",line,trx_Errstr(e));
    if(e==TRX_ErrFormat) eprint("Program line: %d.\n",_trx_errLine);
    closexit();
  }
}

static void dbffcn(int e,int line)
{
  if(e) {
    eprint("\nAborted - Database %s (%d): %s.\n",dbfname,line,dbf_Errstr(e));
    closexit();
  }
}

static int CALLBACK getkey(int typ)
{
  int e;
  PBYTE p;

  if(typ) {
	  eprint(trx_bld_message);
	  return key_abort()?TRX_ErrUsrAbort:0;
  }
  while(rec<lastrec) {
    if((e=dbf_Go(dbf,++rec))) return e;
    if(!exclDel || *recptr!='*') {
      memcpy(trx_bld_keybuf+1,fldptr,fldlen);
      e=fldlen;
      if(!inclTrail) for(p=trx_bld_keybuf+e;e && *p==' ';e--,p--);
      *trx_bld_keybuf=e;
	  if(sizrec>=4) *(ulong *)trx_bld_recbuf=rec;
	  return 0;
    }
  }
  return -1;
}

static apfcn_v BuildDbf(void)
{
  clock_t tm;

  rec=0L;

  eprint("Indexing: ");

  tm=clock();

  trx_bld_cachebufs=sizcache;

  _IFCN(trx_BuildTree(trx,NULL,getkey));
  siztrx=trx_SizFile(trx);

  _IFCN(trx_Close(trx));
  tm=clock()-tm;
  eprint("%7lu\nCompleted - Time: %.2f  Size: %lu.%01d\n",
	rec,
    (double)tm/CLOCKS_PER_SEC,
    siztrx/1024L,((WORD)(siztrx%1024L)*10)/1024);
  dbf_Close(dbf);
}

static apfcn_v ScanDbf(void)
{

  int i;
  PBYTE p;
  clock_t tm;

  rec=0L;

  eprint("Adding keys: ");

  tm=clock();
  while(rec<lastrec) {
    _DFCN(dbf_Go(dbf,++rec));
    if(!exclDel || *recptr!='*') {
      memcpy(wordbuf+1,fldptr,fldlen);
      i=fldlen;
      if(!inclTrail) for(p=wordbuf+i;i && *p==' ';i--,p--);
      *wordbuf=i;
      _IFCN(trx_InsertKey(trx,&rec,wordbuf));
    }
    if((rec%500)==0L) {
      eprint("%7lu\b\b\b\b\b\b\b",rec);
      if(key_abort()) {
        eprint("%7lu\nAborted.\n",rec);
        closexit();
      }
    }
  }
  siztrx=trx_SizFile(trx);

  _IFCN(trx_Close(trx));
  tm=clock()-tm;
  eprint("%7lu\nCompleted - Time: %.2f  Size: %lu.%01d\n",
	rec,
    (double)tm/CLOCKS_PER_SEC,
    siztrx/1024L,((WORD)(siztrx%1024L)*10)/1024);
  dbf_Close(dbf);
}

/*==============================================================*/
int main(int argc, char **argv)
{
  PBYTE p;
  int e;
  BOOL merge=FALSE;

  eprint("\n%s",hdr);

  while(--argc) {
    p=*++argv;

    if(*p=='-') {
      e=atoi(p+2);
      switch(toupper(p[1])) {
	    case 'M' : merge=TRUE; break;
        case 'C' : sizcache=e; break;
        case 'D' : sizdbfbuf=e; break;
        case 'E' : exclDel=TRUE; break;
        case 'F' : mode|=TRX_AutoFlush; break;
        case 'I' : inclTrail=TRUE; break;
        case 'K' : sizkey=e; break;
        case 'L' : sizlink=e; break;
        case 'N' : siznode=e; break;
        case 'P' : minPreWrite=e; break;
        case 'R' : sizrec=e; break;
        case 'U' : initflg |= TRX_KeyUnique;
      }
      continue;
    }
    if(!*dbfname)
      trx_Stncc(dbfname,p,TRX_MAX_PATH);
    else if(!*fldname)
      _strupr(trx_Stncc(fldname,p,10));
    else if(!*trxname)
      trx_Stncc(trxname,p,TRX_MAX_PATH);
    else {
      *fldname=0;
      break;
    }
  }

  if(!*fldname) {
    eprint("\n\n"

    "TRXF <dbfname[.DBF]> <fieldname> [trxname[.TRX]]\n"
    "     [-InclTrail] [-ExclDel] [-Unique] [-Flush] [-p<minPW>]\n"
    "     [-n<siznode>] [-c<sizcache>] [-r<sizrec>] [-l<sizlink]\n"
    "     [-d<sizdbfbuf>] [-k<sizkey>] [-Merge]\n\n"
    "Creates TRX index, <trxname>, from xBase type database, <dbfname>.\n"
    "Defaults: <trxname>=<dbfname>, siznode=%u, sizcache=%u, sizdbfbuf=%u,\n"
    "          sizrec=%u, sizkey=%u, sizlink=%u, minPW=%u.\n\n"
	"Note: Option -Merge will create a compacted index.\n",
      siznode,sizcache,sizdbfbuf,sizrec,sizkey,sizlink,minPreWrite);
    exit(0);
  }

  if(!merge && sizrec>4) sizrec=4;

  _DFCN(dbf_Open(&dbf,dbfname,DBF_ReadOnly));
  dbf_opened=TRUE;

  if((e=dbf_FldNum(dbf,fldname))==0) {
    eprint("\n\nField %s not present.\n",fldname);
    exit(1);
  }
  fldptr=dbf_FldPtr(dbf,e);
  recptr=dbf_RecPtr(dbf);
  fldlen=dbf_FldLen(dbf,e);
  lastrec=dbf_NumRecs(dbf);
  /*If not testing: if(sizrec<4 && lastrec>65535L) sizrec=4;*/

  if(sizdbfbuf>=0)
   _DFCN(dbf_AllocCache(dbf,sizdbfbuf,1,0));

  if(!*trxname) {
    strcpy(trxname,trx_Stpnam(dbfname));
    *trx_Stpext(trxname)=0;
  }
  if(siznode==512) {
    if(sizkey==255) {
      sizkey=0;
      initflg|=TRX_KeyMaxSize;
    }
  }
  _IFCN(trx_Create(&trx,trxname,mode,NUMTREES,SIZEXTRA,siznode,sizlink));
  if(minPreWrite!=1) _IFCN(trx_SetMinPreWrite(trx,minPreWrite));
  _IFCN(trx_InitTree(trx,sizrec,sizkey,initflg));
  trx_opened=TRUE;
  if(!merge) _IFCN(trx_AllocCache(trx,sizcache,FALSE));
  eprint("\n\nCreating %s -- Node size: %u  Buffers: %u\n",
	  trxname,siznode,merge?sizcache:csh_NumBuffers(trx_Cache(trx)));

  (merge?BuildDbf:ScanDbf)();

  exit(0);
}













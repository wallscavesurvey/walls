/*TRXT.C --*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <conio.h>
#include <ctype.h>
#include <dos_io.h>
#include <trx_str.h>
#include <a__trx.h>

#define _TEST

#ifdef _TEST
static byte hdr[]="TRXT v2.00 Copyright 2001 by David McKenzie";
#else
static byte hdr[]="TRXV v2.00 Copyright 2001 by David McKenzie";
#endif

static int siznod=512;

static TRX_NO trx;
static int treeno,sizrec,prewrite,mode;
static long keyno;
static UINT keytotal,leaftotal;
static HNode _show_last_rlink,_show_last_leaf;
static int _show_detail,_show_placemark;
static byte trxname[TRX_MAX_PATH];

#define MAXTREES 100

static flag creatflag,uniqueflag,trx_opened;

#define FCHK(e) filechk(__LINE__,e)

#define dos_KeyPressed() _kbhit()
#define dos_InKey() _getch()

static apfcn_i dos_InChar(void)
{
  int c;
  //Discard function and arrow keys --
  while((c=_getch())==0 || c==0x0E) _getch();
  return c;
}

static char *dos_InStr(PBYTE buf,UINT bufsiz)
{
	//buf[0]=bufsiz-2; strcpy(buf,_cgets(buf)); //SHOULD WORK BUT DOESN'T!

	UINT len=0;
	int c;

	while(len<bufsiz) {
	  if((c=dos_InChar())=='\r') break;
	  if(c>=' ' && c<='~') {
		 if(len+1<bufsiz) {
		   buf[len++]=c;
		   printf("%c",c);
		 }
	  }
	  else if(c=='\b' && len) {
		len--;
		printf("\b \b");
	  }
    }
	buf[len]=0;
	printf("\n");
	return buf;
}

static void nl(void)
{
	printf("\n");
}

static char *xchgchr(char *pstr,char cold,char cnew)
{
	char *p=pstr;
	for(;*p;p++) if(*p==cold) *p=cnew;
	return pstr;
}


static apfcn_v fix_path(PBYTE path)
{
  PBYTE p;
  char buf[TRX_MAX_PATH];

  if(trx_Stpnam(path)!=path || (p=getenv("TRXMAP"))==0 || !*p) return;
  trx_Stncc(buf,p,TRX_MAX_PATH-1);
  p=buf+strlen(buf);
  if(*(p-1)!='\\') *p++='\\';
  trx_Stncc(p,path,TRX_MAX_PATH-(p-buf)-4);
  if(*(p=trx_Stpext(p))==0) strcpy(p,".map");
  strcpy(path,buf);
}

static apfcn_v filechk(int line,int e)
{
  if(e) {
    printf("Aborted: Line %d - %s - %s.\n",line,trxname,trx_Errstr(e));
    if(trx_opened) trx_Close(trx);
    exit(1);
  }
}

static apfcn_i ShowError(int e,int dir)
{
  if(e==TRX_ErrEOF) {
    if(trx_NumRecs(trx)==0L) dir=0;
    if(!dir) printf("\r<TREE IS EMPTY>");
    else printf(dir>0?"\r<END OF INDEX>":"\r<TOP OF INDEX>");
  }
#ifdef _TRX_TEST
  else if(e==TRX_ErrFormat) printf("\rFile corrupted - Line %d.",_trx_errLine);
#endif
  else printf("\rError %d: %Fs.",e,trx_Errstr(e));
  return e;
}

#ifdef _TEST
static apfcn_v skip_d(int depth)
{
  while(depth--) printf("  ");
}

static acfcn_i ErrFormat(PBYTE s,...)
{
  char buf[256];
  va_list p;
  va_start(p,s);
  vsprintf(buf,s,p);
  printf("*** Node format error - %s ***\n",buf);
  return 0;
}

static CSH_HDRP ShowNodeKeys(CSH_HDRP hp,int depth)
{
  int len;
  TRX_pNODE nd=_Node(hp);
  int szlink=-nd->N_SizLink;
  int i,k=nd->N_NumKeys;
  ptr p=(ptr)nd+nd->N_FstLink;

  printf("\n");
  skip_d(depth);
  if(szlink<0) {
	  ErrFormat("Bad link size");
	  return 0;
  }
  for(i=1;i<=k;i++) {
	printf("%3d:",i);
	if(szlink==2) printf("%5u",*(WORD *)p);
	else if(szlink==4) printf("%10u",*(DWORD *)p);
	else printf("  ---");
    p+=szlink;
    printf("|%03u|",(UINT)p[1]);
    for(len=*p,p+=2;len--;p++) printf("%c",(*p>=' ' && *p<='~')?*p:'.');
    printf("|\n");
    if(k) skip_d(depth);
  }
  return hp;
}


static CSH_HDRP ShowNode(CSH_HDRP hp,int depth)
{
  TRX_pNODE nd=_Node(hp);
  flag dupcutright;
  char key[256];

  printf("\n");
  skip_d(depth);
  if(nd->N_Flags&TRX_N_Branch) {
    if(nd->N_NumKeys) {
      _trx_Btkey(hp,1,key,255);
      trx_Stxpc(key);
    }
    else strcpy(key,"<no keys>");
    printf("BRANCH LVL %2d:%5lu  Keys:%3u  Spc:%3u  R:%4lu  %s",
      depth,nd->N_This,
      nd->N_NumKeys,
      nd->N_FstLink-sizeof(TRX_NODE),
      nd->N_Rlink,
      key);
	return _show_detail?ShowNodeKeys(hp,depth):hp;
  }

_loop:

  _trx_Btkey(hp,1,key,255);
  key[key[0]+1]=0;
  printf("LEAF:%4lu  Keys:%3u  Spc:%3u  L:%4lu  R:%4lu %c%c%c %s",
      nd->N_This,nd->N_NumKeys,nd->N_FstLink-sizeof(TRX_NODE),
      nd->N_Llink,nd->N_Rlink,
      (nd->N_Flags&TRX_N_DupCutLeft)?'L':'-',
      (dupcutright=(nd->N_Flags&TRX_N_DupCutRight))?'R':'-',
      (_show_placemark=(nd->N_Flags&TRX_N_EmptyRight))?'E':'-',
      key+1);

  leaftotal++;
  keytotal+=nd->N_NumKeys;

  if(_show_detail && !ShowNodeKeys(hp,depth)) return 0;
  if(_show_last_rlink!=nd->N_This || _show_last_leaf!=nd->N_Llink) {
    printf("\n");
    skip_d(depth);
    ErrFormat("Bad links - LastLf: %u LastR: %u This: %u ThisL: %u",
		_show_last_leaf,_show_last_rlink,nd->N_This,nd->N_Llink);
  }
  _show_last_rlink=nd->N_Rlink;
  _show_last_leaf=nd->N_This;
  if(dupcutright) {
    printf("\n");
    skip_d(depth);
    if((hp=_trx_GetNode(_show_last_rlink))==0) {
      ErrFormat("Cannot access Rlink");
	  return 0;
	}
    nd=_Node(hp);
    goto _loop;
  }
  return hp;
}

static apfcn_i ShowSubtree(HNode n,int depth)
{
  CSH_HDRP hp;
  int i;

  if((hp=_trx_GetNode(n))==0) goto _err;
  if(!ShowNode(hp,depth)) return 0;
  if((_Node(hp)->N_Flags&TRX_N_Branch)!=0) {
    depth++;
    for(i=_Node(hp)->N_NumKeys;i;i--) {
      if(!ShowSubtree(_trx_Btlnkc(hp,i),depth)) return 0;
      /*Guard against reuse of buffer handle hp --*/
      if((hp=_trx_GetNode(n))==0) goto _err;
    }
    if(_show_placemark) {
      if(_Node(hp)->N_Rlink!=_show_last_leaf) {
        skip_d(depth);
        return ErrFormat("Rlink does not point to placemark");
      }
      _show_placemark=FALSE;
    }
    else if(!ShowSubtree(_Node(hp)->N_Rlink,depth)) return 0;
  }
  return 1;

_err:
  skip_d(depth);
  return ErrFormat("Cannot read node %lu - %s",n,trx_Errstr(trx_errno));
}

static apfcn_v ShowNodes(BOOL detail)
{
  _trx_GetTrx(trx);

  if(_trx_pTreeVar->Root==0L) {
    ShowError(TRX_ErrEOF,0);
    return;
  }
  printf("\r \r");
  _show_last_leaf=0L;
  _show_detail=detail;
 _show_last_rlink=_trx_pTreeVar->LowLeaf;
  keytotal=leaftotal=0;
  ShowSubtree(_trx_pTreeVar->Root,0);
  printf("\nTotal leaf keys: %u in %u nodes.",keytotal,leaftotal);
}


static apfcn_v ShowLeafNodes(BOOL detail)
{
	HNode n;
	CSH_HDRP hp;

  _trx_GetTrx(trx);

  if(_trx_pTreeVar->Root==0L) {
    ShowError(TRX_ErrEOF,0);
    return;
  }
  printf("\r \r");

 n=_show_last_rlink=_trx_pTreeVar->LowLeaf;
  _show_last_leaf=0L;
  _show_detail=detail;
 keytotal=leaftotal=0;

 while(n) {
	if((hp=_trx_GetNode(n))==0) goto _err;
    if(!(hp=ShowNode(hp,0))) break;
	n= _Node(hp)->N_Rlink;
 }

 printf("\nTotal leaf keys: %u in %u nodes.",keytotal,leaftotal);
 return;

_err:
 ErrFormat("Cannot read node %lu - %s",n,trx_Errstr(trx_errno));
}

static apfcn_v AddKey(void)
{
  int e;
  ulong numk;
  byte buf[130];

  printf("\rEnter string (0..127 chars):\n");
  /*Max length in DOS can be 128 with trailing LF, or 127 after trim --*/
  dos_InStr(buf,130);
  trx_Stxcpz(buf);
  if(buf[buf[0]]=='\n') {
    buf[buf[0]]=0;
    buf[0]--;
  }
  if(!*buf) return;
  printf("Ready to add key |%s|? [Y] ",buf+1);
  if(dos_InChar()=='N') return;
  numk=trx_NumRecs(trx)+1L;
  printf("\n");
  if((e=trx_InsertKey(trx,&numk,buf))!=0) ShowError(e,0);
  else {
    printf("After add - Nodes: %lu  Keys: %lu",
      trx_NumFileNodes(trx),trx_NumRecs(trx));
    keyno=0L;
  }
}

static apfcn_i DeleteKey(void)
{
  int e;
  PBYTE p;
  ulong n;
  byte buf[256];

  if((e=trx_GetKey(trx,buf,256))!=0) return ShowError(e,0);
  printf("\rDelete ");
  if(keyno) printf("%u|",keyno);
  if(sizrec && sizrec<=4) {
    n=0L;
    if((e=trx_GetRec(trx,&n,sizrec))!=0) return e;
    printf("%lu|",n);
  }
  for(p=buf,e=*p++;e--;p++) printf("%c",(*p>=' ' && *p<='~')?*p:'.');
  printf("|? [Y] ");
  if(dos_InChar()=='N') return 0;
  if((e=trx_DeleteKey(trx))==0) {
    printf("\nKey deleted.");
    if((e=trx_Next(trx))==0) return 0;
    keyno=0L;
    printf("\n");
  }
  return ShowError(e,1);
}
#endif

static apfcn_i ShowKey(void)
{
  int e;
  PBYTE p;
  ulong n;
  byte buf[256];

  if((e=trx_GetKey(trx,buf,256))!=0) return e;
  printf("\r");
#ifdef _TEST
  if(keyno) printf("%05u|",keyno);
#endif
  if(sizrec && sizrec<=4) {
    n=0L;
    if((e=trx_GetRec(trx,&n,sizrec))!=0) return e;
    printf("%07lu|",n);
  }
#ifdef _TEST
  else printf("|");
#endif
  for(p=buf,e=*p++;e--;p++) printf("%c",(*p>=' ' && *p<='~')?*p:'.');
#ifdef _TEST
  printf("|");
#endif
  return 0;
}

static apfcn_i NextAndShow(void)
{
  int e;

  if((e=trx_Next(trx))!=0) return ShowError(e,1);
  if(keyno) keyno++;
  return ShowKey();
}

static apfcn_i PrevAndShow(void)
{
  int e;

  if((e=trx_Prev(trx))!=0) return ShowError(e,-1);
  if(keyno) keyno--;
  return ShowKey();
}

static void ViewStats(void)
{
  printf("\rIndex: %s  Tree: %d of %d  SizPtr: %d  SizRec: %u  "
         "SizKey: %u  Keys: %lu\n",
    trx_FileName(trx),
    treeno,
    trx_NumTrees(trx),
    trx_SizNodePtr(trx),
    trx_SizRec(trx),
    trx_SizKey(trx),
    trx_NumRecs(trx));

  printf("Tree nodes: %lu of %lu x %ub  "
         "MinPWrt: %u (%u)  Free: %lu  Cache: %u x %ub\n",
    trx_NumTreeNodes(trx),
    trx_NumFileNodes(trx),
    trx_SizNode(trx),
    trx_MinPreWrite(trx),
    trx_NumPreWrite(trx),
    trx_NumFreeNodes(trx),
    csh_NumBuffers(trx_Cache(trx)),
    csh_SizBuffer(trx_Cache(trx))
    );
}

static apfcn_v ShowEOF(int e)
{
  if(trx_Last(trx)==0) {
    keyno=trx_NumRecs(trx);
    ShowKey();
    printf("\n");
  }
  ShowError(e,1);
}

static apfcn_v Skip(void)
{
  int e;
  byte buf[20];
  long len;

  printf("\rDistance to skip [+/-]: ");
  if((len=atol(dos_InStr(buf,20)))==0L) return;
  if((e=trx_Skip(trx,len))!=0) {
    if(e!=TRX_ErrEOF) {
      ShowError(e,0);
      return;
    }
    if(len<0L) {
      ShowError(e,-1);
      printf("\n");
      if(trx_First(trx)==0) keyno=1L;
    }
    else {
      ShowEOF(e);
      return;
    }
  }
  else if(keyno) keyno+=len;
  ShowKey();
}

static apfcn_v Find(void)
{
  int e;
  byte buf[256];

  printf("\rEnter key prefix: ");
  trx_Stxcp(dos_InStr(buf,256));
  if(!*buf) return;
  if((e=trx_Find(trx,buf))==0 || !(trx_findResult&0xFF)) {
    keyno=0L;
    ShowKey();
  }
  else if(e==TRX_ErrEOF || e==TRX_ErrMatch) {
	printf("No match found.\n");
	ShowKey();
  }
  else ShowError(e,0);
}

/*==============================================================*/
void main(int argc,PSTR *argv)
{
  int e;
  PBYTE p;
  
  printf("\n%s\n\n",hdr);

  while(--argc) {
    _strupr(p=*++argv);
    if(*p=='-') {
      switch(p[1]) {
        case 'T' : treeno=atoi(p+2); break;
#ifdef _TEST
        case 'U' : uniqueflag=TRX_KeyUnique; break;
        case 'R' : sizrec=atoi(p+2); break;
        case 'P' : prewrite=atoi(p+2); break;
        case 'F' : mode|=TRX_AutoFlush; break;
        case 'O' : mode|=TRX_ReadOnly; break;
        case 'C' : creatflag=TRUE;
                   if(p[2]) siznod=atoi(p+2);
                   break;
#endif
      }
    }
    else if(*trxname) {
      *trxname=0;
      break;
    }
    else trx_Stncc(trxname,p,TRX_MAX_PATH);
  }

  if(!*trxname) {
#ifdef _TEST
    printf("TRXT [-c<siznod>][-r<sizrec>][-t<treeno/numtrees>][-Unique][-Flush]\n"
           "     [-OnlyRead][-p<minPrewrite>] <name[.TRX]>\n");
#else
    printf("TRXV [-t<treeno>] <name[.TRX]>\n\n"
           "Scans a TRX index. If <name>.TRX has no specified path,\n"
           "the path will be taken from environment variable TRXMAP.\n");
#endif
    exit(0);
  }

  if(!creatflag) {
#ifndef _TEST
    mode|=TRX_ReadOnly;
#endif
    e=trx_Open(&trx,trxname,mode);
    if(e) {
      if(e==TRX_ErrFind) {
        fix_path(trxname);
        e=trx_Open(&trx,trxname,mode);
      }
      FCHK(e);
    }
    if(treeno<=0) treeno=1;
    else if(treeno>trx_NumTrees(trx)) treeno=trx_NumTrees(trx);
    sizrec=trx_SizRec(trx+treeno-1);
#ifdef _TEST
    if(uniqueflag) trx_SetUnique(trx+treeno-1,TRUE);
#endif
  }
#ifdef _TEST
  else {
    treeno=(treeno<=0)?1:(treeno>MAXTREES)?MAXTREES:treeno;
    FCHK(trx_Create(&trx,trxname,mode,treeno,0,siznod,2));
    if(prewrite!=1) FCHK(trx_SetMinPreWrite(trx,prewrite));
    FCHK(trx_InitTree(trx,sizrec,0,TRX_KeyMaxSize|uniqueflag));
    treeno=1;
  }
#endif
  trx+=treeno-1;

  trx_opened=TRUE;

  FCHK(trx_AllocCache(trx,32,FALSE));

  ViewStats();

  if((e=trx_First(trx))==0) {
    keyno=1L;
    ShowKey();
  }

  while(TRUE) {
    printf("\n:");
    switch(e=toupper(dos_InChar())) {
       case 'I'    : ViewStats(); break;
       case 'T'    : if((e=trx_First(trx))!=0) {
                       ShowError(e,0);
                       break;
                     }
                     printf("\r<TOP OF INDEX>\n");
                     keyno=1L;
                     ShowKey();
                     break;

       case 'P'    : PrevAndShow(); break;

       case 'B'    : if((e=trx_Last(trx))!=0) {
                       ShowError(e,0);
                       break;
                     }
                     keyno=trx_NumRecs(trx);
                     ShowKey();
                     printf("\n<END OF INDEX>");
                     break;

       case ' '    :
       case 'L'    : if(NextAndShow()) break;
                     if(e!='L') break;
                     while(!dos_KeyPressed()) {
                       printf("\n");
                       if(NextAndShow()) break;
                     }
                     /*Clear key buffer*/
                     while(dos_KeyPressed()) dos_InKey();
                     break;

       case 'S'    : Skip(); break;
       case 'F'    : Find(); break;
#ifdef _TEST
       case 'W'    : ShowLeafNodes(1); break;
       case 'Z'    : ShowLeafNodes(0); break;
       case 'M'    : ShowNodes(1); break;
       case 'N'    : ShowNodes(0); break;
       case 'A'    : AddKey(); break;
       case 'D'    : DeleteKey(); break;
#endif
       case 'X'    : printf("\r \r");
                     goto _ex0;
       default     :
#ifdef _TEST
         printf("\r(sp)next,Prev,Top,Bot,Find,Skip,List,duMp,Nodes,Add,"
         "Delete,Info,eXit --");
#else
         printf("\r(sp)next,Prev,Top,Bot,Find,Skip,List,Info,eXit --");
#endif
    }
  }

_ex0:
  trx_opened=FALSE;
  FCHK(trx_Close(trx));
  exit(0);
}

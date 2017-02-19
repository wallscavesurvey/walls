/*TRXT.C --*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <dos_io.h>
#include <trx_str.h>
#include <a__trx.h>
#include <arc_str.h>

#undef _TEST

#ifdef _TEST
static byte hdr[]="TRXT v2.00 Copyright 1999 by David McKenzie";
#else
static byte hdr[]="TRXV v2.00 Copyright 1999 by David McKenzie";
#endif

static int siznod=512;

static TRX_NO trx;
static int treeno,sizrec,prewrite,mode;
static long keyno;
#ifdef _TEST
static HNode _show_last_rlink,_show_last_leaf;
static int _show_detail,_show_placemark;
#endif
static byte trxname[TRX_MAX_PATH];

#define MAXTREES 100

static flag creatflag,uniqueflag,trx_opened,skutype;

#define FCHK(e) filechk(__LINE__,e)

static char *dos_InStr(PBYTE buf,UINT bufsiz)
{
	//buf[0]=bufsiz-2; strcpy(buf,_cgets(buf)); //SHOULD WORK BUT DOESN'T!

	UINT len=0;
	int c;

	while(len<bufsiz) {
	  if((c=_getch())=='\r') break;
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

static apfcn_v fix_path(char *path)
{
  char *p;
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
  else printf("\rError %d (%d): %s.",e,_trx_errLine,trx_Errstr(e));
  return e;
}

static void show_keystr(BYTE *buf)
{
	BYTE *p;
	UINT e;

	if(skutype) {
		printf(trx_Stxpc(skustr8(buf+8,buf)));
	}
	else {
		for(p=buf,e=*p++;e--;p++) printf("%c",(*p>=' ' && *p<='~')?*p:'.');
	}
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

static apfcn_i ShowNodeKeys(CSH_HDRP hp,int depth)
{
  int len;
  TRX_pNODE nd=_Node(hp);
  int szlink=-nd->N_SizLink;
  int k=nd->N_NumKeys;
  ptr p=(ptr)nd+nd->N_FstLink;

  printf("\n");
  skip_d(depth);
  if(szlink<0) return ErrFormat("Bad link size");
  while(k--) {
    if(szlink==0 || szlink>4) printf("  ---");
    else printf("%5lu",(*(ulong far *)p) & ((1L<<8*szlink)-1L));
    p+=szlink;
    printf("|%03u|",(UINT)p[1]);
    for(len=*p,p+=2;len--;p++) printf("%c",(*p>=' ' && *p<='~')?*p:'.');
    printf("|\n");
    if(k) skip_d(depth);
  }
  return 1;
}


static apfcn_i ShowNode(CSH_HDRP hp,int depth)
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
    return _show_detail?ShowNodeKeys(hp,depth):1;
  }

_loop:

  while(_kbhit()) if(_getch()==0x1B) {
    printf("Aborted..");
    return 0;
  }

  _trx_Btkey(hp,1,key,255);
  key[key[0]+1]=0;
  printf("LEAF:%4lu  Keys:%3u  Spc:%3u  L:%4lu  R:%4lu %c%c%c %s",
      nd->N_This,nd->N_NumKeys,nd->N_FstLink-sizeof(TRX_NODE),
      nd->N_Llink,nd->N_Rlink,
      (nd->N_Flags&TRX_N_DupCutLeft)?'L':'-',
      (dupcutright=(nd->N_Flags&TRX_N_DupCutRight))?'R':'-',
      (_show_placemark=(nd->N_Flags&TRX_N_EmptyRight))?'E':'-',
      key+1);
  if(_show_detail && !ShowNodeKeys(hp,depth)) return 0;
  if(_show_last_rlink!=nd->N_This || _show_last_leaf!=nd->N_Llink) {
    printf("\n");
    skip_d(depth);
    return ErrFormat("Bad node links");
  }
  _show_last_rlink=nd->N_Rlink;
  _show_last_leaf=nd->N_This;
  if(dupcutright) {
    printf("\n");
    skip_d(depth);
    if((hp=_trx_GetNode(_show_last_rlink))==0)
      return ErrFormat("Cannot access Rlink");
    nd=_Node(hp);
    goto _loop;
  }
  return 1;
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
  ShowSubtree(_trx_pTreeVar->Root,0);
}
#endif

static BOOL isNumeric(BYTE *s)
{
  UINT i=*s;

  while(i--) if(!isdigit(*++s)) return FALSE;
  return TRUE;
}

static apfcn_v AddKey(void)
{
  int e;
  ulong numk;
  byte buf[256];

_retry:
  if(skutype) printf("\rEnter SKU to insert (7 digits): ");
  else printf("\rEnter string (0..127 chars):\n");
  trx_Stxcp(dos_InStr(buf,256));
  if(!*buf) return;
  if(skutype) {
	  if(*buf!=7 || !isNumeric(buf)) {
		  printf("Not a 7-digit SKU.\n");
		  goto _retry;
	  }
  }
  printf("Ready to add key |%s|? [Y] ",buf+1);
  if(toupper(_getch())=='N') return;

  if(skutype) skukey(buf,lngval(buf));

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
  ulong n;
  byte buf[256];

  if((e=trx_GetKey(trx,buf,256))!=0) return ShowError(e,0);
  printf("\rDelete ");
  if(keyno) printf("%lu|",keyno);
  if(sizrec && sizrec<=4) {
    n=0L;
    if((e=trx_GetRec(trx,&n,sizrec))!=0) return e;
    printf("%lu|",n);
  }
  show_keystr(buf);
  printf("|? [Y] ");
  if(toupper(_getch())=='N') return 0;
  if((e=trx_DeleteKey(trx))==0) {
    printf("\nKey deleted.");
    if((e=trx_Next(trx))==0) return 0;
    keyno=0L;
    printf("\n");
  }
  return ShowError(e,1);
}

static apfcn_i ShowKey(void)
{
  int e;
  ulong n;
  byte buf[256];

  if((e=trx_GetKey(trx,buf,256))!=0) return e;
  printf("\r");
  if(keyno) printf("%05lu|",keyno);
  if(sizrec && sizrec<=4) {
    n=0L;
    if((e=trx_GetRec(trx,&n,sizrec))!=0) return e;
    printf("%7ld|",n);
  }
  else printf("|");
  show_keystr(buf);
  printf("|");
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

_retry:
  if(skutype) printf("\rEnter SKU to find (7 digits): ");
  else printf("\rEnter key prefix: ");
  trx_Stxcp(dos_InStr(buf,256));
  if(!*buf) return;
  if(skutype) {
	  if(*buf!=7 || !isNumeric(buf)) {
		  printf("Not a 7-digit SKU.\n");
		  goto _retry;
	  }
	  skukey(buf,lngval(buf));
  }
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
int main(int argc,PSTR *argv)
{
  int e;
  PBYTE p;
  
  printf("\n%s\n\n",hdr);

  while(--argc) {
    p=*++argv;
    if(*p=='-') {
      switch(toupper(p[1])) {
        case 'T' : treeno=atoi(p+2); break;
		case 'S' : skutype=TRUE; 
        case 'U' : uniqueflag=TRX_KeyUnique; break;
        case 'R' : sizrec=atoi(p+2); break;
        case 'P' : prewrite=atoi(p+2); break;
        case 'F' : mode|=TRX_AutoFlush; break;
        case 'O' : mode|=TRX_ReadOnly; break;
        case 'C' : creatflag=TRUE;
                   if(p[2]) siznod=atoi(p+2);
                   break;
      }
    }
    else if(*trxname) {
      *trxname=0;
      break;
    }
    else trx_Stncc(trxname,p,TRX_MAX_PATH);
  }

  if(!*trxname) {
    printf("TRXV [-c<siznod>][-r<sizrec>][-t<treeno/numtrees>][-Unique][-Flush]\n"
           "     [-OnlyRead][-p<minPrewrite>] <name[.TRX]>\n");
    exit(0);
  }

  if(!creatflag) {
    mode|=TRX_ReadOnly;
    e=trx_Open(&trx,trxname,mode);
    if(e) {
      if(e==TRX_ErrFind) {
        fix_path(trxname);
        e=trx_Open(&trx,trxname,mode);
      }
      FCHK(e);
    }
    sizrec=trx_SizRec(trx);
    if(treeno<=0) treeno=1;
    else if(treeno>trx_NumTrees(trx)) treeno=trx_NumTrees(trx);
    if(uniqueflag) trx_SetUnique(trx+treeno-1,TRUE);
  }
  else {
    treeno=(treeno<=0)?1:(treeno>MAXTREES)?MAXTREES:treeno;
    FCHK(trx_Create(&trx,trxname,mode,treeno,0,siznod,2));
    if(prewrite!=1) FCHK(trx_SetMinPreWrite(trx,prewrite));
    FCHK(trx_InitTree(trx,sizrec,0,TRX_KeyMaxSize|uniqueflag));
    treeno=1;
  }
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
    switch(e=toupper(_getch())) {
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
                     while(!_kbhit()) {
                       printf("\n");
                       if(NextAndShow()) break;
                     }
                     /*Clear key buffer*/
                     while(_kbhit()) _getch();
                     break;

       case 'S'    : Skip(); break;
       case 'F'    : Find(); break;
#ifdef _TEST
       case 'M'    : ShowNodes(1); break;
       case 'N'    : ShowNodes(0); break;
#endif
       case 'A'    : AddKey(); break;
       case 'D'    : DeleteKey(); break;
       case 'X'    : printf("\r \r");
                     goto _ex0;
       default     :
#ifdef _TEST
         printf("\r(sp)next,Prev,Top,Bot,Find,Skip,List,duMp,Nodes,Add,"
         "Delete,Info,eXit --");
#else
         printf("\r(sp)next,Prev,Top,Bot,Find,Skip,List,Add,"
         "Delete,Info,eXit --");
#endif
    }
  }

_ex0:
  trx_opened=FALSE;
  FCHK(trx_Close(trx));
  exit(0);
}

/*DBFV.C --*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <conio.h>
#include <ctype.h>
#include <dos_io.h>
#include <dbf_file.h>
#include <trx_str.h>

TRXFCN_S trx_FileDate(LPSTR v,UINT days);

#define dos_KeyPressed() _kbhit()

static char hdr[]="DBFV for WIN32 v2.01 Copyright 1997/2006 by David McKenzie";

/*The library is assumed compiled with _DBF_TEST defined --*/
#define _DBF_TEST

#define MAX_NUMFLDS 70
#define MAX_FLDSIZE 256
#define MAX_LINELEN 80
#define MAX_NUMFLDS_STR "70"

static int numbufs=10;
static DBF_NO dbf;
static int numflds,mode,bufsiz;
static char dbfname[MAX_PATH];
static DBF_FLDDEF fld[MAX_NUMFLDS];
static LPBYTE pRec;

static BOOL dbf_opened;
static BOOL bRowFormat=FALSE;

#define FCHK(e) filechk(__LINE__,e)

static apfcn_i dos_InChar(void)
{
  int c;
  //Discard function and arrow keys --
  while((c=_getch())==0 || c==0x0E || (c&0x80)) {
	 while(dos_KeyPressed()) _getch();
  }
  return c;
}

static char *dos_InStr(PBYTE buf,BYTE bufsiz)
{
	//buf[0]=bufsiz-2; strcpy(buf,_cgets(buf)); //SHOULD WORK BUT DOESN'T!

	int len=0;
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

#ifdef _DBF_TEST
static apfcn_v ShowFormatErr(void)
{
  printf("DBF corrupted!");
  //printf("DBF corrupted - LIB fcn line %d.",_dbf_errLine);
}
#endif

static apfcn_v filechk(int line,int e)
{
  if(e) {
    printf("\rAborted - %s: %s (%u).\n",dbfname,dbf_Errstr(e),line);
#ifdef _DBF_TEST
    if(e==DBF_ErrFormat) {
      ShowFormatErr();
      nl();
    }
#endif
    if(dbf_opened) dbf_Close(dbf);
    exit(1);
  }
}

static apfcn_i ShowError(int e,int dir)
{
  printf("\r");
  if(e==DBF_ErrEOF) {
    if(!dbf_NumRecs(dbf)) dir=0;
    if(!dir) printf("<DATABASE IS EMPTY>");
    else printf(dir>0?"<END OF FILE>":"<TOP OF FILE>");
  }
#ifdef _DBF_TEST
  else if(e==DBF_ErrFormat) ShowFormatErr();
#endif
  else printf("Error %d: %s.",e,dbf_Errstr(e));
  return e;
}

static UINT getfldstr(char *buf,DBF_pFLDDEF pf)
{
	PBYTE p=pRec+pf->F_Off;
	UINT i,len=pf->F_Len;
	BYTE longval[4];

	if(pf->F_Nam[0]==DBF_BIN_PREFIX) {
	  switch(pf->F_Nam[1]) {
		case 'I' : return sprintf(buf,"%6d",(int)*(short *)p);
		case 'W' : return sprintf(buf,"%5u",(UINT)*(WORD *)p);
		case 'F' : return sprintf(buf,"%14e",*(float *)p);
		case 'D' : return sprintf(buf,"%14e",*(double *)p);
		case 'U' : return sprintf(buf,"%10u",*(DWORD *)p);
		case 'L' : return sprintf(buf,"%11d",*(long *)p);

		case 'S' : if(pf->F_Len==4) return sprintf(buf,"%07lu",*(DWORD *)p); 
        		   longval[3]=0; longval[2]=p[0]; longval[1]=p[1]; longval[0]=p[2];
                   return sprintf(buf,"%07lu",*(DWORD *)longval);

		case '$' : i=sprintf(buf,"%12ld ",*(DWORD *)p);
		           buf[i-1]=buf[i-2];
		           if((buf[i-2]=buf[i-3])==' ') buf[i-2]='0';
		           buf[i-3]='.';
		           if(buf[i-4]==' ') buf[i-4]='0';
				   return i;

		case '2' : i=(int)*(WORD *)p;
				   p=trx_FileDate((LPSTR)longval,i)+1;
		case 'K' : return sprintf(buf,"%4u/%02u/%02u",p[0]+1900-' ',p[1]-' ',p[2]-' ');
	  }
	}
    else if(dbf_Type(dbf)==DBF2_FILEID) {
      switch(pf->F_Typ) {
        case 'C' : if(!strcmp(pf->F_Nam,"SKUK")) {
        			 longval[3]=0; longval[2]=p[0]; longval[1]=p[1]; longval[0]=p[2];
                     return sprintf(buf,"%07lu",*(DWORD *)longval);
                   }
                   break;
        
        case 'I' : return sprintf(buf,"%6d",*(short *)p);
                     
        case 'D' : return sprintf(buf,"%4u/%02u/%02u",p[0]+1900-' ',p[1]-' ',p[2]-' ');
               	   
	    case '$':  if(!strcmp(pf->F_Nam,"ISKU")) return sprintf(buf,"%07lu",*(DWORD *)p);
		           i=sprintf(buf,"%12ld ",*(DWORD *)p);
		           buf[i-1]=buf[i-2];
		           if((buf[i-2]=buf[i-3])==' ') buf[i-2]='0';
		           buf[i-3]='.';
		           if(buf[i-4]==' ') buf[i-4]='0';
				   return i;
      } /*switch*/

    } /*else*/

	if(len>=MAX_FLDSIZE) len=MAX_FLDSIZE-1;
	for(i=0;i<len;i++) {
	  if(*p<(BYTE)' '||*p>0x7F) *p='.';
	  *buf++=*p++;
	}
	*buf=0;
	return len;
}

static apfcn_i ShowRec(void)
{
  int i;
  UINT maxlen;
  DBF_pFLDDEF pf=fld;
  DWORD n;
  BYTE buf[MAX_FLDSIZE];

  if((n=dbf_Position(dbf))==0L) return dbf_errno;
  i=*pRec;
  if(i<' ' || i>'~') i='?';
  if(bRowFormat) {
	  maxlen=printf("\r%10lu%c|",n,i);
	  for(i=numflds;i;i--,pf++) {
		if(maxlen+getfldstr(buf,pf)>=MAX_LINELEN) return 0;
		maxlen+=printf("%s|",buf);
	  }
  }
  else {
	  printf("\r%10lu%c-------------\n",n,i);
	  for(i=numflds;i;i--,pf++) {
		getfldstr(buf,pf);
		printf("%10s |%s|\n",pf->F_Nam,buf);
	  }
  }
  return 0;
}

static apfcn_i NextAndShow(void)
{
  int e;

  if((e=dbf_Next(dbf))!=0) return ShowError(e,1);
  return ShowRec();
}

static apfcn_i PrevAndShow(void)
{
  int e;

  if((e=dbf_Prev(dbf))!=0) return ShowError(e,-1);
  return ShowRec();
}

static void ViewStats(void)
{
  int i;
  DBF_HDRDATE d;
  TRX_CSH csh;

  dbf_GetHdrDate(dbf,&d);

  printf("\rDatabase %s: %lu records of size %u - Updated %02d/%02d/%04d\n",
    dbf_FileName(dbf),
    dbf_NumRecs(dbf),
    i=dbf_SizRec(dbf),
    d.Month,
    d.Day,
    d.Year+1900);

  if((csh=dbf_Cache(dbf))!=0)
    printf("Header size: %lu Cache buffers: %u of size %u (%u recs)\n",
      dbf_SizHdr(dbf),csh_NumBuffers(csh),csh_SizBuffer(csh),csh_SizBuffer(csh)/i);
}

static apfcn_v ViewStruct(void)
{
  int i;
  DBF_pFLDDEF pf;

  if((pf=dbf_FldDef(dbf,1))!=NULL) {
    printf("\rFld Name  Typ Len Dec (dBase-I%s)\n"
		"=====================\n",(dbf_Type(dbf)==DBF2_FILEID)?"I":"II");
    for(i=0;i<numflds;i++,pf++)
      printf("%-10s %c%5u%4u  :%d\n",pf->F_Nam,pf->F_Typ,pf->F_Len,pf->F_Dec,i+1);
  }
}

static apfcn_v delete_record(void)
{
	char *p=(char *)dbf_RecPtr(dbf);
	int e;

	if(mode&DBF_ReadOnly) {
		printf("\rFile open mode is read only! Use option \"-D\".\n");
		return;
	}

	printf("\r%selete record? [Y/N] ",(*p=='*')?"Und":"D");
	if((e=toupper(dos_InChar()))!='Y') e='N';
	printf("%c\n",e);
	if(e=='Y') {
		*p=(*p=='*')?' ':'*';
		if((e=dbf_Mark(dbf)) || (e=dbf_Go(dbf,0))) {
			if(e==DBF_ErrRecChanged) {
				printf("Cancelled -- Another user has revised the record!");
				e=0;
			}
			else ShowError(e,0);
			printf("\n");
		}
	}
	else e=0;
	if(!e) ShowRec();
}

static apfcn_l get_long(char *s)
{
   char buf[22];
   printf("\r%s: ",s);
   return atol(dos_InStr(buf,22));
}

static apfcn_v ShowRangeError(int e,PSTR s)
{
    if(e!=DBF_ErrEOF) ShowError(e,0);
    else printf("\n%s out of range. Record total: %lu",s,dbf_NumRecs(dbf));
}

static apfcn_v Skip(void)
{
  int e;
  long len;

  if((len=get_long("Distance to skip [+/-]"))==0L) return;
  nl();
  if((e=dbf_Skip(dbf,len))==0) ShowRec();
  else ShowRangeError(e,"Distance");
}

static apfcn_v GotoRec(void)
{
  int e;
  DWORD recno;

  if((recno=(DWORD)get_long("Record number"))==0L) return;
  if((e=dbf_Go(dbf,recno))==0) ShowRec();
  else ShowRangeError(e,"Record");
}

static apfcn_v Find(void)
{
  int e;
  static char last_target[50];
  static int last_len=0;
  static UINT nfld,last_fld=1;
  char buf[50];
  PSTR fldptr;

  _snprintf(buf,50,"Search fld 1-%u(%u)",dbf_NumFlds(dbf),last_fld);
  if((nfld=get_long(buf))>0 && nfld<=dbf_NumFlds(dbf)) {
	 last_fld=nfld;
	 *last_target=0;
	 last_len=0;
  }
  printf("\rField: %u - Target prefix",last_fld);
  if(*last_target) printf(" (RET for %s): ",last_target);
  else printf(": ");
  e=strlen(dos_InStr(buf,50));
  if(*buf=='\n') {
    if(!last_len) return;
  }
  else {
    if(buf[e-1]=='\n') e--;
    while(e && buf[e-1]==' ') e--;
    buf[e]=0;
    if(!e) return;
	//Exchange '_' with spaces --
    if((UINT)e>dbf_FldLen(dbf,last_fld)) {
      printf("\nPrefix longer than field length.");
      return;
    }
	strcpy(last_target,xchgchr(buf,'_',' '));
	last_len=e;
  }
  nl();
  fldptr=(PSTR)dbf_FldPtr(dbf,last_fld);
  e=dbf_Position(dbf)?dbf_Next(dbf):dbf_First(dbf);
  while(!e) {
    if((e=memcmp(last_target,fldptr,last_len))==0) {
      ShowRec();
      return;
    }
    e=dbf_Next(dbf);
  }
  ShowError(e,1);
  return;
}

/*==============================================================*/
void main(int argc,PSTR *argv)
{
  int e;
  PSTR p;
  
  printf("\n%s\n",hdr);

  mode=DBF_ReadOnly;

  while(--argc) {
    _strupr(p=*++argv);
    if(*p=='-') {
      switch(p[1]) {
        case 'D' : mode&=~DBF_ReadOnly; mode|=DBF_AutoFlush; break;
        case 'B' : bufsiz=atoi(p+2); break;
        case 'N' : numbufs=atoi(p+2); break;
		case 'S' : mode|=DBF_Share; break;
		case 'F' : mode|=DBF_ForceOpen; break;
		default  : printf("\nInvalid option: %s\n",p);
			       *dbfname=0;
				   goto _help;
      }
    }
    else if(*dbfname) {
      *dbfname=0;
      break;
    }
    else strcpy(dbfname,dos_FullPath(p,"DBF"));
  }

_help:

  if(!*dbfname) {
    printf("\n"
    "Usage: DBFV <dbfname>[.DBF] [-N<buffers>] [-B<bufsiz>]\n"
	"            [-Share] [-Force] [-DeleteEnable]\n"
    "\n"
    "This program opens a dBase-III database, allowing an interactive\n"
    "scan of structure and data. Use -N to specify number of cache\n"
    "buffers (default 10) and -B to specify buffer size (defaults to\n"
    "smallest size >= 512 holding a whole number of records). Use -F\n"
	"to force open a database which is \"not in closed form\". Use -S\n"
	"to open database in shared mode. Use -D to enable Delete record\n"
	"toggle.\n\n"
    );
    exit(0);
  }

  FCHK(dbf_Open(&dbf,dbfname,mode));
  dbf_opened=TRUE;

  if((numflds=dbf_NumFlds(dbf))>MAX_NUMFLDS) {
    printf("\rAborted - %s: Program supports only " MAX_NUMFLDS_STR 
		   " < %u fields.\n",dbfname,numflds);
	dbf_Close(dbf);
	exit(1);
  }

  FCHK(dbf_GetFldDef(dbf,fld,1,numflds));
  pRec=(LPBYTE)dbf_RecPtr(dbf);

  if(numbufs && !(mode&DBF_Share)) FCHK(dbf_AllocCache(dbf,bufsiz,numbufs,FALSE));

  ViewStats();

  if((e=dbf_First(dbf))!=0) ShowError(e,0);
  else ShowRec();

  while(TRUE) {
    printf("\n:");
	e=toupper(dos_InChar());
    switch(e) {
	   case 'U'    :
	   case 'D'	   : delete_record(); break;

       case 'H'    : ViewStruct(); break;

       case 'I'    : ViewStats(); break;

       case 'T'    : if((e=dbf_First(dbf))!=0) {
                       ShowError(e,0);
                       break;
                     }
                     printf("\r<TOP OF FILE>\n");
                     ShowRec();
                     break;

       case 'G'    : GotoRec(); break;
       case 'F'    : Find(); break;

       case 'P'    : PrevAndShow(); break;

       case 'B'    : if((e=dbf_Last(dbf))!=0) {
                       ShowError(e,0);
                       break;
                     }
                     ShowRec();
					 if(bRowFormat) nl();
                     printf("<END OF FILE>");
                     break;
	   case 'R'    : bRowFormat=!bRowFormat;
					 ShowRec();
					 break;
       case ' '    : NextAndShow();
					 break;
       case 'L'    : while(!dos_KeyPressed()) {
						if(NextAndShow()) break;
						if(bRowFormat) nl();
					 }
                     /*Clear key buffer*/
                     while(dos_KeyPressed()) dos_InChar();
                     break;

       case 'S'    : Skip(); break;
       case 'X'    : printf("\r ");
                     goto _ex0;
       default     :
        printf("\r(sp)next,Prev,Top,Bot,Skip,Go,Find,List,Hdr,Info,Rows,%seXit --",
			(mode&DBF_ReadOnly)?"":"Del,");
    }
  }

_ex0:
  dbf_opened=FALSE;
  FCHK(dbf_Close(dbf));
  exit(0);
}

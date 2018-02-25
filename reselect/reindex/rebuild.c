#include <stdlib.h>
#include <conio.h>
#include <process.h>
#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <trx_str.h>
#include <trx_file.h>
#include <dbf_file.h>

#undef _USE_FILTER //span-related bug in filter.exe (see filter_bug.txt)

//Functions in indexa.c --

extern char filename[128];
extern UINT logtotal;
extern FILE *fplog;

BOOL pfxcmp(LPCSTR p, LPCSTR pfx);
BOOL is_xspace(byte i);
nptr skip_space_types(nptr p);
void index_authors(int trxno,UINT refno,char *pRef);
apfcn_v parse_keywords(nptr p0);
apfcn_n skip_tag(nptr p);
nptr elim_all_st(nptr p0,int n);
nptr elim_all_ins(nptr p0);
nptr elim_all_ole(nptr p,BOOL bNote,DWORD *uLinksOLE);
nptr elim_all_spans(nptr p);
apfcn_v logmsg(LPSTR p, LPCSTR s);

#define RMARGIN 72
#define LEN_REFIDX (16*1024)
static WORD refidx[LEN_REFIDX];

static char hdr[]="REBUILD v3.0\nBibliographic Database Compilation (c) 2015, D. McKenzie\n";

#define MAX_FNAMES 50
#define MAX_KWADDS 500
#define MAX_KWNAMES 2000
#define SIZ_NAMEBUF (48*1024)
#define MAXPATH 128
#define SIZ_LINEBUF 2048
#define SIZ_REFBUF (48*1024)
#define EXPLICIT_SPC 0xA0
#define REF_MASK 0xC0000000
#define COLON_MASK 0x80000000
#define SUFFIX_MASK 0x40000000

//NOTE: Define these the same in resellib.cpp!
#define TRX_KW_TREE (trx)
#define TRX_AUTHOR_TREE (trx+1)
#define TRX_MACRO_TREE (trx+2)

#ifdef USE_MEMMOVE
#define STRCPY_LEFT(dst,src) memmove(dst,src,strlen(src)+1)
#else
#define STRCPY_LEFT(dst,src) strcpy(dst,src)
#endif

static DBF_FLDDEF dbfFlddef = 
 {"",'C',3,0,0};  //offset into htx file

#pragma pack(1)
typedef struct {
	short date;
	WORD refno;
} DATEKEY;

struct {
	UINT offset;
} dbfRec={0};

struct {
	UINT keyid;
} dbkRec={0};
#pragma pack()

static FILE *fp,*fphtx;
static char *kwname[MAX_KWNAMES];
static UINT num_fnames,num_kwadds,linecnt,refcnt,reftotal,nameidx,num_kwnames;
static byte linebuf[SIZ_LINEBUF];
static byte refbuf[SIZ_REFBUF];
static byte cfgname[MAXPATH],fspec[MAXPATH];
static char *pkw;
static char **kwadd[MAX_KWADDS];
static char *fname[MAX_FNAMES];
static char namebuf[SIZ_NAMEBUF];
static UINT inamebuf;
static UINT kw_parsed;

static UINT num_datekeys,siz_datekeys,uLinksOLE;
static DATEKEY comp_datekey;
static DATEKEY *datekeys;

static int trx,dbf,dbk;
static BOOL bErrRefNo,nColonKeys,nColonRefs,bBold,bRefBold;
static BOOL bIndexClass,bBibClass;
static char titlebuf[SIZ_LINEBUF];
static char *title="Bibliographic Database";

static apfcn_n fixspec(byte *name,byte *ext)
{
	strcpy(fspec,name);
	strcat(fspec,".");
	strcat(fspec,ext);
	return _strlwr(fspec);
}

static acfcn_v eprintf(nptr format,...)
{
  va_list marker; 
  va_start(marker,format);
  vfprintf(stderr,format,marker);
  va_end(marker);
}

static void pause(nptr p)
{
	  while(_kbhit()) _getch();
	  eprintf("\nPress any key %s...",p);
	  do _getch(); while(_kbhit());
	  putc('\r',stderr);
}

acfcn_v abortmsg(nptr format,...)
{
  va_list marker;
  va_start(marker,format);
  
  eprintf("\nAborted - ");
  vfprintf(stderr,format,marker);
  eprintf(".\n");
  if(fp) fclose(fp);
  if(fplog) fclose(fplog);
  if(fphtx) {
	 fclose(fphtx);
	 _unlink(fixspec(cfgname,"htx"));
  }
  va_end(marker);
  free(datekeys);
  if(trx) trx_CloseDel(trx);
  if(dbf) dbf_CloseDel(dbf);
  if(dbk) dbf_CloseDel(dbk);
  pause("to exit");
  exit(1);
}

apfcn_v openlog(nptr hdr)
{
	int i;

	byte logname[128];

	if(!fplog) {
		trx_Stcext(logname, cfgname, "LOG", MAXPATH);
		if((fplog=fopen(logname, "wt"))==NULL) abortmsg("Cannot open %s", logname);
	}
	else putc('\n', fplog);

	i=fprintf(fplog,"%s - %s",cfgname,hdr);
	putc('\n',fplog);
	while(i--) putc('=',fplog);
	putc('\n',fplog);
}

static void abort_format(nptr msg)
{
  abortmsg("File %s, line %d: Bad format: %s",fname[nameidx],linecnt,msg);
}

static void elim_outerbold(nptr p0)
{
	int len,npfx;
	nptr p;

	bBold=FALSE;

	if(!pfxcmp(p0,"<b")) {
		p=skip_tag(p0);
		if(*(p-1)!='>') abort_format("\">\" expected following \"<b\"");
		npfx=p-p0;
		len=1;
		for(;*p;p++) {
			if(*p=='<') {
				if(!pfxcmp(p,"</b>")) {
					if(!pfxcmp(p+4,"<b>")) {
						STRCPY_LEFT(p,p+7);
						p--;
					}
					else {
						if(!--len) break;
						p+=3;
					}
				}
				else if(!pfxcmp(p,"<b>")) {
					len++;
					p+=2;
				}
			}
		}
		if(!len) {
			STRCPY_LEFT(p,p+4);
			STRCPY_LEFT(p0,p0+npfx);
			bBold=TRUE;
		}
	}
}

static void repl_chr(nptr p,int c,nptr s)
{
	UINT len=strlen(s);

	while((p=strchr(p,c))) {
		if(len>1) memmove(p+len,p+1,strlen(p+1)+1);
		memcpy(p,s,len);
		p+=len;
	}
}

static BOOL get_cline(void)
{
	nptr p=linebuf;
	int c;

	while((c=getc(fp))!=EOF && c!='\n') {
		if(p<linebuf+SIZ_LINEBUF-1) *p++=c;
		else {
			linecnt++;
			abort_format("Line too long");
		}
	}
	*p=0;
	if(c==EOF && p==linebuf) return FALSE;
	linecnt++;
	return TRUE;
}

static BOOL get_paragraph(void)
{
	UINT linlen,bufcnt=0;
	nptr p;

	while(get_cline()) {
		p=skip_space_types(linebuf);
		if(*p=='<' && p[1]=='p') {
#ifndef _USE_FILTER
			bBibClass=!_memicmp(p, "<p class=Bibliography", 21) || !_memicmp(p, "<p class=MsoBibliography", 24);
			bIndexClass=!bBibClass && !_memicmp(p, "<p class=index", 14);
#endif
			//skip entire paragraph prefix tag --
			p=skip_tag(p);
			while(!p || !*p) {
				if(!get_cline()) abort_format("Paragraph truncated");
				if(p=strchr(linebuf,'>')) p++;
			}
			do {
			  linlen=strlen(p)+1; //include NULL
			  if(bufcnt+linlen>=SIZ_REFBUF) abort_format("Paragraph too large");
			  if(bufcnt) refbuf[bufcnt++]=' ';
			  p=strstr(memcpy(refbuf+bufcnt,p,linlen),"</p>");
			  if(p) {
				  if(strlen(p)!=4) logmsg(p,"Chars following </p> ignored");
				  *p=0;
				  elim_outerbold(refbuf); /*Sets bBold TRUE or FALSE*/
				  repl_chr(refbuf,0xBE,"&mdash;");
				  return TRUE;
			  }
			  bufcnt+=linlen-1;
			}
			while(get_cline() && (p=skip_space_types(linebuf)));
			abort_format("Paragraph truncated");
		}
	}
	return FALSE;
}

static void elim_lspc(nptr p0)
{
	nptr p=p0;
	while(is_xspace(*p)) p++;
	if(p>p0) memmove(p0,p,strlen(p)+1);
}

static int InsertMacroKey(byte *pKey)
{
	byte keybuf[256];
	UINT len=*pKey;

	memcpy(keybuf+1,pKey+1,len);
	keybuf[++len]=0;
	memcpy(keybuf+len+1,pkw+1,*(byte *)pkw);
	*keybuf=(len+=*pkw);
	if(keybuf[len]=='.') keybuf[len]=0;
	return trx_InsertKey(TRX_MACRO_TREE,0,keybuf);
}

static void ClearMacroNames(void)
{
	int c;
	char *p;

	for(c=0;c<(int)num_kwnames;c++) {
		if(p=kwname[c]) (*p)&=0x7F;
	}
}

static void IndexMacroKeys(void)
{
	UINT i,e;
	char **pkwname;
 
    for(i=0;i<num_kwadds;i++) {
		pkwname=kwadd[i];
		pkw=*pkwname;
        while(*++pkwname) {
			(**pkwname)&=0x7F;
			if((e=InsertMacroKey(*pkwname)))
				abortmsg("Error building macro index (%u)",e);
		}
	}
}

static int CALLBACK compare_datekeys(DATEKEY dk)
{
	return dk.date-comp_datekey.date;
}

static int CALLBACK compare_kw_prefix(char **pkwadd)
{
	byte *p=(byte *)(*pkwadd);
	UINT lenp=*p;
	int e;

	//If (p[*p] and p is a prefix of pkw) OR
	  //(p[*p]==0 and *p-1==*pkw and !memcmp(p+1,pkw+1,*pkw)) return 0;
	if(p[lenp]) {
		if(lenp<=*(byte *)pkw) e=memcmp(pkw+1,p+1,lenp);
		else if(!(e=memcmp(pkw+1,p+1,*pkw))) e=-1;
	}
	else {
		lenp--;
		e=memcmp(pkw+1,p+1,(*(byte *)pkw<lenp)?*pkw:lenp);
		if(!e) {
			if(*(byte *)pkw<lenp) e=-1;
			else e=*(byte *)pkw>lenp;
		}
	}
	return e;
}

static int CALLBACK compare_kw(char **pkwadd)
{
	return trx_Strcmpp(pkw,*pkwadd);
}

static int InsertKey(DWORD *pRec,byte *pKey)
{
	int e;
	char ***pp;
	char **pkwname;
	DWORD rec;
 
    if((e=trx_InsertKey(TRX_KW_TREE,pRec,pKey)) || !num_kwadds) return e;

	pkw=pKey;
	trx_Bininit((DWORD *)kwadd,(DWORD *)&num_kwadds,(TRXFCN_IF)compare_kw_prefix);
	pp=(char ***)trx_Blookup(TRX_DUP_IGNORE);
	trx_Bininit((DWORD *)datekeys,(DWORD *)&num_datekeys,(TRXFCN_IF)compare_datekeys);

	if(pp) {
		pkwname=*pp;
		//Left-side key must be a prefix of the key just inserted --
		if(memcmp((*pkwname)+1,pkw+1,(*pkwname)[0])) abortmsg("Unexpected string compare error");
		rec=((*pRec) & ~REF_MASK);
        while(*++pkwname) {
			if((**pkwname)&0x80) continue;
			if((e=trx_InsertKey(TRX_KW_TREE,&rec,*pkwname))) return e;
			(**pkwname)|=0x80;
			dbkRec.keyid++;
		}
	}
	return 0;
}

static UINT add_colon_key(nptr key)
{
	DWORD ref_mask;
	nptr p;
	UINT len;

	if((p=strstr((char *)key+1,": ")) && p[2]) {
		ref_mask=(reftotal|REF_MASK);
		p++;
		len=strlen((char *)p+1);
		*p=(byte)len;
		if(len>255 || InsertKey(&ref_mask,p)) {
			return 0;
		}
		nColonKeys++;
		*p=' ';
		return (reftotal|COLON_MASK);
	}
	return reftotal;
}

apfcn_v IndexDate(int date)
{
	//This will be called once from index_authors() in indexa.c for every reference --
	//Here, in rebuild.c, we're creating a sorted array.

	if(reftotal>0xFFFF) abortmsg("Only 64K references supported in this version");

	if(num_datekeys>=siz_datekeys) {
		siz_datekeys+=5000;
		datekeys=(DATEKEY *)realloc(datekeys,siz_datekeys*sizeof(DATEKEY));
		trx_Bininit((DWORD *)datekeys,(DWORD *)&num_datekeys,(TRXFCN_IF)compare_datekeys);
	}
	if(date<0) date=0;
    comp_datekey.date=date;
	comp_datekey.refno=reftotal;
	trx_Binsert(*(DWORD *)&comp_datekey,TRX_DUP_LAST);
}

apfcn_v AddKeyword(byte *keybuf)
{
	UINT rec;
	if(!(rec=add_colon_key(keybuf)) || InsertKey(&rec,keybuf)) abortmsg("Error writing index");
	dbkRec.keyid++;
	kw_parsed++;
}

static void expand_href(nptr p)
{
	int len;
	char url[512];

	char *ph=strstr(p,"http://");
	if(ph && !((ph-refbuf)>6 && (!memcmp(ph-5,"href=",5)||!memcmp(ph-6,"href=\"",6)))) {
		for(p=ph+7;*p;p++) if(strchr(" ,<>)]&\"\t",*p)) break;
		if(p[-1]=='.') p--;
		len=p-ph-7;

		//<a href="http://..">..</a> -- 16+len+2+len+4 = 22+len+len

		if(len>=512 || strlen(refbuf)+15+len>=SIZ_REFBUF) {
			logmsg(refbuf,"URL skipped -- too long");
			return;
		}
		memcpy(url,ph+7,len); url[len]=0;
		memmove(ph+(len+len+22),p,strlen(p)+1);
		len=sprintf(ph,"<a href=\"http://%s\">%s</a",url,url);
		ph[len]='>';
		len=0;
	}
}

static BOOL bad_format(LPCSTR p)
{
    char buf[80]={"Paragraph skipped: "};
	strcpy(buf+19,p);
	logmsg(refbuf, p);
	return FALSE;
}

static BOOL put_ref_paragraph(void)
{
 	int len;
	nptr p=refbuf;
	UINT refno=0;

	p=skip_space_types(p);

	while(isdigit(*p)) {
		if(!refno) refno=(UINT)atoi(p);
		p++;
	}

	if(*p++!='.' || *p++!=' ' || !*p) {
		if(refcnt) {
			logmsg(refbuf, "Error: Paragraph skipped - \". <author>\" must follow ref number");
		}
		return FALSE;
	}
    if(refno!=reftotal+1 && !bErrRefNo) {
		logmsg(refbuf,"Reference count missmatch");
		bErrRefNo=TRUE;
	}

	index_authors(TRX_AUTHOR_TREE,++reftotal,p);

	expand_href(p);
	len=strlen(p);

	//if((len=fprintf(fphtx,p))<0)
	if(1!=fwrite(p,len,1,fphtx))
		abortmsg("Error writing %s.HTX",cfgname);

	dbfRec.offset+=len;
	return TRUE;
}

static void db_append(void)
{
	DWORD rec=(dbfRec.offset|(bRefBold<<31));

	if(dbf_AppendRec(dbf,&rec) ||
		dbf_AppendRec(dbk,&dbkRec)) abortmsg("Error appending records to %s.DBK",cfgname);
}

static apfcn_v scan_file(void)
{
  BOOL bBreakSeen,bRefSeen;
  nptr p0,p;

  eprintf("\n%-25s - References:     0",fname[nameidx]);

  linecnt=refcnt=uLinksOLE=0;
  bBreakSeen=bRefSeen=FALSE;

  while(get_paragraph()) {

	  if(!(p=elim_all_ole(refbuf,1, &uLinksOLE)))
		  abort_format("name tag doesn't terminate");

      if(uLinksOLE) abort_format("must run REINDEX first");

	  if(!(p=elim_all_ins(p)))
		abort_format("ins tag doesn't terminate");

	  if(!(p=elim_all_st(p,1)) || !(p=elim_all_st(p,2)))
		abort_format("st tag doesn't terminate");

	  if((p0=strstr(p,"<st")) && isdigit(p0[3]))
		abort_format("unexpected st_: tag");

	  if(!(p=elim_all_spans(p)))
		  abort_format("span tag doesn't terminate");

	  p=skip_space_types(p);

	  if(p>refbuf) strcpy(refbuf,p);

	  if(!*refbuf) {
		  if(bRefSeen) {
			  dbkRec.keyid=0;
			  db_append();
			  bRefSeen=FALSE;
		  }
		  bBreakSeen=TRUE;
		  continue;
	  }

	  //refbuf now has reference or keywords --
	  if(bBreakSeen) {
		#ifndef _USE_FILTER
	    if(!bBibClass) abort_format("Biliography class expected - run Reindex first");
		#endif
	    if(put_ref_paragraph()) {
			eprintf("\b\b\b\b\b%5u",++refcnt);
			bRefBold=bBold;
			bRefSeen=TRUE;
		}
	    bBreakSeen=FALSE;
	  }
	  else if(bRefSeen) {
		//Should be keywords --
#ifndef _USE_FILTER
		if(!bIndexClass) abort_format("Index class expected - run Reindex first");
#endif
		if(num_kwnames) ClearMacroNames();
		dbkRec.keyid=0;
		parse_keywords(refbuf);
		db_append();
		bBreakSeen=TRUE;
		bRefSeen=FALSE;
	  }
  }

  if(bRefSeen) {
	  //Last reference is without keywords --
	  dbkRec.keyid=0;
	  db_append();
  }
}

static apfcn_v trim_twsp(nptr ln)
{
	nptr p;
    for(p=ln+strlen(ln)-1;p>=ln && isspace(*p);p--);
    *(p+1)=0;
}

static apfcn_n skip_spc(char *p)
{
	while(isspace(*p)) p++;
    return p;
}

static char *alloc_name(char *p)
{
	int len;

	trim_twsp(p=skip_spc(p));
	len=strlen(p);
	if(!len || inamebuf+len>=SIZ_NAMEBUF) return 0;
	p=trx_Stccp(namebuf+inamebuf,p);
	inamebuf+=len+1;
	return p;
}

static apfcn_v get_kwadd(char *nam)
{
	char *p=strchr(_strupr(nam),':');

	if(!p) abortmsg("Keyword needs colon terminator - CFG line %u",linecnt);
	*p++=0;
	if(num_kwadds>=MAX_KWADDS || num_kwnames>=MAX_KWNAMES-1 || !(pkw=alloc_name(nam))) {
		goto _err;
	}

	kwname[num_kwnames]=pkw;

	trx_Binsert((DWORD)&kwname[num_kwnames],TRX_DUP_NONE);
	if(trx_binMatch) return;
	num_kwnames++;

	while(TRUE) {
		nam=skip_spc(p);
		if(!*nam) break;
		p=strchr(nam,';');
		if(!p) p=nam+strlen(nam);
		else *p++=0;
		trim_twsp(nam);
		if(num_kwnames>=MAX_KWNAMES-1 || !(kwname[num_kwnames++]=alloc_name(nam))) {
			goto _err;
		}
	}
	kwname[num_kwnames++]=0;
	return;

_err:
	abortmsg("Too many keywords are bad definition - CFG line %u",linecnt);
}

static apfcn_v elim_kwdups(int mode)
{
	int i;
	char **pp,**ppi;
	char *p,*pi;
	BOOL bMatch;

	pp=kwadd[mode];

	while(p=*++pp) {
		bMatch=FALSE;
		for(i=0;i<mode;i++) {
			ppi=kwadd[i];
			while(pi=*++ppi) {
				if(!trx_Strcmpp(p,pi)) {
					*pp=pi;
					bMatch=TRUE;
					break;
				}
			}
			if(bMatch) break;
		}
	}
}

static apfcn_v get_fnames(void)
{
  char *p;

  UINT mode=0; //0 - start, 1 - file names, 2 - keyword equivalences

  linecnt=0;
  num_fnames=num_kwadds=inamebuf=0;
  trx_Bininit((DWORD *)kwadd,(DWORD *)&num_kwadds,(TRXFCN_IF)compare_kw);

  while(get_cline()) {
	p=skip_spc(linebuf);
	trim_twsp(p);
    if(*p==0 || *p==';') continue;

    if(*p=='.') {
		p++;
		if(!memicmp(p,"DB",2)) {
		    title=strcpy(titlebuf,skip_spc(p+2));
			mode=1;
		}
		else if(!memicmp(p,"KW",2)) {
		    mode=2;
		}
		else abortmsg("Unrecognized CFG file directive - line %u",linecnt);
		continue;
	}
	if(!mode) abortmsg("CFG file error - No .DB directive present");
	if(mode==1) {
		if(trx_Stpnam(p)!=p) abortmsg("Names in CFG file cannot have path prefixes");
		trx_Stcext(fspec,_strupr(p),"htm",MAXPATH);
		if(num_fnames>=MAX_FNAMES || !(fname[num_fnames++]=trx_Stxpc(alloc_name(fspec))))
		  abortmsg("Too many names in CFG file",MAX_FNAMES);
	}
	else get_kwadd(p);
  }
  if(!num_fnames) abortmsg("No names in file CFG file");

  if(num_kwadds) {
	  for(mode=1;mode<num_kwadds;mode++) {
		  if(trx_Strcmpp(*kwadd[mode-1],*kwadd[mode])>=0) break;
		  elim_kwdups(mode);
	  }
	  if(mode<num_kwadds) abortmsg("Error processings keywords in CFG");
  }
}

static apfcn_v abort_index(char *s)
{
	abortmsg("Cannot create index component %s.%s",cfgname,s);
}

static void write_htx_hdr(void)
{
	if((dbfRec.offset=fprintf(fphtx,title))<0) dbfRec.offset=0;
}

static apfcn_v init_database(void)
{
	int e;

	if((e=trx_Create(&trx,cfgname,0,2+(num_kwadds>0),0,1024,4)) ||
		trx_InitTree(TRX_KW_TREE,4,0,TRX_KeyMaxSize) ||
		trx_InitTree(TRX_AUTHOR_TREE,4,0,TRX_KeyMaxSize) ||
		(num_kwadds && trx_InitTree(TRX_MACRO_TREE,0,0,TRX_KeyMaxSize)) ||
		trx_AllocCache(trx,128,TRUE)) {
		abort_index("TRX");
	}

	trx_SetExact(TRX_AUTHOR_TREE, TRUE);

    if((fphtx=fopen(fixspec(cfgname,"htx"),"wt"))==NULL)
	  abortmsg("Cannot create file %s",fspec);

	write_htx_hdr();
 
	if(dbf_Create(&dbf,fixspec(cfgname,"dbk"),0,1,&dbfFlddef) ||
		dbf_AllocCache(dbf,1024,32,TRUE) ||	dbf_AppendRec(dbf,&dbfRec) ||
		dbf_Create(&dbk,cfgname,0,1,&dbfFlddef))
		abort_index("DBK");
}

static int CALLBACK pckmsg(int typ)
{
    printf(trx_pck_message);
	if(typ==PCK_TREETOTALS || typ==PCK_FILETOTALS) printf("\n");
	return 0;
}

static void condition_dbk(void)
{
	UINT e,rec,keys,refid,keytotal;

	keytotal=reftotal+2;
	for(rec=1;rec<=reftotal;rec++) {
       if((e=dbf_Go(dbk,rec)) || (e=dbf_GetRec(dbk,&keys))) break;
	   //keytotal is position of first key of reference rec
	   if((e=dbf_PutRec(dbk,&keytotal))) break;
	   keytotal+=keys;
	   while(keys--) if((e=dbf_AppendZero(dbk))) break;
	   if(e) break;
	}

	if(!e && !(e=dbf_AppendZero(dbk)) && !(e=dbf_Go(dbk,rec)))
		e=dbf_PutRec(dbk,&keytotal);

	/*Now, dbk[refno] is the record number in dbk[] that is the next
	  one to be initialized with a keyid. After scanning the index,
	  dbk[dbk[refno]] should be reset to contain the keyid of the first
	  key of refno. The last keyid will be dbk[dbk[refno+1]-1].*/

	if(!e) {
		if(trx_NumRecs(TRX_KW_TREE)!=nColonKeys+keytotal-reftotal-2) abortmsg("Unexpected error - bad key counts");
		e=trx_First(TRX_KW_TREE);
	}

	nColonRefs=0;

	while(!e) {

		dbkRec.keyid=trx_GetPosition(TRX_KW_TREE);

		if(!(keys=trx_RunLength(TRX_KW_TREE))) {
			e=trx_errno;
			break;
		}

		do {
			if((e=trx_GetRec(TRX_KW_TREE,&refid,sizeof(refid)))) break;

			if(!(refid&SUFFIX_MASK)) {
				//refid is the refrence number;
				if((e=dbf_Go(dbk,refid&~REF_MASK)) || (e=dbf_GetRec(dbk,&rec))) break;
				rec++;
				if((e=dbf_PutRec(dbk,&rec))) break;
				if(refid&COLON_MASK) {
					dbkRec.keyid|=COLON_MASK;
					nColonRefs++;
				}
				else dbkRec.keyid&=~COLON_MASK; 
				if((e=dbf_Go(dbk,rec-1)) || (e=dbf_PutRec(dbk,&dbkRec))) break;
			}

			e=trx_Next(TRX_KW_TREE);
		}
		while(--keys);

		if(e==TRX_ErrEOF && !keys) {
			e=0;
			break;
		}
	}

    if(nColonKeys!=nColonRefs) abortmsg("Unexpected error - colon key missmatch");

	//Finally, restore positions of keyid's for each reference --
    keytotal=reftotal+2;
	for(rec=1;rec<=reftotal;rec++) {
       if((e=dbf_Go(dbk,rec)) || (e=dbf_GetRec(dbk,&keys))) break;
	   //keys is now position of first key of next reference --
	   if((e=dbf_PutRec(dbk,&keytotal))) break;
	   keytotal=keys;
	}

	if(e) abortmsg("Error processing DBK component - %s",
		(e=dbf_errno)?dbf_Errstr(e):trx_Errstr(e));
}

static void append_datekeys(void)
{
	UINT i;
	for(i=0;i<reftotal;i++) {
		if(dbf_AppendRec(dbf,&datekeys[i])) abortmsg("Error appending datekeys");
	}
}

static void append_dbk(void)
{
	UINT *prec;
	UINT i,imax;

	if(dbf_NumRecs(dbf)!=reftotal+1) goto _err;

	prec=(UINT *)dbf_HdrRes16(dbf);
	*prec=reftotal;
	dbf_MarkHdr(dbf);

	if(!(prec=(UINT *)dbf_RecPtr(dbk))) goto _err;
	imax=dbf_NumRecs(dbk);

	for(i=1;i<=imax;i++) if(dbf_Go(dbk,i) || dbf_AppendRec(dbf,prec)) break;
	if(i<=imax || dbf_NumRecs(dbf)!=1+reftotal+imax) goto _err;
	return;

_err:
	abortmsg("unexp err 1");
}

static int get_first_key(int idx)
{
	DWORD rec;
	return (trx_FillKeyBuffer(idx, -2) || trx_GetRec(idx, &rec, sizeof(DWORD)))?0:rec;
}

static UINT get_next_key(int idx)
{
	DWORD rec;
	return (trx_FillKeyBuffer(idx, 1) || trx_GetRec(idx, &rec, sizeof(DWORD)))?0:rec;
}

static void list_refs(int idx, UINT refcnt)
{
	UINT col;

	col=fprintf(fplog, "\n%s:  %u", refbuf, refidx[--refcnt])-2;
	while(refcnt) {
		col+=fprintf(fplog, ",");
		if(col>=RMARGIN) col=fprintf(fplog, "\n   ")-2;
		col+=fprintf(fplog, " %u", refidx[--refcnt]);
	}
}

static apfcn_v scan_index(int idx)
{
	UINT rec;
	DWORD rlen, rlen_max, refcnt, numkeys;
	BYTE rlen_maxkey[256];

	if(!(numkeys=trx_NumKeys(idx))) return;

	if(idx==TRX_KW_TREE) {
		eprintf("\nWriting keyword list..");
		openlog("KEYWORD INDEX");
	}
	else {
		eprintf("\nWriting author list..");
		openlog("AUTHOR INDEX");
	}

	trx_SetKeyBuffer(idx, linebuf);

	*refbuf=0xFF;
	refbuf[1]=0;
	rlen=rlen_max=refcnt=0;
	rec=get_first_key(idx);

	while(rec) {
		numkeys--;

		linebuf[*linebuf+1]=0;

		if(strcmp(linebuf+1, refbuf)) {
			if(refcnt) list_refs(idx, refcnt);
			refidx[0]=(WORD)rec;
			refcnt=1;
			strcpy(refbuf, linebuf+1);

			if(rlen) break;
			rlen=trx_RunLength(idx)-1;
			if(rlen>rlen_max && strcmp(refbuf, "ANONYMOUS")) {
				rlen_max=rlen;
				strcpy(rlen_maxkey, refbuf);
			}
		}
		else {
			rlen--;
			if(refcnt<LEN_REFIDX) refidx[refcnt++]=(WORD)rec;
		}
		rec=get_next_key(idx);
	}

	if(refcnt) list_refs(idx, refcnt);

	putc('\n', fplog);

	if(rlen) {
		abortmsg("Unexpected index format (runlength=%u)", rlen);
	}

	if(numkeys) abortmsg("Error reading index - code %u", trx_errno);

	eprintf(" Completed.\nMost %s: %s (%u appearances)\n",
		(idx==TRX_KW_TREE)?"frequent keyword":"prolific author", rlen_maxkey, rlen_max+1);

}

#ifdef _USE_FILTER
static void filter_convert()
{
	int i;
	strcpy(trx_Stpext(fspec), ".$$$");
	sprintf(linebuf, "filter -cflmrst %s %s", fname[nameidx], fspec);
	if((i=system(linebuf))>1 || i<0)
		abortmsg("Error executing \"%s\" (Code %d, errno: %d)", linebuf, i, errno);
}
#endif

/*==============================================================*/
void main(int argc,nptr *argv)
{
  nptr p;
  UINT e;
  BOOL blist_kw=TRUE, blist_authors=TRUE;

  eprintf("\n%s",hdr);

  while(--argc) {
    _strupr(p=*++argv);
	if(!strcmp(p, "-K")) {
		blist_kw=FALSE;
		continue;
	}
	if(!strcmp(p, "-A")) {
		blist_authors=FALSE;
		continue;
	}
    if(*cfgname || trx_Stpnam(p)!=p || *trx_Stpext(p)) continue;
	strcpy(cfgname,p);
  }

  if(!*cfgname) {
    eprintf("\n"
	  "Usage: REBUILD [-k] [-a] <cfgname>\n\n"
      "Creates an indexed database from HTML files whose names are listed in\n"
	  "a text file, <cfgname>.CFG. An output text file, <cfgname>.LOG,\n"
	  "containing diagnostic messages will normally also be produced.\n"
	  "See the provided CFG file template for format details. The HTML files\n"
	  "should contain James Reddell's style of bibliographic references,\n"
	  "preconditioned by the REINDEX utility, the latter having been run first\n"
	  "to assign numbers to newly-added references. REINDEX will also have\n"
	  "produced a diagnostic log, <cfgname>.LST. (Note different extension).\n\n"
	
      "Unlike REINDEX, this program creates a database consisting of three files,\n"
	  "<cfgname>.HTX, <cfgname>.TRX, and <cfgname>.DBK, which will serve as\n"
	  "input for the browser program, RESELECT.\n\n"
	  "Use -K and/or -A to omit the keyword or author listing from the log.\n"
	  );
	pause("to exit");
    exit(1);
  }

  datekeys=NULL; //not required since global (array is freed in avortmsg())

  if((fp=fopen(fixspec(cfgname,"cfg"),"rt"))==NULL)
    abortmsg("Cannot open file %s",fspec);

  get_fnames();
  fclose(fp);
  fp=0;

  init_database();

  if(num_kwadds) IndexMacroKeys();

//  trx_Bininit((DWORD *)kwadd,(DWORD *)&num_kwadds,(TRXFCN_IF)compare_kw_prefix);

  num_datekeys=siz_datekeys=0;

  _unlink(fixspec(cfgname,"log"));

  reftotal=logtotal=nameidx=nColonKeys=0;

  for(e=0;e<num_fnames;e++) {
	strcpy(fspec,fname[nameidx]);
#ifdef _USE_FILTER
	filter_convert(); //changes fspec extension, aborts on failure
#endif
    if((fp=fopen(fspec,"rt"))==NULL)
		abortmsg("Cannot open file %s",fspec);
	trx_Stncc(filename, trx_Stpnam(fspec), sizeof(filename));
	*trx_Stpext(filename)=0;
	scan_file();
	fclose(fp);
	fp=0;
#ifdef _USE_FILTER
	if(!bKeepTemp) _unlink(fspec);
#endif
	nameidx++;
  }

  fclose(fphtx);
  fphtx=0;

  e=trx_NumKeys(TRX_KW_TREE);
  eprintf("\n\nFiles: %d  Refs: %u  Authors: %u  Keywords: %u (%u compound)\n\n",
	  num_fnames, reftotal, trx_NumKeys(TRX_AUTHOR_TREE), e, (e-kw_parsed));


  if((e=dbf_NumRecs(dbf))!=reftotal+1) {
	  abortmsg("Unexpected error: Ref count (%u) should be %u",e-1,reftotal);
  }

  if(num_datekeys!=reftotal) abortmsg("Unexpected error: Ref count does not match length of datekeys[]");

  if((e=trx_Pack(&trx,pckmsg))) abortmsg("Error compacting %s.TRX - %s",
	  cfgname,trx_Errstr(e));

  /*Now initialize DBK database --*/

  eprintf("\nConditioning database dbk...");

  condition_dbk();

  //Append dbk file to dbf and store reftotal in dbf header --
  append_dbk();

  //Finally, append datekeys[] to database --
  append_datekeys();
  free(datekeys);

  dbf_CloseDel(dbk);
  dbk=0;

  if(blist_authors) scan_index(TRX_AUTHOR_TREE);
  if(blist_kw) scan_index(TRX_KW_TREE);

  if((e=trx_Close(trx))) {
	  trx=0;
	  abortmsg("Error closing file %s.TRX (code %u)",cfgname,e);
  }
  trx=0;

  if(fplog) {
	  fclose(fplog);
	  eprintf("\nNOTE: There were %u questionable items. See %s.LOG.\n",logtotal,cfgname);
  }

  if((e=dbf_Close(dbf))) {
	  dbf=0;
	  abortmsg("Error closing file %s.DBK (code %u)",cfgname,e);
  }
  dbf=0;

  eprintf("\nDatabase generated - 3 files: %s.htx/trx/dbk.\n",cfgname);

  pause("to exit");
  exit(0);
}

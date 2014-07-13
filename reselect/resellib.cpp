// resellib.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "resellib.h"

#define COLON_MASK 0x80000000
#define REF_MASK 0xC0000000
#define DBF_BUFSIZ 512
#define DBF_NUMBUFS 32
#define TRX_NUMBUFS 128

//NOTE: Define these the same in rebuild.c!
#define TRX_KW_TREE (trx)
#define TRX_AUTHOR_TREE (trx+1)
#define TRX_MACRO_TREE (trx+2)

#define MAXPATH 128
#define SIZ_LINEBUF 1024
#define SIZ_REFBUF (32*1024)

#ifdef USE_MEMMOVE
#define STRCPY_LEFT(dst,src) memmove(dst,src,strlen(src)+1)
#else
#define STRCPY_LEFT(dst,src) strcpy(dst,src)
#endif

typedef struct sTOKEN TOKEN;

struct sTOKEN {
	UINT t_type;
	void *t_ptr;
	TOKEN *t_next;
};

#pragma pack(1)
typedef struct {
	short date;
	WORD refno;
} DATEKEY;

struct {
	UINT keyid;
} dbkRec;
#pragma pack()

enum e_toktyp {TOK_KEYPREFIX,TOK_KEYEXACT,TOK_AND,TOK_OR,
TOK_NOT,TOK_PARENOPEN,TOK_PARENCLOSE,TOK_REFSET};

#define MAX_TOKENS 100
TOKEN token[MAX_TOKENS];
static UINT ntokens,nops,op[MAX_TOKENS];

#define SIZ_KWBUF (16*1024)
static char kwbuf[SIZ_KWBUF];
#define MAX_SHOWKW 500
static char *showkw[MAX_SHOWKW];
static UINT ikw,nshowkw;
static byte *pkw;

#pragma pack(1)
typedef struct {
	DWORD count;
	byte map[1];
} REFSET;

struct {
	byte delflg;
	UINT offset;
} dbfFld={0x20,0};
#pragma pack()

//RESELLIB_API

static int resel_Flags=0;

static FILE *fphtx,*fphtm;
static int tree_id;
static UINT nkeywords,nkeytokens,reftotal,dbk_off,siz_refset,siz_refmap_dw;
static char dbfname[MAXPATH],htmname[MAXPATH],fspec[MAXPATH];

REFSET *pRefset;

#define MAX_KEYWORDS 20
#define MAX_KEYTOKENS 30
#define SIZ_KEYWORD 128
#define SIZ_KEYPHRASE 256
static char keyword[MAX_KEYWORDS][SIZ_KEYWORD];
static char keyphrase[SIZ_KEYPHRASE];
static char rawphrase[SIZ_KEYPHRASE];

static char rawauthors[SIZ_KEYPHRASE];

static BOOL tokquote[MAX_KEYTOKENS];

static UINT trx,dbk,len_keyphrase;
static BOOL bComplete,bBlue,bSort,bOrAuthors;
static UINT bShowKeywords,bShowNumbers;

static char titlebuf[SIZ_LINEBUF];
static byte refbuf[SIZ_REFBUF];
static char *authorphrase;
static char *title="Bibliographic Database";

//=========================================================================
#ifdef RESELLIB_DLL
BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
    }
    return TRUE;
}
#endif

/*
// This is an example of an exported variable
RESELLIB_API int nResellib=0;

// This is an example of an exported function.
RESELLIB_API int fnResellib(void)
{
	return 42;
}

// This is the constructor of a class that has been exported.
// see resellib.h for the class definition
CResellib::CResellib()
{ 
	return; 
}
*/
//=========================================================================
static char _re_errstr[256];

#define ERR_LIMIT (sizeof(_re_errstr)/sizeof(char *))

RESELLIB_API void re_SetFlags(int flags)
{
	resel_Flags=flags;
}

RESELLIB_API char *re_Errstr(void)
{
  return _re_errstr;
}

static char *fixspec(char *name,char *ext)
{
	strcpy(fspec,name);
	strcat(fspec,".");
	strcat(fspec,ext);
	return fspec;
}

static int finish(char *format,...)
{
  UINT len;
  va_list marker;
  va_start(marker,format);

  len=vsprintf(_re_errstr,format,marker);
  _re_errstr[len++]='.';
  _re_errstr[len]=0;
  va_end(marker);

  if(fphtx) fclose(fphtx);
  if(trx) trx_Close(trx);
  if(dbk) dbf_Close(dbk);
  return 1;
}

static int open_database(void)
{
	int e;

	dbf_errno=trx_errno=0;

	if(dbf_Open(&dbk,fixspec(dbfname,"dbk"),DBF_ReadOnly) ||
		((reftotal=*(UINT *)dbf_HdrRes16(dbk))<1 && (dbf_errno=DBF_ErrEOF))) {
		return dbf_errno;
	}

	dbk_off=reftotal+1;

	if((e=dbf_AllocCache(dbk,DBF_BUFSIZ,DBF_NUMBUFS,TRUE)) ||
		(e=trx_Open(&trx,dbfname,TRX_ReadOnly))) {
		dbf_Close(dbk);
		return e;
	}

	if((e=trx_AllocCache(trx,TRX_NUMBUFS,TRUE)) ||
		((fphtx=fopen(fixspec(dbfname,"htx"),"rb"))==NULL &&
		(e=(errno==ENOENT)?TRX_ErrFind:TRX_ErrOpen))) {
		trx_Close(trx);
		dbf_Close(dbk);
		return e;
	}
	return 0;
}

DWORD map_set(void *pMap,PBYTE ckey,BOOL bExact)
{
	int e;
	DWORD rec,n=0;
	byte pkey[256];

    trx_Stccp((char *)pkey,(LPCSTR)ckey);

	if(bExact) {
		trx_SetExact(tree_id,TRUE);
		(*pkey)--;
	}

	e=trx_Find(tree_id,pkey);
	while(!e) {
		if((e=trx_GetRec(tree_id,&rec,sizeof(rec)))) break;
		rec &= ~REF_MASK;
		if((!rec || rec>reftotal) && (e==TRX_ErrFormat)) break;
		rec--;
		if(!mapset(pMap,rec)) n++;
		e=trx_FindNext(tree_id,pkey);
	}
	if(bExact) trx_SetExact(tree_id,FALSE);

	if(e!=TRX_ErrMatch && e!=TRX_ErrEOF) return (DWORD)-1;

#ifdef _DEBUG
	if(n!=mapcnt(pMap,siz_refmap_dw)) return (DWORD)-1;
#endif
	return n;
}

static void write_total(UINT total)
{
	fprintf(fphtm,"%s Listing - <b>%u</b> Reference%s%s%s",
		(total==reftotal)?"Complete":"Partial",
		total,
		(total!=1)?"s":"",
		bSort?" - Sorted by Date":"",
		bComplete?"":"<br>");
}

static void write_htm_hdr(UINT nrefs)
{
	fprintf(fphtm,"<HTML>\n<HEAD>\n<TITLE>%s</TITLE>\n</HEAD>\n<BODY",title);
	if(bBlue) fprintf(fphtm," bgcolor=\"#D8E4F1\"");
	fprintf(fphtm,">\n<p align=center><b>%s</b></p>\n<p align=center>",title);
	if(bComplete) write_total(reftotal);
	else {
		write_total(nrefs);
		if(*keyphrase) {
			fprintf(fphtm,"Keywords: <b>%s</b>",keyphrase);
			if(*authorphrase) fprintf(fphtm,"<br>");
		}
		if(*authorphrase) fprintf(fphtm,"Authors: <b>%s</b>",authorphrase);
	}
	fprintf(fphtm,"</p><p><hr></p>\n");
}

static void write_htm_end(void)
{
	fprintf(fphtm,
		"\n</BODY>\n"
		"</HTML>\n"
	);
}

static int CALLBACK compare_kw(byte *p)
{
	return trx_Strcmpp((char *)pkw,(char *)p);
}

static BOOL format_keyword(byte *pKey)
{
	UINT len;
	int i;

	for(len=0;len<nshowkw;len++) {
		i=trx_Strcmpp((char *)showkw[len],(char *)pKey);
		if(i>0) return FALSE;
		if(i>=-1) break;
	}

	if(len<nshowkw) {
		len=*pKey;
		memmove(pKey+4,pKey+1,len);
		memcpy(pKey+1,"<b>",3);
		memcpy(pKey+len+4,"</b>",4);
		(*pKey)+=7;
		return TRUE;
	}

	return FALSE;
}

static char *colon_space(byte *p)
{
	UINT len=*p++;

	for(;len--;p++) {
		if(*p==':' && len && p[1]==' ') {
			*++p=len-1;
			return (char *)p;
		}
	}
	return 0;
}

static void print_key(char *keybuf,BOOL bSomePrinted)
{
	if(bSomePrinted) fprintf(fphtm,"; ");
	else fprintf(fphtm,(bShowKeywords==1)?"<p>Matched: ":"<p>Keywords: ");
	fprintf(fphtm,trx_Stxpc(keybuf));
}


static int ShowKeywords(DWORD rec)
{
   int e;
   DWORD rec0,rec1;
   char *p;
   BOOL bSomePrinted;
   byte keybuf[256];

   rec+=dbk_off;

   if((e=dbf_Go(dbk,rec)) || (e=dbf_GetRec(dbk,&rec0))) return e;
   rec0+=dbk_off;

   if((e=dbf_Go(dbk,rec+1)) || (e=dbf_GetRec(dbk,&rec1))) return e;
   rec1+=dbk_off;

   if(rec1>rec0) {
	   bSomePrinted=FALSE;
	   for(;rec0<rec1;rec0++) {
			if((e=dbf_Go(dbk,rec0)) || (e=dbf_GetRec(dbk,&dbkRec))) {
			   break;
			}
			if((e=trx_SetPosition(TRX_KW_TREE,dbkRec.keyid&~COLON_MASK))) {
			   break;
			}
			if((e=trx_GetKey(TRX_KW_TREE,keybuf,256))) {
			   break;
			}

			if(bComplete || *keyphrase==0) {
				print_key((char *)keybuf,bSomePrinted);
				bSomePrinted=TRUE;
			}
			else {
			  if(!(e=format_keyword(keybuf))) {
				  if((dbkRec.keyid&COLON_MASK) && (p=colon_space(keybuf))) {
					  e=format_keyword((byte *)p);
					  *p=' ';
					  if(e) (*keybuf)+=7;
				  }
				  if(!e) e=bShowKeywords>1;
			  }
			  if(e) {
				  print_key((char *)keybuf,bSomePrinted);
				  bSomePrinted=TRUE;
				  e=0;
			  }
			}
	   }
	   if(bSomePrinted) fprintf(fphtm,"</p>");
   }
   return e;
}

int write_htm(void)
{
    void *pMap;
	UINT bBold,nrefs=pRefset?pRefset->count:reftotal;
	int refcnt,e;
	DWORD rec,offset0,offset1,seq,seqrec;
	DATEKEY *pDatekey;

	if((e=dbf_Go(dbk,1)) || (e=dbf_GetRec(dbk,&offset1))) return e;

	if(offset1) {
		if(!fseek(fphtx,0,SEEK_SET) && fread(refbuf,offset1,1,fphtx)==1) {
			refbuf[offset1]=0;
			title=(char *)refbuf;
		}
	}

	if((fphtm=fopen(htmname,"wt"))==NULL) return DBF_ErrCreate;

    write_htm_hdr(nrefs);

	refcnt=0;
    pMap=(pRefset && nrefs!=reftotal)?pRefset->map:0;

	if(bSort) {
		seqrec=dbf_NumRecs(dbk)-reftotal;
		pDatekey=(DATEKEY *)dbf_RecPtr(dbk);
	}

	for(seq=1;seq<=reftotal;seq++) {
		if(bSort) {
			if((e=dbf_Go(dbk,++seqrec))) break;
			rec=pDatekey->refno;
		}
		else rec=seq;
        if(!pMap || maptst(pMap,rec-1)) {
			if((e=dbf_Go(dbk,rec)) || (e=dbf_GetRec(dbk,&offset0))) {
				break;
			}
			offset0 &= 0x7fffffff;
			if((e=dbf_Go(dbk,rec+1)) || (e=dbf_GetRec(dbk,&offset1))) {
				break;
			}
			if(bShowNumbers) bBold=(offset1&0x80000000);
			offset1 &= 0x7fffffff;
			if(fseek(fphtx,offset0,SEEK_SET)) break;
			if(fread(refbuf,offset1-offset0,1,fphtx)!=1) {
				break;
			}
			refbuf[offset1-offset0]=0;

			if(bShowNumbers) {
				if(bBold) fprintf(fphtm,"<p><b>%d. %s</b></p>\n",++refcnt,refbuf);
				else fprintf(fphtm,"<p>%d. %s</p>\n",++refcnt,refbuf);
			}
			else fprintf(fphtm,"<p>%s</p>\n",refbuf);

			if(bShowKeywords && (e=ShowKeywords(rec))) {
				break;
			}
			if(!--nrefs) break;
		}
	}

	if(nrefs && !e) {
		e=DBF_ErrFormat;
	}

    write_htm_end();

	fclose(fphtm);

	return e;
}

static BOOL add_phrase(char *p)
{
	UINT len=strlen(p);

	if(len+len_keyphrase>=SIZ_KEYPHRASE) return 0;
	strcpy(keyphrase+len_keyphrase,p);
	len_keyphrase+=len;
	return TRUE;
}

static BOOL get_keyphrase(UINT nStart)
{
	UINT typ,e;

    for(e=nStart+1;e<ntokens-1;e++) {

	  switch(typ=token[e].t_type) {

		case TOK_PARENOPEN :
			if(!(add_phrase("("))) break;
			continue;

		case TOK_PARENCLOSE :
			if(!(add_phrase(")"))) break;
			continue;

		case TOK_AND :
			if(!(add_phrase(" AND "))) break;
			continue;

		case TOK_OR :
			if(!(add_phrase(" OR "))) break;
			continue;

		case TOK_NOT :
			if(!(add_phrase(" NOT "))) break;
			continue;

		case TOK_KEYPREFIX: 
		case TOK_KEYEXACT:
			if(tokquote[e] && !(add_phrase("\""))) break;
			if(!(add_phrase((char *)token[e].t_ptr))) break;
			if(typ==TOK_KEYEXACT) keyphrase[len_keyphrase-1]=tokquote[e]?'"':'.'; 
			if(tokquote[e] && !(add_phrase((typ==TOK_KEYEXACT)?".":"\""))) break;
			continue;
	  }
	  return FALSE;
	}
	return TRUE;
}

#ifdef _DEBUG
static char *toknam[8]={"KEYPREFIX","KEYEXACT","AND","OR",
				"NOT","PARENOPEN","PARENCLOSE","REFSET"};
static char tknambuf[512];

static void	dump_tokens(TOKEN *t)
{
	UINT len;

	len=sprintf(tknambuf,"%s",toknam[t->t_type]);
	if(t->t_type<=TOK_KEYEXACT && t->t_ptr) len+=sprintf(tknambuf+len,"=%s",t->t_ptr);
	t=t->t_next;

	while(t) {
		len+=sprintf(tknambuf+len," %s",toknam[t->t_type]);
	    if(t->t_type<=TOK_KEYEXACT && t->t_ptr) len+=sprintf(tknambuf+len,"=%s",t->t_ptr);
		t=t->t_next;
	}
}
#endif

static BOOL add_token(UINT typ,void *ptr)
{
	if(ntokens>=MAX_TOKENS) return FALSE;
	token[ntokens].t_ptr=ptr;
	token[ntokens].t_type=typ;
	token[ntokens].t_next=&token[ntokens+1];
	tokquote[ntokens]=FALSE;
	ntokens++;
	return TRUE;
}

static BOOL add_keytoken(char *p,BOOL bName)
{
	int i,typ;

	if(!strcmp(p,"AND")) typ=TOK_AND;
	else if(!strcmp(p,"OR")) typ=TOK_OR;
	else if(!strcmp(p,"NOT")) typ=TOK_NOT;
	else {
		i=strlen(p)-1;
		if(nkeywords==MAX_KEYWORDS || i>=SIZ_KEYWORD) return FALSE;
		typ=(!bName && i>0 && p[i]=='.' && (p[i]=(char)0xFF))?TOK_KEYEXACT:TOK_KEYPREFIX;
		p=strcpy(keyword[nkeywords++],p);
	}
	return add_token(typ,p);
}

static byte *alloc_showkwc(byte *p)
{
	int len=strlen((char *)p);
	if(ikw+len>=SIZ_KWBUF) return 0;
	p=(byte *)trx_Stccp(kwbuf+ikw,(char *)p);
	if(p[len]==0xFF) p[len]=0;
	ikw+=len+1;
	return p;
}

static byte *alloc_showkwp(byte *p)
{
	UINT len=*p;
	if(ikw+len>=SIZ_KWBUF) return 0;
	p=(byte *)memcpy(kwbuf+ikw,p,len+1);
	ikw+=len+1;
	return p;
}

static void alloc_macro_kw(void)
{
	byte *p=pkw;
	UINT e;
	BOOL bExact=(p[*p]==0xFF);
	byte keybuf[256];
	
	trx_SetExact(TRX_MACRO_TREE,bExact);
	if(bExact) (*p)--;
	e=trx_Find(TRX_MACRO_TREE,p);
	while(!e) {
		if(trx_GetKey(TRX_MACRO_TREE,keybuf,255)) break;
		pkw=keybuf+strlen((char *)keybuf+1)+1;
		*pkw=keybuf[0]-(pkw-keybuf);
		//if(pkw[*pkw]=='.') pkw[*pkw]=0; (Done in rebuild)
		if(!(pkw=alloc_showkwp(pkw))) {
			break;
		}
		trx_Binsert((DWORD)pkw,TRX_DUP_NONE);
		e=trx_FindNext(TRX_MACRO_TREE,p);
	}
	if(bExact) (*p)++;
}

static void fix_name_targets(void)
{
	UINT i;
	char *p,*ckey;
	char buf[256];

	for(i=1;i<ntokens;i++) {
		if(token[i].t_type==TOK_KEYPREFIX) {
			ckey=(char *)token[i].t_ptr;
			p=strchr(ckey,',');

			if(p || !(p=strrchr(ckey,' '))) {
				if(!p) continue;
				strcpy(buf+1,ckey);
			}
			else {
				*p++=0;
				p=buf+strlen(strcpy(buf+1,p))+1;
				*p++=',';
				strcpy(p,ckey);
			}
			*buf=0;
			for(p=buf,ckey=buf+1;*ckey;ckey++) {
				if(*ckey==' ' && (*p=='.' || *p==',')) continue;
				*++p=*ckey;
			}
			if(*p!='.') p++;
			*p=0;
			strcpy((char *)token[i].t_ptr,buf+1);
		}
	}
}

static BOOL get_tokens(BOOL bName)
{
	UINT i,c,typ;
	char *px,*p=rawphrase;

	if(!add_token(TOK_PARENOPEN,0)) goto _err;

	while(*p) {
		if(isspace(*p)) {
			p++;
			continue;
		}

		if(*p=='"') {
			if(p[1]=='"') p++;
			else {
				if(nkeywords==MAX_KEYWORDS) goto _err;

				i=0;
				do {
					px=++p;
					while(*p && *p!='"') p++;
					if(*p!='"') goto _err;
					if(p[1]=='.') {
						*p++=(char)0xFF;
						typ=TOK_KEYEXACT;
					}
					else if(p[1]=='"') {
						p++;
						typ=TOK_AND;
					}
					else typ=TOK_KEYPREFIX;
					if(i+(p-px)>=SIZ_KEYWORD) goto _err;
					*p=0;
					strcpy(keyword[nkeywords]+i,px);
					i+=p-px;
				}
				while(typ==TOK_AND);

				if(!add_token(typ,keyword[nkeywords++])) goto _err;
				tokquote[ntokens-1]=TRUE;
				p++;
				continue;
			}
		}

		if(*p=='(') {
			do {
				if(!add_token(TOK_PARENOPEN,0)) goto _err;
			}
			while(*++p=='(');
			continue;
		}

		if(*p==')') {
			do {
				if(!add_token(TOK_PARENCLOSE,0)) goto _err;
			}
			while(*++p==')');
			continue;
		}

		px=p;
		while(*++p && !isspace(*p) && *p!=')' && *p!='(' && *p!='"');
		if(*p=='"' && p[1]=='"') p++;
		c=*p;
		*p=0;
		if(!add_keytoken(px,bName)) goto _err;
		if(c=='"' && p[-1]=='"') p++;
		else *p=c;
	}

	if(!add_token(TOK_PARENCLOSE,0)) goto _err;
    token[ntokens-1].t_next=0;

#ifdef _DEBUG
	dump_tokens(token);
#endif

    //Now combine adjacent TOK_KEY* tokens into single tokens --

	for(c=0,i=1;i<ntokens;i++) {
		if(token[i].t_type<=TOK_KEYEXACT && token[c].t_type<=TOK_KEYEXACT) {
			strcat((char *)token[c].t_ptr," ");
			strcat((char *)token[c].t_ptr,(char *)token[i].t_ptr);
			token[c].t_type=token[i].t_type;
		}
		else {
			if(++c<i) {
				token[c]=token[i];
				token[c].t_next=token+c+1;
			}
		}
	}
	token[c].t_next=0;
	ntokens=c+1;

    if(bName) fix_name_targets();

	return TRUE;

_err:
	finish("Can't evaluate \"%s\"",rawphrase);
	return FALSE;
}

static void get_showkw(void)
{
	UINT i,c;

	c=trx_NumKeys(TRX_MACRO_TREE);

	//Finally, assemble keywords for highlight testing..
	if(bShowKeywords) {
		ikw=nshowkw=0;
		trx_Bininit((DWORD *)showkw,(DWORD *)&nshowkw,(TRXFCN_IF)compare_kw);
		for(i=1;i<ntokens;i++) if(token[i].t_type<=TOK_KEYEXACT) {
			pkw=alloc_showkwc((byte *)token[i].t_ptr);
			if(pkw) {
				trx_Binsert((DWORD)pkw,TRX_DUP_NONE);
				if(c && !trx_binMatch) alloc_macro_kw();
			}
		}
	}
}

static REFSET *alloc_refset(PBYTE key,UINT typ)
{
	REFSET *pref;

	if(!key || !*key || !(pref=(REFSET *)calloc(siz_refset,1))) return 0;
		
	if(*key=='*' && key[1]==0) {
		//memset(pref->map,0xFF,siz_refmap_dw*sizeof(DWORD));
		pref->count=reftotal;
	}
	else {
		pref->count=map_set(pref->map,key,typ==TOK_KEYEXACT);
		if(pref->count==(DWORD)-1) {
			free(pref);
			pref=0;
		}
	}
	return pref;
}

static void not_refset(REFSET *pref)
{
	DWORD *pdw;
	UINT len;

	if(pref->count==reftotal) pref->count=0;
	else if(pref->count==0) pref->count=reftotal;
	else {
		pdw=(DWORD *)pref->map;
		len=siz_refmap_dw;
		while(len--) {
			*pdw=~(*pdw);
			pdw++;
		}
		pref->count=reftotal-pref->count;
	}
}

static void and_refsets(REFSET *r0,REFSET *r1)
{
	UINT len;
	DWORD *pdw0,*pdw1;

	if(!r0->count || r1->count==reftotal) return;
	if(!r1->count) {
		//memset(r0,0,siz_refset);
		r0->count=0;
		return;
	}
	if(r0->count==reftotal) {
		memcpy(r0,r1,siz_refset);
		return;
	}
	pdw0=(DWORD *)r0->map;
	pdw1=(DWORD *)r1->map;
	for(len=siz_refmap_dw;len;len--) {
		*pdw0 &= *pdw1++;
		pdw0++;
	}
	r0->count=mapcnt(r0->map,siz_refmap_dw);
}

static void or_refsets(REFSET *r0,REFSET *r1)
{
	UINT len;
	DWORD *pdw0,*pdw1;

	if(!r1->count || r0->count==reftotal) return;

	if(r1->count==reftotal) {
		r0->count=reftotal;
		return;
	}

	if(!r0->count) {
		memcpy(r0,r1,siz_refset);
		return;
	}

	pdw0=(DWORD *)r0->map;
	pdw1=(DWORD *)r1->map;
	for(len=siz_refmap_dw;len;len--) {
		*pdw0 |= *pdw1++;
		pdw0++;
	}
	r0->count=mapcnt(r0->map,siz_refmap_dw);
}

static BOOL combine_refsets(TOKEN *tparen)
{
	TOKEN *t0=tparen;
	TOKEN *t=tparen->t_next;
	BOOL bNotPending=FALSE;
	UINT n=0;

	nops=0;

	while(t->t_type!=TOK_PARENCLOSE) {
		if(t->t_type!=TOK_REFSET) {
			/*must be operator --*/
			if(t->t_type==TOK_NOT) bNotPending=!bNotPending; /*allow "NOT NOT"*/
			else {
				bNotPending=FALSE; /*ignore unexpected NOT*/
				op[nops++]=t->t_type;
			}
		}
		else {
			if(bNotPending) {
				not_refset((REFSET *)t->t_ptr);
				bNotPending=FALSE;
			}
			*t0=*t;
			t0->t_next=t0+1;
			t0++;
			n++;
		}
		t=t->t_next;
	}
	*t0=*t;

	/*Now, tparen is the head of a stack of n REFSET tokens. The chain
	ends at a TOK_PARENCLOSE. The operator stack, op[0..nops-1], contains
	only binary tokens*/

#ifdef _DEBUG
	dump_tokens(tparen);
#endif

	if(tparen->t_type!=TOK_REFSET || n!=nops+1) return FALSE;
	t=tparen->t_next;
	for(n=0;n<nops;n++) {
		if(op[n]==TOK_AND) and_refsets((REFSET *)tparen->t_ptr,(REFSET *)t->t_ptr);
		else or_refsets((REFSET *)tparen->t_ptr,(REFSET *)t->t_ptr);
		free(t->t_ptr);
		t=t->t_next;
	}
	/*t must be the TOK_PARENCLOSE token!*/
	tparen->t_next=t->t_next;

#ifdef _DEBUG
	dump_tokens(tparen);
#endif

	return TRUE;
}

static void free_tokens(void)
{
	TOKEN *t=token;

	do {
		if(t->t_type==TOK_REFSET && t->t_ptr) free(t->t_ptr);
	}
	while(t=t->t_next);

}

static int eval_token_group(TOKEN *tparen)
{
	/*This recursive function replaces the TOK_PARENOPEN token
	with a single REFSET token whose t_next pointer points
	to the token past the matching TOK_PARENCLOSE token.*/
	
	TOKEN *t=tparen->t_next;

    if(!t || t->t_type==TOK_PARENCLOSE) return -1;
	
	while(t->t_type!=TOK_PARENCLOSE) {
		if(t->t_type==TOK_PARENOPEN) {
			if(!eval_token_group(t)) return FALSE;
		}
		else if(t->t_type<=TOK_KEYEXACT) {
			 if(!(t->t_ptr=alloc_refset((PBYTE)t->t_ptr,t->t_type))) return FALSE;
			 t->t_type=TOK_REFSET;
		}
		if((t=t->t_next)==NULL) return FALSE; /*parens don't balance!*/
	}
#ifdef _DEBUG
	dump_tokens(tparen);
#endif
	/*The group now has only operators and REFSETs --*/
	if(!combine_refsets(tparen)) return FALSE; //Non-recursive
	//*All but the one refset has now been freed.
	return TRUE;
}

/*==============================================================*/
RESELLIB_API int re_Select(int argc,char **argv)
{
  UINT e;
  UINT numphrases=0;

  *htmname=*rawphrase=*keyphrase=0;
  authorphrase=keyphrase;
  ntokens=nkeywords=len_keyphrase=nkeytokens=0;
  fphtm=NULL;
  trx=dbk=0;

  bShowKeywords=(resel_Flags & RESEL_SHOWKEY_MASK);
  bShowNumbers=(0!=(resel_Flags & RESEL_SHOWNO_MASK));
  bBlue=(0!=(resel_Flags & RESEL_BLUE_MASK));
  bSort=(0!=(resel_Flags & RESEL_SORT_MASK));
  bOrAuthors=(0!=(resel_Flags & RESEL_OR_AUTHOR_MASK));

  if(argc<3) return finish("Three arguments to DLL required");

  if(!*argv[1] && !*argv[2]) return finish("Please enter a keyword or author expression");

  bComplete=FALSE;

  for(e=1;e<3;e++) {
	  if(*argv[e]=='*' && argv[e][1]==0) {
		  if(bOrAuthors || *argv[3-e]==0 || (*argv[3-e]=='*' && argv[3-e][1]==0)) {
			  pRefset=0;
			  bComplete=TRUE;
			  break;
		  }
		  else *argv[e]=0;
	  }
  }

  if(bShowKeywords==1 && (bComplete || *argv[1]==0)) bShowKeywords=0;

  trx_Stcext(htmname,argv[0],"htm",sizeof(htmname));
  *trx_Stpext(strcpy(dbfname,htmname))=0;

  if((e=open_database())) {
	  trx=0; dbk=0; fphtx=0;
	  return finish("Can't open database %s (.DBK, .TRX, .HTX): %s.\n"
		  "NOTE: The three file components must reside in the program's \"Start in\" folder",
		  _strupr(trx_Stpnam(dbfname)),
		  trx_errno?trx_Errstr(e):dbf_Errstr(e));
  }

  siz_refmap_dw=(reftotal+8*sizeof(DWORD)-1)/(8*sizeof(DWORD));
  siz_refset=(siz_refmap_dw+1)*sizeof(DWORD);

  if(!bComplete) {
	  if(*argv[1]) {
		  tree_id=TRX_KW_TREE;
		  _strupr(strcpy(rawphrase,argv[1]));
		  if(!get_tokens(FALSE)) return 1;

		  get_showkw();

		  #ifdef _DEBUG
		  e=ikw;
		  dump_tokens(token);
		  #endif
		  if(!get_keyphrase(0)) return finish("Expression too long");

		  if(!eval_token_group(token)) {
			  free_tokens();
			  return finish("Can't evaluate \"%s\"",keyphrase);
		  }
		  ntokens=numphrases=1;
	  }

	  len_keyphrase++;
  
	  authorphrase=keyphrase+len_keyphrase;
	  *authorphrase=0;

	  if(*argv[2]) {
		  tree_id=TRX_AUTHOR_TREE;
		  _strupr(strcpy(rawphrase,argv[2]));
		  if(!get_tokens(TRUE)) return 1;

		  #ifdef _DEBUG
		  dump_tokens(token+numphrases);
		  #endif

		  if(!get_keyphrase(numphrases)) return finish("Expression too long");

		  if(!eval_token_group(token+numphrases)) {
			  free_tokens();
			  return finish("Can't evaluate \"%s\"",authorphrase);
		  }
		  ntokens++;
		  numphrases++;
	  }

	  pRefset=(token[0].t_type==TOK_REFSET)?(REFSET *)token[0].t_ptr:0;
	  if(pRefset && numphrases==2 && token[1].t_type==TOK_REFSET) {
		  if(!bOrAuthors) and_refsets(pRefset,(REFSET *)token[1].t_ptr);
		  else or_refsets(pRefset,(REFSET *)token[1].t_ptr);
		  free(token[1].t_ptr);
	  }
  }
  
  if(!pRefset || pRefset->count) {
	  e=write_htm();
	  if(e) {
		  free(pRefset);
		  return finish("Error writing HTM - %s",dbf_Errstr(e));
	  }
  }
  else {
	  if(!*authorphrase) finish("No items were found satisfying the keyword expression");
	  else if(!*keyphrase) finish("No items were found with the specified authors"); 
	  else finish("No items were found with the specified keywords and/or authors");
	  return 1;
  }

  free(pRefset);
  fclose(fphtx);
  trx_Close(trx);
  dbf_Close(dbk);
  *_re_errstr=0;
  return 0;
}

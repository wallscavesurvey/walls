#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <trx_str.h>
#include <trx_file.h>

//Functions in indexa.c --
BOOL is_xspace(byte c);
nptr skip_space_types(nptr p);
void index_author(int trxno,UINT refno,char *pRef);
apfcn_v parse_keywords(nptr p0);
apfcn_n skip_tag(nptr p);
nptr elim_all_st(nptr p0,int n);
nptr elim_all_ins(nptr p0);
nptr elim_all_ole(nptr p0,BOOL bNote,UINT *uLinksOLE);

static char hdr[]="REINDEX v3.0 - Bibliographic Item Indexing (c) 2015, D. McKenzie\n";

#define MAX_FNAMES 20
#define MAXPATH 128
#define SIZ_LINEBUF 2048
#define SIZ_REFBUF (48*1024)
#define LINE_BRKPT 70
#define EXPLICIT_SPC 0xA0
#define RMARGIN 72

#define TRX_KW_TREE (trx)
#define TRX_AUTHOR_TREE (trx+1)

static nptr tmpname="REINDEX.$$$";
static nptr span_str="<span ";
#define LEN_SPAN_STR 6

static FILE *fp,*fptmp,*fplog;
static UINT num_fnames,linecnt,refcnt,reftotal,logtotal,uNoRefs,uLinksOLE,uLinksOLE_ttl;
static UINT kw_parsed;
static byte linebuf[SIZ_LINEBUF],seqbuf[16]; //"xxxxx. "
static byte refbuf[SIZ_REFBUF],new_refbuf[SIZ_REFBUF];
static byte logname[MAXPATH],cfgname[MAXPATH],namebuf[MAXPATH];
static byte fname[MAX_FNAMES][MAXPATH];
static nptr new_refptr;
static int trx;
static BOOL bPeriodFound,bRefLastParagraph,bBibClass;
static BOOL bIndexClass,bRefSeen,bCompound,bListOLE;

#define LEN_REFIDX (16*1024)
static WORD refidx[LEN_REFIDX];

//static char title[SIZ_LINEBUF];

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
  if(fptmp) {
    fclose(fptmp);
    _unlink(tmpname);
  }
  va_end(marker);
  if(fplog) {
	  fclose(fplog);
	  //_unlink(logname);
  }
  if(trx) trx_CloseDel(trx);
  pause("to exit");
  exit(1);
}

static void openlog(nptr hdr)
{
	int i;

	if(!fplog) {
		trx_Stcext(logname,cfgname,"LST",MAXPATH);
		if((fplog=fopen(logname,"wt"))==NULL) abortmsg("Cannot open %s",logname);
	}
	else putc('\n',fplog);

	i=fprintf(fplog,"%s - %s",cfgname,hdr);
	putc('\n',fplog);
	while(i--) putc('=',fplog);
	putc('\n',fplog);
}

apfcn_v logmsg(nptr p, nptr s)
{
  char prefix[56];
  int i;

  if(!fplog) {
	  openlog("QUESTIONABLE ITEMS");
	  putc('\n',fplog);
  }
  strncpy(prefix,p,50);
  if(prefix[49]!=0) strcpy(prefix+49,"..");
  if(*prefix!='<' && (p=strchr(prefix,'<'))) *p=0;
  if((i=strlen(prefix))<51) while(i++<51) strcat(prefix," ");
  p=trx_Stpext(namebuf);
  if(*p) *p=0;
  else p=0;
  //sprintf(buf,"%s:%5u. %s - %s.\n",namebuf,reftotal,prefix,s);
  fprintf(fplog,"%s:%5u. %s - %s.\n",namebuf,reftotal,prefix,s);
  if(p) *p='.';
  logtotal++;
}

static void abort_format(nptr msg)
{
  abortmsg("File %s, line %d: Bad format: %s",namebuf,linecnt,msg);
}

static void abort_full(void)
{
  abortmsg("File %s, line %d: Write error (disk full)",namebuf,linecnt);
}

static apfcn_v put_cline(void)
{
  nptr p=linebuf;
  int c;

  while((c=*p++)!=0) if(putc(c,fptmp)==EOF) abort_full();
  putc('\n',fptmp);
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
  if(strstr(linebuf, "isopoda: Asellota.")) {
	  int jj=0;
  }
  return TRUE;
}

static BOOL get_ref_paragraph(void)
{
	UINT linlen,bufcnt=0;
	nptr p,pStart;

	while(get_cline()) {
		pStart=skip_space_types(linebuf);
		if(!memcmp(pStart,"<p ",3)) {
			bBibClass=!_memicmp(pStart,"<p class=Bibliography",21) || !_memicmp(pStart,"<p class=MsoBibliography",24);
			bIndexClass=!bBibClass && !_memicmp(pStart,"<p class=index",14);
			if(bRefSeen || bBibClass || bIndexClass) {
				//Collect reference, keywords, or blank line --
				do {
				  linlen=strlen(pStart)+1; //include NULL
				  if(bufcnt+linlen>=SIZ_REFBUF) abort_format("Paragraph too large");
				  if(bufcnt) refbuf[bufcnt++]=' ';
				  p=strstr(memcpy(refbuf+bufcnt,pStart,linlen),"</p>");
				  if(p) {
					  if(strlen(p)!=4) logmsg(p,"Unexpected tags after </p> ignored");
					  return TRUE;
				  }
				  bufcnt+=linlen-1;
				}
				while(get_cline() && (pStart=skip_space_types(linebuf)));
				abort_format("Paragraph truncated");
			}
		}
		put_cline();
	}
	return FALSE;
}

static nptr copy_tag(nptr p)
{
	if(*p=='<') {
		do {
			*new_refptr++=*p++;
		}
		while(*p && *p!='>');
		if(*p) *new_refptr++=*p++;
	}
	*new_refptr=0;
	return p;
}

static nptr skip_seqnum(nptr p0)
{
	nptr p=p0;

	while(isdigit(*p)) p++;
	if(*p=='.') {
		while(is_xspace(*++p));
		bPeriodFound=TRUE;
		return p;
	}
	return p0;
}

apfcn_v IndexDate(int date)
{
	//This will be called from index_author() in indexa.c for every reference --
	//reindex.exe does nothing. rebuild.exe creates a sorted array.
}

static UINT add_colon_key(nptr key)
{
	nptr p;
	UINT len;

	if((p=strstr((char *)key+1, ": ")) && p[2]) {
		p++;
		len=strlen((char *)p+1);
		*p=(byte)len;
		if(len>255 || trx_InsertKey(TRX_KW_TREE, &reftotal, p)) {
			return 0;
		}
		*p=' ';
	}
	return reftotal;
}

apfcn_v AddKeyword(byte *keybuf)
{
	UINT rec;
	if(!(rec=add_colon_key(keybuf)) || trx_InsertKey(TRX_KW_TREE,&rec,keybuf))
		abortmsg("Error writing index");
	kw_parsed++;
}

static nptr elim_span(nptr p0)
{
	nptr pn=skip_tag(p0);
	nptr p;

	if(!(p=strstr(pn,"</span>"))) abort_format("Span tag not terminated");
	memcpy(p0,pn,p-pn);
	strcpy(p0+(p-pn),skip_tag(p));
	return p0;
}

static void elim_all_spans(nptr p)
{
	while(p=strstr(p,span_str)) elim_span(p);
}

static BOOL new_ref_paragraph(void)
{
	nptr p0,p=refbuf;
	int len;

	new_refptr=new_refbuf;
	bPeriodFound=0;

	p=copy_tag(p);

	if(!(p=elim_all_ins(p)))
		abort_format("ins tag doesn't terminate");

	if(!(p=elim_all_st(p,1)) || !(p=elim_all_st(p,2)))
		abort_format("st tag doesn't terminate");

	if((p0=strstr(p,"<st")) && isdigit(p0[3])) {
		abort_format("unexpected st_: tag");
	}

	//p points to null or 1st non-space char after copied tag (initially <p ...>)
	//new_refptr points to null (position of next char)

	while(*p=='<') {
		if(!memcmp(p,span_str,LEN_SPAN_STR)) {
			//Ignore span tags altogether --
			p=elim_span(p);
		}
		else {
			if(bRefLastParagraph) {
				p=skip_tag(p);
			}
			else {
				p=copy_tag(p);
				if(!bPeriodFound) p=skip_seqnum(p); //Can set bPeriodFound
			}
		}
		p=skip_space_types(p);
	}


	//p now points to null (unexpected), 1st keyword (if bRefLastParagraph),
	//or (if bBibClass) an author's name, or possibly a sequence number without leading spaces --

	if(!bPeriodFound) p=skip_seqnum(p);

	if(!*p || !memcmp(p,"</p>",4)) {
	    if(bPeriodFound) logmsg(refbuf,"Isolated \". \" prefix");
		return bRefLastParagraph=FALSE; //Blank line - keep original version
	}

	if(bRefLastParagraph || bIndexClass) {
		if(!bRefLastParagraph) {
			//Isolated index style paragraph --
			if(!bPeriodFound) {
				logmsg(p,"ERROR: Orphaned \"index\" paragraph");
				return bRefLastParagraph=FALSE;
			}
			bBibClass=2;
		}
		else {
			if(!bPeriodFound) {
				if(!bIndexClass) logmsg(p,"Keyword style is not \"index\"");
				//parse keywords here --
				elim_all_spans(p);
				parse_keywords(p);
				return bRefLastParagraph=FALSE;
			}
			//Prefixed unseparated item, also possibly not bBibClass;
		}
	}

	if(*p=='.') bPeriodFound=2; //Minimal check of author name format

    if((int)(strlen(p)+7/*LEN_SPAN_STR+14*/)>SIZ_REFBUF-(new_refptr-new_refbuf))
		abort_format("Revised paragraph too large");

	//Insert sequence number --
	sprintf(seqbuf,"%u. ",++reftotal);

	if((len=strlen(seqbuf))<7) {
		//strcpy(new_refptr,span_str);
		//new_refptr+=LEN_SPAN_STR;
		if(len==6) *new_refptr++=EXPLICIT_SPC;
		else {
			while(len++<6) *new_refptr++=EXPLICIT_SPC;
			*new_refptr++=' ';
		}
		//strcpy(new_refptr,"</span>");
		//new_refptr+=7;
	}

	//Insert refrence --
	new_refptr+=strlen(strcpy(new_refptr,seqbuf));
	strcpy(new_refptr,p);

	if(bBibClass && !bRefLastParagraph) {
		index_author(TRX_AUTHOR_TREE,reftotal,p);
	}

	if(bBibClass!=TRUE) {
		if(bIndexClass) logmsg(p,"Item with \"index\" style");
		else logmsg(p,"Item style is not \"Bibliography\"");
	}

	if(!bPeriodFound) {
		//logmsg(p,"Note: Prefix \". \" absent in original");
	}

	if(bRefLastParagraph) {
		logmsg(p,"No separator before numbered item");
	}

	if(bPeriodFound>1) logmsg(p,"Unexpected period");

	return bRefLastParagraph=TRUE;
}

static void put_ref_paragraph(nptr p)
{
	UINT nchr=0;

	while(*p) {
		if(*p==' ' && nchr>=LINE_BRKPT) {
			putc('\n',fptmp);
			p++;
			nchr=0;
		}
		else {
			putc(*p++,fptmp);
			nchr++;
		}
	}
	putc('\n',fptmp);
}

static apfcn_v scan_file(void)
{
  linecnt=refcnt=uLinksOLE=0;
  bRefLastParagraph=bRefSeen=FALSE;

  eprintf("\n%-25s - References:     0",namebuf);
  while(get_ref_paragraph()) {
	  //refbuf now has reference, keywords, or blank line --

	  if(!elim_all_ole(refbuf, bListOLE,&uLinksOLE))
		  abort_format("name tag doesn't terminate");

	  if(new_ref_paragraph()) {

		put_ref_paragraph(new_refbuf);
        eprintf("\b\b\b\b\b%5u",++refcnt);
		bRefSeen=TRUE;
	  }
	  else {
		//Should be an explicit blank line (&nbsp) or keywords --
		put_ref_paragraph(refbuf);
	  }
  }
  if(uLinksOLE) {
	  eprintf("  OLE Links: %5u", uLinksOLE);
	  uLinksOLE_ttl+=uLinksOLE;
  }

}

static apfcn_v trim_twsp(nptr ln)
{
	nptr p;
    for(p=ln+strlen(ln)-1;p>=ln && is_xspace(*p);p--);
    *(p+1)=0;
}

static apfcn_n trim_lwsp(nptr ln)
{
	nptr p;
    for(p=ln;is_xspace(*p);p++);
    return p;
}

static apfcn_v get_fnames(void)
{
  char *p;
  int mode=0;

  linecnt=0;

  while(get_cline()) {
    linecnt++;
	p=trim_lwsp(linebuf);
	trim_twsp(p);
    if(*p==0 || *p==';') continue;
    if(*p=='.') {
		p++;
		if(!memicmp(p,"DB",2)) {
			//p=skip_spc(p+2);
		    //title=strcpy(titlebuf,p);
			mode=1;
		}
		else if(!memicmp(p,"KW",2)) {
			if(mode) break;
		    mode=2;
		}
		else abortmsg("Unrecognized CFG file directive - line %u",linecnt);
		continue;
	}
	if(!mode) abortmsg("CFG file error - No .DB directive present");

	if(mode==1) {
		if(trx_Stpnam(p)!=p) abortmsg("Names in CFG file cannot have path prefixes");
		if(num_fnames>=MAX_FNAMES)
		  abortmsg("More than %d names in %s",MAX_FNAMES,namebuf);
		trx_Stcext(fname[num_fnames++],_strupr(p),"HTM",MAXPATH);
	}
  }
  if(!num_fnames) abortmsg("No names in file %s",namebuf);
}

static apfcn_v init_index(void)
{
	trx_Stcext(namebuf,cfgname,"ID$",MAXPATH);
	trx_Create(&trx,namebuf,0,2,0,1024,4);

	if(trx && (trx_InitTree(TRX_KW_TREE,4,0,TRX_KeyMaxSize) ||
		trx_InitTree(TRX_AUTHOR_TREE,4,0,TRX_KeyMaxSize) ||
		trx_AllocCache(trx,128,TRUE))) {
		trx_CloseDel(trx);
		trx=0;
	}
	trx_SetExact(TRX_AUTHOR_TREE,TRUE);
	if(!trx) abortmsg("Cannot create index %s",namebuf);
}

static int get_first_key(int idx)
{
	DWORD rec;
	return (trx_FillKeyBuffer(idx,-2) || trx_GetRec(idx,&rec,sizeof(DWORD)))?0:rec;
}

static UINT get_next_key(int idx)
{
	DWORD rec;
	return (trx_FillKeyBuffer(idx,1) || trx_GetRec(idx,&rec,sizeof(DWORD)))?0:rec;
}

static void list_refs(int idx,UINT refcnt)
{
	UINT col;
	char *p;

	if(bCompound && idx==TRX_AUTHOR_TREE) {
		for(p=refbuf;*p;p++) {
			if(*p==',' || *p==' ') break;
		}
		if(*p!=' ') return;
	}
	else if(idx==TRX_KW_TREE && uNoRefs && refcnt>uNoRefs) return;

	col=fprintf(fplog,"\n%s:  %u",refbuf,refidx[--refcnt])-2;
	while(refcnt) {
		col+=fprintf(fplog,",");
		if(col>=RMARGIN) col=fprintf(fplog,"\n   ")-2;
		col+=fprintf(fplog," %u",refidx[--refcnt]);
	}
}

static apfcn_v scan_index(int idx)
{
  UINT rec;
  DWORD rlen,rlen_max,refcnt,numkeys;
  BYTE rlen_maxkey[256];

  if(!(numkeys=trx_NumKeys(idx))) return;

  if(idx==TRX_KW_TREE) {
	  eprintf("\nWriting keyword list..");
	  openlog("KEYWORD INDEX");
  }
  else {
	  eprintf("\nWriting author list..");
	  if(bCompound) openlog("AUTHOR INDEX - Compound Last Names Only");
	  else openlog("AUTHOR INDEX");
  }

  trx_SetKeyBuffer(idx,linebuf);

  *refbuf=0xFF;
  refbuf[1]=0;
  rlen=rlen_max=refcnt=0;
  rec=get_first_key(idx);

  while(rec) {
    numkeys--;

	linebuf[*linebuf+1]=0;

	if(strcmp(linebuf+1,refbuf)) {
        if(refcnt) list_refs(idx,refcnt);
		refidx[0]=(WORD)rec;
		refcnt=1;
		strcpy(refbuf,linebuf+1);

		if(rlen) break;
		rlen=trx_RunLength(idx)-1;
		if(rlen>rlen_max && strcmp(refbuf,"ANONYMOUS")) {
			rlen_max=rlen;
			strcpy(rlen_maxkey,refbuf);
		}
	}
	else {
		rlen--;
		if(refcnt<LEN_REFIDX) refidx[refcnt++]=(WORD)rec;
	}
	rec=get_next_key(idx);
  }

  if(refcnt) list_refs(idx,refcnt);

  putc('\n',fplog);

  if(rlen) {
	  abortmsg("Unexpected index format (runlength=%u)",rlen);
  }

  if(numkeys) abortmsg("Error reading index - code %u",trx_errno);

  eprintf(" Completed.\nMost %s: %s (%u appearances)\n",
	  (idx==TRX_KW_TREE)?"frequent keyword":"prolific author",rlen_maxkey,rlen_max+1);

}

/*==============================================================*/
void main(int argc,nptr *argv)
{
  nptr p;
  UINT e;
  BOOL blist_kw=TRUE,blist_authors=TRUE;

  eprintf("\n%s",hdr);

  bListOLE=TRUE;

  while(--argc) {
    _strupr(p=*++argv);
	if(!memcmp(p,"-N",2)) {
		uNoRefs=atoi(p+2);
		continue;
	}
	if(!strcmp(p,"-C")) {
		bCompound=TRUE;
		continue;
	}
	if(!strcmp(p,"-K")) {
		blist_kw=FALSE;
		continue;
	}
	if(!strcmp(p, "-A")) {
		blist_authors=FALSE;
		continue;
	}
	if(!strcmp(p, "-L")) {
		bListOLE=FALSE;
		continue;
	}
	if(*cfgname || trx_Stpnam(p)!=p || *trx_Stpext(p)) continue;
	strcpy(cfgname,p);
  }

  if(!*cfgname) {
    eprintf("\n"
	  "Usage: REINDEX [-k] [-a] [-c] [-l] [-n<number>] <cfgname>\n\n"
      "Creates indexed versions of HTML files whose names are listed in a\n"
	  "plain text file, <cfgname>.CFG>. It also generates an output text file,\n"
	  "<cfgname>.LST, containing diagnostic messages and text representations\n"
	  "of the author and keyword indexes. After a review of this file, program,\n"
	  "REBUILD, can be run to produce the database for RESELECT.\n\n"

	  "The file names listed in <cfgname>.CFG must appear one per line, be\n"
	  "without path prefixes, and must correspond to files residing in the\n"
	  "program's folder along with <cfgname>.CFG. When a file extension is\n"
	  "missing from a name, it is assumed to be .HTM. See the provided CFG\n"
	  "file template for additional format details.\n\n"
	  
	  "The HTML files to be processed should contain James Reddell's style of\n"
	  "bibliographic references and have MS Word's \"Web Page, Filtered\"\n"
	  "format. The program scans the files and revises them by prefixing each\n"
	  "detected reference with a sequence number representing its position in\n"
	  "the complete scan. (Any existing numbers are updated.) Apart from prefix\n"
	  "assignment, the content and essential format of the files is preserved.\n");
	pause("for more info");
	eprintf(
	  "A single paragraph is treated as a bibliographic item when it has a style\n"
	  "named \"bibliography..\" and is separated from the previous reference by an\n"
	  "empty line. A paragraph is treated as a keyword list when it immediately\n"
	  "follows a bibliographic paragraph. Although the expected style is \"index\",\n"
	  "this is not enforced. For example, an unintended paragraph break in a\n"
	  "reference would cause the trailing portion to be treated as keywords.\n"
	  "Since keyword paragraphs, like blank lines, serve as item separators, the\n"
	  "actual keywords would be orphaned (preserved but not indexed). Unexpected\n"
	  "conditions like this should cause diagnostic messages to be logged.\n\n"

	  "Backups of the original files will be saved with extension .HTB.\n"
	  "The diagnostics and index listings are written to <cfgname>.LST.\n\n"

	  "Options: Use -N<number> to not list keywords with more than <number>\n"
	  "references. Use -C to list only authors with compound last names.\n"
	  "Use -K and/or -A to omit the keyword or author listing from the log.\n"
	  "Use -L to omit notes for stripped-out useless OLE link tags that Word\n"
	  "sometimes needlessly inserts during cut/paste operations.\n"
	);
	pause("to exit");
    exit(1);
  }

 trx_Stcext(namebuf,cfgname,"CFG",MAXPATH);
 if(!fp && (fp=fopen(namebuf,"rt"))==NULL)
    abortmsg("Cannot open file %s",namebuf);

  get_fnames();
  fclose(fp);

  init_index();

  _unlink(tmpname);
  
  reftotal=logtotal=0;
  for(e=0;e<num_fnames;e++) {
    strcpy(namebuf,fname[e]);
    if((fp=fopen(namebuf,"rt"))==NULL) abortmsg("Cannot open file %s",namebuf);
    if((fptmp=fopen(tmpname,"wt"))==NULL) abortmsg("Cannot create temporary file");
    scan_file();
	fclose(fp);
	fp=0;
    if(fclose(fptmp)) {
      fptmp=0;
      _unlink(tmpname);
      abort_full();
    }
	fptmp=0;
    strcpy(trx_Stpext(namebuf),".htb");
    _unlink(namebuf);
    if(rename(fname[e],namebuf)) {
		abortmsg("Cannot rename %s to %s", fname[e], namebuf);
	}
    if(rename(tmpname,fname[e])) {
      abortmsg("Cannot rename temporary file to %s",fname[e]);
	}
  }

  e=trx_NumKeys(TRX_KW_TREE);
  eprintf("\n\nFiles: %d  Refs: %u  Authors: %u  Keywords: %u (%u compound)  Removed OLE links: %u\n",
	  num_fnames, reftotal, trx_NumKeys(TRX_AUTHOR_TREE), e, (e-kw_parsed), uLinksOLE_ttl);

  if(blist_authors) scan_index(TRX_AUTHOR_TREE);
  if(blist_kw) scan_index(TRX_KW_TREE);

  trx_CloseDel(trx); trx=0;
  //trx_Close(trx); trx=0;

  if(fplog) {
	  fclose(fplog);
	  if(logtotal) eprintf("\nNOTE: There were %u questionable items. See %s.\n",logtotal,logname);
	  else eprintf("\nNo questionable items. Indexes written to %s.\n",logname);
  }

  pause("to exit");
  exit(0);
}

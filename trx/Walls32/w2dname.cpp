#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "expat.h"
#include "trx_str.h"
#include "trxfile.h"

#define BUFFSIZE 16384
#define SIZ_LINEBREAK 80

//Label to station offsets (pts) --
#define MAX_PTSX10 65790
#define MAX_PTSX10_DX 20
#define MAX_PTSX10_DST (2*(MAX_PTSX10_DX*MAX_PTSX10_DX))
#define LABEL_XOFF	(-0.75)
#define LABEL_YOFF	(0.85)
#define SVG_MAX_SIZNAME 63
#pragma pack(1)
typedef struct {
	byte len;
	char cpos[5];
	char name[SVG_MAX_SIZNAME+1];
} SVG_LABELKEY;
#pragma pack()

typedef struct {
	double x,y;
	BOOL bNew;
	char prefix[SVG_MAX_SIZNAME+1];
	char name[SVG_MAX_SIZNAME+1];
} SVG_ENDPOINT;

CTRXFile *pix=NULL;
CTRXFile *pix_chk=NULL;

static char Buff[BUFFSIZE];
static char buf[BUFFSIZE+2];
static char argv_buf[2*_MAX_PATH];
static char fpin_name[_MAX_PATH],fpout_name[_MAX_PATH];

typedef struct {
	int bEntity;
	int errcode;
	int errline;
} TYP_UDATA;

static double xpos,ypos;
static int namelen,namecnt,veccnt,badveccnt,badnamecnt,badvecidcnt;
static TYP_UDATA udata;
static FILE *fpin,*fpout;

static BOOL bLabelID;
static int Depth,bNoClose,nCol;
static XML_Parser parser;
static int parser_ErrorCode;
static int parser_ErrorLineNumber=0;

#define OUTC(c) putc(c,fpout)
#define OUTS(s) fputs(s,fpout)
#define UD(t) ((TYP_UDATA *)userData)->t

enum {
  SVG_ERR_ABORTED=1,
  SVG_ERR_XMLOPEN,
  SVG_ERR_XMLREAD,
  SVG_ERR_XMLPARSE,
  SVG_ERR_SVGCREATE,
  SVG_ERR_WRITE,
  SVG_ERR_VERSION,
  SVG_ERR_NOMEM,
  SVG_ERR_NOFRAME,
  SVG_ERR_NOID,
  SVG_ERR_INDEXINIT,
  SVG_ERR_INDEXWRITE,
  SVG_ERR_BADNAME,
  SVG_ERR_PARSERCREATE,
  SVG_ERR_TRANSFORM,
  SVG_ERR_NOTEXT,
  SVG_ERR_NOTSPAN,
  SVG_ERR_ENDGROUP,
  SVG_ERR_ENDTEXT,
  SVG_ERR_ENDTSPAN,
  SVG_ERR_SIZNAME,
  SVG_ERR_LABELPOS,
  SVG_ERR_NOPATH,
  SVG_ERR_PARSEPATH,
  SVG_ERR_INDEXOPEN,
  SVG_ERR_TEMPINIT,
  SVG_ERR_POLYPATH,
  SVG_ERR_BADIDSTR,
  SVG_ERR_LABELZERO,
  SVG_ERR_UNKNOWN
};

static NPSTR errstr[]={
  "User abort",
  "Cannot open template",
  "Error reading",
  "Error parsing",
  "Cannot create",
  "Error writing",
  "Incompatible version of wallsvg.dll",
  "Not enough memory or resources",
  "Invalid wallsvg.xml - no w2d_Frame group",
  "Invalid wallsvg.xml - no group id",
  "Error creating index",
  "Error writing index",
  "Bad station name in data set",
  "Error initializing parser",
  "Text transform expected",
  "Text element expected",
  "Tspan element expected",
  "End group expected",
  "End text expected",
  "End tspan expected",
  "Station name too long",
  "Label position out of range",
  "Path element expected",
  "Unexpected path data format",
  "Can't open .._w2d.trx file",
  "Can't create temporary index",
  "Polyline vector with ID",
  "Wrong format for vector path ID",
  "Zero length label",
  "Unknown error"
};

char * ErrMsg(int code)
{
  int len;
  if(code>SVG_ERR_UNKNOWN) code=SVG_ERR_UNKNOWN;
  if(code>SVG_ERR_ABORTED && code<SVG_ERR_SVGCREATE) {
	len=sprintf(argv_buf,"%s file %s",(LPSTR)errstr[code-1],trx_Stpnam(fpin_name));
    if(code==SVG_ERR_XMLPARSE)
		len+=sprintf(argv_buf+len,": %s",XML_ErrorString(parser_ErrorCode));
  }
  else if(code<=SVG_ERR_WRITE) {
	  len=sprintf(argv_buf,"%s file %s",(LPSTR)errstr[code-1],trx_Stpnam(fpout_name));
  }
  else len=strlen(strcpy(argv_buf,errstr[code-1]));
  if(parser_ErrorLineNumber>0) len+=sprintf(argv_buf+len," - Line %d",parser_ErrorLineNumber);
  strcpy(argv_buf+len,"\n");
  return argv_buf;
}

//=============================================================================
static void xmldecl(void *userData,
                       const XML_Char  *version,
                       const XML_Char  *encoding,
                       int             standalone)
{
	OUTS("<?xml");
	if(version) fprintf(fpout," version=\"%s\"",version);
	if(encoding) fprintf(fpout," encoding=\"%s\"",encoding);
	fprintf(fpout," standalone=\"%s\"?>\n",standalone?"yes":"no");
	nCol=0;
}

static void entity(void *userData,
                          const XML_Char *entityName,
                          int is_parameter_entity,
                          const XML_Char *value,
                          int value_length,
                          const XML_Char *base,
                          const XML_Char *systemId,
                          const XML_Char *publicId,
                          const XML_Char *notationName)
{
	if(value) {
		memcpy(buf,value,value_length);
		buf[value_length]=0;
		fprintf(fpout,"  <!ENTITY %s \"%s\">\n",entityName,buf);
	}
}

static void startDoctype(void *userData,
                               const XML_Char *doctypeName,
                               const XML_Char *sysid,
                               const XML_Char *pubid,
                               int has_internal_subset)
{
	fprintf(fpout,"<!DOCTYPE %s ",doctypeName);
	if(pubid && *pubid) fprintf(fpout,"PUBLIC \"%s\" ",pubid);
	else if(sysid) OUTS("SYSTEM ");
	if(sysid && *sysid) fprintf(fpout,"\"%s\"",sysid);
	if(has_internal_subset) {
		UD(bEntity)=1;
		OUTS(" [\n");
	}
	nCol=0;
	Depth=1;
}

static void endDoctype(void *userData)
{
	if(UD(bEntity)) {
		OUTC(']');
		UD(bEntity)=0;
	}
	OUTS(">\n");
	nCol=Depth=0;
}

static void instruction(void *userData,
                                    const XML_Char *target,
                                    const XML_Char *data)
{
	fprintf(fpout,"<?%s %s?>\n",target,data);
}

static int trimctrl(XML_Char *s)
{
	// Replace LFs followed by multiple tabs with single spaces.
	// isgraph(): 1 if printable and not space

	XML_Char *p=s;
	int len=0;
	int bCtl=1;
	while(*s) {
		if(isgraph(*s) || *s==' ') bCtl=0;
		else {
		  if(bCtl) {
		    s++;
		    continue;
		  }
		  bCtl=1;
		  *s=' ';
		}
	    *p++=*s++;
	    len++;
	}
	*p=0;
	return len;
}

static void out_stream(char *p)
{
	//Update nCol while outputting all data --
	OUTS(p);
	while(*p) {
		if(*p++=='\n') nCol=0;
		else nCol++;
	}
}

static void chardata(void *userData,const XML_Char *s,int len)
{
	if(bNoClose) {
		OUTC('>');
		nCol++;
		bNoClose=0;
	}
	memcpy(buf,s,len);
	buf[len]=0;
	if(trimctrl(buf)) out_stream(buf);
}

static int skip_depth(void)
{
	int i;
	for(i=0;i<Depth;i++) OUTC('\t');
	return Depth;
}

static void out_wraptext(char *p)
{
	for(;*p;p++) {
		if(isspace((BYTE)*p)) {
			if(nCol>SIZ_LINEBREAK) {
			  OUTS("\n");
			  nCol=skip_depth();
			}
			else {
				OUTC(' ');
				nCol++;
			}
			while(isspace((BYTE)p[1])) p++;
		}
		else {
			OUTC(*p);
			nCol++;
		}
	}
}

static void comment(void *userData,const XML_Char *s)
{
	if(bNoClose) {
		OUTS(">\n");
		bNoClose=0;
	}
    //else if(nCol) OUTS("\n");
	if(trimctrl((char *)s)) {
		nCol=skip_depth();
		if(nCol+strlen(s)>SIZ_LINEBREAK+20) {
			OUTS("<!--\n");
			Depth++;
			nCol=skip_depth();
			out_wraptext((char *)s);
			OUTS("\n");
			Depth--;
			skip_depth();
			OUTS("-->\n");
		}
		else {
			fprintf(fpout,"<!--%s-->\n",s);
			nCol=0;
		}
	}
	//nCol=0;
}

static char * fltstr(char *fbuf,double f,int dec)
{
	//Obtains the minimal width numeric string of f rounded to dec decimal places.
	//Any trailing zeroes (including trailing decimal point) are truncated.
	BOOL bRound=TRUE;

	if(dec<0) {
		bRound=FALSE;
		dec=-dec;
	}
	if(_snprintf(fbuf,20,"%-0.*f",dec,f)==-1) {
	  *fbuf=0;
	  return fbuf;
	}
	if(bRound) {
		char *p=strchr(fbuf,'.');
		for(p+=dec;dec && *p=='0';dec--) *p--=0;
		if(*p=='.') {
		   *p=0;
		   if(!strcmp(fbuf,"-0")) strcpy(fbuf,"0");
		}
	}
	return fbuf;
}

static char * fltstr(double f,int dec)
{
	static char fbuf[40];
	return fltstr(fbuf,f,dec);
}

static char *get_coord(char *p0,double *d)
{
	char *p=p0;
	char c;

	if(*p=='-') p++;
	else{
		p=++p0;
		if(*p=='-') p++;
	}

	while(isdigit((BYTE)*p)) p++;
	if(*p=='.') {
		p++;
		while(isdigit((BYTE)*p)) p++;
	}
	c=*p;
	if(p==p0) return 0;
	*p=0;
	*d=atof(p0);
	*p=c;
	return p;
}

static void get_endpoint(SVG_ENDPOINT *ep,BOOL bNew)
{
	char *p;

	strcpy(ep->name,buf);
	ep->prefix[0]=0;

	if(!bNew && (p=strchr(buf,':'))) {
		*p++=0;
		strcpy(ep->prefix,buf);
		strcpy(ep->name,p);
	}
	ep->bNew=bNew;
}

static void find_name(SVG_ENDPOINT *ep)
{
	int recX;
	int min_dist=MAX_PTSX10_DST;
	WORD recY;

	//target coordinates --
	int ix=(int)(ep->x*10+0.5);
	int iy=(int)(ep->y*10+0.5);

	SVG_LABELKEY skey;
	char cpos[8];

	if(ix<MAX_PTSX10_DX || ix>MAX_PTSX10) goto _errname;
    sprintf(cpos+1,"%05u",ix-MAX_PTSX10_DX); *cpos=5;
	if((recX=pix->Find(cpos)) && recX!=CTRXFile::ErrMatch) goto _errname;
	cpos[5]=0;
	do {
	  if(pix->Get(&recY,&skey)) goto _errname;
	  recX=atoi((const char *)memcpy(cpos,skey.cpos,5));
	  if((recX-=ix)>MAX_PTSX10_DX) break;
	  if(abs(iy-(int)recY)>MAX_PTSX10_DX) continue;
	  recX=recX*recX+(iy-(int)recY)*(iy-(int)recY);
	  if(recX>min_dist) continue;
	  min_dist=recX;
	  memcpy(buf,skey.name,skey.len-5);
	  buf[skey.len-5]=0;
	  get_endpoint(ep,FALSE);
	}
	while(!pix->Next());
	if(min_dist<MAX_PTSX10_DST) return;

_errname:
	sprintf(buf,"_%u_",++badnamecnt);
	get_endpoint(ep,TRUE);
}

static int get_idstr(char *idstr,char *prefix,char *label,BOOL bForcePrefix)
{
	char *p0=idstr;
	char *p=prefix;
	int iLabel=0;
	*p0++='_';
	while(TRUE) {
	  if(!*p) {
		  if(iLabel) break;
		  iLabel++;
		  p=label;
		  if(p0-idstr>1) *p0++='.';
		  continue;
	  }
	  if(*p>='A' && *p<='Z' || *p>='a' && *p<='z' || *p>='0' && *p<='9') *p0++=*p;
	  else p0+=sprintf(p0,"-%02x",(UINT)*(unsigned char *)p);
	  p++;
	}
	*p0=0;

	iLabel=p0-idstr;
	if(!bForcePrefix && !isdigit((BYTE)idstr[1]) && idstr[1]!='-') {
		memmove(idstr,idstr+1,iLabel--);
	}
	return iLabel;
}

static BOOL is_hex(int c)
{
	return isdigit((BYTE)c) || (c>='a' && c<='f');
}

static int get_name_from_id_component(char *rnam, const char *pnam)
{
	UINT uhex;
	while(*pnam) {
		if(*pnam=='-') {
			if(!is_hex(pnam[1]) || !is_hex(pnam[2]) ||
				1!=sscanf(pnam+1,"%02x",&uhex)) return SVG_ERR_BADIDSTR;
			pnam+=3;
			*rnam++=(byte)uhex;
		}
		else *rnam++=*pnam++;
	}
	*rnam=0;
	return 0;
}

static int get_ep_name(const char *pnam,SVG_ENDPOINT *ep)
{
	int e=0;
	char *p=strchr(pnam,'.');
	if(!p) ep->prefix[0]=0;
	else {
	   *p++=0;
	   e=get_name_from_id_component(ep->prefix,pnam);
	   pnam=p;
	}
	if(!e) e=get_name_from_id_component(ep->name,pnam);
	return e;
}

static int get_ep_names(const char *ids,SVG_ENDPOINT *ep0,SVG_ENDPOINT *ep1)
{
	char idbuf[SVG_MAX_SIZNAME+1];
	char *p1,*p0=idbuf;
	int i;

	if(strlen(ids)>SVG_MAX_SIZNAME) return SVG_ERR_BADIDSTR;
	strcpy(p0,ids);
	if(*p0=='_') p0++;
	if(!(p1=strchr(p0,'_'))) return SVG_ERR_BADIDSTR;
	*p1++=0;
	if(get_ep_name(p0,ep0) || get_ep_name(p1,ep1) ||
		(i=strcmp(ep0->prefix,ep1->prefix))>0 ||
		(i==0 && strcmp(ep0->name,ep1->name)>=0)) return SVG_ERR_BADIDSTR;
	return 0;
}

static BOOL chk_idkey(const char *ids,SVG_ENDPOINT *ep0,SVG_ENDPOINT *ep1)
{
	char key[SVG_MAX_SIZNAME+1];
	char *p=key+1;

	if(ids && get_ep_names(ids,ep0,ep1)) {
		badvecidcnt++;
		return FALSE;
	}

	p+=strlen(strcpy(p,ep0->prefix)); *p++=0;
	p+=strlen(strcpy(p,ep0->name)); *p++=0;
	p+=strlen(strcpy(p,ep1->prefix)); *p++=0;
	p+=strlen(strcpy(p,ep1->name));
	*key=(char)(p-key-1);
	if(pix_chk->Find(key)) return FALSE;
	//Finally, check vector orientation
	return TRUE;
}

static void out_vector_path(const char *ids,const char *style,SVG_ENDPOINT *ep0,SVG_ENDPOINT *ep1)
{
	int i;
	char idstr[SVG_MAX_SIZNAME+1];

	if(ids) {
		strcpy(idstr,(char *)ids);
		if(!strcmp(idstr,"F21_F23")) {
			i=0;
		}
	}
	else {
		if(ep0->bNew) i=strlen(strcpy(idstr,ep0->name));
		else i=get_idstr(idstr,ep0->prefix,ep0->name,FALSE);
		if(ep1->bNew) {
			idstr[i++]='_';
			strcpy(idstr+i,ep1->name);
		}
		else get_idstr(idstr+i,ep1->prefix,ep1->name,TRUE);
	}
	skip_depth();
	OUTS("<path id=\"");
	OUTS(idstr);
	OUTS("\" style=\"");
	if(ep0->bNew || ep1->bNew) {
		OUTS("stroke:blue;stroke-width:1");
		badveccnt++;
	}
	else if(!chk_idkey(ids,ep0,ep1)) {
		OUTS("stroke:green;stroke-width:1");
		badveccnt++;
	}
	else OUTS(style);
	OUTS("\" d=\"M");
	OUTS(fltstr(ep0->x,2));
	OUTC(',');
	OUTS(fltstr(ep0->y,2));
	OUTC('l');
	OUTS(fltstr(ep1->x-ep0->x,2));
	if(ep1->y>=ep0->y) OUTC(',');
	OUTS(fltstr(ep1->y-ep0->y,2));
	OUTS("\"/>\n");
}

static int out_vector_paths(const char *idstr,const char *style,char *data)
{
	int i;
	SVG_ENDPOINT ep0,ep1;
	BOOL bRelative;
	BOOL bStarted=FALSE;

	if(*data!='M') {
		return SVG_ERR_PARSEPATH;
	}
	if(!(data=get_coord(data,&ep0.x)) || !(data=get_coord(data,&ep0.y))) {
		return SVG_ERR_PARSEPATH;
	}
	while(isspace((BYTE)*data)) data++;

	if(!idstr) find_name(&ep0);
	else ep0.bNew=FALSE;

	while(*data) {
		if(bStarted) {
			if(idstr) return SVG_ERR_POLYPATH;
			ep0=ep1;
		}
		else bStarted=TRUE;

		//data is postioned at type char for TO station's coordinates --
		if(!strchr("lLvVhH",*data)) {
			return SVG_ERR_PARSEPATH;
		}
		bRelative=islower(*data);

		switch(toupper(*data)) {
			case 'L' :
				if(!(data=get_coord(data,&ep1.x)) || !(data=get_coord(data,&ep1.y))) {
					return SVG_ERR_PARSEPATH;
				}
				if(bRelative) {
					ep1.x+=ep0.x; ep1.y+=ep0.y;
				}
				break;
			case 'V' :
				ep1.x=ep0.x;
				if(!(data=get_coord(data,&ep1.y))) {
					return SVG_ERR_PARSEPATH;
				}
				if(bRelative) ep1.y+=ep0.y;
				break;
			case 'H' :
				ep1.y=ep0.y;
				if(!(data=get_coord(data,&ep1.x))) {
					return SVG_ERR_PARSEPATH;
				}
				if(bRelative) ep1.x+=ep0.x;
				break;
		}

		while(isspace((BYTE)*data)) data++;

		if(!idstr) {
			find_name(&ep1);
			i=strcmp(ep0.prefix,ep1.prefix);
		}
		else {
			ep1.bNew=FALSE;
			i=-1;
		}

		if(i<0 || (i==0 && strcmp(ep0.name,ep1.name)<=0)) {
			out_vector_path(idstr,style,&ep0,&ep1);
		}
		else {
			out_vector_path(idstr,style,&ep1,&ep0);
		}
		veccnt++;
	}
	return 0;
}

static void startElement(void *userData, const char *el, const char **attr)
{
  if(UD(errcode)) return;

  int i=UD(bEntity);

  if(!i) {
	  if(!strcmp(el,"g") && attr[0] && !strcmp(attr[0],"id") &&
		  strlen(attr[1])>=11 && !memcmp(attr[1],"w2d_Vectors",11)) UD(bEntity)=1;
  }
  else {
	  i=(attr[1] && !strcmp(attr[0],"id"))?2:0;
	  if(strcmp(el,"path") || !attr[i] || strcmp(attr[i],"style") || !attr[i+2] || strcmp(attr[i+2],"d")) {
		  UD(errcode)=SVG_ERR_NOPATH;
		  UD(errline)=XML_GetCurrentLineNumber(parser);
		  return;
	  }
	  if(i=out_vector_paths((i?attr[1]:NULL),attr[i+1],(char *)attr[i+3])) {
		  UD(errcode)=i;
		  UD(errline)=XML_GetCurrentLineNumber(parser);
	  }
	  return;
  }

  if(strcmp(el,"tspan")) {
	  //Normal output --
	  if(bNoClose) {
		OUTS(">\n");
		bNoClose=0;
	  }
	  else if(nCol) OUTS("\n");
	  nCol=skip_depth();
	  Depth++;
  }
  else if(bNoClose) {
	  OUTC('>');
	  nCol++;
	  bNoClose=0;
  }

  nCol+=fprintf(fpout,"<%s", el);

  for (i=0;attr[i];i+=2) {
	el=attr[i];
	if(nCol>80) {
		OUTS("\n");
		nCol=skip_depth();
	}
	else {
		OUTC(' ');
		nCol++;
	}
	el=attr[i+1];
	trimctrl((char *)attr[i+1]);
	sprintf(buf,"%s=\"%s\"",attr[i],attr[i+1]);
	out_wraptext(buf);
  }
  if(UD(bEntity)) {
	OUTS(">\n");
	bNoClose=0;
  }
  else bNoClose=1;
}

static void endElement(void *userData, const char *el)
{
  if(UD(errcode)) return;

  if(UD(bEntity)) {
	  if(strcmp(el,"g")) {
		  if(strcmp(el,"path")) {
			  UD(errcode)=SVG_ERR_UNKNOWN;
			  UD(errline)=XML_GetCurrentLineNumber(parser);
		  }
		  return;
	  }
	  //leaving the group --
	  UD(bEntity)=0;
  }

  if(strcmp(el,"tspan")) {
	  Depth--;
	  if(XML_GetCurrentByteCount(parser)==0) OUTS("/>\n");
	  else {
		if(bNoClose) {
			  OUTS(">\n");
			  nCol=0;
		}
		if(!nCol) skip_depth();
		fprintf(fpout,"</%s>\n",el);
	  }
	  bNoClose=nCol=0;
  }
  else {
	  if(bNoClose) {
		  OUTC('>');
		  nCol++;
		  bNoClose=0;
	  }
	  OUTS("</tspan>");
	  nCol+=8;
  }
}

static void index_startElement(void *userData, const char *el, const char **attr)
{
  if(UD(errcode)) return;

  int i=UD(bEntity);

  if(!i) {
	  if(strcmp(el,"g") || !attr[0] || strcmp(attr[0],"id") ||
		  strlen(attr[1])<10 || memcmp(attr[1],"w2d_Labels",10)) return;
	  UD(bEntity)=1;
	  return;
  }

  if(i==1) {
	  //looking for text element or labeled group --
	  if(!strcmp(el,"text")) {
		  if(attr[0] && !strcmp(attr[0],"transform")) {
			  double d1,d2,d3,d4;
			  if(6==sscanf(attr[1],"matrix(%lf %lf %lf %lf %lf %lf",&d1,&d2,&d3,&d4,&xpos,&ypos)) {
				  xpos=xpos+LABEL_XOFF;
				  ypos=ypos+LABEL_YOFF;
				  UD(bEntity)=2;
				  return;
			  }
		  }
		  else if(bLabelID && attr[0] && attr[2] && !strcmp(attr[2],"x") &&
			  attr[4] && !strcmp(attr[4],"y")) {
			  xpos=atof(attr[3]);
			  ypos=atof(attr[5]);
			  UD(bEntity)=3;
			  namelen=0;
		  }
		  else UD(errcode)=SVG_ERR_TRANSFORM;
	  }
	  //Labels may have IDs --
	  else if(!strcmp(el,"g") && attr[0] || !strcmp(attr[0],"id")) {
		  bLabelID=TRUE;
	  }
	  else UD(errcode)=SVG_ERR_NOTEXT;
  }
  //looking for tspan element --
  else if(i==2 && !strcmp(el,"tspan")) {
	  UD(bEntity)=3;
	  namelen=0;
  }
  else UD(errcode)=SVG_ERR_NOTSPAN;
  if(UD(errcode)) UD(errline)=XML_GetCurrentLineNumber(parser);
}

static int get_raw_name(char *pdest,char *psrc)
{
	int len=0;
	while(*psrc) {
		if(*psrc=='&') {
			psrc++;
			if(!strcmp(psrc,"amp;")) {*pdest++='&'; psrc+=4;}
			else if(!strcmp(psrc,"gt;")) {*pdest++='>'; psrc+=3;}
			else if(!strcmp(psrc,"lt;")) {*pdest++='<'; psrc+=3;}
			else if(!strcmp(psrc,"quot;")) {*pdest++='\"'; psrc+=5;}
			else if(!strcmp(psrc,"apos;")) {*pdest++='\''; psrc+=5;}
			else *pdest++='&';
		}
		else *pdest++=*psrc++;
		len++;
	}
	*pdest=0;
	return len;
}

static int index_name()
{
	SVG_LABELKEY key;
	WORD rec;

	if(!namelen) return SVG_ERR_LABELZERO;
	namecnt++;
	int ix=(int)(xpos*10.0+0.5);
	int iy=(int)(ypos*10.0+0.5);
	if(ix<0 || ix>MAX_PTSX10 || iy<0 || iy>MAX_PTSX10) return SVG_ERR_LABELPOS;
	rec=(WORD)iy;
	sprintf(key.cpos,"%05u",ix);
	key.len=5+get_raw_name(key.name,buf);
	namelen=0;
	return pix->InsertKey(&rec,&key);
}

static void index_endElement(void *userData, const char *el)
{
  int e=0;

  if(UD(errcode)) return;

  switch(UD(bEntity)) {
	case 0: return;
	case 1:
		if(strcmp(el,"g")) e=SVG_ERR_ENDGROUP;
		break;
	case 2:
		if(bLabelID) {
			if(strcmp(el,"g")) e=SVG_ERR_ENDGROUP;
			bLabelID=FALSE;
		}
		else if(strcmp(el,"text")) e=SVG_ERR_ENDTEXT;
		break;

	case 3:
		if(bLabelID) {
			if(strcmp(el,"text")) e=SVG_ERR_ENDTEXT;
		}
		else if(strcmp(el,"tspan")) e=SVG_ERR_ENDTSPAN;
		if(!e) e=index_name();
		break;

	default: e=SVG_ERR_UNKNOWN;
  }

  if(e) {
	  UD(errcode)=e;
	  UD(errline)=XML_GetCurrentLineNumber(parser);
  }
  else UD(bEntity)--;
}

static void index_chardata(void *userData,const XML_Char *s,int len)
{
	//bEntity can be 2 or 3 depending on tspan presence --
	if(UD(errcode) || UD(bEntity)<3) return;
	if(namelen+len>=SVG_MAX_SIZNAME) {
		UD(errcode)=SVG_ERR_SIZNAME;
	    UD(errline)=XML_GetCurrentLineNumber(parser);
		return;
	}
	memcpy(buf+namelen,s,len);
	buf[namelen+=len]=0;
}

static int parse_file()
{
	int e=0;

	bNoClose=nCol=0;
	udata.bEntity=0;
	udata.errcode=0;
	udata.errline=0;
	parser_ErrorLineNumber=0;
	XML_SetUserData(parser,&udata);
	XML_SetParamEntityParsing(parser,XML_PARAM_ENTITY_PARSING_NEVER);
	XML_SetPredefinedEntityParsing(parser,XML_PREDEFINED_ENTITY_PARSING_NEVER);
	XML_SetGeneralEntityParsing(parser,XML_GENERAL_ENTITY_PARSING_NEVER);

	for (;;) {
		int done;
		int len;

		len=fread(Buff, 1, BUFFSIZE, fpin);
		if(ferror(fpin)) {
			e=SVG_ERR_XMLREAD;
			break;
		}
		done=feof(fpin);
		if (!XML_Parse(parser, Buff, len, done)) {
			parser_ErrorLineNumber=XML_GetCurrentLineNumber(parser);
			parser_ErrorCode=XML_GetErrorCode(parser);
			e=SVG_ERR_XMLPARSE;
			break;
		}
		if(udata.errcode) {
			parser_ErrorLineNumber=udata.errline;
			e=udata.errcode;
			break;
		}
		if(done) break;
	}
	XML_ParserFree(parser);
	parser=NULL;
	return e;
}

static int index_names()
{
  int e;
  parser=XML_ParserCreate(NULL);
  if(!parser) return SVG_ERR_PARSERCREATE;
  XML_SetElementHandler(parser,index_startElement,index_endElement);
  XML_SetCharacterDataHandler(parser,index_chardata);
  namecnt=0;
  strcpy(trx_Stpext(strcpy(buf,fpout_name)),".$$$");
  //word-sized records --
  pix=new CTRXFile();
  if(pix->Create(buf,2,CTRXFile::ReadWrite) || pix->AllocCache(32)) e=SVG_ERR_TEMPINIT;
  else e=parse_file();
  return e;
}

static void finish(int e)
{
	if(pix_chk) {
		pix_chk->Close();
		delete pix_chk;
	}
	if(pix) {
		pix->CloseDel();
		delete pix;
	}
	if(fpin) fclose(fpin);
	if(fpout) fclose(fpout);
	if(parser) XML_ParserFree(parser);
	if(e>0) printf(ErrMsg(e));
	exit(e);
}

void main(int argc, char *argv[])
{
  int e;

  if(argc<3) {
	  printf("w2dname v1.0 - Convert Polylines to Named Vectors\n\n"
		     "Command: w2dname <input pathname>[.SVG] <output pathname>[.SVG]\n");
	  exit(-1);
  }
  
  if(!(fpin=fopen(strcpy(fpin_name,argv[1]),"rt"))) {
      finish(SVG_ERR_XMLOPEN);
  }
  strcpy(fpout_name,argv[2]);

  if(e=index_names()) finish(e);

  if(fseek(fpin,0,SEEK_SET)) finish(SVG_ERR_XMLOPEN);

  parser = XML_ParserCreate(NULL);
  if (!parser) finish(SVG_ERR_PARSERCREATE);
  XML_SetXmlDeclHandler(parser,xmldecl);
  XML_SetElementHandler(parser,startElement,endElement);
  XML_SetProcessingInstructionHandler(parser,instruction);
  XML_SetCharacterDataHandler(parser,chardata);
  XML_SetCommentHandler(parser,comment);
  //XML_SetDefaultHandler(parser,defhandler);
  XML_SetDoctypeDeclHandler(parser,startDoctype,endDoctype);
  XML_SetEntityDeclHandler(parser,entity);

  if(!(fpout=fopen(fpout_name,"wt"))) {
      finish(SVG_ERR_SVGCREATE);
  }
  
  veccnt=badveccnt=badnamecnt=badvecidcnt=0;
  pix_chk=new CTRXFile();
  strcpy(buf,fpin_name);
  strcpy(trx_Stpext(buf),"_w2d");
  if(pix_chk->Open(buf)) e=SVG_ERR_INDEXOPEN;
  else {
	pix_chk->SetExact(TRUE);
	e=parse_file();
  }

  if(!e) printf("Completed - Labels: %d  Vecs: %d  Bad vecs: %d  Bad names: %d  Bad IDs: %d\n",
	  namecnt,veccnt,badveccnt,badnamecnt,badvecidcnt);
  finish(e);
}  /* End of main */

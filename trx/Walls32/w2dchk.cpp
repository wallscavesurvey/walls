#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "expat.h"
#include "trx_str.h"
#include "trxfile.h"

#define PI 3.141592653589793
#define U_DEGREES (PI/180.0)

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
	char prefix[SVG_MAX_SIZNAME+1];
	char name[SVG_MAX_SIZNAME+1];
} SVG_ENDPOINT;

CTRXFile *pix_chk=NULL;

static char Buff[BUFFSIZE];
static char buf[BUFFSIZE+2];
static char argv_buf[2*_MAX_PATH];
static char fpin_name[_MAX_PATH],fpout_name[_MAX_PATH];
#define SIZ_REFBUF 384
static char refbuf[SIZ_REFBUF];
static UINT reflen;

typedef struct {
	int bEntity;
	int errcode;
	int errline;
} TYP_UDATA;

static double scale,cosA,sinA,midE,midN,xMid,yMid;

static double xpos,ypos;
static int textlen,veccnt,badveccnt,badvecidcnt;
static TYP_UDATA udata;
static FILE *fpin,*fpout;

static UINT uLevel;
static int Depth,bNoClose,nCol,iLevel;
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
  SVG_ERR_SIZREFBUF,
  SVG_ERR_REFFORMAT,
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
  "Reference string too long",
  "Unrecognized w2d_Ref format",
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

static double *get_xy(double *xy,double *xyz)
{
	//Convert to page coordinates and check if in rectange --
	// x'   = (E-midE)*cosA - (N-midN)*sinA
	// y'   = -(E-midE)*sinA - (N-midN)*cosA
	double x=scale*(xyz[0]-midE);
	double y=scale*(xyz[1]-midN);
	//(x,y) = page coordinates of point wrt center of page
	xy[0]=x*cosA-y*sinA+xMid;
	xy[1]=-x*sinA-y*cosA+yMid;
	return xy;
}

static int parse_georef()
{
	LPCSTR p;
	//Tamapatz Project  Plan: 90  Scale: 1:75  Frame: 17.15x13.33m  Center: 489311.3 2393778.12
	if(reflen && (p=strstr(refbuf,"  Plan: "))) {
		double view;
		int numval=sscanf(p+8,"%lf  Scale: 1:%lf  Frame: %lfx%lfm  Center: %lf %lf",
			&view,&scale,&xMid,&yMid,&midE,&midN);
		if(numval==6 && scale>0.0) {
			//assign values --
			scale=(72*12.0/0.3048)/scale;
			xMid*=(0.5*scale); yMid*=(0.5*scale);
			view*=U_DEGREES;
			sinA=sin(view); cosA=cos(view);
			return 0;
		}
	}
	return SVG_ERR_REFFORMAT;
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

	if(iLevel==2) {
		if(reflen+len>=SIZ_REFBUF) {
		  UD(errcode)=SVG_ERR_SIZREFBUF;
		  UD(errline)=XML_GetCurrentLineNumber(parser);
		  return;
		}
		strcpy(refbuf+reflen,buf);
		reflen+=len;
	}
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

static void get_endpoint(SVG_ENDPOINT *ep)
{
	char *p;

	strcpy(ep->name,buf);
	ep->prefix[0]=0;

	if(p=strchr(buf,':')) {
		*p++=0;
		strcpy(ep->prefix,buf);
		strcpy(ep->name,p);
	}
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
	if(pix_chk->Find(key)) {
		return FALSE;
	}
	//Finally, check vector orientation
	return TRUE;
}

static int out_vector_path(const char *idstr,const char *style,char *data)
{
	SVG_ENDPOINT ep0,ep1;
	BOOL bPoint;

	if(*data!='M') {
		return SVG_ERR_PARSEPATH;
	}
	if(!(data=get_coord(data,&ep0.x)) || !(data=get_coord(data,&ep0.y))) {
		return SVG_ERR_PARSEPATH;
	}
	while(isspace((BYTE)*data)) data++;

	//data is postioned at type char for TO station's coordinates --
	bPoint=(*data==0);

	if(!bPoint) {
		if(!strchr("lLvVhH",*data)) {
			return SVG_ERR_PARSEPATH;
		}
		BOOL bRelative=islower(*data);

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
	}

	skip_depth();
	OUTS("<path id=\"");
	OUTS(idstr);
	OUTS("\" style=\"");
	if(bPoint) {
		OUTS("stroke:blue;stroke-width:10");
		badveccnt++;
	}
	else if(!chk_idkey(idstr,&ep0,&ep1)) {
		OUTS("stroke:green;stroke-width:1");
		badveccnt++;
	}
	else OUTS(style);
	OUTS("\" d=\"M");
	OUTS(fltstr(ep0.x,2));
	OUTC(',');
	OUTS(fltstr(ep0.y,2));
	if(!bPoint) {
		OUTC('l');
		OUTS(fltstr(ep1.x-ep0.x,2));
		if(ep1.y>=ep0.y) OUTC(',');
		OUTS(fltstr(ep1.y-ep0.y,2));
	}
	OUTS("\"/>\n");
	veccnt++;
	return 0;
}

static void startElement(void *userData, const char *el, const char **attr)
{
  int i;
  if(UD(errcode)) return;

  if(!iLevel) {
	  if(!strcmp(el,"g") && attr[0] && !strcmp(attr[0],"id")) {
		if(!strcmp(attr[1],"w2d_Ref")) iLevel=1;
		else if(strlen(attr[1])>=11 && !memcmp(attr[1],"w2d_Vectors",11)) iLevel=-1;
	  }
  }
  else if(iLevel==1) {
	  if(!strcmp(el,"text")) {
		  iLevel=2;
		  reflen=0;
	  }
	  else {
		  UD(errcode)=SVG_ERR_NOTEXT;
		  UD(errline)=XML_GetCurrentLineNumber(parser);
		  return;
	  }
  }
  else if(iLevel==-1) {
	  if(strcmp(el,"path") || !attr[0] || strcmp(attr[0],"id") || !attr[2] ||
		  strcmp(attr[2],"style") || !attr[4] || strcmp(attr[4],"d")) {
		  UD(errcode)=SVG_ERR_NOPATH;
		  UD(errline)=XML_GetCurrentLineNumber(parser);
		  return;
	  }
	  if(i=out_vector_path(attr[1],attr[3],(char *)attr[5])) {
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
  int e;
  if(UD(errcode)) return;

  if(iLevel<0) {
	  if(strcmp(el,"g")) {
		  if(strcmp(el,"path")) {
			  UD(errcode)=SVG_ERR_UNKNOWN;
			  UD(errline)=XML_GetCurrentLineNumber(parser);
		  }
		  return;
	  }
	  //leaving the group --
	  iLevel=0;
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
	  if(iLevel==2 && !strcmp(el,"text")) {
		  iLevel=0;
		  if(e=parse_georef()) {
			  UD(errcode)=e;
			  UD(errline)=XML_GetCurrentLineNumber(parser);
			  return;
		  }
	  }
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

static int get_raw_text(char *pdest,char *psrc)
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

static void finish(int e)
{
	if(pix_chk) {
		pix_chk->Close();
		delete pix_chk;
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
	  printf("w2dchk v1.0 - Check w2d_Vector groups in SVG against Walls index.\n\n"
		     "Command: w2dchk <input pathname>[.SVG] <output pathname>[.SVG]\n"
			 "Note: Index <input pathname>_w2d.trx must be present.");
	  exit(-1);
  }
  
  if(!(fpin=fopen(strcpy(fpin_name,argv[1]),"rt"))) {
      finish(SVG_ERR_XMLOPEN);
  }
  strcpy(fpout_name,argv[2]);

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
  
  veccnt=badveccnt=badvecidcnt=0;
  pix_chk=new CTRXFile();
  strcpy(buf,fpin_name);
  strcpy(trx_Stpext(buf),"_w2d");
  if(pix_chk->Open(buf) || pix_chk->AllocCache(32)) e=SVG_ERR_INDEXOPEN;
  else {
	pix_chk->SetExact(TRUE);
	iLevel=0;
	e=parse_file();
  }

  if(!e) printf("Completed - Vecs: %d  Bad vecs: %d  Bad IDs: %d\n",veccnt,badveccnt,badvecidcnt);
  finish(e);
}  /* End of main */

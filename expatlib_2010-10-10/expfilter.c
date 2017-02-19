#include <stdio.h>
#include "expat.h"
#include <string.h>
#include <ctype.h>

#define BUFFSIZE	8192
#define SIZ_LINEBREAK 80

static char Buff[BUFFSIZE];
static char buf[BUFFSIZE+2];

typedef struct {
	int bHeader;
	int bEntity;
} TYP_UDATA;

static TYP_UDATA udata;
static FILE *fpin,*fpout;

static int Depth,bNoClose,nCol;
static XML_Parser parser;

#define OUTC(c) putc(c,fpout)
#define OUTS(s) fputs(s,fpout)
#define UD(t) ((TYP_UDATA *)userData)->t

static void entity (void *userData,
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
}

static void xmldecl(void *userData,
                       const XML_Char  *version,
                       const XML_Char  *encoding,
                       int             standalone)
{
}

static void defhandler(void *userData,
                            const XML_Char *s,
                            int len)
{
	if(!UD(bHeader)) {
		memcpy(buf,s,len);
		buf[len]=0;
		s=buf;
		fprintf(fpout,"%s\n",buf);
		UD(bHeader)=1;
	}
}

static int trimctrl(XML_Char *s)
{
	// Replace whitespace spans with single spaces.
	// isgraph(): 1 if printable and not space

	XML_Char *p=s;
	int len=0;
	int bCtl=1;
	while(*s) {
		if(isgraph(*s)) bCtl=0;
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

static void chardata(void *userData,const XML_Char *s,int len)
{
	char *p=buf;
	if(bNoClose) {
		OUTC('>');
		nCol++;
		bNoClose=0;
	}
	memcpy(buf,s,len);
	buf[len]=0;
	if(trimctrl(buf)) OUTS(buf);
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
		if(*p==' ' && nCol>SIZ_LINEBREAK) {
			  OUTS("\n");
			  nCol=skip_depth();
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
    else if(nCol) OUTS("\n");
	if(trimctrl((char *)s)) {
		nCol=skip_depth();
		if(nCol+strlen(s)>SIZ_LINEBREAK) {
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
		}
	}
	nCol=0;
}


static void startElement(void *data, const char *el, const char **attr)
{
  int i;

  if(bNoClose) {
	OUTS(">\n");
	bNoClose=0;
  }
  else if(nCol) OUTS("\n");

  nCol=skip_depth();
  nCol+=fprintf(fpout,"<%s", el);

  for (i=0;attr[i];i+=2) {
	el=attr[i];
	if(nCol>SIZ_LINEBREAK) {
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
  bNoClose=1;
  Depth++;
}

static void endElement(void *data, const char *el)
{
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

static void finish(int e)
{
  if(fpin) fclose(fpin);
  if(fpout) fclose(fpout);
  XML_ParserFree(parser);
  exit(e);
}

void main(int argc, char *argv[])
{
  if(argc<3) {
	  printf("Expfilter v1.0 -- Copy parsed XML file\n\n"
		     "Command: expfilter <input pathname> <output pathname>\n");
	  exit(-1);
  }
  
  parser = XML_ParserCreate(NULL);
  if (!parser) {
    printf("Couldn't allocate memory for parser\n");
    exit(-1);
  }
  bNoClose=nCol=0;
  udata.bEntity=0;
  udata.bHeader=0;
  XML_SetUserData(parser,&udata);
  //XML_SetXmlDeclHandler(parser,xmldecl);
  XML_SetElementHandler(parser,startElement,endElement);
  XML_SetProcessingInstructionHandler(parser,instruction);
  XML_SetCharacterDataHandler(parser,chardata);
  XML_SetCommentHandler(parser,comment);
  XML_SetDefaultHandler(parser,defhandler);
  XML_SetDoctypeDeclHandler(parser,startDoctype,endDoctype);
  XML_SetEntityDeclHandler(parser,entity);
  XML_SetParamEntityParsing(parser,XML_PARAM_ENTITY_PARSING_NEVER);
  XML_SetPredefinedEntityParsing(parser,XML_PREDEFINED_ENTITY_PARSING_NEVER);
  XML_SetGeneralEntityParsing(parser,XML_GENERAL_ENTITY_PARSING_NEVER);

  if(!(fpin=fopen(argv[1],"rt"))) {
      printf("Can't open file: %s\n",argv[1]);
      finish(-1);
  }

  if(!(fpout=fopen(argv[2],"wt"))) {
      printf("Can't create file: %s\n",argv[2]);
      finish(-1);
  }

  for (;;) {
    int done;
    int len;

    len = fread(Buff, 1, BUFFSIZE, fpin);
    if (ferror(fpin)) {
      printf("Read error\n");
      finish(-1);
    }
    done = feof(fpin);

    if (!XML_Parse(parser, Buff, len, done)) {
      printf("Parse error at line %d:\n%s\n",
	      XML_GetCurrentLineNumber(parser),
	      XML_ErrorString(XML_GetErrorCode(parser)));
      finish(-1);
    }

    if (done) break;
  }
  printf("Successful parse.\n");
  finish(0);
}  /* End of main */

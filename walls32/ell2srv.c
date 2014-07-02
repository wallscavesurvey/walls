/*ELL2SRV.C -- Survey file conversion*/

#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <trx_str.h>

#define LINLEN 256

static FILE *fp,*fpto;
static  char ln[LINLEN];

static char * trimTB(char *p)
{
  int len=strlen(p);
  while(len && p[len-1]==' ') len--;
  p[len]=0;
  return p;
}

static int CALLBACK line_fcn(NPSTR buf,int sizbuf)
{
  int len;
  if(NULL==fgets(buf,sizbuf,fp)) return CFG_ERR_EOF;
  len=strlen(buf);
  if(len && buf[len-1]=='\n') buf[--len]=0;
  return len;
}

static char *get_units_spec(char *pLine)
{
  /*For now, change only the delimeters --*/
  char *p=pLine;
  static char buf[80];

  _snprintf(buf,80,"#Units Order=VDA %s",p+1);
  if(p=strchr(buf,')')) *p=0;
  return buf;
}

static void print_dflt_units(void)
{
  fprintf(fpto,"#Units Meters Order=VDA\n");
}

static void print_title(void)
{
  char *p=strstr(cfg_commptr," LENGTH");
  char *pLen=NULL;
  if(p) {
    pLen=p+1;
    while(p>=cfg_commptr && *p==' ') p--;
    *(p+1)=0;
  }
  fprintf(fpto,";%s",cfg_commptr);
  if(pLen) fprintf(fpto,"\n;%s",pLen);
}

int main(int argc,char **argv)
{
  int e;
  char *p;
  WORD linecount,filecount=0,bUnits=0;
  struct _find_t ft;

  char frpath[_MAX_PATH];

  if(argc!=2) {
    printf("ELL2SRV 1.0 - Convert Files from Ellipse to WALLS Format\n\n"
           "USAGE: ELL2SRV <pathspec>\n"
           "       Converts matching .ELI files to .SRV files.\n");

    return 0;
  }

  p=_NCHR(trx_Stpext(_strupr(strcpy(frpath,argv[1]))));
  strcpy(p,".ELI");

  if(_dos_findfirst(frpath,_A_NORMAL,&ft)) {
    printf("No files matching %s\n",frpath);
    return 1;
  }
  p=_NCHR(trx_Stpnam(frpath));

  while(TRUE) {
     strcpy(p,ft.name);
     if((fp=fopen(frpath,"r"))==NULL) {
        printf("Cannot open file %s.\n",frpath);
        return 1;
     }
     strcpy(_NCHR(trx_Stpext(frpath)),".SRV");

     if((fpto=fopen(frpath,"w"))==NULL) {
       printf("Cannot create file %s.\n",frpath);
       return 1;
     }

     printf("Writing %s..",frpath);
     filecount++;

  cfg_LineInit(line_fcn,ln,LINLEN,CFG_PARSE_NONE);

  /*Assume .ELI file comments are delimited by '/' --*/
  cfg_commchr='/';

  linecount=0;
  while((e=cfg_GetLine())>=0) {
    linecount++;
    if(cfg_argc) {
      e=*cfg_argv[0];
      if(e=='+') *cfg_argv[0]=' '; /*Delete closure flag*/
      else if(e=='(') {
        /*Use "[..]" instead of "(..)" --*/
        fprintf(fpto,"%s\n",get_units_spec(cfg_argv[0]));
        bUnits=TRUE;
        continue;
       }

      if(!bUnits) {
        print_dflt_units();
        bUnits=TRUE;
      }

      /*For appearances sake, we parse the line to replace whitespace
        (or comma) separators with single tab characters. A null
        argument implies that an extra comma was used to signify
        a missing value. Although not expected in .ELI files, we
        handle the possibility by inserting a comma. The same goes for
        quoted strings. Thus, we will accept a name like "DATUM 4".*/

      cfg_GetArgv(ln,CFG_PARSE_ALL);
      for(e=0;e<cfg_argc;e++) {
        if(e) fprintf(fpto,"\t");
        if(!*cfg_argv[e]) fprintf(fpto,",");
        else if(strchr(cfg_argv[e],' ')) fprintf(fpto,"\"%s\"",cfg_argv[e]);
        else fprintf(fpto,cfg_argv[e]);
      }
    }

    if(cfg_commptr) {
      if(!cfg_argc && linecount==1) {
        /*Print title and units*/
        print_title();
      }
      else {
        if(*trimTB(cfg_commptr)) {
          if(cfg_argc) fprintf(fpto,"\t");
          fprintf(fpto,";%s",cfg_commptr);
        }
      }
    }
    fprintf(fpto,"\n");
  }
  fclose(fpto);
  printf("\nLines written: %u\n",linecount);
  fclose(fp);
  if(_dos_findnext(&ft)) {
    printf("\nFiles processed: %d\n",filecount);
    break;
  }
  }
  return 0;
}


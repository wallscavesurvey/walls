/*INCSRV.C v1.0 -- Walls Survey file correction*/

#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <trx_str.h>
#include <math.h>

#define LINLEN 256

static FILE *fp,*fpto;
static char *pSrc,*pDst;
static char ln[LINLEN];
static char lnbuf[LINLEN];
static char frpath[_MAX_PATH];
static char topath[_MAX_PATH];
static double iarg[3];
static char * i_str[6]={"DAV","DVA","ADV","AVD","VDA","VAD"};
static int i_seq[6][3]={{0,1,2},{0,2,1},{1,0,2},{1,2,0},{2,0,1},{2,1,0}};
int *p_seq=(int *)i_seq[0];
static int nDec=2;
static WORD linecount;

static int getline()
{
  int len;
  if(NULL==fgets(ln,LINLEN,fp)) return CFG_ERR_EOF;
  len=strlen(ln);
  if(len && ln[len-1]=='\n') ln[--len]=0;
  return len;
}

static void skipspc(void)
{
	while(*pSrc && (*pSrc==' '|| *pSrc=='\t')) *pDst++=*pSrc++;
}

static void skiparg(void)
{
	while(*pSrc && *pSrc!=' ' && *pSrc!='\t') *pDst++=*pSrc++;
	skipspc();
}

static void incarg(int i)
{
	char buf[40];
	double d;
	
    if(!*pSrc || iarg[i]==0.0) {
      skiparg();
      return;
    }
    d=atof(pSrc)+iarg[i];
    if(i==1) {
      if(d<0.0) d+=360.0;
      else if(d>=360.0) d-=360.0;
    }
    i=sprintf(buf,"%.*f",nDec,d);
    while(i>1 && buf[i-1]=='0') i--;
    if(buf[i-1]=='.') i--;
    if(i) buf[i]=0;
    else {
      strcpy(buf,"0");
      i=1;
    }
    strcpy(pDst,buf);
    pDst+=i;
	while(*pSrc && *pSrc!=' ' && *pSrc!='\t') pSrc++;
	skipspc();
}    
    

int main(int argc,char **argv)
{
    int e;
    char *p,*pe;
    BOOL bFixed;

    *frpath=0;
    *topath=0;
    iarg[0]=iarg[1]=iarg[2]=0.0;
    bFixed=FALSE;
    for(e=1;e<argc;e++) {
      p=argv[e];
      if(*p=='-') nDec=atoi(p+1);
      else if(pe=strchr(p,'=')) {
        bFixed=TRUE;
        switch(toupper(*p)) {
          case 'D' : iarg[0]=atof(pe+1); break;
          case 'A' : iarg[1]=atof(pe+1); break;
          case 'V' : iarg[2]=atof(pe+1); break;
        }
      }
      else {
	    if(!*frpath) {
	      p=_NCHR(trx_Stpext(_strupr(strcpy(frpath,p))));
	      if(!*p) strcpy(p,".SRV");
	    }
	    else if(!*topath) {
	      p=_NCHR(trx_Stpext(_strupr(strcpy(topath,p))));
	      if(!*p) strcpy(p,".SRV");
	    }
      }
    }
  
      if(!*topath || !bFixed || !strcmp(topath,frpath)) {
        printf("INCSRV v1.0 - Add Correction to Specified Column of Walls Data\n\n"
               "USAGE:   INCSRV <infile> <outfile> [D=<incD>] [A=<incA>] [V=<incV>] [-dN]\n"
               "         <incD>, <incA>, and <IncV> are optional numeric corrections\n"
               "         to be added to the corresponding measurements (distance, azimuth,\n"
               "         and vertical) of each data line. The default file extension is SRV.\n"
               "         Use option -dN to specify rounding to N decimals (default is 2).\n\n"
               "EXAMPLE: incsrv fileold filenew A=-21.717\n");
        return 0;
      }
  
      if((fp=fopen(frpath,"r"))==NULL) {
         printf("Cannot open file %s.\n",frpath);
         exit(1);
      }
      if((fpto=fopen(topath,"w"))==NULL) {
         printf("Cannot create file %s.\n",topath);
         exit(1);
      }

      printf("Writing %s..",topath);
      /*No comment delimeters or quoted strings for Compass --*/
      linecount=0;
      while(getline()>=0) {
         linecount++;
         pDst=lnbuf;
         pSrc=ln;
         skipspc();
         if(!*pSrc || *pSrc==';') goto _copy;
         if(*pSrc=='#') {
           if(*(pSrc+1)=='u' || *(pSrc+1)=='U') {
		     fprintf(fpto,"%s\n",ln);
		     _strupr(ln);       
             if(p=strstr(ln,"ORDER=")) p+=6;
             else if(p=strstr(ln,"O=")) p+=2;
             if(p) {
               p[3]=0;
               for(e=0;e<6;e++) if(!strcmp(p,i_str[e])) break;
               if(e<6) {
                 p_seq=(int *)i_seq[e];
                 printf("Order=%s e=%d i_seq[e][2]=%d\n",p,e,i_seq[e][2]);
               }
             }
             continue;
           }
           goto _copy;
         }
         skiparg();
         skiparg();
         incarg(p_seq[0]);
         incarg(p_seq[1]);
         incarg(p_seq[2]);
_copy:
		 strcpy(pDst,pSrc);
		 fprintf(fpto,"%s\n",lnbuf);       
      }
      fclose(fpto);
	  printf("\nLines written: %u\n",linecount);
	  fclose(fp);
      return 0;
}

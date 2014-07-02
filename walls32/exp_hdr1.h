//exp_hdt1.h -- For inclusion into wallexp.c
#include <time.h>
#include "expsef.h"
static EXPTYP *pEXP;
static NTVRec ntvrec;
static FILE *fpout=NULL;
static char outpath[MAX_PATH];
static lpNTVRec pVec=&ntvrec;

#pragma pack(1)
typedef struct {
	char  nam[16];	/* Full station name*/
	double xyz[3];  /* E-N-U vector components in meters*/
} CPTYP;
#pragma pack()

static CPTYP *pCP;
static int numCP;
static int maxCP;
#define MAXCP_INC 20

static LPSTR *pCC;
static int numCC;
static int maxCC;
#define MAXCC_INC 100

typedef struct {
	double di;
	double az;
	double va;
} sphtyp;

static UINT ctvectors;
static sphtyp ct_sp;
static double ct_decl; 
static double ct_scale,cp_scale;
static char *ct_units,*cp_units;
const char *ct_fmt;
static BOOL ct_islrud;
static double ct_lrud[4];

static char vec_pfx[2][SRV_LEN_PREFIX_DISP+2];
static char def_pfx[SRV_LEN_PREFIX_DISP+2];

static double NEAR PASCAL atn2(double a,double b)
{
  if(a==0.0 && b==0.0) return 0.0;
  return atan2(a,b)*(180.0/3.141592653589793);
}

static void get_ct_sp(double *pd)
{
  double d2=pd[0]*pd[0]+pd[1]*pd[1];

  if(d2<ERR_FLT) ct_sp.az=0.0;
  else {
	  ct_sp.az=atn2(pd[0],pd[1])-ct_decl;
	  if(ct_sp.az<0.0) ct_sp.az+=360.0;
      else if(ct_sp.az>=360.0) ct_sp.az-=360.0;
  }
  ct_sp.va=atn2(pd[2],sqrt(d2));
  ct_sp.di=sqrt(d2+pd[2]*pd[2]);
}

static int CDECL outSEF(const char *format,...)
{
	char buf[256];
	int iRet=0;
    va_list marker;
    va_start(marker,format);
    if(fpout && (iRet=_vsnprintf(buf,256,format,marker))>0) fputs(buf,fpout);
    va_end(marker);
    return iRet;
}

static char * trimtb(char *buf)
{
	char *p=buf+strlen(buf);
	while(p>buf && isspace((BYTE)*(p-1))) p--;
	*p=0;
	return buf;
}

static void outSEF_Comment(char *p)
{
	outSEF(";%s\r\n",trimtb(p));
}

static void freeCC(void)
{
	int i;
	if(pCC) {
		for(i=0;i<numCC;i++) free(pCC[i]);
		free(pCC);
		pCC=NULL;
	}
}

static void outSEF_Comments(void)
{
	int i;
	if(pCC) {
	  for(i=0;i<16 && i<numCC;i++) outSEF("#com %s \r\n",pCC[i]);
	  for(;i<numCC;i++) outSEF_Comment(pCC[i]);
	  freeCC();
	}
}

#define GET_PFX(src,i) exp_GetPrefix(src,i)
#define outSEF_s(s) fputs(s,fpout)
#define outSEF_ln() fputs("\r\n",fpout)

enum exp_err {
	EXP_ERR_INITSEF=SRV_ERR_ERRLIM,
	EXP_ERR_CLOSE,
	EXP_ERR_LENGTH
};

static void addCC(LPSTR pComment)
{
	LPSTR *pcc;

	trimtb(pComment);

	if(numCC>=maxCC) {
		pCC=(LPSTR *)realloc(pCC,(maxCC+=MAXCC_INC)*sizeof(LPSTR));
		if(!pCC) {
			numCC=maxCC=0;
			return;
		}
	}
	pcc=&pCC[numCC++];
	if(*pcc=(LPSTR)malloc(strlen(pComment)+1)) strcpy(*pcc,pComment);
	else  numCC--;
}

static int exp_GetPrefix(char *p,int i)
{
	int pfx;
	char *buf=(i>=0)?vec_pfx[i]:def_pfx;

	if(pEXP->flags&EXP_CVTPFX) {
		pfx=srv_CB(FCN_PREFIX,p);
		if(pfx) sprintf(buf,"%u",pfx);
		else *buf=0;
	}
	else {
	  pfx=1;
	  strncpy(buf,p,SRV_LEN_PREFIX_DISP);
	}
	return pfx;
}



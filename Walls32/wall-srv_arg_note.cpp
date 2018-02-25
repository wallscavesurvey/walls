/*WALL-SRV.CPP -- Survey file compilation */

#include "wall-srv.h"
#include "wall_emsg.h"
#include "assert.h"

/*======================================================================*/

static lpNTVRec pVec;
static typ_cseq dflt_cseq[2]={{'D','A','V',0},{'E','N','U',0}};
static char dflt_lrudstr[5]={'L','R','U','D',0};
/*
enum e_cmd {CMD_NONE,CMD_DATE,CMD_FIX,
			CMD_FLAG,CMD_NOTE,CMD_PREFIX,CMD_PREFIX2,CMD_PREFIX3,
            CMD_SEG,CMD_SYM,CMD_UNITS,CMD_COMMENT};
*/
static LPSTR cmdkey[]={
	"DATE",
	"F",
	"FIX",
	"FLAG",
	"N",
	"NOTE",
	"P",
	"PREFIX",
	"PREFIX1",
	"PREFIX2",
	"PREFIX3",
	"S",
	"SEG",
	"SEGMENT",
	"SYM",
	"SYMBOL",
	"U",
	"UNITS",
	"[",
	"]"
};
#define NUM_CMDKEY (sizeof(cmdkey)/sizeof(LPSTR))

//Eleven unique commands --

static int cmdtyp[NUM_CMDKEY+1]={
	CMD_NONE, 
	CMD_DATE,   //"DATE"
	CMD_FLAG,   //"F"
	CMD_FIX,    //"FIX"
	CMD_FLAG,   //"FLAG"
	CMD_NOTE,   //"N"
	CMD_NOTE,   //"NOTE"
	CMD_PREFIX, //"P"
	CMD_PREFIX, //"PREFIX"
	CMD_PREFIX, //"PREFIX1"
	CMD_PREFIX2,//"PREFIX2"
	CMD_PREFIX3,//"PREFIX3"
	CMD_SEG,    //"S"
	CMD_SEG,    //"SEG",
	CMD_SEG,    //"SEGMENT",
	CMD_SYM,    //"SYM",
	CMD_SYM,    //"SYMBOL",
	CMD_UNITS,	//"U",
	CMD_UNITS,	//"UNITS"
	CMD_COMMENT,//"[",
	CMD_COMMENT //"]"
};

static LPSTR argkey[]={
	"A",
	"AB",
	"AZIMUTH",
	"CASE",
	"CT",
	"D",
	"DECL",
	"DISTANCE",
	"F",
	"FEET",
	"FLAG",
	"GRID",
	"INCA",
	"INCAB",
	"INCD",
	"INCH",
	"INCV",
	"INCVB",
	"LRUD",
	"M",
	"METERS",
	#ifdef _USE_ARGNOTE
	"NOTE",
	#endif
	"O",
	"ORDER",
	"P",
	"PREFIX",
	"PREFIX1",
	"PREFIX2",
	"PREFIX3",
	"RECT",
	"RESET",
	"RESTORE",
	"S",
	"SAVE",
	"TAPE",
	"TYPEAB",
	"TYPEVB",
	"UV",
	"UVH",
	"UVV",
	"V",
	"VB",
	"VERTICAL",
	"X"
};
#define NUM_ARGKEY (sizeof(argkey)/sizeof(LPSTR))

enum e_arg {ARG_NONE,ARG_EQUALS,ARG_PARAM,ARG_NUMERIC,ARG_MACRODEF,  /*Not keys*/
			ARG_RESET,ARG_RESTORE,ARG_SAVE, /*For initial scan*/
			ARG_A,ARG_AB,ARG_CASE,ARG_CT,ARG_D,ARG_DECL,ARG_ERRAB,ARG_ERRVB,
			ARG_FEET,ARG_FLAG,ARG_GRID,ARG_INCA,ARG_INCAB,ARG_INCD,ARG_INCH,
			ARG_INCV,ARG_INCVB,ARG_LRUD,ARG_METERS,
#ifdef _USE_ARGNOTE
			ARG_NOTE,
#endif
			ARG_ORDER,
			ARG_PREFIX,ARG_PREFIX2,ARG_PREFIX3,ARG_RECT,
			ARG_S,ARG_TAPE,ARG_TYPEAB,ARG_TYPEVB,ARG_UV,
			ARG_UVH,ARG_UVV,ARG_V,ARG_VB,ARG_X};

static int argtyp[NUM_ARGKEY+1]={
	ARG_NONE,
	ARG_A,		//"A",
	ARG_AB,     //"AB",
	ARG_A,		//"AZIMUTH",
	ARG_CASE,	//"CASE",
	ARG_CT,		//"CT",
	ARG_D,		//"D",
	ARG_DECL,	//"DECL",
	ARG_D,		//"DISTANCE",
	ARG_FEET,	//"F",
	ARG_FEET,	//"FEET",
	ARG_FLAG,	//"FLAG",
	ARG_GRID,	//"GRID",
	ARG_INCA,	//"INCA",
	ARG_INCAB,	//"INCAB",
	ARG_INCD,	//"INCD",
	ARG_INCH,	//"INCH",
	ARG_INCV,	//"INCV",
	ARG_INCVB,	//"INCVB",
	ARG_LRUD,   //"LRUD",
	ARG_METERS,	//"M",
	ARG_METERS,	//"METERS",
#ifdef _USE_ARGNOTE
	ARG_NOTE,   //"NOTE",
#endif
	ARG_ORDER,  //"O",
	ARG_ORDER,  //"ORDER",
	ARG_PREFIX,	//"P",
	ARG_PREFIX,	//"PREFIX",
	ARG_PREFIX,	//"PREFIX1",
	ARG_PREFIX2,//"PREFIX2",
	ARG_PREFIX3,//"PREFIX3",
	ARG_RECT,	//"RECT",
	ARG_RESET,	//"RESET",
	ARG_RESTORE,//"RESTORE",
	ARG_S,		//"S",
	ARG_SAVE,	//"SAVE",
	ARG_TAPE,	//"TAPE",
	ARG_TYPEAB,	//"TYPEAB",
	ARG_TYPEVB,	//"TYPEVB",
	ARG_UV,		//"UV",
	ARG_UVH,	//"UVH",
	ARG_UVV,	//"UVV",
	ARG_V,		//"V",
    ARG_VB,     //"VB",
	ARG_V,		//"VERTICAL"
	ARG_X
};

static int unityp[CFG_MAXARGS];

static char *au_str="PDMG";
static double au_val[6]={U_PERCENT,U_DEGREES,U_MILS,U_GRADS};

#define NUM_LRUD 4
static char *lrud_str[NUM_LRUD]={"F","FB","T","TB"};
enum e_lrudmeth {LRUD_F,LRUD_FB,LRUD_T,LRUD_TB};

#define NUM_TAPE 4
static char *tape_str[NUM_TAPE]={"IT","SS","IS","ST"};
enum e_tapemeth {TAPE_IT,TAPE_SS,TAPE_IS,TAPE_ST};

static SRV_UNITS unitsDflt={
     U_DEGREES,U_DEGREES,U_DEGREES,U_DEGREES,
     0.0,0.0,0.0,
     0.0,0.0,0.0,0.0,0.0,0.0,0.0,
     MAX_BACKDELTA,MAX_BACKDELTA,
     NET_VAL_FACT,NET_VAL_FACT,
	 {{""},{""},{""}},
     0,
     FALSE,FALSE,
     0,0,0,0,
     U_POLAR,
     U_ANYCASE,
     TAPE_IT,
	 LRUD_F,
     {{2,3,4,0},{2,3,4,0}},
	 {0,1,2,3}
};

static SRV_LRUD lruddat;     

#define U_LEVELS 10

/*Globals for external use*/
SRV_UNITS *u;
double fTemp;
int ErrBackTyp;

static SRV_UNITS u_stack[U_LEVELS];

static FILE *fp=NULL;
LPSTR_CB srv_CB;
static DWORD nLines;
static char ln[LINLEN];
static char srvname[16];
static int segMain_id;
static int _scn_digit;
static BOOL bGenUTM,bUseDates,bFixVtVert,bFixVtLen,bLrudOnly,bLrudPending,bErrTwoNames,bFixed,bRMS;
static WORD fixed_flagid;
#ifdef _USE_ARGNOTE
static WORD fixed_typnote;
static LPCSTR srvtitle;
#endif

//Segment data --
#define SRV_MAX_NSEGS 16

typedef struct {
   int nPending;
   int nSegs;
   int iLen[SRV_MAX_NSEGS];
   char Buf[SRV_SIZ_SEGBUF];
} SEGTYP;
typedef SEGTYP NEAR *pSEGTYP;

#pragma pack(1)
static struct {
	double az;
	WORD pfx;
	char nam[SRV_SIZNAME];
} lastTo, *pLastTo;
#pragma pack()

typedef struct {
	int namidx;
	int validx;
} MACRO_DEF;

#define INC_MACRO_DEF 30
#define SIZ_MACRO_STR 256
#define INC_MACRO_BLK 512

static int macro_stridx,macro_maxcnt,macro_bufspc,macro_numblks;
static int macro_cnt=0;
static char *macro_buf=NULL;
static MACRO_DEF *macro_def=NULL;

static SEGTYP segMain,segLine;
static pSEGTYP pSEG;
static char SegBuf[SRV_SIZ_SEGBUF];
static char NoteBuf[SRV_LEN_NOTESTR+1];

#define COMMAND_FIXED -1

/*
#define SRV_ERR_LOGMSG 1000
enum {
       SRV_ERR_BADFLAGNAME=1,
       SRV_ERR_NOFLAGNAME,
       SRV_ERR_LOCAL
     };

enum {
       SRV_ERR_OPEN=SRV_ERR_LOCAL,
       SRV_ERR_REOPEN,
       SRV_ERR_SIZNAME,
       SRV_ERR_LINLEN,
       SRV_ERR_NUMARGS,
       SRV_ERR_DECLINATION,
       SRV_ERR_CORRECTION,   //==10
       SRV_ERR_SPECIFIER,
       SRV_ERR_ANGLEUNITS,
       SRV_ERR_ORDER,
       SRV_ERR_AZIMUTH,
       SRV_ERR_DISTANCE,
       SRV_ERR_VERTICAL,
       SRV_ERR_NUMERIC,
       SRV_ERR_VARIANCE,
       SRV_ERR_UNEXPVALUE,
       SRV_ERR_SEGPATH,		//==20
       SRV_ERR_SEGSIZE,
       SRV_ERR_DIRECTIVE,
       SRV_ERR_LINECMD,
       SRV_ERR_UV,
       SRV_ERR_PREFIX,
       SRV_ERR_NOPAREN,
       SRV_ERR_BADSYMBOL,
       SRV_ERR_NONAMEPFX,
       SRV_ERR_CMDBLANK,
       SRV_ERR_NOPARAM,		//==30
       SRV_ERR_SLASH,
       SRV_ERR_BACKDELTA,
       SRV_ERR_TAPEMETH,
       SRV_ERR_NOTEARG,
       SRV_ERR_COMMOPEN,
       SRV_ERR_COMMCLOSE,
       SRV_ERR_DATE,
       SRV_ERR_RESTORE,
       SRV_ERR_SAVE,
       SRV_ERR_SIGNREV,		//==40
       SRV_ERR_EQUALS,
       SRV_ERR_LENUNITS,
       SRV_ERR_BADANGLE,
       SRV_ERR_BADLENGTH,
       SRV_ERR_BLANK,
       SRV_ERR_AZIMUTHSIGN,
       SRV_ERR_TYPEBACK,
       SRV_ERR_FSBSERR,
	   SRV_ERR_LRUDARGS,
	   SRV_ERR_LRUDMETH,	//==50
	   SRV_ERR_LATLONG,
	   SRV_ERR_NODATUM,
	   SRV_ERR_BADCOLOR,
	   SRV_ERR_SPRAYNAME,
	   SRV_ERR_MACRONOVAL,
	   SRV_ERR_MACRONODEF,
	   SRV_ERR_NOMEM,
	   SRV_ERR_LENPREFIX,
	   SRV_ERR_COMPPREFIX,
	   SRV_ERR_CHRPREFIX,   //==60
	   SRV_ERR_IT_HEIGHTS,
	   SRV_ERR_HZ_LENGTH,
	   SRV_ERR_REL_DEPTH,
	   SRV_ERR_SEGNAMLEN,
	   SRV_ERR_UNITSDATE,
	   SRV_ERR_ERRLIM
    };
    
    //wallexp.dll only --
    enum exp_err {
	   EXP_ERR_INITSEF=SRV_ERR_ERRLIM, //==66
	   EXP_ERR_LENGTH,
	   EXP_ERR_VERSION		   //==68
    };
*/	
     
static char *errmsg[]=
{
       //CAUTION: Maximum length for the following strings is 39!
       "Flag name too long",
       "Symbol requires flag name",
       "File open failure",
       "DLL already in use",
       "Name length exceeds " SRV_SIZNAME_STR " chars",
       "File line is too long",
       "Vector data incomplete",
       "North correction too large",
       "Correction too large",			//==10
       "Unrecognized units specifier",
       "Bad angle units specifier",
       "Bad order specifier",
       "Bearing out of range",
       "Distance value can't have sign",
       "Inclination out of range",
       "Numeric value expected",
       "Bad variance override",
       "Unexpected data item",
       "Bad format for segment",		//==20
       "Segment path length >255",
       "Unrecognized directive",
       "Use only #SEG on vector lines",
       "Unit variance >=0 expected",
       "Could not process prefix",
       "Expression not closed",
       "Bad symbol definition",
       "Flag name needs \"/\" prefix",
       "Directive has no arguments",
       "Units parameter needs value",		//==30
       "Backsight data must follow \"/\"",
       "%s FS/BS difference too large (%.1f deg)",
       "Tape method not IT,IS,ST,or SS",
       "Note directive has no text",
       "Unmatched end comment (no #[)",
       "Unclosed comment (no #])",
       "Bad date: yyyy-mm-dd preferred",
       "RESTORE without previous SAVE",
       "SAVE/RESTORE nested >10 deep",
       "Tape corr causes sign reversal",	//==40
       "Units keyword takes no value",
       "Bad length units specifier",
       "Bad angle or azimuth qty", 
       "Wrong format for length qty",
       "A non-blank item is required",
       "Bearing can't have sign",
       "Backsight type must be N or C",
       "Value must be between 0 and 90",
	   "Too many LRUD distances",
	   "LRUD type must be F,FB,T, or TB",	//==50
	   "Bad format for lat/long",
	   "Use of Lat/Lon requires a georef",
	   "Bad RGB color. Example: \"(255,0,0)\"",
	   "Wall shot needs a station name",
	   "Macro definition requires a value",
	   "Undefined macro in units argument",
	   "Memory allocation failure",
	   "Name prefix too long",
	   "Name prefix has too many components",
	   "Name prefix can't have colons",		//==60
	   "IT heights inconsistent with distance",
	   "Ambiguity caused by |IH-TH| > distance",
	   "Relative depth > taped distance",
	   "Segment name lengh >80",
	   "\"Date\" not a valid units argument"
};

BOOL _cdecl log_error(char *fmt,...)
{
	va_list va;
	char buf[LINLEN];
	int e,n=_snprintf(buf,LINLEN-1,"%s:%u\t",srvname,nLines);

    if(n<0) return FALSE;

	va_start(va,fmt);
	e=_vsnprintf(buf+n,(LINLEN-1)-n,fmt,va);
	if(e<0) return FALSE;
    n+=e;
	buf[n]='\n';
	buf[n+1]=0;
	return 0==srv_CB(FCN_ERROR,buf);
}

static void log_lrud_error()
{
	int e;

    for(e=SRV_SIZNAME;e;e--) if(lruddat.nam[e-1]!=' ') break;
    lruddat.nam[e]=0;
	e=nLines;
	nLines=lruddat.lineno;
	log_error("LRUD at station %s ignored. Facing direction can't be determined.",lruddat.nam);
	nLines=e;
}

/*
static void check_lrudonly(LPSTR p)
{
	char buf[LINLEN];
	strncpy(buf, p, LINLEN); //the _snprintf variant will NOT store NULL after truncation
	buf[LINLEN-1]=0;
	p=buf+1;
	int bdgt = (*p=='.' || isdigit(*p)) && p++;
	while(*p && ((bdgt && isdigit(*p)) || (!bdgt && *p=='-'))) *p++;
	if((*p==',' ||  *p==' ') && p-buf<=(bdgt?8:3)) {
		*p=0;
		log_error("Caution: Is the second name (%s), part of a non-terminated LRUD?", buf);
	}
}
*/

static float get_lrud_degrees(double az)
{
	az=fmod(az*180.0/PI,360.0);
	if(az<0.0) az+=360.0;
	return (float)az;
}

LPSTR PASCAL srv_ErrMsg(LPSTR buffer,int code)
{
	if(code==SRV_ERR_LOGMSG) {
		log_error(buffer);
		return NULL;
	}
	if(code==SRV_ERR_BACKDELTA)
	  sprintf(buffer,errmsg[code-1],(ErrBackTyp==TYP_V)?"Vertical":"Azimuth",fTemp);
	else strcpy(buffer,errmsg[code-1]);
	return buffer;
}

static int discard_line_overflow()
{
	int i=0;
	char buf[256];
	while(fgets(buf,256, fp)!=NULL) {
		int jj=strlen(buf);
		i+=jj;
		if(jj!=255 || buf[254]=='\n') break;
	}
	return i;
}

static int CALLBACK line_fcn(NPSTR buf,int sizbuf)
{
  char *p;
  if(NULL==fgets(buf,sizbuf,fp)) {
	  sizbuf=ferror(fp);
	  sizbuf=feof(fp);
	  return CFG_ERR_EOF;
  }

  nLines++;

  int jj=strlen(buf);
  if(jj==sizbuf-1) {
	 assert(jj==255);
     if(buf[sizbuf-2]!='\n') {
	     //Discard rest of line, log if more than \n --
		 if(discard_line_overflow()>1)
			 log_error("Line truncated to 255 characters.");
		 buf[sizbuf-1]=0;
	  }
  }

  //Trim comments and whitespace --
  for(p=buf;*p==' ' || *p=='\t';p++);
  if(*p++==SRV_CHAR_COMMAND && *p++=='[') *p=0;

  if(p=strchr(buf,SRV_CHAR_COMMENT)) {
	*p=0;
  }
  else p=buf+strlen(buf);
  while(p>buf && isspace((BYTE)*(p-1))) p--;
  *p=0;
  return p-buf;
}

static apfcn_i parse_lrudmeth(char *p)
{
	int i;
	char *pc;
	
	if(!p) return SRV_ERR_NOPARAM;
    _strupr(p);
	if((pc=strchr(p,':'))) *pc++=0;

    for(i=0;i<NUM_LRUD;i++) if(!strcmp(p,lrud_str[i])) break;
    if(i==NUM_LRUD) return SRV_ERR_LRUDMETH;
    u->lrudmeth=i;

    if(pc) {
		if(strlen(pc)!=4) return SRV_ERR_LRUDMETH;
		for(i=0;i<4;i++) {
		  if(!(p=strchr(pc,dflt_lrudstr[i]))) break;
		  u->lrudseq[i]=(BYTE)(p-pc);
		}
		if(i<4) return SRV_ERR_LRUDMETH;
	}

	bLrudPending=bErrTwoNames=FALSE;
	pLastTo=NULL;

	return 0;
}

static apfcn_i parse_order(char *uc)
{
  int i;
  BYTE *ui;
  char *dc;
  char *p;
  
  if(!uc) return SRV_ERR_NOPARAM;
  
  i=strchr(_strupr(uc),'D')?0:1;
  
  dc=dflt_cseq[i];
  ui=u->iseq[i];
  
  /*Is every char in dc[] also in uc[]?*/
  for(i=0;i<NUM_ORDER_ARGS;i++,ui++,dc++) {
	  if(!(p=strchr(uc,*dc))) {
	    //We allow a missing vertical component --
	    if(i!=NUM_ORDER_ARGS-1) return SRV_ERR_ORDER;
	    *ui=0;
	    break;
	  }  
	  else *ui=(BYTE)(p-uc)+OFFSET_ORDER_ARGS;
  }
  if(strlen(uc)!=(size_t)i) return SRV_ERR_ORDER;
  return 0;
}

int PASCAL srv_Open(LPSTR pathname,lpNTVRec pvec,LPSTR_CB pSrv_CB)
{
  pVec=pvec;
  pVec->date[0]=pVec->date[1]=pVec->date[2]=0;
  srv_CB=pSrv_CB;

#ifdef _USE_ARGNOTE
  srvtitle=(LPCSTR)pvec->lineno;
  fixed_typnote=0;
#endif

  strncpy(srvname,trx_Stpnam(pathname),15);
  *trx_Stpext(srvname)=0;
  bGenUTM=(pVec->root_datum!=255);
  bUseDates=(pVec->charno&SRV_USEDATES)!=0;
  bFixVtLen=(pVec->charno&SRV_FIXVTLEN)!=0;
  bFixVtVert=(pVec->charno&SRV_FIXVTVERT)!=0;
  fixed_flagid=(WORD)-1;
  pVec->charno=0;
  u=&u_stack[0];
  *u=unitsDflt;
  u->gridnorth=pVec->grid;
  if(fp) return SRV_ERR_REOPEN;
  if((fp=fopen(pathname,"r"))==NULL) return SRV_ERR_OPEN;

  //---
  nLines=0;
  cfg_LineInit((CFG_LINEFCN)line_fcn,ln,LINLEN,SRV_CHAR_COMMAND);
  cfg_KeyInit(cmdkey,NUM_CMDKEY);
  cfg_noexact=FALSE; /*Exact matching, but we will allow aliases*/
  cfg_commchr=SRV_CHAR_COMMENT;
  cfg_equals=cfg_quotes=FALSE; /*no equivalences except when parsing commands*/
  cfg_contchr=0; /*no continuation lines*/
  pSEG=&segMain;
  segMain_id=-1;
  segMain.nPending=segLine.nPending=bLrudPending=bErrTwoNames=0;
  pLastTo=NULL;
  segMain.nSegs=segLine.nSegs=0;
  SegBuf[0]=0;
  macro_cnt=0;
  macro_buf=NULL;
  macro_def=NULL;

  return 0;
}

void PASCAL srv_Close(void)
{
  if(bLrudPending) {
	  if(bLrudPending!=3 || lruddat.az==LRUD_BLANK) log_lrud_error();
	  else srv_CB(FCN_LRUD, (LPSTR)&lruddat); //ignore unlikely error
  }
  if(fp) {
    fclose(fp);
    fp=NULL;
  }
  if(macro_buf || macro_def) {
	  free(macro_buf); macro_buf=NULL;
	  free(macro_def); macro_def=NULL;
  }
}

enum e_flg {SCN_NODEC=1,SCN_NOMINUS=2,SCN_NOPLUS=4,SCN_NOSIGN=6};

static apfcn_nc scn_num(char NEAR *p,int flg)
{
  int c=*p;
  _scn_digit=FALSE;
  
  if(c=='+' || c=='-') {
    if(c=='-' && (flg&SCN_NOMINUS) || c=='+' && (flg&SCN_NOPLUS)) return p;
    p++;
  }
  
  while(c=*p++) {
    if(c>'9' || (c<'0' && c!='.')) break;
    if(c!='.') _scn_digit=TRUE;
    else if(1&flg++) break;
  }
  return p-1;      
}  

static apfcn_i isnumeric(char *p)
{
  //Returns TRUE iff atof() would retrieve a number --
  if(*p=='-' || *p=='+') p++;
  if(*p=='.') p++;
  return isdigit((BYTE)*p);
}

static apfcn_i ismacro(char *p)
{
	return (p && (p=strchr(p,SRV_CHAR_MACRO)) && p[1]=='(');
}

static apfcn_i set_angle_units(int arg,char *p)
{
  double *punits;
  
  if(!p) return SRV_ERR_NOPARAM;
  
  p=strchr(au_str,toupper(*p));
  if(!p || p==au_str && (arg==ARG_A || arg==ARG_AB)) return SRV_ERR_ANGLEUNITS;
  
  switch(arg) {
    case ARG_A  : punits=&u->unitsA; break;
    case ARG_AB : punits=&u->unitsAB; break;
    case ARG_V  : punits=&u->unitsV; break;
    case ARG_VB : punits=&u->unitsVB; break;
  }
  *punits=au_val[p-au_str];
  
  return 0;  
}

static apfcn_i set_charno(int i)
{ 
   /*Optain offset in ln of cfg_argv[i]. Note that cfg_argv[i] points to
     a character in ln except when cfg_arg[i]==cfg_equalsptr*/
   char *p;
   for(;i<cfg_argc;i++) {
     p=cfg_argv[i];
     if(!cfg_equals || p!=cfg_equalsptr) break;
   }
   pVec->charno=(i<cfg_argc)?(p-ln):0;
   return TRUE;
}

static apfcn_v Strxchg(char *p,char cOld,char cNew)
{
   for(;*p;p++) if(*p==cOld) *p=cNew;
}

static apfcn_i AddSegment(char *pStr,int len)
{
    int nSeg=pSEG->nSegs;
    int iOffset=nSeg?pSEG->iLen[nSeg-1]:0;
    
    if(nSeg==SRV_MAX_NSEGS || iOffset+len+(nSeg!=0)>=SRV_SIZ_SEGBUF)
      return SRV_ERR_SEGSIZE;
      
    pSEG->nSegs++; /*==nSeg+1*/
    if(nSeg) {
      pSEG->Buf[iOffset++]=SRV_SEG_DELIM;
      pSEG->nPending++;
    }
    
    //So WALLS can distinguish this segment from those defined by
    //the project tree, let's flag the first character --
    *pStr|=(BYTE)0x80;
    
    memcpy(pSEG->Buf+iOffset,pStr,len);
    pSEG->iLen[nSeg]=iOffset+len;
    if(pSEG->nPending<=0) pSEG->nPending=1;
    return 0; 
}

static apfcn_i ParseSegment(char *p)
{
    char *pp;
    int e=SRV_ERR_SEGPATH;
    
    Strxchg(p,'\t',' ');
    
    /*Do NOT allow possibility of other commands --*/
    pp=p+strlen(p);
    
	/*Truncate trailing blanks --*/    
    while(pp>p && *(pp-1)==' ') pp--;
    *pp=0;
    
    Strxchg(p,'\\','/');
    
    /*Disallow empty name --*/
    if(!*p) {
      e=SRV_ERR_SEGPATH;
      goto _error;
    }
    
    /*p now points to an ASCIIZ segment pathname.*/
    
    /*NOTE: iLen[i] is the length of subpath i, i=0..nSegs-1
      exclusive of any trailing "/".
      iLen[nSegs-nPending] is the length of the next subpath
      that needs updating (before returning the next vector)
      when nPending>0. (we assume iLen[-1]==0)
    */ 
    
    if(*p=='/') {
      /*Absolute path --*/
      p++;
      pSEG->nPending=1;
      pSEG->nSegs=0;
    }
    else if(*p=='.') {
      /*Parse leading ../../../ prefix --*/
      do {
	      if(!*++p) break;
	      if(*p=='.') {
	        if(pSEG->nSegs) {
	          pSEG->nSegs--;
	          pSEG->nPending--;
	          if(pSEG->nPending<1) pSEG->nPending=1;
	        }
	        if(!*++p) break;
	      }
	      /*We require that a '\0' or '/' always follow "." or ".."*/
	      if(*p++!='/') goto _error;
	  }
	  while(*p=='.');
    }
    
    /*Disallow double slashes for now --*/
    while(*p) {
      /*Also disallow "/." in the remaining portion of the path --*/
      if(*p=='.') goto _error;
      for(pp=p;*p&&*p!='/';p++);
      if(p==pp) goto _error;
	  if(p-pp > SRV_MAX_SEGNAMLEN) {
		 e=SRV_ERR_SEGNAMLEN;
		 goto _error;
	  }
      if(AddSegment(pp,p-pp)) {
        e=SRV_ERR_SEGSIZE;
        goto _error;
      } 
      if(*p) p++;
    }
    //*p=cSav; ???
    return 0;
    
_error:    
    pVec->charno=p-ln;
    return e;
}

static int parse_numeric(double *val,char *p)
{
  if(!isnumeric(p)) return SRV_ERR_NUMERIC;
  *val=atof(p);
  return 0;
}

static int parse_numeric_arg(double *val,int i)
{
  if(i>=cfg_argc) return SRV_ERR_NUMARGS;
  if(!isnumeric(cfg_argv[i])) return SRV_ERR_NUMERIC;
  *val=atof(cfg_argv[i]);
  return 0;
}

static int parse_length(double *val,char *p0,int bFeet)
{
  char *p;
  int c;
  
  p=scn_num(p0,0);
  
  if(!*p) {
    if(!_scn_digit) return SRV_ERR_NUMERIC;
    *val=atof(p0);
    if(bFeet) *val *= U_FEET;
    return 0;
  }
  
  _strupr(p);
  
  if(*p=='I') {
  	if((c=*scn_num(p+1,SCN_NOSIGN)) && c!='F' ||
  	  !_scn_digit || (fTemp=atof(p+1))>=12.0) return SRV_ERR_BADLENGTH;
  	fTemp*=(U_FEET/12.0);
  	if(*p0=='-') fTemp=-fTemp;
  	*p=0;
  	if(*scn_num(p0,SCN_NODEC)) return SRV_ERR_BADLENGTH;
  	if(_scn_digit) fTemp+=atof(p0)*U_FEET;
  	*val=fTemp;
    return 0; 
  }
  
  if(!_scn_digit || *p=='-') {
    for(p=p0;*p=='-';p++);
    return *p?SRV_ERR_BADLENGTH:SRV_ERR_BLANK;
  }
  
  if(p[1] || ((c=*p)!='M' && c!='F')) return SRV_ERR_BADLENGTH;
  *val=atof(p0);
  if(c=='F') *val *= U_FEET;
  return 0;
}

static apfcn_i find_macro(char *name)
{
	//Few macros, so linear search sufficient --
	int i;
	for(i=0;i<macro_cnt;i++) if(!strcmp(macro_buf+macro_def[i].namidx,name)) return i;
	return -1;
}

static apfcn_nc expand_macros(char *name)
{
	char *p,*pDest=macro_buf;
	int i;

	if(!pDest) goto _erret;

	//name[] guaranteed to contain '$' --
	while(*name) {
		if(*name==SRV_CHAR_MACRO && name[1]=='(') {
			name+=2;
			if(!(p=strchr(name,')'))) goto _erret;
			*p=0;
			i=find_macro(name);
			*p++=')';
			if(i<0) goto _erret;
			strcpy(pDest,macro_buf+macro_def[i].validx);
			pDest+=strlen(pDest);
			name=p;
		}
		else *pDest++=*name++;
	}
	*pDest=0;
	return macro_buf;

_erret:
	pVec->charno=name-ln;
	return NULL;
}

static apfcn_i parse_inclength(double *val,int i,int bFeet)
{
	char *p=cfg_argv[i];
    if(unityp[i]!=ARG_PARAM) return SRV_ERR_NOPARAM;
	if(ismacro(p)) p=macro_buf;
	return parse_length(val,p,bFeet);
}

static apfcn_i parse_length_arg(double *val,int i,int bFeet)
{
  char *p;
  
  if(i>=cfg_argc ||
    *(p=cfg_argv[i])==SRV_CHAR_VAR || *p==SRV_CHAR_LRUD || *p=='*') return SRV_ERR_NUMARGS;
  
  return parse_length(val,p,bFeet);
}

static apfcn_i parse_distance(double *val,int i)
{
  char *p;
  
  if(i>=cfg_argc ||
    *(p=cfg_argv[i])==SRV_CHAR_VAR || *p==SRV_CHAR_LRUD || *p=='*') return SRV_ERR_NUMARGS;
  
  if(*p=='-' || *p=='+') return SRV_ERR_DISTANCE;
  
  return parse_length(val,p,u->bFeet);
}

static apfcn_i get_fTempDMS(char *p0)
{
	//Upon entry p0=="[-][mm]:[dd][:[ss]]" and scn_num(p) has returned with *p==':'
	char *p;
	double min;
	
	BOOL bNeg=(*p0=='-');
	if(bNeg) p0++;
	
    //Get degrees - don't allow decimal --
	p=scn_num(p0,SCN_NOSIGN|SCN_NODEC);
	if(*p!=':') return 1; //impossible
    if(_scn_digit) fTemp=atof(p0);
    else fTemp=0.0;
    
    //Get minutes --
    p=scn_num(p0=p+1,SCN_NOSIGN);
    if(*p) {
      //There is a seconds portion --
      if(*p!=':') return 1;
      //In this case, don't allow decimal for minutes portion--
      *p++=0;
      if(!*p || strchr(p0,'.')) return 1;
    }
    else if(!_scn_digit) return 1; 
    if(_scn_digit) {
      if((min=atof(p0))>=60.0) return 1;
      fTemp+=min/60.0;
    }
    
    //Get seconds --
    if(*p) {
      p=scn_num(p0=p,SCN_NOSIGN);
      if(!_scn_digit || *p) return 1;
      if((min=atof(p0))>=60.0) return 1;
      fTemp+=min/3600.0;
    }
    if(bNeg) fTemp=-fTemp;
    return 0;
}

static apfcn_i parse_latlong(double *val,char *p,int i)
{
  char c=toupper(*p);

  if((i==1 && (c=='N' || c=='S')) || (i==0 && (c=='E' || c=='W'))) {
	  if(c=='W' || c=='S') *p='-';
	  else p++;
	  if(!strchr(p,':')) fTemp=atof(p);
	  else if(get_fTempDMS(p)) return SRV_ERR_LATLONG;
	  if(i==0 && fabs(fTemp)<=180.0 || i==1 && fabs(fTemp)<=90.0) {
		  *val=fTemp;
		  return 0;
	  }
  }
  return SRV_ERR_LATLONG;
}

apfcn_i get_fTempAngle(char *p0,BOOL bAzimuth)
{
    int c_sfx;
    int c_pfx;
    
    char *p=scn_num(p0,bAzimuth?SCN_NOSIGN:0);
    
    //Most common case (ordinary unscaled number) --
    if(!*p) {
      if(!_scn_digit) goto _errblank;
      fTemp=atof(p0);
      return 0;
    }
    _strupr(p0);
    
    //Quadrant bearing (can contain DMS) --
    if(*p0=='N' || *p0=='S') {
      //Implies p==p0 --
      if(!bAzimuth) goto _reterr;
      c_pfx=*p0++;
      if(!*p0) {
         fTemp=(c_pfx=='N')?0.0:PI;
         return 1;
      }
      p=p0+strlen(p0)-1;
      if((c_sfx=*p)!='E' && c_sfx!='W') goto _reterr;
      *p=0;
      p=scn_num(p0,SCN_NOSIGN);
      if(*p) {
        if(*p!=':'|| get_fTempDMS(p0)) goto _reterr;
      }
      else {
        if(!_scn_digit) goto _reterr;
        fTemp=atof(p0);
      }
      if(fTemp>90.0) goto _reterr;
      if(c_pfx=='N') {
        if(c_sfx=='W') fTemp=360.0-fTemp;
      }
      else {
        if(c_sfx=='W') fTemp+=180.0;
        else fTemp=180.0-fTemp;
      }
      fTemp *= U_DEGREES;
      return 1;
    }
    
    if(p==p0) {
	  if(*p=='E' || *p=='W') {
		  if(!bAzimuth || p[1]) goto _reterr;
		  fTemp=(*p=='E')?(PI/2.0):(3.0*PI/2.0);
		  return 1;
	  }
      if(*p=='-' || *p=='+') {
	      p++;
	      if(*p==0) return -SRV_ERR_BLANK;
	      if(*p=='-') goto _errblank;
	      return -SRV_ERR_AZIMUTHSIGN;
	  }
    }
    
    //Non-quadrant DMS --
    if(*p==':') {
      if(get_fTempDMS(p0)) goto _reterr;
      fTemp *= U_DEGREES;
      return 1;
    }

_errblank:    
    //Allow blank value --
    if(!_scn_digit || *p=='-') {
      for(p=p0;*p=='-';p++);
      if(*p) goto _reterr;
      return -SRV_ERR_BLANK;
    }
       
    //Units override --
    if(p[1] || !(p=strchr(au_str,*p)) || bAzimuth && p==au_str) goto _reterr;
	if(p==au_str) {
		//compute angle in radians converted from percent grade --
		//*p0 = tan(a) * 100
		//a = atan(*p/100)
		fTemp=atan(atof(p0)*0.01);
	}
    else fTemp=au_val[p-au_str]*atof(p0);
    return 1;
	
_reterr:
	return -SRV_ERR_BADANGLE;
}

static apfcn_i parse_incangle(int typ,int i)
{
	double *pVal;
	char *p;
	
    if(unityp[i]!=ARG_PARAM) return SRV_ERR_NOPARAM;
    p=cfg_argv[i];
	if(ismacro(p)) p=macro_buf;

    if((i=get_fTempAngle(p,FALSE))<0) return -i;
	     			 
	/*All three declinations are assumed in degrees and maintained in degrees*/
	/*All four angle corrections are assumed in dflt units but maintained in radians*/
	switch(typ) {
	  case ARG_DECL		: pVal=&u->declination;
	  case ARG_RECT		: if(typ==ARG_RECT) pVal=&u->rectnorth;
	  case ARG_GRID		: if(typ==ARG_GRID) pVal=&u->gridnorth;
	  					  if(i) fTemp /= U_DEGREES; /*convert back to degrees*/
	  					  if(fabs(fTemp)>DECLINATION_MAX) return SRV_ERR_DECLINATION;
	  					  *pVal=fTemp;
	  					  return 0;
	  					  
	  case ARG_INCA     : if(!i) fTemp*=u->unitsA;  pVal=&u->incA; break;
	  case ARG_INCAB    : if(!i) fTemp*=u->unitsAB; pVal=&u->incAB; break;

	  case ARG_INCV     : if(!i) {
							if(u->unitsV==U_PERCENT)
								fTemp=atan(fTemp*0.01);
							else fTemp*=u->unitsV;
						  }
						  pVal=&u->incV;
						  break;

	  case ARG_INCVB    : if(!i) {
							if(u->unitsVB==U_PERCENT)
								fTemp=atan(fTemp*0.01);
							else fTemp*=u->unitsVB;
						  }
						  pVal=&u->incVB;
						  break;

	  default			: return SRV_ERR_CORRECTION; /*Impossible*/
	}
    /*Angle corrections are maintained in radians --*/
  	if(fabs(fTemp)>CORRECTION_MAX) return SRV_ERR_CORRECTION;
	*pVal=fTemp;
	return 0;
}

static apfcn_i parse_tape(char *p)
{
	int i;
	
	if(!p) return SRV_ERR_NOPARAM;
    _strupr(p);
    for(i=0;i<NUM_TAPE;i++) if(!strcmp(p,tape_str[i])) break;
    if(i==NUM_TAPE) return SRV_ERR_TAPEMETH;
    u->tapemeth=i;
	return 0;
}

static apfcn_nc skip_token(char *p)
{
	while(*p && !isspace((BYTE)*p)) p++;
	while(isspace((BYTE)*p)) p++;
	return p;
}

static apfcn_i ParseLineCommand(char *line)
{
	int i;
	char *p,*pNote;

	line=skip_token(line); /*skip first token and trailing ws*/
	bLrudOnly=FALSE;
	*NoteBuf=0;
	if(bFixed) {
		line=skip_token(line);
		pNote=strchr(line,SRV_CHAR_LINENOTE);
		p=strchr(line,SRV_CHAR_COMMAND);
		if(pNote && (!p || p>pNote)) {
			*pNote=0;
			while(p && toupper(p[1])!='S') p=strchr(p+1,SRV_CHAR_COMMAND); 
			if(p) p[-1]=0;
			strcpy(NoteBuf,pNote+1);
			if(!p) return 0;
		}
    }
	else {
		if(*line==SRV_CHAR_LRUD || *line=='*') {
			//Determine if this is an LRUD expression as opposed to a second station name --
			pNote=strchr(line+1,(*line==SRV_CHAR_LRUD)?SRV_CHAR_LRUDEND:'*');
			if(pNote && (!pNote[1] || isspace((BYTE)pNote[1]))) {
				//lrud is terminated
				if(!(p=strchr(line+1,SRV_CHAR_COMMAND)) || p>pNote) {
					bLrudOnly=1;
					line=pNote+1;
				}
			}
			else {
			    //This could be a non-terminated LRUD or a station beginning with * or <
				//check_lrudonly(line);
				line=skip_token(line);
			}
		}
		else line=skip_token(line);

		if(!bLrudOnly && (*line==SRV_CHAR_LRUD || *line=='*')) bLrudOnly=2;
		p=strchr(line,SRV_CHAR_COMMAND);
	}

	/*For now, reject other than #SEG line commands --*/

	if(!p) return 0;

	if((i=cfg_GetArgv(p,SRV_CHAR_COMMAND))<0 || cmdtyp[i]!=CMD_SEG) {
	  set_charno(0);
	  return SRV_ERR_LINECMD;
	}
    *p=0;

    if(cfg_argc<2) {
      if(cfg_argc && set_charno(0)) return SRV_ERR_CMDBLANK;
      return 0;
    }
	/*We must already have pSEG==segMain --*/
	segMain_id=pVec->seg_id;
	memcpy(pSEG=&segLine,&segMain,sizeof(SEGTYP));
	return ParseSegment(cfg_argv[1]);
}

static int GetPrefix(int *nPrefix,PFX_COMPONENT *prefix)
{
	int n;
	char buf[128*SRV_MAX_PREFIXCOMPS];
	LPSTR p=buf;

	for(n=SRV_MAX_PREFIXCOMPS-1;n>=0;n--) {
		if(p!=buf) *p++=SRV_CHAR_PREFIX;
		if(prefix[n][0]) {
			strcpy(p,prefix[n]);
			p+=strlen(p);
		}
	}
	*p=0;
	if(!*buf) {
		*nPrefix=0;
		return 0;
	}
	if(strlen(buf)>=SRV_LEN_PREFIX) return SRV_ERR_LENPREFIX;
	if(!(*nPrefix=srv_CB(FCN_PREFIX, buf))) return SRV_ERR_PREFIX;
	return 0;
}

static int GetPrefixOverride(WORD *wPrefix,char *psrc)
{
	PFX_COMPONENT prefix[SRV_MAX_PREFIXCOMPS];
    char *p,*p0=psrc;
	int n,len=0,nmax=0;

	for(p=p0;*p;p++,len++) if(*p==SRV_CHAR_PREFIX) nmax++;

	if(len>=SRV_LEN_PREFIX || nmax>=SRV_MAX_PREFIXCOMPS)
		return (len>=SRV_LEN_PREFIX)?SRV_ERR_LENPREFIX:SRV_ERR_COMPPREFIX;

	//nmax = index of last component to replace

	if(nmax) {
		for(n=0;n<nmax;n++) {
			p=strchr(p0,SRV_CHAR_PREFIX);
			len=p-p0;
			memcpy(prefix[nmax-n],p0,len);
			prefix[nmax-n][len]=0;
			p0=p+1;
		}
	}
	strcpy(prefix[0],p0);

	while(++nmax<SRV_MAX_PREFIXCOMPS)
		strcpy(prefix[nmax],u->prefix[nmax]);

	if(len=GetPrefix(&n,&prefix[0]))
		return len;

	*wPrefix=(WORD)n;

	return 0;
}

static int parse_name(int i)
{
	char *psrc=cfg_argv[i];
	char FAR *pdest=pVec->nam[i];
	char *p;
	int e;

	/*Check for prefix override --*/
	if(p=strrchr(psrc,SRV_CHAR_PREFIX)) {
		*p=0;
		if(e=GetPrefixOverride(&pVec->pfx[i],psrc)) {
			pVec->charno=psrc-ln;
			return e;
		}
		*p++=SRV_CHAR_PREFIX; //restore cfg_argv[i]
		psrc=p;
	}
	else {
		if(u->nPrefix==-1 && (e=GetPrefix(&u->nPrefix,&u->prefix[0]))) {
			pVec->charno=psrc-ln;
			return e;
		}
		pVec->pfx[i]=(WORD)u->nPrefix;
	}

	if((i=strlen(psrc))>SRV_SIZNAME) {
		pVec->charno=psrc-ln;
		return SRV_ERR_SIZNAME;
	}

	if(u->namecase==U_UPPERCASE) strupr(psrc);
	else if(u->namecase==U_LOWERCASE) strlwr(psrc);

	memcpy(pdest,psrc,i);
	return 0;
}

static apfcn_v ClearNames(void)
{
    memset(pVec->nam[0],' ',2*SRV_SIZNAME); /*Arrays are contiguous!*/
}

static apfcn_i FixSegPath(void)
{
    /*NOTE: This function is called only when pSEG->nPending > 0*/
    
    /*iLen[i] is the length of subpath i, i=0..nSegs-1,
      exclusive of any trailing "/".
      iLen[nSegs-nPending] is the length of the next subpath
      that needs updating (before returning the next vector)
      when nPending>0. (we assume iLen[-1]==0)
    */
    
    int e; 
    
    if(!pSEG->nSegs) SegBuf[0]=0;
    else {
        memcpy(SegBuf+1,pSEG->Buf,
          SegBuf[0]=pSEG->iLen[pSEG->nSegs-pSEG->nPending]);
    }
    e=srv_CB(FCN_SEGMENT,SegBuf);
    if(!--pSEG->nPending && pSEG==&segMain) segMain_id=pVec->seg_id;
    
    return e;
}

static char *fixnote(void)
{
	char *p,*p0=NoteBuf;
	for(p=p0;*p;p++) {
		if(*p=='\\' && p[1]=='n') {
			*p0++='\n'; p++;
		}
		else *p0++=*p;
	}
	*p0=0;
	return NoteBuf;
}

static apfcn_i ParseNote(char *line)
{
	int e,len;
	
    cfg_quotes=TRUE;
	cfg_GetArgv(line,CFG_PARSE_ALL);
	cfg_quotes=FALSE;
	
    if(cfg_argc<2) {
      set_charno(0);
      return SRV_ERR_NOTEARG;
    }
    ClearNames();
     
	if(e=parse_name(0)) return e;
	
	len=0;
	if(cfg_argv[1][0]=='/') ++cfg_argv[1];
	for(e=1;e<cfg_argc;e++) {
	  if((len+strlen(cfg_argv[e]))>SRV_LEN_NOTESTR) break;
	  strcpy(NoteBuf+len,cfg_argv[e]);
	  len+=strlen(cfg_argv[e]);
	  NoteBuf[len++]=' ';
	}
	if(!len) return SRV_ERR_LINLEN; /*Can't happen*/
	NoteBuf[len-1]=0;
	
	while(pSEG->nPending) if(e=FixSegPath()) return e;
	
	return srv_CB(FCN_NOTE,fixnote());	
}

static apfcn_i ParseFlagName(char *line)
{
   int e;
   char *p;
   
   while(pSEG->nPending) if(e=FixSegPath()) return e;
	
   if((p=strchr(line,'/')) || (p=strchr(line,'\\'))) {
     *p++=0;
     _strupr(p);
   }
   if(e=srv_CB(FCN_FLAGNAME,p)) {
     pVec->charno=p?(p-ln):0;
     return e;
   }
   return 0;
}   

static apfcn_i ParseFlag(char *line)
{
	int i,e;

	//If no arguments, we are setting the default fixed flag to null;
	if(cfg_argc<2) {
		fixed_flagid=(WORD)-1;
		return 0;
	}
	
	if(e=ParseFlagName(line)) return e;

	//Flagname is now trimmed off --
    cfg_GetArgv(line,CFG_PARSE_ALL);

	if(!cfg_argc) {
		//This is the default flag for fixed stations --
		fixed_flagid=pVec->charno;
		pVec->charno=0;
		return 0;
	}

	for(i=0;i<cfg_argc;i++) {
	  line=cfg_argv[i];
	  cfg_argv[0]=line;
	  ClearNames();
	  if((e=parse_name(0)) || (e=srv_CB(FCN_FLAG,NULL)) && set_charno(i)) return e;
	}  
	return 0;	
}

static apfcn_i ParseSymbol(char *line)
{
	int e;
	char *p;
	DWORD rgb[3],clr=(DWORD)-1;
	
	
	if(e=ParseFlagName(line)) return e;
    pVec->charno=0; //Ignore assigned Flag ID

	if(p=strchr(line,'(')) {
	  *p++=0;
	  cfg_GetArgv(p,CFG_PARSE_ALL);
	  if(cfg_argc>=3) {
		  for(e=0;e<3;e++) {
			  p=cfg_argv[e];
			  if(!isdigit((BYTE)*p) || (DWORD)(rgb[e]=atoi(p))>255) break;
		  }
	  }
	  if(e<3) {
		  set_charno(e);
		  return SRV_ERR_BADCOLOR;
	  }
	  clr=RGB(rgb[0],rgb[1],rgb[2]);
	  e=0;
	}

    cfg_GetArgv(line,CFG_PARSE_ALL);
	
	if(cfg_argc>1) {
	  set_charno(1);
	  return SRV_ERR_NONAMEPFX;
	}
	if(cfg_argc) {
		pVec->lineno=clr;
		if(e=srv_CB(FCN_SYMBOL,_strupr(cfg_argv[0]))) set_charno(1);
		pVec->lineno=nLines;
	}
	return e;	
}

static int GetPrefixComponent(LPSTR pDst,LPCSTR pSrc)
{
	if(strlen(pSrc)>=SRV_LEN_PREFIX) return SRV_ERR_LENPREFIX;
	if(strchr(pSrc,SRV_CHAR_PREFIX)) return SRV_ERR_CHRPREFIX;
	strcpy(pDst,pSrc);
	return 0;
}

static int ParsePrefix(int cmd)
{ 
	int e=0;
    u->nPrefix=-1; //Indicate unresolved prefix
	u->prefix[cmd][0]=0;
    if((cfg_argc>2 && (e=SRV_ERR_PREFIX)) ||
	   (cfg_argc>1 && (e=GetPrefixComponent(u->prefix[cmd],cfg_argv[1])))) {
		set_charno(1);
	}
    return e;
}

static apfcn_i ParseDate(char *p)
{
    //Allow mm-dd-yy, mm-dd-yyyy, or yyyy-mm-dd
    char *p1,*p2;
    int y,m,d;
	DWORD dw;
	int e=SRV_ERR_DATE;
    
    Strxchg(p,'-','/');
    if((p1=strchr(p,'/')) && (p2=strchr(p1+1,'/'))) {
       if(p1-p==4) {
       	  //yyyy-mm-dd
          y=atoi(p); m=atoi(p1+1); d=atoi(p2+1);
       }
       else {
          //mm-dd-yyyy
          y=atoi(p2+1); m=atoi(p); d=atoi(p1+1);
       }
       if(y<100) y+=1900;
       if(y>=1000 && y<2100 && m>0 && m<13 && d>0 && d<32) {
		  dw=(y<<9)+(m<<5)+d; // 15:4:5
		  memcpy(pVec->date,(PBYTE)&dw,3);
		  if(!bUseDates) return 0;
		  if(!(e=srv_CB(FCN_DATEDECL,(LPSTR)&u->declination))) return 0;
	   }
    }
    set_charno(1);
    return e;
}

static apfcn_i ParseComment(void)
{
  int level;
  int e=SRV_ERR_COMMOPEN;
  int n=nLines;
  
  if(*cfg_argv[0]!=']') {
	level=1;
	while((e=cfg_GetLine())!=CFG_ERR_EOF) {
		if(e<=0 || cmdtyp[e]!=CMD_COMMENT) continue; //Ignore "#\n"
		if(*cfg_argv[0]==']') {
			if(!--level) return 0;
		}
		else level++;
	}
	e=SRV_ERR_COMMCLOSE;
  }
  pVec->charno=0;
  pVec->lineno=n;
  return e;
}

static apfcn_i parse_lengthunits(int typ,char *p)
{
	int c;
	      					
	if(!p) return SRV_ERR_NOPARAM;
	c=toupper(*p);
	
	if(c!='M' && c!='F') return SRV_ERR_LENUNITS;
	c=(c=='F');
	if(typ==ARG_S) u->bFeetS=c;
	else u->bFeet=c;
	return 0;
}

static apfcn_i ParseUnitKeys(void)
{
  char *p;
  int i,arg;
  int retval=0;
  
  cfg_KeyInit(argkey,NUM_ARGKEY);
  
  for(i=0;i<cfg_argc;i++) {
    if((p=cfg_argv[i])==cfg_equalsptr) {
      unityp[i]=ARG_EQUALS;
      if(i+1>=cfg_argc) {
        retval=SRV_ERR_NOPARAM;
        break;
      }
      unityp[++i]=ARG_PARAM;
      continue;
    }

	if(*p==SRV_CHAR_MACRO) {
		unityp[i]=ARG_MACRODEF;
		continue;
	}
    
    if((arg=cfg_FindKey(p))<=0) {
      if(isnumeric(p)) {
		unityp[i]=ARG_NUMERIC;
        continue;
      }
	  retval=strcmp(p, "DATE")?SRV_ERR_SPECIFIER:SRV_ERR_UNITSDATE;
	  break;
    }
    
    if((unityp[i]=argtyp[arg])<=ARG_SAVE) {
      switch(unityp[i]) {
        case ARG_RESET	:	*u=unitsDflt; break;
        case ARG_RESTORE:	if(u==u_stack) retval=SRV_ERR_RESTORE;
        					else u--;
        					break;
							
        case ARG_SAVE	:	if(u==u_stack+U_LEVELS-1) retval=SRV_ERR_SAVE;
        					else {
        					  u++;
        					  *u=*(u-1);
        					}
        					break;
      }
      if(retval) break;
    }
  }

  cfg_KeyInit(cmdkey,NUM_CMDKEY);
  if(retval) set_charno(i);
  return retval;
}

static apfcn_i alloc_macrostr(char *s)
{
	int len=strlen(s)+1;
	memcpy(macro_buf+macro_stridx,s,len);
	macro_bufspc-=len;
	macro_stridx+=len;
	return macro_stridx-len;
}

static apfcn_i define_macro(char *name,char *value)
{
	int i;

	if(!macro_buf) {
		macro_cnt=macro_maxcnt=macro_bufspc=macro_numblks=0;
		macro_stridx=SIZ_MACRO_STR;
		macro_def=NULL;
	}
	//A maximum of two allocations here --
	if(macro_cnt==macro_maxcnt) {
		macro_def=(MACRO_DEF *)realloc(macro_def,(macro_maxcnt+=INC_MACRO_DEF)*sizeof(MACRO_DEF));
		if(!macro_def) return SRV_ERR_NOMEM;
	}
	if(macro_bufspc<SIZ_MACRO_STR) {
		macro_buf=(char *)realloc(macro_buf,++macro_numblks*INC_MACRO_BLK+SIZ_MACRO_STR);
		if(!macro_buf) return SRV_ERR_NOMEM;
		macro_bufspc+=INC_MACRO_BLK;
	}

	//Skip over leading '$' --
	if((i=find_macro(++name))<0) {
		//New macro
		i=macro_cnt++;
		macro_def[i].namidx=alloc_macrostr(name);
	}
	//else redefine an existing macro

	macro_def[i].validx=alloc_macrostr(value);

	return 0;
}

static apfcn_i ParseCommand(int cmd,char *p)
{
  int i,retval;
  int i_incA,i_incAB,i_incV,i_incVB,i_incH,i_incD;
  
  if(ismacro(p) && !(p=expand_macros(p))) {
	return SRV_ERR_MACRONODEF;
  }

  switch(cmd) {
    case CMD_DATE:    return ParseDate(p);
    case CMD_SYM :    return ParseSymbol(p);
    case CMD_SEG :    return ParseSegment(p);
    case CMD_NOTE:    return ParseNote(p);
    case CMD_FLAG:    return ParseFlag(p);
	case CMD_PREFIX:
    case CMD_PREFIX2:
	case CMD_PREFIX3: return ParsePrefix(cmd-CMD_PREFIX);

    case CMD_COMMENT: return ParseComment();
    case CMD_UNITS:   break;
    default:          return SRV_ERR_DIRECTIVE; /*impossible*/
  }
  
  /*Translate keywords and process possible RESET argument --*/

  /*Parse arguments while separating equivalences --*/
  cfg_equals=cfg_quotes=TRUE;
  cfg_GetArgv(p,CFG_PARSE_ALL);
  cfg_equals=cfg_quotes=FALSE;
  if(retval=ParseUnitKeys()) return retval;
  
  i_incA=i_incAB=i_incV=i_incVB=i_incH=i_incD=0;
    
  /*Process #Units arguments --*/
  for(i=0;i<cfg_argc;i++) {
  
    cmd=unityp[i];
    
    if(i+1<cfg_argc && unityp[i+1]==ARG_EQUALS) {
		p=cfg_argv[i+=2];
	}
    else p=NULL;
    
    switch(cmd) {
    
      /*NOTE: We "continue" if the argument is parsed successfully --*/
      
      case ARG_RESET  :
      case ARG_RESTORE:
      case ARG_SAVE	  : continue;

	  case ARG_MACRODEF : retval=p?define_macro(cfg_argv[i-2],p):define_macro(cfg_argv[i],"");
						  if(retval) break;
						  continue;
     
      case ARG_CT     : if(p) {
						  retval=SRV_ERR_EQUALS;
						  break;
						}
						u->vectype=U_POLAR;
						continue;
						
	  case ARG_TYPEAB :
	  case ARG_TYPEVB : if(p) {
	                      *p=toupper(*p);
	                      if(*p!='N' && *p!='C') {
	                        retval=SRV_ERR_TYPEBACK;
	                        break;
	                      }
	                      if(cmd==ARG_TYPEAB) u->revAB=(*p=='C');
	                      else u->revVB=(*p=='C');

	                      if(p[1]==':' || (i+1<cfg_argc && unityp[i+1]==ARG_NUMERIC)) {
							if(p[1]==':') p+=2;
							else p=cfg_argv[++i];
	                        fTemp=atof(p);
	                        if(fTemp<0.0 || fTemp>90.0) {
	                          retval=SRV_ERR_FSBSERR;
	                          break;
	                        }
	                        fTemp=(fTemp+0.01)*U_DEGREES;
							if(i+1<cfg_argc && unityp[i+1]==ARG_X) {
							  i++;
							  if(cmd==ARG_TYPEAB) u->revABX=1;
							  else u->revVBX=1;
							}
	                      }
	                      else fTemp=MAX_BACKDELTA;

	                      if(cmd==ARG_TYPEAB) u->errAB=fTemp;
	                      else u->errVB=fTemp;
	                      continue;
      					}
      					retval=SRV_ERR_NOPARAM;
      					break;
                          
      case ARG_CASE   : if(p) {
      					  if((cmd=toupper(*p))=='U' || cmd=='L') u->namecase=(cmd=='U')?U_UPPERCASE:U_LOWERCASE;
      					  else u->namecase=U_ANYCASE;
      					  continue;
      					}
      					retval=SRV_ERR_NOPARAM;
      					break;
                 
	  case ARG_S	  :
      case ARG_D      : if(retval=parse_lengthunits(cmd,p)) break;
      					continue;
      			        
      case ARG_GRID	  :
      case ARG_DECL   : if(retval=parse_incangle(cmd,i)) break;
      			        continue;
      			 
      case ARG_METERS : u->bFeet=u->bFeetS=FALSE; continue;
      case ARG_FEET   : if(p) {
      					  retval=SRV_ERR_EQUALS;
      					  break;
      					}
      					u->bFeet=u->bFeetS=(cmd==ARG_FEET);
      					continue;

	  case ARG_FLAG	  : fixed_flagid=(WORD)-1;
					    if(p) {
							if(retval=srv_CB(FCN_FLAGNAME,_strupr(p))) break;
							fixed_flagid=pVec->charno;
							pVec->charno=0;
						}
						continue;
#ifdef _USE_ARGNOTE
	  case ARG_NOTE:  fixed_typnote=0;
					  if(p) {
						  switch(toupper(*p)) {
							  case 'S' : fixed_typnote=1; break;
							  case 'N' : fixed_typnote=2; break;
							  case 'T' : fixed_typnote=3; break;
						  }
					  }
					  continue;
#endif
	      
      case ARG_RECT   : if(!p) u->vectype=U_RECT;
      			 		else if(retval=parse_incangle(ARG_RECT,i)) break;
		      			continue;
      			       			 
						//Postpone correction processing until the end since
						//Processing these items will depend on assumed units.
	  case ARG_INCD   : i_incD=i;  continue;
	  case ARG_INCH   : i_incH=i;  continue;	  
	  case ARG_INCA   : i_incA=i;  continue;
	  case ARG_INCAB  : i_incAB=i; continue;
	  case ARG_INCV   : i_incV=i;  continue;
	  case ARG_INCVB  : i_incVB=i; continue;

      case ARG_LRUD  : if(retval=parse_lrudmeth(p)) break;
      					continue;
	  
      case ARG_ORDER  : if(retval=parse_order(p)) break;
      					continue;
      
      case ARG_TAPE   : if(retval=parse_tape(p)) break;
      					continue;
      			        
      case ARG_PREFIX  :
	  case ARG_PREFIX2 :  
	  case ARG_PREFIX3 :  
						u->nPrefix=-1; //indicates unresolved prefix
						if(p) {
							if((retval=GetPrefixComponent(u->prefix[cmd-ARG_PREFIX],p)))
								break;
						}
						else
							u->prefix[cmd-ARG_PREFIX][0]=0; //empties prefix component

				 		continue;
      case ARG_UVH	  :
      case ARG_UVV	  :
      case ARG_UV     : if(!p) {
      						retval=SRV_ERR_NOPARAM;
      						break;
      					}
      				    if(!parse_numeric(&fTemp,p) && fTemp>=0.0) {
				   			if(cmd!=ARG_UVV) u->variance[0]=NET_VAL_FACT*fTemp;
				   			if(cmd!=ARG_UVH) u->variance[1]=NET_VAL_FACT*fTemp;
				   			continue;
				 		}
				 		retval=SRV_ERR_UV;
				 		break;
      
      case ARG_A 	  :
      case ARG_AB	  :
      case ARG_V 	  :
      case ARG_VB	  : if(retval=set_angle_units(cmd,p)) break;
						continue;
						
	  default		  : retval=SRV_ERR_SPECIFIER; //Could be ARG_NUMERIC
    }
    
    break; /*error*/
  }
  
  if(!retval) {
	  /*Length corrections are maintained in meters --*/
	  if(i_incH)  retval=parse_inclength(&u->incH,i=i_incH,u->bFeet);
	  if(!retval && i_incD)  retval=parse_inclength(&u->incD,i=i_incD,u->bFeet);
	  /*Angle corrections are maintained in radians --*/
	  if(!retval && i_incA)  retval=parse_incangle(ARG_INCA,i=i_incA);
	  if(!retval && i_incAB) retval=parse_incangle(ARG_INCAB,i=i_incAB);
	  if(!retval && i_incV)  retval=parse_incangle(ARG_INCV,i=i_incV);
	  if(!retval && i_incVB) retval=parse_incangle(ARG_INCVB,i=i_incVB);
  }
      
  if(retval) set_charno(i);
  return retval;
} 

static int parse_hv(char *p0,int i)
{
    int iret,e;
    char *p;

	bRMS=FALSE;
    
    p=strchr(p0,SRV_CHAR_VAREND);
    
    if(p) {
      *p=0;
      iret=0;
    }
    else {
      iret=SRV_ERR_NOPAREN;
      if(i) return iret;
    }

    switch(*p0) {
		case SRV_CHAR_FLTTRAV :
			pVec->flag|=(i?NET_VEC_FLTTRAVV:NET_VEC_FLTTRAVH);
			break;

		case SRV_CHAR_FLTVECT :
			pVec->flag|=(i?NET_VEC_FLTVECTV:NET_VEC_FLTVECTH);
			break;
		case 0 :
		    if(!i && !iret) iret=SRV_ERR_VARIANCE; /*An empty expression*/
			break;
 
		case 'R' :
		case 'r' : bRMS=TRUE; p0++;

		default:
		   if(e=parse_length(&fTemp,p0,u->bFeet)) iret=e;
		   else if(fTemp<0.0) iret=SRV_ERR_VARIANCE;
		   if(!iret || iret==SRV_ERR_NOPAREN) {
			   if(bRMS) {
				   //Default variance of a component is (vector length in meters)/328 msq
				   //ftemp is RMS, which is sqrt(variance) if i==1,
				   //or sqrt(variance)/sqrt(2) = sqrt(variance/2) if i==0.
				   //Therefore, since length = 328 * variance, ...
				   fTemp*=(164.0*fTemp);
				   if(i) fTemp+=fTemp;
			   }
			   pVec->hv[i]=fTemp*u->variance[i];
		   }
	}
	return iret; //could be SRV_ERR_NOPAREN if i==0, in which case a vertical spec remains
}

static int parse_variance(int i)
{
    int e;

    char *p=cfg_argv[i]+1;  /*skip open parenthesis*/
    
    e=parse_hv(p,0);
    
    if(e==SRV_ERR_NOPAREN && i+1<cfg_argc) {
      p=cfg_argv[++i];
      if(!(e=parse_hv(p,1))) return i;
    }
    else if(!e) {
      /*An end parenthesis was found. The override applies to all components*/
      if(e=(pVec->flag & NET_VEC_FLTTRAVH)) {
        /*The Hz vector or traverse was floated --*/
        pVec->flag |= (e==NET_VEC_FLTTRAVH)?NET_VEC_FLTTRAVV:NET_VEC_FLTVECTV;
      }
      else {
        //The Hz variance was numerically overridden.
		//Vert gets twice the variance of a horiz component if both assigned same RMS --
		if(bRMS) fTemp+=fTemp;
        pVec->hv[1]=(fTemp*u->variance[1]);
      }
      return i;
    }
    
	set_charno(i);
	return -e; 
}

static float get_lrud_az(void)
{
	if(fabs(pVec->xyz[0])<0.1 && fabs(pVec->xyz[1])<0.1) return LRUD_BLANK;

	return get_lrud_degrees(atan2(pVec->xyz[0],pVec->xyz[1]));
}

static BOOL LastToOK(int pfx,char *nam)
{
	return pLastTo && (bLrudPending || pLastTo->az!=(double)LRUD_BLANK) && pfx==pLastTo->pfx && !memcmp(nam,pLastTo->nam,SRV_SIZNAME);
}

static float get_az_bisector(double az1, double az2)
{
	double b=az1;
	if(az2<b) {
		b=az2;
		az2=az1;
	}
	az2-=b;
	if(az2<=180) b+=az2/2;
	else b+=(az2/2+180);
	if(b>=360) b-=360;
	return (float)b;
}

static int parse_lrud(int i)
{
	//Called either from parse_polar() with bLrudOnly=0,
	//or from srv_Next() (prior to parse_polar) when
	//bLrudOnly=1 or 2;

	int t,e;
	char *p;
	char char_lrudend;
	double val;
	BOOL bLrudEnd=FALSE;
 	float dim[5],fswap;
	byte flags=0;

	if(*cfg_argv[i]=='*') char_lrudend='*';
	else char_lrudend=SRV_CHAR_LRUDEND;
	cfg_argv[i]++;

	for(t=0;t<5;t++) dim[t]=LRUD_BLANK;
	lruddat.flags=0;
	lruddat.lineno=nLines;

	for(t=0;i<cfg_argc;i++) {
		if(p=strchr(cfg_argv[i],char_lrudend)) {
			bLrudEnd=TRUE;
			*p=0;
		}
		p=cfg_argv[i];
		if(*p) {
			if(t>5) return -SRV_ERR_LRUDARGS;
			if(*p!='-' || p[1]!='-') {
				if(t>3) {
					//Can now be either "C" or a facing direction --
					if(*p=='c' || *p=='C') {
						lruddat.flags|=LRUD_FLG_CS;
						if(bLrudEnd) break;
						continue;
					}
					if((e=get_fTempAngle(p,TRUE))>=0) {
						val=fTemp;
						e=0;
					}
					else e=-e;
				}	
				else e=parse_length(&val,p,u->bFeetS);
				if(e) {
					set_charno(i);
					return -e;
				}
				if(t<5) dim[t]=(float)val;
			}
		}
		if(bLrudEnd) break;
		t++;
	}

	if(i>=cfg_argc) return -SRV_ERR_NOPAREN;
	
	for(t=0;t<4;t++) lruddat.dim[t]=dim[u->lrudseq[t]];
	lruddat.az=dim[4];
	
	bLrudPending=FALSE;

	//get the name associated with this LRUD
	t=(u->lrudmeth<LRUD_T || bLrudOnly)?0:1;
	memcpy(lruddat.nam,pVec->nam[t],SRV_SIZNAME);
	lruddat.pfx=pVec->pfx[t];

    if(lruddat.az==LRUD_BLANK && (lruddat.dim[0] || lruddat.dim[1])) {
		//we need a direction since it wasn't specified --
		if(bLrudOnly) {
			//End or starting station LRUD -
			if(u->lrudmeth>=LRUD_T) {
				//Next shot's FROM name must match the name in lruddat --
				if(bLrudOnly>1) {
					if(!bErrTwoNames) log_error("Second name on LRUD-only line ignored when LRUD type is T.");
					bErrTwoNames=TRUE;
				}
				bLrudPending=1;
			}
			else {
			   //Get direction from pLastTo --
			   if(!LastToOK(lruddat.pfx,lruddat.nam)) {
				   log_lrud_error();
			   }
			   else {
				   lruddat.az=get_lrud_degrees(pLastTo->az);
				   if(bLrudOnly>1) {
					   //Backward compatibility -- two stations on a line
					   fswap=lruddat.dim[0];
					   lruddat.dim[0]=lruddat.dim[1];
					   lruddat.dim[1]=fswap;
				   }
			   }
			}
		}
		else {
		   if((lruddat.az=get_lrud_az())==LRUD_BLANK) {
			   //This is a vertical shot -- FROM name must match the last TO name,
			   //or the next shot's FROM name must match this TO-name, which will be placed in pLastTo->nam.
			   //Thus TO-station and FROM-station LRUDs are treated the same way: The direction of the
			   //previously established vector takes priority if it's non-vertical and the TO name matches
			   //the current FROM name. Otherwise, direction is obtained from the next establishied
			   //vector if it qualifies. If neither vector qualifies, the LRUD is ignored and
			   //log_lrud_error() is called to generate a non-fatal warning.
			   if(LastToOK(pVec->pfx[0],pVec->nam[0])) {
				   lruddat.az=get_lrud_degrees(pLastTo->az); //converts to degrees between 0 and 360
				   if((u->lrudmeth&1)) {
					   //bisector style!
					   bLrudPending=3; //we still need az of next vector to average with this one if it's available!
				   }
			   }
			   else bLrudPending=2; //use az of next vecor for this lrud
		   }
		   else {
			   //We'll use the az of this vector unless this is bisector LRUD
			   if((u->lrudmeth&1)) {
				   if(u->lrudmeth==LRUD_FB) {
					   if(LastToOK(pVec->pfx[0], pVec->nam[0]))
						   lruddat.az=get_az_bisector(lruddat.az, get_lrud_degrees(pLastTo->az));
				   }
				   else {
					   //use bisector of lruddat.az and next vector's azimuth --
					   bLrudPending=3;
				   }
			   }
		   }
		}
	}

	pLastTo=NULL;

	if(!bLrudPending && lruddat.az!=LRUD_BLANK) {
		if((t=srv_CB(FCN_LRUD,(LPSTR)&lruddat))!=0) return -t;
	}

	return i;
}

static int ParseRect(void)
{
  BYTE *iseq=u->iseq[U_RECT];
  int i,e;
  static double xyz[3];
  double ca,sa;
  BOOL bLatLong=FALSE;
  
  for(i=0;i<3;i++) {
    if(!iseq[i]) {
      /*No vertical component given*/
      xyz[i]=0.0;
      break;
    }
    if((e=parse_length_arg(&xyz[i],iseq[i],u->bFeet))) {
		if(i==2 && e==SRV_ERR_BLANK) {
			xyz[i]=0.0;
			continue;
		}
		if(bFixed && e==SRV_ERR_BADLENGTH && i==0) bLatLong=TRUE;
	}
	else if(bLatLong && i==1) {
		e=SRV_ERR_LATLONG;
		bLatLong=FALSE;
	}
	if(bLatLong && e==SRV_ERR_BADLENGTH) {
		e=parse_latlong(&xyz[i],cfg_argv[iseq[i]],i);
	}
	if(e) {
	  set_charno(iseq[i]);
	  return e;
	}
  }
  
  i+=2;   //== args parsed (including 2 names) == index of next arg
  
  pVec->hv[0]=pVec->hv[1]=0.0; /*Default variance is zero!*/
  if(cfg_argc>i) {
	if(cfg_argv[i][0]==SRV_CHAR_VAR) {
	  if((i=parse_variance(i))<0) return -i;
      i++;
    }
    if(i<cfg_argc && set_charno(i)) return SRV_ERR_UNEXPVALUE;
  }

  if(bLatLong) {
	if(pVec->ref_datum==255 && set_charno(0)) return SRV_ERR_NODATUM;
    for(i=0;i<2;i++) pVec->xyz[i]=xyz[i];
	if((e=srv_CB(FCN_CVTLATLON,NULL)) && set_charno(0))	return e;
    for(i=0;i<2;i++) xyz[i]=pVec->xyz[i];
  }
  
  if(bFixed) fTemp=u->gridnorth;
  else fTemp=u->rectnorth;

  if(bGenUTM) fTemp-=u->gridnorth;

  if(fTemp) {
    fTemp *= -U_DEGREES;
    ca=cos(fTemp);
    sa=sin(fTemp);
    fTemp  = xyz[0]*ca - xyz[1]*sa;
	xyz[1] = xyz[0]*sa + xyz[1]*ca;
    xyz[0] = fTemp;
  }
  
  if(bFixed) {
    for(i=0;i<3;i++) pVec->xyz[i]=xyz[i];
	if(bGenUTM) {
		if(pVec->ref_datum!=pVec->root_datum) {
			if((e=srv_CB(FCN_CVTDATUM,NULL))) {
				set_charno(0);
				return e;
			}
		}
		if(!bLatLong && pVec->ref_zone!=pVec->root_zone) {
			if((e=srv_CB(FCN_CVT2OUTOFZONE,NULL))) {
				set_charno(0);
				return e;
			}
		}
	}
  }
  else {
    ca=0.0;
    for(i=0;i<3;i++) {
      pVec->xyz[i]=xyz[i];
      ca+=xyz[i]*xyz[i];
    }
    pVec->len_ttl=sqrt(ca);
  }

  return 0;
}

static apfcn_i parse_height(double *val,int i)
{
	int e=parse_length_arg(val,i,u->bFeetS);
	
    //We allow possibility of "--" fields --
	if(e) {
	  if(e==SRV_ERR_NUMARGS) return -1;
	  if(e!=SRV_ERR_BLANK) return e;
	  *val=0.0;
	}
	return 0;
}

static BOOL name_isnull(int e)
{
	char *p=pVec->nam[e];
	if(*p!='-') return FALSE;
	return *++p==' ' || (*p=='-' && *++p==' ');
}

#pragma optimize("s",on)
int PASCAL parse_angle_arg(int typ,int i)
{
  char *p,*pbak;
  double bakval,delta;
  
  //Calls get_fTempAngle() while handling possible backsight --
  
  if(i>=cfg_argc || *(p=cfg_argv[i])==SRV_CHAR_VAR ||
    *p==SRV_CHAR_LRUD) return SRV_ERR_NUMARGS;
   
  if(pbak=strchr(p,'/')) {
      *pbak++=0;
	  if(!*pbak) return SRV_ERR_SLASH;
    
  	  if((i=get_fTempAngle(pbak,typ==TYP_A))<0) {
  	    if(i!=-SRV_ERR_BLANK || !*p) return -i;
  	    pbak=0;
  	  }
  	  else if(typ==TYP_V) {
		if(!i) {
			if(u->unitsVB==U_PERCENT)
				fTemp=atan(fTemp*0.01);
			else fTemp *= u->unitsVB;
		}
        //Apply correction if shot not vertical --
        if(fabs(fabs(fTemp)-PI/2)>ERR_FLT) fTemp+=u->incVB;
        if(fabs(fTemp)>(PI/2.0+ERR_FLT)) return SRV_ERR_VERTICAL;
        if(u->revVB) bakval=fTemp;
        else bakval=-fTemp;
      }
      else {
        if(!i) fTemp *= u->unitsAB;
        if(fTemp>(PI+PI+ERR_FLT) || fTemp<-ERR_FLT) return SRV_ERR_AZIMUTH;
        //Apply correction --
        if((fTemp+=u->incAB)>PI+PI) fTemp-=PI+PI;
        //Reverse shot if required --
        if(!u->revAB && (fTemp+=PI)>PI+PI) fTemp-=PI+PI;
        bakval=fTemp;
      }
  }
  
  if(!*p) {
    if(!pbak) fTemp=0.0;
    else fTemp=bakval;
    return 0;
  }
     
  if((i=get_fTempAngle(p,typ==TYP_A))<0) {
    if(i==-SRV_ERR_BLANK) {
		if(pbak) {
		  fTemp=bakval;
		  return 0;
		}
	}
    return -i;
  }
  
  if(typ==TYP_V) {
	if(!i) {
		if(u->unitsV==U_PERCENT)
			fTemp=atan(fTemp*0.01);
		else fTemp *= u->unitsV;
	}
    if(fabs(fabs(fTemp)-PI/2)>ERR_FLT) fTemp+=u->incV;
    if(fabs(fTemp)>(PI/2+ERR_FLT)) return SRV_ERR_VERTICAL;
    if(pbak) delta=u->errVB;
  }
  else {
    if(!i) fTemp *= u->unitsA;
    if(fTemp>(PI+PI+ERR_FLT) || fTemp<-ERR_FLT) return SRV_ERR_AZIMUTH;
    if((fTemp+=u->incA)>PI+PI) fTemp-=PI+PI;
	if(!pbak) return 0;
    delta=u->errAB;
    if(bakval-fTemp>PI) fTemp+=PI+PI;
    else if(fTemp-bakval>PI) bakval+=PI+PI;
  }
  
  if(pbak) {
    if(fabs(bakval-fTemp)>delta) {
	    delta=fabs(bakval-fTemp)*(180.0/PI);
	    ErrBackTyp=typ;
		if(!log_error("%s FS/BS diff: %6.1f deg, %s to %s",
			(typ==TYP_V)?"Vt":"Az",delta,cfg_argv[0],cfg_argv[1])) return SRV_ERR_BACKDELTA;
    }
	if(typ==TYP_A && !u->revABX || typ==TYP_V && !u->revVBX)
		fTemp=(fTemp+bakval)/2.0;
  }
  
  return 0;
}
#pragma optimize("t",on)

#ifdef _DEBUG
static int GetOldVec(double d, double az, double va, double iab, double tab)
{
	//d == corrected tape distance in meters
	//az == azimuth in radians (0 <= az < 2*PI)
	//va == inclination in radians (vd*PI/180), where -90d <= vd <= 90d has already been enforced
	//iab == height of clinometer above station
	//tab == height of target abobe station

	if(!d) {
		pVec->xyz[0]=pVec->xyz[1]=pVec->xyz[2]=0;
		return 0;
	}

	if(u->tapemeth!=TAPE_IT) {
		//Adjust d so that d is the measured distance between stations
		//based on d (taped distance), iab, and tab -- 
		if(u->tapemeth==TAPE_SS) fTemp=tab-iab;
		else if(u->tapemeth==TAPE_IS) fTemp=tab;
		else fTemp=-iab;

		double r2=fabs(fTemp);

		if(d<r2) {
			//if difference less that 1 ft, adjust d without warning
			if(r2-d<SS_ERR_FATAL) {
				d=r2;
			}
			else return SRV_ERR_IT_HEIGHTS;
		}
		iab-=tab;
		if(fTemp!=0.0) {
			tab=fTemp*sin(va);
			if((fTemp=d*d+tab*tab-fTemp*fTemp)>=0.0 && (d=tab+sqrt(fTemp))<0.0) d=0.0;
		}
	}
	else iab-=tab;

	/*For pure vertical shots we ignore the clinometer and height correction --*/
	if(fabs(fabs(va)-PI/2)>ERR_FLT) iab+=u->incH;

	fTemp=cos(va)*d; //=horizontal component length
	pVec->xyz[0]=sin(az)*fTemp;
	pVec->xyz[1]=cos(az)*fTemp;
	pVec->xyz[2]=sin(va)*d+iab;
	return 0;
}
#endif

static apfcn_i parse_polar(void)
{
  int e,i;
  BYTE *iseq=u->iseq[U_POLAR];
  double va,d,az,iab,tab;
  int iPureVert=0;
  int iSpray=0;
  
  if(name_isnull(1)) iSpray=2;
  else if(name_isnull(0)) iSpray=1;

  if(i=iseq[TYP_V]) {
    if(e=parse_angle_arg(TYP_V,i)) {
      if(e!=SRV_ERR_BLANK && !(iSpray && e==SRV_ERR_NUMARGS)) goto _error;
      fTemp=0.0;
    }
	else if(fabs(fabs(fTemp)-PI/2)<=ERR_FLT) {
		iPureVert=(fTemp<0)?-1:1;
		if(fabs(fTemp)>PI/2) fTemp=iPureVert*PI/2;
	}
    va=fTemp;
    i=5;
  }
  else {
    va=0.0;
    i=4;
  }

  if(e=parse_angle_arg(TYP_A,iseq[TYP_A])) {
    if(!iPureVert || e!=SRV_ERR_NUMARGS && e!=SRV_ERR_BLANK) {
      i=iseq[TYP_A];
      goto _error;
    }
    //Pure vertical shot requires no azimuth
    if(e==SRV_ERR_NUMARGS) i--;
    az=0.0;
  }
  else {
	az=fTemp+u->declination*U_DEGREES;
	if(bGenUTM) az-=u->gridnorth*U_DEGREES;
  }
  
  if(e=parse_distance(&d,iseq[TYP_D])) {
	i=iseq[TYP_D];
	goto _error;
  }
  
  if(u->incD!=0.0 && d!=0.0) {
	if(d+u->incD>=0.0) d+=u->incD;
	else {
		e=SRV_ERR_SIGNREV;
		goto _error;
	}
  }
  
  /*Process iab/tab --*/
  iab=tab=0.0;
  if((e=parse_height(&iab,i))>0 || !e && (e=parse_height(&tab,++i))>0) goto _error;
  if(!e) i++; /*Move to next argument*/
 
  /*
	d		  - corrected tape distance in meters (non-negative)
	az		  - corrected azimuth (UTM or true north relative) in radians (0 <= az < 2*PI)
	va		  -	inclination in radians, where -PI/2 <= va <= PI/2
				iPureVert is 1 if |va-PI/2|<=0.001, -1 if |va+PI/2|<=0.001, 0 otherwise (0.001~=0.57d)
	iab		  - height of clinometer above station
	tab		  - height of target above station
  */

#ifdef _DEBUG
    double dbe=GetOldVec(d, az, va, iab, tab);
#endif

  double x,y,z;

  if(!d) {
	//We accept two stations positioned at the same location.
	//The default 0 variance assigned when d==0 could have been overridden,
	//in which case this is an adjustable estimate, not a fixed constraint.
	if(iab || tab) {
		log_error("IT heights ignored for zero-length shot."); //non-fatal
	}
	x=y=z=0;
  }
  else {
	assert(d>0.0);
	double c=cos(va); // 0 < c <= 1
	double s=sin(va); // -1 < s < 1
	if(u->tapemeth!=TAPE_IT) {
		double h;
		switch(u->tapemeth) {
			case TAPE_ST: h = iab; break;
			case TAPE_IS: h = -tab; break;
			case TAPE_SS: h = iab-tab; break;
			assert(0);
		}

		double r2=fabs(h);
		bool bFatal=false;

		if(d<r2) {
			//if difference less that 1 ft, adjust d without warning
			if(r2-d<SS_ERR_IGNORE) {
				d=r2;
			}
			else if(r2-d>SS_ERR_FATAL) {
				bFatal=true;
#ifdef _DEBUG
#if 0
				if(!dbe) e=SRV_ERR_TAPEMETH;
				goto _error;
#endif
#endif
			}
		}

	    r2=d*d-c*c*h*h;
		if(d*d < h*h) {
			//This equivalent to |h*s| > sqrt(r2) (or else r2 < 0).
			LPCSTR  pe;
			//3 cases worth distinguishing:
			if(va==0 && u->tapemeth==TAPE_SS) {
				//assume an underwater survey (most likely situation):
				if(bFatal) e=SRV_ERR_REL_DEPTH;
				else pe="Chg in depth greater than taped dist";
			} 
			else if(h*s < 0 && r2 > 0) {
				//Two positive solutions for x_hz:
				if(bFatal) e=SRV_ERR_HZ_LENGTH;
				else pe="Ambiguity due to |IH-TH| > taped dist";
			}
			else {
				if(bFatal) e=SRV_ERR_IT_HEIGHTS;
				else pe="IT heights conflict with taped dist";
			}

			if(bFatal) goto _error;
			log_error("%s! Dist assumed %.2fm (not %.2fm) to match %s.",pe,fabs(h),d,(*pe=='C')?"chg":"|IH-TH|");
			d=fabs(h);
			r2=d*d-c*c*h*h;
		}
		d = -h*s + sqrt(r2); //IT distance
	}
	z = d*s + iab-tab;
	if(!iPureVert) z += u->incH; //constant vertical positioning correction (rarely used)
	else if(fabs(fabs(va)-PI/2)<FLT_EPSILON) d=0; //remove effects of roundoff
	x = c*d; //horizontal component lenth
	y = cos(az)*x;
	x = sin(az)*x;
  }

#ifdef _DEBUG
  //Temporary test against old code --
  if(dbe || fabs(pVec->xyz[0]-x) > FLT_EPSILON ||
	 fabs(pVec->xyz[1]-y) > FLT_EPSILON ||
	 fabs(pVec->xyz[2]-z) > FLT_EPSILON)
  {
	  e=SRV_ERR_TAPEMETH; //only indicates mismatch (tape method already verified)
	  goto _error;
  }
#endif

  pVec->xyz[0]=x;
  pVec->xyz[1]=y;
  pVec->xyz[2]=z;

  //Check if this is simply a spray shot --
  if(iSpray) {
	  iSpray=1-(--iSpray);
	  if(name_isnull(iSpray)) {
		  set_charno(iSpray);
		  return SRV_ERR_SPRAYNAME;
	  }
	  if(pSEG==&segLine) {
		pSEG=&segMain; //discard any line-specific segment
		log_error("Segment directive ignored on spray shot.");
	  }
	  memcpy(lruddat.nam,pVec->nam[iSpray],SRV_SIZNAME);
	  lruddat.pfx=pVec->pfx[iSpray];
	  lruddat.dim[0]=LRUD_BLANK_NEG;
	  if(iSpray) az-=PI;
	  iSpray=(z<0.0);
	  lruddat.dim[2+iSpray]=(float)fabs(z);
	  lruddat.dim[3-iSpray]=(float)0;
	  if(iPureVert) {
		  lruddat.dim[1]=(float)0;
		  lruddat.az=LRUD_BLANK;
	  }
	  else {
		  lruddat.dim[1]=(float)fTemp;
		  lruddat.az=get_lrud_degrees(az);
	  }
	  lruddat.flags=0;
	  if((e=srv_CB(FCN_LRUD,(LPSTR)&lruddat))!=0) return e;
	  return -1;
  }

  if(bLrudPending) {
	  //The last data line was a LRUD_F? or LRUD_T? vertical shot (bLrudPending=2),
	  //or a starting LRUD_T?, from station bLrudOnly (bLrudPending=1),
	  //or a bisector style LRUD_TB is that needs this shot's az (bLrudPending=3).
	  //Get the direction from this vector --
	  if(iPureVert || 
		  (bLrudPending==1 && (lruddat.pfx!=pVec->pfx[0] || memcmp(lruddat.nam,pVec->nam[0],SRV_SIZNAME))) ||
		  (bLrudPending==2 && !LastToOK(pVec->pfx[0],pVec->nam[0]))) log_lrud_error();
	  else {
		  if(bLrudPending==3) {
			  lruddat.az=get_az_bisector(lruddat.az, get_lrud_degrees(az));
		  }
		  else lruddat.az=get_lrud_degrees(az);
	      if((e=srv_CB(FCN_LRUD,(LPSTR)&lruddat))!=0) return e;
	  }
	  bLrudPending=0;
  }
  
  fTemp=sqrt(x*x+y*y+z*z);
  pVec->len_ttl=fTemp;

  /*Use the vector's length as the default variance --*/
  pVec->hv[0]=fTemp*u->variance[0];
  pVec->hv[1]=fTemp*u->variance[1];

  if(iPureVert) {
	  if(bFixVtVert) pVec->hv[0]=fTemp*VAR_FIXFACT;
	  if(bFixVtLen) pVec->hv[1]=fTemp*VAR_FIXFACT;
  }
  
  /*Optional variance and LRUD expressions --*/
  for(;i<cfg_argc;i++) {
  	e=cfg_argv[i][0];
    if(e==SRV_CHAR_VAR && iseq) {
      if((i=parse_variance(i))<0) return -i;
      iseq=NULL; /*Only one variance override allowed*/
    }
    else if(e==SRV_CHAR_LRUD || e=='*') {
      if((i=parse_lrud(i))<=0) return -i;
    }
    else {
 	  e=SRV_ERR_UNEXPVALUE;
	  goto _error;
	}
  }
  
  //Prepare for possible end station LRUD or a subsequent vertical shot --
  pLastTo=&lastTo;
  lastTo.az=iPureVert?(double)LRUD_BLANK:az;
  lastTo.pfx=pVec->pfx[1];
  memcpy(lastTo.nam,pVec->nam[1],SRV_SIZNAME);
  return 0;
      
_error:
   set_charno(i);
   return e;
}    

int PASCAL srv_SetOptions(LPSTR pOpt)
{
	pVec->lineno=0;
	while(isspace((BYTE)*pOpt)) pOpt++;
	if(!*pOpt) return 0;
	strcpy(ln,pOpt);
	cfg_argc=0;
	return ParseCommand(CMD_UNITS,ln);
}

int PASCAL srv_Next()
{
  int e;
  char *p;

  //Fill pVec with vector data, returning 0 or error code --
 
  if(pSEG!=&segMain) {
  	pVec->seg_id=segMain_id;
  	pSEG=&segMain;
  }
  
  while((e=cfg_GetLine())>=0) {
    if(!cfg_argc) continue;
	e=cmdtyp[e];
	    
	pVec->lineno=nLines;
	bFixed=FALSE;
	      
	if(e) {
	    //A directive was encountered -- Only #PREFIX and #[ #] can stand alone
        if(cfg_argc<2 && (e<CMD_PREFIX && e>CMD_PREFIX3) && e!=CMD_FLAG && e!=CMD_COMMENT &&
          set_charno(0)) return SRV_ERR_CMDBLANK;
        
	    if(e!=CMD_FIX) {
			//If a #SEG directive was encountered, pSEG->nPending is set
			//to the number of segment specifications that must be processed
			//before returning the next vector --
			if(!(e=ParseCommand(e,cfg_argv[1]))) continue;
			return e;
		}
	    bFixed=TRUE;
	}
	
	p=cfg_argv[0];
	
	if(bFixed) p[strlen(p)]=' '; //Concatenate cfg_argv[1]
	      
	//This may truncate ln and set pSEG=&segLine and
	//pSEG->nPending > 0. It also sets bLrudOnly to 0, 1, or 2.
	if(e=ParseLineCommand(p)) return e;
	      
	cfg_GetArgv(p,CFG_PARSE_ALL);
	if(cfg_argc<3) return SRV_ERR_NUMARGS; 
	      
	/*Initially assume vector is non-flagged --*/
	pVec->flag=NET_VEC_NORMAL;
	      
	/*Blank fill name arrays --*/
	ClearNames(); 
	      
	if(bFixed) {
		pVec->flag|=NET_VEC_FIXED;
		if(!(e=parse_name(1))) e=ParseRect();
	}
	else {

		if((e=parse_name(0))) return e;

		if(bLrudOnly) {
			//We encountered LRUD in ParseLineCommand() --
		    if((e=parse_lrud(bLrudOnly))<=0) return -e;
		    continue;
		}
		
		if((e=parse_name(1))) return e;

		//WALLS will check that the two names are not identical!
		      
		//Compute float values --
		if(u->vectype==U_POLAR) {
			e=parse_polar();
			if(e==-1) continue; //spray shot
		}
		else e=ParseRect();
	}
	      
	while(!e && pSEG->nPending) e=FixSegPath();

#ifdef _USE_ARGNOTE
	if(!e && bFixed && (*NoteBuf || fixed_flagid!=(WORD)-1 || fixed_typnote)) {
		pVec->pfx[0]=pVec->pfx[1];
		memcpy(pVec->nam[0], pVec->nam[1], SRV_SIZNAME);
		if(!*NoteBuf && fixed_typnote) {
			switch(fixed_typnote) {
				case 1: memcpy(NoteBuf,pVec->nam[0],SRV_SIZNAME);
					    NoteBuf[SRV_SIZNAME]=0;
						for(LPSTR p=NoteBuf+SRV_SIZNAME-1; p>=NoteBuf && *p==' '; p--) {
							*p=0;
						}
						break;
				case 2: strcpy(NoteBuf,srvname); break;
				case 3: strcpy(NoteBuf,srvtitle); break;
				default: fixed_typnote=0;
			}
			if(*NoteBuf) e=srv_CB(FCN_NOTE, NoteBuf);
			fixed_typnote=0;
		}
		else {
			fixed_typnote=0;
			if(*NoteBuf) e=srv_CB(FCN_NOTE, fixnote());
		}
		if(!e && fixed_flagid!=(WORD)-1) e=srv_CB(FCN_FLAG, (LPSTR)&fixed_flagid);
	}
#else
	if(!e && bFixed && (*NoteBuf || fixed_flagid!=(WORD)-1)) {
		pVec->pfx[0]=pVec->pfx[1];
		memcpy(pVec->nam[0],pVec->nam[1],SRV_SIZNAME);
		if(*NoteBuf) e=srv_CB(FCN_NOTE,fixnote());
		if(!e && fixed_flagid!=(WORD)-1) e=srv_CB(FCN_FLAG,(LPSTR)&fixed_flagid);
	}
#endif	  
	return e;
  }

  if(e<CFG_ERR_EOF) {
    pVec->lineno=nLines;
    if(e==CFG_ERR_KEYWORD) {
      e=SRV_ERR_DIRECTIVE;
      set_charno(0);
    }
    else {
      e=SRV_ERR_LINLEN;
      pVec->charno=0;
    }
  }
  return e;
}

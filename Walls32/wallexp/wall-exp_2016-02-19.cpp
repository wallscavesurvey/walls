/*WALL-EXP.C -- SEF export compilation */
/*Compare with WALL-SRV.CPP -- Used to generate wallexp.dll*/

#include "wall-srv.h"
#include "assert.h"
/*======================================================================*/
#include "expsef.h"

typedef struct { //units=deg and m
	double d;
	double fazi;
	double bazi;
	double finc;
	double binc;
	double iab;
	double tab;
} EXP_VEC;

static EXP_VEC vec, vec_dflt={-9999,-9999,-9999,-9999,-9999,0,0};

static EXPTYP *pEXP;
static NTVRec ntvrec;
static FILE *fpout=NULL;
static char outpath[MAX_PATH];
#define LEN_LINE_COMMENT 77
static char line_comment[LEN_LINE_COMMENT+3];
static lpNTVRec pVec=&ntvrec;
static bool bNoInc,bLast_NoInc,bConvert;;
static LPSTR *pCC;
static int numCC;
static int maxCC;
static int iPureVert;

static UINT ctvectors,cpvectors,filegroups;
static double ct_decl;
static double ct_scale;
static char *ct_units;
const char *ct_fmt;
static BOOL ct_islrud;
static double ct_lrud[4];
static char ct_type[6];

static char def_pfx[SRV_LEN_PREFIX+2];
static char vec_pfx[2][SRV_LEN_PREFIX+2];
static char *pVec_pfx;

//forward references --
static void outSEF_CC();
static void CDECL addCC(const char *format, ...);
static int CDECL outSEF(const char *format, ...);
static void freeCC(void);

//================================================================================================

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
			ARG_INCV,ARG_INCVB,ARG_LRUD,ARG_METERS,ARG_ORDER,
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
static SRV_UNITS last_u;
static BYTE last_date[4];
static char seg[16], last_seg[16];

static FILE *fp=NULL;
LPSTR_CB srv_CB;
static DWORD nLines;
static char ln[LINLEN];
static char srvname[16];
static int segMain_id;
static int _scn_digit;
static BOOL bUseDates,bFixVtVert,bFixVtLen,bLrudOnly,bLrudPending,bFixed,bRMS;
static WORD fixed_flagid;

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
       SRV_ERR_VERSION,
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
	   SRV_ERR_ERRLIM
    };

	enum exp_err {
		EXP_ERR_INITSEF=SRV_ERR_ERRLIM, //==62
		EXP_ERR_CLOSE,
		EXP_ERR_LENGTH		//==64
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
       "Wrong version of WALL-" EXP_DLLTYPE ".DLL",
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
       "Segment path too long",
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
	   "Cannot initialize SEF file",
	   "Error closing SEF file",
	   "Prefixed name length > 16 chars"
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
	log_error("LRUD at station %s ignored. Facing direction can't be determined",lruddat.nam);
	nLines=e;
}

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

static char * trimtb(char *buf)
{
	char *p=buf+strlen(buf);
	while(p>buf && isspace((BYTE)*(p-1))) p--;
	*p=0;
	return buf;
}

static LPSTR getline(LPSTR buf, int sizbuf)
{
	if(!fgets(buf, sizbuf, fp)) {
		return NULL;
	}
	nLines++;
	while(isspace(*buf)) buf++;
	return trimtb(buf);
}

static int CALLBACK line_fcn(LPSTR buf,int sizbuf)
{
  char *p=getline(buf, sizbuf);
  if(!p) return CFG_ERR_EOF;

  //Output comments (directives will be saved when encountered) --
  while(!*p || *p==SRV_CHAR_COMMENT || *p==SRV_CHAR_COMMAND && p[1]=='[') {
	  if(*p==SRV_CHAR_COMMAND) {
		  p++;
		  while(1) {
			  addCC("%s",p);
			  if(!(p=getline(buf, sizbuf))) return CFG_ERR_EOF;
			  if(*p==SRV_CHAR_COMMAND && p[1]==']') break;
		  }
		  addCC("%s", p+1);
	  }
	  else if(*p++) {
		 addCC("%s", p);
	  }
	  if(!(p=getline(buf, sizbuf))) return CFG_ERR_EOF;
  }

  int len=strlen(p);
  memcpy(buf,p,len);
  buf[len]=0;
  *line_comment=0;
  if(p=strchr(buf, SRV_CHAR_COMMENT)) {
	  *p=0;
	  len=strlen(trimtb(buf));
      while(isspace(*++p) || *p=='#');
	  if(*p) strncpy(line_comment,p,LEN_LINE_COMMENT);
  }
  return len;
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

	bLrudPending=FALSE;
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
  
  bNoInc=false;
  i=strchr(_strupr(uc),'D')?0:1;
  if(!i && !strchr(uc,'V')) bNoInc=true;
  
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

static void parse_type(LPSTR pseg)
{
	LPSTR p=pseg+strlen(pseg)-1;
	if(p<pseg) return;
	for(int i=0;i<4;i++,p--) {
		if(p<pseg || *p=='/' || *p=='\\') {
			p++;
			break;
		}
	}
	if(*p) strcpy(ct_type,p);
}

static int ParseLineCommand(char *line)
{
	int i;
	char *p,*pNote;

	strcpy(ct_type,"*");

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
				if(!(p=strchr(line+1,SRV_CHAR_COMMAND)) || p>pNote) {
					bLrudOnly=1;
					line=pNote+1;
				}
			}
			else line=skip_token(line);
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
	parse_type(cfg_argv[1]);
	return 0;
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
	if(pEXP->flags&EXP_NOCVTPFX) strcpy(pVec_pfx,buf);
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
		pVec_pfx=vec_pfx[i];
		if(e=GetPrefixOverride(&pVec->pfx[i],psrc)) {
			pVec->charno=psrc-ln;
			return e;
		}
		*p++=SRV_CHAR_PREFIX; //restore cfg_argv[i]
		psrc=p;
	}
	else {
		pVec_pfx=def_pfx;
		if(u->nPrefix==-1 && (e=GetPrefix(&u->nPrefix,&u->prefix[0]))) {
			pVec->charno=psrc-ln;
			return e;
		}
		pVec->pfx[i]=(WORD)u->nPrefix;
		strcpy(vec_pfx[i],def_pfx);
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
		  pEXP->date=dw;
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
      retval=SRV_ERR_SPECIFIER;
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
    case CMD_SEG :    if(*p=='/') strncpy(seg,p+1,15);
	case CMD_SYM:
	case CMD_NOTE:
    case CMD_FLAG:    return 0;
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

	t=(u->lrudmeth<LRUD_T || bLrudOnly)?0:1;
	memcpy(lruddat.nam,pVec->nam[t],SRV_SIZNAME);
	lruddat.pfx=pVec->pfx[t];

    if(lruddat.az==LRUD_BLANK && (lruddat.dim[0] || lruddat.dim[1])) {

		if(bLrudOnly) {
			//End or starting station LRUD -
			if(u->lrudmeth>=LRUD_T) {
				//Next shot's FROM name must match the name in lruddat --
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
		else if((lruddat.az=get_lrud_az())==LRUD_BLANK) {
		   //Vertical shot -- FROM name must match the last TO name,
		   //or the next shot's FROM name must match this TO-name, which will be placed in pLastTo->nam.
		   //Thus TO-station and FROM-station LRUDs are treated the same way: The direction of the
		   //previously established vector takes priority if it's non-vertical and the TO name matches
		   //the current FROM name. Otherwise, direction is obtained from the next establishied
		   //vector if it qualifies. If neither vector qualifies, the LRUD is ignored and
		   //log_lrud_error() is called to generate a non-fatal warning.
		   if(LastToOK(pVec->pfx[0],pVec->nam[0])) lruddat.az=get_lrud_degrees(pLastTo->az);
		   else bLrudPending=2;
		}

	}

	pLastTo=NULL;

#if 1
	ct_lrud[0]=ct_lrud[1]=ct_lrud[2]=ct_lrud[3]=LRUD_BLANK;
	if(!bLrudPending && lruddat.az!=LRUD_BLANK) {
		for(e=0;e<4;e++) ct_lrud[e]=lruddat.dim[e];
		ct_islrud=TRUE;
	}
#else
	if(!bLrudPending && lruddat.az!=LRUD_BLANK) {
		if((t=srv_CB(FCN_LRUD,(LPSTR)&lruddat))!=0) return -t;
	}
#endif

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

  if(bFixed) fTemp=0; //bGenUTM=false in walls_srv.c
  else fTemp=u->rectnorth;

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
  }
  else {
    ca=0.0;
    for(i=0;i<3;i++) {
      pVec->xyz[i]=xyz[i];
      ca+=xyz[i]*xyz[i];
	  if(i==1) iPureVert=(ca<ERR_FLT*ERR_FLT)?((xyz[2]<0)?-1:1):0;
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
    
	  //get uncorrected backsight angle, either in radians (i=1),
	  //a number with default units assumed (i=0), or an error or blank (i<0)
  	  if((i=get_fTempAngle(pbak,typ==TYP_A))<0) {
  	    if(i!=-SRV_ERR_BLANK) return -i;
  	    pbak=0;
  	  }
  	  else if(typ==TYP_V) {
		if(!i) {
			if(u->unitsVB==U_PERCENT)
				fTemp=atan(fTemp*0.01);
			else fTemp *= u->unitsVB; //fTemp now in radians
			vec.binc=fTemp/U_DEGREES;
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
		vec.bazi=fTemp/U_DEGREES;
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
	vec.finc=fTemp/U_DEGREES;
    if(fabs(fabs(fTemp)-PI/2)>ERR_FLT) fTemp+=u->incV;
    if(fabs(fTemp)>(PI/2+ERR_FLT)) return SRV_ERR_VERTICAL;
    if(pbak) delta=u->errVB;
  }
  else {
    if(!i) fTemp *= u->unitsA;
    if(fTemp>(PI+PI+ERR_FLT) || fTemp<-ERR_FLT) return SRV_ERR_AZIMUTH;
	vec.fazi=fTemp/U_DEGREES;
    if((fTemp+=u->incA)>PI+PI) fTemp-=PI+PI;
	if(!pbak) return 0;
    delta=u->errAB;
    if(bakval-fTemp>PI) fTemp+=PI+PI;
    else if(fTemp-bakval>PI) bakval+=PI+PI;
  }
  
  if(pbak) {
	if(typ==TYP_A && !u->revABX || typ==TYP_V && !u->revVBX)
		fTemp=(fTemp+bakval)/2.0;
  }
  
  return 0;
}
#pragma optimize("t",on)

static apfcn_i parse_polar(void)
{
  int e,i;
  BYTE *iseq=u->iseq[U_POLAR];
  double va,d,az,iab,tab;
  int iSpray=0;
  
  if(name_isnull(1)) iSpray=2;
  else if(name_isnull(0)) iSpray=1;
  if(iSpray) return -1;

  if(i=iseq[TYP_V]) {
	//get fTemp=corrected inclination in radians 
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
  }

  
  if(e=parse_distance(&d,iseq[TYP_D])) {
	i=iseq[TYP_D];
	goto _error;
  }
  vec.d=d;

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
  vec.iab=iab; vec.tab=tab;

  if(!e) i++; /*Move to next argument*/
 
  /*
	d		  - corrected tape distance in meters (non-negative)
	az		  - corrected azimuth (UTM or true north relative) in radians (0 <= az < 2*PI)
	va		  -	inclination in radians, where -PI/2 <= va <= PI/2
				iPureVert is 1 if |va-PI/2|<=0.001, -1 if |va+PI/2|<=0.001, 0 otherwise (0.001~=0.57d)
	iab		  - height of clinometer above station
	tab		  - height of target above station
  */

  double x,y,z;

  if(!d) {
	//We accept two stations positioned at the same location.
	//The default 0 variance assigned when d==0 could have been overridden,
	//in which case this is an adjustable estimate, not a fixed constraint.
	if(iab || tab) log_error("IT heights ignored for zero-length shot"); //non-fatal
	x=y=z=0;
  }
  else {
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
	    double r2=d*d-c*c*h*h;
		if(d*d < h*h) {
			//This equivalent to |h*s| > sqrt(r2) (or else r2 < 0).
			//3 cases worth distinguishing:
			if(va==0 && u->tapemeth==TAPE_SS)
				//assume an underwater survey (most likely situation):
				e=SRV_ERR_REL_DEPTH;  //"Relative depth > taped distance"  
			else if(h*s < 0 && r2 > 0)
				//Two positive solutions for x_hz:
				e=SRV_ERR_HZ_LENGTH;  //"Ambiguity caused by |IH-TH| > distance"
			else
				e=SRV_ERR_IT_HEIGHTS; //"IT heights inconsistent with distance"
			goto _error;
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

  pVec->xyz[0]=x;
  pVec->xyz[1]=y;
  pVec->xyz[2]=z;

  if(bLrudPending) {
	  //The last data line was a FROM-station vertical shot
	  //or a TO-station LRUD-only (at from station).
	  //Get the direction from this vector --
	  if(iPureVert || 
		  (bLrudPending==1 && (lruddat.pfx!=pVec->pfx[0] || memcmp(lruddat.nam,pVec->nam[0],SRV_SIZNAME))) ||
		  (bLrudPending==2 && !LastToOK(pVec->pfx[0],pVec->nam[0]))) log_lrud_error();
	  else {
		  lruddat.az=get_lrud_degrees(az);
	      //if((e=srv_CB(FCN_LRUD,(LPSTR)&lruddat))!=0) return e;
		  //Output next FROM sta lrud for "T" method
	  }
	  bLrudPending=FALSE;
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

static int PASCAL exp_SetOptions(LPSTR pOpt)
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
			if(*line_comment) {
				memmove(line_comment+2, line_comment, strlen(line_comment)+1);
				*line_comment=' '; line_comment[1]=';';
			}
			if(cfg_argc>1) addCC("|#%s %s%s",cfg_argv[0],cfg_argv[1],line_comment);
			else addCC("|#%s%s", cfg_argv[0],line_comment);
			*line_comment=0;
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
		iPureVert=0;
		vec=vec_dflt;
		if(u->vectype==U_POLAR) {
			e=parse_polar();
			if(e==-1) continue; //spray shot
		}
		else {
			e=ParseRect();
		}
	}
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

//==============================================================================
//==============================================================================
static void exp_InitDefaults()
{
	pVec->charno=0;
	u=&u_stack[0];
	*u=unitsDflt;
	pVec->date[0]=pVec->date[1]=pVec->date[2]=0;
}

static int PASCAL exp_Open()
{
	if(fp) return SRV_ERR_REOPEN;
	strcpy(ln, pEXP->path);
	if((fp=fopen(ln, "r"))==NULL) return SRV_ERR_OPEN;
	strncpy(srvname, trx_Stpnam(ln), 15);
	*trx_Stpext(srvname)=0;
	*seg=*last_seg=0;
	u->gridnorth=pEXP->grid;
	bUseDates=(pEXP->flags&EXP_USEDATES)!=0;
	bNoInc=bLast_NoInc=false;
	filegroups=ctvectors=cpvectors=0;
	freeCC();
	//---
	nLines=0;
	cfg_LineInit((CFG_LINEFCN)line_fcn, ln, LINLEN, SRV_CHAR_COMMAND);
	cfg_KeyInit(cmdkey, NUM_CMDKEY);
	cfg_noexact=FALSE; /*Exact matching, but we will allow aliases*/
	cfg_commchr=SRV_CHAR_COMMENT;
	cfg_equals=cfg_quotes=FALSE; /*no equivalences except when parsing commands*/
	cfg_contchr=0; /*no continuation lines*/
	pSEG=&segMain;
	segMain_id=-1;
	segMain.nPending=segLine.nPending=bLrudPending=0;
	pLastTo=NULL;
	segMain.nSegs=segLine.nSegs=0;
	SegBuf[0]=0;
	macro_cnt=0;
	macro_buf=NULL;
	macro_def=NULL;
	return 0;
}

static void PASCAL exp_Close(void)
{
	freeCC();

	if(fp) {
		fclose(fp);
		fp=NULL;
	}
	if(macro_buf || macro_def) {
		free(macro_buf); macro_buf=NULL;
		free(macro_def); macro_def=NULL;
	}
}

static double NEAR PASCAL atn2(double a, double b)
{
	if(a==0.0 && b==0.0) return 0.0;
	return atan2(a, b)*(180.0/3.141592653589793);
}

static void convert_vec()
{
	//only remove declination from resolved vector
	double d2=pVec->xyz[0]*pVec->xyz[0]+pVec->xyz[1]*pVec->xyz[1];

	if(iPureVert || d2<ERR_FLT*ERR_FLT) vec.fazi=d2=0.0;
	else {
		vec.fazi=atn2(pVec->xyz[0], pVec->xyz[1])-ct_decl;
		if(!bConvert && (pEXP->flags&EXP_USE_ALLSEF)) {
			vec.fazi-=u->incA/U_DEGREES;
		}
		if(vec.fazi<0.0) vec.fazi+=360.0;
		else if(vec.fazi>=360.0) vec.fazi-=360.0;
	}
	if(iPureVert) vec.finc=iPureVert*90.0;
	else vec.finc=atn2(pVec->xyz[2], sqrt(d2));

	if(!bConvert && !iPureVert && (pEXP->flags&EXP_USE_ALLSEF)) {
		vec.finc-=u->incV/U_DEGREES;
	}
	vec.d=sqrt(d2+pVec->xyz[2]*pVec->xyz[2]);
	vec.iab=vec.tab=0;
	vec.bazi=vec.binc=-9999;
}

static void outSEF_F(double d,BOOL bComma)
{
   char buf[32];
   char *p=buf;
   snprintf(p,30,"%.4f",d); p[30]=0;
   if(strchr(p, '.')) {
	   int i=strlen(p);
	   while(i>1 && p[i-1]=='0') i--;
	   if(p[i-1]=='.') i--;
	   p[i]=0;
	   if(!*p) p="0";
   }
   if(bComma) strcat(p,(bComma>1)?"\r\n":",");
   outSEF_s(p);
}

static void outSEF_FD(double d, BOOL bComma)
{
	if(d==-9999) outSEF_s(bComma?"*,":"*");
	else outSEF_F(d*ct_scale,bComma);
}

static void outSEF_FA(double a, BOOL bComma)
{
	if(a==-9999) outSEF_s(bComma?"*,":"*");
	else outSEF_F(a, bComma);
}

static int CDECL outSEF(const char *format, ...)
{
	char buf[256];
	int iRet=0;
	va_list marker;
	va_start(marker, format);
	if(fpout && (iRet=_vsnprintf(buf, 256, format, marker))>0) fputs(buf, fpout);
	va_end(marker);
	return iRet;
}

static void freeCC(void)
{
	if(pCC) {
		for(int i=0; i<numCC; i++) free(pCC[i]);
		free(pCC);
		pCC=NULL;
		numCC=maxCC=0;
	}
}

static void outSEF_CC(void)
{
	if(pCC) {
		for(int i=0; i<numCC; i++) {
			outSEF("%s\r\n", pCC[i]);
		}
		freeCC();
	}
}

static void addCC(const char *format,...)
{
	char buf[256];
	*buf=buf[255]=0;
	va_list marker;
	va_start(marker, format);
	_vsnprintf(buf, 255, format, marker);
	va_end(marker);

	LPSTR pbuf=buf;
	while(*pbuf && (*pbuf=='#' || isspace(*pbuf))) pbuf++;
	if(!*pbuf) return;

	if(numCC>=maxCC) {
		pCC=(LPSTR *)realloc(pCC, (maxCC+=MAXCC_INC)*sizeof(LPSTR));
		if(!pCC) {
			numCC=maxCC=0;
			return;
		}
	}
	LPSTR *pcc=&pCC[numCC++];
	if(*pcc=(LPSTR)malloc(strlen(pbuf)+1)) strcpy(*pcc, pbuf);
	else  numCC--;
}

static char * timestr()
{
	time_t t;
	struct tm *ptm;
	static char timestr[20];

	timestr[0]=0;
	time(&t);
	ptm=localtime(&t);
	sprintf(timestr, "%02d:%02d %04d-%02d-%02d",
		ptm->tm_hour, ptm->tm_min, ptm->tm_year+1900,
		ptm->tm_mon+1, ptm->tm_mday);
	return timestr;
}

static int get_SEF_name(char *buf, int i)
{
	char nam[EXP_MAX_NAMLEN+2];
	char *p;

	memcpy(nam, pVec->nam[i], SRV_SIZNAME);
	p=nam+SRV_SIZNAME;
	while(p>nam && isspace((BYTE)p[-1])) p--;
	*p=0;
	if(pVec->pfx[i]) {
		if(!(pEXP->flags&EXP_NOCVTPFX)) {
			p=buf+10;
			*p=0; *--p=':';
			for(int j=pVec->pfx[i]-1; j>=0; j--) {
				*--p='A'+(j%26);
				j/=26;
			}
			strcpy(buf, p);
		}
		else {
			_snprintf(buf, EXP_MAX_NAMLEN, "%s:", vec_pfx[i]);
			buf[EXP_MAX_NAMLEN+1]=0;
		}
		if((i=strlen(buf)+strlen(nam))>EXP_MAX_NAMLEN) {
		    pEXP->maxnamlen=i;
			return EXP_ERR_LENGTH;
		}
		if(i>pEXP->maxnamlen) pEXP->maxnamlen=i;
		strcat(buf, nam);
	}
	else strcpy(buf, nam);
	return 0;
}

static bool compare_uCT()
{
	if(u->bFeet!=last_u.bFeet || u->vectype!=last_u.vectype || strcmp(seg, last_seg) ||
		memcmp(&pVec->date, &last_date, 3)) return true;
	if(u->vectype==U_RECT) return false;
	if(u->declination!=last_u.declination) return true;
	if(pEXP->flags&EXP_USE_ALLSEF) {
		if(bNoInc!=bLast_NoInc || u->tapemeth!=last_u.tapemeth || u->incA!=last_u.incA || u->incAB!=last_u.incAB ||
			u->incH!=last_u.incH || u->incV!=last_u.incV || u->incVB!=last_u.incVB ||u->incD!=last_u.incD)
			return true;
	}
	return false;
}

static void outSEF_group(LPCSTR type)
{
	LPCSTR p=(*seg)?seg:srvname;
	outSEF("#%s %s.%u - %s\r\n", type, p, ++filegroups, pEXP->title);
}

static void Out_CT_Header(void)
{
	strcpy(last_seg, seg);
	outSEF_group("ctsurvey");
	outSEF_CC();

	if(u->bFeet) {
		ct_units="feet";
		ct_scale=1.0/U_FEET;
	}
	else {
		ct_units="meters";
		ct_scale=1.0;
	}

	bConvert=u->vectype==U_RECT || u->incH || !(pEXP->flags&EXP_USE_ALLSEF);

	LPCSTR ptape=bConvert?"":"ss";

	ct_decl=u->declination;
	if(bConvert) {
		if(u->vectype==U_RECT) {
			outSEF_s("NOTE: Spherical coordinates were derived from RECT vectors in Walls.\r\n");
			ct_decl=0;
		}
		else if(bNoInc && u->tapemeth==TAPE_SS) outSEF_s("NOTE: Vectors were derived from a dive survey in Walls.r\n");
		else if(u->incH && (pEXP->flags&EXP_USE_ALLSEF)) outSEF_s("NOTE: Use of \"incH\" in Walls required use of simplified vector definitions.\r\n");
	}
	else {
		switch(u->tapemeth) {
		case TAPE_IT: ptape="it"; break;
		case TAPE_ST: ptape="st "; break;
		case TAPE_IS: ptape="is"; break;
		}
	}

	DWORD dw=*(DWORD *)pVec->date;
	// dw=(y<<9)+(m<<5)+d; //15:4:5
	if(dw&0xFFFFFF)	outSEF("#date %02d/%02d/%d\r\n", (dw>>5)&0xF, dw&0x1F, (dw>>9)&0x7FFF);

	outSEF_s("#decl "); outSEF_F(ct_decl, 2);

	outSEF("#units %s\r\n", ct_units);

	if(!bConvert) {
		if(*ptape) outSEF("#tapemethod %s\r\n", ptape);
		if(pEXP->flags&EXP_USE_ALLSEF) {
			outSEF_s("#fcc "); outSEF_F(u->incA/U_DEGREES, 2);
			outSEF_s("#bcc "); outSEF_F(u->incAB/U_DEGREES, 2);
			outSEF_s("#fic "); outSEF_F(u->incV/U_DEGREES, 2);
			outSEF_s("#bic "); outSEF_F(u->incVB/U_DEGREES, 2);
			outSEF_s("#mtc "); outSEF_F(u->incD*ct_scale, 2);
		}
	}

	outSEF_s("#data type,from,to,dist,fazi,");
	if(bConvert) outSEF_s("finc");
	else {
		if(pEXP->flags&EXP_USE_VDIST) {
			outSEF_s("vdist,bazi");
		}
		else {
			outSEF_s("finc,bazi,binc,iab,tab");
		}
	}
	outSEF_s(",left,right,ceil,floor\r\n");
}

static int Out_CT_Vector(void)
{
	int e;

	if(!ctvectors || compare_uCT()) {
		if(ctvectors) {
			assert(!cpvectors);
			outSEF_s("#endctsurvey\r\n\r\n");
			ctvectors=0;
		}
		else if(cpvectors) {
			outSEF_s("#endcpoint\r\n\r\n");
			cpvectors=0;
		}
		Out_CT_Header();
		bLast_NoInc=bNoInc;
		memcpy(&last_u, u, sizeof(last_u));
		memcpy(&last_date,&pVec->date,3);
	}
	else outSEF_CC();

	if(*line_comment) outSEF("%s\r\n",line_comment);

	char nam1[EXP_MAX_NAMLEN+2], nam2[EXP_MAX_NAMLEN+2];

	if(!(e=get_SEF_name(nam1, 0)))
	    e=get_SEF_name(nam2, 1);

	if(!e) {
		if(bConvert) {
			convert_vec(); //overwrite existing values in vec
		}
		outSEF(" %s,%s,%s,",ct_type, nam1, nam2);
		outSEF_FD(vec.d,1);
		outSEF_FA(vec.fazi,1);
		if(bConvert) outSEF_FA(vec.finc,ct_islrud);
		else {
			BOOL bComma;
			if(pEXP->flags&EXP_USE_VDIST) {
				outSEF_FD(vec.iab-vec.tab, bComma = ct_islrud || vec.bazi!=-9999);
				if(bComma) outSEF_FA(vec.bazi, ct_islrud);
			}
			else {
				outSEF_FA(vec.finc, bComma = ct_islrud || vec.bazi!=-9999 || vec.binc!=-9999 || vec.iab || vec.tab);
				if(bComma) outSEF_FA(vec.bazi, bComma = ct_islrud || vec.binc!=-9999 || vec.iab || vec.tab);
				if(bComma) outSEF_FA(vec.binc, bComma = ct_islrud || vec.iab || vec.tab);
				if(bComma) {
					outSEF_FD(vec.iab,1);
					outSEF_FD(vec.tab,ct_islrud);
				}
			}
		}
		if(ct_islrud) {
			for(int i=0; i<4; i++) {
				outSEF_FD(ct_lrud[i],i<3);
			}
		}
		outSEF_ln();
		ctvectors++;
	}
	return e;
}

static int Out_CP_Vector(void)
{
	int e;

	if(!cpvectors || u->bFeet!=last_u.bFeet || strcmp(last_seg, seg)) {

		if(cpvectors) {
			assert(!ctvectors);
			outSEF_s("#endcpoint\r\n");
			cpvectors=0;
		}
		else if(ctvectors) {
			outSEF_s("#endctsurvey\r\n");
			ctvectors=0;
		}

		memcpy(&last_u, u, sizeof(last_u));
		strcpy(last_seg, seg);

		outSEF_group("cpoint");
		outSEF_CC();


		if(u->bFeet && !pEXP->zone) {
			ct_units="feet";
			ct_scale=1.0/U_FEET;
		}
		else {
			ct_units="meters";
			ct_scale=1.0;
		}
  		if(pEXP->zone) {
			outSEF("#units utm\r\n");
			outSEF("#zone %u\r\n", pEXP->zone);
			outSEF("datum %s, grid convergence %.3f\r\n",pEXP->pdatumname,pEXP->grid);
		}
		else {
			outSEF("#units %s\r\n",ct_units);
		}
		outSEF("#elevunits %s\r\n",ct_units);
		outSEF_s("#data station,east,north,elev\r\n");
	}
	else outSEF_CC();

	if(*NoteBuf) outSEF("/%s --\r\n",NoteBuf);
	if(*line_comment) outSEF("%s --\r\n", line_comment);

	char  name[32];	/* Full station name*/
	e=get_SEF_name(name, 1);
	if(!e) {
		outSEF(" %s,", name);
		outSEF_F(ct_scale*pVec->xyz[0], 1);
		outSEF_F(ct_scale*pVec->xyz[1], 1);
		outSEF_F(ct_scale*pVec->xyz[2], 0);
		outSEF_ln();
		cpvectors++;
	}
	return e;
}

DLLExportC int PASCAL Export(EXPTYP *pexp)
{
	int e=0;

	pEXP=pexp;
	pexp->lineno=0;

	switch(pEXP->fcn) {
	case EXP_OPEN:
		if(pEXP->flags!=EXP_VERSION) e=SRV_ERR_VERSION;
		else {
			pEXP->numgroups=pEXP->numvectors=pEXP->numfixedpts=pEXP->maxnamlen=0;
			pEXP->fTotalLength=0.0;
			srv_CB=(LPSTR_CB)pEXP->title;
			//Initialize SEF file --
			if(!(fpout=fopen(strcpy(outpath, pEXP->path), "wb"))) e=EXP_ERR_INITSEF;
			else {
				exp_InitDefaults(); //returns 0
				outSEF(";SEF Created by Walls v2.0 - %s\r\n", timestr());
			}
		}
		break;

	case EXP_SETOPTIONS:
		if(!pEXP->title) exp_InitDefaults();
		else if((e=exp_SetOptions(pEXP->title))) {
			exp_Close();
		}
		memset(&last_u,0,sizeof(last_u));
		break;

	case EXP_SRVOPEN:
		//exp_Open() should be called before exp_SetOptions()!
		e=exp_Open();
		break;

	case EXP_SURVEY:
		ct_islrud=FALSE;

		while(!(e=srv_Next())) {
			if(pVec->flag&NET_VEC_FIXED) {
				if(e=Out_CP_Vector()) break;
				pEXP->numfixedpts++;
			}
			else {
				if(e=Out_CT_Vector()) break;
				pEXP->fTotalLength+=pVec->len_ttl;
				pEXP->numvectors++;
			}
			ct_islrud=FALSE;
		}
		if(e==CFG_ERR_EOF) e=0;

		if(!e) {
			if(ctvectors) outSEF_s("#endctsurvey\r\n");
			else if(cpvectors) outSEF_s("#endcpoint\r\n");
		}

		exp_Close();
		break;

	case EXP_CLOSE:
		//Close SEF file -- delete if pEXP->charno!=0 --
		if(fpout) {
			if(fclose(fpout)) e=EXP_ERR_CLOSE;
			if(pEXP->charno) _unlink(outpath);
			fpout=NULL;
		}
		break;

	case EXP_OPENDIR:
		outSEF("#dir %s\r\n", pEXP->title);
		break;
	case EXP_CLOSEDIR:
		outSEF_s("#up\r\n");
		break;
	}

	if(e>0) {
		pEXP->title=errmsg[e-1];
		pEXP->lineno=pVec->lineno;
		pEXP->charno=pVec->charno;
	}
	return e;
}

/*WALLISEF.CPP -- Import SEF file*/
#include "stdafx.h"
#include "wallisef.h"

typedef int (FAR PASCAL *LPFN_IMPORT_CB)(int fcn, LPVOID param);

enum { SEF_ALLOWCOLONS=1, SEF_COMBINE=2, SEF_FLG_CHRS_PFX=4, SEF_FLG_CHRS_REPL=8, SEF_ATTACHSRC=16};

enum { SEF_GETPARAMS };

static PSEF_PARAMS psef_params;

#define SEF_CB_INCREMENT 100 
#define SEF_MAXDEC 6
#define SIZLN 256
#define SIZBF 512
#define PI 3.141592653589793
#define BLANK_VAL 0.000000101
#define U_DEGREES (PI/180.0)

static double lentotal;
static UINT linecount,filecount,veccount,fixcount,dircount,dirnest,surveycount,datacount,nCautions;
static UINT repflags; // nchrs_repl, nchrs_pfx;
static int ln_len,bookidx,srvidx;
static bool bCombine,bExcluding,bLongNames,bDateFirst,bDeclSeen,bDateSeen,bAttachSrc;
static bool bAllowColons;
static char cUnits;
static LPCSTR pchrs_torepl;
static LPCSTR pchrs_repl;
static bool bUsingUTM,bUTM_used;

static CString errMsg;

static FILE *fp,*fpto,*fpts;
static  char ln[SIZLN],bf[SIZBF],person[SIZLN],date[20];

static char prjpath[_MAX_PATH];
static char sefpath[_MAX_PATH];
static char prjname[80];
static char bookpfx[8],bookname[16];
static char timestr[32];
static char *sefname;
static char *srvtail;
static char *srvname;

static LPSTR seftypes[]={
   "ATC",
   "ATDEC",
   "ATID",
   "ATUNITS",
   "BCC",
   "BCDEC",
   "BCID",
   "BCUNITS",
   "BIC",
   "BIDEC",
   "BIID",
   "BIUNITS",
   "CLINERR",
   "COM",
   "COMPBS",
   "COMPERR",
   "CPOINT",
   "CTSURVEY",
   "DATA",
   "DATE",
   "DEC",
   "DECL",
   "DIR",
   "DUTY",
   "ELEVDEC",
   "ELEVUNITS",
   "ENDCPOINT",
   "ENDCTSURVEY",
   "FCC",
   "FCDEC",
   "FCID",
   "FCUNITS",
   "FIC",
   "FIDEC",
   "FIID",
   "FIUNITS",
   "INCBS",
   "INCLUDE",
   "MTC",
   "MTDEC",
   "MTID",
   "MTUNITS",
   "PERSON",
   "ROOT",
   "TAPEMETHOD",
   "UNITS",
   "UP",
   "ZONE"
   };

enum {
  SEF_ATC=1,
  SEF_ATDEC,
  SEF_ATID,
  SEF_ATUNITS,
  SEF_BCC,
  SEF_BCDEC,
  SEF_BCID,
  SEF_BCUNITS,
  SEF_BIC,
  SEF_BIDEC,
  SEF_BIID,
  SEF_BIUNITS,
  SEF_CLINERR,
  SEF_COM,
  SEF_COMPBS,
  SEF_COMPERR,
  SEF_CPOINT,
  SEF_CTSURVEY,
  SEF_DATA,
  SEF_DATE,
  SEF_DEC,
  SEF_DECL,
  SEF_DIR,
  SEF_DUTY,
  SEF_ELEVDEC,
  SEF_ELEVUNITS,
  SEF_ENDCPOINT,
  SEF_ENDCTSURVEY,
  SEF_FCC,
  SEF_FCDEC,
  SEF_FCID,
  SEF_FCUNITS,
  SEF_FIC,
  SEF_FIDEC,
  SEF_FIID,
  SEF_FIUNITS,
  SEF_INCBS,
  SEF_INCLUDE,
  SEF_MTC,
  SEF_MTDEC,
  SEF_MTID,
  SEF_MTUNITS,
  SEF_PERSON,
  SEF_ROOT,
  SEF_TAPEMETHOD,
  SEF_UNITS,
  SEF_UP,
  SEF_ZONE
};

#define SEF_NUMTYPES (sizeof(seftypes)/sizeof(LPSTR))

#define SIZ_CC 5
enum e_cc {CC_A,CC_AB,CC_V,CC_VB,CC_D};
static LPSTR sefd_ccnam[SIZ_CC]={
  "IncA",
  "IncAB",
  "IncV",
  "IncVB",
  "IncD"
};

static double sefd_ccval[SIZ_CC];
static BOOL sefd_ccunits[SIZ_CC];

#define SIZ_IDBUF 256
#define NUM_ID 5
static char sefd_idbuf[SIZ_IDBUF+2];
static int sefd_id[NUM_ID]={SEF_FCID,SEF_BCID,SEF_FIID,SEF_BIID,SEF_MTID};
static LPSTR sefd_idnam[NUM_ID]={
  "A",
  "AB",
  "V",
  "VB",
  "D"
};

static LPSTR sefdtypes[]={
  "bazi",
  "binc",
  "ceil",
  "dist",
  "east",
  "elev",
  "fazi",
  "finc",
  "floor",
  "from",
  "iab",
  "left",
  "north",
  "right",
  "station",
  "tab",
  "to",
  "type",
  "vdist"
};

enum {
  SEFD_BAZI=0,
  SEFD_BINC,
  SEFD_CEIL,
  SEFD_DIST,
  SEFD_EAST,
  SEFD_ELEV,
  SEFD_FAZI,
  SEFD_FINC,
  SEFD_FLOOR,
  SEFD_FROM,
  SEFD_IAB,
  SEFD_LEFT,
  SEFD_NORTH,
  SEFD_RIGHT,
  SEFD_STATION,
  SEFD_TAB,
  SEFD_TO,
  SEFD_TYPE,
  SEFD_VDIST
};

#define SEFD_NUMTYPES (sizeof(sefdtypes)/sizeof(LPSTR))

static int sefd_numtypes;

static int sefd_ccid[SIZ_CC]={
  SEFD_FAZI, //"IncA",
  SEFD_BAZI, //"IncAB",
  SEFD_FINC, //"IncV",
  SEFD_BINC, //"IncVB",
  SEFD_DIST  //"IncD"
};

#define SEFD_SIZBF 40
#define SEFD_SIZUNITS 8

static int sefd_off[SEFD_NUMTYPES];
static int sefd_len[SEFD_NUMTYPES];
static double sefd_val[SEFD_NUMTYPES];
static char sefd_from[SEFD_SIZBF],sefd_to[SEFD_SIZBF];
static char sefd_type[SEFD_SIZBF];
static char sefd_buf[SEFD_SIZBF];

static int sefd_binc,sefd_bazi,sefd_ord;

enum {SEFD_DAV,SEFD_DVA,SEFD_ADV,SEFD_VDA,SEFD_AVD,SEFD_VAD,SEF_DA,SEF_AD};
static LPCSTR sefd_ordstr[8]={"DAV","DVA","ADV","VDA","AVD","VAD","DA","AD"};
static LPCSTR sefd_colnam[8]={
	"DIST\tAZ\tINCL","DIST\tINCL\tAZ","AZ\tDIST\tINCL","INCL\tDIST\tAZ",
	"AZ\tVERT\tDIST","VERT\tAZ\tDIST","DIST\tAZ","AZ\tDIST"
};
static int sefd_ordidx[8][3]={
  {SEFD_DIST,SEFD_FAZI,SEFD_FINC},
  {SEFD_DIST,SEFD_FINC,SEFD_FAZI},
  {SEFD_FAZI,SEFD_DIST,SEFD_FINC},
  {SEFD_FINC,SEFD_DIST,SEFD_FAZI},
  {SEFD_FAZI,SEFD_FINC,SEFD_DIST},
  {SEFD_FINC,SEFD_FAZI,SEFD_DIST},
  {SEFD_DIST,SEFD_FAZI,0},
  {SEFD_FAZI,SEFD_DIST,0}};

enum {SEFD_IT,SEFD_SS,SEFD_IS,SEFD_ST};
static LPSTR seftmeth[4]={"IT","SS","IS","ST"};

static int sefd_lrud[4]={SEFD_LEFT,SEFD_RIGHT,SEFD_CEIL,SEFD_FLOOR};

static char sefd_units[SEFD_SIZUNITS],sefd_atunits[SEFD_SIZUNITS];
static char sefd_cpunits[SEFD_SIZBF];
static double sefd_decl,sefd_atc,sefd_errAB,sefd_errVB;

static int sefd_atdec,sefd_mtdec,sefd_fcdec,sefd_fidec,sefd_lendav;
static int sefd_elevdec,sefd_cpdec,sefd_elevunits;
static int * sefd_ccdec[SIZ_CC]={&sefd_fcdec,&sefd_fcdec,&sefd_fidec,&sefd_fidec,&sefd_mtdec};

static int sefd_tmeth,sefd_htcorrect,sefd_uselrud;
static bool bHdrLine,bCPoint,bParen,sefd_revAB,sefd_revVB;
static bool lastblank;
static int *sefd_idx;
static int sefd_dec[3];

static char *pDataCP="type(2,4)station(6,12)east(18,16)north(34,16)elev(50,10)";
static char *pDataCT="type(2,4)from(6,12)to(18,12)dist(30,10)fazi(40,10)bazi(50,10)finc(60,10)binc(70,10)"
					 "left(80,10)right(90,10)ceil(100,10)floor(110,10)iab(120,10)tab(130,10)vdist(140,10)";

static LPSTR prjtypes[]={
   "BOOK",
   "ENDBOOK",
   "NAME",
   "STATUS",
   "SURVEY"
};

enum {
  PRJ_BOOK=1,
  PRJ_ENDBOOK,
  PRJ_NAME,
  PRJ_STATUS,
  PRJ_SURVEY
};

#define PRJ_NUMTYPES (sizeof(prjtypes)/sizeof(LPSTR))


enum {
	SEF_ERR_ABORTED=1,
	SEF_ERR_SEFOPEN,
	SEF_ERR_WRITE,
	SEF_ERR_CREATE,
	SEF_ERR_SIZLN,
	SEF_ERR_LINBLANK,
	SEF_ERR_NOARG,
	SEF_ERR_BADSEQ,
	SEF_ERR_DATAKEY,
	SEF_ERR_DATAPAREN,
	SEF_ERR_DATAARGS,
	SEF_ERR_TAPEMETHOD,
	SEF_ERR_MAXDEC,
	SEF_ERR_ROOT,
	SEF_ERR_INCLUDE,
	SEF_ERR_LATLONG,
	SEF_ERR_BADNAME,
	SEF_ERR_CODELIMIT,
	SEF_ERR_VERSION=999
};

static LPSTR errstr[]={
  "User abort",
  "Cannot open file",
  "Error writing file",
  "Cannot create file",

  "Line too long",
  "Blank line encountered",
  "Argument expected",
  "SEF line type out of sequence",
  "Unrecognized keyword in #data statement",
  "Bad (column,length) specifier",
  "Unsupported #data statement",
  "Unsupported #tapemethod option",
  "Decimals must be between 1 and 6",
  "#root disallowed. SEF must have \"relative\" paths",
  "#include directive not supported",
  "#units geog (Lat/Long) not supported",
  "Station name(s) expected"
};

static apfcn_v trimeb(char *p)
{
  char *p0;
  if(*p==' ') {
    for(p0=p;*p0==' ';p0++);
    strcpy(p,p0);
  }
  p0=p+strlen(p);
  while(p0>p && *(p0-1)==' ') p0--;
  *p0=0;
  
}

static apfcn_v trimzeros(char *p)
{
	char *p0;
	
	if(p0=strchr(p,'.')) {
	  p0+=strlen(p0);
	  while(*(p0-1)=='0') p0--;
	  if(*(p0-1)=='.') {
	    p0--;
	    if(p0-p==1 && (*p=='-' || *p=='+')) p0--;
	    if(p0==p) *p0++='0';
	  }
	  *p0=0;
	}
}

static char * fltstr(double f,int dec)
{
	//Obtains the minimal width numeric string of f rounded to dec decimal places.
	//Any trailing zeroes (including trailing decimal point) are truncated.
	static char buf[40];
	char *p;

    if(f==BLANK_VAL || _snprintf(buf,20,"%-0.*f",dec,f)==-1 ||
	 !(p=strchr(buf,'.'))) return strcpy(buf,"--");

	for(p+=dec;dec && *p=='0';dec--) *p--=0;
	if(*p=='.') {
	   *p=0;
	   if(!strcmp(buf,"-0")) strcpy(buf,"0");
	}
	return buf;
}

static LPSTR GetRelativePath(LPSTR buf, LPCSTR pPath, LPCSTR pRefPath, BOOL bNoNameOnPath)
{
	//Note: buf has assumed size >= MAX_PATH and pPath==buf is allowed.
	//Last arg should be FALSE if pPath contains a file name, in which case it will
	//be appended to the relative path in buf.

	int lenRefPath=trx_Stpnam(pRefPath)-pRefPath;
	ASSERT(lenRefPath>=3);

	LPSTR pDupPath=(pPath==buf)?_strdup(pPath):NULL;
	if(pDupPath)
		pPath=pDupPath;
	else if(buf!=pPath)
		strcpy(buf, pPath);

	if(bNoNameOnPath) strcat(buf, "\\");
	else *trx_Stpnam(buf)=0;

	int lenFilePath=strlen(buf);
	int maxLenCommon=min(lenFilePath, lenRefPath);
	for(int i=0; i<=maxLenCommon; i++) {
		if(i==maxLenCommon || toupper(buf[i])!=toupper(pRefPath[i])) {
			if(!i) break;

			while(i && buf[i-1]!='\\') i--;
			ASSERT(i);
			if(i<3) break;
			lenFilePath-=i;
			pPath+=i;
			LPSTR p=(LPSTR)pRefPath+i;
			for(i=0; p=strchr(p, '\\'); i++, p++); //i=number of folder level prefixes needed
			if(i*3+lenFilePath>=MAX_PATH) {
				//at least don't overrun buf
				break;
			}
			for(p=buf; i; i--) {
				strcpy(p, "..\\"); p+=3;
			}
			//pPath=remaining portion of path that's different
			//pPath[lenFilePath-1] is either '\' or 0 (if bNoNameOnPath)
			if(lenFilePath) {
				memcpy(p, pPath, lenFilePath);
				if(bNoNameOnPath) p[lenFilePath-1]='\\';
			}
			p[lenFilePath]=0;
			break;
		}
	}
	if(!bNoNameOnPath) {
		LPCSTR p=trx_Stpnam(pPath);
		if((lenFilePath=strlen(buf))+strlen(p)<MAX_PATH)
			strcpy(buf+lenFilePath, p);
	}
	if(pDupPath) free(pDupPath);
	return buf;
}

static int PASCAL comp_sefd(int i)
{
  return strcmp(sefd_buf,(LPSTR)i);
}

static apfcn_i errexit(int e)
{
  if(fpto) fclose(fpto);
  if(fpts) fclose(fpts);
  if(fp) fclose(fp);
  return e;
}

DLLExportC int PASCAL ErrMsg(LPCSTR *pMsg,int code)
{
	int len=0;

	if(!code) {
		errMsg.Format("Conversion of %s to %s --\n\nFiles: %u  Surveys: %u  Vectors: %u  Fixed pts: %u\n"
				"Total taped vector length: %.2f m (%.2f ft)",
			trx_Stpnam(sefpath), prjname,
			filecount, surveycount, veccount, fixcount, lentotal,lentotal/0.3048);

		if(repflags) {
			CString sf("\n\nSome replacements were required in station names");
			char cpfx=':';
			for(LPCSTR p=pchrs_repl; *p; p++) {
				if(repflags&(1<<(p-pchrs_repl))) {
					sf.AppendFormat("%c '%c' for '%c'", cpfx, *p, pchrs_torepl[p-pchrs_repl]);
					cpfx=',';
				}
			}
			errMsg+=sf;
		}
		if(bLongNames) {
			errMsg+="\n\nSome names of length >8 chars were converted to prefixed names.";
		}
		else if(!repflags) errMsg+="\n\nNo station name modifications were required.";
		if(nCautions) {
			errMsg.AppendFormat("\n%d messages prefixed with \"***Caution\" appear in the data files.",nCautions);
		}

		if(bUTM_used) {
			errMsg+="\n\nNOTE: Since control points with UTM coordinates were encountered,\n"
				    "you'll next need to assign the appropriate geographical reference\n"
					"and enable generation of grid-relative coordinates.";
		}

		*pMsg=(LPCSTR)errMsg;
		return 0;
	}

	if(code<=SEF_ERR_CREATE) {
		if(code==SEF_ERR_ABORTED) strcpy(ln,errstr[code-1]);
		else wsprintf(ln,"%s %s",(LPSTR)errstr[code-1],
			(LPSTR)((code==SEF_ERR_SEFOPEN)?sefpath:prjpath));
	}
	else if(code>=SEF_ERR_CODELIMIT) {
		wsprintf(ln,"Version %.1f (not %.1f) of wallsef.dll required",psef_params->version,DLL_VERSION);
	}
	else {
		len=wsprintf(ln,"Line %u: ",linecount);
		strcpy(ln+len,errstr[code-1]);
	}
	AfxMessageBox(ln);
	return linecount;
}

static apfcn_i srv_close(void)
{
  int e=0;
  if(fpts && fclose(fpts)) e=SEF_ERR_WRITE;
  fpts=NULL;
  return e;
}

static apfcn_i line_fcn(void)
{
  int len;
  if(NULL==fgets(ln,SIZLN,fp)) return CFG_ERR_EOF;
  len=strlen(ln);
  while(len && isspace((BYTE)ln[len-1])) ln[--len]=0;
  ln_len=len;
  return (len<SIZLN-1)?len:CFG_ERR_TRUNCATE;
}

static apfcn_i write_prj(char *key,char *tail)
{
  if(!bHdrLine) {
	fputs(";WALLS Project File\n", fpto);
  }

  int len=wsprintf(bf,".%s\n",(LPSTR)key);
  
  if(tail && *tail) wsprintf(bf+len-1,"\t%s\n",(LPSTR)tail);
  if(fputs(bf,fpto)==EOF) return SEF_ERR_WRITE;
  return 0;
}

static apfcn_i process_comment(char *p)
{
  if(*p=='*' || *p==';') p++;
  if(fpts) {
	  if(*p || !lastblank) {
		  if(!*p || p!=strstr(p,"FROM\tTO\t")) {
			  if(lastblank=(*p==0)) strcpy(bf,"\n");
			  else {
				  wsprintf(bf,";%s\n", (LPSTR)p);
			  }
			  if(fputs(bf, fpts)==EOF) return SEF_ERR_WRITE;
		  }
	  }
  }
  return 0;
}

static apfcn_i process_date(char *p)
{
  static char year[6];
  
  /*Convert date to International Standard ISO 8601 --*/
  if(fpts) {
    if(strlen(p)==10 && (p[2]=='-' || p[2]=='/') && (p[5]=='-' || p[5]=='/')) {
      memcpy(year,p+6,4);
      p[5]=0; p[2]='-'; 
      wsprintf(date,"#Date %s-%s\n",(LPSTR)year,(LPSTR)p);
	  if(bCPoint && fputs(date,fpts)==EOF) return SEF_ERR_WRITE; 
    }
    else {
	  wsprintf(bf,";#date %s\n",(LPSTR)p);
      if(fputs(bf,fpts)==EOF) return SEF_ERR_WRITE;
	}
  }
  //lastblank=FALSE;
  return 0;
}

static apfcn_i process_duty(char *p)
{
  if(*p=='*') p++;
  if(fpts && *person) {
    wsprintf(bf,";%s - Duty: %s\n",(LPSTR)person,(LPSTR)p);
    if(fputs(bf,fpts)==EOF) return SEF_ERR_WRITE;
  }
  //lastblank=FALSE;
  return 0;
}

static apfcn_i process_id(int typ,char *p)
{
  int i,len;
  
  if(!*p) return 0;
  for(i=0;i<NUM_ID;i++) if(sefd_id[i]==typ) break;
  if(i==NUM_ID) return 0;
  if(!sefd_idbuf[0]) len=wsprintf(sefd_idbuf,";Instruments -\n");
  else len=strlen(sefd_idbuf);
  if(len+strlen(p)+2+strlen(sefd_idnam[i])<=SIZ_IDBUF)
   wsprintf(sefd_idbuf+len-1," %s:%s\n",(LPSTR)sefd_idnam[i],(LPSTR)p);
  return 0;
}

static apfcn_i end_exclude(void)
{
    bExcluding=FALSE;
	if(EOF==fputs("#] (Comment end)\n",fpts)) return SEF_ERR_WRITE;
	return 0;
}

static apfcn_i out_attached_sef()
{
	int e;
	char buf[_MAX_PATH];
	LPSTR p=GetRelativePath(buf,sefpath,prjpath, 0); //0 == there's a name on makpath
	p=trx_Stpnam(buf);

	e=write_prj("SURVEY",p);
	if(!e) e=write_prj("NAME",p);
	if(!e) {
		if(p>buf && p[-1]=='\\') p--;
		*p=0;
		if(*buf) {
			e=write_prj("PATH",buf);
		}
		if(!e) e=write_prj("STATUS", "196618");
	}
	return e;
}

static apfcn_i new_dir(char *title)
{
  int e;
  
  if(bHdrLine) {
	  sprintf(bookname, "%s#%03d", bookpfx, ++bookidx);
//	  wsprintf(srvtail, "%03uD", dircount++);
   }

  if(!(e=write_prj("BOOK",title)) &&
     !(e=write_prj("NAME", bHdrLine?bookname:"PROJECT")) &&
	 (dirnest || !(e=write_prj("OPTIONS","flag=Fixed"))) &&
     !(e=write_prj("STATUS",dirnest?"8":"3")))
  {
     dirnest++;
  }

  if(!e && !bHdrLine) {
	  bHdrLine=true;
	  if(bAttachSrc) {
		 e=out_attached_sef();
	  }
  }

  srvidx=0;
  return e;
}

static apfcn_i up_dir(void)
{
  int e=0;
  if(dirnest) {
    e=write_prj("ENDBOOK",0);
    dirnest--;
  }
  return e;
}

static char *get_filetime(LPSTR pBuf, LPCSTR path)
{
	WIN32_FILE_ATTRIBUTE_DATA fdata;
	SYSTEMTIME stLocal;
	memset(&stLocal, 0, sizeof(SYSTEMTIME));

	if(GetFileAttributesEx(path, GetFileExInfoStandard, &fdata)) {
		// Convert the last-write time to local time.
		SYSTEMTIME stUTC;
		if(FileTimeToSystemTime(&fdata.ftLastWriteTime, &stUTC))
			SystemTimeToTzSpecificLocalTime(NULL, &stUTC, &stLocal);
    }

	char c=(stLocal.wHour>=12)?'P':'A';
	if(stLocal.wHour>12) {
		stLocal.wHour-=12;
	}
	else if(stLocal.wHour==0) {
		stLocal.wHour=12;
	}

	sprintf(pBuf, "%d-%02d-%02d %02d:%02d %cM",
		stLocal.wYear, stLocal.wMonth, stLocal.wDay,
		stLocal.wHour, stLocal.wMinute, c);
	return pBuf;
}

static apfcn_i new_survey(char *title)
{
  int e;
  
  if(fpts && !bCombine) return SEF_ERR_BADSEQ;
  datacount=0;

  if(!dirnest && (e=new_dir(sefname))) return e; //Rare case of individual survey export
  
  if(!fpts) {
	  if(write_prj("SURVEY",title)) return SEF_ERR_WRITE;
	  wsprintf(srvtail,"%03u#%04u",bookidx,srvidx++);
	  if(write_prj("NAME",srvtail) || write_prj("STATUS","8")) return SEF_ERR_WRITE;
	  strcpy(trx_Stpext(srvtail),".SRV");
	  if((fpts=fopen(prjpath,"w"))==NULL) return SEF_ERR_CREATE;
	  filecount++;
  }
  else if(fputs("\n",fpts)==EOF) return SEF_ERR_WRITE;
  
  wsprintf(bf,";%s\n;Import from %s, dated %s\n",
	    (LPSTR)title,(LPSTR)sefname,(LPSTR)timestr);
	    
  if(fputs(bf,fpts)==EOF) return SEF_ERR_WRITE;
  //lastblank=TRUE;

  /*Set defaults --*/
  *date=0;
  *sefd_idbuf=0;
  sefd_revAB=sefd_revVB=bExcluding=FALSE;
  *sefd_cpunits=0;
  cUnits='F';
  strcpy(sefd_atunits,strcpy(sefd_units,"Feet"));
  sefd_elevunits='F';
  sefd_decl=sefd_atc=0.0;
  sefd_errAB=sefd_errVB=0.0;
  sefd_mtdec=sefd_fcdec=sefd_fidec=sefd_elevdec=sefd_cpdec=sefd_atdec=2;
  sefd_tmeth=SEFD_SS; /*default is "ss"*/
  memset(sefd_ccval,0,sizeof(sefd_ccval));
  memset(sefd_ccunits,0,sizeof(sefd_ccunits));
  return 0;
}

static apfcn_v append_cc(char *bp)
{
  int i;
  char units[2];
  units[1]=0;
  for(i=0;i<SIZ_CC;i++) if(sefd_off[sefd_ccid[i]]) {
	 if(bCombine || sefd_ccval[i]) {
		 bp+=strlen(bp)-1;
		 if(i==CC_D || sefd_ccval[i]==0.0) *units=0;
		 else *units='d';
		 wsprintf(bp," %s=%s%s\n",(LPSTR)sefd_ccnam[i],fltstr(sefd_ccval[i],*sefd_ccdec[i]),(LPSTR)units);
	 }
  }
}  

static apfcn_i process_data(char *p)
{
  char *pnxt;
  int ikey,inc_off,az_off;
  BOOL bNext;
  
  if(!fpts) return 0;

  datacount=0;
  memset(sefd_off,0,sizeof(sefd_off));
  
  bParen=strchr(p,'(')!=NULL;

  while(*p) {
    datacount++;

    /*At this point p points to the next keyword --*/
    if(bParen) pnxt=strchr(p,'(');
    else {
      pnxt=strchr(p,' ');
      if(!pnxt) pnxt=strchr(p,'\t');
      if(!pnxt) pnxt=strchr(p,',');
      if(!pnxt) {
		pnxt=p+strlen(p);
        bNext=FALSE;
      }
      else bNext=TRUE;
    }
    if(pnxt-p>SEFD_SIZBF-1) return SEF_ERR_DATAPAREN;
    *pnxt=0;
    
    trimeb(strcpy(sefd_buf,p));
    
    ikey=(int)trx_Blookup(TRX_DUP_NONE);
    if(!ikey) return SEF_ERR_DATAKEY;
    ikey=(LPSTR *)ikey-sefdtypes;
    
    if(bParen) {
	    if(!(pnxt=strchr(p=pnxt+1,','))) return SEF_ERR_DATAPAREN;
	    *pnxt=0;
	    if((sefd_off[ikey]=atoi(p)-1)<=0) return SEF_ERR_DATAPAREN;
	
	    if(!(pnxt=strchr(p=pnxt+1,')'))) return SEF_ERR_DATAPAREN;
	    *pnxt=0;
	    if((sefd_len[ikey]=atoi(p))<=0) return SEF_ERR_DATAPAREN;
	    if((UINT)sefd_len[ikey]>SEFD_SIZBF-1 ||  sefd_off[ikey]+sefd_len[ikey]>SIZLN) {
			return SEF_ERR_DATAARGS;
		}
	}
	else {
	  sefd_off[ikey]=datacount;
	  sefd_len[ikey]=-1;
	  if(!bNext) break;
	}
    p=pnxt+1;
  }
  
  if(bCPoint) {
	  if(!sefd_off[SEFD_STATION] || !sefd_off[SEFD_EAST] ||
	     !sefd_off[SEFD_NORTH] || !sefd_off[SEFD_ELEV])
	    return SEF_ERR_DATAARGS;
  }
  else {
	  if(!sefd_off[SEFD_FROM] || !sefd_off[SEFD_TO] || !sefd_off[SEFD_DIST])
	    return SEF_ERR_DATAARGS;
	    
	  sefd_uselrud=sefd_off[SEFD_LEFT]!=0 || sefd_off[SEFD_RIGHT]!=0
	    || sefd_off[SEFD_CEIL]!=0 || sefd_off[SEFD_FLOOR]!=0;
	
	  sefd_htcorrect=sefd_off[SEFD_VDIST]!=0;
	  if(sefd_off[SEFD_IAB]) sefd_htcorrect|=2;
	  if(sefd_off[SEFD_TAB]) sefd_htcorrect|=4;

	  if(sefd_htcorrect&1 && (sefd_htcorrect&6))
		  return SEF_ERR_DATAARGS;  //either vdist or iab/tab, not both types
	
	  if((az_off=sefd_off[SEFD_FAZI])==0) {
		  if((az_off=sefd_off[SEFD_BAZI])==0)
				return SEF_ERR_DATAARGS; //azimuth requred
		  sefd_bazi=-1;
	  }
	  else sefd_bazi=sefd_off[SEFD_BAZI]!=0;
	
	  if((inc_off=sefd_off[SEFD_FINC])==0 && (inc_off=sefd_off[SEFD_BINC])!=0) {
			sefd_binc=-1;
	  }
	  else sefd_binc=sefd_off[SEFD_BINC]!=0;

	  sefd_lendav=3;
	  if(!inc_off) {
		  //assuming zero inclinations
		  if(az_off<sefd_off[SEFD_DIST]) sefd_ord=SEF_AD;
		  else sefd_ord=SEF_DA;
		  sefd_lendav=2;
	  }
	  else if(sefd_off[SEFD_DIST]<az_off) {
	    if(az_off<inc_off) sefd_ord=SEFD_DAV;
	    else if(sefd_off[SEFD_DIST]<inc_off) sefd_ord=SEFD_DVA;
	    else sefd_ord=SEFD_VDA;
	  }
	  else {
	    if(sefd_off[SEFD_DIST]<inc_off) sefd_ord=SEFD_ADV;
	    else if(az_off<inc_off) sefd_ord=SEFD_AVD;
	    else sefd_ord=SEFD_VAD;
	  }
  }
  
  /*Write units specifier in SRV file --*/
  //if(!lastblank) fputs("\n",fpts);
  //lastblank=FALSE;
  
  if(*sefd_idbuf && fputs(sefd_idbuf,fpts)==EOF) return SEF_ERR_WRITE;
  
  bExcluding=FALSE;
  
  if(bCPoint) {
	  wsprintf(bf,"#Units %s order=ENU",(LPSTR)sefd_units);
	  if(bUsingUTM) {
		  strcat(bf,(LPSTR)sefd_cpunits);
	  }
	  if(fputs(bf,fpts)==EOF || fputs("\n", fpts)==EOF) return SEF_ERR_WRITE;
  }
  else {
      //if(*sefd_units==*sefd_atunits) strcpy(sefd_buf,sefd_units);
      //else wsprintf(sefd_buf,"D=%s S=%s",(LPSTR)sefd_units,(LPSTR)sefd_atunits);

	  if(bDateFirst && *date && fputs(date, fpts)==EOF) return SEF_ERR_WRITE;

	  wsprintf(bf,"#Units %s Order=%s Decl=%s\n",
	     (LPSTR)sefd_units,(LPSTR)sefd_ordstr[sefd_ord],fltstr(sefd_decl,4));
	  
	  if(sefd_htcorrect)
	    wsprintf(bf+strlen(bf)-1," Tape=%s\n",(LPSTR)seftmeth[sefd_tmeth]);
	  
	  if(sefd_bazi && (sefd_errAB!=0.0 || sefd_revAB)) {
	    wsprintf(bf+strlen(bf)-1," typeAB=%c\n",sefd_revAB?'C':'N');
	    if(sefd_errAB!=0.0) wsprintf(bf+strlen(bf)-1,",%s\n",fltstr(sefd_errAB,1));
	  }
	     
	  if(sefd_binc && (sefd_errVB!=0.0 || sefd_revVB)) {
	    wsprintf(bf+strlen(bf)-1," typeVB=%c\n",sefd_revVB?'C':'N');
	    if(sefd_errVB!=0.0) wsprintf(bf+strlen(bf)-1,",%s\n",fltstr(sefd_errVB,1));
	  }
	     
	  append_cc(bf); /*append correction arguments*/
	     
	  if(fputs(bf,fpts)==EOF) return SEF_ERR_WRITE;
	  
	  for(ikey=0;ikey<4;ikey++) {
		  if(sefd_ccunits[ikey]&&sefd_ccval[ikey]!=0.0) break;
	  }
	  if(ikey<4) {
		  if(EOF==fputs(";***Caution: Units for angular corrections may be wrong. Degrees assumed.\n",fpts))
			  return SEF_ERR_WRITE;
		  nCautions++;
	  }
	
      if(!bDateFirst) {
		  if(*date && fputs(date, fpts)==EOF) return SEF_ERR_WRITE;
	  }
	  else if(*date && !sefd_decl) {
		  if(EOF==fputs(";***Caution: \"decl=0\" will override a declination derived from the preceding #date directive.\n", fpts))
			  return SEF_ERR_WRITE;
		  nCautions++;
	  }

	  sefd_idx=&sefd_ordidx[sefd_ord][0];
	
	  for(ikey=0;ikey<sefd_lendav;ikey++) {
	    switch(sefd_idx[ikey]) {
	      case SEFD_FINC : sefd_dec[ikey]=sefd_fidec; break;
	      case SEFD_DIST : sefd_dec[ikey]=sefd_mtdec; break;
	      case SEFD_FAZI : sefd_dec[ikey]=sefd_fcdec; break;
	    }
	  }

	  strcpy(bf,";FROM\tTO\t");
	  strcat(bf,sefd_colnam[sefd_ord]);
	  if(sefd_htcorrect) strcat(bf,(sefd_htcorrect>1)?"\tIAB\tTAB":"\tFRDEPTH-TODEPTH");
	  strcat(bf, "\n");
 	  if(EOF==fputs(bf, fpts)) return SEF_ERR_WRITE;
  }

  return 0;
}

static apfcn_i process_key(int typ)
{
  int retval=0;
  char *p=cfg_argc>1?cfg_argv[1]:"";

  switch(typ) {
    case SEF_CPOINT   : bUsingUTM=false;
    case SEF_CTSURVEY : bCPoint=(typ==SEF_CPOINT);
    				    retval=new_survey(p);
						bDeclSeen=bDateSeen=false;
						bDateFirst=false; //set to true if #date seen before #decl
						surveycount++;
                        break;

	case SEF_ENDCPOINT: bCPoint=FALSE;
						if(bUsingUTM) {
							bUTM_used=true;
							bUsingUTM=false;
						}
    case SEF_ENDCTSURVEY : if(!fpts) retval=SEF_ERR_BADSEQ;
                           else {
                             if(bExcluding && (retval=end_exclude())) break;
                             if(!bCombine) retval=srv_close();
                             datacount=0;
                           }
                           break;

    case SEF_DATA     : retval=process_data(p);
                        break;
                        
    case SEF_ZONE	  : if(bCPoint) {
		                  //Assume units are UTM
		                  _snprintf(sefd_cpunits,sizeof(sefd_cpunits)-1,"\t;UTM Zone %s",p);
						  bUsingUTM=true;
    					}
    					break;
    					
    case SEF_UNITS    : _strupr(p);
						if(!bCPoint) {
							cUnits=*p;
							break;
						}
 						if(*p=='G') {
							retval=SEF_ERR_LATLONG;
							break;
						}
    					if(*p!='F') { 
							if(*p=='U' && !bUsingUTM) {
								bUsingUTM=true;
								strcpy(sefd_cpunits,"\t;UTM");
							}
							strcpy(sefd_units, "Meters");
						}
                        break;
                          
    case SEF_MTUNITS  : if(toupper(*p)=='M') strcpy(sefd_units,"Meters");
                        break;
                        
    case SEF_ATUNITS  : if(toupper(*p)=='M') strcpy(sefd_atunits,"Meters");
                        break;
                        
    case SEF_MTC      : sefd_ccval[CC_D]=atof(p); /*Always in feet?*/
    					break;
    					
    case SEF_COMPERR  : sefd_errAB=atof(p); break;
    case SEF_CLINERR  : sefd_errVB=atof(p); break;
    
     case SEF_TAPEMETHOD :
						_strupr(p);
                        for(typ=0;typ<4;typ++)
                          if(!strcmp(seftmeth[typ],p)) {
                            sefd_tmeth=typ;
                            break;
                          }
                        if(typ==4) retval=SEF_ERR_TAPEMETHOD;
                        break;
                        
    case SEF_INCLUDE  : retval=SEF_ERR_INCLUDE;
    					break;

    case SEF_DECL     : sefd_decl=atof(p);
		                if(bDateSeen) bDateFirst=true;
						bDeclSeen=true;
                        break;

    case SEF_ATC     :  sefd_atc=atof(p);
                        break;

    case SEF_ATDEC    : if((sefd_atdec=atoi(p))>SEF_MAXDEC||sefd_atdec<0) retval=SEF_ERR_MAXDEC;
                        break;

    case SEF_MTDEC    : if((sefd_mtdec=atoi(p))>SEF_MAXDEC||sefd_mtdec<0) retval=SEF_ERR_MAXDEC;
                        break;

    case SEF_FCDEC    : if((sefd_fcdec=atoi(p))>SEF_MAXDEC||sefd_fcdec<0) retval=SEF_ERR_MAXDEC;
                        break;

    case SEF_FCC      : sefd_ccval[CC_A]=atof(p); /*Always in degrees?*/
    					break;

    case SEF_BCC      : sefd_ccval[CC_AB]=atof(p);
    					break;
    					
    case SEF_FCUNITS  : sefd_ccunits[CC_A]=TRUE; break;
    case SEF_BCUNITS  : sefd_ccunits[CC_AB]=TRUE; break;
    case SEF_FIUNITS  : sefd_ccunits[CC_V]=TRUE; break;
    case SEF_BIUNITS  : sefd_ccunits[CC_VB]=TRUE; break;

    case SEF_FIDEC    : if((sefd_fidec=atoi(p))>SEF_MAXDEC||sefd_fidec<0) retval=SEF_ERR_MAXDEC;
                        break;

    case SEF_FIC      : sefd_ccval[CC_V]=atof(p); /*Always in degrees?*/
    					break;

    case SEF_BIC      : sefd_ccval[CC_VB]=atof(p);
    					break;

    case SEF_DEC      : if((sefd_cpdec=atoi(p))>SEF_MAXDEC||sefd_cpdec<0) retval=SEF_ERR_MAXDEC;
                        break;
                        
    case SEF_ELEVDEC  : if((sefd_elevdec=atoi(p))>SEF_MAXDEC||sefd_elevdec<0) retval=SEF_ERR_MAXDEC;
                        break;
                        
    case SEF_ELEVUNITS: sefd_elevunits=toupper(*p);
    					if(sefd_elevunits!='M') sefd_elevunits='F';
    					break;
                        
    case SEF_DIR      : retval=new_dir(p);
    					if(bCombine && !retval) retval=srv_close();
                        break;
                        
    case SEF_ROOT     : while(dirnest && !(retval=up_dir()));
    					if(bCombine && !retval) retval=srv_close();
                        break;

    case SEF_UP       : retval=up_dir();
    					if(bCombine && !retval) retval=srv_close();
                        break;
                        
    case SEF_DATE      :retval=process_date(p);
		                if(bDeclSeen) bDateFirst=false;
		                bDateSeen=true;
                        break;

    case SEF_COM      : retval=process_comment(p);
                        break;
    case SEF_PERSON   : strcpy(person,p);
    					break;
    case SEF_DUTY     : retval=process_duty(p);
    					break;
    case SEF_MTID     :
    case SEF_FCID     :
    case SEF_BCID     :
    case SEF_FIID     :
    case SEF_BIID     : retval=process_id(typ,p);
    					break;
    					
    case SEF_COMPBS   : if(toupper(*p)=='C') sefd_revAB=TRUE; break;
    case SEF_INCBS    : if(toupper(*p)=='C') sefd_revVB=TRUE; break;
    
  }
  if(typ!=SEF_COM) lastblank=FALSE;
  return retval;
}

static void ElimSpc(char *p0)
{
    //Convert long names to prefixed names and
    //substitute for certain WALLS special characters.

	LPSTR p;
	LPCSTR pp;
	int i;

	for(p=p0; *p; p++) {
		pp=strchr(pchrs_torepl, *p);
		if(pp) {
			i=pp-pchrs_torepl;
			if(i==1 && bAllowColons) continue;
			*p=pchrs_repl[i];
			repflags|=(1<<i);
		}
	}
	i=p-p0;

	if(!bAllowColons && i>8) {
		bLongNames=true;
		p=p0+i;
		p[1]=0;
		for(i=0; i<8; i++, p--) *p=*(p-1);
		*p=':';
	}
}

static char * PASCAL StrFromField(char *buf,int typ)
{
  int off=sefd_off[typ];
  
  if(!off) {
    *buf=0;
    return buf;
  }
  
  if(!bParen) {
	if(off>cfg_argc) {
		*buf=0;
		return buf;
	}
    off--;
    if(strlen(cfg_argv[off])>SEFD_SIZBF-1) cfg_argv[off][SEFD_SIZBF-1]=0;
    strcpy(buf,cfg_argv[off]);
  }
  else {
	  if(off>=ln_len) typ=0;
	  else {
	    typ=sefd_len[typ];
	    if(off+typ>ln_len) typ=ln_len-off;
	    while(typ && ln[off]==' ') {
	      typ--;
	      off++;
	    }
	    if(typ) {
		    memcpy(buf,ln+off,typ);
		    while(typ && buf[typ-1]==' ') typ--;
		}
	  }
	  buf[typ]=0;
  }
  return buf;
}

static apfcn_v append_suffix(char *buf,int c)
{
  int len=strlen(buf);
  buf[len]=tolower(c);
  buf[len+1]=0;
}

static double PASCAL get_number(int typ)
{
  double d;
  //cUnits - 'F' or 'M' as set by #units key in SEF, the units of numbers read from the file
  //*sefd_units - "Feet" or "Meters" that's being specified in Walls #units command
  
  if(*StrFromField(sefd_buf,typ) && *sefd_buf!='*') {
	d=atof(sefd_buf);

	if(typ==SEFD_DIST && !bExcluding) {
		lentotal+=((cUnits=='F')?(d*0.3048):d);
	}

	switch(typ) {
		case SEFD_LEFT:
		case SEFD_RIGHT:
		case SEFD_CEIL:
		case SEFD_FLOOR:
		case SEFD_IAB:
		case SEFD_TAB:
		case SEFD_VDIST:
			if(*sefd_atunits!=cUnits) d*=(cUnits=='F')?0.3048:(1/0.3048);
			break;
		case SEFD_DIST:
			if(*sefd_units!=cUnits) d*=(cUnits=='F')?0.3048:(1/0.3048);
			break;
	}
  }
  else d=BLANK_VAL;
  return sefd_val[typ]=d;
}

static apfcn_i append_lrud(char *buf)
{
   int i,len;
   BOOL bPos=FALSE;
   double val;
   
   *buf='\t'; buf[1]='<'; len=2;
   for(i=0;i<4;i++) {
		val=get_number(sefd_lrud[i]);
		if(val==BLANK_VAL) strcpy(sefd_buf,"--");
		else {
			bPos=TRUE;
			strcpy(sefd_buf,fltstr(val+sefd_atc,sefd_atdec));
			if(*sefd_atunits!=*sefd_units) append_suffix(sefd_buf,*sefd_atunits);
		}  
		strcpy(buf+len,sefd_buf);
		len+=strlen(sefd_buf);
		if(i<3) *(buf+len++)=',';
   }
   if(!bPos) return 0;
   if(!strcmp(sefd_to, sefd_from) && get_number(SEFD_FAZI)!=BLANK_VAL) {
		*sefd_buf=',';
		strcpy(sefd_buf+1,fltstr(sefd_val[SEFD_FAZI], 1));
		strcpy(buf+len, sefd_buf);
		len+=strlen(sefd_buf);
   }
   strcpy(buf+len,">");
   return ++len;
}

static LPSTR davstr(int ikey,int dec)
{
  int bkey,bsign;
  char *p;
  static char buf[64];
  
  if(ikey==SEFD_DIST || (ikey==SEFD_FAZI && sefd_bazi==0) ||
     (ikey==SEFD_FINC && sefd_binc==0)) return fltstr(sefd_val[ikey],dec);
     
  if(ikey==SEFD_FINC) {
    bkey=SEFD_BINC;
    bsign=sefd_binc;
  }
  else {
    bkey=SEFD_BAZI;
    bsign=sefd_bazi;
  }
  
  if(bsign>0 && sefd_val[ikey]!=BLANK_VAL) {
    strcpy(buf,fltstr(sefd_val[ikey],dec));
    p=buf+strlen(buf);
  }
  else p=buf;
  
  if(sefd_val[bkey]!=BLANK_VAL) {
    *p++='/';
    strcpy(p,fltstr(sefd_val[bkey],dec));
  }
  else if(p==buf) strcpy(p,"--");
  return buf;  
}

static apfcn_i append_height(char *buf,double val)
{
	int len=wsprintf(buf,"\t%s",fltstr(val,sefd_atdec));
	if(*sefd_atunits!=*sefd_units && val!=BLANK_VAL)
	  append_suffix(buf+len++,*sefd_atunits);
	return len;
}

static apfcn_i process_vector(void)
{
  int i,len;
  double htcorr,iab,tab;

  if(!fpts) return 0;

  if(!datacount) {
	  if(i=process_data(strcpy(bf,bCPoint?pDataCP:pDataCT))) return i;
  }
  if(!bParen) cfg_GetArgv(ln+1,CFG_PARSE_ALL);
  
  *sefd_type=0;
  
  if(sefd_off[SEFD_TYPE]) {
    StrFromField(sefd_type,SEFD_TYPE);
	LPSTR pe=sefd_type;
	for(LPSTR p=pe; *p && (pe-p<4); p++) {
		if(*p!='\\' && *p!='/' && !isspace(*p)) *pe++=*p;
	}
	*pe=0;
    if(strchr(sefd_type,'X')) {
      if(!bExcluding) {
        bExcluding=TRUE;
        wsprintf(sefd_buf,"#[ Excluded %s --\n",(LPSTR)(bCPoint?"control pts":"shots"));
        if(EOF==fputs(sefd_buf,fpts)) return SEF_ERR_WRITE;
      }
    }
    else if(bExcluding && end_exclude()) return SEF_ERR_WRITE;
  }
  
  if(bCPoint) {
	  fixcount++;
	  ElimSpc(StrFromField(sefd_to,SEFD_STATION));
	  get_number(SEFD_EAST);
	  get_number(SEFD_NORTH);
	  get_number(SEFD_ELEV);
	
	  len=wsprintf(bf,"#Fix\t%s",(LPSTR)sefd_to);
	  len+=wsprintf(bf+len,"\t%s",fltstr(sefd_val[SEFD_EAST],sefd_cpdec));
	  len+=wsprintf(bf+len,"\t%s",fltstr(sefd_val[SEFD_NORTH],sefd_cpdec));
	  len+=wsprintf(bf+len,"\t%s",fltstr(sefd_val[SEFD_ELEV],sefd_elevdec));
	  if(sefd_elevunits!=*sefd_units) append_suffix(bf+len++,sefd_elevunits);
  }
  else {
  	  /*Due to an S4TOS5.EXE bug, a SMAPS 5.2 SEF export can produce names with
	  embedded spaces!*/
	  ElimSpc(StrFromField(sefd_to,SEFD_TO));
	  if(*sefd_to=='*' && !sefd_to[1]) *sefd_to=0;
	  ElimSpc(StrFromField(sefd_from, SEFD_FROM));
	  if(*sefd_from=='*' && !sefd_from[1]) *sefd_from=0;

	  if(!*sefd_to && !*sefd_from) {
	      return bExcluding?0:SEF_ERR_BADNAME;
	  }
	  if(!*sefd_to || !*sefd_from) {
		  if(!*sefd_from) strcpy(sefd_from,sefd_to);
	  }
	  
	  if(strcmp(sefd_to, sefd_from)) {
		if(!bExcluding) veccount++;
		get_number(SEFD_DIST);
		get_number(SEFD_FAZI);
		if(sefd_lendav>2) get_number(SEFD_FINC);
		if(sefd_bazi) get_number(SEFD_BAZI);
		if(sefd_binc) get_number(SEFD_BINC);
		if(sefd_htcorrect&1) htcorr=get_number(SEFD_VDIST);
		htcorr=(sefd_htcorrect==1)?get_number(SEFD_VDIST):BLANK_VAL;
		iab=(sefd_htcorrect&2)?get_number(SEFD_IAB):BLANK_VAL;
		tab=(sefd_htcorrect&4)?get_number(SEFD_TAB):BLANK_VAL;
		len=wsprintf(bf, "%s\t%s", (LPSTR)sefd_from, (LPSTR)sefd_to);
		for(i=0;i<sefd_lendav;i++) len+=wsprintf(bf+len,"\t%s",davstr(sefd_idx[i],sefd_dec[i]));
		if(htcorr!=BLANK_VAL) len+=append_height(bf+len, htcorr);
		else if(iab!=BLANK_VAL || tab!=BLANK_VAL) {
			len+=append_height(bf+len, iab);
			if(tab!=BLANK_VAL) len+=append_height(bf+len, tab);
		}
      }
	  else {
		  if(!sefd_uselrud) return bExcluding?0:SEF_ERR_BADNAME;
		  len=wsprintf(bf, "%s", (LPSTR)sefd_from);
	  }
	  if(sefd_uselrud) len+=append_lrud(bf+len);  
  }
  
  if(*sefd_type && *sefd_type!='*') len+=wsprintf(bf+len,"\t#S %s",(LPSTR)sefd_type);
  
  strcpy(bf+len,"\n");
  if(EOF==fputs(bf,fpts)) return SEF_ERR_WRITE;

  return 0;
}

static void elim_key_equals(void)
{
	char *p=ln+1;

	while(*p && !isspace((BYTE)*p) && *p!='=') p++;
	if(*p=='=') *p=' ';
}

static void fix_bookpfx()
{
	LPSTR p=bookpfx;

	for(LPCSTR ppj=prjname; *ppj && *ppj!='.'; ppj++) {
		*p++=(*ppj==' ')?'_':*ppj;
		if(p-bookpfx==4) break;
	}
	while(p<bookpfx+4) *p++='_';
	*p=0;
	_strupr(bookpfx);
	bookidx=0;
}

DLLExportC int PASCAL Import(LPSTR lpSefPath, LPSTR lpPrjPath, LPFN_IMPORT_CB pCB)
{
  int e;
 
  if(!pCB(SEF_GETPARAMS, &psef_params) || psef_params->version!=DLL_VERSION)
	  return SEF_ERR_VERSION;
 
  bAllowColons=(psef_params->flags&SEF_ALLOWCOLONS)!=0;
  bCombine=(psef_params->flags&SEF_COMBINE)!=0;
  bAttachSrc=(psef_params->flags&SEF_ATTACHSRC)!=0;
  pchrs_torepl=(LPCSTR)&psef_params->chrs_torepl;
  pchrs_repl=(LPCSTR)&psef_params->chrs_repl;

  fp=fpto=fpts=NULL;
  nCautions=0;

  LPCSTR p=dos_FullPath(lpSefPath,".sef");
  strcpy(sefpath,p?p:lpSefPath);
  if(!p || !(fp=fopen(sefpath, "r"))) return SEF_ERR_SEFOPEN;
  get_filetime(timestr, sefpath);

  p=dos_FullPath(lpPrjPath, ".wpj");
  strcpy(prjpath, p?p:lpPrjPath);
  if(!p || !(fpto=fopen(prjpath, "w"))) return errexit(SEF_ERR_CREATE);

  //Set pointer prjpath tail, to be replaced with each file --
  srvtail=trx_Stpnam(prjpath);

  trx_Stncc(prjname,trx_Stpnam(prjpath),80); 

  fix_bookpfx();

  sefname=trx_Stpnam(sefpath);

  cfg_KeyInit(seftypes,SEF_NUMTYPES);
  cfg_noexact=FALSE; /*Require exact matches*/
  cfg_quotes=cfg_equals=FALSE;
  
  sefd_numtypes=SEFD_NUMTYPES;
  trx_Bininit((DWORD *)sefdtypes,(DWORD *)&sefd_numtypes,comp_sefd);
  linecount=filecount=veccount=fixcount=surveycount=dircount=dirnest=datacount=0;
  bCPoint=bHdrLine=bUsingUTM=bUTM_used=false;
  repflags=0; bLongNames=false;
  lentotal=0.0;

  while((e=line_fcn())>=0) {
    linecount++;
    if(!e) continue;
    /*if(!e) return errexit(SEF_ERR_LINBLANK);*/
    if(*ln=='#') {
      /*Unrecognized keywords are ignored for now --*/
	  elim_key_equals(); /*Handle CML style SEF*/
	  e=process_key(cfg_GetArgv(ln+1,CFG_PARSE_KEY));
      if(e) return errexit(e);
    }
    else {
		lastblank=FALSE;
		if(*ln==' ') {
		  e=process_vector();
		  if(e) return errexit(e);
		}
		else {
		  process_comment(ln);
		}
	}
  }
  
  if(e==CFG_ERR_TRUNCATE) return errexit(SEF_ERR_SIZLN);

  /*Close current survey file --*/
  if(fpts && fclose(fpts)) {
     fpts=NULL;
     return errexit(SEF_ERR_WRITE);
  }
  
  /*Finish out closing ENDBOOKs --*/
  while(dirnest) {
    if(e=up_dir()) return errexit(e);
  }
  
  /*Close project file --*/
  if(fclose(fpto)) {
     fpto=NULL;
     return errexit(SEF_ERR_WRITE);
  }

  /*Close SEF file --*/
  fclose(fp);

  return 0;
}

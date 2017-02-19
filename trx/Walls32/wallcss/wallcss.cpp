// css2srv3.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <georef.h>

using namespace std;

#define DLL_VERSION 2.0f

typedef int (FAR PASCAL *LPFN_IMPORT_CB)(int fcn, LPVOID param);

struct CSS_PARAMS
{
	float version;
	UINT flags;
	char chrs_torepl[6];
	char chrs_repl[6];
};

typedef CSS_PARAMS *PCSS_PARAMS;
enum { CSS_ALLOWCOLONS=1, CSS_COMBINE=2, CSS_FLG_CHRS_PFX=4, CSS_FLG_CHRS_REPL=8, CSS_ATTACHSRC=16};
enum { CSS_GETPARAMS };
static PCSS_PARAMS pcss_params;
#define LEN_CHRS_REPL 4

#define LINLEN 512
#define MAX_SIZ_TITLE 24

#define SIZ_WARNMSG 120
#define SIZ_NAMBUF 80

static LPSTR errstr[]={
	"User abort",
	"Can't open or access project file",
	"Can't open or access data file",
	"Error writing file",
	"Can't create file",
	"Can't create folder",
	"No matching data files found."
};

enum {
	CSS_ERR_ABORTED=1,
	CSS_ERR_MAKOPEN,
	CSS_ERR_DATOPEN,
	CSS_ERR_WRITE,
	CSS_ERR_CREATE,
	CSS_ERR_DIRCREATE,
	CSS_ERR_NODATFILES,
	CSS_ERR_DATLINEABORT,
	CSS_ERR_MAKLINEABORT,
	CSS_ERR_VERSION=999
};

static FILE *fp,*fpto,*fppr;
static double degForward;
static LPCSTR pchrs_torepl;
static LPCSTR pchrs_repl;
static UINT repflags; // nchrs_repl, nchrs_pfx;
static UINT outlines, inlines, warncount, warntotal, revtotal;
static char ln[LINLEN],formatstr[LINLEN],cavename[84];
static char makpath[_MAX_PATH],frpath[_MAX_PATH], datpath[_MAX_PATH], wpjname[_MAX_PATH], prpath[_MAX_PATH];
static char nambuf0[SIZ_NAMBUF],nambuf1[SIZ_NAMBUF];
static char srvtitle[MAX_SIZ_TITLE], survey_sfx[256], srvname[16],bookname[16],bookpfx[8];
static int srvidx;
static char datfullname[80],datbasename[80],makname[80],makdate[24];
static char D_units[16],A_units[16],V_units[16],S_units[16];
static char item_typ[8];
static int	item_idx[8];
static char datestr[16];
static char teamline[104];
static bool bRedundant,bLrudOnly,bForwardOK,bHasLinksOrFixedPts,bLinksPending;
static bool A_bQuad,D_bInches,S_bInches,V_bDepth,V_bMinutes,L_bTo;
static bool bFileHasCorrections,bFileHasIncD,bFileHasIncA,bFileHasIncAB,bFileHasIncV,bFileHasIncVB,bFileHasLrudTo;
static bool bMakFile,bWarn,bUsePrefix2,bCombine,bAttachSrc,bAllowColons,bLongNames,bColonsAccepted,bDefLrudTo;

#define PI 3.141592653589793
#define U_DEGREES (PI/180.0)

#define SSBUFLEN 32
static char ssDepthBuf[SSBUFLEN];

static int nameinc=0;
static UINT filecount,segcount,segtotal,veccount,vectotal,fixtotal,linktotal;
static double lentotal,lensurvey;

static VEC_CSTR vDatName,vWarnMsg,vMakWarnMsg;
static double refeast,refnorth,refup;
static int refdatum,refzone;
static CString errMsg;

/*
struct DATPFX
{
	DATPFX(LPCSTR pNam) : cnt(-1) 
	{
		LPSTR p=pfx;
		for(;p-pfx<5;p++) {
		   *p=*pNam;
		   if(!*p || (*p=*pNam++)==' ') *p='_';
		}
		*p++='@'; *(LPSTR)p=0;
	}
	LPCSTR getname(LPSTR buf)
	{
		if(cnt==99) { //should be 99
			if(++pfx[5]=='[') return NULL;
			cnt=-1;
		}
		sprintf(buf,"%s%02u",pfx,++cnt);
		return buf;
	}
	char pfx[8];
	int cnt;
};
*/

struct NAMREC {
	NAMREC(int id, LPCSTR s) : idx(id), nam(s) {}
	NAMREC() : idx(-1) {}
	bool operator == (const NAMREC &nr) {return idx==nr.idx && !nam.Compare(nr.nam);}
	int idx;
	CString nam;
};

struct FIXREC
{
	NAMREC nr;
	double fE,fN,fU;
};

typedef std::vector<FIXREC> VFIX;
typedef VFIX::iterator it_fix;
typedef std::vector<NAMREC> VNAMREC;
typedef VNAMREC::iterator it_namrec;
typedef std::vector<VNAMREC> VVNAMREC;
typedef VVNAMREC::iterator it_vnamrec;

//typedef std::vector<DATPFX> VDATPFX;
//typedef VDATPFX::iterator it_datpfx;

struct GEOREC
{
	char gPrefix2[16];
	int gParent;
	int gZ;
	int gD;
	int iMakLine;
	double gE,gN,gU;
	VFIX vFix;
	VNAMREC vLink;
	GEOREC(int refZ,int refD,double refE,double refN,double refU,int line) :
	gZ(refZ), gD(refD), gE(refE), gN(refN), gU(refU),iMakLine(line) {}
};

typedef std::vector<GEOREC> VGEO;
typedef VGEO::iterator it_geo;

static GEOREC *lastdat_pgeo=NULL;
static FIXREC *lastdat_pfix=NULL;
static int lastdat_status=8;

static VGEO vGeo;
static VVNAMREC vEquiv;

struct DATUM2
{
	LPCSTR wnam;
	LPCSTR cnam;
};

static DATUM2 sDatum[] ={
	{ "Adindan",               "Adindan" },
	{ "Arc 1950",              "Arc 1950" },
	{ "Arc 1960",              "Arc 1960" },
	{ "Australian Geod `66",   "Australian 1966" },
	{ "Australian Geod `84",   "Australian 1984" },
	{ "Camp Area Astro",       "Camp Area Astro" },
	{ "Cape",                  "Cape" },
	{ "European 1950",         "European 1950" },
	{ "European 1979",         "European 1979" },
	{ "Geodetic Datum `49",    "Geodetic 1949" },
	{ "Hong Kong 1963",        "Hong Kong 1963" },
	{ "Hu-Tzu-Shan",		   "Hu Tzu Shan" },
	{ "Indian Bangladesh",     "Indian" },
	{ "NAD27 Alaska",          "" },
	{ "NAD27 Canada",          "" },
	{ "NAD27 CONUS",           "North American 1927" },
	{ "NAD27 Cuba",            "" },
	{ "NAD27 Mexico",          "" },
	{ "NAD27 San Salvador",    "" },
	{ "NAD83",                 "North American 1983" },
	{ "Oman",                  "Oman" },
	{ "Ord Srvy Grt Britn",    "Ordinance Survey 1936" },
	{ "Prov So Amrican `56",   "South American 1956" },
	{ "Puerto Rico",           "" },
	{ "South American `69",    "South American 1969" },
	{ "Tokyo",                 "Tokyo" },
	{ "WGS 1972",              "WGS 1972" },
	{ "WGS 1984",              "WGS 1984" }
};

#define NDATUMS (sizeof(sDatum)/sizeof(DATUM2))
#define WGS84_INDEX (NDATUMS-1)

static int find_linknam(VNAMREC &vnam, LPCSTR s)
{
	for(it_namrec it=vnam.begin(); it!=vnam.end(); it++) {
		if(!it->nam.Compare(s)) return it->idx;
	};
	return -1;
}

static bool find_fixnam(VFIX &vf, LPCSTR nam)
{
	for(it_fix it=vf.begin(); it!=vf.end(); it++) {
		if(!strcmp(nam,it->nr.nam))	return 1;
	}
	return 0;
}

static bool find_namrec(VNAMREC &vnr, NAMREC nr)
{
	for(it_namrec it=vnr.begin(); it!=vnr.end(); it++) {
		if(nr==*it)	return 1;
	}
	return 0;
}

static int filter_name(LPSTR nam)
{
	LPCSTR pp;
	int len=strlen(nam);
	bool bColonAllow=bAllowColons;
	bool bHasColon=false;

	for(LPSTR p=nam+len-1; p>=nam; p--) {
		pp=strchr(pchrs_torepl, *p);
		if(pp) {
			int i=pp-pchrs_torepl;
			if(i==1 && bColonAllow) {
			   bColonAllow=false;
			   //allow the last colon if base name would be no longer than 8 --
			   if(len-(p-nam+1)<=8) {
				   bHasColon=bColonsAccepted=true;
				   continue;
			   }
			}
			*p=pchrs_repl[i];
			repflags|=(1<<i);
		}
	}
	return bHasColon?-len:len;
}

static LPCSTR get_name8(char *name8,LPSTR p)
{
	int len=filter_name(p);

	if(len<0) {
		ASSERT(bAllowColons);
		//accept name with one colon separator--
		len=0;
	}
	else {
		len-=8;
		if(len>0) {
			//split the name to produce 1st-level prefix
			memcpy(name8,p,len);
			p+=len;
			name8[len++]=':';
			bLongNames=true;
		}
		else len=0;
	}
	strcpy(name8+len, p);
	return name8;
}

static LPCSTR get_name(char *nambuf,const NAMREC &nr,int idx)
{
	int len=0;
	if(nr.idx!=idx) {
		//Possible only when there are links --
		ASSERT(bUsePrefix2);
		strcpy(nambuf, vGeo[nr.idx].gPrefix2);
		len=strlen(nambuf);
		nambuf[len++]=':';
		if(strlen(nr.nam)<=8) nambuf[len++]=':';
	}
	get_name8(nambuf+len, (LPSTR)(LPCSTR)nr.nam);
	return nambuf;
}

static int chk_equiv(const NAMREC &nr0,const NAMREC &nr1)
{
	//If both names are in the same equiv set, return corresponding index ov vEquiv.
	//Otherwise insert another equivalence, while possibly merging 2 existing elements,
	//and return -1;

	if(!vEquiv.empty()) {
		for(it_vnamrec itv=vEquiv.begin(); itv!=vEquiv.end(); itv++) {

			bool f0=find_namrec(*itv,nr0);
			bool f1=find_namrec(*itv,nr1);

			if(f0==f1) {
				if(f0 && f1) return itv-vEquiv.begin(); //redundant equivalence
				continue;
			}
			
			const NAMREC &nr2(f1?nr0:nr1);
			//see if nr2 is in another equiv set. if so, allow it and
			//move that set to this one. If not, simply add nr2 to this one --
			for(it_vnamrec itv2=itv+1; itv2!=vEquiv.end(); itv2++) {
				if(find_namrec(*itv2,nr2)) {
					for(it_namrec itv3=itv2->begin();itv3!=itv2->end(); itv3++)
						itv->push_back(*itv3);
					vEquiv.erase(itv2);
					return -1; //ok!
				}
			}
			//not found in another set, so add it to this one --
			itv->push_back(nr2);
			return -1; //OK so far!
		}
	}

	//neither string exists, so add a new set --
	vEquiv.push_back(VNAMREC());
	vEquiv.back().push_back(nr0);
	vEquiv.back().push_back(nr1);
	return -1;
}

static _inline void outs(LPCSTR s)
{
	fputs(s,fpto);
}

static void _cdecl outf(LPCSTR fmt,...)
{
	va_list va;
	va_start(va,fmt);
	vfprintf(fpto,fmt,va);
}

static int getline(char *line=ln)
{
  int len;
  if(NULL==fgets(line,LINLEN,fp)) return CFG_ERR_EOF;
  len=strlen(line);
  if(len && line[len-1]=='\n') line[--len]=0;
  return len;
}

static int getnextline()
{
	long off=ftell(fp);
	int len=getline(formatstr);
	fseek(fp,off,0);
	return len;
}


static int err_format(char *msg)
{
  strcpy(ln,msg);
  return CSS_ERR_DATLINEABORT;
} 

static void _cdecl warn_format(LPCSTR fmt,...)
{
	if(!fmt) {
		//flush any displayed warning to srv file
		if(!vWarnMsg.empty()) {
			for(it_cstr it=vWarnMsg.begin(); it!=vWarnMsg.end();it++) {
				outf(";***Caution: %s:%s.\n",datfullname,(LPCSTR)*it);
				outlines++;
			}
			vWarnMsg.clear();
			bWarn=false; //msg no longer pending
		}
		return;
	}

	char buf[SIZ_WARNMSG];
	va_list va;
	va_start(va,fmt);
	_vsnprintf(buf,SIZ_WARNMSG-1,fmt,va);
	buf[SIZ_WARNMSG-1]=0;
	CString s;
	s.Format("%u - %s",inlines,buf);
	vWarnMsg.push_back(s);
	warncount++;
	bWarn=true; //msg savved but not yet written to srv
} 

static void outtab(void)
{
	outs("\t");
}

static LPSTR get_trimmed_numeric(LPSTR p)
{
   if(strchr(p,'.')) {
	   int i=strlen(p);
	   while(i>1 && p[i-1]=='0') i--;
	   if(p[i-1]=='.') i--;
	   p[i]=0;
	   if(!*p) p="0";
   }
   return p;
}

static LPSTR CheckNumeric(LPSTR p)
{
	static char buf[32];
	if(!IsNumeric(p)) {
		_snprintf(buf,32,"%.2f",atof(p));
		warn_format("Unexpected numeric format (%s) replaced with %s",p,buf);
		return buf;
	}
	return p;
}
static void out_trimmed_numeric(LPSTR p)
{
	fputs(get_trimmed_numeric(p),fpto);
}

static void out_trimmed_numeric(double d,int dec=2)
{
	char buf[40];
	sprintf(buf, "%.*f", dec, d);
	out_trimmed_numeric(buf);
}

static void out_numeric(int i)
{
   if(i>=cfg_argc) warn_format("Numeric value expected");
   else {
	  LPSTR p=CheckNumeric(cfg_argv[i]);
	  fputs(get_trimmed_numeric(p),fpto);
   }
}

static void outln(void)
{
  outs("\n");
  outlines++;
}

static int out_dimensions(BOOL bOut)
{
    int i;
    INT8 dsgn[12];
    double f;
	int numpositive=0;

    for(i=5;i<9;i++) {
	  f=atof(cfg_argv[i]);
	  if(f<0.0 || f>999.0) dsgn[i]=-1;
	  else if(dsgn[i]=f?1:0) numpositive--; //actually negative of positive count
	}

	if(numpositive) {
		if(!dsgn[5] && !dsgn[8]) dsgn[5]=dsgn[8]=-1;
		else numpositive=-numpositive;
	}

	if(!bOut || !numpositive) return numpositive;
	
	outs("\t<");
 	if(dsgn[5]<0) outs("--"); else out_numeric(5);
	outs(",");
 	if(dsgn[8]<0) outs("--"); else out_numeric(8);
	outs(",");
 	if(dsgn[6]<0) outs("--"); else out_numeric(6);
	outs(",");
 	if(dsgn[7]<0) outs("--"); else out_numeric(7);
	outs(">");

	/*
	if(numpositive<0) {
		warn_format("Both L and R wall distances had zero values, assuming n/a (--)");
	}
	*/

	return numpositive;
}  

static BOOL get_datestr(UINT m,UINT d,UINT y)
{
	if(!m || m>12 || !d || d>31 || y>3000) return FALSE;
	if(y<50) y+=2000;
	else if(y<100) y+=1900;
	else if(y<1800) return FALSE;
    sprintf(datestr,"%4u-%02u-%02u",y,m,d);
	return TRUE;
}

static BOOL out_survey_fld(char *p,char *typ)
{
	if(strlen(p)<11 || _memicmp(p+7,typ,4)) {
	  sprintf(ln,"\"SURVEY %s\" expected",typ);
	  err_format(ln);
	  return 0;
	}

	if(*typ=='T') return TRUE;

	p+=11;
	if(*p==':') p++;
	while(isspace(*p)) p++;

	if(*typ=='N') {
		strncpy(srvtitle,p, MAX_SIZ_TITLE-1);
		srvtitle[MAX_SIZ_TITLE-1]=0; //max length in Compass is 12!
		for(p=srvtitle; *p; p++) {
			if(isspace(*p)) break;
		}
		*p=0;

		if(!*srvtitle) {
			warn_format("\"SURVEY NAME:\" has no argument");
			strcpy(srvtitle, "???");
		}

		*survey_sfx=0;
	}
	else if(*typ=='D') {
		if(typ=strstr(p, "COMMENT:")) {
			*typ=0;
			typ+=8;
			while(isspace((BYTE)*typ)) typ++;
			if(*typ) {
				strncpy(survey_sfx, typ, sizeof(survey_sfx));
				survey_sfx[sizeof(survey_sfx)-1]=0;
			}
		}
		cfg_GetArgv(p,CFG_PARSE_ALL);
		*datestr=0;
		if(cfg_argc<3 || !get_datestr(atoi(cfg_argv[0]), atoi(cfg_argv[1]), atoi(cfg_argv[2]))) {
			CString s;
			for(int i=0;i<cfg_argc;i++) {
				s+=cfg_argv[i]; s+=' ';
			}
			s.TrimRight();
			warn_format("Unrecognized date format (%s)",(LPCSTR)s);
		}
	}
	return TRUE;
}

static int get_item_typ(int c,int i)
{
	switch(c) {
		case 'L' : c='D'; item_idx[i]=2; break;
		case 'A' : c='A'; item_idx[i]=3; break;
		case 'D' : c='V'; item_idx[i]=4; break;
		default  : err_format("Bad shot item order");
	}
	return c;
}

static BOOL get_formatstr(int i)
{
	char *pf;
	char order[20];
	int lenf;

	A_bQuad=D_bInches=S_bInches=V_bDepth=false;
	bRedundant=false;
	L_bTo=bDefLrudTo;

	if(i>=cfg_argc || (lenf=strlen(pf=_strupr(cfg_argv[i])))<11) {
		err_format("FORMAT argument missing");
		return FALSE;
	}

	bRedundant=pf[11]=='B';

	if(!L_bTo) L_bTo = (lenf>12 && pf[12]=='T');

	if(pf[3]=='W') {
		V_bDepth=TRUE;
		/*reorder 'D' (inclination) measurment to be last*/
		if(pf[10]!='D') {
			if(pf[8]=='D') pf[8]=pf[10];
			else pf[9]=pf[10];
			pf[10]='D';
		}
	}

	for(i=0;i<3;i++) {
		item_typ[i]=get_item_typ(pf[i+8],i);
	} 
	item_typ[3]=0;

	for(i=4;i<8;i++) item_typ[i]='S';
	item_idx[4]=4+1; //cfg_argv idx of L
	item_idx[5]=4+4; // "   " R
	item_idx[6]=4+2; // "   " U
	item_idx[7]=4+3; // "   " D

	if(!strchr(item_typ,'D') ||
	   !strchr(item_typ,'A') ||
	   !strchr(item_typ,'V')) err_format("Bad shot item order");

	if(pf[0]=='R') strcpy(A_units,"Grads");
	else {
	  strcpy(A_units,"Deg");
	  if(pf[0]=='Q') A_bQuad=TRUE;
	}

	if(pf[1]=='M') strcpy(D_units,"Meters");
	else {
	  strcpy(D_units,"Feet");
	  if(pf[1]=='I') D_bInches=TRUE;
	}

	if(pf[2]=='M') strcpy(S_units,"Meters");
	else {
	  strcpy(S_units,"Feet");
	  if(pf[2]=='I') S_bInches=TRUE;
	}

	strcpy(order,item_typ);

	if(V_bDepth) {
		item_typ[2]='D';
		strcpy(order+2," Tape=SS");
	}
	else {
		if(pf[3]=='R') strcpy(V_units,"Grads");
		else if(pf[3]=='G') strcpy(V_units,"Pcnt");
		else {
		  strcpy(V_units,"Deg");
		  if(pf[3]=='M') V_bMinutes=TRUE;
		}
	}

	pf=formatstr;

	pf+=sprintf(pf,"Order=%s D=%s A=%s",order,D_units,A_units);
	if(!V_bDepth) pf+=sprintf(pf," V=%s",V_units);
	pf+=sprintf(pf," S=%s",S_units);

	if(bRedundant) {
		pf+=sprintf(pf," AB=%s",A_units);
		pf+=sprintf(pf," VB=%s",V_units);
	}

	*pf=0;
	return TRUE;
}	

static void out_correct(char *p,char *nam)
{
	outf(" %s=",nam);
	out_trimmed_numeric(p);
	outf("%c",(nam[3]=='D')?'f':'d');
}

static BOOL out_units(char *p)
{
	int format_idx;
	static char *pZero="0";
	char *pIncA=pZero,*pIncV=pZero,*pIncD=pZero,*pIncAB=pZero,*pIncVB=pZero;

	outln();

	if(bWarn) warn_format(0);
    cfg_GetArgv(p,CFG_PARSE_ALL);
	outs("#Units decl=");
	out_numeric(0);
	outs("d");

	int i=0;

	if(cfg_argc>i+2 && !stricmp(cfg_argv[i+1],"FORMAT:")) i+=2;
	else cfg_argv[0]="DDDDUDRLLADN";

	if(!get_formatstr(i)) return FALSE;
	format_idx=i;

	if(cfg_argc>i+1 && !stricmp(cfg_argv[++i],"CORRECTIONS:")) {
		  if(++i<cfg_argc) p=cfg_argv[i];
		  if(atof(p)) pIncA=p;
		  if(++i<cfg_argc) p=cfg_argv[i];
		  if(atof(p)) pIncV=p;
		  if(++i<cfg_argc) p=cfg_argv[i];
		  if(atof(p)) pIncD=p;
		  if(cfg_argc<=i) warn_format("Three instrument corrections expected on line");
	}

	if(bRedundant && cfg_argc>i+1 && !stricmp(cfg_argv[++i],"CORRECTIONS2:")) {
		  if(++i<cfg_argc) p=cfg_argv[i];
		  if(atof(p)) pIncAB=p;
		  if(++i<cfg_argc) p=cfg_argv[i];
		  if(atof(p)) pIncVB=p;
		  if(cfg_argc<=i) warn_format("Two backsight corrections expected on line");
	}

	outf(" %s",formatstr);
	if(format_idx) outf(" ;Format: %s",cfg_argv[format_idx]);
	outln();

	if(!bFileHasCorrections) {
		bFileHasCorrections = L_bTo || (bCombine && bFileHasLrudTo) || pIncA!=pZero || pIncV!=pZero || pIncD!=pZero ||
			pIncAB!=pZero || pIncVB!=pZero;
	}

	if(bFileHasCorrections) {
		//Once a correction!=0 is assigned to a measurement type, assign *some* correction to that
		//type for each of the remaining surveys in the file --
		outs("#Units");
		if(bCombine) {
			if(bFileHasIncD || (bFileHasIncD=pIncD!=pZero)) out_correct(pIncD, "IncD");
			if(bFileHasIncA || (bFileHasIncA=pIncA!=pZero)) out_correct(pIncA, "IncA");
			if(bFileHasIncV || (bFileHasIncV=pIncV!=pZero)) out_correct(pIncV, "IncV");
			if(bFileHasIncAB || (bFileHasIncAB=pIncAB!=pZero)) out_correct(pIncAB, "IncAB");
			if(bFileHasIncVB || (bFileHasIncVB=pIncVB!=pZero)) out_correct(pIncVB, "IncVB");
			if(bFileHasLrudTo || (bFileHasLrudTo=L_bTo)) outf(" LRUD=%c", L_bTo?'T':'F');
		}
		else {
			if(pIncD!=pZero) out_correct(pIncD, "IncD");
			if(pIncA!=pZero) out_correct(pIncA, "IncA");
			if(pIncV!=pZero) out_correct(pIncV, "IncV");
			if(pIncAB!=pZero) out_correct(pIncAB, "IncAB");
			if(pIncVB!=pZero) out_correct(pIncVB, "IncVB");
			if(L_bTo) outs(" LRUD=T\n");
		}
		outln();
	}

	if(*datestr) {
	    outf("#Date %s\n",datestr);
		outlines++;
	}
	return TRUE;
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

static void out_inches(double d)
{
	double w;

	if(d<0.0) {
		outs("-");
		d=-d;
	}
	d=modf(d,&w); //d==fractional portion of ft length
	if(d*12 >= 11.5) {
		d=0; w+=1.0;
	}
	outf("%.0fi",w);
	fprintf(fpto,"%.0f",d*12);
}

static void warn_backsight(LPCSTR ptyp)
{
	warn_format("Reversed sign assumed on %s backsight (corrected)", ptyp);
	revtotal++;
}

static void out_measurement(char *p,int c,BOOL bBack)
{
	double d,w;
	bool bInches,bOutofRange=false;
	int units;
	
	p=CheckNumeric(p);

	switch(c) {
		case 'S' :
		case 'D' :
			d=atof(p);
			if(c=='S') {
				//LRUD
				bInches=S_bInches;
				units=*S_units;
			}
			else {
				bInches=D_bInches;
				units=*D_units;
			}
		
			if(d<0.0) {
				warn_format("Negative distance, \"%s ft\", will abort compilation", p);
				out_trimmed_numeric(p);
				if(units=='M') outs("f");
				break;
			}
		
			lensurvey+=d;
			if(units=='M') {
				out_trimmed_numeric(d*0.3048);
				break;
			}
			if(bInches) {
				out_inches(d);
				break;
			}
			//distance in feet
			out_trimmed_numeric(p);
			if(c=='S' && units!=*D_units) outf("%c",tolower(units));
			break;

		case 'A' :
			d=atof(p);
			if(d==360.0) d=0.0;
			if(d<0.0 || d>360.0) {
				if(d==-999.0) {
					if(bBack) {
						if(bForwardOK) break; //ignore
					}
					else if(bRedundant) {
						outs("--");
						bForwardOK=false;
						break;
					}
				}
				else if(bBack) {
					warn_format("Out-of-range back azimuth (%s deg) will abort compilation", p);
				}
				else warn_format("Azimuth (%s deg) out of range and will abort compilation", p);
			}
			else if(bRedundant) {
				if(!bBack) {
					bForwardOK=true;
					degForward=d;
				}
				else if(bForwardOK) {
					double df=abs(d-degForward);
					if(df>180) df=360-df;
					if(df<20) {
						d+=180;
						if(d>=360) d-=360;
						warn_backsight("azimuth");
					}
				}
			}
			if(bBack) outs("/");
			if(A_bQuad) {
				if(d<=90.0 || d>=270.0) {
				  outs("N");
				  if(d>=270.0) {
					  d=360.0-d;
					  units='W';
				  }
				  else units='E';
				}
				else {
				  outs("S");
				  if(d>180.0) {
					units='W';
					d-=180.0;
				  }
				  else {
					units='E';
					d=180.0-d;
				  }
				}
				out_trimmed_numeric(d);
				outf("%c",units);
				break;
			}

			if(*A_units=='G') d*=(10.0/9.0);
			out_trimmed_numeric(d);
			break;

		case 'V' :
			d=atof(p);
			if(d<-90.0 || d>90.0) {
				if(d==-999.0) {
					if(bBack) {
						if(bForwardOK) break;
					}
					else if(bRedundant) {
						outs("--");
						bForwardOK=false;
						break;
					}
				}
				else if(bBack && bForwardOK) {
					warn_format("Out-of-range back inclination (%s deg) will abort compilation", p);
					break;
				}
				warn_format("Out-of-range inclination (%s deg) will abort compilation", p);
			}
			else if(bRedundant) {
				if(!bBack) {
					bForwardOK=true;
					degForward=d;
				}
				else if(bForwardOK) {
					if((d>=0)==(degForward>=0)) {
						double ba=abs(d), fa=abs(degForward);
						if(min(ba,fa)>5 && abs(ba-fa)/max(ba,fa)<=0.2) {
							d=-d;
							warn_backsight("inclination");
						}
					}
				}
			}

			if(bBack) outs("/");
			if(*V_units=='G') d*=(10.0/9.0);
			else if(*V_units=='P') d*=(100.0/45.0);
			else if(V_bMinutes) {
				if(d<0.0) {
					outs("-");
					d=-d;
				}
				d=modf(d,&w);
				outf("%.0f:",w);
				out_trimmed_numeric(d*60,1);
				break;
			}
			out_trimmed_numeric(d);
			break;
	}
}

static bool IsVertical()
{
	//allow editing of compass-generated DAT --
	LPCSTR p=cfg_argv[4];
	if(*p=='-') p++;
	return *p++=='9' && *p++=='0' && ( !*p || (*p++=='.' && (!*p || (*p++=='0' && (!*p || *p=='0')))));
}

static void out_item(int i)
{
	if(V_bDepth && i==2) {
		//cfg_argv[2] is SS distance in ft assuming IH=TH=0
		//cfg_argv[3] is azimuth (not used here)
		//cfg_argv[4] is inclination
		//
		//must get IH-TH (frDepth-toDepth) --
		double ddepth=atof(cfg_argv[2])*sin(atof(cfg_argv[4])*U_DEGREES); //difference in feet
		if(*D_units=='M') {
			ddepth*=0.3048;
			i=2;
		}
		else {
			if(D_bInches) {
				out_inches(ddepth);
				return;
			}
			i=1;
		}
		_snprintf(ssDepthBuf, SSBUFLEN-1, "%.*f", i, ddepth);
		out_trimmed_numeric(ssDepthBuf);
		if(*S_units!=*D_units) outf("%c",tolower(*D_units));
		return;
	}
	int c=item_typ[i];

	if(c=='V' || c=='A') bForwardOK=false; 
	
	out_measurement(cfg_argv[item_idx[i]],c,FALSE);

	if(bRedundant && (c=='V' || c=='A') && !IsVertical()) {
		i=(c=='A')?9:10;
		if(i<cfg_argc) out_measurement(cfg_argv[i], c, TRUE);
	}
}

static void out_flagstr(LPCSTR pF)
{
	outf("\t#S %s",pF);
	//while (*pF) outf("/%c",*pF++);
}	

static BOOL out_comment(int i)
{
	static char buf[LINLEN];
	char *pb;

	if(i) {
		for(pb=buf;i<cfg_argc;i++) pb+=sprintf(pb," %s",cfg_argv[i]);
		if(strlen(buf)<=30)
			//Print on same line as vector data
			return TRUE;
		//or print on line above vector data --
	}
	else outtab();

	for(pb=buf;isspace((BYTE)*pb) || *pb==';';pb++);
	outf(";%s",pb);
	if(i) outln();
	return FALSE;
}

static void out_labels(void)
{
	int e;

	outs(";FROM\tTO");
	for(e=0;e<3;e++) {
	   switch(item_typ[e]) {
		   case 'D' : if(V_bDepth && e==2) outs("\tFRDEPTH-TODEPTH");
					  else outs("\tDIST");
					  break;
		   case 'A' : outs("\tAZ");
					  break;
		   case 'V' : outs("\tINCL");
					  break;
	   }
	}
	outln();
	outln();
}

static void deg2dms(double fdeg, int &ideg, int &imin, double &fsec)
{ 
	if(fdeg<0) fdeg=-fdeg;
	ideg = (int)fdeg;
	imin = (int) ((fdeg - ideg) * 60.0);
	fsec = (fdeg - ideg - imin/60.0) * 60.0 * 60.0;
}

static void	outRefSpec(int datum,int zone,double east,double north, double up)
{
	double conv,lat,latsec,lon,lonsec;
	int latdeg,latmin,londeg,lonmin;
	//int dtm=datum?((datum==1)?19:15):27;
	//void geo_UTM2LatLon(int zone, double x, double y, double *lat, double *lon, int datum, double *conv=0);
	geo_UTM2LatLon(zone, east, north, &lat, &lon, datum, &conv);
	deg2dms(lat, latdeg, latmin, latsec);
	deg2dms(lon, londeg, lonmin, lonsec);
	int flags=2+(lon<0.0)*4+(lat<0.0)*8;

	fprintf(fppr,".REF\t%.3f %.3f %d %.3f %.0f %u %u %u %.3f %u %u %.3f %u \"%s\"\n",
	north,
	east,
	zone,
	conv,
	up,
	flags,
	latdeg,
	latmin,
	latsec,
	londeg,
	lonmin,
	lonsec,
	datum,
	sDatum[datum].wnam);
}

static void wrap_comment(LPSTR buf)
{
	bool bspc=true;
	LPSTR p0=buf;

	for(LPSTR p=buf; *p; p++) {
		if(isspace(*p)) {
			*p=' ';
			if(bspc) continue;
			bspc=true;
			if(p-p0>75) {
				*p=0;
				outf(";%s\n", p0);
				outlines++;
				p0=p+1;
				continue;
			}
		}
		else {
			bspc=false;
		}
	}
	if(*p0) {
		outf(";%s\n", p0);
		outlines++;
	}
}

static void out_wpj_srvhdr(LPCSTR path)
{
	char buf[260];
	LPSTR p=GetRelativePath(buf, path, prpath, 0); //0 == there's a name on makpath
	p=trx_Stpnam(buf);
	fprintf(fppr, ".SURVEY\t%s\n.NAME\t%s\n", p, p);
	if(p>buf && p[-1]=='\\') p--;
	*p=0;
	if(*buf) fprintf(fppr, ".PATH\t%s\n", buf);
	fputs(".STATUS\t196618\n",fppr);
}

static int open_fpto(UINT idx)
{
	GEOREC *pgeo=bMakFile?&vGeo[idx]:NULL;
	FIXREC *pfix=pgeo?(pgeo->vFix.empty()?NULL:&pgeo->vFix[0]):NULL;
	//char pathbuf[_MAX_PATH];

	if(!fppr) {
		if((fppr=fopen(prpath,"w"))==NULL) {
			return CSS_ERR_CREATE;
		}
		fprintf(fppr,";WALLS Project File\n"
			".BOOK\t%s Project\n"
			".NAME\tPROJECT\n"
			".OPTIONS\tflag=Fixed\n"
			".STATUS\t%u\n",wpjname,refzone?1051139:1048579);
		if(refzone) {
			ASSERT(bMakFile);
			outRefSpec(refdatum,refzone,refeast,refnorth,refup);
		}
		if(bMakFile && bAttachSrc) {
			out_wpj_srvhdr(makpath);
		}
	}

	if(!srvidx) {
		//Prior to 1st data file, add BOOK to project -
		//If DAT has no fixed pts, use the same ref and status settings as assigned to the
		//previous file, or, if this is the 1st file, inherit everything from the root.
		//If DAT has fixed points, call outRefSpec() only if zone and/or datum
		//is different from that of root, in which case use 1st fix as reference location
		//and don't inherit status, but set UTM and derive from date flags.
		//
		//if open,   status= 8 (inherited), 2568 (own ref), 1352 (unspecified)
		int status=8;
		if(pfix) {
			if(pgeo->gZ) {
				if(pgeo->gD!=refdatum || pgeo->gZ!=refzone) {
					status=2568; //UTM and DATE flags
					lastdat_pgeo=pgeo;
					lastdat_pfix=pfix;
				}
			}
			else if(!refzone) status=1352;

		}
		else if(idx) {
			//use the same settings as that of the previous file!
			status=lastdat_status;
		}

		lastdat_status=status;

		if((int)vDatName.size()<(bCombine?5:1)) status|=1; //open books in these cases
		fprintf(fppr,
		    ".BOOK\t%s\n"
			".NAME\t%s\n"
			".STATUS\t%u\n", datbasename, bookname, status);

		if((status|1)==2569) {
			outRefSpec(lastdat_pgeo->gD, lastdat_pgeo->gZ, lastdat_pfix->fE, lastdat_pfix->fN, lastdat_pfix->fU);
		}

		if(bAttachSrc) {
			out_wpj_srvhdr(datpath);
		}
	}

	LPCSTR pLinkSfx="";

	if(bLinksPending) {
		if(pgeo->vLink.empty())
			pLinkSfx=" (Fixed Pts)";
		else if(pgeo->vFix.empty())
			pLinkSfx=" (Links)";
		else
			pLinkSfx=" (Links and Fixed Pts)";
	}

_first:

	fprintf(fppr,".SURVEY\t");
	if(bLinksPending) {
		fprintf(fppr,"%s%s\n",datbasename,pLinkSfx);
	}
	else {
		if(bCombine) fprintf(fppr,"%s\n", datbasename);
		else {
			fprintf(fppr, "%s", srvtitle);
			if(*survey_sfx && strlen(survey_sfx)<=40) {
				fprintf(fppr, " - %s\n", survey_sfx);
			}
			else fprintf(fppr,"\n");
		}
	}

	sprintf(srvname, "#%03d-%03d", idx, srvidx++);
	fprintf(fppr,".NAME\t%s\n.STATUS\t8\n",srvname);

	strcpy(trx_Stpnam(prpath),srvname);
	strcpy(trx_Stpext(prpath),".srv");

	if((fpto=fopen(prpath, "w"))==NULL) {
		return CSS_ERR_CREATE;
	}
	outlines=0;

	if(!bMakFile) return 0;

	if(bUsePrefix2) {
		ASSERT(bMakFile);
		if(idx && pgeo->vLink.empty() && pgeo->vFix.empty()) {
			strcpy(pgeo->gPrefix2,vGeo[idx-1].gPrefix2);
			pgeo->gParent=vGeo[idx-1].gParent;
		}
		else {
			strcpy(pgeo->gPrefix2, bookname);
			pgeo->gParent=idx;
		}
	}

	if(bLinksPending) {

		outf(";%s%s\n;Import from %s dated %s\n",datbasename,pLinkSfx,makname,makdate);
		outlines+=2;

		if(!pgeo->vLink.empty()) {
			if(pgeo->vFix.empty()) {
				outs(";NOTE: This file has only link information since no fixed points were assigned.\n");
				outlines++;
			}
			outs("\n;Links to other files with their assigned prefixes:\n");
			outlines+=2;
			int nchk=pgeo->vLink.size();
			VEC_BYTE vchk(nchk, 0);

			for(it_byte itc=vchk.begin(); itc!=vchk.end(); itc++) {
				if(!*itc) {
					NAMREC *pnr=&pgeo->vLink[itc-vchk.begin()];
					int id=pnr->idx; //look for links to this file
					int len=fprintf(fpto, ";%s - %s::", trx_Stpnam(vDatName[id]),vGeo[id].gPrefix2);
					for(it_byte it=itc; it!=vchk.end(); it++, pnr++) {
						if(pnr->idx!=id) continue;
						*it=1; //mark as listed
						if(len>=80) {
							outs("\n;");
							outlines++;
							len=3;
						}
						ASSERT(pnr->idx!=idx);
						get_name8(nambuf0, (LPSTR)(LPCSTR)pnr->nam); //No bad names at this stage
						len+=fprintf(fpto, "  %s", nambuf0);
						nchk--;
					}
					outln(); //through with this file
				}
			}
			ASSERT(!nchk);
		}

		if(!pgeo->vFix.empty()) {
			//add fixed points to 1st vector of equivalence set to test zero-variance vectors and equivalences --
			if(pgeo->vLink.empty()) vEquiv.clear();
			if(vEquiv.empty()) vEquiv.push_back(VNAMREC());
			VNAMREC &vFixEquiv=vEquiv[0];
			for(it_fix itf=pgeo->vFix.begin(); itf!=pgeo->vFix.end(); itf++) {
				if(!find_namrec(vFixEquiv, itf->nr)) {
					if(idx!=itf->nr.idx && vEquiv.size()>1) {
						//This is a linked station not previously fixed, but it may be in another eqivalence set!
						//If so, merge this set with the first one (avoiding dups) and continue to next fixed pt --
						it_vnamrec itve=vEquiv.begin()+1;
						for(; itve!=vEquiv.end(); itve++) {
							if(find_namrec(*itve, itf->nr)) break;
						}
						if(itve!=vEquiv.end()) {
							for(it_namrec it=itve->begin(); it!=itve->end(); it++) {
								if(!find_namrec(vFixEquiv, *it))
									vFixEquiv.push_back(*it);
							}
							vEquiv.erase(itve);
							continue;
						}
					}
				}
				vFixEquiv.push_back(itf->nr);
			}
		}

		if(!pgeo->vFix.empty()) {
			if(bUsePrefix2) {
				outf("\n#Prefix2 %s", pgeo->gPrefix2);
				outlines++;
			}
			outf("\n#Units Meters Order=ENU\t;Datum %s, UTM Zone %d\n"
				"\n;\tNAME\tEAST\tNORTH\tUP\n\n", sDatum[pgeo->gD].wnam, pgeo->gZ);
			outlines+=5;
			for(it_fix it=pgeo->vFix.begin(); it!=pgeo->vFix.end(); it++) {
				outlines++;
				get_name(nambuf0, it->nr, idx);
				outf("#Fix\t%s\t%.2f\t%.2f\t%.2f\n", nambuf0, it->fE, it->fN, it->fU);
			}
		}

		fclose(fpto); fpto=NULL;
		bLinksPending=false;
		goto _first;
	}
	return 0;
}

static int clear_slashes(LPSTR p0)
{
	int len=0;
	bool bsp=false;
	for(LPSTR p=p0; *p; p++) {
		if(*p=='\\'||*p=='/') *p='-';
		else if(isspace(*p)) {
			if(bsp) continue; //clear extra spaces
			bsp=true;
		}
		else bsp=false;
		*p0++=*p;
		len++;
	}
	*p0=0;
	return len;
}

static void out_seg_survey_sfx()
{
    //Segment components are limited to 80 chars and must not have
	//embedded slashes --
	char buf[256];

	clear_slashes(strcpy(buf,srvtitle)); //0 < strlen(srvtitle) <= 12
	outf("#S /%s",buf);
	if(*survey_sfx) {
		strcpy(buf,survey_sfx);
		if(strlen(survey_sfx)<=40) {
			if(clear_slashes(buf)) outf(" - %s",buf);
			outln();
		}
		else if(strlen(buf)<=60) {
			outf("\t;%s", buf);
			outln();
		}
		else {
			outln();
			wrap_comment(buf);
		}
	}
	else outln();
}

static void out_hdr(UINT idx)
{
	GEOREC *pgeo=bMakFile?&vGeo[idx]:NULL;

	if(!bCombine || segcount==1) {
		outf(";%s", bCombine?datbasename:srvtitle);
		if(!bCombine) {
			if(*survey_sfx) {
				if(strlen(survey_sfx)<=40) {
					outf(" - %s", survey_sfx);
					outln();
				}
				else {
					outln();
					wrap_comment(survey_sfx);;
				}
			}
			else {
				outln();
			}
			outf(";Cave or area: %s\n", cavename);
			outlines++;
		}
		else outln();
		outf(";Import from %s dated %s\n", datfullname, get_filetime(ln, vDatName[idx]));
		outlines++;
		if(bUsePrefix2) {
			outf("\n#Prefix2 %s", pgeo->gPrefix2);
			outlines++;
			if(pgeo->gParent!=idx) {
				outf("\t;Connects to surveys/fixed pts in %s", trx_Stpnam(vDatName[pgeo->gParent]));
			}
		}
	}

	outs("\n;==================================================================\n");
	outlines+=2;
	if(bCombine) {
		outf(";%s\n", cavename);
		outlines++;
		out_seg_survey_sfx();
	}

	outf(";Team: %s\n", teamline);
	outlines++;
}

static int copy_hdr(UINT idx)
{
	//returns >0 if error, -1 if no survey headers found
	int e;
	char *p;
	UINT slinecount=0; //survey header line count
	
	e=getline();

	if(e==CFG_ERR_EOF) return -1;

    while(e>=0) {
		 slinecount++;
		 inlines++;
		 if(e>LINLEN-2) {
			 return err_format("Overlong line in survey header");
		 }
		 for(p=ln;isspace((BYTE)*p);p++);

         switch(slinecount) {
         
           case 1:	if(!*p || (*p=='?' && !p[1])) p="Unnamed cave";
					strncpy(cavename,p,sizeof(cavename)-1);   //cave name
					break;

           case 2:  *srvtitle=0;
                    if(!out_survey_fld(p,"NAME")) { //fills srvtitle[]
						return CSS_ERR_DATLINEABORT;
					}
					break;

           case 3: if(!out_survey_fld(p,"DATE")) { //can modify srvtitle[]!
			           return CSS_ERR_DATLINEABORT;
			       }
				   if(!bCombine && fpto) {
					   fclose(fpto);
					   fpto=NULL;
				   }
				   //***************************************************
				   if(!fpto) {
					   if(e=open_fpto(idx)) //opens or updates wpj
						   return e;
				   }
				   //***************************************************
				   bWarn=false; //clear any pending warning to output
				   segcount++;
				   break;

		   case 4: if(!out_survey_fld(p, "TEAM")) {
			           return CSS_ERR_DATLINEABORT;
		           }
			       break;

		   case 5: strncpy(teamline,p,sizeof(teamline)-1);
			       out_hdr(idx); //write survey header up to units
				   break;

		   case 6: if(_memicmp(p,"DECLINATION:",12)) {
			       	 return err_format("Missing \"DECLINATION:\" line");
				   }
				   out_units(p+12);
				   if(bWarn) warn_format(0);
				   outln();
				   break;

		   case 7: break;

		   case	8: out_labels();
				   break;

		   case 9: break;

		   default : if(e) return 0; //10 header lines read
         }
         e=getline();
	}

	if(slinecount && slinecount<10) {
		return err_format("Incomplete survey header");
	}
	return e;
}

static void trimMakLine()
{
	LPSTR p,p0=ln;

	while((p=strchr(p0, '/'))) {
		LPSTR pc=strchr(p+1, '/');
		if(!pc) {
			*p=0;
			break;
		}
		memmove(p,pc+1,strlen(pc));
	}
	while(isspace((BYTE)*p0)) p0++;
	if(p0>ln) memmove(ln, p0, strlen(p0)+1);
	for(p=ln+strlen(ln);p>ln && isspace((BYTE)*(p-1));*--p=0);
}

static int getdatum(LPCSTR perr)
{
	LPSTR p,p0=ln;
	if(!(p=strchr(p0,';')) || *p0++!='&') {
		perr="Datum line missing or badly formatted";
		return -1;
	}
	*p=0;
	int i=0;
	for(; i<NDATUMS; i++) {
		if(!stricmp(p0,sDatum[i].cnam)) break;
	}
	if(i==NDATUMS) {
		vMakWarnMsg.push_back("Datum unrecognized, WGS 1984 assumed");
		ASSERT(NDATUMS==28);
		return WGS84_INDEX;
	}
	return i; 
}

static BOOL getdatafile(LPCSTR &perr, int zone, int datum)
{
	CString s;
	int fixCnt=0,linkCnt=0;
	LPSTR p0=ln+1;
	LPSTR p=strchr(p0,',');

	if(!p) p=strchr(p0,';');
	if(!p) goto _errEOL;
	char c=*p;
	*p++=0;
	s=p0;
	s.Trim();

	if(_access(s, 0)==-1) {
		sprintf(formatstr, "Data file %s not found",(LPCSTR)s);
		perr=formatstr;
		return FALSE;
	}

	vDatName.push_back(s);
	vGeo.push_back(GEOREC(zone,datum,refeast,refnorth,refup,inlines));
	GEOREC &geo=vGeo.back();

	//retreive any fixed points or links --
	while(c!=';') {
		while(!*p) {
			if(getline()==CFG_ERR_EOF) {
				goto _errTrunc;
			}
			inlines++;
			trimMakLine();
			p=ln;
		}
		//p is at "name," or "name[" --
		p0=p; 
		while(*p && *p!=',' && *p!=';' && *p!='[') p++;
		c=*p;
		if(!c) goto _errEOL;
		if(c=='[') {
			FIXREC fr;
			fr.nr.idx=inlines;
			fr.nr.nam.SetString(p0,p-p0);
			fr.nr.nam.Trim();
			p0=p+1;
			if(!(p=strchr(p0, ']'))) {
				goto _errFix;
			}
			*p++=0;
			if(cfg_GetArgv(p0,CFG_PARSE_ALL)!=4)
				goto _errFix;
			fr.fE=atof(cfg_argv[1]);
			fr.fN=atof(cfg_argv[2]);
			fr.fU=atof(cfg_argv[3]);
			p0=cfg_argv[0];
			if(*p0=='f' ||*p0=='F') {
				fr.fE*=0.3048;
				fr.fN*=0.3048;
				fr.fU*=0.3048;
			}
			if(!refzone) {
				refzone=zone;
				refdatum=datum;
				geo.gE=refeast=fr.fE;
				geo.gN=refnorth=fr.fN;
				geo.gU=refup=fr.fU;
			}
			if(find_fixnam(geo.vFix, fr.nr.nam)) {
				sprintf(formatstr,"Duplicate fix name (%s) defined for %s",(LPCSTR)fr.nr.nam,(LPCSTR)vDatName.back());
				perr=formatstr;
				return FALSE;
			}
			geo.vFix.push_back(fr);
			fixCnt++;
		}
		else if(p>p0) {
			s.SetString(p0,p-p0);
			s.Trim();
			if(vGeo.size()>1) { //ignore links if 1st dat file
			    //also ignore dublicate links
				if(find_linknam(geo.vLink,s)<0) {
					int lastidx=vGeo.size()-2;
					int i=find_linknam(vGeo[lastidx].vLink,s);
					if(i<0) i=lastidx;
					geo.vLink.push_back(NAMREC(i,s));
					linkCnt++;
				}
			}
		}
		c=*p++; //c==char following link name or ']'
	}

	if(fixCnt) {
		//Store the correct indexes for the fixed points (possibly links) --
		for(it_fix itf=geo.vFix.begin();itf!=geo.vFix.end(); itf++) {
			int i=find_linknam(geo.vLink,itf->nr.nam);
			if(i<0) i=vGeo.size()-1;
			else {
				//error if also fix in parent file
				if(find_fixnam(vGeo[i].vFix, itf->nr.nam)) {
					sprintf(formatstr,"Location of linked station, %s, already specified",(LPCSTR)itf->nr.nam);
					perr=formatstr;
					return FALSE;
				}
			}
			itf->nr.idx=i;
		}
	}

	if((fixCnt || linkCnt) && vGeo.size()>1) {
		//If not the first file in the MAK, any fixes or links will require use
		//of 2nd-level prefixes--
		bUsePrefix2=true;
    }

	fixtotal+=fixCnt;
	linktotal+=linkCnt;

	return TRUE;

_errFix:
	perr="Fixed point format error";
	return FALSE;
_errTrunc:
	perr="MAK file ended prematurely";
	return FALSE;
_errEOL:
	perr="Comma or semicolon expected prior to end of line";
	return FALSE;
}

static int process_mak()
{
	LPCSTR perr=NULL;
	inlines=0;
	refdatum=-1;
	refzone=0;
	LPSTR p;
	int zone=0,datum=WGS84_INDEX;

	while(getline()!=CFG_ERR_EOF) {
		inlines++;
		//strip comments
		trimMakLine();
		if(!*ln) continue;
		p=ln;

		if(*p=='@') {
			if(cfg_GetArgv(++p, CFG_PARSE_ALL)<4 || !(refeast=atof(cfg_argv[0])) ||
				!(refnorth=atof(cfg_argv[1])) || !(refzone=atoi(cfg_argv[3])) || abs(refzone)>60) {
				refzone=0; //warn of bad header line, obtain ref from first file with a fix
			}
			else {
				refup=atof(cfg_argv[2]);
			}
			if(getline()==CFG_ERR_EOF) {
				perr="Unexpected end of file";
				goto _err;
			}
			++inlines;
			if((refdatum=getdatum(perr))<0) goto _err;
			zone=refzone;
			datum=refdatum;
			continue;
		}

		if(*p=='$') {
			if(!(zone=atoi(p+1)) || abs(zone)>60) {
				perr="UTM zone out of range";
				goto _err;
			}
			continue;
		}

		if(*p=='&') {
			if((datum=getdatum(perr))<0) goto _err;
			continue;
		}

		if(*p=='#') {
			if(!getdatafile(perr,zone,datum))
			 goto _err;
		}

		if(*p=='!' && p[2]=='T' && p[1]=='O')
			bDefLrudTo=true;
	}

	fclose(fp);
	return 0;

_err:
	strcpy(ln,perr);
	fclose(fp);
	return CSS_ERR_MAKLINEABORT;
}

static int InitMFC()
{
	int nRetCode = 0;
	HMODULE hModule = ::GetModuleHandle(NULL);
	if (hModule != NULL)
	{
		// initialize MFC and print and error on failure
		if (!AfxWinInit(hModule, NULL, ::GetCommandLine(), 0))
		{
			// TODO: change error code to suit your needs
			_tprintf(_T("Fatal Error: MFC initialization failed\n"));
			nRetCode = 1;
		}
	}
	else
	{
		// TODO: change error code to suit your needs
		_tprintf(_T("Fatal Error: GetModuleHandle failed\n"));
		nRetCode = 1;
	}
	return nRetCode;
}

static void get_namrec_idx(NAMREC &nr)
{
	int e=find_linknam(vGeo[nr.idx].vLink,nr.nam);
	if(e>=0) {
		ASSERT(e<nr.idx);
		nr.idx=e;
	}
}

static int process_dat(UINT idx)
{
	int e=0;
	LPCSTR pPath=vDatName[idx];
	GEOREC *pgeo=bMakFile?&vGeo[idx]:NULL;

	pPath=dos_FullPath(pPath, 0);
	strcpy(datpath, pPath?pPath:vDatName[idx]);

	if(!pPath || !(fp=fopen(datpath,"r"))) {
		return CSS_ERR_DATOPEN;
	}

	strncpy(datfullname, trx_Stpnam(pPath),79);
	strcpy(datbasename,datfullname);
	*trx_Stpext(datbasename)=0;

	sprintf(bookname, "%s#%03d", bookpfx, idx);
	srvidx=0;

	fpto=NULL;
	bFileHasIncA=bFileHasIncAB=bFileHasIncV=bFileHasIncVB=bFileHasLrudTo=bFileHasCorrections=false;
	inlines=segcount=veccount=outlines=warncount=0;
	lensurvey=0.0;

	bHasLinksOrFixedPts = pgeo && (!pgeo->vFix.empty() || !pgeo->vLink.empty());
	bLinksPending = bHasLinksOrFixedPts;

	//process first survey
	e=copy_hdr(idx); //updates project file (fppr), creating it if necessary
	//e>0 (code) if error, -1 if no survey headers found at all

	if(e) {
	  fclose(fp); fp=NULL; //finished with DAT
	  if(fpto) {
		  //may have written fixed points!
		  fclose(fpto); fpto=NULL;
	  }
	  return (e<0)?0:e;
    } 

    bool bFixed,bExclude,bFloated;
	BOOL bComm;
	CString csFlags;

	filecount++;

	do {
		//process next vector --
		if(*ln==0x0C) {
			//process next survey, if any, in file --
			if(!bCombine) bFileHasCorrections=false; //no need for zero corrections on #units
			e=copy_hdr(idx); //returns >0 if error, -1 if no *more* survey headers fouund
			if(e) {
			   if(e<0) break;
			   if(fpto) {
				   fclose(fpto); fpto=NULL;
			   }
			   return e;
			}
		}

		cfg_GetArgv(ln,CFG_PARSE_ALL);
		if(!cfg_argc) continue; /*Ignore blank data lines*/
       
		bFixed=bWarn=bExclude=bFloated=false;
		bComm=FALSE;
		csFlags.Empty();

		int i=bRedundant?11:9;
		if(cfg_argc<i) {
			warn_format("Less than %u line items, \"%s %s...\", not used", i, cfg_argv[0], (cfg_argc>1)?cfg_argv[1]:"");
			warn_format(0);
			continue;
		}

		if(!IsNumeric(cfg_argv[2])) {
			outf(";%s",cfg_argv[0]);
			for(i=1; i<cfg_argc; i++) outf(" %s",cfg_argv[i]);
			outln();
			warn_format("3rd line item is not numeric, shot commented out");
			warn_format(0);
			continue;
		}

		if(i<cfg_argc) {
			LPCSTR p=cfg_argv[i];
			if(*p=='#' && *++p=='|') {
				CString flgs(p+1);
				int e0=i;
				while((i=flgs.ReverseFind('#'))<0 && e0+1<cfg_argc) flgs+=cfg_argv[++e0]; //allow for spaces in #|...#
				if(i>=0) {
					flgs.Truncate(i);
					flgs.MakeUpper();
					for(p=(LPCSTR)flgs;*p;p++) {
						char c=*p;
						switch(c) {
							case 'C': bFixed=true;
								      break;
 							case 'X': bExclude=true; /*Show flags spec as part of comment --*/
									  break;
							case '|':
							case '\\':
							case '/':
							case ';': break;
							default: if(csFlags.Find(c)<0 && csFlags.GetLength()<=5) csFlags+=c;
						}
					}
					if(bExclude) {
					   i=e0; //show flags
					}
					else i=e0+1;
				}
				else {
					warn_format("Unterminated flag string ignored");
					i=e0;
				}
			}
			if(i<cfg_argc) bComm=out_comment(i);
			//bComm is TRUE if comment only parsed and saved
		}
       
		NAMREC nr0(idx,cfg_argv[0]),nr1(idx,cfg_argv[1]);
		if(bUsePrefix2) {
			get_namrec_idx(nr0);
			get_namrec_idx(nr1);
		}

		bLrudOnly=nr0==nr1;
		if(!bExclude) {
			double d=atof(cfg_argv[2]);
			if(bLrudOnly) {
				if(d>0.0) {
					warn_format("Self loop with length %.2f commented out", d);
					bExclude=true;
				}
				else if(!out_dimensions(0)) {
					warn_format("Self loop with empty LRUD commented out");
					bExclude=true;
				}
			}
			else if(d==0.0 || bFixed) {
				i=chk_equiv(nr0,nr1);
				if(i>=0) {
					bFixed=false;
					if(d==0.0) {
						warn_format("Redundant or inconsistent station equivalence commented out");
						bExclude=true;
					}
					else {
						warn_format("Shot floated to avoid non-adjustable loop of fixed vectors");
						bFloated=true;
					}
				}
			}
		}

		if(bExclude) outs(";");
		else veccount++;

		outs(get_name(nambuf0, nr0, idx));

		if(!bLrudOnly) {
			outtab();
			outs(get_name(nambuf1, nr1, idx));
			for(i=0;i<3;i++) {
				outtab();
				out_item(i);
			}
			if(bFixed) outs("\t(0)");
			else if(bFloated) outs("\t(?)");
		}
		out_dimensions(1);
       
		if(!csFlags.IsEmpty()) out_flagstr(csFlags);
       
		if(bComm) out_comment(0);
		outln();
		if(bWarn) warn_format(0);
	}
	while ((e=getline())>=0 && ++inlines);

	if(fpto) {
		//close SRV
		fclose(fpto); fpto=NULL;
	}
	if(e<=0) {
		ASSERT(fppr);
		if(fppr) {
			fprintf(fppr, ".ENDBOOK\n");
		}
	}
	lentotal+=lensurvey;
	segtotal+=segcount;
	vectotal+=veccount;
	warntotal+=warncount;
	if(fp) {
	    //close DAT
		fclose(fp); fp=NULL;
	}
	return 0;
}

DLLExportC int PASCAL ErrMsg(LPCSTR *pPath, LPCSTR *pMsg, int code)
{
	int len=0;

	if(!code) {
		errMsg.Format("Created Walls project: %s.wpj --\n\n"
			"Data files: %u  Surveys: %u  Fixed pts: %u  Vectors: %u  Length: %.0f ft",
			wpjname, filecount, segtotal, fixtotal, vectotal, lentotal);
		if(repflags) {
			CString s("\n\nSome replacements were required in names");
			char cpfx=':';
		    for(LPCSTR p=pchrs_repl;*p;p++) {
				if(repflags&(1<<(p-pchrs_repl))) {
				   s.AppendFormat("%c '%c' for '%c'",cpfx,*p,pchrs_torepl[p-pchrs_repl]);
				   cpfx=',';
				}
			}
			errMsg+=s;
		};

		if(bUsePrefix2) errMsg+="\n\nFixed pts and/or links required 2nd-level prefixes to be assigned.";


		if(bLongNames) {
			errMsg+="\n\nSome names of length >8 chars were converted to prefixed names.";
			if(bColonsAccepted) errMsg+="\nOther ";
		}
		else {
			if(!repflags) {
				errMsg+="\n\nNo station name modifications were required.";
			}
			if(bColonsAccepted) {
				if(repflags) errMsg+="\n";
				errMsg+="\nSome ";
			}
		}

		if(bColonsAccepted)	errMsg+="names had a colon accepted as a prefix delimiter.";

		if(revtotal) errMsg.AppendFormat("\n\n%u backsights with signs obvously reversed were corrected on import.", revtotal);

		if(warntotal)
		  errMsg.AppendFormat("\n\n%u caution message%s generated. Search data files for %s containing \"***Caution.\"",
			  warntotal, (warntotal>1)?"s were":" was", (warntotal>1)?"lines": "a line");
	}
	else {
	    errMsg="Import aborted --\n\n";
		switch(code) {

			case CSS_ERR_MAKOPEN:
			case CSS_ERR_DATOPEN:
			case CSS_ERR_WRITE:
			case CSS_ERR_CREATE:
			case CSS_ERR_DIRCREATE:
			     {
					 LPCSTR pfile=trx_Stpnam((code>CSS_ERR_DATOPEN)?prpath:((code=CSS_ERR_DATOPEN)?datpath:frpath));
					 errMsg.AppendFormat("%s: %s.", (LPSTR)errstr[code-1], pfile);
				 }
				 break;

			case CSS_ERR_NODATFILES: 
				     if(bMakFile) errMsg.AppendFormat("No data files specified in %s.",trx_Stpnam(makpath));
					 else errMsg.AppendFormat(errstr[code-1]);
				 break;

		    case CSS_ERR_DATLINEABORT:
			case CSS_ERR_MAKLINEABORT:
				*pPath=(code==CSS_ERR_DATLINEABORT)?datpath:frpath;
				errMsg.AppendFormat("File %s, Line %u: %s.", trx_Stpnam(*pPath), inlines, ln);
				len=inlines;
				break;

			case CSS_ERR_VERSION:
				errMsg.AppendFormat("Wrong DLL version: wallcss.dll v.%.2f (v.%.2f required).",
					DLL_VERSION,pcss_params->version);
			    break;
		}
	}

	*pMsg=(LPCSTR)errMsg;
	return len;
}

static void fix_bookpfx()
{
	LPSTR p=bookpfx;

	for(LPCSTR ppj=wpjname; *ppj && *ppj!='.'; ppj++) {
		*p++=(*ppj==' ')?'_':*ppj;
		if(p-bookpfx==4) break;
	}
	while(p<bookpfx+4) *p++='_';
	*p=0;
	_strupr(bookpfx);
}

DLLExportC int PASCAL Import(LPSTR lpDatPath, LPSTR lpPrjPath, LPFN_IMPORT_CB pCB)
{
	//if(InitMFC()) return 1; //fail
	int e=0;

	if(!pCB(CSS_GETPARAMS, &pcss_params) || pcss_params->version!=DLL_VERSION)
		return CSS_ERR_VERSION;

	bAllowColons=(pcss_params->flags&CSS_ALLOWCOLONS)!=0;
	bCombine=(pcss_params->flags&CSS_COMBINE)!=0;
	bAttachSrc=(pcss_params->flags&CSS_ATTACHSRC)!=0;
	pchrs_torepl=(LPCSTR)&pcss_params->chrs_torepl;
	pchrs_repl=(LPCSTR)&pcss_params->chrs_repl;
	repflags=0;
	bColonsAccepted=bLongNames=bDefLrudTo=false;

	char *p=dos_FullPath(lpPrjPath,".wpj");

	strcpy(prpath, p?p:lpPrjPath);
	strcpy(wpjname,p=trx_Stpnam(prpath));
	*trx_Stpext(wpjname)=0;

	if(p!=prpath && !DirCheck(prpath)) {
		*p=0;
		return CSS_ERR_DIRCREATE;
	}

	fix_bookpfx();

	bUsePrefix2=false;
	fixtotal=linktotal=0;
	fppr=NULL; //wpj file

	/*cfg_argv() will recognize no comment delimeters, quoted strings, etc.*/
	cfg_commchr=cfg_quotes=cfg_equals=0;
 
	ssDepthBuf[SSBUFLEN-1]=0;
	vDatName.clear();

 	p=trx_Stpext(strcpy(frpath, lpDatPath));

	bMakFile=!_stricmp(p,".mak");

	if(bMakFile) {
		p=dos_FullPath(frpath,0);

		if(!p || (fp=fopen(strcpy(makpath,p),"r"))==NULL) {
			return CSS_ERR_MAKOPEN;
		}

		if(!MakeFileDirectoryCurrent(makpath)) {
			return CSS_ERR_MAKOPEN;
    	}

		if(e=process_mak()) {
			goto _finish;
		}
		get_filetime(makdate, makpath);
		strncpy(makname,trx_Stpnam(makpath),sizeof(makname)-1);
	}
	else {
		ASSERT(!stricmp(p,".dat"));
		struct _finddata_t ft;
		long hFile;

		if((hFile=_findfirst(frpath, &ft))==-1L)
			return CSS_ERR_NODATFILES;

		p=trx_Stpnam(frpath);

		while(TRUE) {
			if(!(ft.attrib&(_A_HIDDEN|_A_SUBDIR|_A_SYSTEM))) {
				strcpy(p,ft.name);
				CString s(frpath);
				for(it_cstr it=vDatName.begin(); it!=vDatName.end(); it++) {
					if(!s.CompareNoCase(*it)) goto _next;
				}
				vDatName.push_back(s);
			}
		_next:
			if(_findnext(hFile, &ft)) break;
		}
		_findclose(hFile);
	}

	cfg_ignorecommas=1;

	filecount=segtotal=vectotal=warntotal=revtotal=0;
	lentotal=0.0;
	vMakWarnMsg.clear();
	vWarnMsg.clear();
	bWarn=false;

	ASSERT(!bMakFile || vDatName.size()==vGeo.size());

	for(UINT idx=0; idx<vDatName.size();idx++) {
		if(e=process_dat(idx)) goto _finish;
	}

	if(!filecount) {
		e=CSS_ERR_NODATFILES;
		goto _finish;
	}
	if(fppr) {
		fprintf(fppr,".ENDBOOK\n");
		fclose(fppr);
	}

_finish:
	return e;
}

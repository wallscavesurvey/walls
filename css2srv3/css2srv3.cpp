// css2srv3.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "css2srv3.h"
#include <georef.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// The one and only application object
CWinApp theApp;
using namespace std;

#define LINLEN 512

#define SIZ_SURVEY _MAX_PATH
#define SIZ_WARNMSG 120
#define SIZ_NAMBUF 80

static FILE *fp,*fpto,*fppr;
static double degForward;
static char ln[LINLEN],formatstr[LINLEN];
static char frpath[_MAX_PATH],prpath[_MAX_PATH];
static char nambuf0[SIZ_NAMBUF],nambuf1[SIZ_NAMBUF];
static char *prname;
static char survey[SIZ_SURVEY],srvname[16];
static char datname[SIZ_SURVEY],prjname[SIZ_SURVEY];
static char D_units[16],A_units[16],V_units[16],S_units[16];
static char item_typ[8];
static int	item_idx[8];
static char datestr[16];
static UINT outlines,inlines,warncount,warntotal,revtotal;
static bool bRedundant,bLrudOnly,bForwardOK;
static bool A_bQuad,D_bInches,S_bInches,V_bDepth,V_bMinutes,L_bTo;
static bool bMakFile,bWarn,bUsePrefix2;

#define PI 3.141592653589793
#define U_DEGREES (PI/180.0)

#define SSBUFLEN 32
static char ssDepthBuf[SSBUFLEN];

static int nameinc=0;
static UINT filecount,segcount,segtotal,veccount,vectotal,fixtotal,linktotal;
static double lentotal,lensurvey;
static BOOL bQuiet;

static bool bProject;

static VEC_CSTR vDatName,vWarnMsg;
static double refeast,refnorth,refup;
static int refdatum,refzone,makln;

struct DATPFX
{
	DATPFX(LPCSTR p) : cnt(-1) 
	{
		memcpy(pfx, p, 5);
		pfx[5]='#'; pfx[6]=0;
	}
	LPCSTR getname(LPSTR buf)
	{
		if(cnt==99) { //should be 99
			if(++pfx[5]=='\'') return NULL;
			cnt=-1;
		}
		sprintf(buf,"%s%02u",pfx,++cnt);
		return buf;
	}
	char pfx[8];
	int cnt;
};

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
typedef std::vector<DATPFX> VDATPFX;
typedef VDATPFX::iterator it_datpfx;

struct GEOREC
{
	char gPrefix2[16];
	int gParent;
	int gZ;
	int gD;
	double gE,gN,gU;
	VFIX vFix;
	VNAMREC vLink;
	GEOREC(int refZ,int refD,double refE,double refN,double refU) :
	gZ(refZ), gD(refD), gE(refE), gN(refN), gU(refU) {}
};

typedef std::vector<GEOREC> VGEO;
typedef VGEO::iterator it_geo;

static VDATPFX vDatpfx;
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
	int len=0;

	for(;*nam;len++,nam++) {
		switch(*nam) {
			case ':' : *nam='|'; break;
			case '#' : *nam='~'; break;
			case ';' : *nam='^'; break;
			case ',' : *nam='`'; break;
		}
	}
	return len;
}

static LPCSTR get_name8(char *name8,LPCSTR p)
{
	int len=filter_name((LPSTR)p)-8;

	if(len>0) {
	   memcpy(name8,p,len);
	   p+=len;
	   name8[len++]=':';
	}
	else len=0;
	strcpy(name8+len,p);
	return name8;
}

static LPCSTR get_name(char *nambuf,const NAMREC &nr,int idx)
{
	int len=0;
	if(nr.idx!=idx) {
		strcpy(nambuf, vGeo[nr.idx].gPrefix2);
		len=strlen(nambuf);
		nambuf[len++]=':';
		if(strlen(nr.nam)<=8) nambuf[len++]=':';
	}
	get_name8(nambuf+len,nr.nam);
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


static void err_format(char *msg)
{
  printf("\nAborted: %s.dat, line: %d - %s.\n",datname,inlines,msg);
} 

static void _cdecl warn_format(LPCSTR fmt,...)
{
	if(!fmt) {
		//flush any displayed warning to srv file
		if(!vWarnMsg.empty()) {
			for(it_cstr it=vWarnMsg.begin(); it!=vWarnMsg.end();it++) {
				outf(";*** Caution: %s.dat:%s.\n",datname,(LPCSTR)*it);
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
	if(!warncount) {
		//DLL: if(!warntotal) open fplog;
		//printf("\nFile %s.dat Notes --\n",datname);
	}
	warncount++;
	if(bQuiet<=5) {
		int i=6-s.Find(' ');
		if(i>0) {
			buf[i]=0;
			while(i--) buf[i]=' ';
		}
		else *buf=0;
		printf("*** Ln%s%s%s.\n",buf,(LPCSTR)s,(bQuiet<5)?"":" (...)");
	}
	bWarn=true; //msg displayed but not yet written to srv
} 

static void outtab(void)
{
	outs("\t");
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
	/*
	if(bOut && inlines==433) {
		int jj=0;
	}
	*/
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
	  if(*typ=='N') return FALSE;
	  sprintf(ln,"\"SURVEY %s\" line expected",typ);
	  err_format(ln);
	}

	if(*typ=='T') return TRUE;

	p+=11;
	if(*p==':') p++;
	while(isspace((BYTE)*p)) p++;

	if(*typ=='N') {
		if(!*p) err_format("\"SURVEY NAME:\" has no argument");
		strncpy(survey,p,SIZ_SURVEY-1);
	}
	else if(*typ=='D') {
		outf("\n;==================================================================\n"
			 ";%s\n",formatstr);
		outlines+=3;
		if(typ=strstr(p,"COMMENT:")) {
			*typ=0;
			typ+=8;
			while(isspace((BYTE)*typ)) typ++;
		}
		outf("#Segment /%s",survey);
		if(typ && *typ) {
			/*Don't output excessively long name string --*/
			if(strlen(typ)<=40) outf("  ;%s",typ);
			else {
			   outlines++;
			   outf("\n;%s",typ);
			}
		}
		outln();
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
		return TRUE;
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

	A_bQuad=D_bInches=S_bInches=V_bDepth=L_bTo=false;
	bRedundant=false;

	if(i>=cfg_argc || (lenf=strlen(pf=_strupr(cfg_argv[i])))<11) {
		err_format("FORMAT argument missing");
		return FALSE;
	}

	bRedundant=pf[11]=='B';
	if(lenf>12 && pf[12]=='T') L_bTo=true;

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

	outs("#Units");
	out_correct(pIncA,"IncA");
	out_correct(pIncV,"IncV");
	out_correct(pIncD,"IncD");
	if(bRedundant) {
		if(pIncAB) out_correct(pIncA,"IncAB");
		if(pIncVB) out_correct(pIncV,"IncVB");
	}
	if(L_bTo) outs(" LRUD=T");
	outln();

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

static void warn_backsight(double dnew,double dold,LPCSTR ptyp)
{
	if(revtotal>=5) {
		bQuiet=revtotal;
	}
	warn_format("Reversed %s BS. Using %.2f/%.2f, not %.2f/%.2f",
	  ptyp, degForward, dnew, degForward, dold);
	bQuiet=0;
	revtotal++;
}

static void out_measurement(char *p,int c,BOOL bBack)
{
	double d,w;
	char buf[40];
	bool bInches,bOutofRange=false;
	int units;

	p=CheckNumeric(p);

	switch(c) {
	case 'S' :
	case 'D' :
		if(*p=='-') {
			warn_format("Negative distance, \"%s\", made positive", p);
			p++;
		}
		if(c=='S') {
			//LRUD
			bInches=S_bInches;
			units=*S_units;
		}
		else {
			bInches=D_bInches;
			units=*D_units;
		}
		d=atof(p);
		lensurvey+=d;
		if(units=='M') {
			sprintf(buf,"%.2f",d*0.3048);
			out_trimmed_numeric(buf);
			break;
		}
		if(bInches) {
			out_inches(d);
			break;
		}
		//distance in feet
		out_trimmed_numeric(p);
		if(*S_units!=*D_units) outf("%c",tolower(units));
		break;

	case 'A' :
		d=atof(p);
		if(d<0.0 || d>=360.0) {
			if(d==-999.0) {
				if(bBack) {
					if(bForwardOK) break;
				}
				else if(bRedundant) {
					bForwardOK=false;
					break;
				}
			}
			else if(bBack && bForwardOK) {
				warn_format("Back azimuth (%s deg) out of range and not used", p);
				break;
			}
			d=0.0;
			if(atof(p)!=360.0) warn_format("Azimuth (%s deg) out of range, replaced with 0", p);
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
					df=d;
					d+=180;
					if(d>=360) d-=360;
					warn_backsight(d,df,"az");
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
			sprintf(buf,"%.2f",d);
			out_trimmed_numeric(buf);
			outf("%c",units);
			break;
		}

		if(*A_units=='G') d*=(10.0/9.0);
		sprintf(buf,"%.2f",d);
		out_trimmed_numeric(buf);
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
				warn_format("Back inclination (%s deg) out of range and not used", p);
				break;
			}
			d=0;
			warn_format("Inclination (%s deg) out of range, replaced with 0", p);
		}
		else if(bRedundant) {
			if(!bBack) {
				bForwardOK=true;
				degForward=d;
			}
			else if(bForwardOK) {
				if((d>=0)==(degForward>=0)) {
					double da=abs(d), fa=abs(degForward);
					if(min(da,fa)>5 && abs(da-fa)/max(da,fa)<=0.2) {
					  warn_backsight(-d, d, "vt");
					  d=-d;
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
			sprintf(buf,"%.1f",d*60);
			out_trimmed_numeric(buf);
			break;
		}
		sprintf(buf,"%.2f",d);
		out_trimmed_numeric(buf);
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
		else if(!bForwardOK) outs("--");
	}
}

static void out_flagstr(LPCSTR pF)
{
	outf("\t#S %c",*pF++);
	while (*pF) outf("/%c",*pF++);
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
		   case 'D' : if(V_bDepth && e==2) outs("\tFR_Depth-TO_Depth");
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

static BOOL open_fpto(UINT idx)
{
	char *path;
	GEOREC *pgeo=bMakFile?&vGeo[idx]:NULL;
	FIXREC *pfix=pgeo?(pgeo->vFix.empty()?NULL:&pgeo->vFix[0]):NULL;

	if(bProject) {
		if(!fppr) {
			if((fppr=fopen(prpath,"w"))==NULL) {
				printf("Error creating project %s.\n",prpath);
				return FALSE;
			}
			fprintf(fppr,";WALLS Project File\n"
				".BOOK\t%s Project\n"
				".NAME\tPROJECT\n"
				".OPTIONS\tflag=Fixed\n"
				".STATUS\t%u\n",prjname,refzone?1051139:1048579);

			if(refzone) {
				ASSERT(bMakFile);
				outRefSpec(refdatum,refzone,refeast,refnorth,refup);
			}
		}

		int status=8; //2568 (own ref), 1352 (unspecified), or 8 (all inherited)

		if(bMakFile) {
			if(pgeo->gZ) {
				if(pgeo->gD!=refdatum || pgeo->gZ!=refzone) {
					status=pfix?2568:1352;
				}
			}
			else if(refzone) status=1352;
		}

		fprintf(fppr,".SURVEY\t%s\n"
			         ".NAME\t%s\n"
					 ".STATUS\t%u\n",datname,srvname,status);

		if(status==2568) {
			outRefSpec(pgeo->gD,pgeo->gZ,pfix->fE,pfix->fN,pfix->fU);
		}
		path=prpath;
	}
	else path=frpath;

	strcpy(trx_Stpnam(path),srvname);
	strcpy(trx_Stpext(path),".srv");

	if(bUsePrefix2) {
		ASSERT(bMakFile);
		if(idx && pgeo->vLink.empty() && pgeo->vFix.empty()) {
			strcpy(pgeo->gPrefix2,vGeo[idx-1].gPrefix2);
			pgeo->gParent=vGeo[idx-1].gParent;
		}
		else {
			strcpy(pgeo->gPrefix2, srvname);
			pgeo->gParent=idx;
		}
	}

	if((fpto=fopen(path,"w"))==NULL) {
		printf("\nAborted - Error creating %s.\n",path);
		return FALSE;
	}
	printf("\nProcessing: %s.dat -> %s.srv\n",datname,srvname);

	outf(";%s\n;Source Compass file dated %s\n",datname,get_filetime(ln,vDatName[idx]));
	outlines=2;
	if(bUsePrefix2) {
		outf("#Prefix2 %s",pgeo->gPrefix2);
		if(pgeo->gParent!=idx) {
			outf("  ;Connects to surveys/fixed pts in %s",trx_Stpnam(vDatName[pgeo->gParent]));
		}
		outln();
	}

	if(!bMakFile) return TRUE;

	if(!pgeo->vFix.empty()) {
		outlines+=3;
		outf("\n;Datum %s, UTM Zone %d\n#Units Meters Order=ENU\n",sDatum[pgeo->gD].cnam,pgeo->gZ);
		for(it_fix it=pgeo->vFix.begin();it!=pgeo->vFix.end();it++) {
			outlines++;
			outf("#Fix\t%s\t%.2f\t%.2f\t%.2f\n", get_name(nambuf0,it->nr,idx),it->fE,it->fN,it->fU);
		}
	}

	if(!pgeo->vLink.empty()) {
		outs("\n;Links to other files --\n");
		outlines+=2;
		int nchk=pgeo->vLink.size();
		VEC_BYTE vchk(nchk,0);

		for(it_byte itc=vchk.begin();itc!=vchk.end();itc++) {
			if(!*itc) {
				NAMREC *pnr=&pgeo->vLink[itc-vchk.begin()];
				int id=pnr->idx; //look for links to this file
				int len=fprintf(fpto, ";%s:", trx_Stpnam(vDatName[id]));
				for(it_byte it=itc;it!=vchk.end();it++,pnr++) {
					if(pnr->idx!=id) continue;
					*it=1; //mark as listed
					if(len>=80) {
						outs("\n;");
						outlines++;
						len=3;
					}
					ASSERT(pnr->idx!=idx);
					len+=fprintf(fpto,"  %s",get_name8(nambuf0,pnr->nam));
					nchk--;
				}
				outln(); //through with this file
			}
		}
		ASSERT(!nchk);
	}
	return TRUE;
}

static int copy_hdr(UINT idx)
{
    //rtn -1 if error, 0 if no more surveys in this file
	int e;
	char *p;
	int max_team_lines=10;
	UINT slinecount=0; //survey line count

	while(!(e=getline())) inlines++;

	if(e==CFG_ERR_EOF) return 0;

    while(e>=0) {
		 slinecount++;
		 inlines++;
		 if(e>LINLEN-2) {
			 if(slinecount>2) err_format("Overlong line in survey header");
			 return -1;
		 }
		 for(p=ln;isspace((BYTE)*p);p++);

         switch(slinecount) {
         
           case 1:	if(!*p) p="Untitled Survey";
					strcpy(formatstr,p);
					break;

           case 2:  *survey=0;
                    if(!out_survey_fld(p,"NAME")) return 0;
					if(!fpto && !open_fpto(idx)) //opens or updates wpj
					   return -1; 
					bWarn=false; //clear any pending warning to output
					break;

           case 3: out_survey_fld(p,"DATE");
			       break;
           case 4: out_survey_fld(p,"TEAM");
			       break;
		   case 5: if(*p) {
			         outf(";Team: %s\n",p);
			         outlines++;
				   }
				   break;

		   case 6: if(_memicmp(p,"DECLINATION:",12)) {
			          if(!max_team_lines--) {
						  err_format("Missing \"DECLINATION:\" line");
						  return -1;
					  }
					  if(*p) {
						outf(";      %s\n",p);
					    outlines++;
					  }
				      slinecount--;
					  e=getline();
					  continue;
				   }
				   out_units(p+12);
				   if(bWarn) warn_format(0);
				   outln();
				   break;

		   case 7: break;

		   case	8: out_labels();
				   break;

		   case 9: break;

		   default : if(e) return e; //10 header lines read
         }
         e=getline();
	}

	if(slinecount && slinecount<10) {
		err_format("Incomplete survey header");
		e=-1;
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

void mak_warning(LPCSTR perr)
{
	printf("Caution: MAK file line %u - %s",makln,perr);
}

int getdatum(LPCSTR perr)
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
		mak_warning("Datum unrecognized, WGS 1984 assumed");
		ASSERT(NDATUMS==28);
		return WGS84_INDEX;
	}
	return i; 
}

#ifdef _DEBUG
static void show_equiv(BOOL bStart)
{
	printf("\n%s =================================================== %s.%s\n",
		bStart?"FIXED POINTS":"EQUIVALENCES",bStart?prjname:datname,bStart?"mak":"dat");

	if(vEquiv.empty()) printf("<NONE>");
	else {
	  for(it_vnamrec itv=vEquiv.begin(); itv!=vEquiv.end(); itv++) {
	    int cnt=0;
		for(it_namrec itn=itv->begin(); itn!=itv->end(); itn++, cnt++) {
			if(cnt==10) {
				printf("\n");
				cnt=0;
			}
			printf(" %u:%s",itn->idx,(LPCSTR)itn->nam);
		}
		if(bStart || itv+1==vEquiv.end()) break;
		printf("\n------------------------------------\n");
	  }
	}
	printf("\n================================================================\n");
}
#endif

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

	vDatName.push_back(s);
	vGeo.push_back(GEOREC(zone,datum,refeast,refnorth,refup));
	GEOREC &geo=vGeo.back();

	//retreive any fixed points or links --
	while(c!=';') {
		while(!*p) {
			if(getline()==CFG_ERR_EOF) {
				goto _errTrunc;
			}
			makln++;
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
			fr.nr.idx=makln;
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

	if(fixCnt || linkCnt) bUsePrefix2=true;

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

static BOOL process_mak()
{
	LPCSTR perr=NULL;
	makln=0;
	refdatum=-1;
	refzone=0;
	LPSTR p;
	int zone=0,datum=WGS84_INDEX;

	while(getline()!=CFG_ERR_EOF) {
		makln++;
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
			++makln;
			if((refdatum=getdatum(perr))<0) goto _err;
			zone=refzone;
			datum=refdatum;
			continue;
		}

		if(*p=='$') {
			if(!(zone=atoi(p+1)) || abs(zone)>60) goto _badzone;
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
	}

	fclose(fp);
	return TRUE;

_badzone:
	perr="UTM zone out of range";
_err:
	printf("MAK error, line %u: %s.\n",makln,perr);
	fclose(fp);
	return FALSE;
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

static void print_options()
{
   printf( "\n"
           "Usage: css2srv [-p WPJ_pathname] <DAT_pathname> [DAT_pathname...]\n"
		   "or     css2srv [-p WPJ_pathname] <MAK_pathname>\n"
		   "\n"
           "Creates Walls SRV files from Compass DAT files matching one or\n"
		   "more file specification arguments. If a MAK file is specified,\n"
		   "the DAT file pathnames will be obtained from that file along\n"
		   "with any fixed points, links, and georeferencing info.\n"
		   "\n"
		   "Examples:\n"
		   "\n"
		   "css2srv NBC.MAK (creates NBC.wpj and SRV files in current folder)\n"
		   "css2srv C:\\NBC\\*.DAT (converts all DAT files in NBC folder, no WPJ)\n"
		   "\n"
		   "Use \"-p WPJ_pathname\" to override the destination folder and to\n"
		   "force creation of a WPJ script when a MAK file isn't specified.\n"
		   "\n"
		   "DAT files with base names longer than 8 characters will produce\n"
		   "SRV files with 8-char names containing the same 5-char prefix.\n"
		   "Station names longer than 8 characters are converted to prefixed\n"
		   "names. --DMcK (2015-07-13)\n");
}

static void fix_srvname()
{
	_strupr(srvname);
	for(LPSTR p=srvname; *p; p++) {
		if(isspace((BYTE)*p)) *p='_';
	}
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
	int e;
	LPCSTR pPath=vDatName[idx];
	GEOREC *pgeo=bMakFile?&vGeo[idx]:NULL;

	strcpy(datname,trx_Stpnam(pPath));

	if((fp=fopen(pPath,"r"))==NULL) {
		printf("*** Error: Cannot open %s.\n",pPath);
		return 0;
	}

	*trx_Stpext(datname)=0;
	*srvname=0;
	if(strlen(datname)>8) {
		for(it_datpfx it=vDatpfx.begin();it!=vDatpfx.end();it++) {
			if(!memcmp(it->pfx, datname, 5)) {
				if(!it->getname(srvname)) {
					//Possible only with >500 files with same name prefix!
					printf("*** Error: %s.DAT - Can't generate 8-char SRV file name.\n",datname);
					fclose(fp);
					return 0;
				}
				break;
			}
		}
		if(!*srvname) {
			DATPFX datpfx(datname);
			datpfx.getname(srvname);
			vDatpfx.push_back(datpfx);
		}
	}
	else {   
		strcpy(srvname, datname);
	}
	fix_srvname();

	if(bMakFile) {
		if(!pgeo->vFix.empty()) {
			//add fixed points to 1st vector of equivalence set to test zero-variance vectors and equivalences --
			if(pgeo->vLink.empty()) vEquiv.clear();
			if(vEquiv.empty()) vEquiv.push_back(VNAMREC());
			VNAMREC &vFixEquiv=vEquiv[0];
			for(it_fix itf=pgeo->vFix.begin();itf!=pgeo->vFix.end(); itf++) {
				if(!find_namrec(vFixEquiv,itf->nr)) {
					if(idx!=itf->nr.idx && vEquiv.size()>1) {
						//This is a linked station not previously fixed, but it may be in another eqivalence set!
						//If so, merge this set with the first one (avoiding dups) and continue to next fixed pt --
						it_vnamrec itve=vEquiv.begin()+1;
						for(;itve!=vEquiv.end(); itve++) {
							if(find_namrec(*itve, itf->nr)) break;
						}
						if(itve!=vEquiv.end()) {
							for(it_namrec it=itve->begin(); it!=itve->end(); it++) {
								if(!find_namrec(vFixEquiv,*it))
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
	}

	fpto=NULL;
	inlines=segcount=veccount=outlines=warncount=0;
	lensurvey=0.0;

	e=copy_hdr(idx); //updates project file

	if(e<=0) {
	  //error shown if -1
	  if(!e) err_format("No surveys found in file");
	  fclose(fp);
	  return -1;
    } 

    bool bFixed,bExclude,bFloated;
	BOOL bComm;
	CString csFlags;

	filecount++;
	segcount++;

	do {
		if(*ln==0x0C) {
			//process next survey, if any, in file --
			e=copy_hdr(idx); //returns -1 if error, 0 if no more survey headers fouund
			if(!e) break;
			if(e<0) {
			  fclose(fp);
			  return e;
			} 
			segcount++;
		}

		cfg_GetArgv(ln,CFG_PARSE_ALL);
		if(!cfg_argc) continue; /*Ignore blank data lines*/
       
		bFixed=bWarn=bExclude=bFloated=false;
		bComm=FALSE;
		csFlags.Empty();

		e=bRedundant?11:9;
		if(cfg_argc<e) {
			warn_format("Less than %u line items, \"%s %s...\", not used", e, cfg_argv[0], (cfg_argc>1)?cfg_argv[1]:"");
			warn_format(0);
			continue;
		}

		if(!IsNumeric(cfg_argv[2])) {
			outf(";%s",cfg_argv[0]);
			for(int i=1; i<cfg_argc; i++) outf(" %s",cfg_argv[i]);
			outln();
			warn_format("3rd line item is not numeric (spaces in mames?), shot commented out");
			warn_format(0);
			continue;
		}

		if(e<cfg_argc) {
			LPCSTR p=cfg_argv[e];
			if(*p=='#' && *++p=='|') {
				CString flgs(p+1);
				int i,e0=e;
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
					   e=e0; //show flags
					}
					else e=e0+1;
				}
				else {
					warn_format("Unterminated flag string ignored");
					e=e0;
				}
			}
			if(e<cfg_argc) bComm=out_comment(e);
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
				e=chk_equiv(nr0,nr1);
				if(e>=0) {
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

		outs(get_name(nambuf0,nr0,idx));

		if(!bLrudOnly) {
			outtab();
			outs(get_name(nambuf1,nr1,idx));
			for(e=0;e<3;e++) {
				outtab();
				out_item(e);
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

	fclose(fp);
	if(fpto) fclose(fpto);
   
    lentotal+=lensurvey;
	segtotal+=segcount;
	vectotal+=veccount;
	warntotal+=warncount;
	if(outlines) printf("Surveys: %u  Vectors: %u  Length: %.0f ft  Cautions: %u\n", segcount,veccount,lensurvey, warncount);
    return 1;
}

int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
  if(InitMFC()) return 1; //fail

  int e=-1,c=0;
  char *p;
  struct _finddata_t ft;

  bProject=false;

  printf("CSS2SRV v3.01 - Convert Compass Data to Walls Format\n");

  for(int i=1;i<argc;i++) {
    p=argv[i];
    if(*p=='-') {
      while(c=*++p) {
		  switch(tolower(c)) {
		    case 'p' : if(i==argc-1) {
						   printf("\nProject pathname must follow option -p\n");
						   goto _finish;
					   }
					   argv[i++]=0;
					   if(!(p=dos_FullPath(argv[i], ".wpj"))) {
							printf("\nThe projct path, %s, is invalid.\n",argv[i]);
							goto _finish;
					   }
					   strcpy(prpath,p);
					   prname=trx_Stpnam(prpath);
					   if(prname!=prpath && !DirCheck(prpath)) {
							*trx_Stpnam(prpath)=0;
							printf("\nFolder %s can't be created.\n",prpath);
							goto _finish;
					   }
					   *trx_Stpext(strcpy(prjname,prname))=0;
					   bProject=true;
					   break;

			default  : printf("\nInvalid option: -%c\n",*p);
				       goto _finish;
		  }
		  break;
      }
	  argv[i]=NULL;
    }
	else c++;
  }
  
  if(!c) {
	  print_options();
      if(argc==1) e=0;
	  goto _finish;
  }

  bMakFile=bUsePrefix2=false;
  fixtotal=linktotal=0;
  fppr=NULL; //wpj file

  /*cfg_argv() will recognize no comment delimeters, quoted strings, etc.*/
  cfg_commchr=cfg_quotes=cfg_equals=0;
 
  ssDepthBuf[SSBUFLEN-1]=0;
  vDatName.clear();

  c=0;

  while(--argc) {
      if(!(p=*++argv)) continue;
	  c++;

	  p=trx_Stpext(strcpy(frpath,p));
	  if(!*p) {
		  printf("\nFile argument \"%s\" must have an extension.\n",trx_Stpnam(frpath));
		  goto _finish;
	  }

	  if(c==1) {
		  //This is first file argument - check if mak file
		  if(!stricmp(p, ".mak")) {
			 if((fp=fopen(frpath,"r"))==NULL) {
			   printf("\nFile %s absent or can't be opened.\n",frpath);
			   goto _finish;
			 }
			 bMakFile=true;
			 if((p=trx_Stpnam(frpath))!=frpath) {
				 VERIFY(MakeFileDirectoryCurrent(frpath));
				 memmove(frpath, p, strlen(p)+1);
			 }
			 if(!bProject) {
				strcpy(prpath,frpath);
				prname=trx_Stpnam(prpath);
				strcpy(trx_Stpext(prname),".wpj");
				*trx_Stpext(strcpy(prjname,prname))=0;
				bProject=true;
			 }
			 printf("\nReading %s...\n",trx_Stpnam(frpath));
			 if(!process_mak()) {
				 goto _finish;
			 }
			 if(vGeo.size()<2) bUsePrefix2=false;
		}
	  }

	  if(!bMakFile) {
		  if(stricmp(p, ".dat")) {
			  printf("\nA data file specification must have extension .dat.");
			  goto _finish;
		  }

		  long hFile;

		  if((hFile=_findfirst(frpath,&ft))==-1L) continue;

		  p=trx_Stpnam(frpath);

		  while(TRUE) {
			 if(ft.attrib&(_A_HIDDEN|_A_SUBDIR|_A_SYSTEM)) goto _next;
			 {
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
  } //argc

  cfg_ignorecommas=1;

  filecount=segtotal=vectotal=warntotal=revtotal=0;
  lentotal=0.0;
  vWarnMsg.clear(); bWarn=false;

  ASSERT(!bMakFile || vDatName.size()==vGeo.size());

  for(UINT idx=0; idx<vDatName.size();idx++) {
	  if(!process_dat(idx)) goto _finish;
 	  #ifdef _DEBUG
	  show_equiv(0);
 	  #endif
  }

  if(!filecount) {
	  printf("\nNo data files were found that match your specification.\n");
	  goto _finish;
  }
  if(fppr) {
	  fprintf(fppr,".ENDBOOK\n");
	  fclose(fppr);
	  printf("\nCreated Walls project: %s.wpj\n",prjname);
  }

  printf("\nData files: %u  Surveys: %u  Fixed pts: %u  Vectors: %u  Length: %.0f ft\n",
	  filecount, segtotal, fixtotal, vectotal, lentotal);

  if(warntotal) 
	  printf("\nNOTE: In Walls, search data files for %u line%s containing \"*** Caution.\"\n",warntotal,(warntotal>1)?"s":"");

  e=0;

_finish:
  while(_kbhit()>0) _getch();
  printf("\nPress any key to exit...");
  _getch();
  return e;
}

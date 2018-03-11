// htm2csv.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "htm2csv.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// The one and only application object

CWinApp theApp;

using namespace std;

#define LINLEN 4096

static FILE *fp,*fpout;
static char fname[30];
static char ln[LINLEN],tdstr[LINLEN];
static int clast,lenln,nlines;
static CString sval[6];
static LPCSTR perrdeg="Unexpected degree symbol or comma encountered";
static LPCSTR perrcoord="Bad coordinate format";

static int InitMFC()
{
	int nRetCode = 0;
	HMODULE hModule = ::GetModuleHandle(NULL);
	if(hModule != NULL)
	{
		// initialize MFC and print and error on failure
		if(!AfxWinInit(hModule, NULL, ::GetCommandLine(), 0))
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

static void fixcase(CString &nam)
{
	if(nam.GetLength()<=1) return;
	CString lnam(nam);
	lnam.MakeLower();
	bool bseen=true;
	LPSTR p=strcpy(tdstr, lnam);
	for(int i=0;*p;p++,i++) {
		if(isspace((BYTE)*p)||*p=='(') bseen=true;
		else if(bseen) {
		    *p=nam[i];
			bseen=false;
		}
	}
	nam=tdstr;
	nam.Trim();
	int i=nam.ReverseFind(' ');
	if(i>0 && tdstr[i+1]=='I' && tdstr[i+2]=='i') {
		p=&tdstr[i+2];
		*p='I';
		while(!strcmp(++p,"i")) *p='I';
		nam=tdstr;
	}
}

static int getline()
{
	//ignore empty lines terminated by /r/n --
	int c;
	lenln=0;
	if(clast) {
		if(clast==EOF) {
			return EOF;
		}
		if(clast=='\r') {
			//empty line
			*ln=0;
			nlines++;
			if((clast=fgetc(fp))=='\n') {
				//this is empty line terminated by crlf ignored --
				clast=0;
				goto _get;
			}
			return 0;
		}
		ln[lenln++]=clast;
		clast=0;
	}
_get:
	while((c=fgetc(fp))!=EOF && c!='\r') ln[lenln++]=c;
	ln[lenln]=0;
	if(c==EOF) {
		clast=EOF;
		if(lenln) {
			nlines++;
			return lenln;
		}
		return EOF;
	}
	nlines++;
	if(c=='\r' && (clast=fgetc(fp))=='\n') {
		clast=0;
		if(!lenln)
			goto _get;
	}
	return lenln;
}

static bool iscoord()
{
	return strchr(ln, 'º') && !strstr(ln, "Nº") || strchr(ln, '°') && !strstr(ln, "N°");
}

static BOOL hascomma()
{
	return strchr(ln, ',')!=NULL;
}

static int isnumber()
{
	CString s(ln);
	s.Trim();
	if(!s.IsEmpty()) {
		for(LPCSTR p=(LPCSTR)s; *p; p++) {
			if(!isdigit((BYTE)*p)) return -1;
		}
		return atoi(s)+1;
	}
	return 0;
}

static int get_tablehdr()
{
	while(getline()!=EOF) {
		if(iscoord()) {
			goto _err;
		}
		if(!strcmp(ln,"GPS")) {
			while(getline()!=EOF) {
				if(isnumber()>1) {
					return 1;
				}
				if(iscoord()) {
					goto _err;
				}
			}
			printf("Incomplete table header in file %s.\n",fname);
			return -1;
		}
	}
	return 0;
_err:
	printf("File %s, line %u: %s.\n", fname, nlines, perrdeg);
	return -1;
}

static double get_coord(LPSTR p0)
{
	double val=0.0;
	LPSTR pd, ps, pm; // '
	if(!(pd=strchr(p0, 'º')) && !(pd=strchr(p0, '°'))) return -999.0;
	if(!(pm=strchr(pd+1,'’')) || !(ps=strstr(pm+1,"’’"))) {
		if(!(pm=strchr(pd+1, '\'')) || !(ps=strstr(pm+1, "\'\'")))
			return -999;
	}
	while(pd>p0 && isdigit(*(pd-1))) pd--;
	val=atof(pd);
	while(pm>p0 && (BYTE)pm[-1]<0x7F && isdigit((BYTE)pm[-1])) pm--;
	val+=atof(pm)/60.0;
	while(ps>p0 && (BYTE)ps[-1]<0x7F && (isdigit((BYTE)ps[-1]) || ps[-1]=='.')) ps--;
	val+=atof(ps)/3600.0;
	return val;
}

static int getrec()
{
	LPCSTR perr;
	int i;

	for(i=1; i<6; i++) sval[i].Empty();
	if((i=isnumber())<0 || i==1) {
		perr="First field value should be positive number or empty";
		goto _err;
	}
	sval[1]=ln; //id
	perr="EOF encountered unexpectedly";
	if(getline()==EOF)
		goto _err;
	if(iscoord() || hascomma()) {
		perr=perrdeg; goto _err;
	}
	sval[2]=ln; //locality
	if(getline()==EOF)
		goto _err;
	if(iscoord() ||hascomma()) {
		perr=perrdeg; goto _err;
	}
	sval[3]=ln; //name
	if(getline()==EOF)
		goto _err;
	if(hascomma()) {
		perr=perrdeg; goto _err;
	}
	if(iscoord()) {
		double val=get_coord(ln);
		if(val==-999.0) {
			perr=perrcoord;
			goto _err;
		}
		sval[4].Format("%.7f", val);
		if(getline()==EOF)
			goto _err;
		val=get_coord(ln);
		if(val==-999.0) {
			perr=perrcoord;
			goto _err;
		}
		sval[5].Format("-%.7f", val);
		for(int i=0; i<6; i++) {
			if(i==2 || i==3) fixcase(sval[i]);
			fputs(sval[i], fpout);
			fputs((i<5)?", ":"\r\n", fpout);
		}
		return 1;
	}
	sval[4]=ln; //id
	sval[4].Trim();
	if(!sval[4].IsEmpty() && sval[4].Compare("0")) {
		if(!sval[4].Compare("INVENTARIO ESTATAL DE CENOTES Y GRUTAS")) {
			return -2;
		}
		perr="Location field should be \"0\" or empty";
		goto _err;
	}

	return 0; //unlocated record
_err:
	printf("File %s, line %u: %s.\n",fname,nlines,perr);
    return -1;
}

int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
	if(InitMFC()) return 1; //fail

	_set_fmode(_O_BINARY);

	struct _finddata_t ft;
	long hFile;
	int nFiles=0,nLocs=0,e=0;

	if((hFile=_findfirst("*.txt", &ft))==-1L) goto _fin0;

	if(!(fpout=fopen("cenotes.csv", "w"))) {
		printf("Can't create file cenotes.csv.\n");
		e=1;
		goto _fin0;
	}
	fprintf(fpout,"FILE, ID, LOCALITY, NAME, LATITUDE_, LONGITUDE_\r\n");

	while(TRUE) {
		if(ft.attrib&(_A_HIDDEN|_A_SUBDIR|_A_SYSTEM)) goto _next;
		if((fp=fopen(ft.name,"rb"))==NULL) {
			printf("Cannot open %s.\n",ft.name);
			e=1;
			break;
		}
		nlines=clast=0;
		nFiles++;
		strcpy(fname,ft.name);
		LPSTR p=strstr(fname," ok.txt");
		if(p) *p=0;
		sval[0]=fname;
		fixcase(sval[0]);
		//navigate to first table --
		e=get_tablehdr();
		if(e<=0) {
			if(e<0) goto _fin0;
		}
		if(!e) goto _close;

		bool bCol0=true,bStart=true;;
		 
		//process remaining lines in file --
		while(bStart || getline()!=EOF) {
			bStart=false;
			if(!strcmp(ln, "Nº") || !strcmp(ln, "N°")) {
				ASSERT(bCol0);
				e=get_tablehdr();
				if(e<=0) {
					if(e<0) goto _fin0;
				}
				if(!e) goto _close;
				bCol0=bStart=true;
				continue;
			}
			if((e=getrec())<0) {
				if(e==-2) {
					//new hdr encountered
					e=get_tablehdr();
					if(e<=0) {
						if(e<0) goto _fin0;
					}
					if(!e) goto _close;
					bCol0=true;
					continue;
				}
				//must be e==-1 (error)
				goto _fin0;
			}
			if(e) nLocs++;
			bCol0=!bCol0;
		}
		ASSERT(bCol0);

    _close:
		fclose(fp);
		fp=NULL;

	_next:
		if(_findnext(hFile, &ft)) break;
	}

_fin0:
	if(hFile) _findclose(hFile);
	if(fp) fclose(fp);
	if(fpout) fclose(fpout);
	printf("Files processed: %d  Locations: %d\n",nFiles,nLocs);
    return e;
}

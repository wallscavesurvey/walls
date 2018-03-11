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
static int clast,lenln,nlines,nlocs;
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

static void errmsg(LPCSTR perr)
{
	printf("File %s, line %u: %s.\n", fname, nlines, perr);
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

static int get_gps()
{
	double val=get_coord(ln);
	if(val==-999.0) return -2;
	sval[4].Format("%.7f", val);
	if(getline()==EOF) return -1;
	if(!iscoord()) {
		if(*ln!='0' && isnumber()) return -3;
		return -2;
	}
	val=get_coord(ln);
	if(val==-999.0) return -2;
	sval[5].Format("-%.7f", val);
	for(int i=0; i<6; i++) {
		if(i==2 || i==3) fixcase(sval[i]);
		if(i==3) fputs("Cenote ", fpout);
		fputs(sval[i], fpout);
		fputs((i<5)?",":"\r\n", fpout);
	}
	return 0;
}

int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
	if(InitMFC()) return 1; //fail

	_set_fmode(_O_BINARY);

	struct _finddata_t ft;
	long hFile;
	int nFiles=0,nLocs=0,nlocs=0,e=0,fldnam=0;
	CString s;

	if((hFile=_findfirst("*.txt", &ft))==-1L) goto _fin0;

	if(!(fpout=fopen("cenotes.csv", "w"))) {
		printf("Can't create file cenotes.csv.\n");
		e=1;
		goto _fin0;
	}
	fprintf(fpout,"FILE,ID,LOCALITY,NAME,LATITUDE_,LONGITUDE_\r\n");

	while(TRUE) {
		if(ft.attrib&(_A_HIDDEN|_A_SUBDIR|_A_SYSTEM)) goto _next;
		if((fp=fopen(ft.name,"rb"))==NULL) {
			printf("Cannot open %s.\n",ft.name);
			e=1;
			break;
		}
		nlines=clast=nlocs=0;
		nFiles++;
		strcpy(fname,ft.name);
		LPSTR p=strstr(fname," ok.txt");
		if(!p) p=strstr(fname, ".txt");
		if(p) *p=0;
		sval[0]=fname;
		fixcase(sval[0]);

		while(getline()!=EOF) {
			s=ln;
			s.Trim();
			if(!s.CompareNoCase("LOCALIDAD") || !s.CompareNoCase("CENOTE")) {
				fldnam=(s[0]=='L')?3:2;
				break;
			}
		}
		if(!fldnam) {
			printf("File %s - No header found\n",fname);
			goto _close;
		}

	    BOOL bstep=1;
			 
		//process remaining lines in file --
		while(getline()!=EOF) {
			s=ln;
			s.Trim();
			strcpy(ln,s);
			if(bstep==4) {
				if(iscoord()) {
					if(e=get_gps()) {
						if(e==EOF) errmsg("2nd coord expected");
						else if(e==-3) {
							bstep=1;
							goto _strtrec;
						}
						else {
							errmsg("Bad coordinate format");
						}
						break;
					}
					nLocs++;
					nlocs++;
				}
				//else record has no coordinates, OK
				bstep=1;
				continue;
			}
			if(iscoord()) {
				errmsg("No rec no. found for coordinates");
				break;
			}
_strtrec:
			if(bstep==1) {
				if(*ln!='0' && isnumber()>1) {
					sval[1].Format("%s-%u",(LPCSTR)sval[0],atoi(ln));
					for(int i=2; i<6; i++) sval[i].Empty();
					bstep=2;
				}
				continue;
			}
			if(hascomma()) {
				errmsg("Comma found in name field");
				break;
			}
			ASSERT(bstep==2 || bstep==3);

			if(bstep==2 && !*ln) {
				bstep=1;
				continue;
			}

			if(fldnam==3) sval[bstep]=ln;
			else sval[(bstep==2)?3:2]=ln;

			bstep++;
        }

		printf("File %s - Locations: %d\n", fname, nlocs);

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
	printf("Total files processed: %d  Locations: %d\n",nFiles,nLocs);
    return e;
}

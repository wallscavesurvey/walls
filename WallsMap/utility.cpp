#include "stdafx.h"
#include "WallsMap.h"
#include "mainfrm.h"
#include "resource.h"
#include <string.h>
#include <georef.h>

#ifdef LYR_TIMING
bool bLYRTiming=false;
UINT nLayersSkipped=0;
#endif

bool bTS_OPT=false,bIgnoreEditTS=false;

const UINT uCF_RTF=::RegisterClipboardFormat("Rich Text Format");
const UINT WM_FINDDLGMESSAGE = ::RegisterWindowMessage(FINDMSGSTRING);
const UINT WM_ADVANCED_SRCH = ::RegisterWindowMessage("ADVANCED_SRCH");
const UINT WM_CLEAR_HISTORY = ::RegisterWindowMessage("CLEAR_HISTORY");

char _string128[128];

char *CommaSep(char *outbuf,__int64 num)
{
	static NUMBERFMT fmt={0,0,3,".",",",1};
	//Assumes outbuf has size >= 20!
	TCHAR inbuf[20];
	sprintf(inbuf,"%I64d",num);
	int iRet=::GetNumberFormat(LOCALE_SYSTEM_DEFAULT,0,inbuf,&fmt,outbuf,20);
	if(!iRet) {
		strcpy(outbuf,inbuf);
	}
	return outbuf;
}

BOOL TrackMouse(HWND hWnd)
{
		TRACKMOUSEEVENT tme;
		tme.cbSize = sizeof(tme);
		tme.dwFlags = TME_LEAVE;
		tme.hwndTrack = hWnd;
		return _TrackMouseEvent(&tme);
}

void CDECL SetWndTitle(CWnd *dlg,UINT nIDS,...)
{
	char format[256];
	char buf[256];
	if(nIDS) {
	  if(!::LoadString(AfxGetInstanceHandle(),nIDS,format,256)) return;
	}
	else dlg->GetWindowText(format,256);
    va_list marker;
    va_start(marker,nIDS);
    _vsnprintf(buf,256,format,marker);
	buf[255]=0;
    va_end(marker);
    dlg->SetWindowText(buf);
}

bool AddFileSize(const char *pszPathName,__int64 &size)
{
   struct __stat64 status;

   if(!_stat64(pszPathName,&status)) {
	   size+=status.st_size;
	   return true;
   }
   return false;
}

struct tm *GetLocalFileTime(const char *pszPathName,__int64 *pSize,BOOL *pbReadOnly /*=NULL*/)
{
   struct __stat64 status;

   if(_stat64(pszPathName,&status)) return NULL;
   if(pbReadOnly) *pbReadOnly=(status.st_mode&_S_IWRITE)==0;
   if(pSize) *pSize=status.st_size;
   return _localtime64(&status.st_mtime);

}

char * GetTimeStr(struct tm *ptm,__int64 *pSize)
{
	static char buf[64];
	if(!ptm) *buf=0;
	else {
		buf[63]=0;
		int len=_snprintf(buf,63,"%4u-%02u-%02u %02u:%02u:%02u",
			ptm->tm_year+1900,ptm->tm_mon+1,ptm->tm_mday,
			ptm->tm_hour,ptm->tm_min,ptm->tm_sec);
		if(pSize) {
			char sizebuf[20];
			_snprintf(buf+len,63-len,"  Size: %s bytes",CommaSep(sizebuf,*pSize));
		}
	}
	return buf;
}

LPSTR GetLocalTimeStr()
{
	struct tm tm_loc;
	__time64_t ltime;
	_time64(&ltime);
	memset(&tm_loc,0,sizeof(tm_loc));
	return _localtime64_s(&tm_loc, &ltime)?NULL:GetTimeStr(&tm_loc,0);
}

static char BASED_CODE LoadErrorMsg[]="Out of workspace.";

int CMsgBox(UINT nType,UINT nIDS,...)
{
	char buf[1024];
	CString format;
    
    //if((nType&MB_ICONEXCLAMATION)==MB_ICONEXCLAMATION) ::MessageBeep(MB_ICONEXCLAMATION);
	
	TRY {
	    //Exceptions can be generated here --
		if(format.LoadString(nIDS)) {
			va_list marker;
			va_start(marker,nIDS);
			_vsnprintf(buf,1024,format,marker); buf[1023]=0;
			va_end(marker);
			nType=(UINT)CWinApp::ShowAppMessageBox(&theApp,buf,nType,0);
			//int CWinApp::ShowAppMessageBox(CWinApp *pApp, LPCTSTR lpszPrompt, UINT nType, UINT nIDPrompt)
		}
		else nType=0;
	}
	CATCH_ALL(e) {
	    nType=0;
	}
	END_CATCH_ALL
	
	if(!nType) ::MessageBox(NULL,LoadErrorMsg,NULL,MB_ICONEXCLAMATION|MB_OK);
	  
	return (int)nType;
}

int CMsgBox(UINT nType,LPCSTR format,...)
{
  char buf[1024];

  va_list marker;
  va_start(marker,format);
  _vsnprintf(buf,1024,format,marker); buf[1023]=0;
  va_end(marker);
  
  TRY {
      //May try to create temporary objects?
	  //nType=(UINT)XMessageBox(NULL,buf,title,nType);
	  nType=(UINT)CWinApp::ShowAppMessageBox(&theApp,buf,nType,0);
  }
  CATCH_ALL(e) {
  	  nType= 0;
  }
  END_CATCH_ALL
  
  if(!nType) ::MessageBox(NULL,LoadErrorMsg,NULL,MB_ICONEXCLAMATION|MB_OK);
  return (int)nType;
}

void CMsgBox(LPCSTR format,...)
{
  char buf[1024];

  va_list marker;
  va_start(marker,format);
  _vsnprintf(buf,1024,format,marker); buf[1023]=0;
  va_end(marker);
  AfxMessageBox(buf);
}

char * GetIntStr(__int32 i)
{
	static char buf[12];
	sprintf(buf,"%d",i);
	return buf;
}

char * GetFloatStr(double f,int dec)
{
	//Obtains the minimal width numeric string of f rounded to dec decimal places.
	//Any trailing zeroes (including trailing decimal point) are truncated.
	static char buf[32];
	bool bRound=true;

	if(dec<0) {
		bRound=false;
		dec=-dec;
	}
	ASSERT(dec<=16);
	if((UINT)_snprintf(buf,20,"%-0.*f",dec,f)>=20) {
	  *buf=0;
	  return buf;
	}
	if(bRound) {
		char *p=strchr(buf,'.');
		if(p) {
			for(p+=dec;dec && *p=='0';dec--) *p--=0;
			if(*p=='.') {
				*p=0;
				if(!strcmp(buf,"-0")) {
					*buf=0; buf[1]=0;
				}
			}
		}
	}
	return buf;
}

char * GetPercentStr(double f)
{
	double af=abs(f);
	int dec=0;
	if(af<1000.0) {
		dec++;
		if(af<100.0) {
			dec++;
			if(af<10.0) dec++;
		}
	}
	return GetFloatStr(f,dec);
}

static int MakeNestedDir(char *path)
{
	char *p=path;
	
	for(;*p;p++) if(*p=='/' || *p=='\\') {
	  *p=0;
	  _mkdir(path);
	  *p='\\';
	}
	//Success if and only if last directory can be created
	return _mkdir(path); //0 if successful
}

BOOL AddFilter(CString &filter,int nIDS)
{
    //Fix this later to handle memory exceptions --
	CString newFilter;
	
	VERIFY(newFilter.LoadString(nIDS));
    filter += newFilter;
    filter += (char)'\0';
	
    char *pext=strchr(newFilter.GetBuffer(0),'(');
    ASSERT(pext!=NULL);
    filter += ++pext;
    
    //filter.SetAt(filter.GetLength()-1,'\0') causes assertion failure!
    //Then why doesn't filter+='\0' fail also?
    
    filter.Truncate(filter.GetLength()-1); //delete trailing ')'
    filter += (char)'\0';
    return TRUE;
}    

BOOL DoPromptPathName(CString& pathName,DWORD lFlags,
   int numFilters,CString &strFilter,BOOL bOpen,UINT ids_Title,LPCSTR defExt)
{
	CFileDialog dlgFile(bOpen);  //TRUE for open rather than save

	CString title;
	VERIFY(title.LoadString(ids_Title));
	
	char namebuf[_MAX_PATH];
	char pathbuf[_MAX_PATH];

	strcpy(namebuf,trx_Stpnam(pathName));
	strcpy(pathbuf,pathName);
	*trx_Stpnam(pathbuf)=0;
	if(*pathbuf) {
		size_t len=strlen(pathbuf);
		ASSERT(pathbuf[len-1]=='\\' || pathbuf[len-1]=='/');
		pathbuf[len-1]=0;
		//lFlags|=OFN_NOCHANGEDIR;
	}
	/*
	else {
	  _getcwd(pathbuf,_MAX_PATH);
	}
	*/

	dlgFile.m_ofn.lpstrInitialDir=pathbuf; //no trailing slash

	dlgFile.m_ofn.Flags |= (lFlags|OFN_ENABLESIZING);
	if(!(lFlags&OFN_OVERWRITEPROMPT)) dlgFile.m_ofn.Flags&=~OFN_OVERWRITEPROMPT;

	dlgFile.m_ofn.nMaxCustFilter+=numFilters;
	dlgFile.m_ofn.lpstrFilter = strFilter;
	dlgFile.m_ofn.hwndOwner = AfxGetApp()->m_pMainWnd->GetSafeHwnd();
	dlgFile.m_ofn.lpstrTitle = title;
	dlgFile.m_ofn.lpstrFile = namebuf;
	dlgFile.m_ofn.nMaxFile = _MAX_PATH;
	if(defExt) dlgFile.m_ofn.lpstrDefExt = defExt+1;

	if(dlgFile.DoModal() == IDOK) {
		pathName=namebuf;
		return TRUE;
	}
	//DWORD err=CommDlgExtendedError();
	return FALSE;
}

BOOL DirCheck(LPSTR pathname,BOOL bPrompt)
{
	//Return 1 if directory exists, or 2 if successfully created after prompting.
	//If bPrompt==FALSE, creation is attempted without prompting.
	//Return 0 if directory was absent and not created, in which case
	//a message or prompt was displayed.

	struct _stat st;
	char *pnam;
	BOOL bRet=TRUE;
	char cSav;
	
	pnam=trx_Stpnam(pathname);
	if(pnam-pathname<3 || pnam[-1]!='\\') return TRUE;
	if(pnam[-2]!=':') pnam--;
	cSav=*pnam;
	*pnam=0; //We are only examining the path portion
	if(!_stat(pathname,&st)) {
	  //Does directory already exist?
	  if(st.st_mode&_S_IFDIR) goto _restore;
	  goto _failmsg; //A file by that name must already exist
	}
	//_stat falure due to anything but nonexistence?
	if(errno!=ENOENT) goto _restore; //Simply try opening the file
	
	if(bPrompt) {
		//Directory doesn't exist, select OK to create it --
		if(IDCANCEL==CMsgBox(MB_OKCANCEL,IDS_ERR_NODIR1,pathname)) goto _fail;
		bRet++;
	}
	if(!MakeNestedDir(pathname)) goto _restore;

_failmsg:
	//Can't create directory -- possible file name conflict
	CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_DIRPROJECT,pathname);
_fail:
    bRet=FALSE;
	
_restore:
	*pnam=cSav;
	return bRet;
}

bool ForceExtension(CString &path,LPCSTR ext)
{
	LPCSTR p=trx_Stpext(path);
	if(_stricmp(p,ext)) {
		path.Truncate(p-(LPCSTR)path);
		path+=ext;
		return true;
	}
	return false;
}

static void FixPathname(CString &path,LPCSTR ext)
{
	if(!*trx_Stpext(path)) {
		char *p=trx_Stpext(path.GetBuffer(path.GetLength()+5));
		*p++='.';
		strcpy(p,ext);
	    path.ReleaseBuffer();
	}
}

BOOL BrowseFiles(CEdit *pw,CString &path,LPCSTR ext,UINT idType,UINT idBrowse) 
{
	CString strFilter;
	CString Path;

	//int numFilters=(idType==IDS_PRJ_FILES)?1:2;
	
	pw->GetWindowText(Path);
	
	if(!AddFilter(strFilter,idType) /*|| numFilters==2 &&
	   !AddFilter(strFilter,AFX_IDS_ALLFILTER)*/) return FALSE;
	   
	if(!DoPromptPathName(Path,OFN_HIDEREADONLY,1/*numFilters*/,strFilter,FALSE,idBrowse,ext)) return FALSE;
	
	path=Path;
	FixPathname(path,ext+1);
	pw->SetWindowText(path);
	pw->SetSel(0,-1);
	pw->SetFocus();
	return TRUE;
}

int CheckDirectory(CString &path,BOOL bOpen /*=FALSE*/)
{
	//Return 0 if directory was created (if necessary) and either the file
	//was absent or permission was given to overwrite an existing file.
	//Return 1 if operation cancelled
	//Return 2 if permission given to open existing file (bOpen=TRUE)

	//First, let's see if the directory needs to be created --
	int e=DirCheck((char *)(LPCSTR)path,TRUE);
	//e=1 if directory already exists,
	//e=2 if successfully created after prompting.
	//e=0 if directory was absent and not created after prompting

	if(!e) e=1;
	else if(e==1) {
		e=0;
		//The directory already existed. Check for file existence --
		if(_access(path,6)!=0) {
			if(errno!=ENOENT) {
				//file not rewritable --
				AfxMessageBox(IDS_ERR_FILE_RW);
				e=1;
			}
		}
		else if(!bOpen) {
			//File exists and is rewritable.
			__int64 filesize=0;
			struct tm *ptm=GetLocalFileTime(path,&filesize);
			e=(IDOK==CMsgBox(MB_OKCANCEL,IDS_FILE_EXISTDLG,(LPCSTR)path,GetTimeStr(ptm,&filesize)))?0:1;
		}
	}
	else e=0;
	return e;
}

//Function : CheckAccess( LPCTSTR fname, int mode)
//			 Note: Workaround for bug in _access()
//Returns  : 0 when the access is present, -1 when access is not available
//Desc     : This function can be used as a substitue for _access. The paramters are the same as in _access()
//fname - file/directory whose access is to be checked
//mode - mode to be checked, pass the code corresponding to the access test required.
// 0 - Test existence
// 2 - Write access
// 4 - Read access
// 6 - Read & Write access.
//
int CheckAccess(LPCTSTR fname, int mode)
{
         DWORD dwAccess;
         if (mode == 0)
             return _access(fname, mode);   //access would do when we want to check the existence alone.

         if (mode == 2)
             dwAccess = GENERIC_WRITE;
         else if (mode == 4)
             dwAccess = GENERIC_READ;
         else if (mode == 6)
             dwAccess = GENERIC_READ  | GENERIC_WRITE;

         HANDLE hFile = 0;
         hFile = CreateFile(fname,
                 dwAccess,
                 FILE_SHARE_READ|FILE_SHARE_WRITE,       // share for reading and writing
                 NULL,                                   // no security
                 OPEN_EXISTING,                          // existing file only
                 FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS,
                 NULL);                                  // no attr. template

         if (hFile == INVALID_HANDLE_VALUE)
             { return(-1); }
         CloseHandle(hFile);
         return(0);
}

BOOL CheckFlt(const char *buf,double *pf,double fMin,double fMax,int dec,LPCSTR nam /*=""*/)
{
	double f;
	if(!IsNumeric(buf) || (f=atof(buf))<fMin || f>fMax) {
		CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_BADFLT3,dec,fMin,dec,fMax,nam);
		return FALSE;
	}
	if(pf) *pf=f;
	return TRUE;
}

BOOL CheckZoomLevel(CDataExchange* pDX,UINT id,LPCSTR s)
{
	if(!CheckFlt(s,NULL,(id==IDC_LABELZOOM)?0.0:0.001,100000.0,3," for a zoom percentage")) {
        pDX->m_idLastControl=id;
	    pDX->m_bEditLastControl=TRUE;
		pDX->Fail();
		return FALSE;
	}
	return TRUE;
}


static UINT GetShpSize(LPCSTR pszPathName,UINT uTyp,__int64 *pSize)
{
	char buf[MAX_PATH];
	LPSTR pExt=trx_Stpext(strcpy(buf,pszPathName));
	struct __stat64 status;

	if(uTyp>1) {
		strcpy(pExt,".dbt");
		if(!_stat64(buf,&status)) *pSize+=(UINT)status.st_size;
		else ASSERT(0);
	}
	strcpy(pExt,".shx");
	if(!_stat64(buf,&status)) {
		*pSize+=(UINT)status.st_size;
		uTyp++;
	}
	else ASSERT(0);
	strcpy(pExt,".dbf");
	if(!_stat64(buf,&status)) {
		*pSize+=(UINT)status.st_size;
		uTyp++;
	}
	else ASSERT(0);
	return uTyp;
}

void AppendFileTime(CString &cs,LPCSTR lpszPathName,UINT uTyp)
{
	__int64 nFileSize;
	struct tm *ptm=GetLocalFileTime(lpszPathName,&nFileSize);
	CString s2;
	if(uTyp) {
		s2.Format(" (%u files)",GetShpSize(lpszPathName,uTyp,&nFileSize));
	}
	CString s1(GetTimeStr(ptm,&nFileSize));
	cs.AppendFormat("File: %s\r\nModified: %s%s\r\n",lpszPathName,(LPCSTR)s1,(LPCSTR)s2);
}

int GetNad(LPCSTR p)
{
	int i=0;
	if(p && *p) {
		if(strstr(p,"NAD83") ||
			strstr(p,"GCS_North_American_1983") //for NAIP MrSID Gen 3 published 2010-06-14 at TNRIS
		) i=1;
		else if(strstr(p,"WGS 84") || strstr(p,"WGS84")) i=-1;
		else if(strstr(p,"NAD27")) i=2;
		else i=3;
	}
	return i;
}

#ifdef _DEBUG
// Makes trace windows a little bit more informative...
void _TraceLastError()
{
    LPVOID lpMsgBuf;
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,    
                  NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
                  (LPTSTR) &lpMsgBuf, 0, NULL);
    TRACE1("Last error: %s\n", lpMsgBuf);
    LocalFree(lpMsgBuf);
}
#endif

void LastErrorBox()
{
	LPVOID lpMsgBuf=NULL;
	DWORD error=GetLastError();
	if (FormatMessage( 
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM | 
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		error,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR) &lpMsgBuf,
		0,
		NULL ))
	{

		// Process any inserts in lpMsgBuf.
		// ...

		// Display the string.
		MessageBox( NULL, (LPCTSTR)lpMsgBuf, "WallsMap Error", MB_OK | MB_ICONINFORMATION );

	}
	if(lpMsgBuf) LocalFree( lpMsgBuf );
}

BOOL ClipboardCopy(const CString &strData,UINT cf /*=CF_TEXT*/) 
{
	BOOL bRet=FALSE;
    // test to see if we can open the clipboard first before
    // wasting any cycles with the memory allocation
	  if (AfxGetMainWnd()->OpenClipboard())
	  {
      // Empty the Clipboard. This also has the effect
      // of allowing Windows to free the memory associated
      // with any data that is in the Clipboard
		  EmptyClipboard();

      // Ok. We have the Clipboard locked and it's empty. 
      // Now let's allocate the global memory for our data.

      // Here I'm simply using the GlobalAlloc function to 
      // allocate a block of data equal to the text in the
      // "to clipboard" edit control plus one character for the
      // terminating null character required when sending
      // ANSI text to the Clipboard.
		  HGLOBAL hClipboardData;
		  hClipboardData = GlobalAlloc(GMEM_DDESHARE, 
		                               strData.GetLength()+1);

		  // Calling GlobalLock returns to me a pointer to the 
		  // data associated with the handle returned from GlobalAlloc
		  char * pchData;
		  pchData = (char*)GlobalLock(hClipboardData);
		  
		  // At this point, all I need to do is use the standard 
		  // C/C++ strcpy function to copy the data from the local 
		  // variable to the global memory.
		  strcpy(pchData, LPCSTR(strData));
		  
		  // Once done, I unlock the memory - remember you don't call
		  // GlobalFree because Windows will free the memory 
		  // automatically when EmptyClipboard is next called.
		  GlobalUnlock(hClipboardData);
		  
		  // Now, set the Clipboard data by specifying that 
		  // ANSI text is being used and passing the handle to
		  // the global memory.
		  bRet=(SetClipboardData(cf,hClipboardData)!=NULL);
		  
		  // Finally, when finished I simply close the Clipboard
		  // which has the effect of unlocking it so that other
		  // applications can examine or modify its contents.
		  CloseClipboard();
	  }
	  return bRet;
}

bool IsNumeric(LPCSTR p)
{
	bool bPeriodSeen=false,bDigitSeen=false;
	while(xisspace(*p)) p++;
	if(*p=='-' || *p=='+') p++;

	for(;*p && !xisspace(*p);p++) {
		if(isdigit((BYTE)*p)) bDigitSeen=true;
		else if(bPeriodSeen || !(bPeriodSeen=(*p=='.')))
			return false;
	}
	if(!bDigitSeen) return false;
	while(xisspace(*p)) p++;
	return *p==0;
}

static bool SetToDecimalDegrees(CString &csLat,LPCSTR pSrc)
{
	//csLat forms allowed:
	//[-]ddd.ddd°mm'ss.ddd"
	//[-]ddd.ddd°mm.ddd'

	char buf[42];
	double fdeg=0.0;
	LPSTR p=buf;
	strcpy(p,pSrc);
	LPSTR p0=strchr(p,(BYTE)0xB0);
	if(p0) { 
		*p0++=0;
		if(!IsNumeric(p)) return false;
		fdeg=atof(p);
	}
	else p0=p;
	if(*p0 && (p=strchr(p0,'\''))) {
		if(!isdigit(*p0)) return false;
		*p++=0;
		double frac=atof(p0)/60.0;
		if(*p && (p0=strchr(p,'\"'))) {
			if(!isdigit(*p)) return false;
			*p0++=0;
			frac+=atof(p)/3600.0;
		}
		if(csLat[0]=='-') fdeg-=frac;
		else fdeg+=frac; 

	}
	csLat=GetFloatStr(fdeg,7);
	return true;
}

static int get_latlon(double &fdeg,int i0,int imax,bool bFirst)
{
	//return 1 if OK, -1 if reversed, 0 if error
	char buf[42];
	LPSTR p,p0;
	int ityp;
	static char ctyp[3]={(BYTE)0xB0,'\'','\"'};
	int iRet=1;

	if(imax-i0>1) {
		//multiple args - concatenate to s, inserting units chars as necessary;
		CString s;
		ityp=0;
		for(int i=i0;i<imax;i++) {
			p=cfg_argv[i];
			while(p=strchr(p,':')) {
				if(ityp>=2) return 0; //no more than 2 colons expected
				*p++=ctyp[ityp++];
			}
		}
		ityp=0; //getting degrees, 1 (minutes), 2 (seconds);
		for(int i=i0;i<imax;i++) {
			p0=cfg_argv[i];
			s+=p0;
			for(int t=ityp;t<3;t++) {
				//look for embeddeded units chars
				if(strchr(p0,ctyp[t])) {
					ityp=t+1; //next to append
				}
			}
			BYTE c=p0[strlen(p0)-1];
			if(isdigit(c)) {
				if(ityp>=3) return 0;
				s+=ctyp[ityp++];
			}
		}
		p0=strcpy(buf,s);
	}
	else p0=cfg_argv[i0];

	bool bsgn=false,bneg=false;
	
	if((p=strchr(p0,'S')) && (bneg=true) || (p=strchr(p0,'N')) ||
		(p=strchr(p0,'W')) && (bneg=true) || (p=strchr(p0,'E')) ||
		*p0=='-' && (bneg=true)) {
		if(p) {
			if(p!=p0 && p-p0+1!=strlen(p0)) return 0;
			if(bFirst!=(*p=='S' || *p=='N')) {
				bFirst=!bFirst;
				iRet=-1;
			}
		}
		if(!p || p==p0) p0++;
		else *p=0;
		bsgn=true;
	}
	if(!*p0) return 0;

	fdeg=0.0;
	ityp=0;
	for(p=p0;*p;p++) {
		switch((BYTE)*p) {
			case '-': break;
			case 0xB0 :
				{
					*p=0;
					if(!ityp && IsNumeric(p0)) {
						ityp++;
						fdeg=atof(p0);
						p0=p+1;
						continue;
					}
					break;
				}

			case '\'' :
				{
					*p=0;
					if(ityp==1 && IsNumeric(p0)) {
						ityp++;
						fdeg+=atof(p0)/60;
						p0=p+1;
						continue;
					}
					break;
				}

			case '\"' :
				{
					*p=0;
					if(ityp==2 && IsNumeric(p0)) {
						ityp++;
						fdeg+=atof(p0)/3600;
						p0=p+1;
						continue;
					}
					break;
				}

			default:
				continue;
		}
		return 0;
	}
	if(*p0) {
		if(ityp>2 || !IsNumeric(p0)) return 0;
		fdeg+=atof(p0)/(ityp?((ityp==1)?60:3600):1);
	}

	if(fdeg>=(bFirst?90.0:180.0)) return 0;
	if(bneg) fdeg=-fdeg;
	return iRet;
}

int CheckCoordinatePair(CString &csSrc,bool &bUtm,double *pf0,double *pf1)
{
	//return 0 (error), 1 normal order, -1 reversed order

	if(csSrc.GetLength()>42) return 0;

	csSrc.MakeUpper();

	cfg_quotes=FALSE;
	int i=cfg_GetArgv((LPSTR)(LPCSTR)csSrc,CFG_PARSE_ALL);
	cfg_quotes=TRUE;
	if(i<2 || i>8 || (i&1)!=0) return 0;
	i>>=1;

	LPSTR p0=cfg_argv[0],p1=cfg_argv[1];
	double f0,f1;
	int iret=0;

	//check for UTM with 'E' and 'N' suffixes --
	char c0,c1;
	if(i==1 && ((c0=p0[strlen(p0)-1])=='E' || c0=='N') && ((c1=p1[strlen(p1)-1])=='N' || c1=='E')) {
		if(c0==c1) return 0;
		*strrchr(p0,c0)=0; *strrchr(p1,c1)=0;
		if(!IsNumeric(p0) || !IsNumeric(p1) ||	fabs(f0=atof(p0))>=10000000.0 || fabs(f1=atof(p1))>=10000000.0)
			return 0;
		iret=(c0=='E')?1:-1;
		bUtm=true;
	}

	if(i>1 || !IsNumeric(p1) || !IsNumeric(p0)) {
		iret=get_latlon(f0,0,i,true);
		if(!iret || (iret*get_latlon(f1,i,i+i,false)!=1)) return 0;
		bUtm=false;
	}

	if(!iret) {
		//two numeric arguments, proper order assumed
		iret=1;
		if(fabs(f0=atof(p0))>=10000000 || fabs(f1=atof(p1))>=10000000.0)
			return 0;
		if(!bUtm && (fabs(f0)>=90.0 || fabs(f1)>=180.0))
		bUtm=true;
	}

	if(pf0) {
		*pf0=(iret<0)?f1:f0;
		*pf1=(iret<0)?f0:f1;
	}
	return iret;
}

bool GetCoordinate(CString &csSrc,double &fCoord,bool bFirst,bool bUtm)
{
	if(bUtm) {
		int i=csSrc.GetLength()-1;
		if(toupper(csSrc[i])==(bFirst?'E':'N'))
			csSrc.Truncate(i);
	}
	if(IsNumeric(csSrc)) {
		fCoord=atof(csSrc);
		if(fabs(fCoord)>=(bUtm?10000000.0:(bFirst?90.0:180.0))) return false;
		return true;
	}
	if(bUtm) return false;
	if(csSrc.GetLength()>24) return false;
	csSrc.MakeUpper();
	cfg_quotes=FALSE;
	int i=cfg_GetArgv((LPSTR)(LPCSTR)csSrc,CFG_PARSE_ALL);
	cfg_quotes=TRUE;
	if(i<=0 || i>4) return false;
	return get_latlon(fCoord,0,i,bFirst)==1;
}

BOOL ClipboardPaste(CString &strData,UINT maxLen) 
{
 // Test to see if we can open the clipboard first.
	BOOL bRet=FALSE;
	strData.Empty();
	if (AfxGetMainWnd()->OpenClipboard()) 
	{
		if (::IsClipboardFormatAvailable(CF_TEXT)
			|| ::IsClipboardFormatAvailable(CF_OEMTEXT))
		{
			// Retrieve the Clipboard data (specifying that 
			// we want ANSI text (via the CF_TEXT value).
			HANDLE hClipboardData = GetClipboardData(CF_TEXT);

			// Call GlobalLock so that to retrieve a pointer
			// to the data associated with the handle returned
			// from GetClipboardData.
			char *pchData = (char*)GlobalLock(hClipboardData);

			// Set the CString variable to the data
			UINT len = strlen(pchData);
			if(len>maxLen) len=maxLen;
			strData.SetString(pchData,len);

     		// Unlock the global memory.
			GlobalUnlock(hClipboardData);
			bRet=!strData.IsEmpty();
		}
		// Finally, when finished I simply close the Clipboard
		// which has the effect of unlocking it so that other
		// applications can examine or modify its contents.
		CloseClipboard();
	}
	return bRet;
}

LPSTR String128(LPCSTR format,...)
{
    va_list marker;
    va_start(marker,format);
    _vsnprintf(_string128,128,format,marker); _string128[127]=0;
    va_end(marker);
	return _string128;
}

BOOL InitLabelDC(HDC hdc,CLogFont *pFont,COLORREF clr)
{
	static COLORREF textClr;
	static int bkMode;
	static HFONT hFont,hFontOld;

	if(!hdc) return FALSE;
	if(pFont) {
		if(!(hFont=pFont->Create())) return FALSE;
		hFontOld=(HFONT)::SelectObject(hdc,hFont);
		textClr=::SetTextColor(hdc,clr);
		bkMode=::SetBkMode(hdc,TRANSPARENT);
	}
	else {
		::SetBkMode(hdc,bkMode);
		::SetTextColor(hdc,textClr);
		::DeleteObject(::SelectObject(hdc,hFontOld));
	}
	return TRUE;
}

LPCSTR DatumStr(INT8 iNad)
{
	return (iNad==2)?"NAD27":((iNad==1)?"NAD83":"     ");
}

LPSTR GetRelativePath(LPSTR buf,LPCSTR pPath,LPCSTR pRefPath,BOOL bNoNameOnPath)
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
		strcpy(buf,pPath);

	if(bNoNameOnPath) strcat(buf,"\\");
	else *trx_Stpnam(buf)=0;

	int lenFilePath=strlen(buf);
	int maxLenCommon=min(lenFilePath,lenRefPath);
	for(int i=0;i<=maxLenCommon;i++) {
		if(i==maxLenCommon || toupper(buf[i])!=toupper(pRefPath[i])) {
			if(!i) break;

			while(i && buf[i-1]!='\\') i--;
			ASSERT(i);
			if(i<3) break;
			lenFilePath-=i;
			pPath+=i;
			LPSTR p=(LPSTR)pRefPath+i;
			for(i=0;p=strchr(p,'\\');i++,p++); //i=number of folder level prefixes needed
			if(i*3+lenFilePath>=MAX_PATH) {
				//at least don't overrun buf
				break;
			}
			for(p=buf;i;i--) {
				strcpy(p,"..\\"); p+=3;
			}
			//pPath=remaining portion of path that's different
			//pPath[lenFilePath-1] is either '\' or 0 (if bNoNameOnPath)
			if(lenFilePath) {
				memcpy(p,pPath,lenFilePath);
				if(bNoNameOnPath) p[lenFilePath-1]='\\';
			}
			p[lenFilePath]=0;
			break;
		}
	}
	if(!bNoNameOnPath) {
		LPCSTR p=trx_Stpnam(pPath);
		if((lenFilePath=strlen(buf))+strlen(p)<MAX_PATH)
			strcpy(buf+lenFilePath,p);
	}
	if(pDupPath) free(pDupPath);
	return buf;
}

LPSTR GetFullFromRelative(LPSTR buf,LPCSTR pPath,LPCSTR pRefPath)
{
	if(pPath[1]==':') {
		//fix for network //name/d/...
		strcpy(buf,pPath);
		return buf;
	}

	// pRefPath = "\\David-Vaio\c\gis\TSS_Data\TSS_Data.ntl"
	// pPath    = "..\TSS_NTI"
	// return   = "\\David-Vaio\c\gis\TSS_Data\..\TSS_NTI

	trx_Stncc(buf,pRefPath,MAX_PATH);
	LPSTR p=trx_Stpnam(buf);
	ASSERT(p[-1]=='\\');
	if((p-buf)+strlen(pPath)>=MAX_PATH) return NULL;
	if(*pPath) strcpy(p,pPath);
	try {
		if(!(p=(LPSTR)dos_FullPath(buf,NULL))) return NULL;
	}
	catch(...) {
		//c function dos_FullPath() uses GetFullPathName(), which can crash for some values of input,
		//such as a too-deeply nested "..\..\..\.."
		return NULL;
	}
	strcpy(buf,p);
	return buf;

/*

	LPSTR p=buf+1;
	if(*p!=':' || p[1]!='\\') {
		ASSERT(0);
		return NULL;
	}
	p+=2;
	if(*pPath=='.' && pPath[1]=='\\') pPath+=2; //allow pPath == ".\aaa\bbb\fname.rxt
	if(*pPath=='\\') pPath++;
	else {
		//allow pPath such as "..\..\aaa\bbb\fname.rxt
		p=trx_Stpnam(buf);
		while(*pPath=='.') {
			if(p==buf || *--p!='\\' || pPath[1]!='.' || pPath[2] && pPath[2]!='\\') {
				return NULL; //invalid path
			}
			while(p>buf && p[-1]!='\\') p--;
			pPath+=2;
			if(*pPath) pPath++;
		}
	}

	//at this point p
	if((p-buf)+strlen(pPath)>=MAX_PATH) return NULL;
	if(*pPath) strcpy(p,pPath);
	else {
		if(p>buf && *--p=='\\') *p=0;
		else return NULL;
	}

	return buf;
	*/
}

BOOL MakeFileDirectoryCurrent(LPCSTR pathName)
{
	LPSTR p=(LPSTR)trx_Stpnam(pathName);
	char c=*p;
	*p=0;
	if(!::SetCurrentDirectory(pathName)) {
		*p=c;
		return FALSE;
	}
	*p=c;
	return TRUE;
}

BOOL SearchDirRecursively(LPSTR pRelPathToSearch,LPCSTR pFileName,LPCSTR pSkipDir)
{
	/*	pRelPathToSearch: path (with trailing '\') relative to the current in which to
		 search recursively  for the first occurrence of pFilename

		pFilename: raw filename to search for (no wildcards for now)

		pSkipDir: Skip any directory with that name at this starting level

		Upon sucess, rtns TRUE with pRelPathToSearch set to a found relative path with pFileName attached.
	*/
    WIN32_FIND_DATA FindFileData;     
    HANDLE hFind;

	LPSTR pEnd=trx_Stpnam(pRelPathToSearch);
	ASSERT(*pEnd==0);

	strcpy(pEnd,pFileName);

	BOOL bRet=(_access(pRelPathToSearch,0)==0); //don't worry about folders with extensions like .jpg!

	if(bRet) {
		return TRUE;
	}

	strcpy(pEnd,"*");
	hFind=::FindFirstFileEx(pRelPathToSearch,FindExInfoStandard,&FindFileData,FindExSearchLimitToDirectories,NULL,0);

	if(hFind==INVALID_HANDLE_VALUE) {
		*pEnd=0;
		return FALSE;
	}

	do {
		if((FindFileData.dwFileAttributes&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_ARCHIVE))==FILE_ATTRIBUTE_DIRECTORY) {
			if(*FindFileData.cFileName=='.' || !_stricmp(pSkipDir,FindFileData.cFileName))
				continue;
			strcat(strcpy(pEnd,FindFileData.cFileName),"\\");
			bRet=SearchDirRecursively(pRelPathToSearch,pFileName,"");
			if(bRet) break;
		}
	}
	while(::FindNextFile(hFind,&FindFileData));

	FindClose(hFind);
	if(!bRet)
		*pEnd=0;

	return bRet;
}


LPSTR LabelToValidFileName(LPCSTR pLabel,LPSTR pBuf,int sizBuf)
{
	//sizBuf includes trailing NULL
	
	ASSERT(sizBuf>=1);
	if(sizBuf<1) return NULL;

	LPSTR p0=pBuf;

	bool bLastSpc=true;
	for(;*pLabel;pLabel++) {
		if(sizBuf<=1) break;
		char c=*pLabel;
		if(c>=0 && c<=31) continue;
		switch(c) {
			case ' '  : if(!bLastSpc) {
						  bLastSpc=true;
						  break;
						}
			case '\"' :
			case '\\' :
			case '*'  :
			case '#'  :
			case '?'  :
			case '<'  :
			case '>'  : continue;

			case ':'  :
			case '/'  :
			case '|'  :
			case '.'  : if(bLastSpc) continue;
						c=' ';
						bLastSpc=true;
						break;

			default	  : bLastSpc=false;
		}
		*p0++=c;
		sizBuf--;
	}
	if(bLastSpc && p0>pBuf) {
		p0--;
		sizBuf++;
	}
	*p0=0;
	if(p0==pBuf && sizBuf>6) {
		strcpy(p0,"NoName");
		p0+=6;
	}
	return p0;
}

void GetTimestamp(LPSTR pts,const struct tm &gmt)
{
	//1900-01-31 08:06:00
	int len=_snprintf(pts,20,"%04u-%02u-%02u %02u:%02u:%02u",
		gmt.tm_year+1900,gmt.tm_mon+1,gmt.tm_mday,gmt.tm_hour,gmt.tm_min,gmt.tm_sec);

	if(len!=LEN_TIMESTAMP)	*pts=0;
}

void OpenContainingFolder(LPCSTR path)
{
	CString sParam;
	sParam.Format("/select,\"%s\"",path);
	if((int)ShellExecute(NULL,"OPEN","explorer.exe", sParam, NULL, SW_NORMAL)<=32) {
		AfxMessageBox("Unable to launch Explorer");
	}
}

BOOL GetTempFilePathWithExtension(CString &csPath,LPCSTR pExt)
 {
   UUID uuid;
   unsigned char* sTemp;
   HRESULT hr = UuidCreateSequential(&uuid);
   if (hr != RPC_S_OK) return FALSE;
   hr = UuidToString(&uuid, &sTemp);
   if (hr != RPC_S_OK) return FALSE;
   CString sUUID(sTemp);
   sUUID.Remove('-');
   RpcStringFree(&sTemp);

   TCHAR pszPath[MAX_PATH];
   if(!::GetTempPath(MAX_PATH,pszPath)) return FALSE;
   csPath.Format("%s%s.%s",pszPath,(LPCSTR)sUUID,pExt);
   if(csPath.GetLength()>MAX_PATH) {
	   csPath.Empty();
	   return FALSE;
   }
   return TRUE;
}

void OpenFileAsText(LPCSTR file)
{
	char buf[MAX_PATH];
	CString path;
	HANDLE hFile;
	if(!GetTempFilePathWithExtension(path,"txt") ||
		(hFile=CreateFile(path,GENERIC_WRITE,0,NULL,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL))==INVALID_HANDLE_VALUE)
			goto _error;
	VERIFY(CloseHandle(hFile));
	HINSTANCE h=::FindExecutable(path,NULL,buf);
	VERIFY(DeleteFile(path));
	if((int)h>32 && (int)ShellExecute(NULL, "open", buf, file, NULL, SW_NORMAL)>32)
		return;
_error:
	CMsgBox("File %s missing or could not be opened.",(LPCSTR)file);
}

BOOL GetKmlPath(CString &path,LPCSTR pLabel)
{
	TCHAR pszPath[MAX_PATH];
	DWORD len=::GetTempPath(MAX_PATH,pszPath);
	len=::GetLongPathName(pszPath,pszPath,MAX_PATH); //needed for XP!
	if(len>MAX_PATH) {
	  ASSERT(0);
	  return FALSE; //shouldn't happen
	}
	LPSTR p=LabelToValidFileName(pLabel,pszPath+len,MAX_PATH-len-5);
	if(!p) return FALSE;
	strcpy(p,".kml");
	path.SetString(pszPath);
	return TRUE;
}

void ToolTipCustomize(int nInitial,int nAutopop,int nReshow,CToolTipCtrl* pToolTip)
{
		if(!pToolTip)
			pToolTip = AfxGetModuleState()->m_thread.GetDataNA()->m_pToolTip;
		else nInitial+=400;

		if(!pToolTip || !::IsWindow(pToolTip->m_hWnd))
			return;

		// Set max tip width in pixel.
		// you can change delay time, tip text or background color also. enjoy yourself!
		pToolTip->SetMaxTipWidth(500);

#ifdef _DEBUG
		UINT tm=GetDoubleClickTime();
		tm=pToolTip->GetDelayTime(TTDT_INITIAL); //500   these defaults correspond to a double-click time of 500 ms
		tm=pToolTip->GetDelayTime(TTDT_AUTOPOP); //5000                3400
		tm=pToolTip->GetDelayTime(TTDT_RESHOW);	 //100                 68
#endif
		pToolTip->SetDelayTime(TTDT_INITIAL,nInitial);     //thumblist: //64
		pToolTip->SetDelayTime(TTDT_AUTOPOP,nAutopop);                  //32000
		pToolTip->SetDelayTime(TTDT_RESHOW,nReshow);                    //32
}

BOOL SetFileToCurrentTime(LPCSTR path)
{
	BOOL f=FALSE;
    HANDLE hFile = CreateFile(path, GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_DELETE,NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
	if(hFile) {
		FILETIME ft;
		SYSTEMTIME st;
		GetSystemTime(&st);              // Gets the current system time
		SystemTimeToFileTime(&st, &ft);  // Converts the current system time to file time format
		f= SetFileTime(hFile,            // Sets last-write time of the file 
			(LPFILETIME) NULL,           // to the converted current system time 
			(LPFILETIME) NULL, 
			&ft);
		CloseHandle(hFile);
	}
	return f;
}

WORD GetOSVersion();
WORD wOSVersion=GetOSVersion();

static WORD GetOSVersion()
{
	//XP SP3 returns 0x0501 !
	//Vista returns 0x600 if not in compatability mode.
	//In release mode only, when Vista is set to Windows Classic theme this fcn returns 0x0501!
	/*
	OSVERSIONINFO osver = {0};
	osver.dwOSVersionInfoSize = sizeof(osver);
	GetVersionEx(&osver);
	WORD w=MAKEWORD(osver.dwMinorVersion, osver.dwMajorVersion);
	*/

	OSVERSIONINFOEX osvi;
	DWORDLONG dwlConditionMask = 0;
	int op=VER_GREATER_EQUAL;

	// Initialize the OSVERSIONINFOEX structure.

	ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	osvi.dwMajorVersion = 6; //5
	//osvi.dwMinorVersion = 1;
	//osvi.wServicePackMajor = 2;
	//osvi.wServicePackMinor = 0;

    // Initialize the condition mask.
	VER_SET_CONDITION( dwlConditionMask, VER_MAJORVERSION, op );
    //VER_SET_CONDITION( dwlConditionMask, VER_MINORVERSION, op );
    //VER_SET_CONDITION( dwlConditionMask, VER_SERVICEPACKMAJOR, op );
    //VER_SET_CONDITION( dwlConditionMask, VER_SERVICEPACKMINOR, op );

	BOOL bRet=::VerifyVersionInfo(
      &osvi, 
      VER_MAJORVERSION //| VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR
      , dwlConditionMask);

	bRet=bRet?0x0600:0x0500;

	//CMsgBox("version=%x",bRet);
	return bRet;
}

bool GetConvertedPoint(CFltPoint &fpt,const int iNad,int *piZone,const int iNadSrc,const int iZoneSrc)
{
	//Retrieve UTM iff piZone!=NULL, in which case force zone to *piZone iff *piZone!=0.
	ASSERT((iNad==1 || iNad==2) && (iNadSrc==1 || iNadSrc==2) && abs(iZoneSrc)<=60 && (!piZone || abs(*piZone)<=60));
	if(iNad==iNadSrc && ((!piZone && !iZoneSrc) || (piZone && *piZone && *piZone==iZoneSrc))) return true;
	if(iZoneSrc) {
		//Need to convert to Lat/Long before zone or datum change --
		geo_UTM2LatLon(fpt,iZoneSrc,(iNadSrc==2)?geo_NAD27_datum:geo_WGS84_datum);
	}
	if(iNadSrc!=iNad) {
		geo_FromToWGS84(iNadSrc==1,&fpt.y,&fpt.x,geo_NAD27_datum); //converting from wgs84 to nad27 OR from nad27 to wgs84
	}
	if(piZone) {
		//requesting UTM --
		if(!geo_LatLon2UTM(fpt,piZone,(iNad==2)?geo_NAD27_datum:geo_WGS84_datum)) {
			return false;
		}
	}
	return true;
}

int GetZone(const double &lat,const double &lon)
{
	double lon0 = 6.0 * floor(lon / 6.0) + 3.0;
	if(lon0==183) lon0=177;
	int zone=((int)lon0 +183) / 6;
	if (lat < 0.0) {
		zone=-zone;
	}
	return zone;
}

LPCSTR GetMediaTyp(LPCSTR sPath,BOOL *pbLocal /*=NULL*/)
{
	static LPCSTR MediaTyp[]={
		".png",".jpg",".tif",".nti",".ai",".avi",".doc",".docx",".flv",".gmw",
		".kml",".m4v",".mp4",".mpg",".ntl",".pdf",".qgs",".rtf",".shp",".svg",".svgz",
		".tt6",".wmv",".wpj"};
	#define NUM_MEDIATYP (sizeof(MediaTyp)/sizeof(LPCSTR))
	#define NUM_LOCALTYP 4

	LPCSTR pExt=trx_Stpext(sPath);
	if(*pExt) {
		LPCSTR *pTyp=MediaTyp;
		for(int i=0;i<NUM_MEDIATYP;i++,pTyp++) {
			if(!_stricmp(*pTyp,pExt)) {
				if(pbLocal) *pbLocal=(i<NUM_LOCALTYP);
				return *pTyp;
			}
		}
	}
	return NULL;
}

LPCSTR ReplacePathExt(CString &path,LPCSTR pExt)
{
	int i=path.ReverseFind('.');
	if(i>path.ReverseFind('\\')) path.Truncate(i);
	path+=pExt;
	return path;
}

LPCSTR ReplacePathName(CString &path,LPCSTR name)
{
	path.Truncate(trx_Stpnam(path)-(LPCSTR)path);
	path+=name;
	return path;
}

void Pause()
{
	MSG msg;
	while (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		 if (msg.message == WM_QUIT)
		 {
		    //bDoingBackgroundProcessing = FALSE;
		    ::PostQuitMessage(msg.wParam);
		    break;
		 }
		 if (!AfxGetApp()->PreTranslateMessage(&msg))
		 {
		    ::TranslateMessage(&msg);
		    ::DispatchMessage(&msg);
		 }
	}
	AfxGetApp()->OnIdle(0);   // updates user interface
	AfxGetApp()->OnIdle(1);   // frees temporary objects 
}

#ifdef _DEBUG
BOOL SaveBitmapToFile(CBitmap *bitmap, CDC* pDC, LPCSTR lpFileName)
{
	HBITMAP hBitmap;
	int iBits;
	WORD wBitCount;
	DWORD dwPaletteSize=0, dwBmBitsSize, dwDIBSize, dwWritten;
	BITMAP Bitmap;
	BITMAPFILEHEADER bmfHdr;
	BITMAPINFOHEADER bi;
	LPBITMAPINFOHEADER lpbi;

	HANDLE fh, hDib, hOldPal = NULL;
	hBitmap = (HBITMAP)*bitmap;
	iBits = pDC->GetDeviceCaps(BITSPIXEL) * pDC->GetDeviceCaps(PLANES);

	if (iBits <= 1 )
		wBitCount = 1;
	else if (iBits <= 4 )
		wBitCount = 4;
	else if (iBits <= 8 )
		wBitCount = 8;
	else if (iBits <= 24 )
		wBitCount = 24;
	else if (iBits <= 32 )
		wBitCount = 32;

	if (wBitCount <= 8 )
	dwPaletteSize = (1 << wBitCount) * sizeof(RGBQUAD);

	GetObject(hBitmap, sizeof(BITMAP), (LPSTR)&Bitmap);
	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = Bitmap.bmWidth;
	bi.biHeight = Bitmap.bmHeight;
	bi.biPlanes = Bitmap.bmPlanes;
	bi.biBitCount = wBitCount;
	bi.biCompression = BI_RGB;
	bi.biSizeImage = 0;
	bi.biXPelsPerMeter = 0;
	bi.biYPelsPerMeter = 0;
	bi.biClrUsed = 0;
	bi.biClrImportant = 0;
	dwBmBitsSize = ((Bitmap.bmWidth * wBitCount+31) / 32) * 4 * Bitmap.bmHeight ;

	hDib = GlobalAlloc(GHND,dwBmBitsSize+dwPaletteSize+sizeof(BITMAPINFOHEADER));
	lpbi = (LPBITMAPINFOHEADER)GlobalLock(hDib);
	*lpbi = bi;

	CPalette pal;
	pal.m_hObject = GetStockObject(DEFAULT_PALETTE);
	CPalette* pOldPal = pDC->SelectPalette(&pal, FALSE);
	pDC->RealizePalette();

	GetDIBits(pDC->m_hDC, hBitmap, 0, (UINT) Bitmap.bmHeight,
		(LPSTR)lpbi + sizeof(BITMAPINFOHEADER) + dwPaletteSize, (LPBITMAPINFO )lpbi, DIB_RGB_COLORS);

	if(pOldPal)
	{
		pDC->SelectPalette(pOldPal, TRUE);
		pDC->RealizePalette();
	}

	fh = CreateFile(lpFileName, GENERIC_WRITE,	0, NULL, CREATE_ALWAYS,	FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);

	if (fh == INVALID_HANDLE_VALUE)
		return FALSE;

	bmfHdr.bfType = 0x4D42; // "BM"
	dwDIBSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER)	+ dwPaletteSize + dwBmBitsSize;
	bmfHdr.bfSize = dwDIBSize;
	bmfHdr.bfReserved1 = 0;
	bmfHdr.bfReserved2 = 0;
	bmfHdr.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER) + dwPaletteSize;
	WriteFile(fh, (LPSTR)&bmfHdr, sizeof(BITMAPFILEHEADER), &dwWritten, NULL);
	WriteFile(fh, (LPSTR)lpbi, dwDIBSize, &dwWritten, NULL);
	GlobalUnlock(hDib);
	GlobalFree(hDib);
	CloseHandle(fh);
	return TRUE;
}
#endif

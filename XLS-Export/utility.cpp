#include "stdafx.h"
#include "crack.h"
#include "XlsExport.h"
#include "XMessageBox.h"
#include "utility.h"

static __inline bool isspc(char c)
{
	return c==' ' || c=='\\';
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

void CMsgBox(LPCSTR format,...)
{
  char buf[512];

  va_list marker;
  va_start(marker,format);
  _vsnprintf(buf,512,format,marker); buf[511]=0;
  va_end(marker);
  AfxMessageBox(buf);
}

int CMsgBox(UINT nType,LPCSTR format,...)
{
  static char BASED_CODE LoadErrorMsg[]="Out of workspace.";
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

long GetLong(CDaoRecordset &rs,int nFld)
{
	COleVariant var;
	rs.GetFieldValue(nFld,var);
	if(var.vt==VT_NULL) return 0;
	if(var.vt==VT_I4) return (long)V_I4(&var);
	if(var.vt==VT_R8) return (long)V_R8(&var);
	if(var.vt==VT_DECIMAL) return (long)V_DECIMAL(&var).Lo64;
	CString s(CCrack::strVARIANT(var));
	return s.IsEmpty()?0:atol(s);
}

__int64 GetInt64(CDaoRecordset &rs,int nFld)
{
	COleVariant var;
	rs.GetFieldValue(nFld,var);
	if(var.vt==VT_NULL) return 0;
	if(var.vt==VT_I4) return (__int64)V_I4(&var);
	if(var.vt==VT_R8) return (__int64)V_R8(&var);
	if(var.vt==VT_DECIMAL) return (__int64)V_DECIMAL(&var).Lo64;
	CString s(CCrack::strVARIANT(var));
	if(s.IsEmpty()) return 0;
	__int64 ll;
	sscanf(s,"%I64d",&ll);
	return ll;
}

double GetDouble(CDaoRecordset &rs,int nFld)
{
	COleVariant var;
	rs.GetFieldValue(nFld,var);
	if(var.vt==VT_NULL) return 0;
	if(var.vt==VT_R8) return (double)V_R8(&var);
	if(var.vt==VT_DECIMAL) return (double)V_DECIMAL(&var).Lo64;
	CString s(CCrack::strVARIANT(var));
	return s.IsEmpty()?0:atof(s);
}

char * GetFloatStr(double f,int dec)
{
	//Obtains the minimal width numeric string of f rounded to dec decimal places.
	//Any trailing zeroes (including trailing decimal point) are truncated.
	static char buf[40];
	bool bRound=true;

	if(dec<0) {
		bRound=false;
		dec=-dec;
	}
	if(_snprintf(buf,20,"%-0.*f",dec,f)==-1) {
	  *buf=0;
	  return buf;
	}
	if(bRound) {
		char *p=strchr(buf,'.');
		ASSERT(p);
		for(p+=dec;dec && *p=='0';dec--) *p--=0;
		if(*p=='.') {
		   *p=0;
		   if(!strcmp(buf,"-0")) strcpy(buf,"0");
		}
	}
	return buf;
}

int GetStringTest(CString &s,CDaoRecordset &rs,int nFld)
{
	COleVariant var;
	rs.GetFieldValue(nFld,var);
	if(var.vt==VT_EMPTY || var.vt==VT_NULL)
		s.Empty();
	else {
		s=CCrack::strVARIANT(var);
		s.Trim();
		LPCSTR p=s;
		int iret=1;
		for(;*p;p++) {
		  if((*(BYTE *)p)&0x80) {
			  return iret;
		  }
		  iret++;
		}
	}
	return 0;
}

void GetString(CString &s, CDaoRecordset &rs, int nFld)
{
	COleVariant var;
	rs.GetFieldValue(nFld, var);
	if(var.vt==VT_EMPTY || var.vt==VT_NULL)
		s.Empty();
	else {
		s=CCrack::strVARIANT(var);
		s.Trim();
	}
}
	
char *GetDateUTC8()
{
	static char date[10];
	struct tm gmt;
	time_t ltime;

	time(&ltime);
	if(_gmtime64_s( &gmt, &ltime )) *date=0;
	else if(8!=_snprintf(date,10,"%04u%02u%02u",gmt.tm_year+1900,gmt.tm_mon+1,gmt.tm_mday))	*date=0;
	return date;
}

int GetText(LPSTR dst,CDaoRecordset &rs,int nFld,int nLen)
{
	COleVariant var;
	rs.GetFieldValue(nFld,var);
	if(var.vt==VT_EMPTY || var.vt==VT_NULL) {
		*dst=0;
		return 0;
	}
	CString s(CCrack::strVARIANT(var));
	s.Trim();
	trx_Stncc(dst,s,nLen+1);
	return s.GetLength();
}

BOOL GetBoolCharVal(LPSTR dst,char c)
{
    *dst=0;
	switch(c) {
		case 'T':
		case 't':
		case 'Y':
		case 'y': *dst='Y'; break;
		case 'F':
		case 'f':
		case 'N':
		case 'n': break;
		default : return FALSE;
	}
	return TRUE; //valid char
}

bool GetBool(CDaoRecordset &rs,int nFld)
{
	COleVariant var;
	rs.GetFieldValue(nFld,var);
	if(var.vt==VT_BOOL) return (V_BOOL(&var)!=0);
	if(var.vt==VT_BSTR) {
		 CString s(CCrack::strVARIANT(var));
	     s.Trim();
		 if(!s.IsEmpty()) {
			 s.MakeUpper();
			 if(!s.Compare("TRUE") || !s.Compare("YES")) return true;
		 }
	}
	return false;
}

void GetDateStr(LPSTR p,CDaoRecordset &rs,int nFld)
{
	COleVariant var;
	rs.GetFieldValue(nFld,var);
	if(var.vt==VT_DATE) {
		 CString s(CCrack::strVARIANT(var));
	     s.Trim();
		 strcpy(p,s);
	}
	else *p=0;
}

void GetBoolStr(LPSTR s,CDaoRecordset &rs,int nFld)
{
	strcpy(s, GetBool(rs, nFld)?"Y":"N");
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

int MsgYesNoCancelDlg(HWND hWnd, LPCSTR msg, LPCSTR title, LPCSTR pYes, LPCSTR pNo, LPCSTR pCancel)
{
		XMSGBOXPARAMS xmb;

		xmb.bUseUserDefinedButtonCaptions = TRUE; 
		_tcscpy(xmb.UserDefinedButtonCaptions.szYes, pYes); 
		_tcscpy(xmb.UserDefinedButtonCaptions.szNo, pNo);
		if(pCancel) _tcscpy(xmb.UserDefinedButtonCaptions.szCancel, pCancel);

		return 0xFF & XMessageBox(hWnd, msg, title,
			pCancel?(MB_YESNOCANCEL|MB_ICONQUESTION|MB_NORESOURCE):(MB_YESNO|MB_ICONQUESTION|MB_NORESOURCE), &xmb);
}

int MsgCheckDlg(HWND hWnd, UINT mb, LPCSTR msg, LPCSTR title, LPCSTR pDoNotAsk)
{
		XMSGBOXPARAMS xmb;

		xmb.bUseUserDefinedButtonCaptions = TRUE; 
		_tcscpy(xmb.UserDefinedButtonCaptions.szDoNotTellAgain, pDoNotAsk);

		if(mb==MB_OK) mb |= (MB_ICONINFORMATION | MB_NOSOUND | MB_DONOTTELLAGAIN | MB_NORESOURCE);
		else mb |= (MB_ICONQUESTION | MB_NOSOUND | MB_DONOTTELLAGAIN | MB_NORESOURCE);
		int i=XMessageBox(hWnd, msg, title, mb, &xmb);
		return ((i&0xFF)==IDOK)?(1+((i&MB_DONOTTELLAGAIN)!=0)):0;
}

bool MsgOkCancel(HWND hWnd, LPCSTR msg, LPCSTR title)
{
	int i=XMessageBox(hWnd, msg, title,MB_OKCANCEL | MB_NOSOUND | MB_ICONQUESTION | MB_NORESOURCE, NULL);
	return (i&0xFF)==IDOK;
}

int MsgInfo(HWND hWnd, LPCSTR msg, LPCSTR title, UINT style /*= MB_NOSOUND | MB_ICONINFORMATION*/)
{
	if(!style) style = (MB_NOSOUND | MB_ICONINFORMATION);
	return XMessageBox(hWnd, msg, title, style | MB_NORESOURCE, NULL);
}

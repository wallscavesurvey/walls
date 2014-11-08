#include "stdafx.h"
#include "UpdateTSS.h"
#include "crack.h"
#include "utility.h"

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


LPSTR GetFullPath(LPCSTR pathName,LPCSTR defExt)
{
  /*Get absolute filename. Append extension defExt if
    an extension of any kind (including trailing '.') is absent,
	or force extension override if defExt has a '.' prefix.
	Return pointer to static buffer, or NULL if path is invalid. */

  /*Static buffer for for returned pathname --*/
  static char _dos_nambuf[MAX_PATH+2];
  LPSTR pName;
  DWORD len;

  if(!(len=GetFullPathName(pathName,MAX_PATH+1,_dos_nambuf,&pName)) ||
	  len>MAX_PATH || *pName==0) return NULL;
  if(defExt && *defExt) {
	  while(*pName && *pName!='.') pName++;
	  if(*defExt=='.' || *pName==0) {
		//Extension override required --
		if(*pName==0) *pName='.';
		pName[1]=0;
		if(*defExt=='.') defExt++;
		if(strlen(_dos_nambuf)+strlen(defExt)>MAX_PATH) return NULL;
		strcpy(pName+1,defExt);
	  }
  }
  return _dos_nambuf;
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

bool PutLong(CDaoRecordset &rs,int nFld,long n)
{
	COleVariant var(n);
	try {
		rs.SetFieldValue(nFld,var);
	}
	catch (CDaoException *ex)
	{
		if(ex) ex->Delete();
		return false;
	}
	return true;
}

bool PutDouble(CDaoRecordset &rs,int nFld,double d)
{
	COleVariant var(d);
	try {
		rs.SetFieldValue(nFld,var);
	}
	catch (CDaoException *ex)
	{
		if(ex) ex->Delete();
		return false;
	}
	return true;
}

bool PutDouble(CDaoRecordset &rs,int nFld,double d,int dec)
{
	d=atof(GetFloatStr(d,dec));
	COleVariant var(d);
	try {
		rs.SetFieldValue(nFld,var);
	}
	catch (CDaoException *ex)
	{
		if(ex) ex->Delete();
		return false;
	}
	return true;
}

bool PutString(CDaoRecordset &rs,int nFld,LPCSTR s)
{
	//COleVariant var(s); //fails!
	try {
		rs.SetFieldValue(nFld,s);
	}
	catch (CDaoException *ex)
	{
		if(ex) ex->Delete();
		return false;
	}
	return true;
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

bool GetBool(CDaoRecordset &rs,int nFld)
{
	COleVariant var;
	rs.GetFieldValue(nFld,var);
	return (var.vt==VT_BOOL)?(V_BOOL(&var)!=0):false;
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

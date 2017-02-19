#include "Stdafx.h"
#include "resource.h"
#include "ShpLayer.h"
#ifdef _USE_GE_COM
#include "earth_h.h"
#endif
#include "zip.h"
#include "kmldef.h"

bool GE_Checked=false;

static CString csGE_fmt;
static CString csGE_ptfmt;
static CString csGE_path;
static CString csGE_lkfmt;
static LPCSTR pKML_EOF="</Document>\r\n</kml>\r\n";

static BOOL init_csFmt()
{
	HGLOBAL hg;
	HRSRC hrfmt;
	LPCSTR fmt;

	if(!(hrfmt=FindResource(NULL,MAKEINTRESOURCE(IDR_DATA2),RT_RCDATA)) ||
	   !(hg=LoadResource(NULL,hrfmt)) ||
	   !(fmt=(LPCSTR)LockResource(hg)))
	{
		return FALSE;
	}
	csGE_fmt.SetString(fmt,SizeofResource(NULL, hrfmt));
	csGE_ptfmt.LoadStringA(IDS_FMT_PLACEMARK);
	csGE_lkfmt.LoadStringA(IDS_FMT_LOOKAT);
	return TRUE;
}

/*
static bool FixName(CString &name)
{
	static LPCSTR _badchr="<>|\":*?#";
	char buf[MAX_PATH];
	LPSTR pbuf=buf;
	bool bSkip=false;

	for(LPCSTR p=name;*p;p++) {
		if(!strchr(_badchr,*p)) *pbuf++=*p;
		else bSkip=true;
	}
	if(bSkip) {
		*pbuf=0;
		name=(*buf)?buf:"$$$";
	}
	return bSkip;
}
*/

static void FixLabel(CString &s)
{
	s.Replace("&","&amp;");
	s.Replace("<","&lt;");
	s.Replace(">","&gt;");
	s.Replace("'","&apos;");
	s.Replace("\"","&quot;");
}

CString &GE_Path()
{
	return csGE_path;
}

BOOL GE_Launch(CShpLayer *pShp,LPCSTR pathName,GE_POINT *pt,UINT numpts,BOOL bFly)
{
	/*
		bFly = 0   - save only pathname (from options dlg)
		bFly = 1   - fly direct (no descrptions - ignore pShp->m_vKmlFlds)
		bFly = 2   - fly direct (from options dlg)
	*/

	BOOL bRet=FALSE;

	#ifdef _USE_GE_COM
	IApplicationGE* pGEI=NULL;
	#endif

	try {
		if(csGE_fmt.IsEmpty() && !init_csFmt()) {
			ASSERT(0);
			return FALSE;
		}

		LPCSTR p=trx_Stpnam(pathName);
		CString csName(p);
		FixLabel(csName);
		CString csLookat,kmlData;
		double range=CShpLayer::m_uKmlRange;
		double alat=pt->m_lat,alon=pt->m_lon;

		if(numpts==1) {
			ASSERT(!pt->m_csLabel.IsEmpty());
			p=CShpLayer::m_csIconSingle;
		}
		else {
			//Determine range (width of view in meters) from point spread -
			double latmin,latmax,lonmin,lonmax;
			latmin=latmax=pt->m_lat;
			lonmin=lonmax=pt->m_lon;
			for(GE_POINT *pn=pt+numpts-1;pn!=pt;pn--) {
				if(latmin>pn->m_lat) latmin=pn->m_lat;
				if(latmax<pn->m_lat) latmax=pn->m_lat;
				if(lonmin>pn->m_lon) lonmin=pn->m_lon;
				if(lonmax<pn->m_lon) lonmax=pn->m_lon;
			}
			alat=(latmin+latmax)/2;
			double rw=1000*MetricDistance(alat,lonmin,alat,lonmax);
			alon=(lonmin+lonmax)/2;
			double rh=1000*MetricDistance(latmin,alon,latmax,alon);
			//assume 4w x 3h screen dinensions --
			if(3*rw>=4*rh) range=1.5*rw; //base on range width
			else  range=2*rh; //==(4/3)*1.5*rh; //base on range height
			if(range<CShpLayer::m_uKmlRange)
				range=CShpLayer::m_uKmlRange;

			p=CShpLayer::m_csIconMultiple;
		}

		if(numpts>1)
			csLookat.Format(csGE_lkfmt,"\t","\t\t",alon,alat,"\t\t",(UINT)range,"\t");

		kmlData.Format(csGE_fmt,(LPCSTR)csName,(LPCSTR)csLookat,CShpLayer::m_uIconSize/10.0,p,CShpLayer::m_uLabelSize/10.0);

		/* File can be too large for CString and not worth zipping as a final step --
		if(!strcmp(trx_Stpext(pathName),".kmz")) {
			HZIP hz=CreateZip(pathName,NULL); //no password
			if(!hz) throw 0;
			ZRESULT zr=ZipAdd(hz,"doc.kml",(void *)(LPCSTR)kmlData,(UINT)kmlData.GetLength());
			CloseZip(hz);
			if(zr!=ZR_OK) {
				_unlink(pathName);
				throw 0;
			}
		}
		else {
		*/

		CFile cf;
		if(!cf.Open(pathName, CFile::modeWrite|CFile::modeCreate)) {
			throw 0;
		}
		cf.Write(kmlData,kmlData.GetLength());

		ASSERT(pShp || bFly==1);

		bool bDesc=(bFly!=1 && !pShp->m_vKmlFlds.empty());
		csName.Empty();

		for(UINT n=0;n<numpts;n++,pt++) {
			if(bDesc) {
				CString s;
				if(pShp->InitDescGE(s,pt->m_rec,pShp->m_vKmlFlds,TRUE)) { //skip empty fields
					s.Replace("\r\n","<br>\r\n");
					csName.Format("\r\n\t\t<description>\r\n<![CDATA[\r\n%s]]>\r\n\t\t</description>",(LPCSTR)s);
				}
				else csName.Empty();
			}
			FixLabel(pt->m_csLabel);
			csLookat.Format(csGE_lkfmt,"\t\t","\t\t\t",pt->m_lon,pt->m_lat,"\t\t\t",CShpLayer::m_uKmlRange,"\t\t");
			kmlData.Format(csGE_ptfmt,(LPCSTR)pt->m_csLabel,(LPCSTR)csLookat,(LPCSTR)csName,pt->m_lon,pt->m_lat);
			cf.Write(kmlData,kmlData.GetLength());
		}
		cf.Write(pKML_EOF,strlen(pKML_EOF));
		cf.Close();

		bRet=TRUE;

		if(bFly) {
#ifdef _USE_GE_COM
			HRESULT hr;
			if(CoCreateInstance(CLSID_ApplicationGE,
				NULL,
				CLSCTX_LOCAL_SERVER,
				IID_IApplicationGE,
				(void**) &pGEI )!=S_OK)
			{
				CMsgBox("%s was created but Google Earth could not be accessed.",trx_Stpnam(pathName));
				return FALSE;
			}

			//LoadKmlData() doesn't work: adds duplicates!
			BSTR bsFname = pathName.AllocSysString();

			hr=pGEI->OpenKmlFile(bsFname,0); // loads the string

			::SysFreeString(bsFname);
			bRet=(hr == S_OK);
#else
			ASSERT(GE_IsInstalled());
			csName.Format("\"%s\"",(LPCSTR)pathName);
			if((int)ShellExecute(NULL,"open",csGE_path, csName, NULL, SW_NORMAL)<=32)
				bRet=FALSE;
#endif
		}
	}
	catch(...) {
		CMsgBox("Unable to create %s.",trx_Stpnam(pathName));
	}

#ifdef _USE_GE_COM
	if(pGEI) pGEI->Release();
#endif

	return bRet;
}

static DWORD ReadClassesKey(LPCSTR keynam, LPSTR value)
{
	HKEY hkey;
	char keystr[80];
	int len=_snprintf(keystr,80,"SOFTWARE\\Classes\\%s",keynam);
	if(len==80 || RegOpenKeyEx(HKEY_LOCAL_MACHINE,keystr,0,KEY_READ,&hkey))
		return 0;

	DWORD sz_value=_MAX_PATH; //includes space for null terminator
	if(RegQueryValueEx(hkey,NULL,NULL,NULL,(LPBYTE)value,&sz_value))
		sz_value=0;
	RegCloseKey(hkey);
	return sz_value; 
}

#ifdef _DEBUG
  #define _DEBUG_KML
#endif

bool GE_IsInstalled()
{
	static bool bAvail=false;

	if(GE_Checked) return bAvail;
	GE_Checked=true;
	bAvail=false;

	if(!csGE_path.IsEmpty() && !_access(csGE_path, 0))
		return bAvail=true;

	char path[_MAX_PATH];
	char kmzfile[80];
#ifdef _DEBUG_KML
	CString s;
#endif

	DWORD len=ReadClassesKey(".kmz", path);
	if(!len || len>=80) {
#ifdef _DEBUG_KML
	    s="Key .kmz not found.";
		goto _err;
#else
		return false;
#endif
	}
	strcpy(kmzfile, path);
	sprintf(path,"%s\\shell\\open\\command",kmzfile);
	if(!ReadClassesKey(path, path)) {
#ifdef _DEBUG_KML
		s.Format("Command not found for file type %s.",kmzfile);
		goto _err;
#else
		return false;
#endif
	}
	LPSTR p=strstr(path,".exe");
	if(p) {
		if(p[4]=='"') p[5]=0; else p[4]=0;
		csGE_path=path;
		if(*path=='"' && p[4]=='"') {
			p[4]=0;
			p=path+1;
		}
		else p=path;
		if(!_access(p,0)) {
			bAvail=true;
			csGE_path=p;
		}
#ifdef _DEBUG_KML
		else {
		   s.Format("Command path not found:\n%s",p);
		   goto _err;
		}
	}
	else {
		s.Format("Extension .exe not found in command path:\n%s",p);
		goto _err;
#endif
	}
	return bAvail;

#ifdef _DEBUG_KML
_err:
	AfxMessageBox(s);
	return false;
#endif
}


// XlsExportDlg.cpp : implementation file
//

#include "stdafx.h"
#include "XlsExport.h"
#include "PromptPath.h"
#include <dbfile.h>
#include <georef.h>
#include "XlsExportDlg.h"
#include "utility.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

static LPCSTR pFldSep;
static int nSep;

#define NUM_DATUMS 3
static LPCSTR datum_name[NUM_DATUMS]={"NAD27","WGS84","NAD27"};

static bool bGetDMS=false;

static UINT xlsline=0,numRecs=0;

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()

static CString csLogPath;

// CXlsExportDlg dialog

CXlsExportDlg::CXlsExportDlg(int argc, char **argv, CWnd* pParent /*=NULL*/)
	: CDialog(CXlsExportDlg::IDD, pParent)
	, m_argc(argc)
	, m_argv(argv)
	, m_nLogMsgs(0)
	, m_bLogFailed(FALSE)
	, m_fbLog(2048)
	, m_bLatLon(TRUE)
	, m_psd(NULL)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	VERIFY(::GetModuleFileName(NULL, m_szModule, MAX_PATH-4));
	strcpy(trx_Stpext(m_szModule),".ini");
	*m_szSourcePath=*m_szShpdefPath=*m_szShpPath=0;

	if(m_argc>1) {
		int len;
		if(!(len=GetFullPathName(m_argv[1],MAX_PATH,m_szSourcePath,&m_pSourceName)) || len==MAX_PATH-1) *m_szSourcePath=0;
		if(m_argc>2) {
			m_tableName=m_argv[2];
			if(m_argc>3) {
				if(!(len=GetFullPathName(m_argv[3],MAX_PATH-7, m_szShpdefPath, &m_pShpdefName)) || len==MAX_PATH-1) *m_szShpdefPath=0;
				else strcpy(trx_Stpext(m_szShpdefPath),".shpdef");
				if(m_argc>4) {
					if(!(len=GetFullPathName(m_argv[4],MAX_PATH, m_szShpPath, &m_pShpName)) || len==MAX_PATH-1) *m_szShpPath=0;
					else strcpy(trx_Stpext(m_szShpPath),".shp");
					if(m_argc>5 && !stricmp(m_argv[5], "UTM")) m_bLatLon=FALSE;
				}
				else if(*m_szShpdefPath) {
					strcpy(m_szShpPath,m_szShpdefPath);
					strcpy(trx_Stpext(m_szShpPath),".shp");
				}
			}
		}
	}
	else {
		::GetPrivateProfileString("Paths","Source","",m_szSourcePath,_MAX_PATH,m_szModule);
		if(*m_szSourcePath) {
			char table[80];
			::GetPrivateProfileString("Paths","Table","",table,80,m_szModule);
			m_tableName=table;
			::GetPrivateProfileString("Paths","Shpdef","",m_szShpdefPath,_MAX_PATH,m_szModule);
			::GetPrivateProfileString("Paths","Output","",m_szShpPath,_MAX_PATH,m_szModule);
			m_bLatLon=::GetPrivateProfileInt("Paths","UTM",0,m_szModule)==0;
		}
	}
	m_sourcePath=m_szSourcePath;
	m_shpdefPath=m_szShpdefPath;
	m_shpPath=m_szShpPath;
}

void CXlsExportDlg:: SaveOptions()
{
	::WritePrivateProfileString("Paths","Source",m_szSourcePath,m_szModule);
	::WritePrivateProfileString("Paths","Table",m_tableName,m_szModule);
	::WritePrivateProfileString("Paths","Shpdef",m_szShpdefPath,m_szModule);
	::WritePrivateProfileString("Paths","Output",m_szShpPath,m_szModule);
	::WritePrivateProfileString("Paths","UTM",m_bLatLon?NULL:"1",m_szModule);
}

BEGIN_MESSAGE_MAP(CXlsExportDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BROWSE, OnBnClickedBrowse)
	ON_BN_CLICKED(IDC_BROWSE_TMP, &CXlsExportDlg::OnBnClickedBrowseShpdef)
	ON_BN_CLICKED(IDC_BROWSE_SHP, &CXlsExportDlg::OnBnClickedBrowseShp)
END_MESSAGE_MAP()

void CDECL CXlsExportDlg::WriteLog(const char *format,...)
{
  char buf[256];
  int len;

  if(m_bLogFailed) return;

  if(!m_fbLog.IsOpen()) {
	  csLogPath=FixShpPath(".log");

	  if((m_bLogFailed=m_fbLog.OpenFail(csLogPath,CFile::modeCreate|CFile::modeReadWrite|CFile::shareExclusive)))
		  return;
	  CString s;
	  if(m_bXLS<2) s.Format(", Table: %s",(LPCSTR)m_tableName);
	  len=_snprintf(buf,255,"Export Log - Source: %s%s\r\nShapefile: %s\r\n\r\n",m_pSourceName,(LPCSTR)s,m_pShpName);
	  m_fbLog.Write(buf,len);
  }

  m_nLogMsgs++;

  va_list marker;
  va_start(marker,format);
  //NOTE: Does the 2nd argument include the terminating NULL?
  //      The return value definitely does NOT.
  CString key;
  if(!m_fldKeyVal.IsEmpty()) key.Format(", Key: %s", (LPCSTR)m_fldKeyVal);
  len=_snprintf(buf,255,"Rec:%7u%s - ",numRecs+1,key); //also should show xlsline id reford is skipped
  m_fbLog.Write(buf,len);
  if((len=_vsnprintf(buf,255,format,marker))<0) len=254;
  m_fbLog.Write(buf,len);
  m_fbLog.Write("\r\n",2);
  va_end(marker);
}

static BOOL GetMemoFld(LPSTR pDest,CString &cs)
{
	UINT dbtrec=dbt_AppendRec(cs);
	if(!dbtrec) return FALSE;
	sprintf(pDest,"%010u",dbtrec);
	return TRUE;
}

LPCSTR CXlsExportDlg::FixShpPath(LPCSTR ext)
{
	m_pathBuf.Truncate(m_nPathLen);
	m_pathBuf+=ext;
	return m_pathBuf;
}

static double getDeg(double y)
{
	y*=0.0001;
	double deg=(UINT)y;
	y=(y-deg)*100.0;
	double min=(UINT)y;
	y=(y-min)*100.0;
	double sec=(UINT)y;
	return deg+min/60.0+sec/3600.0;
}

int CXlsExportDlg::StoreFldData(int f)
{
	DBF_FLDDEF &fd=m_psd->v_fldDef[f];
	int fSrc=m_psd->v_srcIdx[f];
	if(fSrc<0) {
		LPCSTR pDefault=(LPCSTR)m_psd->v_srcDflt[f];
		if(*pDefault) {
			StoreText(f,pDefault);
		}
		return 0;
	}
	char buf[256];
	int iRet=0,len=0;
	switch(fd.F_Typ) {
		case 'N':
			{
				if(fd.F_Dec==0 && fd.F_Len<9) {
					long n=GetDbLong(fSrc);
					len=_snprintf(buf,fd.F_Len,"%*d",fd.F_Len,n);
				}
				else {
					double d=GetDbDouble(fSrc); //handles dbDecimal for now
					len=_snprintf(buf,fd.F_Len,"%*.*f",fd.F_Len,fd.F_Dec,d);
				}
				if(len<0) {
					iRet=-1;
					len=fd.F_Len;
				}
				break;
			}
		case 'C':
			{
				if(fSrc>=0)	{
					len=GetDbText(buf, fSrc, fd.F_Len); //len = actual data length
				}
				if(!len) {
					len=strlen(strcpy(buf, m_psd->v_srcDflt[f]));
				}
				if(len>fd.F_Len) {
					iRet=fd.F_Len-len;
					len=fd.F_Len;
				}
				break;
			}
		case 'M':
			{
			    CString memo;
				CString &rMemo=(m_bXLS<2)?memo:m_vcsv[fSrc];
				if(m_bXLS<2) GetString(memo,m_rs,fSrc);
				if(!rMemo.IsEmpty()) {
					if(!GetMemoFld(buf, rMemo)) iRet=-rMemo.GetLength();
					else len=10;
				}
				break;
			}
		case 'L':
			{
				GetDbBoolStr(buf,fSrc);
				len=*buf?1:0;
				break;
			}
		default: return 0;
	}
	if(len>0) {
		StoreText(f,buf);
	}
	return iRet;
}

int CXlsExportDlg::GetCoordinates(double *pLat, double *pLon, double *pEast, double *pNorth,  int *pZone)
{
	bool bDefX=false,bDefY=false;
	double y=GetDbDouble(m_psd->v_srcIdx[m_psd->iFldY]);
	if(!y) {
		bDefY=true;
		*pLat=m_psd->latDflt;
	}
	else if(bGetDMS) {
		y=getDeg(y);
		//y*=m_psd->v_fldScale[m_psd->iFldY];
	}
	double x=GetDbDouble(m_psd->v_srcIdx[m_psd->iFldX]);
	if(!x) {
		bDefX=true;
		*pLon=m_psd->lonDflt;
	}
	else if(bGetDMS) {
		x=-getDeg(x);
		//x*=m_psd->v_fldScale[m_psd->iFldX];
	}

	int e=0;

	if(bDefX!=bDefY) {
	   int dec=m_psd->iTypXY?2:6;
	   WriteLog("Partial coordinate set: %s = %.*f, %s = %.*f",
	      m_psd->v_fldDef[m_psd->iFldX].F_Nam, dec, x, m_psd->v_fldDef[m_psd->iFldY].F_Nam, dec, y);
	   *pLat=m_psd->latErr;
	   *pLon=m_psd->lonErr; 
	   e=-1;
	   goto _fix;
	}

	if(bDefX) {
		*pLat=m_psd->latDflt;
		*pLon=m_psd->lonDflt;
		*pZone=0;
		geo_LatLon2UTM(*pLat, *pLon, pZone, pEast, pNorth, m_iDatum);
		return 1; //default location acceptable if both coordinates missing
	}

	*pZone=m_psd->iZoneDflt;

	if(!m_bLatLon) {
		//creating UTM shapefile, in which case a fixed zone is specified in shpdef
		//whether or not UTM coordinates are being read --
		ASSERT(*pZone);
	}
	else {
		//creating lat/long shapefile -- we can either be reading Lat/Long,
		//in which case zone is always calculated, or reading UTM, in which case
		//zone is either initialized or read from source.
		
		//This condition tested when shpdef was processed --
		ASSERT(!m_psd->iTypXY || *pZone || m_psd->iFldZone>=0 && m_psd->v_srcIdx[m_psd->iFldZone]>=0);

		if(m_psd->iTypXY && !*pZone) {
			*pZone=GetDbLong(m_psd->v_srcIdx[m_psd->iFldZone]);
			if(!*pZone || *pZone<m_psd->zoneMin || *pZone>m_psd->zoneMax) { 
				*pLat=m_psd->latErr;
				*pLon=m_psd->lonErr;
				WriteLog("Bad or missing UTM zone (%d): %.2fE, %.2fN",*pZone,x,y);
				e=-1;
				goto _fix;
			}
		}
	}

	if(m_psd->iTypXY) {
	    //x,y are utm coordinates read from source --

		geo_UTM2LatLon(*pZone, x, y, pLat, pLon, m_iDatum);
		if(*pLat<m_psd->latMin || *pLat>m_psd->latMax || *pLon<m_psd->lonMin || *pLon>m_psd->lonMax) {
			*pLat=m_psd->latErr;
			*pLon=m_psd->lonErr; 
			WriteLog("Location out of range: % .2fE, %.2fN, %u%c",x,y,abs(*pZone),(*pZone>0)?'N':'S');
			e=-2;
		}
		else {
			*pEast=x;
			*pNorth=y;
		}
	}
	else {
		if(y<m_psd->latMin || y>m_psd->latMax || x<m_psd->lonMin ||x>m_psd->lonMax) {
			*pLat=m_psd->latErr;
			*pLon=m_psd->lonErr; 
			WriteLog("Location out of range: %.6f %.6f",y,x);
			e=-2;
		}
		else {
			*pLat=y;
			*pLon=x;
			geo_LatLon2UTM(y, x, pZone, pEast, pNorth, m_iDatum);
		}
	}

_fix:
	if(e) {
		*pZone=0;
		geo_LatLon2UTM(*pLat, *pLon, pZone, pEast, pNorth, m_iDatum);
	}

	return e; 
}

static bool _check_sep(LPCSTR p, LPCSTR pSep)
{
	int n=strlen(pSep);
	for(int i=0;i<3;i++,p+=n)
		if(!(p=strstr(p, pSep))) return false;
	pFldSep=pSep;
	nSep=n;
	return true;
}

static int _getFieldStructure(V_CSTRING &v_cs, LPCSTR p)
{
	if(!_check_sep(p, "\",\"") && !_check_sep(p,"\"|\"") && !_check_sep(p,"|") && 
		!_check_sep(p, "\t") && !_check_sep(p, ",")) return 0;

	if(nSep!=1) {
		if(*p++ != '"') return 0; //skip past first quote
	}
	LPCSTR p0=p;
	for(;p=strstr(p, pFldSep); p+=nSep, p0=p) {
		v_cs.push_back(CString(p0, p-p0));
	}
	if(nSep==1) v_cs.push_back(CString(p0));
	else {
		int i=strlen(p0);
		if(!i || p0[--i]!='"') return 0;
		v_cs.push_back(CString(p0,i));
	}
	return v_cs.size();
}

static bool _stopMsg(LPCSTR ptyp)
{
	CMsgBox("Line %u: A quote (\") must %s a field value.",xlsline,ptyp);
	return false;
}

static int _sepCount(LPCSTR p)
{
	int n=0;
	while(*p && (p=strstr(p,pFldSep))) {
		n++;
		p+=nSep;
	}
	return n;
}

bool CXlsExportDlg::GetCsvRecord()
{
	CString s;
#ifdef _DEBUG
	if(xlsline>=108) {
		int jj=0;
	}
#endif
	if(!m_csvfile.ReadString(s)) return false;
	xlsline++;
	LPCSTR p0,p=s;
	if(nSep!=1) {
		if(*p!='"') {
		  return _stopMsg("start");
		}
		while(true) {
			p0=p+strlen(p)-1;
			if(*p0=='"' && (p0[-1]!=',' || p0[-2]!='"')) {
				//count fields --
				int n=_sepCount(s);
				if(n >= m_psd->numDbFlds-1) {
					if(n==m_psd->numDbFlds) {
						CMsgBox("Line %u, Rec %u: Too many field separators (%u) in data record.",xlsline,numRecs,n);
						return false;
					}
					break;
				}
			}
			CString s0;
			if(!m_csvfile.ReadString(s0)) {
				return _stopMsg("end");
			}
			xlsline++;
			//if(*p0=='"')
			s+="\r\n"; //required?
			s+=s0;
			p=s;
		}
		p++;
	}

	p0=p;
	int f=0;
	for(;f<m_psd->numDbFlds-1 && (p=strstr(p, pFldSep)); p+=nSep, p0=p, f++) {
		m_vcsv[f].SetString(p0,p-p0);
		m_vcsv[f].Trim();
	}
	if(f==m_psd->numDbFlds-1) {
		if(nSep==1) m_vcsv[f]=p0;
		else {
			int i=strlen(p0);
			ASSERT(!i || p0[--i]=='"');
			m_vcsv[f].SetString(p0,i);
		}
	}
	while(f<m_psd->numDbFlds-1) m_vcsv[f++].Empty();

	return true;
}

void CXlsExportDlg::InitFldKey()
{
	int f=m_psd->v_srcIdx[m_psd->iFldKey];
	DBF_FLDDEF &fd=m_psd->v_fldDef[m_psd->iFldKey];
	if(m_bXLS==2) {
		m_fldKeyVal=m_vcsv[f];
	}
	else {
		if(fd.F_Typ=='N') {
			__int64 n=GetInt64(m_rs,f);
			m_fldKeyVal.Format("%*I64d",fd.F_Len,n);
		}
		else {
			GetString(m_fldKeyVal,m_rs,f);
		}
	}
	if(m_fldKeyVal.GetLength()>fd.F_Len)
		m_fldKeyVal.Truncate(fd.F_Len);
}

void CXlsExportDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_PATHNAME, m_sourcePath);
	DDV_MaxChars(pDX, m_sourcePath, _MAX_PATH);
	DDX_Text(pDX, IDC_TABLENAME, m_tableName);
	DDV_MaxChars(pDX, m_tableName, 50);
	DDX_Text(pDX, IDC_TMPNAME, m_shpdefPath);
	DDV_MaxChars(pDX, m_shpdefPath, _MAX_PATH);
	DDX_Text(pDX, IDC_SHPNAME, m_shpPath);
	DDV_MaxChars(pDX, m_shpPath, _MAX_PATH);
	DDX_Radio(pDX, IDC_UTM, m_bLatLon);

	if(pDX->m_bSaveAndValidate) {
		CWaitCursor wait;

		m_sourcePath.Trim();
		if(m_sourcePath.IsEmpty()) {
			pDX->m_idLastControl=IDC_PATHNAME;
			AfxMessageBox("A source pathname is required.");
			pDX->Fail();
			return;
		}

		::GetFullPathName(m_sourcePath,MAX_PATH,m_szSourcePath,&m_pSourceName);

		LPSTR pExt=trx_Stpext(m_szSourcePath);
		m_bXLS=!_stricmp(pExt, ".xls") || !_stricmp(pExt, ".xlsx");

		if(!m_bXLS && _stricmp(pExt, ".mdb")) {
			if(!_stricmp(pExt, ".csv")) m_bXLS=2;
			else {
				pDX->m_idLastControl=IDC_PATHNAME;
				AfxMessageBox("Source extension must be one of .xls, .mdb, and .csv.");
				pDX->Fail();
				return;
			}
		}

		if(_access(m_szSourcePath, 4)) {
			pDX->m_idLastControl=IDC_PATHNAME;
			CMsgBox("Source file %s is absent or inaccessible.", m_pSourceName);
			pDX->Fail();
			return;
		}

		m_tableName.Trim();
		if(m_bXLS<2 && m_tableName.IsEmpty()) {
			pDX->m_idLastControl=IDC_TABLENAME;
			AfxMessageBox("A table or sheet name is required.");
			pDX->Fail();
			return;
		}

		m_shpdefPath.Trim();
		if(m_shpdefPath.IsEmpty() || stricmp(trx_Stpext(m_shpdefPath), ".shpdef")) {
			pDX->m_idLastControl=IDC_TMPNAME;
			AfxMessageBox("A template file with extension .shpdef must be secified.");
			pDX->Fail();
			return;
		}

		::GetFullPathName(m_shpdefPath,MAX_PATH,m_szShpdefPath,&m_pShpdefName);

		if(_access(m_szShpdefPath, 4)) {
			pDX->m_idLastControl=IDC_TMPNAME;
			CMsgBox("Template file %s is absent or inaccessible.", m_pShpdefName);
			pDX->Fail();
			return;
		}

		m_shpPath.Trim();
		if(m_shpPath.IsEmpty()) {
			pDX->m_idLastControl=IDC_SHPNAME;
			AfxMessageBox("An output shapefile pathname is required.");
			pDX->Fail();
			return;
		}

		::GetFullPathName(m_shpPath,MAX_PATH,m_szShpPath,&m_pShpName);
		pExt=trx_Stpext(m_szShpPath);
		strcpy(pExt,".shp");
		m_nPathLen=pExt-m_szShpPath;
		m_pathBuf=m_szShpPath;
		FixShpPath(".dbf");

		if(!_access(m_pathBuf,0)) {
			//file exists --
		    bool ok=false;
			//dbf exists
			if(CheckAccess(m_pathBuf,4)) {
			    //not accesible for reaf/write --
				CMsgBox("%s is protected and can't be overwritten.",m_pShpName);
			}
			else if(IDYES==CMsgBox(MB_YESNO, "%s already exists.\nDo you want to overwrite it?", m_pShpName)) {
				ok=true;
			}
			if(!ok) {
				pDX->m_idLastControl=IDC_SHPNAME;
				pDX->Fail();
				return;
			}
		}

		ASSERT(!m_psd);

		xlsline=0;

		if(m_bXLS>1) {
			if(!m_csvfile.Open(m_szSourcePath, CFile::modeRead)) {
				CMsgBox("File %s missing or can't be opened.", m_pSourceName);
				pDX->m_idLastControl=IDC_PATHNAME;
				pDX->Fail();
				return;
			}

			m_psd=new CShpDef();

			//determine field names and separator --
			CString s;
			if(!m_csvfile.ReadString(s) || !(m_psd->numDbFlds=_getFieldStructure(m_psd->v_srcNames, s))) {
				delete m_psd;
				m_psd=NULL;
				m_csvfile.Close();
				CMsgBox("Fewer than 3 field names recognized in first line of %s.", m_pSourceName);
				pDX->m_idLastControl=IDC_PATHNAME;
				pDX->Fail();
				return;
			}
			xlsline++;

			SaveOptions();
			if(!m_psd->Process(NULL, m_szShpdefPath, m_bLatLon)) {
				delete m_psd;
				m_psd=NULL;
				m_csvfile.Close();
				pDX->m_idLastControl=IDC_TMPNAME;
				pDX->Fail();
				return;
			}

		}
		else {
			try
			{
				m_database.Open(m_szSourcePath, TRUE, TRUE, m_bXLS?"Excel 5.0":"");
				ASSERT(m_database.IsOpen());
				CString table;
				table.Format("[%s%s]", (LPCSTR)m_tableName, m_bXLS?"$":"");
				m_rs.m_pDatabase=&m_database;
				m_rs.Open(dbOpenTable, table, dbReadOnly);

				m_psd=new CShpDef();
				m_psd->numDbFlds=m_rs.GetFieldCount();
			}
			catch(CDaoException* e)
			{
				e->Delete();
				CString s;
				BOOL bOpen=m_database.IsOpen();
				if(bOpen) s.Format("Unable to open table \"%s\" in %s.", (LPCSTR)m_tableName, m_pSourceName);
				else s.Format("Unable to access source: %s", m_pSourceName);
				if(m_rs.IsOpen()) m_rs.Close();
				if(m_psd) {
					delete m_psd;
					m_psd=NULL;
				}
				AfxMessageBox(s);
				pDX->m_idLastControl=IDC_PATHNAME;
				pDX->Fail();
				return;
			}

			SaveOptions();
			if(!m_psd->Process(&m_rs, m_szShpdefPath, m_bLatLon)) {
				CloseDB();
				delete m_psd;
				m_psd=NULL;
				pDX->m_idLastControl=IDC_TMPNAME;
				pDX->Fail();
				return;
			}
		}

		if(InitShapefile()) {
			//shouldn't happen since DBF is available for writing --
			CloseDB();
			delete m_psd; //done in constructor
			m_psd=NULL;
			CMsgBox("Error initializing %s.",m_pShpName);
			EndDialog(IDCANCEL);
			return;
		}

		{
			LPCSTR pd=(m_psd->iFldDatum>=0)?m_psd->v_srcDflt[m_psd->iFldDatum]:"WGS84";
			if(!strcmp(pd,"NAD83")) m_iDatum=geo_NAD83_datum;
			else if(!strcmp(pd,"NAD27")) m_iDatum=geo_NAD27_datum;
			else m_iDatum=geo_WGS84_datum;
		}

		UINT nLocErrors=0, nLocMissing=0, nLocErrRange=0, nTruncated=0;
		numRecs=0;

		try {
			BOOL bNotEOF;
			if(m_bXLS>1) {
				m_vcsv.assign(m_psd->numDbFlds, CString());
				bNotEOF=GetCsvRecord();
			}
			else bNotEOF=!m_rs.IsEOF();

			while(bNotEOF>0) {

				if(m_psd->iFldKey>=0) InitFldKey();

				double lat, lon, east, north;
				int zone=0;
				int e=GetCoordinates(&lat, &lon, &east, &north, &zone);

				if(e) {
					if(e==1) nLocMissing++;
					else if(e==-1) nLocErrors++;
					else if(e==-2) nLocErrRange++;
				}

				if(m_bLatLon) {
					e=WriteShpRec(lon,lat);
				}
				else {
					e=WriteShpRec(east,north);
				}

				if(e) {
					CString msg;
					msg.Format("Aborted: Error writing shp record %u.", numRecs+1);
					WriteLog(msg);
					AfxMessageBox(msg);
					break;
				}

				for(int f=0; f<m_psd->numFlds; f++) {
					DBF_FLDDEF &fd=m_psd->v_fldDef[f];
					int n=m_psd->GetLocFldTyp(fd.F_Nam)-1;
					if(n>=LF_LAT && n<=LF_ZONE) {
						//Store already-computed location data --
						switch(n) {
							case LF_LAT: StoreDouble(fd, f, lat);
								break;
							case LF_LON: StoreDouble(fd, f, lon);
								break;
							case LF_EAST: StoreDouble(fd, f, east);
								break;
							case LF_NORTH: StoreDouble(fd, f, north);
								break;
							case LF_ZONE:
							{
								CString s;
								s.Format("%02u%c", (zone<0)?-zone:zone, (zone<0)?'S':'N');
								StoreText(f, (LPCSTR)s);
								break;
							}
						}
						continue;
					}

					//normal field --

					int len=StoreFldData(f); //suuports only N, C, M, and L for now
					if(len<0) {
						CString msg;
						if(fd.F_Typ=='C') {
							msg.Format("Character data in field %s (length %u) truncated to %u.", fd.F_Nam, fd.F_Len-len, fd.F_Len);
						}
						else if(fd.F_Typ=='M') {
							msg.Format("Field %s memo (length %u) couldn't be stored.", fd.F_Nam, fd.F_Len);
						}
						else {
							msg.Format("Numeric data in field %s truncated and may be corrupt.", fd.F_Nam);
						}
						WriteLog(msg);
						nTruncated++;
					}
				}

				numRecs++;
				if(m_bXLS<2) {
					m_rs.MoveNext();
					bNotEOF=!m_rs.IsEOF();
					if(!bNotEOF) xlsline++;
				}
				else {
					bNotEOF=GetCsvRecord();
				}
			}
			if(bNotEOF<0) {
				CMsgBox("Reading terminated after %u records due an unrecognized line format.", numRecs);
			}
		}
		catch(CDaoException* e)
		{
			// Do nothing--used for security violations when opening tables
			e->Delete();
		}

		CloseDB();

		if(numRecs) {
			if(CloseFiles())
				AfxMessageBox("Error closing shapefile");
			else {
				CString msg, msg2;

				msg.Format("Shapefile with %u records created.\nLocations missing: %u, Locations out-of-range: %u,\nPartial (bad) locations: %u, Truncated fields: %u",
					numRecs, nLocMissing, nLocErrRange, nLocErrors, nTruncated);

				if(m_nLogMsgs) {
					msg2.Format("\n\nNOTE: %u diagnostic messages were written to %s.",
						m_nLogMsgs, trx_Stpnam(csLogPath));
					msg+=msg2;
				}
				AfxMessageBox(msg);
			}
		}
		else {
			DeleteFiles();
			AfxMessageBox("No shapefile created.");
		}

		delete m_psd;
		m_psd=NULL;

		if(m_fbLog.IsOpen()) {
			m_fbLog.Close();
			if((int)::ShellExecute(NULL, "Open", csLogPath, NULL, NULL, SW_NORMAL)<=32) {
				CMsgBox("Couldn't open log %s automatically.", csLogPath);
			}
		}
	}
}

// CXlsExportDlg message handlers

BOOL CXlsExportDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CXlsExportDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CXlsExportDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CXlsExportDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CXlsExportDlg::OnBnClickedBrowse()
{
	CString strFilter;
	if(!AddFilter(strFilter,IDS_ALL_FILES) || !AddFilter(strFilter,IDS_XLS_FILES) || !AddFilter(strFilter,IDS_MDB_FILES) || !AddFilter(strFilter,IDS_CSV_FILES)) return;
	CString s;
	if(DoPromptPathName(s,OFN_FILEMUSTEXIST,3,strFilter,TRUE,IDS_XLS_EXPORT,".xls")) {
		LPCSTR p=trx_Stpext(s);
		if(stricmp(p, ".xls") && stricmp(p,".xlsx") && stricmp(p, ".csv") && stricmp(p, ".mdb")) {
			s.Truncate(p-(LPCSTR)s);
			s+=".xls";
			p=trx_Stpext(s);
		}
		char typ=_toupper(p[1]);
		GetDlgItem(IDC_PATHNAME)->SetWindowText(s);
		if(_toupper(p[1])!='M') GetDlgItem(IDC_TABLENAME)->SetWindowText((typ=='X')?"Sheet1":"");
	}
}


void CXlsExportDlg::OnBnClickedBrowseShpdef()
{
	CString strFilter;
	if(!AddFilter(strFilter,IDS_SHPDEF_FILES)) return;
	CString s;
	if(DoPromptPathName(s,OFN_FILEMUSTEXIST,1,strFilter,TRUE,IDS_SEL_SHPDEF,".shpdef")) {
		LPCSTR p=trx_Stpext(s);
		if(stricmp(p,".shpdef")) {
			s.Truncate(p-(LPCSTR)s);
			s+=".shpdef";
			p=trx_Stpext(s);
		}
		GetDlgItem(IDC_TMPNAME)->SetWindowText(s);
	}
}

void CXlsExportDlg::OnBnClickedBrowseShp()
{
	CString strFilter;
	if(!AddFilter(strFilter,IDS_SHP_FILES)) return;
	CString s;
	if(DoPromptPathName(s,OFN_HIDEREADONLY,1,strFilter,TRUE,IDS_SHP_FILES,".shp")) {
		LPCSTR p=trx_Stpext(s);
		if(stricmp(p,".shp")) {
			s.Truncate(p-(LPCSTR)s);
			s+=".shp";
			p=trx_Stpext(s);
		}
		GetDlgItem(IDC_SHPNAME)->SetWindowText(s);
	}
}

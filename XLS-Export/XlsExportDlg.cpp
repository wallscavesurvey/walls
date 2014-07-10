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

static LPCSTR pFldSep="\"|\"";
//static LPCSTR pFldSep="\",\"";

#define NUM_DATUMS 3
static LPCSTR datum_name[NUM_DATUMS]={"NAD27","WGS84","NAD27"};

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
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	if(m_argc>1) {
		m_dbPath=m_argv[1];
		if(m_argc>2) {
			m_tableName=m_argv[2];
			if(m_argc>3) m_shpName=m_argv[3];
			else m_shpName=m_tableName;
		}
		else {
			LPCSTR pExt=trx_Stpext(m_dbPath);
			UINT len=trx_Stpnam(m_dbPath)-(LPCSTR)m_dbPath;
			m_tableName.SetString((LPCSTR)m_dbPath+len, pExt-(LPCSTR)m_dbPath-len);
			m_shpName=m_tableName;
		}
	}
}

void CDECL CXlsExportDlg::WriteLog(const char *format,...)
{
  char buf[256];
  int len;

  if(m_bLogFailed) return;

  if(!m_fbLog.IsOpen()) {
	  csLogPath=FixShpPath(".log");

	  if((m_bLogFailed=m_fbLog.OpenFail(csLogPath,CFile::modeCreate|CFile::modeReadWrite|CFile::shareExclusive)))
		  return;
	  len=_snprintf(buf,255,"Export Log - Source: %s, Table: %s\r\nShapefile: %s\r\n\r\n",m_pdbName,(LPCSTR)m_tableName,FixPath(m_shpName,".shp"));
	  m_fbLog.Write(buf,len);
  }

  m_nLogMsgs++;

  va_list marker;
  va_start(marker,format);
  //NOTE: Does the 2nd argument include the terminating NULL?
  //      The return value definitely does NOT.
  len=_snprintf(buf,255,"%s%7u: ",(LPCSTR)m_fldKeyVal,numRecs+1); //also should show xlsline id reford is skipped
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

LPCSTR CXlsExportDlg::FixPath(LPCSTR name, LPCSTR ext)
{
	m_pathBuf.Truncate(m_nPathLen);
	m_pathBuf+=name;
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
	DBF_FLDDEF &fd=m_sd.v_fldDef[f];
	int fSrc=m_sd.v_srcIdx[f];
	if(fSrc<0) return 0;
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
					len=strlen(strcpy(buf, m_sd.v_srcDflt[f]));
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

int CXlsExportDlg::GetCoordinates(double *pLat, double *pLon)
{
	bool bDefX=false,bDefY=false;
	double y=GetDbDouble(m_sd.v_srcIdx[m_sd.iFldY]);
	if(!y) {
		bDefY=true;
		*pLat=m_sd.latDflt;
	}
	else {
		y=getDeg(y);
		//y*=m_sd.v_fldScale[m_sd.iFldY];
	}
	double x=GetDbDouble(m_sd.v_srcIdx[m_sd.iFldX]);
	if(!x) {
		bDefX=true;
		*pLon=m_sd.lonDflt;
	}
	else {
		x=-getDeg(x);
		//x*=m_sd.v_fldScale[m_sd.iFldX];
	}

	if(bDefX!=bDefY) {
	   int dec=m_sd.iTypXY?2:6;
	   WriteLog("Partial coordinate set: %s=%.*f %s=%.*f.",
	      m_sd.v_fldDef[m_sd.iFldX].F_Nam, dec, x, m_sd.v_fldDef[m_sd.iFldY].F_Nam, dec, y);
	   *pLon=m_sd.lonErr; *pLat=m_sd.latErr;
	   return -1;
	}

	if(bDefX) return 0; //default location acceptable id both coordinates missing

	int iDatum=(m_sd.iFldDatum>=0 && !strcmp(m_sd.v_srcDflt[m_sd.iFldDatum],"NAD27"))?geo_NAD27_datum:geo_WGS84_datum;

	if(m_sd.iTypXY) {
	    //have utm coordinates
		int iZone=(m_sd.iFldZone>=0 && m_sd.v_srcIdx[m_sd.iFldZone]>=0)?GetDbLong(m_sd.v_srcIdx[m_sd.iFldZone]):0;
		if(!iZone) iZone=m_sd.iZoneDflt;
		if(!iZone || iZone<m_sd.zoneMin || iZone>m_sd.zoneMax) { 
			WriteLog("Bad or missing UTM zone (%d). EASTING_=%.2f _NORTHING_=%.2f.",iZone,x,y);
			*pLon=m_sd.lonErr; *pLat=m_sd.latErr;
			return -1;
		}
		geo_UTM2LatLon(iZone, x, y, pLat, pLon, iDatum);
		if(*pLat<m_sd.latMin || *pLat>m_sd.latMax || *pLon<m_sd.lonMin || *pLon>m_sd.lonMax) {
			*pLon=m_sd.lonErr; *pLat=m_sd.latErr;
			WriteLog("Location out of range: EASTING_=%.2f _NORTHING_=%.2f ZONE_=%u.",x,y,iZone);
			return -1;
		}
	}
	else {
		*pLat=y; *pLon=x;
		if(*pLat<m_sd.latMin || *pLat>m_sd.latMax || *pLon<m_sd.lonMin || *pLon>m_sd.lonMax) {
			*pLon=m_sd.lonErr; *pLat=m_sd.latErr;
			WriteLog("Location out of range: LATITUDE_=%.6f LONGITUDE_=%.6f. Error location used",y,x);
			return -1;
		}
	}

	//We're using the assigned location, converting to lat/lon, WGS84 --
	if(iDatum==geo_NAD27_datum)
		geo_FromToWGS84(false,pLat,pLon,geo_NAD27_datum);

	return 1;
}

static int __inline _getStrFieldCount(CString &s)
{
	int len=1;
	for(int i=1; (i=s.Find(pFldSep,i))>=0; i+=3) len++;
	return len;
}

BOOL CXlsExportDlg::GetCsvRecord()
{
	CString s;
	if(!m_csvfile.ReadString(s) || s[0]!='"') return FALSE;

	int f,nFlds=m_sd.numDbFlds;
	while((f=_getStrFieldCount(s))<nFlds || f==nFlds && s[s.GetLength()-1]!='"') {
		CString s0;
		if(!m_csvfile.ReadString(s0)) return FALSE;
		s+="\r\n"; s+=s0;
	}
	int iLast=s.GetLength()-1;
	if(f>nFlds || s[iLast]!='"') return -1;  //too many fields found! 

	int i,iNext;
	f=0;
	for(i=1; f<nFlds && ((iNext=s.Find(pFldSep,i))>=0 || (f==nFlds-1 && (iNext=iLast)!=0)); i=iNext+3,f++) {

		m_vcsv[f].SetString((LPCSTR)s+i,iNext-i);
		m_vcsv[f].Trim();
	}
	if(f!=nFlds) return -1;
	return TRUE;
}

void CXlsExportDlg::InitFldKey()
{
	int f=m_sd.v_srcIdx[m_sd.iFldKey];
	DBF_FLDDEF &fd=m_sd.v_fldDef[m_sd.iFldKey];
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
	DDX_Text(pDX, IDC_SHAPENAME, m_shpName);
	DDV_MaxChars(pDX, m_shpName, 50);
	DDX_Text(pDX, IDC_TABLENAME, m_tableName);
	DDV_MaxChars(pDX, m_tableName, 50);
	DDX_Text(pDX, IDC_PATHNAME, m_dbPath);
	DDV_MaxChars(pDX, m_dbPath, 260);

	if(pDX->m_bSaveAndValidate) {
		CWaitCursor wait;

		m_dbPath.Trim();
		if(m_dbPath.IsEmpty()) {
			pDX->m_idLastControl=IDC_PATHNAME;
			AfxMessageBox("A source pathname is required.");
			pDX->Fail();
			return;
		}

		m_nPathLen=trx_Stpext(m_dbPath)-(LPCSTR)m_dbPath;
		LPCSTR pExt=(LPCSTR)m_dbPath+m_nPathLen;
		m_bXLS=!_stricmp(pExt,".xls") || !_stricmp(pExt,".xlsx");

		if(!m_bXLS && _stricmp(pExt,".mdb")) {
			if(!_stricmp(pExt,".csv")) m_bXLS=2;
			else {
				pDX->m_idLastControl=IDC_PATHNAME;
				AfxMessageBox("Source extension must be one of .xls, .mdb, and .csv.");
				pDX->Fail();
				return;
			}
		}

		m_pdbName=trx_Stpnam(m_dbPath);
		m_pathBuf=dos_FullPath(m_dbPath,NULL);
		m_nPathLen=trx_Stpnam(m_pathBuf)-(LPCSTR)m_pathBuf;

		if(_access(m_pathBuf, 4)) {
			pDX->m_idLastControl=IDC_PATHNAME;
			CMsgBox("Source file %s is absent or inaccessible.",m_pdbName);
			pDX->Fail();
			return;
		}

		if(m_bXLS<2 && m_tableName.IsEmpty()) {
			pDX->m_idLastControl=IDC_TABLENAME;
			AfxMessageBox("A table or sheet name is required.");
			pDX->Fail();
			return;
		}

		if(m_shpName.IsEmpty()) {
			pDX->m_idLastControl=IDC_SHAPENAME;
			AfxMessageBox("A shapefile base name is required.");
			pDX->Fail();
			return;
		}

		if(m_bXLS>1) {
			if(!m_csvfile.Open(m_pathBuf, CFile::modeRead)) {
				CMsgBox("File %s missing or can't be opened.", m_pdbName);
				pDX->m_idLastControl=IDC_PATHNAME;
				pDX->Fail();
				return;
			}
			CString s;
			int iLast,nFlds=0;
			//can fix this section, getStrFieldCount(), and GetCsvRecord() to not require quotes, use tabs, etc --
			if(!m_csvfile.ReadString(s) || s[0]!='"')
			   goto _noFlds;
			iLast=s.GetLength()-1;
			if(iLast<1)
			   goto _noFlds;
			char cLast=s[iLast];
			if(cLast!='"')
			   goto _noFlds;
			nFlds=_getStrFieldCount(s);
			if(!nFlds)
			   goto _noFlds;
			//now retrieve field names --
			m_sd.v_srcNames.assign(nFlds, CString());
			int iNext,f=0;
			for(int i=1; f<nFlds && ((iNext=s.Find(pFldSep,i))>=0 || (f==nFlds-1 && (iNext=iLast)!=0)); i=iNext+3,f++) {
				m_sd.v_srcNames[f].SetString((LPCSTR)s+i,iNext-i);
				m_sd.v_srcNames[f].Trim();
			}
			ASSERT(f==nFlds);
			m_sd.numDbFlds=nFlds;

_noFlds:
			if(!nFlds) {
				CMsgBox("No field names found in first line of %s.",m_pdbName);
				m_csvfile.Close();
				pDX->m_idLastControl=IDC_PATHNAME;
				pDX->Fail();
				return;
			}
		}
		else {
			try
			{
				m_database.Open(m_pathBuf,TRUE,TRUE,m_bXLS?"Excel 5.0":"");
				ASSERT(m_database.IsOpen());
				CString table;
				table.Format("[%s%s]",(LPCSTR)m_tableName,m_bXLS?"$":"");
				m_rs.m_pDatabase=&m_database;
				m_rs.Open(dbOpenTable,table,dbReadOnly);
				m_sd.numDbFlds=m_rs.GetFieldCount();
			}
			catch (CDaoException* e)
			{
				e->Delete();
				CString s;
				BOOL bOpen=m_database.IsOpen();
				if(bOpen) s.Format("Unable to open table \"%s\" in %s.",(LPCSTR)m_tableName,m_pdbName);
				else s.Format("Unable to access source: %s",m_pdbName);
				if(m_rs.IsOpen()) m_rs.Close();
				AfxMessageBox(s);
				pDX->m_idLastControl=IDC_PATHNAME;
				pDX->Fail();
				return;
			}
		}

		if(!m_sd.Process((m_bXLS<2)?&m_rs:NULL,FixPath(m_shpName,".shpdef"))) {
			CloseDB();
			EndDialog(IDCANCEL);
			return;
		}
	
		if(InitShapefile()) {
			CloseDB();
			AfxMessageBox("Error initializing shapefile");
			EndDialog(IDCANCEL);
			return;
		}

		UINT nLocErrors=0,nLocMissing=0,nTruncated=0;
		xlsline=numRecs=0;

		try {
			BOOL bNotEOF;
			if(m_bXLS>1) {
				m_vcsv.assign(m_sd.numDbFlds,CString());
			    bNotEOF=GetCsvRecord();
			}
			else bNotEOF=!m_rs.IsEOF();

			while(bNotEOF>0) {
				xlsline++;

				if(m_sd.iFldKey>=0) InitFldKey();

				double lat,lon,east,north;
				int zone=GetCoordinates(&lat,&lon);
				if(zone<=0) {
					if(zone<0) nLocErrors++;
					else nLocMissing++;
				}
				if(m_sd.uFlags&SHPD_UTMFLAGS) {
					zone=0;
					geo_LatLon2UTM(lat,lon,&zone,&east,&north,geo_WGS84_datum);
				}

				if(WriteShpRec(lat,lon)) {
					CString msg;
					msg.Format("Aborted: Error writing shapefile record %u.", xlsline);
					WriteLog(msg);
					AfxMessageBox(msg);
					break;
				}

				for(int f=0; f<m_sd.numFlds; f++) {
					DBF_FLDDEF &fd=m_sd.v_fldDef[f];
					int n=m_sd.GetLocFldTyp(fd.F_Nam)-1;
					if(n>=LF_LAT && n<=LF_DATUM) {
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
							case LF_DATUM: StoreText(f, "WGS84");
							break;
						}
						continue;
					}

					//normal field --

					int len=StoreFldData(f); //suuports only N, C, M, and L for now
					if(len<0) {
						CString msg;
						if(fd.F_Typ=='C') {
							msg.Format("Character data in field %s (length %u) truncated to %u.", fd.F_Nam, fd.F_Len-len, fd.F_Len );
						}
						else if(fd.F_Typ=='M') {
							msg.Format("Field %s memo (length %u) couldn't be stored.", fd.F_Nam,fd.F_Len);
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
				}
				else bNotEOF=GetCsvRecord();
			}
			if(bNotEOF<0) {
				CMsgBox("Reading terminated after %u records due an unrecognized line format.",numRecs);
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

				msg.Format("Shapefile with %u records created.\nLocations missing: %u, Location errors: %u, Truncated fields: %u",
				 numRecs,nLocMissing,nLocErrors,nTruncated);

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
		if(m_fbLog.IsOpen()) {
			m_fbLog.Close();
			if((int)::ShellExecute(NULL, "Open", csLogPath, NULL, NULL, SW_NORMAL)<=32) {
				CMsgBox("Couldn't open log %s automatically.", csLogPath);
			}
		}
	}
}

BEGIN_MESSAGE_MAP(CXlsExportDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BROWSE, OnBnClickedBrowse)
END_MESSAGE_MAP()


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
	if(!AddFilter(strFilter,IDS_XLS_FILES) || !AddFilter(strFilter,IDS_MDB_FILES) || !AddFilter(strFilter,IDS_CSV_FILES)) return;
	CString s(m_dbPath);
	if(DoPromptPathName(s,OFN_OVERWRITEPROMPT,3,strFilter,TRUE,IDS_XLS_EXPORT,".xls")) {
		bool bSkip=false;
		LPCSTR p=trx_Stpext(s);
		if(stricmp(p, ".xls") && stricmp(p, ".xlsx") && stricmp(p, ".mdb") && !(!stricmp(p, ".csv") && (bSkip=true))) {
			s.Truncate(p-(LPCSTR)s);
			s+=".xls";
		}
		GetDlgItem(IDC_PATHNAME)->SetWindowText(s);
		s.Truncate(s.GetLength()-4); 
		GetDlgItem(IDC_SHAPENAME)->SetWindowText(p=trx_Stpnam(s));
		GetDlgItem(IDC_TABLENAME)->SetWindowText(bSkip?"":p);
	}
}

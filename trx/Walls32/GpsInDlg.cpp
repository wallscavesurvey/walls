// GpsInDlg.cpp : implementation file
//

#include "stdafx.h"
#include "walls.h"
#include "prjdoc.h"
#include "prjview.h"
#include "GpsInDlg.h"

enum e_gpsopts {GPS_PORTMASK=15,GPS_GETWP=16,GPS_GETTRK=32,GPS_LOCALTIME=64,GPS_WRITECSV=128,GPS_RESTRICT=256};
//enum e_gpsopts {GPS_PORTMASK=7,GPS_GETWP=8,GPS_GETTRK=16,GPS_LOCALTIME=32,GPS_WRITECSV=64,GPS_RESTRICT=128};

static const TCHAR szGPSParams[] = _T("GPSParams");
static const TCHAR szGPSOptions[] = _T("Options2");
//static const TCHAR szGPSOptions[] = _T("Options");
static const TCHAR szGPSWpt[] = _T("Wpt");
static const TCHAR szGPSDist[] = _T("Dist");
static const TCHAR szGPSTrkDist[] = _T("TrkDst");
//Initially COM1, both waypoints and tracks --
#define GPS_DFLT_OPTIONS (GPS_LOCALTIME+GPS_GETTRK+GPS_GETWP)

enum e_errmsg {
	GPS_ERR_NONE=0,
	GPS_ERR_ALLOC,
	GPS_ERR_SENDRECORD,
	GPS_ERR_NOWPEOF,
	GPS_ERR_NOTRKEOF,
	GPS_ERR_PROTOCOL,
	GPS_ERR_NORESPOND,
	GPS_ERR_PROTVIOL,
	GPS_ERR_PROTUNK,
	GPS_ERR_MAXNAKS,
	GPS_ERR_NUMRECS,
	GPS_ERR_BADWP,
	GPS_ERR_PORTOUT,
	GPS_ERR_ZONECOUNT,
	GPS_ERR_NODATA,
	GPS_ERR_SRVCREATE,
	GPS_ERR_PRJCREATE,
	GPS_ERR_CSVCREATE,
	GPS_ERR_CANCELLED
};

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

static CGpsInDlg *pDLG;
static BOOL bImporting;
static BOOL bCancelled;

static int PASCAL import_CB(LPCSTR msg)
{
	if(msg) pDLG->GetDlgItem(IDC_ST_TESTCOM)->SetWindowText(msg);
	Pause();

	if(!bCancelled) return 0;
	//Cancelled connection or cancelled download --
	bCancelled=FALSE;
    if(msg && *msg!='O') {
	   //return 1 to accept incomplete download
		if(IDYES==CMsgBox(MB_YESNO,IDS_GPS_PARTIALOK)) {
			return 1;
		}
    }
    return -1; //Don't write project
}

/////////////////////////////////////////////////////////////////////////////
// CGpsInDlg dialog


CGpsInDlg::CGpsInDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CGpsInDlg::IDD, pParent)
{
    pDLG=this;
	bImporting=FALSE;
	m_hinst=NULL;
	//{{AFX_DATA_INIT(CGpsInDlg)
	//}}AFX_DATA_INIT
}

CGpsInDlg::~CGpsInDlg()
{
    if(m_hinst) ::FreeLibrary(m_hinst);
}

BOOL CGpsInDlg::CheckPathname()
{
	int i;
	char buf[_MAX_PATH];
	
	m_Pathname.Trim();
	if(!*m_Pathname) {
		AfxMessageBox(IDS_ERR_NOGPSPATH);
		return FALSE;
	}
	LPCSTR pExt=trx_Stpext(m_Pathname);
	if(strcmp(pExt,".wpj")) {
		m_Pathname.Truncate(pExt-(LPCSTR)m_Pathname);
		m_Pathname+=".wpj";
	}
	if(!_fullpath(buf,m_Pathname,_MAX_PATH)) {
		AfxMessageBox(IDS_ERR_BADPATH);
		return FALSE;
	}
	
	m_Pathname=buf;
	GetDlgItem(IDC_IMPPRJPATH)->SetWindowText(buf);

	CPrjDoc *pDoc=theApp.FindOpenProject(buf);
	if(pDoc && !pDoc->SaveModified()) return FALSE;

	//Now let's see if the directory needs to be created --
	if(!(i=DirCheck(buf,TRUE))) return FALSE;
	if(i!=2) {
		//The directory exists. Check for file existence --
		if(_access(buf,6)!=0) {
			if(errno!=ENOENT) {
				//file not rewritable
				AfxMessageBox(IDS_ERR_FILE_RW);
				return FALSE;
			}
			//The project file itself doesn't exist but what about proj-xxx.SRV?
			char *p=trx_Stpnam(buf);
			for(i=0;i<4;i++,p++) if(*p=='.' || *p==' ') break;
			strcpy(p,"-???.srv");
			if(FilePatternExists(buf) &&
				IDYES!=CMsgBox(MB_YESNO,IDS_GPS_OVERWRITE1,p-i)) return FALSE;
		}
		else {
		   //File exists and is rewritable.
			if(IDYES!=CMsgBox(MB_YESNO,pDoc?IDS_GPS_PRJOPEN:IDS_GPS_OVERWRITE)) return FALSE;
		   if(pDoc) {
			   ((CMainFrame *)theApp.m_pMainWnd)->OnWindowCloseAll();
			   pDoc->GetPrjView()->SendMessage(WM_COMMAND,ID_FILE_CLOSE);
			   Pause();
		   }
		}
	}
	return TRUE;
}

int CGpsInDlg::CheckData()
{

	if(m_bRestrict) {
		if(!CheckInt(m_wptDist,&m_gps_area.uLimitDist,0,10000000)) return IDC_WPTDIST;
		m_wayPoint.Trim();
		strcpy(m_gps_area.name,m_wayPoint);
		if(!m_gps_area.name[0]) {
			AfxMessageBox(IDS_ERR_WPTNAME);
			return IDC_WAYPOINT;
		}
	}
	if(!CheckInt(m_trkDist,&m_gps_area.uTrkDist,0,10000000)) return IDC_TRKBREAK;
	if(!CheckPathname()) return IDC_IMPPRJPATH;
	return 0;
}

void CGpsInDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGpsInDlg)
	DDX_Text(pDX, IDC_IMPPRJPATH, m_Pathname);
	DDV_MaxChars(pDX, m_Pathname, 250);
	DDX_CBIndex(pDX, IDC_COMPORT, m_iComPort);
	DDX_Radio(pDX, IDC_WAYPOINTS, m_iWaypoints);
	DDX_Check(pDX, IDC_LOCALTIME, m_bLocalTime);
	DDX_Check(pDX, IDC_WRITECSV, m_bWriteCSV);
	DDX_Text(pDX, IDC_WPTDIST, m_wptDist);
	DDV_MaxChars(pDX, m_wptDist, 8);
	DDX_Text(pDX, IDC_WAYPOINT, m_wayPoint);
	DDV_MaxChars(pDX, m_wayPoint, 15);
	DDX_Check(pDX, IDC_RESTRICT, m_bRestrict);
	DDX_Text(pDX, IDC_TRKBREAK, m_trkDist);
	DDV_MaxChars(pDX, m_trkDist, 8);
	//}}AFX_DATA_MAP

	if(!pDX->m_bSaveAndValidate) {
	    //Load DLL --
	    m_hinst=LoadLibrary(GPS_DLL_NAME); //EXP_DLL_NAME
	    m_pfnImport=NULL;
		if(m_hinst) {
			m_pfnImport=(LPFN_GPS_IMPORT)GetProcAddress(m_hinst,GPS_DLL_IMPORT);
			m_pfnErrmsg=(LPFN_GPS_ERRMSG)GetProcAddress(m_hinst,GPS_DLL_ERRMSG);
		}
		if(!m_pfnImport || !m_pfnErrmsg) {
	    	  CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_IMPLOADDLL1,GPS_DLL_NAME);
			  EndDialog(IDCANCEL);
		}
		GetDlgItem(IDC_WAYPOINT)->EnableWindow(m_bRestrict);
		GetDlgItem(IDC_WPTDIST)->EnableWindow(m_bRestrict);
	}
	else {
		int e=CheckData();
		if(e) {
	       pDX->m_idLastControl=e;
		   //***7.1 pDX->m_hWndLastControl=GetDlgItem(e)->m_hWnd;
		   pDX->m_bEditLastControl=TRUE;
		   pDX->Fail();
		}
	}
}

BEGIN_MESSAGE_MAP(CGpsInDlg, CDialog)
	//{{AFX_MSG_MAP(CGpsInDlg)
	ON_BN_CLICKED(IDC_TESTCOM, OnTestCom)
	ON_WM_CTLCOLOR()
	ON_BN_CLICKED(IDBROWSE, OnBrowse)
	ON_BN_CLICKED(ID_STARTDL, OnStartDL)
	ON_BN_CLICKED(IDC_RESTRICT, OnRestrict)
	//}}AFX_MSG_MAP
    ON_MESSAGE(WM_COMMANDHELP,OnCommandHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGpsInDlg message handlers

void CGpsInDlg::ShowConnecting()
{
	Enable(IDC_TESTCOM,FALSE);
	Enable(ID_STARTDL,FALSE);
	Enable(IDBROWSE,FALSE);
	GetDlgItem(IDC_ST_TESTCOM)->SetWindowText("Connecting...");
	bImporting=TRUE;
	bCancelled=FALSE;
	Pause();
}

void CGpsInDlg::OnTestCom() 
{
	m_iComPort=((CComboBox *)GetDlgItem(IDC_COMPORT))->GetCurSel();
	ShowConnecting();
	int e=m_pfnImport(NULL,NULL,m_iComPort,(LPFN_GPS_CB)import_CB);
	bCancelled=bImporting=FALSE;
	Enable(IDC_TESTCOM,TRUE);
	Enable(ID_STARTDL,TRUE);
	Enable(IDBROWSE,TRUE);
	if(e) {
		if(e==GPS_ERR_CANCELLED) EndDialog(IDCANCEL);
		else {
			GetDlgItem(IDC_ST_TESTCOM)->SetWindowText(m_pfnErrmsg(e));
			//CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_GPSTIME1,m_pfnErrmsg(e));
		}
	}
}

void CGpsInDlg::LoadGPSParams()
{
	UINT flags=(UINT)AfxGetApp()->GetProfileInt(szGPSParams,szGPSOptions,GPS_DFLT_OPTIONS);
	m_iComPort=(flags&GPS_PORTMASK);
	m_bLocalTime=((flags&GPS_LOCALTIME)!=0);
	m_bWriteCSV=((flags&GPS_WRITECSV)!=0);
	m_iWaypoints=((GPS_GETWP&flags)!=0)+2*((GPS_GETTRK&flags)!=0);
	if(m_iWaypoints) m_iWaypoints--;
	m_bRestrict=((flags&GPS_RESTRICT)!=0);
	m_wptDist=GetIntStr(AfxGetApp()->GetProfileInt(szGPSParams,szGPSDist,0));
	m_wayPoint=AfxGetApp()->GetProfileString(szGPSParams,szGPSWpt,NULL);
	m_trkDist=GetIntStr(AfxGetApp()->GetProfileInt(szGPSParams,szGPSTrkDist,5000));
}

UINT CGpsInDlg::GetFlags()
{
	UINT flags=((m_iWaypoints+1)&3);
	return m_iComPort+(flags&1)*GPS_GETWP+(flags>>1)*GPS_GETTRK+
		m_bLocalTime*GPS_LOCALTIME+m_bWriteCSV*GPS_WRITECSV+m_bRestrict*GPS_RESTRICT;
}

void CGpsInDlg::SaveGPSParams()
{
	AfxGetApp()->WriteProfileInt(szGPSParams,szGPSOptions,GetFlags());
	AfxGetApp()->WriteProfileInt(szGPSParams,szGPSTrkDist,atoi(m_trkDist));
	if(m_bRestrict) {
		AfxGetApp()->WriteProfileInt(szGPSParams,szGPSDist,atoi(m_wptDist));
		AfxGetApp()->WriteProfileString(szGPSParams,szGPSWpt,m_wayPoint);
	}
}

BOOL CGpsInDlg::OnInitDialog() 
{
	LoadGPSParams();

	CDialog::OnInitDialog(); //Calls DoDataExchange()
	
	// TODO: Add extra initialization here
	
	return TRUE;
}

HBRUSH CGpsInDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
	HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
    if(pWnd->GetDlgCtrlID()==IDC_ST_TESTCOM) {
		pDC->SetTextColor(RGB_DKRED);
    }
	return hbr;
}

void CGpsInDlg::OnBrowse() 
{
	BrowseFiles((CEdit *)GetDlgItem(IDC_IMPPRJPATH),
		m_Pathname,PRJ_PROJECT_EXTW,IDS_PRJ_FILES,IDS_GPS_PRJPATH);
}

void CGpsInDlg::OnCancel() 
{
    bCancelled=TRUE;
    if(!bImporting) CDialog::OnCancel();
}

void CGpsInDlg::OnStartDL() 
{
	if(UpdateData(TRUE)) {
		ASSERT(m_hinst);
		//Download data --
		if(!m_bRestrict) m_gps_area.uLimitDist=-1;
		ShowConnecting();
		int e=m_pfnImport(m_Pathname,&m_gps_area,
			GetFlags(),(LPFN_GPS_CB)import_CB);
		if(e) {
			if(e!=GPS_ERR_CANCELLED) CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_GPSABORT1,m_pfnErrmsg(e));
			EndDialog(IDCANCEL);
		}
		else EndDialog(IDOK);
		SaveGPSParams();
	}
}

LRESULT CGpsInDlg::OnCommandHelp(WPARAM wNone, LPARAM lParam)
{
	AfxGetApp()->WinHelp(25,HELP_CONTEXT);
	return TRUE;
}

void CGpsInDlg::OnRestrict() 
{
	m_bRestrict=(IsDlgButtonChecked(IDC_RESTRICT)!=0);
	GetDlgItem(IDC_WAYPOINT)->EnableWindow(m_bRestrict);
	GetDlgItem(IDC_WPTDIST)->EnableWindow(m_bRestrict);
}

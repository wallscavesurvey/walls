// GPSPortSettingDlg.cpp : implementation file
//

#include "stdafx.h"
#include "WallsMap.h"
#include "MainFrm.h"
#include "WallsMapDoc.h"
#include "ShpLayer.h"
#include "GPSPortSettingDlg.h"

// CGPSPortSettingDlg dialog

IMPLEMENT_DYNAMIC(CGPSPortSettingDlg, CDialog)

//Global --
CGPSPortSettingDlg *pGPSDlg=NULL;
VEC_GPSFIX vptGPS;

//crMrk(RGB(0,0,0)),crBkg(RGB(255,0,0)),wFilled(1),wShape(0),wSize(9),fLineWidth(1),iOpacitySym(100),iOpacityVec(100)

SHP_MRK_STYLE CGPSPortSettingDlg::m_mstyle_hD(RGB(215,0,0),RGB(0,0,0),0,1,25,2.5f); //red circle on top
SHP_MRK_STYLE CGPSPortSettingDlg::m_mstyle_sD(RGB(215,0,0),RGB(0,0,0),0,3,21,2.5f); //red cross
SHP_MRK_STYLE CGPSPortSettingDlg::m_mstyle_tD(RGB(215,0,0),RGB(0,0,0),0,3, 1,1.6f,70); //trackline

SHP_MRK_STYLE CGPSPortSettingDlg::m_mstyle_h=CGPSPortSettingDlg::m_mstyle_hD;
SHP_MRK_STYLE CGPSPortSettingDlg::m_mstyle_s=CGPSPortSettingDlg::m_mstyle_sD;
SHP_MRK_STYLE CGPSPortSettingDlg::m_mstyle_t=CGPSPortSettingDlg::m_mstyle_tD;

bool CGPSPortSettingDlg::m_bGPSMarkerChg=false;
bool CGPSPortSettingDlg::m_bMarkerInitialized=false;
WORD CGPSPortSettingDlg::m_wFlags=GPS_FLG_AUTORECORD+(GPS_FLG_AUTOCENTER-1); //(m_wflags&GPS_FLG_AUTOCENTER) == 0, 1, 2 (% choices in dialog)
int CGPSPortSettingDlg::m_nConfirmCnt=50;

CGPSPortSettingDlg::CGPSPortSettingDlg(CWnd* pParent /*=NULL*/)
	: CDialog()
	, m_bPortOpened(false)
	, m_bHavePos(false)
	, m_bHaveGPS(false)
	, m_bWindowDisplayed(false)
	, m_bPortAvailable(false)
	, m_nLastSaved(0)
	, m_bRecordingGPS((m_wFlags&GPS_FLG_AUTORECORD)!=0)
	, m_bLogPaused(false)
	, m_bLogStatusChg(false)
{
	m_pSerial=NULL;
	m_iBaud=CMainFrame::m_iBaudDef;
	m_nPort=CMainFrame::m_nPortDef;
	m_pMF=GetMF();

	VERIFY(Create(CGPSPortSettingDlg::IDD,pParent));
}

CGPSPortSettingDlg::~CGPSPortSettingDlg()
{
	{
	VEC_GPSFIX().swap(vptGPS);
	}
	delete m_pSerial;
	pGPSDlg=NULL;
}

void CGPSPortSettingDlg::OnClose() 
{
	//X icon at top-right
	if(ConfirmClearLog()) DestroyWindow(); //CDialog::OnCancel(); ?
}

void CGPSPortSettingDlg::PostNcDestroy() 
{
	ASSERT(pGPSDlg==this);
	if(m_bPortOpened) {
		m_bHavePos=m_bHaveGPS=false;
		m_pMF->EnableGPSTracking();
		m_pSerial->Close();

		if(m_nPort!=CMainFrame::m_nPortDef || m_iBaud!=CMainFrame::m_iBaudDef) {
			CMainFrame::m_nPortDef=m_nPort;
			CMainFrame::m_iBaudDef=m_iBaud;
			m_wFlags|=GPS_FLG_UPDATED;
		}
	}
	delete this;
}

void CGPSPortSettingDlg::OnCancel() 
{
	OnClose(); //cancel button
}

void CGPSPortSettingDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PORT, m_cbPort);
	DDX_Control(pDX, IDC_BAUD, m_cbBaud);

	if(!pDX->m_bSaveAndValidate) {
		char buf[256]; //must be >128 to see GPSGate ports bizVSerial1 and bizVSerial2
		CString s;
		int iSel=-1,iInitSel=-1;
		for(int i=0;i<GPS_MAX_PORTS;i++) {
			s.Format("COM%u",i+1);
			buf[0]=0;
			//Will fail with bizVSerial1 if only 128 bytes specified, though not this much used!
			DWORD e=QueryDosDevice(s,buf,255);
			if(!e) {
				e=GetLastError();
				//LastErrorBox();
				continue;
			}
			LPCSTR p=buf+strlen(buf);
			if(p>buf) {
				while(p>buf && p[-1]!='\\') p--;
			}
			if(p==buf) s.Format("%2u",i+1);
			else s.Format("%2u (%s)",i+1,p);
			iSel=m_cbPort.AddString((LPCSTR)s);
			if(iInitSel<0 && i==m_nPort-1) iInitSel=iSel;
		}
		if(iInitSel<0) {
			if((iInitSel=iSel)<0) {
				AfxMessageBox("No COM ports available!");
				EndDialog(IDCANCEL);
				return;
			}
			iInitSel=0;
		}
		m_cbPort.SetCurSel(iInitSel);
		m_cbPort.GetLBText(iInitSel,s);
		m_nPort=atoi(s);
		m_strComPort.Format("\\\\.\\COM%u",m_nPort);
		m_cbBaud.SetCurSel(m_iBaud);
		if(!m_bRecordingGPS) {
			Show(IDC_ST_TRACKLBL,false);
			Show(IDC_ST_TRACKCNT,false);
			Show(IDC_PAUSELOG,false);
			Show(IDC_SAVELOG,false);
			Show(IDC_CLEARLOG,false);
		}
	}
	else {
		CString s;
		m_nPort=m_cbPort.GetCurSel();
		ASSERT(m_nPort>=0);
		m_cbPort.GetLBText(m_nPort,s);
		m_nPort=atoi(s);
		m_iBaud=m_cbBaud.GetCurSel();
	}
}

BEGIN_MESSAGE_MAP(CGPSPortSettingDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_CTLCOLOR()
	ON_WM_CLOSE()
	ON_WM_WINDOWPOSCHANGED()
	ON_MESSAGE(WM_APP,OnWindowDisplayed)
	ON_CBN_SELCHANGE(IDC_PORT,OnPortChange)
	ON_BN_CLICKED(IDC_TEST, OnBnClickedTest)
	ON_BN_CLICKED(IDC_COPYLOC,OnBnClickedCopyloc)
	ON_WM_SERIAL(OnSerialMsg)
	ON_BN_CLICKED(IDC_MINIMIZE,OnBnClickedMinimize)
	ON_BN_CLICKED(IDC_CLEARLOG, &CGPSPortSettingDlg::OnBnClickedClearlog)
	ON_BN_CLICKED(IDC_PAUSELOG, &CGPSPortSettingDlg::OnBnClickedPauselog)
	ON_BN_CLICKED(IDC_SAVELOG, &CGPSPortSettingDlg::OnBnClickedSavelog)
	ON_BN_CLICKED(IDCANCEL, &CGPSPortSettingDlg::OnBnClickedCancel)
END_MESSAGE_MAP()

HBRUSH CGPSPortSettingDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
	HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
    switch(pWnd->GetDlgCtrlID()) {
		case IDC_ST_POSITION:
		case IDC_ST_TRACKCNT:
		case IDC_ST_ALT:
		case IDC_ST_HDOP:
		case IDC_ST_SATSUSED:
			pDC->SetTextColor(RGB(0x80,0,0));
			break;
    }
	return hbr;
}

void CGPSPortSettingDlg::OnBnClickedCopyloc()
{
	CString s;
	GetDlgItem(IDC_ST_POSITION)->GetWindowText(s);
	int i=s.Find('(');
	if(i>0) s.Truncate(i-1);
	VERIFY(ClipboardCopy(s)); 
}

void CGPSPortSettingDlg::ShowPortMsg(LPCSTR p)
{
	CString s;
	s.Format("COM%u%s",m_nPort,p);
	SetStatusText(s);
}

void CGPSPortSettingDlg::OnPortChange()
{
	ASSERT(!m_bPortOpened);
	int nPort=m_cbPort.GetCurSel();
	ASSERT(nPort>=0);
	CString s;
	m_cbPort.GetLBText(nPort,s);
	nPort=atoi(s);

	if(nPort!=m_nPort) {
		m_strComPort.Format("\\\\.\\COM%u",m_nPort=nPort);
		ShowPortStatus(PortStatus());
	}
}

void CGPSPortSettingDlg::ShowPortStatus(CSerial::EPort e)
{
	m_bPortAvailable=false;
	LPCSTR p=" can't be opened.";
	switch(e) {
		case CSerial::EPortAvailable :
			p=" is available but unopened.";
			SetText(IDC_TEST,"Open Port");
			m_bPortAvailable=true;
			break;
		case CSerial::EPortNotAvailable :
			p=" doesn't exist.";
			break;
		case CSerial::EPortInUse :
			p=" is currently in use.";
			break;
	}
	ShowPortMsg(p);
	Enable(IDC_TEST,m_bPortAvailable);
}

CSerial::EPort CGPSPortSettingDlg::PortStatus()
{
	CString s;
	s.Format("Checking COM%u status...",m_nPort);
	SetStatusText(s);
	BeginWaitCursor();
	CSerial::EPort e=CSerial::CheckPort((LPCTSTR)m_strComPort);
	EndWaitCursor();
	return e;
}

void CGPSPortSettingDlg::OnBnClickedMinimize()
{
	ShowWindow(SW_MINIMIZE);
}

void CGPSPortSettingDlg::OnBnClickedTest()
{
	if(m_bPortOpened) {
		if(!ConfirmClearLog()) return;
		//Cancelling test --
		ASSERT(m_pSerial);
		Enable(IDC_TEST,0);
		Enable(IDC_COPYLOC,0);
		SetStatusText("Closing port...");
		m_bHavePos=m_bHaveGPS=false;
		m_pMF->EnableGPSTracking();
		m_pSerial->Close();
		m_bPortOpened=m_bHavePos=m_bHaveGPS=false;
		vptGPS.clear();
		m_nLastSaved=0;
		if(m_bRecordingGPS) {
			Show(IDC_ST_TRACKLBL,false);
			Show(IDC_ST_TRACKCNT,false);
			Enable(IDC_PAUSELOG,0);
			Enable(IDC_SAVELOG,0);
			Enable(IDC_CLEARLOG,0);
		}
		SetText(IDC_ST_POSITION,"");
		SetText(IDC_ST_ALT,"");
		SetText(IDC_ST_HDOP,"");
		SetText(IDC_ST_SATSUSED,"");
		SetText(IDC_ST_GROUP,"");
		SetText(IDC_TEST,"Open Port");
		ShowPortStatus(CSerial::EPortAvailable); //Assume port still available
		Enable(IDC_COPYLOC,0);
		m_cbPort.EnableWindow();
		m_cbBaud.EnableWindow();
		return;
	}

	ASSERT(m_bPortAvailable);
	//Open port and test communication
	if(!m_pSerial) m_pSerial=new CSerialMFC();
	else m_pSerial->ParserReset();
	m_bHavePos=m_bHaveGPS=false;
	SetStatusText("Opening port...");
	Enable(IDC_TEST,0);
	BeginWaitCursor();
	if(m_pSerial->Open(m_strComPort,(CSerial::EBaudrate)CMainFrame::Baud_Range[m_iBaud],this)==ERROR_SUCCESS) {
		EndWaitCursor();
		m_bPortOpened=true;
		m_cbPort.EnableWindow(0);
		m_cbBaud.EnableWindow(0);
		Enable(IDC_TEST,1);
		SetText(IDC_TEST,"Close Port");
		SetStatusText("Waiting for data...");
	}
	else {
		EndWaitCursor();
		ShowPortStatus(CSerial::EPortUnknownError);
	}
}

void CGPSPortSettingDlg::DisplayData()
{
	if(!m_bHaveGPS) {
		m_bHaveGPS=true;
		SetStatusText("Receiving GPS data...");
		if(m_bRecordingGPS) {
			Show(IDC_ST_TRACKLBL,true);
			Show(IDC_ST_TRACKCNT,true);
			Show(IDC_SAVELOG,true);
			Show(IDC_PAUSELOG,true);
			Show(IDC_CLEARLOG,true);
		}
	}

	CString s;

	if(m_bRecordingGPS) {
		Enable(IDC_CLEARLOG,vptGPS.size()>1);
		Enable(IDC_PAUSELOG,!vptGPS.empty());
		Enable(IDC_SAVELOG,vptGPS.size()>1);
		s.Format("%u",vptGPS.size());
		SetText(IDC_ST_TRACKCNT,s);
		s.Empty();
	}

	if(m_gps.wFlags&GPS_FTIME) {
		s.Format("Local time: %02u:%02u:%02u  ",m_gps.gpf.h,m_gps.gpf.m,m_gps.gpf.s);
	}

	SetText(IDC_ST_GROUP,s);

	if(m_bHavePos) {
		LPCSTR p="Unk";
		switch(m_gps.bQual) {
			case 1 : p="SPS"; break;
			case 2 : p="DGPS"; break;
			case 3 : p="PPS"; break;
		}
		CFltPoint fp(m_gps.gpf);
		s.Format("%f  %f  (%s)",fp.y,fp.x,p);
		SetText(IDC_ST_POSITION,s);

		if(m_gps.wFlags&GPS_FALT) s.Format("%.1f m",m_gps.gpf.elev);
		else s.Empty();
		SetText(IDC_ST_ALT,s);
		{
			CString sv;
			if(m_gps.bSatsInView || !m_gps.bSatsUsed)
				sv.Format(" of %u in view",m_gps.bSatsInView);
			s.Format("%u%s",m_gps.bSatsUsed,(LPCSTR)sv);
			SetText(IDC_ST_SATSUSED,s);
		}
		s.Format("%.2f",m_gps.fHdop);
		SetText(IDC_ST_HDOP,s);
		Enable(IDC_COPYLOC,1);
	}
	else {
		SetText(IDC_ST_POSITION,"Waiting for fix...");
		s.Format("0 of %u in view",m_gps.bSatsInView);
		SetText(IDC_ST_SATSUSED,s);
		s.Format("%.2f",m_gps.fHdop);
		SetText(IDC_ST_HDOP,"");
		Enable(IDC_COPYLOC,0);
	}
}

void CGPSPortSettingDlg::InitGPSMarker()
{
	m_bMarkerInitialized=true;
	CString s(theApp.GetProfileString(szPreferences,szGPSMrk,NULL));
	m_mstyle_s.ReadProfileStr(s);
	s=theApp.GetProfileString(szPreferences,szGPSMrkH,NULL);
	m_mstyle_h.ReadProfileStr(s);
	s=theApp.GetProfileString(szPreferences,szGPSMrkT,NULL);
	m_mstyle_t.ReadProfileStr(s);
}

LRESULT CGPSPortSettingDlg::OnSerialMsg (WPARAM wParam, LPARAM lParam)
{
	CSerial::EEvent eEvent = CSerial::EEvent(LOWORD(wParam));
	CSerial::EError eError = CSerial::EError(HIWORD(wParam));

	if (eError) {
		//m_pSerial->Close();
		//AfxMessageBox("Port closed - An internal error occurred!");
		return 0;
	}

	if(lParam) {
		// lock mutex for gp access!!
		DWORD dwWaitResult = WaitForSingleObject(m_pSerial->ghMutex,1000); //1 sec vs INFINTE
 		switch (dwWaitResult) 
		{
			// The thread got ownership of the mutex
			case WAIT_OBJECT_0:
				{
					m_gps=*(GPS_POSITION *)lParam;
					ReleaseMutex(m_pSerial->ghMutex);
					bool bHavePos=((m_gps.wFlags&(GPS_FPOS|GPS_FQUAL))==(GPS_FPOS|GPS_FQUAL)) && m_gps.bQual>0;
					if(bHavePos) {
						if(!m_bHavePos) {
							ASSERT(vptGPS.empty());
							if(!m_bMarkerInitialized)
								InitGPSMarker();
						}
						//For now --
						CGPSFix fpGPS(m_gps.gpf);
						if(vptGPS.empty() || vptGPS.back()!=fpGPS) {

							if(!m_bRecordingGPS) {
								//not recording track, just showing position --
								ASSERT(m_nLastSaved==0 && vptGPS.empty());
								vptGPS.clear();
								vptGPS.push_back(fpGPS);
							}
							else {
								if(vptGPS.size()>=MAX_GPS_POINTS) {
									//option to save?
									vptGPS.erase(vptGPS.begin(),vptGPS.begin()+MAX_GPS_POINTS/4);
								}
								if(m_bLogPaused) {
									ASSERT(!vptGPS.empty());
									if(m_bLogStatusChg || vptGPS.size()<2 ) {
										m_bLogStatusChg=false;
										if(vptGPS.empty()) {
											ASSERT(0);
											fpGPS.flag=1;
										}
										else vptGPS.back().flag=1;
										vptGPS.push_back(fpGPS); //track line not drawn to this point
									}
									else {
										ASSERT((vptGPS.end()-2)->flag==1);
										vptGPS.back()=fpGPS;
									}
								}
								else {
									m_bLogStatusChg=false;
									vptGPS.push_back(fpGPS);
								}
						    }
							if(!m_bHavePos) {
								m_bHavePos=true;
								m_pMF->EnableGPSTracking();
							}
							else m_pMF->UpdateGPSTrack();
						}
					}
					if(!IsIconic()) DisplayData();
				}
				break;

			case WAIT_TIMEOUT:
				ASSERT(0);
				break;

			case WAIT_FAILED:
				ASSERT(0); //happened on vaio with BT but no BT GPS
				break;

			// The thread got ownership of an abandoned mutex
			case WAIT_ABANDONED: 
				ASSERT(0);
				break;

			default: ASSERT(0);
		}
	}
	return 0;
}
void CGPSPortSettingDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	CDialog::OnSysCommand(nID,lParam);
	if(nID==SC_RESTORE)
		DisplayData();
}

void CGPSPortSettingDlg::OnWindowPosChanged(WINDOWPOS *lpwndpos)
{
    if((lpwndpos->flags & SWP_SHOWWINDOW) && !m_bWindowDisplayed) {
      m_bWindowDisplayed=true;
      PostMessage(WM_APP,0,0);
    }
}

LONG CGPSPortSettingDlg::OnWindowDisplayed(UINT, LONG)
{
	ASSERT(m_bWindowDisplayed);
	ShowPortStatus(PortStatus());
	return 0;
}

bool CGPSPortSettingDlg::ConfirmClearLog()
{
	int np=vptGPS.size();
	bool bRet=true;
	if(m_bRecordingGPS && m_nConfirmCnt<np-m_nLastSaved) {
		ShowWindow(SW_HIDE);
		bRet=(IDOK==CMsgBox(MB_OKCANCEL,"GPS track logging in progress...\n\nSelect OK to discard %u track points (%u unsaved).",np,np-m_nLastSaved));
		ShowWindow(SW_SHOW);
		if(!bRet && IsIconic()) ShowWindow(SW_RESTORE);
	}
	return bRet;
}

void CGPSPortSettingDlg::OnBnClickedClearlog()
{
	if(vptGPS.size()>1 && ConfirmClearLog()) {
		m_nLastSaved=0;
		CGPSFix gp(vptGPS.back());
		vptGPS.clear();
		vptGPS.push_back(gp);
		GetMF()->UpdateGPSTrack();
	}
}

void CGPSPortSettingDlg::OnBnClickedPauselog()
{
	ASSERT(m_bRecordingGPS);
	m_bLogPaused=!m_bLogPaused;
	SetText(IDC_PAUSELOG,m_bLogPaused?"Resume Logging":"Pause Logging");
	m_bLogStatusChg=true;
}

void CGPSPortSettingDlg::OnBnClickedSavelog()
{
	CString newPath;
	CWallsMapDoc *pDoc=GetMF()->FirstGeoRefSupportedDoc();
	if(pDoc) {
		LPCSTR p=(LPCSTR)pDoc->GetPathName();
		newPath.SetString(p,trx_Stpnam(p)-p);
	}
	CString name("GPS_");
	LPSTR pbuf=GetLocalTimeStr();
	if(pbuf) {
		//pbuf=="%4u-%02u-%02u %02u:%02u:%02u"
		pbuf[10]=0;
		name+=pbuf;
	}
	else name+="Track";
	name+=".shp";
	newPath+=name;
	CString strFilter;
	if(!AddFilter(strFilter,IDS_SHP_FILES)) return;

	ShowWindow(SW_HIDE);

_retry:
	if(!DoPromptPathName(newPath,OFN_HIDEREADONLY,1,strFilter,FALSE,IDS_GPS_SAVEAS,0))
	{
		ShowWindow(SW_SHOW);
		return;
	}

	ReplacePathExt(newPath,".shp");

	if(!CShpLayer::check_overwrite(newPath))
		goto _retry;

	int recs=GPSExportLog(newPath);
	if(recs>0) {
		m_nLastSaved=recs+1;
		strFilter.Format("Created %s with %u records.",(LPCSTR)name,recs);
		if(pDoc) {
		   name.Format("\n\nAdd layer to project %s?",(LPCSTR)pDoc->GetTitle());
		   strFilter+=name;
		}
		recs=CMsgBox(pDoc?MB_YESNO:MB_ICONINFORMATION,strFilter);
		if(recs==IDYES) {
			ASSERT(pDoc);
			pDoc->DynamicAddLayer(newPath);
		}
	}

	ShowWindow(SW_SHOW);
}

void CGPSPortSettingDlg::OnBnClickedCancel()
{
	OnClose();
}

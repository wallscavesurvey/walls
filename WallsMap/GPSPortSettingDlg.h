#pragma once
//#include "afxwin.h"
#include "SerialMFC.h"
#include "ComboBoxNS.h"

#define GPS_MAX_PORTS 41

class CMainFrame;
class CSerialMFC;

// CGPSPortSettingDlg dialog

#define MAX_GPS_POINTS 20000 //320K
enum {GPS_FLG_AUTOCENTER=3,GPS_FLG_AUTORECORD=4,GPS_FLG_UPDATED=0xF000};

class CGPSPortSettingDlg : public CDialog
{
	DECLARE_DYNAMIC(CGPSPortSettingDlg)

public:
	CGPSPortSettingDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CGPSPortSettingDlg();

	int m_iBaud,m_nPort;
	CString m_strComPort;
	GPS_POSITION m_gps;

	static SHP_MRK_STYLE m_mstyle_sD,m_mstyle_hD,m_mstyle_tD;
	static SHP_MRK_STYLE m_mstyle_s,m_mstyle_h,m_mstyle_t;
	static bool m_bGPSMarkerChg,m_bMarkerInitialized;
	static WORD m_wFlags;
	static int m_nConfirmCnt;
	int m_nLastSaved;

// Dialog Data
	enum { IDD = IDD_GPS_PORTSETTING };

	void Enable(UINT id,BOOL bEnable)
	{
		GetDlgItem(id)->EnableWindow(bEnable);
	}
	void SetText(UINT id,LPCSTR s)
	{
		GetDlgItem(id)->SetWindowText(s);
	}
	void SetStatusText(LPCSTR s)
	{
		GetDlgItem(IDC_ST_STATUS)->SetWindowText(s);
	}

	static void InitGPSMarker();

	bool HavePos() {return m_bHavePos;}
	bool HaveGPS() {return m_bHaveGPS;}
	bool IsRecording() {return m_bRecordingGPS;}
	void DisplayData();
	bool ConfirmClearLog();

private:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	CMainFrame *m_pMF;

	CSerialMFC *m_pSerial;
	CComboBoxNS m_cbPort;
	CComboBoxNS m_cbBaud;
	bool m_bPortOpened,m_bHaveGPS,m_bHavePos,m_bWindowDisplayed, m_bTimerSet,
		 m_bPortAvailable,m_bRecordingGPS,m_bLogPaused,m_bLogStatusChg;
	UINT_PTR m_tmTimer;
	void ShowPortMsg(LPCSTR p);

	void Show(UINT id,bool bShow) {GetDlgItem(id)->ShowWindow(bShow?SW_SHOW:SW_HIDE);}

	CSerial::EPort PortStatus();
	void ShowPortStatus(CSerial::EPort e);

	virtual void OnCancel();
	virtual void PostNcDestroy();

	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnBnClickedTest();
	afx_msg void OnPortChange();
	afx_msg void OnClose();
	afx_msg void OnBnClickedCopyloc();
	afx_msg LRESULT OnSerialMsg (WPARAM wParam, LPARAM lParam);
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnBnClickedMinimize();
	afx_msg void OnWindowPosChanged( WINDOWPOS*);
	afx_msg LONG OnWindowDisplayed(UINT, LONG);
	afx_msg void OnTimer(UINT nIDEvent);

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedClearlog();
	afx_msg void OnBnClickedPauselog();
	afx_msg void OnBnClickedSavelog();
	afx_msg void OnBnClickedCancel();
};
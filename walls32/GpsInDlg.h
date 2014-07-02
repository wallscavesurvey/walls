#if !defined(AFX_GPSINDLG_H__F6B5948A_02A8_4A15_BBA2_2D4EB9929B38__INCLUDED_)
#define AFX_GPSINDLG_H__F6B5948A_02A8_4A15_BBA2_2D4EB9929B38__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// GpsInDlg.h : header file
//

#define GPS_DLL_NAME "WALLGPS.DLL"
#define GPS_DLL_IMPORT "_Import@16"
#define GPS_DLL_ERRMSG "_ErrMsg@4"

#define GPS_LENNAME 15
typedef struct {
  int uTrkDist;
  int uLimitDist;
  char name[GPS_LENNAME+1];
} GPS_AREA;

typedef int (FAR PASCAL *LPFN_GPS_CB)(LPCSTR msg);
typedef int (FAR PASCAL *LPFN_GPS_IMPORT)(LPCSTR pathname,GPS_AREA *pA,UINT flags,LPFN_GPS_CB cb);
typedef LPSTR (FAR PASCAL *LPFN_GPS_ERRMSG)(int errcode);

/////////////////////////////////////////////////////////////////////////////
// CGpsInDlg dialog

class CGpsInDlg : public CDialog
{
private:
	GPS_AREA m_gps_area;
// Construction
public:
	CGpsInDlg(CWnd* pParent = NULL);   // standard constructor
	~CGpsInDlg();
	HINSTANCE m_hinst;
	LPFN_GPS_IMPORT m_pfnImport;
	LPFN_GPS_ERRMSG m_pfnErrmsg;

// Dialog Data
	//{{AFX_DATA(CGpsInDlg)
	enum { IDD = IDD_GPSINDLG };
	CString	m_Pathname;
	int		m_iComPort;
	int		m_iWaypoints;
	BOOL	m_bLocalTime;
	BOOL	m_bWriteCSV;
	CString	m_wptDist;
	CString	m_wayPoint;
	BOOL	m_bRestrict;
	CString	m_trkDist;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGpsInDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	int CheckData();
	BOOL CheckPathname();
	void ShowConnecting();
	void LoadGPSParams();
	void SaveGPSParams();
	UINT GetFlags();
	void Enable(int id,BOOL bEnable) {GetDlgItem(id)->EnableWindow(bEnable);}

	// Generated message map functions
	//{{AFX_MSG(CGpsInDlg)
	afx_msg void OnTestCom();
	virtual void OnCancel();
	virtual BOOL OnInitDialog();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnBrowse();
	afx_msg void OnStartDL();
	afx_msg void OnRestrict();
	//}}AFX_MSG
    afx_msg LRESULT OnCommandHelp(WPARAM wNone, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GPSINDLG_H__F6B5948A_02A8_4A15_BBA2_2D4EB9929B38__INCLUDED_)

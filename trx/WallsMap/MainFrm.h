// MainFrm.h : interface of the CMainFrame class
//
#pragma once

#include "FileDropTarget.h"
#include "NMEAParser.h"

extern const UINT WM_SWITCH_EDITMODE;

extern const TCHAR szPreferences[];
extern const TCHAR szEditorName[];
extern const TCHAR szGPSPort[];
extern const TCHAR szGPSMrk[];
extern const TCHAR szGPSMrkH[];
extern const TCHAR szGPSMrkT[];

//Global flags --
enum {STAT_DATUM,STAT_XY,STAT_DEG,STAT_SIZE,STAT_ZOOM,STAT_SCALE};

enum {PRF_OPEN_EDITABLE=1,PRF_OPEN_LAST=2,PRF_OPEN_MAXIMIZED=4,PRF_ZOOM_FROMCURSOR=8,PRF_ZOOM_FORWARDOUT=16,
		PRF_EDIT_TOOLBAR=32,PRF_EDIT_INITRTF=64};

#define PRF_DEFAULTS (PRF_OPEN_EDITABLE|PRF_OPEN_LAST|PRF_OPEN_MAXIMIZED)

extern const int Baud_Range[5];

class CWallsMapView;
class CWallsMapDoc;
class CSerialMFC;
class CGPSPortSettingDlg;

#define _USE_MYMDI

class CSyncHint : public CObject
{
	DECLARE_DYNAMIC(CSyncHint)
public:
	CSyncHint() :fpt(NULL),fupx(0) {}
	CSyncHint(const CFltPoint *pt,const double upx) : fpt(pt),fupx(upx) {}
    const CFltPoint *fpt;
    double fupx;
};

#ifdef _USE_MYMDI
// CMyMdi window
class CMyMdi : public CWnd
{
protected:
    //{{AFX_MSG(CMyMdi)
    afx_msg LRESULT OnMdiTile(WPARAM wParam, LPARAM lParam);
	//}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};
#endif

//Global --
extern CGPSPortSettingDlg *pGPSDlg;
extern VEC_GPSFIX vptGPS;
int GPSExportLog(LPCSTR shpName);

class CMainFrame : public CMDIFrameWnd
{
	DECLARE_DYNAMIC(CMainFrame)
public:
	CMainFrame() {}

    void OnDrop();

// Attributes

#ifdef _USE_MYMDI
    CMyMdi m_MyMdiClientWnd;	//MDI client window 
#endif
	static BOOL m_bSync;

	static UINT m_uPref;

	static bool   IsPref(UINT flg) {return (m_uPref&flg)!=0;}
	static void	  SetPref(UINT flg) {m_uPref|=flg;}
	static void	  ClearPref(UINT flg) {m_uPref&=~flg;}
	static void   TogglePref(UINT flg) {m_uPref^=flg;}

	static COLORREF m_clrBkg;
	static COLORREF m_clrTxt;
	static CLogFont m_fEditFont;
	static bool m_bChangedPref,m_bChangedEditPref,m_bWebMapFormatChg;

	static CSerialMFC *m_pSerial;
	static int m_nPortDef;
	static int m_iBaudDef;
	static const int CMainFrame::Baud_Range[5];

	int NumSyncCompatible(CWallsMapDoc *pSender);
	int StatusBarHt();
	int ToolBarHt();

	static CString m_csWebMapFormat; //modified user url
	static int m_idxWebMapFormat,m_idxWebMapFormatOrg;  //index of unmodified version
	
	static LPSTR m_pComparePrg,m_pCompareArgs;
	bool TextCompOptions();
	static void LaunchComparePrg(LPCSTR pLeft, LPCSTR pRight);

// Operations
public:
	CStatusBar&	GetStatusBar() { return m_wndStatusBar; }
	void ClearStatusPanes(UINT first=1,UINT last=3);
	//BOOL SetStatusPane(UINT id,LPCSTR format,...);
	void ClearStatusPane(UINT i) {m_wndStatusBar.SetPaneText(i,"");}

	int NumDocsOpen();
	CWallsMapDoc * FindOpenDoc(LPCSTR pPath);
	CWallsMapDoc * GetActiveDoc() {return (CWallsMapDoc *)GetActiveDocument();}
	void UpdateViews(CWallsMapView *pSender,LPARAM lHint,CSyncHint *pHint=NULL);
	void UpdateGPSTrack();
	void UpdateGPSCenters();
	CWallsMapDoc *FirstGeoRefSupportedDoc();
	void EnableGPSTracking();
	void RestoreWindow(int nShow);
	static void LoadPreferences();
	static void SavePreferences();
	void Launch_WebMap(CFltPoint &fpt,LPCSTR nam);
	bool GetWebMapName(CString &name); //for popup menu use
	BOOL GetWebMapFormatURL(BOOL bFlyTo);
	CFileDropTarget m_dropTarget;
	static CDialog *m_pMemoParentDlg;

private:
	void OnWebMapFormat(); //initialize web mapping site URL
	void OnUpdateStatusBar(CCmdUI* pCmdUI,int id);
	void DockControlBarLeftOf(CToolBar* Bar, CToolBar* LeftOf);

// Overrides
public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
#ifdef _USE_MYMDI
	virtual BOOL CreateClient(LPCREATESTRUCT lpCreateStruct, CMenu* pWindowMenu); 
#endif
	BOOL OnMDIWindowCmd(UINT nID);

// Implementation
public:
	virtual ~CMainFrame();

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:  // control bar embedded members
	CStatusBar  m_wndStatusBar;
	CToolBar    m_wndToolBar;
	CSyncHint	m_syncHint;

protected:
	DECLARE_MESSAGE_MAP()

// Generated message map functions
public:
	afx_msg LRESULT OnPropViewDoc(WPARAM wParam,LPARAM);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnFileOpen();
	afx_msg void OnDestroy();
	afx_msg void OnUpdate0(CCmdUI* pCmdUI);
	afx_msg void OnUpdate1(CCmdUI* pCmdUI);
	afx_msg void OnUpdate2(CCmdUI* pCmdUI);
	afx_msg void OnUpdate3(CCmdUI* pCmdUI);
	afx_msg void OnUpdate4(CCmdUI* pCmdUI);
	afx_msg void OnUpdate5(CCmdUI* pCmdUI);
	afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
	afx_msg void OnFileCloseall();
	afx_msg void OnOpenEditable();
	afx_msg void OnSetSelectedMarker();
	afx_msg void OnUpdateOpenEditable(CCmdUI *pCmdUI);
	afx_msg void OnPrefOpenlast();
	afx_msg void OnUpdatePrefOpenlast(CCmdUI *pCmdUI);
	afx_msg void OnPrefOpenMaximized();
	afx_msg void OnUpdatePrefOpenMaximized(CCmdUI *pCmdUI);
	afx_msg void OnZoomforwardin();
	afx_msg void OnUpdateZoomforwardin(CCmdUI *pCmdUI);
	afx_msg void OnZoomforwardout();
	afx_msg void OnUpdateZoomforwardout(CCmdUI *pCmdUI);
	afx_msg void OnZoomfromcenter();
	afx_msg void OnUpdateZoomfromcenter(CCmdUI *pCmdUI);
	afx_msg void OnZoomfromcursor();
	afx_msg void OnUpdateZoomfromcursor(CCmdUI *pCmdUI);
	afx_msg void OnMemoDefaults();
	afx_msg void OnGEDefaults();
	afx_msg void OnPrefEditorname();
	afx_msg LRESULT OnSwitchEditMode(UINT wParam, LONG lParam);
	afx_msg LRESULT OnCommandHelp(WPARAM wNone, LPARAM lParam);
	afx_msg void OnHelpOpenreadme();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnGPSPortSetting();
	afx_msg LRESULT OnSerialMsg (WPARAM wParam, LPARAM lParam);
	afx_msg void OnGpsOptions();
	afx_msg void OnGpsSymbol();
	afx_msg void OnClose();
};


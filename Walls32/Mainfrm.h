// mainfrm.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////
#ifndef MAINFRM_H
#define MAINFRM_H

#include "Picture.h"
#include "prjref.h"

#if !defined(LSTBOXEX_H)
#include "lstboxex.h"
#endif

#ifndef __WALLMAG_H
#include "wallmag.h"
#endif

#define _USE_IMAGE
#define _USE_OLE

#ifndef _WIN32
#define WALLS_CLASS_NAME ((LPCSTR)"WallsMainFrm")
#endif

extern const UINT WM_RETFOCUS;
extern const UINT WM_PROPVIEWLB;
extern const UINT WM_TIMERREFRESH;

enum { E_ENTERTABS = 1, E_GRIDLINES = 2, E_TABLINES = 4, E_AUTOSEQ = 8, E_CHANGED = 128, E_SIZECHG = 1024 };

enum e_ref { REF_FMTDM = 1, REF_FMTDMS = 2, REF_FMTMASK = 3, REF_LONDIR = 4, REF_LATDIR = 8 };

#define REF_WGS84 27
#define REF_NAD27 254
#define REF_MAXDATUM 250
#define REF_NUMFLDS 14

extern int moveBranch_id;
extern BOOL moveBranch_SameList;

enum { MV_COPY = 1, MV_FILES = 2 };

class CPrjDoc;
class CLineDoc;
class CMagDlg;
class CPrjListNode;

/////////////////////////////////////////////////////////////////////////////
// CMyMdi window
class CMyMdi : public CWnd
{
	// Construction
public:
	CMyMdi();

	// Attributes
public:

#ifdef _USE_IMAGE
#ifdef _USE_OLE
	CPicture m_Picture;
#else
	CBitmap m_bmpBackGround;
	CSize m_szBmp;
#endif
#endif

	// Implementation
public:
	virtual ~CMyMdi();

	// Generated message map functions
protected:
	//{{AFX_MSG(CMyMdi)
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

struct POS_MDI {
	CDocTemplate *pTemp;
	POSITION dPos;
	POSITION vPos;
};

/////////////////////////////////////////////////////////////////////////////
class CMainFrame : public CMDIFrameWnd
{
	DECLARE_DYNAMIC(CMainFrame)
public:
	CMainFrame();

	// Attributes
public:
	CListBoxExResources m_ListBoxRes1;
	CMyMdi m_MyMdiClientWnd;	//MDI client window 
	CPrjDoc *m_pFirstPrjDoc;	//Linked list of project documents
	CLineDoc *m_pFirstLineDoc;  //Linked list of documents
	static BOOL	m_bIns;         //Insert mode toggle
	static int m_cyStatus;
	static UINT m_EditFlags[2]; //Flags and tabintervals for non-SRV and SRV files

	static UINT m_EditWidth[2]; //Default window size for non-SRV and SRV files
	static UINT m_EditHeight[2];

	static PRJREF m_reflast;

	// Operations
public:
	static void PutRef(PRJREF *pr, MAGDATA *pMD = &app_MagData);
	static void InitRef(PRJREF *pr, MAGDATA *pMD = &app_MagData); //Calls PutRef and also initializes date/time
	static void GetRef(PRJREF *pr); //fills pr with app_MagData
	static void CheckDatumName(PRJREF *pr);

	static void ConvertUTM2LL(double *pXY);

	void RestoreWindow(int nShow);
	void SetHierRefresh();

	virtual BOOL CreateClient(LPCREATESTRUCT lpCreateStruct, CMenu* pWindowMenu); //MYMDI 
	static void IconEraseBkgnd(CDC* pDC); //Called by CPrjFrame::OnIconEraseBkgnd(), etc.
	static void ChangeLabelFont(PRJFONT *pF);
	static void LoadEditFlags(int iType);
	static void SaveEditFlags(int iType);
	static void XferVecLimits(BOOL bSave);
	static BOOL Launch3D(const char *name, BOOL b3D);
	static CString Get3DFilePath(BOOL b3D, BOOL bConfig = FALSE);
	static CString GetViewerProg(BOOL b3D);
	static BOOL GetViewerStatus(BOOL b3D);
	static void SaveViewerStatus(BOOL bStatus, BOOL b3D);

	//static void GetSetGridSVG(UINT *pInt,UINT *pSub,BOOL bSet);
	static CString Get3DPathName(CPrjListNode *pNode, BOOL b3D);
	static void GetMrgPathNames(CString &merge1, CString &merge2);
	static BOOL CloseLineDoc(LPCSTR pathname, BOOL bNoSave);
	static BOOL LineDocOpened(LPCSTR pathname);

	void OnViewerConfig(BOOL b3D);
	void BrowseNewProject(CDialog *pDlg, CString &prjPath);
	void BrowseFileInput(CDialog *pDlg, CString &srcPath, CString &outPath, UINT ids_outfile);

#if 0
	//Public static utility used for window placement in view's OnInitialUpdate --
	static POSITION GetFirstMdiFrame(CDocument *pDoc);
	static CFrameWnd *GetNextMdiFrame(POSITION &pos, CRect *pRect, UINT *pCmd);
#endif

	//Used to mark updated document window title with asterisk --
	enum { MAX_TITLE_LEN = 80 };

private:

	// Implementation
	BOOL EditOptions(UINT *pFlags, int typ);

public:
	static void ToggleIns();

	void SetOptions(BOOL bPrinter); //Helper for OnPrintOptions(), etc.
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual	void AssertValid() const;
	virtual	void Dump(CDumpContext& dc) const;
#endif

	//protected:
	CStatusBar	m_wndStatusBar;
	CToolBar	m_wndToolBar;

	// MFC Overrides --
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

#ifdef _USEHTML
	virtual BOOL OnHelpInfo(HELPINFO* pHelpInfo);
#endif

public:
	afx_msg void OnWindowCloseAll();
	//***7.1 afx_msg LRESULT OnCopyData(WPARAM wNone,COPYDATASTRUCT *pcd);
	afx_msg LRESULT OnCopyData(WPARAM wNone, LPARAM lParam);

	// Generated message map functions
protected:
	//{{AFX_MSG(CMainFrame)
#ifdef _USE_FILETIMER
	afx_msg void OnTimer(UINT nIDEvent);
#endif
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnEditorFont();
	afx_msg void OnSurveyFont();
	afx_msg void OnBookFont();
	afx_msg void OnReviewFont();
	afx_msg void OnChecknames();
	afx_msg void OnUpdateChecknames(CCmdUI* pCmdUI);
	afx_msg void OnDisplayOptions();
	afx_msg void OnPrintOptions();
	afx_msg void OnOptEditCurrent();
	afx_msg void OnUpdateOptEditCurrent(CCmdUI* pCmdUI);
	afx_msg void OnOptEditOther();
	afx_msg void OnOptEditSrv();
	afx_msg void OnPrintLabels();
	afx_msg void OnScreenLabels();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnUpdateWindowCloseAll(CCmdUI* pCmdUI);
	afx_msg void OnVecLimits();
	afx_msg void OnUpdateEditAutoseq(CCmdUI* pCmdUI);
	afx_msg void OnDestroy();
	afx_msg void On3dConfig();
	afx_msg void OnPrintNotes();
	afx_msg void OnScreenNotes();
	afx_msg void OnFrameFont();
	afx_msg void OnSysColorChange();
	afx_msg void OnExportOptions();
	afx_msg void OnExportLabels();
	afx_msg void OnExportNotes();
	afx_msg void OnSwitchToMap();
	afx_msg void OnUpdateSwitchToMap(CCmdUI* pCmdUI);
	afx_msg void OnMagRef();
	afx_msg void OnUpdateMagRef(CCmdUI* pCmdUI);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnMoveBranchFiles();
	afx_msg void OnMoveBranchLinks();
	afx_msg void OnCopyBranchFiles();
	afx_msg void OnUpdateCopyBranchFiles(CCmdUI* pCmdUI);
	afx_msg void OnCopyBranchLinks();
	afx_msg void OnUpdateCopyBranchLinks(CCmdUI* pCmdUI);
	afx_msg void OnMoveCopyCancel();
	afx_msg void On2dConfig();
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnEditVector();
	afx_msg void OnLocateVector();
	afx_msg void OnLaunchLst();
	afx_msg void OnLaunch3d();
	afx_msg void OnLaunchLog();
	afx_msg void OnLaunch2D();
	afx_msg void OnUpdateLaunch(CCmdUI* pCmdUI);
	afx_msg void OnMarkerStyle();
	afx_msg void OnUpdateMarkerStyle(CCmdUI* pCmdUI);
	afx_msg void On3DExport();
	afx_msg void OnSvgExport();
	afx_msg void OnUpdateSvgExport(CCmdUI* pCmdUI);
	afx_msg void OnDetails();
	afx_msg void OnUpdateDetails(CCmdUI* pCmdUI);
	//}}AFX_MSG

	afx_msg LRESULT OnCommandHelp(WPARAM wNone, LPARAM lParam);
	afx_msg void OnUpdateLine(CCmdUI* pCCmdUI);
	afx_msg void OnUpdateIns(CCmdUI* pCCmdUI);
	afx_msg LRESULT OnFindReplace(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnClose();
};
////////////////////////////////////////////////////////////////
// CWindowPlacement 1996 Microsoft Systems Journal.
// If this code works, it was written by Paul DiLascia.
// If not, I don't know who wrote it.

////////////////
// CWindowPlacement reads and writes WINDOWPLACEMENT 
// from/to application profile and CArchive.
//
struct CWindowPlacement : public tagWINDOWPLACEMENT {
	CWindowPlacement();
	~CWindowPlacement();

	// Read/write to app profile
	BOOL GetProfileWP();
	void WriteProfileWP();

	// Save/restore window pos (from app profile)
	void Save(CWnd* pWnd);
	BOOL Restore(CWnd* pWnd);
};
#endif  


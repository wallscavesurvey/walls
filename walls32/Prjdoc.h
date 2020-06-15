// prjdoc.h : interface of the CPrjDoc class
//
/////////////////////////////////////////////////////////////////////////////

#ifndef PRJDOC_H
#define PRJDOC_H

#ifndef PRJHIER_H
#include "prjhier.h"
#endif

#ifndef __NETLIB_H
#include "netlib.h"
#endif
#ifndef __TRXFILE_H
#include <trxfile.h>
#endif

typedef LPVOID *PPVOID;

#define PRJ_SIZ_FINDBUF 82

#define PRJ_SURVEY_EXT  ".SRV"
#define PRJ_PROJECT_EXT ".prj"
#define PRJ_PROJECT_EXTW ".wpj"

#define LEN_BASENAME 8

//High byte different causes *old* NTA to be "not found"
//Only low byte different causes header to be reinitialized and segment tree reused
//If either different, SEGHDR is reininitalized.

#define NTA_VERSION 0x0306

//Changing this causes tree items to not be considered compiled. Hence
//a new compilation is required before a review --
#define NTV_VERSION 0x0201

#define SRV_VEC_EXT ".NTV"
#define SRV_STR_EXT ".NTS"
#define SRV_PLT_EXT ".NTP"
#define SRV_NET_EXT ".NTN"

#define NET_SEF_EXT "SEF"
#define NET_LOG_EXT "LOG"

#define INCHES_PER_METER (12.0/0.3048)
#define CENTIMETERS_PER_INCH 2.54

#define LEN_SCALE(m) (CPrjDoc::m_ReviewScale*(m))
#define LEN_INVSCALE(f) ((f)/CPrjDoc::m_ReviewScale)
#define LEN_ISFEET() (CPrjDoc::m_ReviewScale>1.0)
#define LEN_UNITSTR() (CPrjDoc::UnitStr())
#define VIEW_UNITSTR() (CPrjDoc::ViewUnitStr())

class CCompView;
class CPrjView;
class CMapView;
class CMapFrame;
class CFileBuffer;
class CDBFile;
struct VIEWFORMAT;
struct GRIDFORMAT;
class CExpSefDlg;
class CGradient;

class CPrjDoc : public CDocument
{
protected: // create from serialization only
	CPrjDoc();

	DECLARE_DYNCREATE(CPrjDoc)

	// Attributes
public:
	enum { SAVEBUFSIZ = 0x400 };
	enum { LHINT_FONT = 1, LHINT_CLOSEMAP = 2, LHINT_REFRESHNET = 4, LHINT_FLGCHECK = 8, LHINT_FLGREFRESH = 16 };

	static double m_AzLimit, m_VtLimit, m_LnLimit;
	static BOOL m_bNewLimits;
	static BOOL m_bReviewing, m_bRefreshing;
	static int m_iValueRangeNet;
	static int m_nMapsUpdated;
	static CPrjDoc *m_pLogDoc;

	CPrjListNode *m_pNodeLog;
	CMapFrame *m_pMapFrame;

#ifdef _DEBUG
	int GetFrameCnt();
#endif

	FILE *m_fplog;
	int log_write(LPCSTR lpStr);
	void InitLogPath(CPrjListNode *pNode);
	CMapFrame *FrameNodeExists(CPrjListNode *pNode);

	CPrjListNode * m_pRoot;
	CPrjDoc *m_pNextPrjDoc;  //May not really need this!
	int m_nActivePage;		//first active page of CItemProp dlg

	char *m_pFindString;
	WORD m_wFindStringFlags;
	pcre *m_pRegEx;
	LINENO FindString(CPrjListNode *pNode, WORD findflags);
	static CFileBuffer *m_pFileBuffer;

	enum {
		TYP_NTV, TYP_NTN, TYP_NTP, TYP_NTS, TYP_NTA, TYP_NAM, TYP_TMP,
		TYP_LST, TYP_3D, TYP_WMF, TYP_SEF, TYP_LOG, TYP_2D, TYP_NTW, TYP_NTAC, TYP_COUNT
	};
	static char *WorkTyp[TYP_COUNT];

	static CGradient *GetGradientPtr(int iTyp);

	static BOOL m_bComputing;
	static BOOL m_bSwitchToMap; //Switch to Map page upon review
	static BOOL m_bSaveView;    //Save "located" views
	static BOOL m_bPageActive;
	static CPrjDoc *m_pReviewDoc;
	static CPrjListNode *m_pReviewNode;
	static double m_ReviewScale;
	static const char *UnitStr();
	static const char *ViewUnitStr();
	static CPrjListNode *m_pErrorNode;
	//static CPrjListNode *m_pErrorNode2;
	static LINENO m_nLineStart;
	//static LINENO m_nLineStart2;
	static int m_nCharStart;

	static NET_DATA net_data;   //Struct for WALLNET4.DLL communications

	static BOOL m_bCheckNames;  //Checks for vector duplicates when compiling

	//Data for constructing project pathnames --
	char *m_pPath;		//project data directory path (includes trailing "\", not 0)
	int  m_lenPath;		//Position after trailing "\" in m_pPath
	char *m_pszName;		//Project name
	//Buffer for survey file pathnames --
	static char m_pathBuf[_MAX_PATH];

	static int m_iFindVector;
	static int m_iFindVectorDir;
	static int m_iFindVectorMax;
	static int m_iFindVectorIncr;
	static int m_iFindVectorCnt;
	static int m_bVectorDisplayed;

	static int m_iFindStation;
	static int m_iFindStationMax;
	static int m_iFindStationIncr;
	static int m_iFindStationCnt;

	static double GetFRatio();
	static double GetFRatio(int i);
	static void GetValueRange(double *fMinimum, double *fMaximum, int typ);
	static void FillHistArray(float *pHistValue, int len, double fStart, double fEnd, int typ);
	static void InitGradValues(CGradient &grad, double fMin, double fMax, int iTyp);
	static void InitGradient(CGradient *pGrad, int n);
	static void VectorProperties();
	static void ViewSegment();
	static LPCSTR GetDatumStr(char *buf, CPrjListNode *pNode);
	/*static BOOL IsLinkFlagged(int frgID,int flag);*/

	int GetDataWidth(BOOL bIgnoreFlags = FALSE);
	static int InitNamTyp();
	static void AllocNamTyp();
	static void RefreshOtherDocs(CPrjDoc *pThisDoc);
	UINT m_uShapeTypes; //Shapefile type flags

 // Implementation

private:
	//Would allow reversible modified flag when detaching/attaching --
	//CHierListNode *m_pNodeFloat;

	BOOL SetName(const char *pszPathName);

	void SetRootFlag(UINT flag)
	{
		//if not CHK make UNCHK
		if (!(m_pRoot->m_uFlags&(flag << 1))) m_pRoot->m_uFlags |= flag;
	}

	int CompileBranch(CPrjListNode *pNode, int iSeg);
#ifdef _COMPILEWORK
	int CompileWorkFile(CPrjListNode *pNode, int iSeg);
#endif
	int InitCompView(CPrjListNode *pNode);
	BOOL OpenWorkFiles(CPrjListNode *pNode);
	char *m_pShapeFilePath;

#ifdef _USE_FILETIMER
	int m_bCheckForChange;
#endif

public:
	virtual ~CPrjDoc();
	char * Title() { return m_pRoot->Title(); }
	char * WorkPath(CPrjListNode *pNode, int typ);
	char * WorkPath() { return WorkPath(NULL, 0); }
	char * LogPath() { return WorkPath(m_pNodeLog, TYP_LOG); }
	char * ShapeFilePath() { return m_pShapeFilePath ? m_pShapeFilePath : WorkPath(); }
	void SetShapeFilePath(const char *path) {
		if (_stricmp(path, ShapeFilePath())) {
			m_pShapeFilePath = (char *)realloc(m_pShapeFilePath, strlen(path) + 1);
			if (m_pShapeFilePath) strcpy(m_pShapeFilePath, path);
		}
	}

	int OpenWork(PPVOID pRecPtr, CDBFile *pDB, CPrjListNode *pNode, int typ);
	int OpenSegment(CTRXFile *pix, CPrjListNode *pNode, int mode);
	char * SurveyPath(CPrjListNode *pNode);
	char * ProjectPath();
	char * CPrjDoc::GetPathPrefix(char *pathbuf, CPrjListNode *pNode);
	char * GetRelativePathName(char *pathbuf, CPrjListNode *pNode);
	void SendAsEmail(CString &csFile);

	BOOL SetReadOnly(CPrjListNode *pNode, BOOL bReadOnly, BOOL bNoPrompt = FALSE);
	int SetBranchReadOnly(CPrjListNode *pNode, BOOL bReadOnly);

	CLineDoc *OpenSurveyFile(CPrjListNode *pNode);
	int CopyBranchSurveys(CPrjListNode *pNode, char *newPath, int bCopy); //CPrjList:OnDrop()
	int DeleteBranchSurveys(CPrjListNode *pNode);
	int CountBranchReadOnly(CPrjListNode *pNode);

	void LaunchFile(CPrjListNode *pNode, int typ);
	int ResolveLaunchFile(char *pathname, int typ);
	int FloatStr(int iSys, int iTrav, int i);

	void MarkProject(BOOL bUpd = TRUE/*,CHierListNode *pNode=NULL*/)
	{
		SetModifiedFlag(bUpd);
	}

	void SaveProject()
	{
		SetModifiedFlag(TRUE);
		SaveModified();
	}

	CPrjView *GetPrjView();
	CPrjListNode *GetSelectedNode();

#ifdef _USE_FILETIMER
	void TimerRefresh();
#endif

	int ExportSEF(CPrjListNode *pNode, const char *pathname, UINT flags);
	int ExportBranch(CPrjListNode *pNode);

	int Compile(CPrjListNode *pNode);
	void Review(CPrjListNode *pNode);
	int PurgeWorkFiles(CPrjListNode *pNode, BOOL bIncludeNTA = FALSE);
	void ClearDefaultViews(CPrjListNode *pNode);
	void PurgeAllWorkFiles(CPrjListNode *pNode, BOOL bIncludeNTA = FALSE, BOOL bIgnoreFloating = FALSE);
	void RefreshBranch(CPrjListNode *pNode);
	void CloseWorkFiles();

	void ProcessSVG();

	static BOOL IsActiveFrame();
	BOOL RefreshMaps();
	BOOL RefreshMapTraverses(UINT bFlg);
	BOOL CPrjDoc::UpdateMapViews(BOOL bChkOnly, UINT flgs);

	static void InitTransform(CMapFrame *pf);
	static BOOL FindStation(const char *pName, BOOL bMatchCase, int iFindNext = 0);
	static BOOL FindNoteTreeRec(char *keybuf, BOOL bUseFirst = TRUE);

	static void PositionReview();
	static BOOL FindRefStation(int iNext);
	static int GetStrFlag(int iSys, int iTrav);
	static int SetNetwork(int iNet);
	static void SetVector();
	static void SetSystem(int iSys);
	static BOOL SetTraverse(int iSys, int iTrav);
	static int TravToggle(int iSys, int iTrav);
	static void GetCoordinate(DPOINT &coord, CPoint point, BOOL bGrid);
	static void PlotTraverse(int iSys, int iTrav, int iZoom);
	static void SelectAllPages();
	static void PlotPageGrid(int iZoom);
	static void PlotFrame(UINT flags);
	static BOOL GetVecData(vecdata *pvecd, int nRec);
	static void LoadVecFile(int nRec);
	static int GetNetFileno();
	static LINENO GetNetLineno();
	static BOOL GetRefOffsets(const char *pName, BOOL bMatchCase);
	static void LocateStation();
	static BOOL FindVector(BOOL bMatchCase, int iNext = 0);
	static void LocateOnMap();
	static DWORD GetVecPltPosition();
	static void AppendStationName(CString &str, int pfx, const void *name);
	static void AppendPltStationName(CString &str, int iForward);
	static void AppendVecStationNames(CString &str);
	static BOOL GetPrefixedName(CString &buf, BOOL bVector);
	static void PlotFlagSymbol(CDC *pDC, CSize sz, BOOL bFlag,
		COLORREF fgclr, COLORREF bkclr, WORD style);

	//Called from CPrjView::OnNewItem() an CPrjList::OnDrop() --
	void BranchFileChk(CPrjListNode *pNode);
	int AttachedFileChk(CPrjListNode *pNode);
	void BranchWorkChk(CPrjListNode *pNode);
	int BranchEditsPending(CPrjListNode *pNode);
	int AllBranchEditsPending(CPrjListNode *pNode);

	//Called from CPrjView after call to CItemProp() dialog --
	CLineDoc * GetOpenLineDoc(CPrjListNode *pNode);

	//Called from CLineDoc::OnSaveDocument() --
	static void FixAllFileChecksums(const char *pszPathName);

	//Used by CLineDoc::OnFileSaveAs() --
	static CPrjListNode *FindFileNode(CPrjDoc **pFindDoc, LPCSTR pathname, BOOL bFindOther = FALSE);

	//Called from CLineDoc::OnUpdateFileSave() and CLineDoc::OnCloseDocument() --
	static void FixAllEditsPending(const char *pszPathName, BOOL bChg);

	//Static functions available when workfiles are open and positioned --
	static char * GetNetNameOfOrigin(char *name); //Used by CPlotView::OnGridOptions()

	static void InitViewFormat(VIEWFORMAT *pVF);
	static int NTS_id(int iSys, int iTrav);

#ifdef _DEBUG
	virtual	void AssertValid() const;
	virtual	void Dump(CDumpContext& dc) const;
#endif

protected:
	BOOL CanCloseFrame(CFrameWnd* pFrameArg); //MFC version is buggy!

	void FixFileChecksums(const char *pszPathName, DWORD dwChk);
	void FixEditsPending(const char *pszPathName, BOOL bChg);
	virtual	BOOL OnNewDocument();
	virtual BOOL OnOpenDocument(const char* pszPathName);
	virtual BOOL OnSaveDocument(const char* pszPathName);
public:
	virtual BOOL SaveModified();

	// Generated message map functions
protected:
	afx_msg void OnUpdateFileSave(CCmdUI* pCmdUI);
	afx_msg void OnFileSaveAs();
	afx_msg void OnFileSave();
	afx_msg void OnFileClose();

	DECLARE_MESSAGE_MAP()
};

//In compile.cpp --
DWORD HdrChecksum(const char *pszPathName);
/////////////////////////////////////////////////////////////////////////////
#endif

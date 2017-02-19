// plotview.h : header file
//

#ifndef __PLOTVIEW_H
#define __PLOTVIEW_H

#ifndef __AFXEXT_H__
#include <afxext.h>
#endif

#ifndef __REVIEW_H
#include "review.h"
#endif

#ifndef __SEGHIER_H
#include "seghier.h"
#endif

#ifndef __HTBUTTON_H
#include "htbutton.h"
#endif

#ifndef __FTRACKER_H
#include "ftracker.h"
#endif

//#define MAX_FRAME_PIXELS ((UINT)14400)
#define MAX_FRAME_PIXELS ((UINT)32767)
#define MAX_FRAMEWIDTHINCHES 227.54 //printer only was 120
#define MAX_SCREEN_FRAMEWIDTH 30
#define MIN_SCREEN_FRAMEWIDTH 3
#define MAX_META_RES 600
#define PERCENT_ZOOMOUT 50.0

/////////////////////////////////////////////////////////////////////
class CMapFrame;

class CPlotView : public CPanelView
{
	DECLARE_DYNCREATE(CPlotView)
	
protected:

	CPlotView();

	CPrjDoc *GetDocument() {return (CPrjDoc *)CView::GetDocument();}
	virtual void OnInitialUpdate();
	virtual void OnUpdate(CView *pSender,LPARAM lHint,CObject *pHint);
	virtual void OnDraw(CDC* pDC);
	virtual BOOL PreTranslateMessage(MSG* pMsg );

// Form Data
public:
	//{{AFX_DATA(CPlotView)
	enum { IDD = IDD_PLOTDLG };
	CStatic	m_PltFrm;
	//}}AFX_DATA

// Attributes
public:
	CSize m_sizeBmp;    // Size (width and height) of bitmap in pixels
	CSize m_sizePrint;  // Size of printer frame in pixels (set in OnBeginPrinting)
	static CDC m_dcBmp; // Compatible Memory DC, with m_cBmp already selected
	int m_recNTN;       // Component plotted in bitmap (if >0)
	int m_iTrav;		// Highlighted string (piSTR[] index +1)
	int m_iSys;         // Highlighted system (piSYS[] index +1) 
	CPoint m_ptBmp;     // Position for upper left corner of bitmap
	int m_iOverlapX,m_iOverlapY; //Frame overlap in printer pixels
    static CRect m_rectBmp;  // Bitmap's client-relative rectangle in view
	static UINT m_uMetaRes;  // Metafile resolution (used by CPrjDoc::PlotFrame())
	static POINT *m_pathPoint; //Path drawing in CPrjDoc::PlotFrame()
	
	CFixedTracker m_tracker;
	HCURSOR m_hCursorBmp;
	
	int m_iView;			  //Current orientation;
	VIEWFORMAT *m_pViewFormat;
	VIEWFORMAT m_vfSaved[2];
	
    double m_xscale;			//pixels per meter computed in CPrjDoc::PlotTraverse()
    double m_fPrintAspectRatio; //Ratio of Y pixels to X pixels (set in OnBeginPrinting)
    double m_fPanelWidth;	  //Panel width in meters
	BOOL m_bTrackerActive;
	int m_iTrackerWidth;      //Current tracker width in pixels
	CDC *m_pDCPrint;          //Set in OnPrint(), used by CPrjDoc::PlotFrame()
	UINT m_nCurPage;          //Same -- Current page
	DPOINT *m_pPanPoint;      //Set/reset by OnLButtonDblClk() when panning
	BOOL m_bCursorInBmp,m_bTracking;
	
	BOOL m_bInches;			  //For scale and frame size formatting
	BOOL m_bProfile;
	BOOL m_bExternUpdate;
    
	static PRJFONT m_FrameFont;
	static PRJFONT m_LabelPrntFont;
	static PRJFONT m_LabelScrnFont;
	static PRJFONT m_LabelExportFont;
	static PRJFONT m_NotePrntFont;
	static PRJFONT m_NoteScrnFont;
	static PRJFONT m_NoteExportFont;
	
	static MAPFORMAT m_mfFrame, m_mfPrint, m_mfExport;
	static const MAPFORMAT m_mfDflt[3];
	static GRIDFORMAT m_GridFormat;
	static char *szMapFormat;
	
	CBitmap m_cBmpBkg;    // Bitmap containing map and highlighted system
private:
	CBitmap m_cBmp;       // Displayed bitmap with highlighted traverse
	HBITMAP m_hBmpOld;    // Handle of old bitmap to save
	HBRUSH m_hBrushOld;   // Handle of old brush to save
	HPEN m_hPenOld;       // Handle of old pen to save
	CBitmap m_bmRotate;   // Bitmap for owner-draw rotate buttons 
	CDC m_dcRotBmp;
	HBITMAP m_hRotBmpOld;
	BOOL m_bPrinterOptions,m_bMeasuring; //Used when invoking CMapDlg()
	bool m_bPanning,m_bPanned;
    
// Operations
private:
#ifdef VISTA_INC
	void ShowTrackingTT(CPoint &point);
#endif
	BOOL IncSys(int dir);
	void IncTrav(int dir);
	BOOL TrackerOK();
	void LabelProfileButton(BOOL bProfile);
	void SetOptions(MAPFORMAT *pMF);
	void DrawFrame(CDC *pDC,CRect *pClip);
	void DisableButton(UINT id);
	void RefreshMeasure();
	void OnFindPrevNext(int iNext);
	BOOL ZoomInOK();
	BOOL ZoomOutOK();

public:
    static void Initialize();
    static void Terminate();
   
	CReView *GetReView() {return (CReView *)m_pTabView;}
	CPageView *GetPageView() {return GetReView()->GetPageView();} //Used by CPrjDoc::PlotFrame()
	void LoadViews();
	void SaveViews();
	void EnableRecall(BOOL bEnable);
	void ClearTracker(BOOL bDisp=TRUE);
	void SetTravChecks(BOOL bTravSelected,BOOL bVisible);
	void SetTrackerPos(int l,int t,int r,int b);
	int GetTrackerPos(CPoint *pt);
	void ViewMsg();
    void ResetContents();
	void EnableDefaultButtons(BOOL bDefEnable);
    void EnableSysButtons(BOOL bEnable)
    {
    	Enable(IDC_SYSSTATIC,bEnable);
    	Enable(IDC_SYSNEXT,bEnable);
    	Enable(IDC_SYSPREV,bEnable);
    }
    void EnableTravButtons(BOOL bEnable)
    {
    	Enable(IDC_TRAVSTATIC,bEnable);
    	Enable(IDC_TRAVNEXT,bEnable);
    	Enable(IDC_TRAVPREV,bEnable);
    }

	BOOL IsEnabled(UINT id)
    {
		return GetDlgItem(id)->IsWindowEnabled();
	}

    void EnableTraverse(BOOL bEnable)
    {
    	Enable(IDC_TRAVSELECTED,bEnable);
    	Enable(IDC_TRAVFLOATED,bEnable);
    }

	double PanelWidth()
	{
		return m_fPanelWidth;
	}

	void DisplayPanelWidth();

	afx_msg void OnFileExport();  //Used also from CSegView::OnFileExport()
	afx_msg void OnExternFrame(); //Used also from CSegView::OnDisplay()
	afx_msg void OnUpdateFrame();
	afx_msg void OnGridOptions();
	afx_msg void OnFilePrint();
	afx_msg void OnFilePrintPreview();
	afx_msg void OnMapExport();
	afx_msg void OnChgView();
	afx_msg void OnUpdateUTM(CCmdUI* pCmdUI);
    afx_msg LRESULT OnMouseLeave(WPARAM wNone, LPARAM lParam);

protected:
	virtual ~CPlotView();
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
		
    // Printing support
    virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
    virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
    virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnPrint(CDC *pDC,CPrintInfo *pInfo);
	virtual void OnEndPrintPreview(CDC* pDC,CPrintInfo* pInfo,POINT point,CPreviewView* pView);

	void ToggleNoteFlag(UINT id,UINT flag);
	afx_msg void OnFlagToggle();
	afx_msg void OnExternNotes();

	afx_msg void OnUpdateTravToggle(CCmdUI* pCmdUI);
	afx_msg void OnTravToggle();
    afx_msg void OnSysNext();
    afx_msg void OnSysPrev();
    afx_msg void OnTravNext();
    afx_msg void OnTravPrev();
    afx_msg void OnSysCommand(UINT nChar,LONG lParam);
    afx_msg void OnEditFind();
    afx_msg void OnFindNext();

#ifdef _SAV_LBL
	afx_msg void OnExternLabels();
	afx_msg void OnExternMarkers();
#endif

	// Generated message map functions
	//{{AFX_MSG(CPlotView)
	afx_msg void OnDestroy();
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnPanelZoomIn();
	afx_msg void OnPanelDefault();
	afx_msg void OnPanelZoomOut();
	afx_msg void OnPlanProfile();
	afx_msg void OnExternGridlines();
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRotateLeft();
	afx_msg void OnRotateRight();
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	afx_msg void OnTravSelected();
	afx_msg void OnTravFloated();
	afx_msg void OnExternFlags();
	afx_msg void OnDisplayOptions();
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnRecall();
	afx_msg void OnSave();
	afx_msg void OnWalls();
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnPltPanToHere();
	afx_msg void OnPltZoomIn();
	afx_msg void OnPltZoomOut();
	afx_msg void OnUpdatePltRecall(CCmdUI* pCmdUI);
	afx_msg void OnUpdatePltDefault(CCmdUI* pCmdUI);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMeasureDist();
	afx_msg void OnUpdateMeasureDist(CCmdUI* pCmdUI);
	afx_msg void OnUpdateFindNext(CCmdUI* pCmdUI);
	afx_msg void OnUpdateFindPrev(CCmdUI* pCmdUI);
	afx_msg void OnFindPrev();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
#endif

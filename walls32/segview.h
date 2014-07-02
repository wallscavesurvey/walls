// segview.h : header file
//

#ifndef __SEGVIEW_H
#define __SEGVIEW_H

#ifndef __SEGHIER_H
#include "seghier.h"
#endif
 
#ifndef __WALL_SRV_H
#include "wall_srv.h"
#endif

#ifndef __REVIEW_H
#include "review.h"
#endif

#include "colorbutton.h"
#include "gradient.h"

typedef CTypedPtrArray<CObArray,CGradient*> CGradientArray;
#define SEG_NUM_GRADIENTS 3

typedef struct {
	COLORREF clrMarker,clrBkgnd;
	HBRUSH hbrShade[M_NUMSHADES];
	HPEN hPen;
	int iMarkerThick;
	WORD dfltSize;
	WORD shade;
} FLAGSTYLE;

#define SRV_PEN_NETCOLOR RGB_GRAY
#define SRV_PEN_SYSCOLOR RGB_BLACK
#define SRV_PEN_SYSSELCOLOR RGB_BLUE
#define SRV_PEN_TRAVSELCOLOR RGB_RED
#define SRV_PEN_TRAVRAWCOLOR RGB_YELLOW
#define SRV_PEN_TRAVFRAGCOLOR RGB_LTBLUE
#define SRV_PEN_TRAVFRAGRAWCOLOR RGB_LTYELLOW
#define SRV_PEN_FLOATEDCOLOR RGB_GREEN
#define SRV_PEN_RAWFLOATEDCOLOR RGB_YELLOW

enum {TYP_LST_VECTOR,TYP_LST_NAME=1,TYP_LST_NOTE=2,TYP_LST_FLAG=4,TYP_LST_LOC=8};

class CExpavDlg;
class CExpSvgDlg;
struct SVG_VIEWFORMAT;

class CSegView : public CPanelView
{
	DECLARE_DYNCREATE(CSegView)
	
public:
	enum {TYP_SEL=0,TYP_DETACH,TYP_COLOR};
	
friend class CSegList;
friend class CExpavDlg;
friend class CExpSvgDlg;

private:
	CSegList m_SegList;       //Constructed after CSegView()
	BOOL m_bTravSelected;
	CToolTipCtrl m_ToolTips;
	    
// Form Data
public:
	//{{AFX_DATA(CSegView)
	enum { IDD = IDD_SEGDLG };
	CStatic	m_SegFrm;
	//}}AFX_DATA

// Attributes
public:
	CColorCombo m_colorLines;
	CColorCombo m_colorBackgnd;
	CColorCombo m_colorLabels;
	CColorCombo m_colorNotes;
	CColorCombo m_colorFloor;
	CColorCombo m_colorMarkers;

	CGradientArray m_Gradient;

    static int m_LineHeight;
    static BOOL m_bChanged;
    static BOOL m_bNewVisibility;
	static BOOL m_bNoPromptStatus;
    static HBRUSH m_hBrushBkg;
    static DWORD FAR *pPrefixRecs;
	static int m_iLastFlag;
	static double m_fThickPenPts;
    
    //Points to array of bkgnd, labels, markers, grid, and traverse styles in NTA header --
    static styletyp *m_StyleHdr;
    static GRIDFORMAT *m_pGridFormat;
    enum {MAX_VECTORTHICK=10};
	enum {PS_SOLID_IDX,PS_HEAVY_IDX,PS_DASH_IDX,PS_DOT_IDX,PS_DASHDOT_IDX,
		  PS_DASHDOTDOT_IDX,PS_NULL_IDX};
    static int m_PenStyle[SEG_NUM_PENSTYLES];
	
	//Used bu SegList
	static DWORD *pFlagNamePos; //ordered by seq
	static DWORD *pFlagNameSeq; //Ordered by id
    
    CSegListNode *m_pRoot;
    CSegListNode *m_pSegments;
    CSegListNode *m_pGridlines;
	CSegListNode *m_pOutlines;
    CSegListNode *m_pTraverse;
    CSegListNode *m_pRawTraverse;
    UINT m_recNTN;				//Component of computed stats

private:
	BOOL m_bGradientChanged;
    
// Operations
public:
	static void Initialize();
	void RefreshGradients();
	void ResetContents();
	void InitSegStats();
	void SelectSegments() {
		m_SegList.SetCurSel(4);
	}
	
	//Set default segment styles --
	static void InitOutlinesHdr(SEGHDR *sh);
	static void InitSegHdr(SEGHDR *sh);
	static void CopySegHdr_7P(SEGHDR *sh,SEGHDR_7P *sh7);
	static void InitDfltStyle(styletyp *st,SEGHDR *sh);
    static void GetPrefixRecs();
    static UINT GetPrefix(char *sprefix,UINT pfx,BOOL bLblFlags=8);
	static void GetLRUD(UINT id,float *lrud);
    static BOOL FlagVisible(CSegListNode *pNode);
    static void UnFlagVisible(CSegListNode *pNode)
    {
    	pNode->FlagVisible(~LST_INCLUDE);
    }

	static UINT GetFlagNameRecs();
	static void FreeFlagNameRecs() {
		free(pFlagNamePos); pFlagNamePos=NULL;
	}

	CComboBox *CBStyle(){
		return (CComboBox*)GetDlgItem(IDC_LINESTYLE);
	} 
	void UpdateBranchStyles(CSegListNode *pNode);
	void UpdateStats(CSegListNode *pNode);
	void UpdateControls(UINT uTYPE=TYP_SEL); //A selection change by default
	
	CSegListNode *SegNode(UINT i) {return m_pRoot+(i+SEGNODE_OFFSET);}
	int SegID(CSegListNode *pNode) {return (pNode-m_pRoot)-SEGNODE_OFFSET;}
	COLORREF SegColor(UINT i) {return SegNode(i)->LineColor();}

	BOOL LstInclude(int segid)
	{
		return ((m_pSegments+segid)->m_bVisible & LST_INCLUDE)!=0;
	}

	BOOL IsSegSelected()
	{
		return m_pRoot!=NULL && (m_SegList.GetSelectedNode()>=m_pSegments);
	}

	BOOL ShpInclude(int segid)
	{
		return LstInclude(segid) && !((m_pSegments+segid)->Style())->IsNoLines();
	}

	void GetParentTitles(int segid,char *buf,UINT bufsiz);
	void GetAttributeNames(int segid,char *buf,UINT bufsiz);

	CReView *GetReView() {return (CReView *)m_pTabView;}

	BOOL IsOutlinesVisible() {return m_pOutlines->IsVisible();}
	BOOL IsGridVisible() {return m_pGridlines->IsVisible();}
	BOOL IsSegmentsVisible() {return m_pSegments->IsVisible();}
	BOOL IsTravVisible() {return m_pTraverse->IsVisible();}
	BOOL IsTravSelected() {return m_bTravSelected;}
	
	styletyp * TravStyle() {return m_pTraverse->Style();}
	styletyp * RawTravStyle() {return m_pRawTraverse->Style();}
	styletyp * OutlineStyle() {return m_pOutlines->OwnStyle();}

	BOOL IsLinesVisible() {return m_pSegments->IsLinesVisible();}
	BOOL IsRawTravVisible() {return m_pRawTraverse->IsVisible();}
	int RawTravLabelFlags() {return m_pRawTraverse->LabelFlags();}
	int  TravLabelFlags() {return m_pTraverse->LabelFlags();}
	void SetOutlinesVisible(BOOL bEnable) {HdrSegEnable(m_pOutlines,bEnable);}
	void SetGridVisible(BOOL bEnable) {HdrSegEnable(m_pGridlines,bEnable);}
	void SetTravVisible(BOOL bEnable) {HdrSegEnable(m_pTraverse,bEnable);}
	void SetTravSelected(BOOL bTravSelected);
	
    static int PenStyle(CSegListNode *pNode) {return m_PenStyle[pNode->LineIdx()];}
	static COLORREF LineColor(CSegListNode *pNode) {return pNode->LineColor();}
	static COLORREF LineColorGrad(CSegListNode *pNode) {return pNode->LineColorGrad();}
	static COLORREF BackColor() {return m_StyleHdr[HDR_BKGND].LineColor();}
	static COLORREF FloorColor() {return m_StyleHdr[HDR_FLOOR].LineColor();}
	static int		FloorGradIdx() {return m_StyleHdr[HDR_FLOOR].GradIdx();}
	static COLORREF NoteColor()	{return m_StyleHdr[HDR_NOTES].NoteColor();}
	static COLORREF LabelColor() {return m_StyleHdr[HDR_LABELS].LabelColor();}
	static COLORREF MarkerColor() {return m_StyleHdr[HDR_MARKERS].MarkerColor();}
	COLORREF LrudColor() {return OutlineStyle()->LineColor();}

	static void SetMarkerColor(COLORREF clr)
	{
	  m_StyleHdr[HDR_MARKERS].SetMarkerColor(clr);
	}

	static void SetNoOverlap(BOOL On)
	{
	  m_StyleHdr[HDR_MARKERS].SetNoMark(On);
	}

	static BOOL IsNoOverlap()
	{
	  return m_StyleHdr[HDR_MARKERS].IsNoMark();
	}

	static void SetHideNotes(BOOL On)
	{
	  m_StyleHdr[HDR_MARKERS].SetHideNotes(On);
	}

	static BOOL IsHideNotes()
	{
	  return m_StyleHdr[HDR_MARKERS].IsHideNotes();
	}

	static WORD MarkerStyle()
	{
	  return m_StyleHdr[HDR_MARKERS].MarkerStyle();
	}

	static void SetMarkerStyle(WORD s)
	{
	  m_StyleHdr[HDR_MARKERS].SetMarkerStyle(s);
	}

	WORD LrudStyle() {return m_StyleHdr[HDR_FLOOR].LrudStyle();}
	void SetLrudStyleIdx(int i) {m_StyleHdr[HDR_FLOOR].SetLrudStyleIdx(i);}
	int LrudStyleIdx() {return m_StyleHdr[HDR_FLOOR].LrudStyleIdx();}
	int LrudEnlarge() {return m_StyleHdr[HDR_FLOOR].LrudEnlarge();}
	void SetLrudEnlarge(int i) {m_StyleHdr[HDR_FLOOR].SetLrudEnlarge(i);}
    void SetSVGUseIfAvail(BOOL bUse) {m_StyleHdr[HDR_FLOOR].SetSVGUseIfAvail(bUse);}
    BOOL IsSVGUseIfAvail() {return m_StyleHdr[HDR_FLOOR].IsSVGUseIfAvail();}
    void SetSVGAvail(BOOL bAvail) {m_StyleHdr[HDR_FLOOR].SetSVGAvail(bAvail);}
    BOOL IsSVGAvail() {return m_StyleHdr[HDR_FLOOR].IsSVGAvail();}
	bool HasSVGFloors() {return IsSVGAvail() && IsSVGUseIfAvail();}
	BOOL HasFloors();

	BOOL IsNewVisibility() {return m_bNewVisibility;}
	static void SetNewVisibility(BOOL bNew) {m_bNewVisibility=bNew;}

	void ApplyClrToFrames(COLORREF clr,UINT id);
	
    int InitSegTree();
	static void ReviseLineHeight();
    CPlotView * InitPlotView();
    void SaveSegments();
	void OnLaunchLst(SEGSTATS *pst,CSegListNode *pNode=NULL);

 	afx_msg void OnDetails();
	afx_msg void OnMapExport();
	afx_msg void OnSvgExport();
	afx_msg void On3DExport();
	afx_msg LRESULT OnChgColor(WPARAM,LPARAM);
	afx_msg void OnSymbols();
	afx_msg void OnLrudStyle();
   
// Private helpers --
private:
	BOOL LoadGradients();
	void SaveGradients();
	CSegListNode *GetSegStats(SEGSTATS &st);
	COLORREF SafeColor(COLORREF clr);
	CSegListNode * UpdateStyle(UINT flag,BOOL bTyp);
	void CountNotes(SEGSTATS *st,UINT segid,BOOL bChkBranch);
    void HdrSegEnable(CSegListNode *pNode,BOOL bEnable);
    void UpdateDetachButton(CSegListNode *pNode);
    void SaveTravStyles();
	void EnableControls(CSegListNode *pNode);
	void ApplyBranchStyle(CSegListNode *pNode,CSegListNode *pNodeTgt);
	CSegListNode *AttachHdrNode(int idParent,int idThis,
			styletyp *pStyle,char *pTitle,BOOL bIsSibling);

// Implementation
protected:
	CSegView();			// protected constructor used by dynamic creation
	virtual ~CSegView();
	
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	CPrjDoc *GetDocument() {return (CPrjDoc *)CPanelView::GetDocument();}
	
    void MarkSeg(CSegListNode *pNode)
    {
       pNode->SetChanged();
       m_bChanged=TRUE;
    } 
	void WriteVectorList(SEGSTATS *st,LPCSTR pathname,CSegListNode *pNode,int typ_lst,UINT uDecimals);
    BOOL ExportWRL(LPCSTR pathname,LPSTR title,UINT flags,UINT dimGrid,float fScaleVert);
	BOOL ExportSHP(CExpavDlg *pDlg,LPCSTR pathname,LPCSTR basename,CSegListNode *pNode,UINT flags);
	int ExportSVG(CExpSvgDlg *pDlg,SVG_VIEWFORMAT *pVF);

	LONG OnGetGradient(UINT lParam, LONG /*wParam*/);
	LONG OnDrawGradient(UINT lParam, LONG /*wParam*/);

//Overrides --
    virtual void OnUpdate(CView *pSender,LPARAM lHint,CObject *pHint);
	virtual void OnInitialUpdate();
	virtual void OnOK();
	afx_msg void OnEnterKey();
	
	afx_msg void OnFileExport();
	afx_msg void OnFilePrint();
	afx_msg void OnFilePrintPreview();
	afx_msg void OnLineIdxChg();
	afx_msg void OnSegmentChg();
	afx_msg void OnUpdateDetails(CCmdUI* pCmdUI);
 
	// Generated message map functions
	//{{AFX_MSG(CSegView)
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnSegDetach();
	afx_msg void OnNames();
	afx_msg void OnApplyToAll();
	afx_msg void OnSegDisable();
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	afx_msg void OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMeasureItemStruct);
	afx_msg void OnDisplay();
	//afx_msg void OnExternUpdate();
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnMarks();
	afx_msg void OnEditLeaf();
	afx_msg void OnUpdateEditLeaf(CCmdUI* pCmdUI);
	afx_msg void OnEditProperties();
	afx_msg void OnUpdateEditProperties(CCmdUI* pCmdUI);
	afx_msg void OnApply();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
#endif

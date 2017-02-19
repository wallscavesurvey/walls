// DlgSymbols.cpp : implementation file
//

#include "stdafx.h"
#include "WallsMap.h"
#include "WallsMapDoc.h"
#include "LayerSetSheet.h"
#include "ShowShapeDlg.h"
#include "DlgSymbols.h"

// CDlgSymbols dialog

IMPLEMENT_DYNAMIC(CDlgSymbols, CDialog)
CDlgSymbols::CDlgSymbols(CWallsMapDoc *pDoc,CShpLayer *pShp,CWnd* pParent /*=NULL*/)
	: CDialog(CDlgSymbols::IDD, pParent),m_pDoc(pDoc),m_pShp(pShp)
{
	m_bInitFlag=true;
	m_bTestMemos=(m_pShp->m_uFlags&NTL_FLG_TESTMEMOS)!=0;
	m_bSelectable=(m_pShp->m_uFlags&NTL_FLG_NONSELECTABLE)==0;
	m_bSelectableSrch=!m_pShp->IsSearchExcluded();
	m_bShowLabels=(m_pShp->m_uFlags&NTL_FLG_LABELS)!=0;
	if(m_bShowLabels && (m_pShp->m_uFlags&NTL_FLG_SHOWLABELS))
		m_bShowLabels++;
	m_bShowMarkers=(m_pShp->m_uFlags&NTL_FLG_MARKERS)!=0;
	if(m_bShowMarkers && (m_pShp->m_uFlags&NTL_FLG_SHOWMARKERS))
		m_bShowMarkers++;
	m_clrPrev=pDoc->m_clrPrev;
	m_nLblFld=m_pShp->m_nLblFld;
	m_vSrchFlds=m_pShp->m_vSrchFlds;
	m_crLbl=m_pShp->m_crLbl;
	m_font=m_pShp->m_font;
	m_mstyle=m_pShp->m_mstyle;
	m_bDotCenter=(m_mstyle.wFilled&FSF_DOTCENTER)!=0;
	m_hBmpOld=NULL;

	VERIFY(0 <= d2d_pFactory->CreateDCRenderTarget(&d2d_props, &m_pRT)); //also calls Init2D()
}

CDlgSymbols::~CDlgSymbols()
{
	SafeRelease(&m_pRT);
}

bool CDlgSymbols::UpdateSrchFlds()
{
	int n=m_list2.GetCount();
	std::vector<BYTE> vSrchFlds(n,0);
	CString s;
	for(int i=0;i<n;i++) {
		m_list2.GetText(i,s);
		vSrchFlds[i]=m_pShp->m_pdb->FldNum(s);
	}
	return m_pShp->SwapSrchFlds(vSrchFlds);
}

void CDlgSymbols::UpdateMap()
{
	bool bMarkerChanged=memcmp(&m_pShp->m_mstyle,&m_mstyle,sizeof(SHP_MRK_STYLE))!=0;
	bool bLblFldChanged=m_nLblFld!=m_pShp->m_nLblFld;

	#define NTL_FLG_NEEDREFRESH (NTL_FLG_LABELS|NTL_FLG_SHOWLABELS|NTL_FLG_MARKERS|NTL_FLG_SHOWMARKERS|NTL_FLG_NONSELECTABLE)

	UINT uFlags=m_pShp->m_uFlags&~(NTL_FLG_NEEDREFRESH|NTL_FLG_SEARCHEXCLUDED|NTL_FLG_NONSELECTABLE|NTL_FLG_TESTMEMOS);
	uFlags |= (m_bShowLabels!=0)*NTL_FLG_LABELS+(m_bShowLabels==2)*NTL_FLG_SHOWLABELS +
			  (m_bShowMarkers!=0)*NTL_FLG_MARKERS+(m_bShowMarkers==2)*NTL_FLG_SHOWMARKERS +
			  (m_bSelectable==FALSE)*NTL_FLG_NONSELECTABLE +
			  (m_bSelectableSrch==FALSE)*NTL_FLG_SEARCHEXCLUDED +
			  (m_bTestMemos!=0)*NTL_FLG_TESTMEMOS;

	if(!bMarkerChanged && (uFlags&NTL_FLG_MARKERS)!=(m_pShp->m_uFlags&NTL_FLG_MARKERS)) {
		bMarkerChanged=true;
	}
	if(!bLblFldChanged && (uFlags&NTL_FLG_LABELS)!=(m_pShp->m_uFlags&NTL_FLG_LABELS)) {
		bLblFldChanged=true;
	}

	bool bNeedRefresh = bMarkerChanged || bLblFldChanged ||
		(m_pShp->m_uFlags&NTL_FLG_NEEDREFRESH)!=(uFlags&NTL_FLG_NEEDREFRESH) ||
		memcmp(&m_pShp->m_font,&m_font,sizeof(CLogFont))!=0 ||
		m_crLbl!=m_pShp->m_crLbl;

	if(bNeedRefresh || m_pShp->m_uFlags!=uFlags) {
		m_pShp->m_nLblFld=m_nLblFld;
		m_pShp->m_mstyle=m_mstyle;
		m_pShp->m_font=m_font;
		m_pShp->m_crLbl=m_crLbl;
		m_pShp->m_uFlags=uFlags;

		if(bMarkerChanged) {
			m_pShp->UpdateImage(m_pDoc->LayerSet().m_pImageList);
			if(hPropHook && m_pDoc==pLayerSheet->GetDoc())
				pLayerSheet->RefreshListItem(m_pShp);
		}

		if(bMarkerChanged || bLblFldChanged) {
			if(app_pShowDlg && app_pShowDlg->m_pDoc==m_pDoc)
				app_pShowDlg->m_PointTree.Invalidate(FALSE);
		}

		m_pDoc->SetChanged();
		if(bNeedRefresh && m_pShp->IsVisible()) m_pDoc->RefreshViews();
	}

}

void CDlgSymbols::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_SELECTABLE, m_bSelectable);
	DDX_Check(pDX, IDC_SELECTABLE_SRCH, m_bSelectableSrch);
	DDX_Check(pDX, IDC_SHOWLABELS, m_bShowLabels);
	DDX_Check(pDX, IDC_SHOWMARKERS, m_bShowMarkers);
	DDX_Check(pDX, IDC_DOT_CENTER, m_bDotCenter);
	DDX_Control(pDX, IDC_COLORFRM, m_rectSym);
	DDX_Control(pDX, IDC_FGCOLOR, m_BtnMrk);
	DDX_Control(pDX, IDC_BKGCOLOR, m_BtnBkg);
	DDX_Control(pDX, IDC_LBLCOLOR, m_BtnLbl);
	DDX_Control(pDX, IDC_BKGNDCOLOR, m_BtnBkgnd);
	DDX_Control(pDX, IDC_LIST1, m_list1);
	DDX_Control(pDX, IDC_LIST2, m_list2);
	DDX_Check(pDX, IDC_TEST_MEMOS, m_bTestMemos);
	DDX_Control(pDX, IDC_OPACITY_SYM2, m_sliderSym);
	DDX_Slider(pDX, IDC_OPACITY_SYM2, m_mstyle.iOpacitySym);

	if(pDX->m_bSaveAndValidate) {
		//m_mstyle.fLineWidth=(float)atof(GetFloatStr(m_mstyle.fLineWidth,1));
		if(UpdateSrchFlds() || (m_bSelectableSrch==TRUE)==m_pShp->IsSearchExcluded()) {
			m_pShp->m_bSearchExcluded=(m_bSelectableSrch==FALSE);
			if(m_pShp->m_pvSrchFlds!=&m_pShp->m_vSrchFlds) {
				delete m_pShp->m_pvSrchFlds;
				m_pShp->m_pvSrchFlds=&m_pShp->m_vSrchFlds;
			}
			m_pDoc->SetChanged();
		}
		m_pDoc->m_clrPrev=m_clrPrev;
		UpdateMap();
	}
}

BEGIN_MESSAGE_MAP(CDlgSymbols, CDialog)
    ON_MESSAGE(CPN_SELCHANGE, OnChgColor)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_MSIZE, OnMarkerSize)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_MLINEW, OnMarkerLineWidth)
	ON_EN_CHANGE(IDC_MSIZE, OnEnChangeMarkerSize)
	ON_EN_CHANGE(IDC_MLINEW, OnEnChangeMarkerLineWidth)
	ON_CBN_SELCHANGE(IDC_MSHAPE, OnSelShape)
	ON_CBN_SELCHANGE(IDC_LBLFIELD, OnSelField)
	ON_LBN_DBLCLK(IDC_LIST1,OnListDblClk)
	ON_LBN_DBLCLK(IDC_LIST2,OnListDblClk)
	ON_BN_CLICKED(IDC_LBLFONT, OnLblFont)
	ON_BN_CLICKED(IDC_MOPAQUE, OnFilled)
	ON_BN_CLICKED(IDC_MCLEAR, OnClear)
	ON_WM_DESTROY()
	ON_WM_PAINT()
	ON_BN_CLICKED(IDC_DOT_CENTER, OnBnClickedDotCenter)
	ON_BN_CLICKED(IDC_APPLY, OnBnClickedApply)
	ON_BN_CLICKED(IDC_MOVELEFT, OnBnClickedMoveleft)
	ON_BN_CLICKED(IDC_MOVERIGHT, OnBnClickedMoveright)
	ON_MESSAGE(WM_CTLCOLORLISTBOX,OnColorListbox)
    ON_MESSAGE(WM_COMMANDHELP,OnCommandHelp)
END_MESSAGE_MAP()

void CDlgSymbols::InitLabelFields()
{
	CComboBox *pcb=(CComboBox *)GetDlgItem(IDC_LBLFIELD);
	CShpDBF *pdb=m_pShp->m_pdb;

	int nFlds=pdb->NumFlds();
	ASSERT(m_nLblFld && m_pShp->m_nLblFld==m_nLblFld);

	std::vector<BYTE> vSrchFlgs(nFlds+1,0);
	for(std::vector<BYTE>::iterator it=m_vSrchFlds.begin();it!=m_vSrchFlds.end();it++) {
		vSrchFlgs[*it]=1;  //field *it is searchable
		m_list2.AddString(pdb->FldNamPtr(*it));
	}

	int nFldsUsed=0,nFldSel=-1;

	for(int n=1;n<=nFlds;n++) {
		LPCSTR p=pdb->FldNamPtr(n);
		if(pdb->FldTyp(n)!='M') {
			VERIFY(pcb->AddString(p)==nFldsUsed);
			if(n==m_nLblFld) nFldSel=nFldsUsed;
			nFldsUsed++;
		}
		if(!vSrchFlgs[n]) m_list1.AddString(p);
	}
	ASSERT(nFldsUsed && nFldSel>=0);
	VERIFY(CB_ERR!=pcb->SetCurSel(nFldSel));
	m_list1.SetCurSel(0); //ignore possible errors
	m_list2.SetCurSel(0);
}

BOOL CDlgSymbols::OnInitDialog()
{
	CDialog::OnInitDialog();

	SetWndTitle(this,0,m_pShp->Title());

	m_BtnLbl.SetColor(m_crLbl);
	m_BtnBkgnd.SetColor(m_clrPrev);

	m_BtnMrk.SetColor(m_mstyle.crMrk);
	m_BtnBkg.SetColor(m_mstyle.crBkg);

    m_rectSym.GetClientRect(&m_rectBox);

    m_sizeBox.cx = m_rectBox.right; //-2; //Add 2 since we will InflateRect(1,1)
    m_sizeBox.cy = m_rectBox.bottom; //-2;
	m_ptCtrBox.x = m_sizeBox.cx/2;
	m_ptCtrBox.y = m_sizeBox.cy/2;

    m_rectSym.ClientToScreen(&m_rectBox);
    ScreenToClient(&m_rectBox);
    //m_rectBox.InflateRect(-1,-1);
        
    // Get temporary DC for dialog - Will be released in dc destructor
    CClientDC dc(this);
    VERIFY(m_cBmp.CreateCompatibleBitmap(&dc,m_sizeBox.cx,m_sizeBox.cy));
    VERIFY(m_dcBmp.CreateCompatibleDC(&dc));
	//Use API so as to keep m_hBmpOld valid --
    VERIFY(m_hBmpOld=(HBITMAP)::SelectObject(m_dcBmp.m_hDC,m_cBmp.GetSafeHandle()));

	VERIFY(0<=m_pRT->BindDC(m_dcBmp,CRect(0, 0, m_sizeBox.cx, m_sizeBox.cy)));
	m_pRT->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

	VERIFY(InitLabelDC(m_dcBmp.m_hDC,&m_font,m_crLbl));
	GetLabelSize();
	InitLabelFields();

    VERIFY(CB_ERR!=((CComboBox *)GetDlgItem(IDC_MSHAPE))->SetCurSel(m_mstyle.wShape));
	VERIFY(::CheckRadioButton(m_hWnd,IDC_MCLEAR,IDC_MOPAQUE,(m_mstyle.wFilled&FSF_FILLED)?IDC_MOPAQUE:IDC_MCLEAR));

	UpdateOpacityLabel();

	GetDlgItem(IDC_MSIZE)->SetWindowTextA(GetIntStr(m_mstyle.wSize));
	GetDlgItem(IDC_MLINEW)->SetWindowTextA(GetFloatStr(m_mstyle.fLineWidth,1));
	GetDlgItem(IDC_TEST_MEMOS)->EnableWindow(m_pShp->HasMemos());

	m_bInitFlag=false;

	return TRUE;  // return TRUE unless you set the focus to a control
}

void CDlgSymbols::OnDestroy() 
{
	CDialog::OnDestroy();
	if(m_dcBmp.m_hDC) {
		VERIFY(InitLabelDC(m_dcBmp.m_hDC));
		if(m_hBmpOld)
			VERIFY(::SelectObject(m_dcBmp.m_hDC,m_hBmpOld));
		VERIFY(m_dcBmp.DeleteDC());
	}
}

void CDlgSymbols::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	
	dc.BitBlt(m_rectBox.left,m_rectBox.top,m_rectBox.Width(),m_rectBox.Height(),
	  	&m_dcBmp,0,0,SRCCOPY);
}

void CDlgSymbols::DrawSymbol()
{
	int isize=(m_mstyle.wSize-1)/2;
	int iOff=max(isize-2,1);
	CPoint pt(m_ptCtrBox);
	pt.x-=(m_szLabel.cx+iOff+2*isize)/2;
	if(pt.x<=isize) pt.x=isize+1;
	pt.y-=(m_szLabel.cy+iOff+2+isize)/2;
	if (pt.y <= isize) pt.y = isize + 1;

	m_dcBmp.FillSolidRect(0, 0, m_sizeBox.cx, m_sizeBox.cy, m_clrPrev);

	//Place symbol on bitmap --
	m_pRT->BeginDraw();
	CPlaceMark::PlotStyleSymbol(m_pRT, m_mstyle, pt);
	VERIFY(0 <= m_pRT->EndDraw());

	//Place label on bitmap --
	m_dcBmp.TextOut(pt.x+iOff,pt.y+iOff,"AaBbYyZz",8);

	CRect rect(m_rectBox);
	rect.DeflateRect(1,1); //don't refresh border
	InvalidateRect(&rect,FALSE);
}

LRESULT CDlgSymbols::OnChgColor(WPARAM clr,LPARAM id)
{
	if(id==IDC_LBLCOLOR) {
		if(clr==m_crLbl) return TRUE;
		::SetTextColor(m_dcBmp.m_hDC,m_crLbl=clr);
	}
	else if(id==IDC_BKGCOLOR) {
		if(clr==m_mstyle.crBkg) return TRUE;
		m_mstyle.crBkg=clr;
	}
	else if(id==IDC_FGCOLOR) {
		if(clr==m_mstyle.crMrk) return TRUE;
		m_mstyle.crMrk=clr;
	}
	else {
		ASSERT(id == IDC_BKGNDCOLOR);
		m_clrPrev=clr;
	}
	DrawSymbol();
	return TRUE;
}

void CDlgSymbols::UpdateMarkerSize(int inc0)
{
	CString s;
	GetDlgItem(IDC_MSIZE)->GetWindowText(s);
	if(!inc0 && s.IsEmpty()) return;
	int i=atoi(s)-inc0;
	if(i<=0) i=0;
	else if(i>CShpLayer::MARKER_MAXSIZE) i=CShpLayer::MARKER_MAXSIZE;
	else if(!(i&1)) i-=inc0;
	if(inc0) {
		m_bInitFlag=true;
		GetDlgItem(IDC_MSIZE)->SetWindowText(GetIntStr(i));
		m_bInitFlag=false;
	}
	m_mstyle.wSize=(WORD)i;
	DrawSymbol();
}

void CDlgSymbols::OnMarkerSize(NMHDR* pNMHDR, LRESULT* pResult) 
{
	*pResult = 1;
	UpdateMarkerSize(((NM_UPDOWN*)pNMHDR)->iDelta);
}

void CDlgSymbols::OnEnChangeMarkerSize()
{
	if(!m_bInitFlag) UpdateMarkerSize(0);
}

void CDlgSymbols::UpdateMarkerLineWidth(int inc0)
{
	CString s;
	GetDlgItem(IDC_MLINEW)->GetWindowText(s);
	if(!inc0 && s.IsEmpty()) return;
	double fsiz=atof(s)-inc0*0.2;
	if(fsiz<0.0) fsiz=0;
	else if(fsiz>CShpLayer::MARKER_MAXLINEW) fsiz=CShpLayer::MARKER_MAXLINEW;
	s=GetFloatStr(fsiz,1);
	if(inc0) {
		m_bInitFlag=true;
		GetDlgItem(IDC_MLINEW)->SetWindowTextA(s);
		m_bInitFlag=false;
	}
	m_mstyle.fLineWidth=(float)atof(s);
	DrawSymbol();
}

void CDlgSymbols::OnMarkerLineWidth(NMHDR* pNMHDR, LRESULT* pResult) 
{
	*pResult = 1;
	UpdateMarkerLineWidth(((NM_UPDOWN*)pNMHDR)->iDelta);
}

void CDlgSymbols::OnEnChangeMarkerLineWidth()
{
	if(!m_bInitFlag) UpdateMarkerLineWidth(0);
}

void CDlgSymbols::OnFilled() 
{
	m_mstyle.wFilled|=FSF_FILLED;
	DrawSymbol();
}

void CDlgSymbols::OnClear() 
{
	m_mstyle.wFilled&=~FSF_FILLED;
	DrawSymbol();
}

void CDlgSymbols::OnSelShape() 
{
	int i=((CComboBox *)GetDlgItem(IDC_MSHAPE))->GetCurSel();
	ASSERT(i>=0);
	m_mstyle.wShape=i;
	DrawSymbol();
}

void CDlgSymbols::OnSelField() 
{
	CComboBox *pcb=(CComboBox *)GetDlgItem(IDC_LBLFIELD);
	int i=pcb->GetCurSel();
	ASSERT(i>=0);
	if(i>=0) {
		CString s;
		pcb->GetLBText(i,s);
		VERIFY(m_nLblFld=m_pShp->m_pdb->FldNum(s));
	}
}

void CDlgSymbols::OnBnClickedApply()
{
	//m_bShowLabels=((CButton *)GetDlgItem(IDC_SHOWLABELS))->GetCheck(); 
	//m_bShowMarkers=((CButton *)GetDlgItem(IDC_SHOWMARKERS))->GetCheck(); 
	UpdateData();
}

void CDlgSymbols::OnBnClickedDotCenter()
{
	if(m_bDotCenter=!m_bDotCenter)
		m_mstyle.wFilled|=FSF_DOTCENTER;
	else m_mstyle.wFilled&=~FSF_DOTCENTER;
	if(m_mstyle.wShape!=FST_PLUSES) DrawSymbol();
}

void CDlgSymbols::GetLabelSize()
{
	m_szLabel=m_dcBmp.GetTextExtent("AaBbYyZz",8);
}

void CDlgSymbols::OnLblFont()
{
	if(m_font.GetFontFromDialog()) {
		InitLabelDC(m_dcBmp.m_hDC);
		VERIFY(InitLabelDC(m_dcBmp.m_hDC,&m_font,m_crLbl));
		GetLabelSize();
		DrawSymbol();
	}
}

void CDlgSymbols::OnBnClickedMoveleft()
{
	int i2=m_list2.GetCurSel();
	if(i2>=0) {
		CShpDBF *pdb=m_pShp->m_pdb;
		CString s1,s2;
		m_list2.GetText(i2,s2);
		m_list2.DeleteString(i2);
		if(i2==m_list2.GetCount()) i2--;
		m_list2.SetCurSel(i2);
		UINT f=pdb->FldNum(s2);
		int i1max=m_list1.GetCount();
		int i1;
		for(i1=0;i1<i1max;i1++) {
			m_list1.GetText(i1,s1);
			if(pdb->FldNum(s1)>=f) break;
		}
		if(i1==i1max) i1=-1;
		i1=m_list1.InsertString(i1,s2);
		m_list1.SetCurSel(i1);
	}
}

void CDlgSymbols::OnBnClickedMoveright()
{
	int i1=m_list1.GetCurSel();
	if(i1>=0) {
		CString s1;
		m_list1.GetText(i1,s1);
		m_list1.DeleteString(i1);
		if(i1==m_list1.GetCount()) i1--;
		m_list1.SetCurSel(i1);
		i1=m_list2.GetCurSel()+1;
		if(i1==m_list2.GetCount()) i1=-1;
		i1=m_list2.InsertString(i1,s1);
		m_list2.SetCurSel(i1);
	}
}

void CDlgSymbols::OnListDblClk()
{
	const MSG* pMsg=GetCurrentMessage();
	switch(pMsg->wParam&0xFFFF) {
		case IDC_LIST1 : OnBnClickedMoveright(); break;
		case IDC_LIST2 : OnBnClickedMoveleft(); break;
		default : ASSERT(0);
	}
}

LRESULT CDlgSymbols::OnColorListbox(WPARAM wParam,LPARAM lParam)
{
	if((HWND)lParam==m_list1.m_hWnd) {
		::SetBkColor((HDC)wParam,::GetSysColor(COLOR_BTNFACE));
		return (LRESULT)::GetSysColorBrush(COLOR_BTNFACE);
	}
	return Default();
}

void CDlgSymbols::UpdateOpacityLabel()
{
	CString s;
	s.Format("%3d %%",m_mstyle.iOpacitySym);
	GetDlgItem(IDC_ST_OPACITY_SYM)->SetWindowText(s);
	DrawSymbol();
}

BOOL CDlgSymbols::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	// Slider Control
	if(wParam == IDC_OPACITY_SYM2)
	{
		int i=m_sliderSym.GetPos();
		if(i!=m_mstyle.iOpacitySym) {
			m_mstyle.iOpacitySym=i;
			UpdateOpacityLabel();
		}
	}
	return CDialog::OnNotify(wParam, lParam, pResult);
}

LRESULT CDlgSymbols::OnCommandHelp(WPARAM wNone, LPARAM lParam)
{
	AfxGetApp()->WinHelp(1013);
	return TRUE;
}


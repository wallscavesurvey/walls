// DlgSymbolsPoly.cpp : implementation file
//

#include "stdafx.h"
#include "WallsMap.h"
#include "WallsMapDoc.h"
#include "LayerSetSheet.h"
#include "dlgSymbolsPoly.h"
#include "QuadTree.h"

// CDlgSymbolsPoly dialog

IMPLEMENT_DYNAMIC(CDlgSymbolsPoly, CDialog)
CDlgSymbolsPoly::CDlgSymbolsPoly(CWallsMapDoc *pDoc,CShpLayer *pLayer,CWnd* pParent /*=NULL*/)
	: CDialog(CDlgSymbolsPoly::IDD,pParent),m_pDoc(pDoc),m_pLayer(pLayer)
	, m_bUseIndex(FALSE)
{
	m_bInitFlag=true;
	m_bTestMemos=(m_pLayer->m_uFlags&NTL_FLG_TESTMEMOS)!=0;
	m_bShowLabels=(m_pLayer->m_uFlags&NTL_FLG_LABELS)!=0;
	if(m_bShowLabels && (m_pLayer->m_uFlags&NTL_FLG_SHOWLABELS))
		m_bShowLabels++;
	m_nLblFld=m_pLayer->m_nLblFld;
	m_crLbl=m_pLayer->m_crLbl;
	m_font=m_pLayer->m_font;
	m_mstyle=m_pLayer->m_mstyle;
	m_iOpacitySym=m_mstyle.iOpacitySym;
	m_iOpacityVec=m_mstyle.iOpacityVec;
	m_bAntialias=((m_mstyle.wFilled&FSF_ANTIALIAS)!=0);
	m_uIdxDepth=m_pLayer->m_bQTdepth;
	if(!m_uIdxDepth) m_uIdxDepth=6;
	m_bUseIndex=m_pLayer->m_bQTusing!=0;
}

CDlgSymbolsPoly::~CDlgSymbolsPoly()
{
}

void CDlgSymbolsPoly::UpdateMap()
{
	bool bMarkerChanged=memcmp(&m_pLayer->m_mstyle,&m_mstyle,sizeof(SHP_MRK_STYLE))!=0;
	UINT uFlags=(m_pLayer->m_uFlags&~(NTL_FLG_LABELS|NTL_FLG_SHOWLABELS|NTL_FLG_TESTMEMOS));
	uFlags |= (m_bShowLabels!=0)*NTL_FLG_LABELS+(m_bShowLabels==2)*NTL_FLG_SHOWLABELS+(m_bTestMemos!=0)*NTL_FLG_TESTMEMOS;

	bool bChanged=bMarkerChanged ||
		memcmp(&m_pLayer->m_font,&m_font,sizeof(CLogFont))!=0 ||
		m_crLbl!=m_pLayer->m_crLbl ||
		uFlags!=m_pLayer->m_uFlags ||
		m_nLblFld!=m_pLayer->m_nLblFld;

	if(bChanged) {
		m_pLayer->m_nLblFld=m_nLblFld;
		m_pLayer->m_mstyle=m_mstyle;
		m_pLayer->m_font=m_font;
		m_pLayer->m_crLbl=m_crLbl;
		m_pLayer->m_uFlags=uFlags;

		if(bMarkerChanged) {
			m_pLayer->UpdateImage(m_pDoc->LayerSet().m_pImageList);
			if(hPropHook && m_pDoc==pLayerSheet->GetDoc())
				pLayerSheet->RefreshListItem(m_pLayer);
		}

		m_pDoc->SetChanged();
		if(m_pLayer->IsVisible()) m_pDoc->RefreshViews();
	}
}

void CDlgSymbolsPoly::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_SHOWLABELS, m_bShowLabels);
	DDX_Control(pDX, IDC_FGCOLOR, m_BtnMrk);
	DDX_Control(pDX, IDC_BKGCOLOR, m_BtnBkg);
	DDX_Control(pDX, IDC_LBLCOLOR, m_BtnLbl);
	DDX_Control(pDX, IDC_OPACITY_SYM, m_sliderSym);
	DDX_Slider(pDX, IDC_OPACITY_SYM, m_iOpacitySym);

	DDX_Control(pDX, IDC_OPACITY_VEC, m_sliderVec);
	DDX_Slider(pDX, IDC_OPACITY_VEC, m_iOpacityVec);
	DDX_Check(pDX, IDC_ANTIALIAS, m_bAntialias);
	DDX_Check(pDX, IDC_USE_INDEX, m_bUseIndex);
	DDX_Check(pDX, IDC_TEST_MEMOS2, m_bTestMemos);

	if(pDX->m_bSaveAndValidate) {
		if((BYTE)m_bUseIndex!=m_pLayer->m_bQTusing || (BYTE)m_uIdxDepth!=m_pLayer->m_bQTdepth) {
			if((BYTE)m_uIdxDepth!=m_pLayer->m_bQTdepth) {
				m_pLayer->m_bQTdepth=(BYTE)m_uIdxDepth;
				m_pLayer->m_uQTnodes=0;
			}
			if(m_pLayer->m_pQT) {
				ASSERT(m_pLayer->m_bQTusing);
				ASSERT(m_pLayer->m_pdbfile->ppoly);
				delete m_pLayer->m_pQT;
				m_pLayer->m_pQT=NULL;
			}
			m_pLayer->m_bQTusing=m_bUseIndex;
			if(m_bUseIndex && m_pLayer->m_ext.size()==m_pLayer->m_nNumRecs) {
				m_pLayer->AllocateQT();
			}
			m_pDoc->SetChanged();
		}
		m_mstyle.iOpacitySym=m_iOpacitySym;
		m_mstyle.iOpacityVec=m_iOpacityVec;
		if(m_bAntialias) m_mstyle.wFilled|=(FSF_ANTIALIAS+FSF_USED2D);
		else {
			m_mstyle.wFilled&=~FSF_ANTIALIAS;
			if(m_iOpacitySym!=100 && (m_mstyle.wFilled&FSF_FILLED) || m_iOpacityVec!=100 && m_mstyle.fLineWidth) {
				m_mstyle.wFilled|=FSF_USED2D;
			}
			else m_mstyle.wFilled&=~FSF_USED2D;
		}
		UpdateMap();
	}
}

BEGIN_MESSAGE_MAP(CDlgSymbolsPoly, CDialog)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPINSIZE_VEC, OnLineSize)
	ON_EN_CHANGE(IDC_EDIT_VEC_SZ, OnEnChangeLineSize)
    ON_MESSAGE(CPN_SELCHANGE, OnChgColor)
	ON_CBN_SELCHANGE(IDC_LBLFIELD, OnSelField)
	ON_BN_CLICKED(IDC_LBLFONT, OnLblFont)
	ON_BN_CLICKED(IDC_MOPAQUE, OnFilled)
	ON_BN_CLICKED(IDC_MCLEAR, OnClear)
	ON_BN_CLICKED(IDC_APPLY, OnBnClickedApply)
    ON_MESSAGE(WM_COMMANDHELP,OnCommandHelp)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_IDXDEPTH, OnIdxDepth)
	ON_BN_CLICKED(IDC_USE_INDEX, &CDlgSymbolsPoly::OnBnClickedUseIndex)
END_MESSAGE_MAP()


// CDlgSymbolsPoly message handlers

void CDlgSymbolsPoly::InitLabelFields()
{
	CComboBox *pcb=(CComboBox *)GetDlgItem(IDC_LBLFIELD);
	CShpDBF *pdb=m_pLayer->m_pdb;
	UINT nFlds=pdb->NumFlds();
	for(UINT n=1;n<=nFlds;n++) {
		VERIFY(pcb->AddString(pdb->FldNamPtr(n))==n-1);
	}
	if(m_nLblFld>nFlds) m_nLblFld=1; 
	VERIFY(CB_ERR!=pcb->SetCurSel(m_nLblFld-1));
}

BOOL CDlgSymbolsPoly::OnInitDialog()
{
	CDialog::OnInitDialog();

	SetWndTitle(this,0,m_pLayer->Title());

	/*
	m_ToolTips.Create(this);
	m_BtnMrk.AddToolTip(&m_ToolTips);
	m_BtnLbl.AddToolTip(&m_ToolTips);
	*/
	m_BtnMrk.SetColor(m_mstyle.crMrk);
	m_BtnLbl.SetColor(m_crLbl);

	InitLabelFields();

	if(/*!m_bUseIndex && */ m_pLayer->ShpType()!=CShpLayer::SHP_POLYGON) {
		Hide(IDC_EDIT_IDXDEPTH);
		Hide(IDC_SPIN_IDXDEPTH);
		Hide(IDC_ST_DEPTH);
		Hide(IDC_USE_INDEX);
	}
	else {
		RefreshIdxDepth();
		if(!m_bUseIndex) GetDlgItem(IDC_EDIT_IDXDEPTH)->EnableWindow(0);
	}

	GetDlgItem(IDC_EDIT_VEC_SZ)->SetWindowTextA(GetFloatStr(m_mstyle.fLineWidth,1));
	m_bInitFlag=false;
	
	m_sliderSym.SetRange(0,100);
	m_sliderVec.SetRange(0,100);

	if(m_pLayer->ShpType()==CShpLayer::SHP_POLYGON) {
		//m_BtnBkg.AddToolTip(&m_ToolTips);
		m_BtnBkg.SetColor(m_mstyle.crBkg);
		VERIFY(::CheckRadioButton(m_hWnd,IDC_MCLEAR,IDC_MOPAQUE,(m_mstyle.wFilled&FSF_FILLED)?IDC_MOPAQUE:IDC_MCLEAR));
	}
	else {
		GetDlgItem(IDC_ST_POLYLINE)->SetWindowText("Polyline Style");
		Enable(IDC_ST_POLYINT,FALSE);
		Enable(IDC_BKGCOLOR,FALSE);
		Enable(IDC_MCLEAR,FALSE);
		Enable(IDC_MOPAQUE,FALSE);
		Enable(IDC_ST_FILLCLR,FALSE);
		Enable(IDC_ST_INTOPACITY,FALSE);
		Enable(IDC_ST_OPACITY_SYM,FALSE);
		Enable(IDC_OPACITY_SYM,FALSE);
	}

	GetDlgItem(IDC_TEST_MEMOS2)->EnableWindow(m_pLayer->HasMemos());

	return TRUE;  // return TRUE unless you set the focus to a control
}

LRESULT CDlgSymbolsPoly::OnChgColor(WPARAM clr,LPARAM id)
{
	if(id==IDC_LBLCOLOR) {
		m_crLbl=clr;
		return TRUE;
	}
	if(id==IDC_BKGCOLOR) {
		ASSERT(m_pLayer->ShpType()==CShpLayer::SHP_POLYGON);
		m_mstyle.crBkg=clr;
		//UpdateBrush();
	}
	else {
		ASSERT(id==IDC_FGCOLOR);
		m_mstyle.crMrk=clr;
		//UpdatePen();
	}
	return TRUE;
}

void CDlgSymbolsPoly::UpdateLineSize(int inc0)
{
	CString s;
	GetDlgItem(IDC_EDIT_VEC_SZ)->GetWindowText(s);
	if(!inc0 && s.IsEmpty()) return;
	double fsiz=atof(s)-inc0*0.2;
	if(fsiz<0.0) fsiz=0;
	else if(fsiz>15.0) fsiz=15.0;
	s=GetFloatStr(fsiz,1);
	if(inc0) {
		m_bInitFlag=true;
		GetDlgItem(IDC_EDIT_VEC_SZ)->SetWindowTextA(s);
		m_bInitFlag=false;
	}
	m_mstyle.fLineWidth=(float)atof(s);
}

void CDlgSymbolsPoly::OnLineSize(NMHDR* pNMHDR, LRESULT* pResult) 
{
	*pResult=1;
	UpdateLineSize(((NM_UPDOWN*)pNMHDR)->iDelta);
}

void CDlgSymbolsPoly::RefreshIdxDepth()
{
	CString s;
	s.Format("%u",m_uIdxDepth);
	GetDlgItem(IDC_EDIT_IDXDEPTH)->SetWindowText(s);
}

// CGEDefaultsDlg message handlers
void CDlgSymbolsPoly::OnIdxDepth(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
	int i=m_uIdxDepth-pNMUpDown->iDelta;
	if(i<3) i=10;
	else if(i>10) i=3;
	m_uIdxDepth=i;
	RefreshIdxDepth();
	*pResult = TRUE;
}

void CDlgSymbolsPoly::OnEnChangeLineSize() 
{
	if(!m_bInitFlag) UpdateLineSize(0);
}

void CDlgSymbolsPoly::OnFilled() 
{
	ASSERT(m_pLayer->ShpType()==CShpLayer::SHP_POLYGON);
	m_mstyle.wFilled|=FSF_FILLED;
}

void CDlgSymbolsPoly::OnClear() 
{
	ASSERT(m_pLayer->ShpType()==CShpLayer::SHP_POLYGON);
	m_mstyle.wFilled&=~FSF_FILLED;
}

void CDlgSymbolsPoly::OnSelField() 
{
	int i=((CComboBox *)GetDlgItem(IDC_LBLFIELD))->GetCurSel();
	ASSERT(i>=0);
	m_nLblFld=i+1;
}

void CDlgSymbolsPoly::OnBnClickedApply()
{
	//m_bShowLabels=((CButton *)GetDlgItem(IDC_SHOWLABELS))->GetCheck(); 
	UpdateData();
}

void CDlgSymbolsPoly::OnLblFont()
{
	m_font.GetFontFromDialog();
}

LRESULT CDlgSymbolsPoly::OnCommandHelp(WPARAM wNone, LPARAM lParam)
{
	AfxGetApp()->WinHelp(1014);
	return TRUE;
}
BOOL CDlgSymbolsPoly::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	// Slider Control
	int id=0;
	int i;
	if(wParam == m_sliderSym.GetDlgCtrlID())
	{
		i=m_sliderSym.GetPos();
		id=IDC_ST_OPACITY_SYM;
	}
	else if(wParam == m_sliderVec.GetDlgCtrlID())
	{
		i=m_sliderVec.GetPos();
		id=IDC_ST_OPACITY_VEC;
	}
	if(id) {
		CString s;
		s.Format("%3d %%",i);
		GetDlgItem(id)->SetWindowText(s);
		//m_pView->UpdateOpacity(id,i);
	}
	return CDialog::OnNotify(wParam, lParam, pResult);
}


void CDlgSymbolsPoly::OnBnClickedUseIndex()
{
	m_bUseIndex=!m_bUseIndex;
	GetDlgItem(IDC_EDIT_IDXDEPTH)->EnableWindow(m_bUseIndex);
}

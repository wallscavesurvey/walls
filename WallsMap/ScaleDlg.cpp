// ScaleDlg.cpp : implementation file
//

#include "stdafx.h"
#include "WallsMap.h"
#include "MainFrm.h"
#include "WallsMapDoc.h"
#include "WallsMapView.h"
#include "ShpLayer.h"
#include "ScaleDlg.h"

// CScaleDlg dialog

IMPLEMENT_DYNAMIC(CScaleDlg, CDialog)

CScaleDlg::CScaleDlg(CWallsMapDoc *pDoc,CWnd* pParent /*=NULL*/)
	: CDialog(CScaleDlg::IDD, pParent),m_pDoc(pDoc),m_pFolder(NULL)
{
	m_pLayer=(PML)pDoc->SelectedLayerPtr();
	if(m_pLayer->IsFolder()) {
		m_pFolder=m_pLayer;
		m_pLayer=(PML)m_pLayer->FirstChildPML();
		VERIFY(m_pLayer);
	}
		
	m_bLayerShowAll=!m_pLayer->scaleRange.bLayerNoShow;
	m_csLayerZoomIn.Format("%u",m_pLayer->scaleRange.uLayerZoomIn);
	m_csLayerZoomOut.Format("%u",m_pLayer->scaleRange.uLayerZoomOut);

	m_bLabelShowAll=!m_pLayer->scaleRange.bLabelNoShow;
	m_csLabelZoomIn.Format("%u",m_pLayer->scaleRange.uLabelZoomIn);
	m_csLabelZoomOut.Format("%u",m_pLayer->scaleRange.uLabelZoomOut);
}

CScaleDlg::~CScaleDlg()
{
}

void CScaleDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Radio(pDX, IDC_LABEL_NOSHOW, m_bLabelShowAll);
	DDX_Radio(pDX, IDC_LAYER_NOSHOW, m_bLayerShowAll);

	DDX_Control(pDX, IDC_LABEL_ZOOMIN, m_ceLabelZoomIn);
	DDX_Text(pDX, IDC_LABEL_ZOOMIN, m_csLabelZoomIn);
	DDV_MaxChars(pDX, m_csLabelZoomIn, 12);

	DDX_Control(pDX, IDC_LABEL_ZOOMOUT, m_ceLabelZoomOut);
	DDX_Text(pDX, IDC_LABEL_ZOOMOUT, m_csLabelZoomOut);
	DDV_MaxChars(pDX, m_csLabelZoomOut, 12);

	DDX_Control(pDX, IDC_LAYER_ZOOMIN, m_ceLayerZoomIn);
	DDX_Text(pDX, IDC_LAYER_ZOOMIN, m_csLayerZoomIn);
	DDV_MaxChars(pDX, m_csLayerZoomIn, 12);

	DDX_Control(pDX, IDC_LAYER_ZOOMOUT, m_ceLayerZoomOut);
	DDX_Text(pDX, IDC_LAYER_ZOOMOUT, m_csLayerZoomOut);
	DDV_MaxChars(pDX, m_csLayerZoomOut, 12);

	if(pDX->m_bSaveAndValidate) {
		CScaleRange range;
		range.bLabelNoShow=!m_bLabelShowAll;
		range.bLayerNoShow=!m_bLayerShowAll;
		range.uLabelZoomIn=atoi(m_csLabelZoomIn);
		range.uLabelZoomOut=atoi(m_csLabelZoomOut);
		range.uLayerZoomIn=atoi(m_csLayerZoomIn);
		range.uLayerZoomOut=atoi(m_csLayerZoomOut);
		if(SetChildRanges(m_pFolder?(PML)m_pFolder:m_pLayer,range)) {
			m_pDoc->RefreshViews();
		}
	}
}

bool CScaleDlg::SetChildRanges(PML pml,CScaleRange &range)
{
	bool bRet=false;
	if(!pml->IsFolder()) {
		if(pml->LayerType()!=TYP_SHP) range.bLabelNoShow=0;
		if(memcmp(&range,&pml->scaleRange,sizeof(CScaleRange))) {
			int iVisibility=pml->IsVisible()?pml->Visibility():0;
			pml->scaleRange=range;
			m_pDoc->SetChanged();
			bRet=pml->IsVisible() && iVisibility!=pml->Visibility();
		}
		range.bLabelNoShow=!m_bLabelShowAll;
		return bRet;
	}
	for(pml=(PML)pml->child;pml;pml=(PML)pml->nextsib) {
	  if(SetChildRanges(pml,range)) bRet=true;
	}
	return bRet;
}

BEGIN_MESSAGE_MAP(CScaleDlg, CDialog)
	ON_BN_CLICKED(IDC_LABEL_NOSHOW, &CScaleDlg::OnBnClickedLabelNoshow)
	ON_BN_CLICKED(IDC_LAYER_NOSHOW, &CScaleDlg::OnBnClickedLayerNoshow)
	ON_BN_CLICKED(IDC_LABEL_SHOWALL, &CScaleDlg::OnBnClickedLabelNoshow)
	ON_BN_CLICKED(IDC_LAYER_SHOWALL, &CScaleDlg::OnBnClickedLayerNoshow)
	ON_MESSAGE(WM_COMMANDHELP,OnCommandHelp)
END_MESSAGE_MAP()


// CScaleDlg message handlers

BOOL CScaleDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	SetWndTitle(this,0,(m_pFolder?m_pFolder:m_pLayer)->Title());

	{
		CString csFormat,csMsg;
		CWallsMapView *pView=m_pDoc->GetFirstView();
		ASSERT(pView && pView->GetDocument()==m_pDoc);
		GetDlgItem(IDC_ST_CURRENTSCALE)->GetWindowText(csFormat);
		csMsg.Format(csFormat,pView->GetScaleDenom());
		GetDlgItem(IDC_ST_CURRENTSCALE)->SetWindowText(csMsg);
	}

	if(m_pLayer->LayerType()!=TYP_SHP) {
		m_bLabelShowAll=TRUE;
		Enable(IDC_LABEL_NOSHOW,FALSE);
		Enable(IDC_LABEL_SHOWALL,FALSE);
	}

	EnableLayerRange();
	EnableLabelRange();

	// TODO:  Add extra initialization here

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CScaleDlg::EnableLayerRange()
{
	Enable(IDC_LAYER_ZOOMOUT,!m_bLayerShowAll);
	Enable(IDC_LAYER_ZOOMIN,!m_bLayerShowAll);
}

void CScaleDlg::EnableLabelRange()
{
	Enable(IDC_LABEL_ZOOMOUT,!m_bLabelShowAll);
	Enable(IDC_LABEL_ZOOMIN,!m_bLabelShowAll);
}

void CScaleDlg::OnBnClickedLabelNoshow()
{
	m_bLabelShowAll=!IsChecked(IDC_LABEL_NOSHOW);
	EnableLabelRange();
}

void CScaleDlg::OnBnClickedLayerNoshow()
{
	m_bLayerShowAll=!IsChecked(IDC_LAYER_NOSHOW);
	EnableLayerRange();
}

LRESULT CScaleDlg::OnCommandHelp(WPARAM wNone, LPARAM lParam)
{
	AfxGetApp()->WinHelp(1011);
	return TRUE;
}

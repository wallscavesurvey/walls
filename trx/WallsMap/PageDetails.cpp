// LayerSetPropSheet.cpp : implementation file
//

#include "stdafx.h"
#include "WallsMap.h"
#include "MainFrm.h"
#include "WallsMapDoc.h"
#include "WallsMapView.h"
#include "LayerSetSheet.h"

/////////////////////////////////////////////////////////////////////////////
// CPageDetails dialog

//IMPLEMENT_DYNCREATE(CPageDetails, CLayerSetPage)

CPageDetails::CPageDetails()
	: CLayerSetPage(CPageDetails::IDD),m_nPos(-1)
{
	m_bNewLayer=false;
	m_pDoc=NULL;
}

CPageDetails::~CPageDetails()
{
}

void CPageDetails::UpdateLayerData(CWallsMapDoc *pDoc)
{
	CLayerSet &lSet=pDoc->LayerSet();
	int nPos=lSet.SelectedLayerPos();
	ASSERT(nPos<lSet.NumLayers());
	if(m_bNewLayer || m_pDoc!=pDoc || nPos!=m_nPos) {
		m_bNewLayer=false;
		m_pDoc=pDoc; m_nPos=nPos;
		SetText(IDC_EDIT_DETAILS,lSet.LayerPtr(nPos)->GetInfo());
	}
}

void CPageDetails::DoDataExchange(CDataExchange* pDX)
{
	CLayerSetPage::DoDataExchange(pDX);
	if(!pDX->m_bSaveAndValidate) {
		UpdateLayerData(Sheet()->GetDoc());
		m_pDoc->GetPropStatus().nTab=1;
	}
}

BOOL CPageDetails::OnInitDialog()
{
	CLayerSetPage::OnInitDialog();
	AddControl(IDC_EDIT_DETAILS, CST_RESIZE, CST_RESIZE, CST_RESIZE, CST_RESIZE, 0);
	AddControl(IDCANCEL, CST_REPOS, CST_REPOS, CST_NONE, CST_REPOS, 0);
	return TRUE;
	//GetDlgItem(IDC_EDIT_DETAILS)->SetFocus();
	//return FALSE;
}

BEGIN_MESSAGE_MAP(CPageDetails, CLayerSetPage)
	ON_REGISTERED_MESSAGE(WM_RETFOCUS,OnRetFocus)
	ON_WM_SIZE()
	ON_BN_CLICKED(IDCANCEL, OnClose)
    ON_MESSAGE(WM_COMMANDHELP,OnCommandHelp)
END_MESSAGE_MAP()

// CPageDetails message handlers
LRESULT CPageDetails::OnRetFocus(WPARAM,LPARAM)
{
	//if(m_idLastFocus) GetDlgItem(m_idLastFocus)->SetFocus();
	//else m_idLastFocus=checkid(GetFocus()->GetDlgCtrlID(),getids());
	if(m_pDoc!=Sheet()->GetDoc())
		UpdateLayerData(Sheet()->GetDoc());
	m_pDoc->GetPropStatus().nTab=1;
	return TRUE;
}

void CPageDetails::OnClose()
{
	VERIFY(Sheet()->DestroyWindow());
}

LRESULT CPageDetails::OnCommandHelp(WPARAM wNone, LPARAM lParam)
{
	AfxGetApp()->WinHelp(0);
	return TRUE;
}


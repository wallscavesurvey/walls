// SelEditedDlg.cpp : implementation file
//

#include "stdafx.h"
#include "WallsMap.h"
#include "MainFrm.h"
#include "MapLayer.h"
#include "ShpLayer.h"
#include "WallsMapDoc.h"
#include "WallsMapView.h"
#include "SelEditedDlg.h"
#include "ShowShapeDlg.h"

static BOOL bEditAdd=TRUE,bEditDBF=TRUE,bEditDel=FALSE,bEditSHP=TRUE,bAddToSel=FALSE,bViewOnly=TRUE;

// CSelEditedDlg dialog

IMPLEMENT_DYNAMIC(CSelEditedDlg, CDialog)

CSelEditedDlg::CSelEditedDlg(CWallsMapView *pView, CWnd* pParent /*=NULL*/)
	: m_pView(pView), CDialog(CSelEditedDlg::IDD, pParent)
	, m_bEditAdd(bEditAdd)
	, m_bEditDBF(bEditDBF)
	, m_bEditDel(bEditDel)
	, m_bEditSHP(bEditSHP)
	, m_bAddToSel(bAddToSel)
	, m_bViewOnly(bViewOnly)
{
	m_bAddEnabled=(app_pShowDlg && app_pShowDlg->m_pDoc==m_pView->GetDocument() && !app_pShowDlg->IsEmpty());
	if(!m_bAddEnabled) m_bAddToSel=FALSE;
}

CSelEditedDlg::~CSelEditedDlg()
{
}

void CSelEditedDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_EDIT_ADD, m_bEditAdd);
	DDX_Check(pDX, IDC_EDIT_DBF, m_bEditDBF);
	DDX_Check(pDX, IDC_EDIT_DEL, m_bEditDel);
	DDX_Check(pDX, IDC_EDIT_SHP, m_bEditSHP);
	DDX_Check(pDX, IDC_ADDTOSEL, m_bAddToSel);
	DDX_Check(pDX, IDC_VIEW_ONLY, m_bViewOnly);
	if(pDX->m_bSaveAndValidate) {

		m_uFlags=m_bEditAdd*SHP_EDITADD+m_bEditSHP*SHP_EDITSHP+m_bEditDBF*SHP_EDITDBF+m_bEditDel*SHP_EDITDEL;

		if(!m_uFlags) {
			AfxMessageBox("You must select at least one modification type (or Cancel).");
			pDX->Fail();
		}

		bEditAdd=m_bEditAdd;
		bEditDBF=m_bEditDBF;
		bEditDel=m_bEditDel;
		bEditSHP=m_bEditSHP;
		bViewOnly=m_bViewOnly;
		if(m_bAddEnabled) bAddToSel=m_bAddToSel;

		//Build sorted vector of CShpLayer pointers and record numbers that have been flagged new --
		CFltRect rectView;
		if(m_bViewOnly) m_pView->GetViewExtent(rectView);

		//Fill and sort document's m_vec_shprec --
		BeginWaitCursor();
		UINT count=m_pView->GetDocument()->LayerSet().SelectEditedShapes(m_uFlags,m_bAddToSel,m_bViewOnly?&rectView:NULL);
		EndWaitCursor();

		if(!count) {
			CMsgBox("No%s records matching the selection criteria were found. "
				"Either modify the criteria or Cancel.",m_bAddToSel?" additional":"");
			pDX->Fail();
		}
	}
}


BEGIN_MESSAGE_MAP(CSelEditedDlg, CDialog)
    ON_MESSAGE(WM_COMMANDHELP,OnCommandHelp)
END_MESSAGE_MAP()


// CSelEditedDlg message handlers

BOOL CSelEditedDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	if(!m_bAddEnabled) GetDlgItem(IDC_ADDTOSEL)->EnableWindow(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

LRESULT CSelEditedDlg::OnCommandHelp(WPARAM wNone, LPARAM lParam)
{
	AfxGetApp()->WinHelp(1005);
	return TRUE;
}

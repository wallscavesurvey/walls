// DlgOptionsGE.cpp : implementation file
//

#include "stdafx.h"
#include "WallsMap.h"
#include "mainfrm.h"
#include "kmldef.h"
#include "ShpLayer.h"
#include "ShowShapeDlg.h"
#include "DlgOptionsGE.h"
#include "dbtfile.h"
#include <ShpDBF.h>

#define MAX_ICONSIZE 30

IMPLEMENT_DYNAMIC(CDlgOptionsGE, CDialog)

CDlgOptionsGE::CDlgOptionsGE(CShpLayer *pShp,VEC_DWORD &vRec,CWnd* pParent /*=NULL*/)
	: CDialog(CDlgOptionsGE::IDD, pParent)
	, m_pShp(pShp)
	, m_vFld(pShp->m_vKmlFlds)
	, m_vRec(vRec)
	, m_uRecCount(vRec.size())
	, m_uRecIdx(0)
	, m_bFldChg(false)
	, m_bFlyEnabled(GE_IsInstalled())
{
}

CDlgOptionsGE::~CDlgOptionsGE()
{
}

void CDlgOptionsGE::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_PATHNAME, m_pathName);
	DDV_MaxChars(pDX, m_pathName, 250);
	DDX_Control(pDX, IDC_PATHNAME, m_cePathName);
	DDX_Control(pDX, IDC_DESCRIPTION, m_ceDescription);

	if(pDX->m_bSaveAndValidate) {
		LPCSTR p=trx_Stpext(m_pathName);
		if(_stricmp(p,".kml")) {
			if(*p) {
				if(IDOK!=CMsgBox(MB_OKCANCEL,"NOTE: Only an uncompressed .kml file will be written.")) p=0;
				m_pathName.Truncate(m_pathName.ReverseFind('.'));
			}
			m_pathName+=".kml";
			m_cePathName.SetWindowText(m_pathName);
		}
		if(!p || !DirCheck((char *)(LPCSTR)m_pathName,TRUE)) {
			pDX->m_idLastControl=IDC_PATHNAME;
			pDX->m_bEditLastControl=TRUE;
			pDX->Fail();
			return;
		}
		if(m_bFldChg)
			m_pShp->m_vKmlFlds.swap(m_vFld);
	}
}

BEGIN_MESSAGE_MAP(CDlgOptionsGE, CDialog)
	ON_BN_CLICKED(IDC_SAVE, &CDlgOptionsGE::OnBnClickedSave)
	ON_BN_CLICKED(IDC_CLEAR_ALL, &CDlgOptionsGE::OnBnClickedClear)
	ON_BN_CLICKED(IDC_CLEAR_LAST, &CDlgOptionsGE::OnBnClickedClrLast)
	ON_BN_CLICKED(IDC_APPEND, &CDlgOptionsGE::OnBnClickedAppend)
	ON_BN_CLICKED(IDC_BROWSE, &CDlgOptionsGE::OnBnClickedBrowse)
	ON_BN_CLICKED(IDC_NEXT, &CDlgOptionsGE::OnBnClickedNext)
	ON_BN_CLICKED(IDC_PREV, &CDlgOptionsGE::OnBnClickedPrev)
	ON_BN_CLICKED(IDC_REMOVE_REC, &CDlgOptionsGE::OnBnClickedRemoveRec)
    ON_MESSAGE(WM_COMMANDHELP,OnCommandHelp)
END_MESSAGE_MAP()

void CDlgOptionsGE::InitLabelFields()
{
	CComboBox *pcb=(CComboBox *)GetDlgItem(IDC_DESC_FIELD);
	CShpDBF *pdb=m_pShp->m_pdb;

	int nFlds=pdb->NumFlds();

	for(int n=1;n<=nFlds;n++) {
		pcb->AddString(pdb->FldNamPtr(n));
	}
	pcb->SetCurSel(0);
}

void CDlgOptionsGE::UpdateRecControls()
{
	char label[80];
	sprintf(label,"%u of %u",m_uRecIdx+1,m_uRecCount);
	GetDlgItem(IDC_ST_RECORD)->SetWindowText(label);
	m_pShp->GetRecordLabel(label,80,m_vRec[m_uRecIdx]);

	CString desc;
	m_pShp->InitDescGE(desc,m_vRec[m_uRecIdx],m_vFld,FALSE); //Do not skip empty flds (include fld name)
	m_ceDescription.SetWindowText(desc);

	desc.Format("Description for %s",label);
	GetDlgItem(IDC_ST_DESC)->SetWindowText(desc);
}

BOOL CDlgOptionsGE::OnInitDialog()
{
	CDialog::OnInitDialog();
	InitLabelFields();
	//m_ceDescription.SetReadOnly(TRUE); //in resource

	if(m_uRecCount<=1) {
		GetDlgItem(IDC_NEXT)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_PREV)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_REMOVE_REC)->ShowWindow(SW_HIDE);
	}

	UpdateRecControls();

	if(m_vFld.empty()) {
		GetDlgItem(IDC_CLEAR_ALL)->EnableWindow(FALSE);
		GetDlgItem(IDC_CLEAR_LAST)->EnableWindow(FALSE);
	}

	if(!m_bFlyEnabled) GetDlgItem(IDOK)->ShowWindow(SW_HIDE);

	return TRUE;  // return TRUE unless you set the focus to a control
}

void CDlgOptionsGE::OnBnClickedClear()
{
	m_vFld.clear();
	m_ceDescription.SetWindowText("");
	GetDlgItem(IDC_CLEAR_LAST)->EnableWindow(FALSE);
	GetDlgItem(IDC_CLEAR_ALL)->EnableWindow(FALSE);
	m_bFldChg=true;
}

void CDlgOptionsGE::OnBnClickedClrLast()
{
	CString desc;
	m_vFld.pop_back();
	m_pShp->InitDescGE(desc,m_vRec[m_uRecIdx],m_vFld,FALSE); //Do not skip empty flds (include fld name)
	m_ceDescription.SetWindowText(desc);
	int len0=m_ceDescription.GetWindowTextLength();
	m_ceDescription.SetSel(len0,len0,0); //scroll caret into view
	m_bFldChg=true;

	if(m_vFld.empty()) {
		GetDlgItem(IDC_CLEAR_ALL)->EnableWindow(FALSE);
		GetDlgItem(IDC_CLEAR_LAST)->EnableWindow(FALSE);
	}
}
	
void CDlgOptionsGE::OnBnClickedAppend()
{
	CComboBox *pcb=(CComboBox *)GetDlgItem(IDC_DESC_FIELD);
	UINT nFld=pcb->GetCurSel()+1;

	if(std::find(m_vFld.begin(),m_vFld.end(),(BYTE)nFld)!=m_vFld.end()) {
		CMsgBox("Field %s is already part of the description.",m_pShp->m_pdb->FldNamPtr(nFld));
		return;
	}
	m_bFldChg=true;

	CString s;
	m_pShp->GetFieldTextGE(s,m_vRec[m_uRecIdx],nFld,FALSE);

	try {
		int len0=m_ceDescription.GetWindowTextLength();
		m_ceDescription.SetSel(len0,len0,0); //scroll caret into view
		m_ceDescription.ReplaceSel(s);
		m_vFld.push_back((BYTE)nFld);
		GetDlgItem(IDC_CLEAR_LAST)->EnableWindow(TRUE);
		GetDlgItem(IDC_CLEAR_ALL)->EnableWindow(TRUE);
	}
	catch(...) {
		ASSERT(0);
	}
	
	if(++nFld>m_pShp->m_pdb->NumFlds()) nFld=1;
	pcb->SetCurSel(nFld-1);
}

void CDlgOptionsGE::OnBnClickedSave()
{
	m_bFlyEnabled=FALSE;
	CDialog::OnOK();
}

void CDlgOptionsGE::OnBnClickedBrowse()
{
	CString strFilter;
	if(!AddFilter(strFilter,IDS_KML_FILES)) return;
	if(!DoPromptPathName(m_pathName,
		OFN_HIDEREADONLY,
		1,strFilter,FALSE,IDS_KML_EXPORT,".kml")) return;

	m_cePathName.SetWindowText(m_pathName);
}

void CDlgOptionsGE::OnBnClickedNext()
{
	ASSERT(m_uRecCount>1);
	if(++m_uRecIdx>=m_uRecCount) m_uRecIdx=0;
	UpdateRecControls();
}

void CDlgOptionsGE::OnBnClickedPrev()
{
	ASSERT(m_uRecCount>1);
	if(m_uRecIdx--==0) m_uRecIdx=m_uRecCount-1;
	UpdateRecControls();
}

void CDlgOptionsGE::OnBnClickedRemoveRec()
{
	ASSERT(m_uRecCount>1);
	m_vRec.erase(m_vRec.begin()+m_uRecIdx);
	if(m_uRecIdx==--m_uRecCount) m_uRecIdx--;

	if(m_uRecCount<=1) {
		GetDlgItem(IDC_NEXT)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_PREV)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_REMOVE_REC)->ShowWindow(SW_HIDE);
	}
	UpdateRecControls();
}

LRESULT CDlgOptionsGE::OnCommandHelp(WPARAM wNone, LPARAM lParam)
{
	AfxGetApp()->WinHelp(1008);
	return TRUE;
}

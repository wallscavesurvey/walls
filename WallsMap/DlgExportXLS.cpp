// DlgExportXLS.cpp : implementation file
//

#include "stdafx.h"
#include "WallsMap.h"
#include "ShpLayer.h"
#include "ShpDef.h"
#include "DlgExportXLS.h"
//#include "afxdialogex.h"


// CDlgExportXLS dialog

IMPLEMENT_DYNAMIC(CDlgExportXLS, CDialog)

static BOOL bStripRTF=TRUE;
static UINT uMaxChars=32767;

CDlgExportXLS::CDlgExportXLS(CShpLayer *pShp,VEC_DWORD &vRec,CWnd* pParent /*=NULL*/)
	: CDialog(CDlgExportXLS::IDD, pParent)
	, m_pShp(pShp)
	, m_vRec(vRec)
	, m_bStripRTF(bStripRTF)
	, m_uMaxChars(uMaxChars)
{
	char buf[_MAX_PATH];
	strcpy(buf,pShp->PathName());
	char *p=trx_Stpext(buf);
	if(p-buf+4<_MAX_PATH) strcpy(p,".xls");
	m_pathName=buf;
}

CDlgExportXLS::~CDlgExportXLS()
{
}

void CDlgExportXLS::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PROGRESS, m_Progress);
	DDX_Check(pDX, IDC_STRIP_RTF, m_bStripRTF);
	DDX_Text(pDX, IDC_MAX_CHARS, m_uMaxChars);
	DDX_Text(pDX, IDC_PATHNAME, m_pathName);
	DDX_Control(pDX, IDC_PATHNAME, m_cePathName);
	DDV_MinMaxUInt(pDX, m_uMaxChars, 4, 32767);

	if(pDX->m_bSaveAndValidate) {
		CShpDef shpdef;
		shpdef.uFlags=0; //specify strip rtf?
		LPSTR p=NULL;

		m_pathName.Trim();
		if(!m_pathName.IsEmpty()) {
			p=dos_FullPath(m_pathName,"xls");

			if(p && !_stricmp(trx_Stpext(p),".xls")) {
				if(!_access(p,0) &&	IDYES!=CMsgBox(MB_YESNO,"File %s already exists. Do you want to overwrite it?",trx_Stpnam(p))) {
					pDX->m_idLastControl=IDC_PATHNAME;
					pDX->m_bEditLastControl=TRUE;
					pDX->Fail();
					return;
				}
			}
		}

		if(p) {
			SetText(IDC_PATHNAME,p);
			m_pathName=p;
			if(!m_pShp->InitShpDef(shpdef,TRUE,true)) {
				pDX->Fail();
				return;
			}

			m_Progress.ShowWindow(SW_SHOW);

			if(m_pShp->ExportXLS(this,p,shpdef,m_vRec)) {
				uMaxChars=m_uMaxChars;
				bStripRTF=m_bStripRTF;
			}
			else m_uMaxChars=0;
			return;
		}
	}
}


BEGIN_MESSAGE_MAP(CDlgExportXLS, CDialog)
	ON_BN_CLICKED(IDBROWSE, OnBnClickedBrowse)
    ON_MESSAGE(WM_COMMANDHELP,OnCommandHelp)
END_MESSAGE_MAP()

BOOL CDlgExportXLS::OnInitDialog()
{
	CDialog::OnInitDialog();

	CString title;
	title.Format("XLS Export of %s (%u selected record%s)",m_pShp->Title(),m_vRec.size(),
		(m_vRec.size()>1)?"s":"");

	SetWindowText(title);

	return TRUE;  // return TRUE unless you set the focus to a control
}

void CDlgExportXLS::OnBnClickedBrowse()
{
	CString strFilter;
	if(!AddFilter(strFilter,IDS_XLS_FILES)) return;
	if(!DoPromptPathName(m_pathName,
		OFN_HIDEREADONLY,
		1,strFilter,FALSE,IDS_XLS_EXPORT,".xls")) return;
	SetText(IDC_PATHNAME,m_pathName);
}

void CDlgExportXLS::SetStatusMsg(LPCSTR msg)
{
	m_Progress.ShowWindow(SW_HIDE);
	GetDlgItem(IDC_ST_MSG)->ShowWindow(SW_SHOW);
	GetDlgItem(IDC_ST_MSG)->SetWindowText(msg);
}


LRESULT CDlgExportXLS::OnCommandHelp(WPARAM wNone, LPARAM lParam)
{
	AfxGetApp()->WinHelp(1009);
	return TRUE;
}

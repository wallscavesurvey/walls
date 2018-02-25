// ImpSefDlg.cpp : implementation file
////

#include "stdafx.h"
#include "walls.h"
#include "prjdoc.h"
#include "lineview.h"
#include "ImpSefDlg.h"
#include "wallsef\wallisef.h"

#define IMP_DLL_IMPORT "_Import@12"
#define IMP_DLL_ERRMSG "_ErrMsg@8"

typedef int (FAR PASCAL *LPFN_IMPORT_CB)(int fcn, LPVOID param);
typedef int (FAR PASCAL *LPFN_IMPORT)(LPSTR sefPath, LPSTR prjPath, LPFN_IMPORT_CB cb);
typedef int (FAR PASCAL *LPFN_ERRMSG)(LPCSTR *pMsg,int code);

enum { SEF_ALLOWCOLONS=1, SEF_COMBINE=2,SEF_FLG_CHRS_PFX=4, SEF_FLG_CHRS_REPL=8, SEF_ATTACHSRC=16};

enum { SEF_GETPARAMS, SEF_LINECOUNT };

static SEF_PARAMS sef_params={ DLL_VERSION,0,{'#', ':', ',', ';', ' ', 0 },{ '~', '|', '`', '^', '@', 0 } };

static LPCSTR pDflt_chrs_repl="~|`^@";
#define LEN_CHRS_REPL 5

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

static int PASCAL import_CB(int fcn, LPVOID param)
{
	if(fcn==SEF_GETPARAMS) {
		*(PSEF_PARAMS *)param = &sef_params;
	}
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CImpSefDlg dialog

CImpSefDlg::CImpSefDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CImpSefDlg::IDD, pParent)
{
	m_PrjPath = "";
	m_SefPath = "";
	bImporting = FALSE;
	m_bAttachSrc = (sef_params.flags&SEF_ATTACHSRC)!=0;
	m_bCombine = (sef_params.flags&SEF_COMBINE)!=0;
	m_bAllowColons=(sef_params.flags&SEF_ALLOWCOLONS)!=0;
	m_csChrs_repl=sef_params.chrs_repl;
}

BOOL CImpSefDlg::Check_Chrs_repl()
{
	if(m_csChrs_repl.GetLength()!=LEN_CHRS_REPL) {
		AfxMessageBox("Four replacement characters must be specified.");
		return FALSE;
	}
	for(LPCSTR p=(LPCSTR)m_csChrs_repl; *p; p++) {
		if(strchr(p+1, *p)) {
			AfxMessageBox("The replacement characters must be distinct.");
			return FALSE;
		}
		if(strchr(sef_params.chrs_torepl, *p)) {
			CMsgBox(MB_ICONINFORMATION, "Character '%c' can't be used as a replacement.");
			return FALSE;
		}
     }
	 return TRUE;
}

void CImpSefDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_IMPPRJPATH, m_PrjPath);
	DDV_MaxChars(pDX, m_PrjPath, 250);
	DDX_Text(pDX, IDC_IMPDATPATH, m_SefPath);
	DDV_MaxChars(pDX, m_SefPath, 250);
	DDX_Check(pDX, IDC_IMPCOMBINE, m_bCombine);
	DDX_Check(pDX, IDC_ATTACH_SRC, m_bAttachSrc);
	DDX_Check(pDX, IDC_ALLOWCOLONS, m_bAllowColons);
	DDX_Text(pDX, IDC_CHRS_REPL, m_csChrs_repl);
	DDV_MaxChars(pDX, m_csChrs_repl, LEN_CHRS_REPL);

	if(pDX->m_bSaveAndValidate) {
		UINT e;
		if(!Check_Chrs_repl()) {
			e=IDC_CHRS_REPL;
			goto _fail;
		}
		m_SefPath.Trim();
		m_PrjPath.Trim();
		if((e=m_SefPath.IsEmpty()) || m_PrjPath.IsEmpty()) {
			CMsgBox(MB_ICONEXCLAMATION, IDS_ERR_IMPNAME1, e?"SEF":"WPJ");
			e=e?IDC_IMPDATPATH:IDC_IMPPRJPATH;
			goto _fail;
		}
		char buf[_MAX_PATH];
		trx_Stcext(buf, m_SefPath, "sef", _MAX_PATH);
		if(_access(buf, 4)) {
			CMsgBox(MB_OKCANCEL, IDS_ERR_IMPSEFPATH1, buf);
			e=IDC_IMPDATPATH;
			goto _fail;
		}
		_fullpath(buf, m_PrjPath, _MAX_PATH);
		strcpy(trx_Stpext(buf), PRJ_PROJECT_EXTW);
		m_PrjPath=buf;
		if(!_access(buf, 0)) {
			if(theApp.IsProjectOpen(buf) || IDCANCEL==CMsgBox(MB_OKCANCEL, IDS_ERR_IMPPRJPATH1, buf)) {
				e=IDC_IMPPRJPATH;
				goto _fail;
			}
		}
		else if(!DirCheck(buf, TRUE)) {
			//Declined to create new folder --
			e=IDC_IMPPRJPATH;
			goto _fail;
		}

		//Load DLL --
		//UINT prevMode=SetErrorMode(SEM_NOOPENFILEERRORBOX);
		HINSTANCE hinst=LoadLibrary(IMP_DLL_NAME); //EXP_DLL_NAME
		LPFN_IMPORT pfnImport=NULL;
		LPFN_ERRMSG pfnErrMsg=NULL;

		if(hinst) {
			pfnImport=(LPFN_IMPORT)GetProcAddress(hinst, IMP_DLL_IMPORT);
			if(pfnImport) pfnErrMsg=(LPFN_ERRMSG)GetProcAddress(hinst, IMP_DLL_ERRMSG);
		}

		if(pfnErrMsg) {
			bImporting=TRUE;

			sef_params.flags=m_bAllowColons*SEF_ALLOWCOLONS+m_bCombine*SEF_COMBINE+m_bAttachSrc*SEF_ATTACHSRC;
			strcpy(sef_params.chrs_repl, m_csChrs_repl);
			e=pfnImport((LPSTR)(LPCSTR)m_SefPath, buf, (LPFN_IMPORT_CB)import_CB);
			bImporting=FALSE;

			LPCSTR pMsg=NULL;
			CLineView::m_nLineStart=pfnErrMsg(&pMsg, e);
			if(pMsg) AfxMessageBox(pMsg);
			if(e) m_PrjPath.Empty();
			FreeLibrary(hinst);
		}
		else {
			if(hinst) FreeLibrary(hinst);
			CMsgBox(MB_ICONEXCLAMATION, IDS_ERR_IMPLOADDLL1, IMP_DLL_NAME);
			EndDialog(IDCANCEL);
			pDX->Fail();
		}
		return;

	_fail:
		if(e) {
			pDX->m_idLastControl=e;
			pDX->m_bEditLastControl=TRUE;
			pDX->Fail();
		}
	}
}

BEGIN_MESSAGE_MAP(CImpSefDlg, CDialog)
 	ON_BN_CLICKED(IDC_SEFBROWSE, OnSefBrowse)
	ON_BN_CLICKED(IDC_PRJBROWSE, OnPrjbrowse)
	ON_BN_CLICKED(IDC_REPL_DFLT, OnBnClickedReplDflt)
	ON_EN_CHANGE(IDC_CHRS_REPL, OnChangeChrsRepl)
	ON_MESSAGE(WM_COMMANDHELP, OnCommandHelp)
END_MESSAGE_MAP()


BOOL CImpSefDlg::OnInitDialog()
{ 
	CDialog::OnInitDialog(); 
	
	GetDlgItem(IDC_REPL_DFLT)->EnableWindow(strcmp(m_csChrs_repl, pDflt_chrs_repl)!=0);
	GetDlgItem(IDC_IMPDATPATH)->SetFocus();
	
	return FALSE;  //We set the focus to a control
}

/////////////////////////////////////////////////////////////////////////////
// CImpSefDlg message handlers

void CImpSefDlg::OnSefBrowse()
{
	((CMainFrame *)AfxGetMainWnd())->BrowseFileInput(this, m_SefPath, m_PrjPath, IDS_SEF_FILES);
}

void CImpSefDlg::OnPrjbrowse()
{
	((CMainFrame *)AfxGetMainWnd())->BrowseNewProject(this,m_PrjPath);
}


void CImpSefDlg::OnCancel()
{
    if(!bImporting) CDialog::OnCancel();
}

void CImpSefDlg::OnBnClickedReplDflt()
{
	GetDlgItem(IDC_CHRS_REPL)->SetWindowText(m_csChrs_repl=pDflt_chrs_repl);
	GetDlgItem(IDC_CHRS_REPL)->SetFocus();
}

void CImpSefDlg::OnChangeChrsRepl()
{
	GetDlgItem(IDC_CHRS_REPL)->GetWindowText(m_csChrs_repl);
	BOOL bEnable=strcmp(m_csChrs_repl, pDflt_chrs_repl)!=0;
	if(bEnable!=(GetDlgItem(IDC_REPL_DFLT)->IsWindowEnabled()!=0))
		GetDlgItem(IDC_REPL_DFLT)->EnableWindow(bEnable);
}

LRESULT CImpSefDlg::OnCommandHelp(WPARAM wNone, LPARAM lParam)
{
	AfxGetApp()->WinHelp(12, HELP_CONTEXT);
	return TRUE;
}

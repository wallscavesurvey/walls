// impdlg.cpp : implementation file
////

#include "stdafx.h"
#include "walls.h"
#include "prjdoc.h"
#include "lineview.h"
#include "ImpCssDlg.h"

#define DLL_VERSION 2.0f
#define IMP_DLL_NAME "WALLCSS.DLL"
#define IMP_DLL_IMPORT "_Import@12"
#define IMP_DLL_ERRMSG "_ErrMsg@12"

typedef int (FAR PASCAL *LPFN_IMPORT_CB)(int fcn, LPVOID param);
typedef int (FAR PASCAL *LPFN_IMPORT)(LPSTR cssPath, LPSTR wpjPath, LPFN_IMPORT_CB cb);
typedef int (FAR PASCAL *LPFN_ERRMSG)(LPCSTR *pPath, LPCSTR *pMsg, int code);

struct CSS_PARAMS
{
	float version;
	UINT flags;
	char chrs_torepl[6];
	char chrs_repl[6];
};

typedef CSS_PARAMS *PCSS_PARAMS;

enum { CSS_ALLOWCOLONS = 1, CSS_COMBINE = 2, CSS_FLG_CHRS_PFX = 4, CSS_FLG_CHRS_REPL = 8, CSS_ATTACHSRC = 16 };

enum { CSS_GETPARAMS };

static CSS_PARAMS css_params = { DLL_VERSION,0,{ '#', ':', ',', ';', 0 },{ '~', '|', '`', '^', 0 } };

static LPCSTR pDflt_chrs_repl = "~|`^";
#define LEN_CHRS_REPL 4

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

static int PASCAL import_CB(int fcn, LPVOID param)
{
	if (fcn == CSS_GETPARAMS) {
		*(PCSS_PARAMS *)param = &css_params;
	}
	return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// CImpCssDlg dialog

CImpCssDlg::CImpCssDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CImpCssDlg::IDD, pParent)
{
	m_WpjPath = "";
	m_CssPath = "";
	m_bCombine = (css_params.flags&CSS_COMBINE) != 0;
	m_bAttachSrc = (css_params.flags&CSS_ATTACHSRC) != 0;
	m_bAllowColons = (css_params.flags&CSS_ALLOWCOLONS) != 0;
	m_csChrs_repl = css_params.chrs_repl;
	m_bImporting = FALSE;
}

BOOL CImpCssDlg::Check_Chrs_repl()
{
	if (m_csChrs_repl.GetLength() != LEN_CHRS_REPL) {
		AfxMessageBox("Four replacement characters must be specified.");
		return FALSE;
	}
	for (LPCSTR p = (LPCSTR)m_csChrs_repl; *p; p++) {
		if (strchr(p + 1, *p)) {
			AfxMessageBox("The replacement characters must be distinct.");
			return FALSE;
		}
		if (strchr(css_params.chrs_torepl, *p)) {
			CMsgBox(MB_ICONINFORMATION, "Character '%c' can't be used as a replacement.");
			return FALSE;
		}
	}
	return TRUE;
}

void CImpCssDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CImpCssDlg)
	DDX_Text(pDX, IDC_IMPPRJPATH, m_WpjPath);
	DDV_MaxChars(pDX, m_WpjPath, 250);
	DDX_Text(pDX, IDC_IMPDATPATH, m_CssPath);
	DDV_MaxChars(pDX, m_CssPath, 250);
	DDX_Check(pDX, IDC_ALLOWCOLONS, m_bAllowColons);
	DDX_Text(pDX, IDC_CHRS_REPL, m_csChrs_repl);
	DDV_MaxChars(pDX, m_csChrs_repl, LEN_CHRS_REPL);
	DDX_Check(pDX, IDC_IMPCOMBINE, m_bCombine);
	DDX_Check(pDX, IDC_ATTACH_SRC, m_bAttachSrc);

	if (pDX->m_bSaveAndValidate) {
		UINT e;
		if (!Check_Chrs_repl()) {
			e = IDC_CHRS_REPL;
			goto _fail;
		}
		m_CssPath.Trim();
		m_WpjPath.Trim();
		if ((e = m_CssPath.IsEmpty()) || m_WpjPath.IsEmpty()) {
			CMsgBox(MB_ICONEXCLAMATION, IDS_ERR_IMPNAME1, e ? "MAK or DAT" : "WPJ");
			e = e ? IDC_IMPDATPATH : IDC_IMPPRJPATH;
			goto _fail;
		}
		else {
			char buf[_MAX_PATH];
			_strupr(strcpy(buf, trx_Stpext(m_CssPath)));
			if (strcmp(buf, ".DAT") && strcmp(buf, ".MAK")) {
				AfxMessageBox("Input file extension must be .mak or .dat.");
				e = IDC_IMPDATPATH;
				goto _fail;
			}
			trx_Stncc(buf, m_CssPath, _MAX_PATH);
			if (e = _access(buf, 4)) {
				LPCSTR p = trx_Stpnam(buf);
				if (!strchr(p, '*') && !strchr(p, '?')) {
					CMsgBox(MB_OKCANCEL, IDS_ERR_IMPPATH1, buf);
					e = IDC_IMPDATPATH;
					goto _fail;
				}
			}
			if (!_fullpath(buf, m_WpjPath, _MAX_PATH)) {
				AfxMessageBox("Output folder name or location invalid.");
				e = IDC_IMPPRJPATH;
				goto _fail;
			}

			strcpy(trx_Stpext(buf), PRJ_PROJECT_EXTW);
			m_WpjPath = buf;
			GetDlgItem(IDC_IMPPRJPATH)->SetWindowText(buf);


			if (!_access(buf, 0)) {
				//overwrite existing wpj?
				if (theApp.IsProjectOpen(buf) || IDCANCEL == CMsgBox(MB_OKCANCEL, IDS_ERR_WPJPATH1, buf)) {
					e = IDC_IMPPRJPATH;
					goto _fail;
				}
			}
			else {
				//Let's see if the directory needs to be created --
				if (!DirCheck(buf, TRUE)) {
					e = IDC_IMPPRJPATH;
					goto _fail;
				}
			}

			//Load DLL --
			//UINT prevMode=SetErrorMode(SEM_NOOPENFILEERRORBOX);

			LPFN_IMPORT pfnImport = NULL;
			LPFN_ERRMSG pfnErrMsg = NULL;
			HINSTANCE hinst = LoadLibrary(IMP_DLL_NAME); //Wallcss.dll

			if (hinst) {
				if (pfnErrMsg = (LPFN_ERRMSG)GetProcAddress(hinst, IMP_DLL_ERRMSG))
					pfnImport = (LPFN_IMPORT)GetProcAddress(hinst, IMP_DLL_IMPORT);
			}

			if (pfnImport) {
				m_bImporting = TRUE;

				css_params.flags = m_bAllowColons * CSS_ALLOWCOLONS + m_bCombine * CSS_COMBINE + m_bAttachSrc * CSS_ATTACHSRC;
				strcpy(css_params.chrs_repl, m_csChrs_repl);

				e = pfnImport((LPSTR)(LPCSTR)m_CssPath, buf, (LPFN_IMPORT_CB)import_CB);
				m_bImporting = FALSE;
				LPCSTR pPath = NULL, pMsg = NULL;
				CLineView::m_nLineStart = pfnErrMsg(&pPath, &pMsg, e);
				if (pMsg) AfxMessageBox(pMsg);
				if (pPath) m_CssPath = pPath;
			}
			else {
				CMsgBox(MB_ICONEXCLAMATION, IDS_ERR_IMPLOADDLL1, IMP_DLL_NAME);
				e = -1;
			}
			if (hinst) FreeLibrary(hinst);
		}
		if (e) {
			//m_WpjPath.Empty();
			EndDialog(IDCANCEL);
			pDX->Fail();
		}
		return;
	_fail:
		pDX->m_idLastControl = e;
		pDX->m_bEditLastControl = TRUE;
		pDX->Fail();
	}
}

BEGIN_MESSAGE_MAP(CImpCssDlg, CDialog)
	ON_BN_CLICKED(IDC_CSSBROWSE, OnCssBrowse)
	ON_BN_CLICKED(IDC_WPJBROWSE, OnWpjBrowse)
	ON_BN_CLICKED(IDC_REPL_DFLT, OnBnClickedReplDflt)
	ON_EN_CHANGE(IDC_CHRS_REPL, OnChangeChrsRepl)
	ON_MESSAGE(WM_COMMANDHELP, OnCommandHelp)
END_MESSAGE_MAP()


BOOL CImpCssDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	CenterWindow();
	GetDlgItem(IDC_IMPDATPATH)->SetFocus();

	return FALSE;  //We set the focus to a control
}

/////////////////////////////////////////////////////////////////////////////
// CImpCssDlg message handlers
void CImpCssDlg::OnBnClickedReplDflt()
{
	GetDlgItem(IDC_CHRS_REPL)->SetWindowText(m_csChrs_repl = pDflt_chrs_repl);
	GetDlgItem(IDC_CHRS_REPL)->SetFocus();
}

void CImpCssDlg::OnChangeChrsRepl()
{
	GetDlgItem(IDC_CHRS_REPL)->GetWindowText(m_csChrs_repl);
	BOOL bEnable = strcmp(m_csChrs_repl, pDflt_chrs_repl) != 0;
	if (bEnable != (GetDlgItem(IDC_REPL_DFLT)->IsWindowEnabled() != 0))
		GetDlgItem(IDC_REPL_DFLT)->EnableWindow(bEnable);
}

void CImpCssDlg::OnCssBrowse()
{
	((CMainFrame *)AfxGetMainWnd())->BrowseFileInput(this, m_CssPath, m_WpjPath, IDS_CSS_FILES);
}

void CImpCssDlg::OnWpjBrowse()
{
	((CMainFrame *)AfxGetMainWnd())->BrowseNewProject(this, m_WpjPath);
}

void CImpCssDlg::OnCancel()
{
	if (!m_bImporting) EndDialog(IDCANCEL);
}

LRESULT CImpCssDlg::OnCommandHelp(WPARAM wNone, LPARAM lParam)
{
	AfxGetApp()->WinHelp(330, HELP_CONTEXT);
	return TRUE;
}

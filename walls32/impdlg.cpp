// impdlg.cpp : implementation file
////

#include "stdafx.h"
#include "walls.h"
#include "prjdoc.h"
#include "lineview.h"
#include "impdlg.h"

#define IMP_DLL_NAME "WALLSEF.DLL"
#define IMP_DLL_IMPORT "_Import@12"
#define IMP_DLL_ERRMSG "_ErrMsg@8"

typedef int (FAR PASCAL *LPFN_IMPORT_CB)(int lines);
typedef int (FAR PASCAL *LPFN_IMPORT)(LPSTR sefPath,LPSTR prjPath,LPFN_IMPORT_CB cb);
typedef int (FAR PASCAL *LPFN_ERRMSG)(LPSTR buffer,int code);

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

static CImpDlg *pDLG;

static int PASCAL import_CB(int lines)
{
    if(pDLG) {
      char buf[8];
      sprintf(buf,"%u",lines);
 	  pDLG->GetDlgItem(IDC_IMPLINECOUNT)->SetWindowText(buf);
	  Pause();
	}
	return pDLG!=NULL;
}

/////////////////////////////////////////////////////////////////////////////
// CImpDlg dialog

CImpDlg::CImpDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CImpDlg::IDD, pParent)
{
    pDLG=NULL;
    bImporting=FALSE;
	//{{AFX_DATA_INIT(CImpDlg)
	m_PrjPath = "";
	m_SefPath = "";
	m_bCombine = FALSE;
	//}}AFX_DATA_INIT
}

void CImpDlg::ShowControl(UINT idc,BOOL bShow)
{
	GetDlgItem(idc)->ShowWindow(bShow?SW_NORMAL:SW_HIDE);
	GetDlgItem(idc)->EnableWindow(bShow);
}

void CImpDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CImpDlg)
	DDX_Text(pDX, IDC_IMPPRJPATH, m_PrjPath);
	DDV_MaxChars(pDX, m_PrjPath, 250);
	DDX_Text(pDX, IDC_IMPSEFPATH, m_SefPath);
	DDV_MaxChars(pDX, m_SefPath, 250);
	DDX_Check(pDX, IDC_IMPCOMBINE, m_bCombine);
	//}}AFX_DATA_MAP
	
	if(pDX->m_bSaveAndValidate) {
		UINT e;
	    if((e=m_SefPath.IsEmpty()) || m_PrjPath.IsEmpty()) {
	      CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_IMPNAME1,e?"SEF":"WPJ");
	      e=1;
	    }
	    else {
	    	char buf[_MAX_PATH];
	    	trx_Stcext(buf,m_SefPath,"SEF",_MAX_PATH);
	    	_strupr(buf);
	    	if(e=_access(buf,4)) {
	    	  CMsgBox(MB_OKCANCEL,IDS_ERR_IMPSEFPATH1,buf);
	    	}
	    	else {
	    	  _fullpath(buf,m_PrjPath,_MAX_PATH); m_PrjPath=buf;
	    	  strcpy(trx_Stpext(_strupr(buf)),PRJ_PROJECT_EXTW);
	    	  m_PrjPath=buf;
	    	  if(!_access(buf,0)) {
	    	    if(IDCANCEL==CMsgBox(MB_OKCANCEL,IDS_ERR_IMPPRJPATH1,buf)) e=1;
	    	  }
	    	  else {
	    	    //Let's see if the directory needs to be created --
	    	    if(!DirCheck(buf,TRUE)) e=1;
	    	  }
	    	  
	    	  if(!e) {
	    	    
	    	    //Load DLL --
	    	    //UINT prevMode=SetErrorMode(SEM_NOOPENFILEERRORBOX);
	    	    HINSTANCE hinst=LoadLibrary(IMP_DLL_NAME); //EXP_DLL_NAME
	    	    LPFN_IMPORT pfnImport=NULL;
	    	    
	    	    if(hinst)
	    	      pfnImport=(LPFN_IMPORT)GetProcAddress(hinst,IMP_DLL_IMPORT);
	    	    
	    	    if(pfnImport) {
	    	      if(m_bCombine) *buf|=0x80;
	    	      ShowControl(IDC_IMPBROWSE,FALSE);
	    	      ShowControl(IDC_IMPORTING,TRUE);
	    	      ShowControl(IDOK,FALSE);
	    	      ShowControl(IDC_IMPLINECOUNT,TRUE);
	    	      bImporting=TRUE;
	    	      pDLG=this;
	    	      e=pfnImport((LPSTR)(LPCSTR)m_SefPath,buf,(LPFN_IMPORT_CB)import_CB);
	    	      bImporting=FALSE;
	    	      if(e>1) {  //e==1 implies user abort
	    	        LPFN_ERRMSG pfnErrMsg=(LPFN_ERRMSG)GetProcAddress(hinst,IMP_DLL_ERRMSG);
	    	        if(pfnErrMsg) {
	    	          CLineView::m_nLineStart=pfnErrMsg(buf,e);
	    	          CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_IMPERRMSG1,buf);
	    	        }
	    	      }
	    	    }
	    	    else {
	    	      CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_IMPLOADDLL1,IMP_DLL_NAME);
	    	      e=1;
	    	    }
	    	    if(hinst) FreeLibrary(hinst);
	    	    //SetErrorMode(prevMode);
	    	    if(e) {
	    	      m_PrjPath.Empty();
	    	      return;
	    	    }
	    	  }
	    	}
	    }
	    if(e) {
			pDX->m_bEditLastControl=TRUE;
			pDX->Fail();
		}
	}
}

BEGIN_MESSAGE_MAP(CImpDlg, CDialog)
    ON_MESSAGE(WM_COMMANDHELP,OnCommandHelp)
	//{{AFX_MSG_MAP(CImpDlg)
	ON_BN_CLICKED(IDC_IMPBROWSE, OnImpbrowse)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


BOOL CImpDlg::OnInitDialog()
{ 
	CDialog::OnInitDialog(); 
	CenterWindow();
	GetDlgItem(IDC_IMPSEFPATH)->SetFocus();
	
	return FALSE;  //We set the focus to a control
}

/////////////////////////////////////////////////////////////////////////////
// CImpDlg message handlers

void CImpDlg::OnImpbrowse()
{
    
	CString strFilter;
	if(!AddFilter(strFilter,IDS_SEF_FILES)) return;
	if(!DoPromptPathName(m_SefPath,
	  OFN_HIDEREADONLY|OFN_FILEMUSTEXIST,
	  1,strFilter,TRUE,
	  IDS_IMPBROWSE,
	  ".SEF")) return;
	  
	GetDlgItem(IDC_IMPSEFPATH)->SetWindowText(m_SefPath);
	
	CEdit *pw=(CEdit *)GetDlgItem(IDC_IMPPRJPATH);
	
	if(!pw->GetWindowTextLength()) {
        char *p=strFilter.GetBuffer(m_SefPath.GetLength()+4);
        strcpy(p,m_SefPath);
        strcpy(trx_Stpext(p),PRJ_PROJECT_EXTW);
		pw->SetWindowText(p);
		strFilter.ReleaseBuffer();
	}
	pw->SetSel(0,-1);
	pw->SetFocus();
}


void CImpDlg::OnCancel()
{
    if(pDLG) {
      pDLG=NULL;
    }
    else if(!bImporting) CDialog::OnCancel();
}

LRESULT CImpDlg::OnCommandHelp(WPARAM wNone, LPARAM lParam)
{
	AfxGetApp()->WinHelp(12,HELP_CONTEXT);
	return TRUE;
}

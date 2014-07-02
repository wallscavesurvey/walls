// copyfile.cpp : implementation file
//

#include "stdafx.h"
#include "walls.h"
#include "copyfile.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

BOOL bMoveFile=FALSE;
BOOL bCopiedFile=FALSE;

/////////////////////////////////////////////////////////////////////////////
// CCopyFileDlg dialog


CCopyFileDlg::CCopyFileDlg(LPSTR pTitle,CWnd* pParent /*=NULL*/)
	: CDialog(IDD_COPYFILE,pParent)
{
	m_pTitle=pTitle;
}

BEGIN_MESSAGE_MAP(CCopyFileDlg, CDialog)
	//{{AFX_MSG_MAP(CCopyFileDlg)
	ON_COMMAND(ID_SKIP, OnSkip)
	ON_COMMAND(ID_REPLACEALL, OnReplaceAll)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CCopyFileDlg message handlers

BOOL CCopyFileDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	
	//CenterWindow();
	GetDlgItem(IDC_STATICBOX)->SetWindowText(m_pTitle);
	GetDlgItem(IDC_DATESRC)->SetWindowText(m_szDateSrc);
	GetDlgItem(IDC_DATETGT)->SetWindowText(m_szDateTgt);
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CCopyFileDlg::OnSkip() 
{
	EndDialog(IDNO);
}

void CCopyFileDlg::OnReplaceAll() 
{
	EndDialog(IDYES);
}

int MyCopyFile(const char *source,char *target,int bOverwrite)
{
    /*If bOverwrite is --
      0 - don't overwrite if conflict, no prompt.
      1 - prompt for overwrite if conflict
      2 - always overwrite if conflict, no prompt
      
    Return -1 if disk I/O error, else suggest bOverwrite value on call*/

	DWORD dwErr=0;
	LPVOID lpMsgBuf;
    
	bCopiedFile=FALSE;
    
    if(bOverwrite<0) return -1;
	if(!_stricmp(source,target)) return bOverwrite;

	if(!DirCheck(target,FALSE)) {
		bOverwrite=-1;
		lpMsgBuf=(LPVOID)"Can't create directory";
		goto _fail;
	}

	if(!CopyFile(source,target,bOverwrite<2)) {
		dwErr=GetLastError();
		if(dwErr==ERROR_ALREADY_EXISTS || dwErr==ERROR_FILE_EXISTS) {
			dwErr=0;
			CCopyFileDlg dlg((LPSTR)target);
			long size;
			dlg.m_szDateSrc=GetTimeStr(GetLocalFileTime(source,&size),&size);
			dlg.m_szDateTgt=GetTimeStr(GetLocalFileTime(target,&size),&size);
			if(bOverwrite || dlg.m_szDateSrc.Compare(dlg.m_szDateTgt)) {
			  bOverwrite=dlg.DoModal();
			  //"Skip"          = IDNO
			  //"Replace all"   = IDYES
			  //"Replace"       = IDOK
			  //"Skip all Dups" = IDCANCEL
			  if(bOverwrite==IDCANCEL) bOverwrite=0; /*Skip all future duplicates*/
			  else if(bOverwrite==IDNO) bOverwrite=1;
			  else {
				if(!CopyFile(source,target,FALSE)) {
					bOverwrite=-1;
					dwErr=GetLastError();
				}
				else {
					bCopiedFile=TRUE;
					bOverwrite=(bOverwrite==IDOK)?1:2; /*Yes/Yes to all*/
				}
			  }
			}
		}
		else {
			if(dwErr!=ERROR_FILE_NOT_FOUND && dwErr!=ERROR_PATH_NOT_FOUND) bOverwrite=-1;
		}
	}
	else bCopiedFile=TRUE;

	if(bCopiedFile && bMoveFile) _unlink(source);

_fail:    
    if(bOverwrite<0) {
		if(dwErr) 
		    FormatMessage( 
				FORMAT_MESSAGE_ALLOCATE_BUFFER | 
				FORMAT_MESSAGE_FROM_SYSTEM | 
				FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL,
				dwErr,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
				(LPTSTR) &lpMsgBuf,
				0,
				NULL 
			);
        //Inform user that some kind of copy failure occurred --
        CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_PRJCOPYFILE1,target,lpMsgBuf);
	    if(dwErr) LocalFree(lpMsgBuf);
    }
    return bOverwrite;
}


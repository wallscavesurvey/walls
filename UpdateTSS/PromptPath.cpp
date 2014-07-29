
#include "stdafx.h"
#include <direct.h>
#include <trx_str.h>
#include "PromptPath.h"

BOOL AddFilter(CString &filter,int nIDS)
{
    //Fix this later to handle memory exceptions --
	CString newFilter;
	try {
		VERIFY(newFilter.LoadString(nIDS));
		filter += newFilter;
		filter += (char)'\0';
		
		char *pext=strchr(newFilter.GetBuffer(0),'(');
		ASSERT(pext!=NULL);
		filter += ++pext;
	    
		//filter.SetAt(filter.GetLength()-1,'\0') causes assertion failure!
		//Then why doesn't filter+='\0' fail also?
	    
		filter.GetBufferSetLength(filter.GetLength()-1); //delete trailing ')'
		filter += (char)'\0';
	}
	catch(...) {
		return FALSE;
	}
    return TRUE;
}    

BOOL DoPromptPathName(CString& pathName,DWORD lFlags,
   int numFilters,CString &strFilter,BOOL bOpen,UINT ids_Title,LPCSTR defExt)
{
	CFileDialog dlgFile(bOpen);  //TRUE for open rather than save

	CString title;
	VERIFY(title.LoadString(ids_Title));
	
	char namebuf[_MAX_PATH];
	char pathbuf[_MAX_PATH];

	strcpy(namebuf,trx_Stpnam(pathName));
	strcpy(pathbuf,pathName);
	*trx_Stpnam(pathbuf)=0;
	if(*pathbuf) {
		size_t len=strlen(pathbuf);
		ASSERT(pathbuf[len-1]=='\\' || pathbuf[len-1]=='/');
		pathbuf[len-1]=0;
		//lFlags|=OFN_NOCHANGEDIR;
	}
	else {
	  _getcwd(pathbuf,_MAX_PATH);
	}
	
	dlgFile.m_ofn.lpstrInitialDir=pathbuf; //no trailing slash

	dlgFile.m_ofn.Flags |= (lFlags|OFN_ENABLESIZING);
	if(!(lFlags&OFN_OVERWRITEPROMPT)) dlgFile.m_ofn.Flags&=~OFN_OVERWRITEPROMPT;

	dlgFile.m_ofn.nMaxCustFilter+=numFilters;
	dlgFile.m_ofn.lpstrFilter = strFilter;
	dlgFile.m_ofn.hwndOwner = AfxGetApp()->m_pMainWnd->GetSafeHwnd();
	dlgFile.m_ofn.lpstrTitle = title;
	dlgFile.m_ofn.lpstrFile = namebuf;
	dlgFile.m_ofn.nMaxFile = _MAX_PATH;
	if(defExt) dlgFile.m_ofn.lpstrDefExt = defExt+1;

	if(dlgFile.DoModal() == IDOK) {
		pathName=namebuf;
		return TRUE;
	}
	//DWORD err=CommDlgExtendedError();
	return FALSE;
}

static void FixPathname(CString &path,LPCSTR ext)
{
	if(!*trx_Stpext(path)) {
		char *p=trx_Stpext(path.GetBuffer(path.GetLength()+5));
		*p++='.';
		strcpy(p,ext);
	    path.ReleaseBuffer();
	}
}

BOOL BrowseFiles(CEdit *pw,CString &path,LPCSTR ext,UINT idType,UINT idBrowse,UINT flags) 
{
	CString strFilter;
	CString Path;

	//int numFilters=(idType==IDS_PRJ_FILES)?1:2;
	
	pw->GetWindowText(Path);
	
	if(!AddFilter(strFilter,idType) /*|| numFilters==2 &&
	   !AddFilter(strFilter,AFX_IDS_ALLFILTER)*/) return FALSE;
	   
	if(!DoPromptPathName(Path,flags,1/*numFilters*/,strFilter,
		(flags&(OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT))==0,
		idBrowse,ext)) return FALSE;
	
	path=Path;
	FixPathname(path,ext+1);
	pw->SetWindowText(path);
	pw->SetSel(0,-1);
	pw->SetFocus();
	return TRUE;
}

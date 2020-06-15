
#include "stdafx.h"
#include <direct.h>
#include <trx_str.h>

BOOL AddFilter(CString &filter, int nIDS)
{
	//Fix this later to handle memory exceptions --
	CString newFilter;

	VERIFY(newFilter.LoadString(nIDS));
	filter += newFilter;
	filter += (char)'\0';

	char *pext = strchr(newFilter.GetBuffer(0), '(');
	ASSERT(pext != NULL);
	filter += ++pext;

	//filter.SetAt(filter.GetLength()-1,'\0') causes assertion failure!
	//Then why doesn't filter+='\0' fail also?

	filter.GetBufferSetLength(filter.GetLength() - 1); //delete trailing ')'
	filter += (char)'\0';
	return TRUE;
}

BOOL DoPromptPathName(CString& pathName, DWORD lFlags, int numFilters, CString &strFilter, BOOL bOpen, UINT ids_Title, LPCSTR defExt)
{
	CFileDialog dlgFile(bOpen);  //TRUE for open rather than save

	CString title;
	VERIFY(title.LoadString(ids_Title));

	char namebuf[_MAX_PATH];
	char pathbuf[_MAX_PATH];

	strcpy(namebuf, trx_Stpnam(pathName));
	strcpy(pathbuf, pathName);
	*trx_Stpnam(pathbuf) = 0;
	if (*pathbuf) {
		size_t len = strlen(pathbuf);
		ASSERT(pathbuf[len - 1] == '\\' || pathbuf[len - 1] == '/');
		pathbuf[len - 1] = 0;
		//lFlags|=OFN_NOCHANGEDIR;
	}
	/*
	else {
	  _getcwd(pathbuf,_MAX_PATH);
	}
	*/

	dlgFile.m_ofn.lpstrInitialDir = pathbuf; //no trailing slash

	dlgFile.m_ofn.Flags |= (lFlags | OFN_ENABLESIZING);
	if (!(lFlags&OFN_OVERWRITEPROMPT)) dlgFile.m_ofn.Flags &= ~OFN_OVERWRITEPROMPT;

	dlgFile.m_ofn.nMaxCustFilter += numFilters;
	dlgFile.m_ofn.lpstrFilter = strFilter;
	dlgFile.m_ofn.hwndOwner = AfxGetApp()->m_pMainWnd->GetSafeHwnd();
	dlgFile.m_ofn.lpstrTitle = title;
	dlgFile.m_ofn.lpstrFile = namebuf;
	dlgFile.m_ofn.nMaxFile = _MAX_PATH;
	if (defExt) dlgFile.m_ofn.lpstrDefExt = defExt + 1;

	if (dlgFile.DoModal() == IDOK) {
		pathName = namebuf;
		return TRUE;
	}
	//DWORD err=CommDlgExtendedError();
	return FALSE;
}


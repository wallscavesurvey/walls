
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

BOOL DoPromptPathName(CString& pathName, DWORD lFlags,
	int numFilters, CString &strFilter, BOOL bOpen, UINT ids_Title, char *defExt)
{
	CFileDialog dlgFile(bOpen);  //TRUE for open rather than save

	CString title;
	VERIFY(title.LoadString(ids_Title));

	char namebuf[_MAX_PATH];

	size_t len = strlen(strcpy(namebuf, trx_Stpnam(pathName)));
	char *path = namebuf + len + 1;
	memcpy(path, (LPCSTR)pathName, len = pathName.GetLength() - len);
	path[len] = 0;

	if (*path) path[--len] = 0;
	else
		_getcwd(path, _MAX_PATH - (int)strlen(namebuf) - 1);

	dlgFile.m_ofn.lpstrInitialDir = path; //no trailing slash

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


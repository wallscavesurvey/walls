BOOL	AddFilter(CString &filter,int nIDS);
BOOL	DoPromptPathName(CString& pathName,DWORD lFlags,
			int numFilters,CString &strFilter,BOOL bOpen,UINT ids_Title,char *defExt);
BOOL	BrowseFiles(CEdit *pw,CString &path,LPCSTR ext,UINT idType,UINT idBrowse,UINT flags);


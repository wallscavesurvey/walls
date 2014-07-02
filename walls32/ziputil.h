struct CReport : public CString
{
	void Append(LPCTSTR lpszMessage);
	void AppendFileName(LPCTSTR lpszFileName);
	bool GetFromException(CException* e); // retrun true if aborted safely
};

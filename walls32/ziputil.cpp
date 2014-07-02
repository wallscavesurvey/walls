#include <ZipArchive.h>
#include "ziputil.h"

static bool IsSafeAbort(CException* pEx)
{
	if (pEx->IsKindOf(RUNTIME_CLASS(CZipException))
		&& ((CZipException*)pEx)->m_iCause == CZipException::abortedSafely)
		return true;
	else
		return false;
}
static bool IsAbort(CException* pEx)
{
	if (pEx->IsKindOf(RUNTIME_CLASS(CZipException))
		&& ((CZipException*)pEx)->m_iCause == CZipException::aborted)
		return true;
	else
		return false;
}

void CReport::Append(LPCTSTR lpszMessage)
{
	if (lpszMessage)
		*this += lpszMessage;
	*this += _T("\r\n");
}

void CReport::AppendFileName(LPCTSTR lpszFileName)
{
	if (!lpszFileName)
		return;
	CString sz;
	sz.Format("File: %s", lpszFileName);
	Append(sz);
	*this += _T("\r\n"); // one more time to separate multiline errors (one from exception, one from this)
}

bool CReport::GetFromException(CException* e)
{
	ASSERT(e);
	if (!e)	return false;

	const iBufSize = 512;
	TCHAR buf[iBufSize];
	e->GetErrorMessage(buf, iBufSize);
	Append(buf);
	return IsSafeAbort(e);
}

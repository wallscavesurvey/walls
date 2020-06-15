#ifndef __SAFEMIRRORFILE_H
#define __SAFEMIRRORFILE_H
#ifndef __AFXPRIV_H__
#include <afxpriv.h>
#endif
class CSafeMirrorFile : public CMirrorFile
{
	// Implementation
public:
	virtual CString GetFilePath() const; // CFile override
	BOOL SafeGetStatus(CFileStatus& rStatus) const; // custom GetStatus	with fix from VC.NET 2005

private:
	BOOL WINAPI IsValidFILETIME(const FILETIME& fileTime) const;
};
#endif

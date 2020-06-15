#include "stdafx.h"
#include "utility.h"

BOOL IsNumeric(LPCSTR p)
{
	UINT bPeriodSeen = 0;

	if (*p == '-' || *p == '+' || (bPeriodSeen = (*p == '.'))) p++;
	if (*p && !isdigit((BYTE)*p)) return FALSE;
	while (*p && !isspace((BYTE)*p)) {
		if (!isdigit((BYTE)*p) && (bPeriodSeen || !(bPeriodSeen = (*p == '.')))) return FALSE;
		p++;
	}
	while (isspace((BYTE)*p)) p++;
	return *p == 0;
}

int _stat_fix(const char *pszPathName, DWORD *pSize /*=NULL*/, BOOL *pbReadOnly /*=NULL*/, BOOL *pbIsDir /*=NULL*/)
{
	WIN32_FILE_ATTRIBUTE_DATA fdata;
	if (!GetFileAttributesEx(pszPathName, GetFileExInfoStandard, &fdata)) {
#ifdef _DEBUG
		DWORD e = GetLastError();
		if (e == 1 || e == 2 || e == 3) {
			return -1;
		}
#endif
		return -1;
	}
	if (pSize) *pSize = fdata.nFileSizeLow;
	if (pbReadOnly) *pbReadOnly = (fdata.dwFileAttributes&FILE_ATTRIBUTE_READONLY) != 0;
	if (pbIsDir) *pbIsDir = (fdata.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) != 0;

	return 0;

}

BOOL MakeFileDirectoryCurrent(LPCSTR pathName)
{
	LPSTR p = (LPSTR)trx_Stpnam(pathName);
	char c = *p;
	*p = 0;
	if (!::SetCurrentDirectory(pathName)) {
		*p = c;
		return FALSE;
	}
	*p = c;
	return TRUE;
}

int MakeNestedDir(char *path)
{
	char *p = path;

	for (; *p; p++) if (*p == '/' || *p == '\\') {
		*p = 0;
		_mkdir(path);
		*p = '\\';
	}
	//Success if and only if last directory can be created
	return _mkdir(path); //0 if successful
}

BOOL DirCheck(LPSTR pathname, BOOL bPrompt)
{
	//Return 1 if directory exists, or 2 if successfully created after prompting.
	//If bPrompt==FALSE, creation is attempted without prompting.
	//Return 0 if directory was absent and not created, in which case
	//a message or prompt was displayed.

	BOOL bIsDir;
	char *pnam;
	BOOL bRet = TRUE;
	char cSav;

	pnam = trx_Stpnam(pathname);
	if (pnam - pathname < 3 || pnam[-1] != '\\') return TRUE;
	if (pnam[-2] != ':') pnam--;
	cSav = *pnam;
	*pnam = 0; //We are only examining the path portion
	if (!_stat_fix(pathname, NULL, NULL, &bIsDir)) {
		//Does directory already exist?
		if (bIsDir) goto _restore;
		goto _failmsg; //A file by that name must already exist
	}
	//_stat falure due to anything but nonexistence?
	if (errno != ENOENT) goto _restore; //Simply try opening the file


	if (bPrompt) {
		goto _fail;
		//Directory doesn't exist, select OK to create it --
		//if(IDCANCEL==CMsgBox(MB_OKCANCEL,IDS_ERR_NODIR1,pathname)) goto _fail;
		bRet++;
	}

	if (!MakeNestedDir(pathname)) goto _restore;

_failmsg:
	//Can't create directory -- possible file name conflict
	//CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_DIRPROJECT,pathname);

_fail:
	bRet = FALSE;

_restore:
	*pnam = cSav;
	return bRet;
}

LPSTR GetRelativePath(LPSTR buf, LPCSTR pPath, LPCSTR pRefPath, BOOL bNoNameOnPath)
{
	//Note: buf has assumed size >= MAX_PATH and pPath==buf is allowed.
	//Last arg should be FALSE if pPath contains a file name, in which case it will
	//be appended to the relative path in buf.

	int lenRefPath = trx_Stpnam(pRefPath) - pRefPath;
	ASSERT(lenRefPath >= 3);

	LPSTR pDupPath = (pPath == buf) ? _strdup(pPath) : NULL;
	if (pDupPath)
		pPath = pDupPath;
	else if (buf != pPath)
		strcpy(buf, pPath);

	if (bNoNameOnPath) strcat(buf, "\\");
	else *trx_Stpnam(buf) = 0;

	int lenFilePath = strlen(buf);
	int maxLenCommon = min(lenFilePath, lenRefPath);
	for (int i = 0; i <= maxLenCommon; i++) {
		if (i == maxLenCommon || toupper(buf[i]) != toupper(pRefPath[i])) {
			if (!i) break;

			while (i && buf[i - 1] != '\\') i--;
			ASSERT(i);
			if (i < 3) break;
			lenFilePath -= i;
			pPath += i;
			LPSTR p = (LPSTR)pRefPath + i;
			for (i = 0; p = strchr(p, '\\'); i++, p++); //i=number of folder level prefixes needed
			if (i * 3 + lenFilePath >= MAX_PATH) {
				//at least don't overrun buf
				break;
			}
			for (p = buf; i; i--) {
				strcpy(p, "..\\"); p += 3;
			}
			//pPath=remaining portion of path that's different
			//pPath[lenFilePath-1] is either '\' or 0 (if bNoNameOnPath)
			if (lenFilePath) {
				memcpy(p, pPath, lenFilePath);
				if (bNoNameOnPath) p[lenFilePath - 1] = '\\';
			}
			p[lenFilePath] = 0;
			break;
		}
	}
	if (!bNoNameOnPath) {
		LPCSTR p = trx_Stpnam(pPath);
		if ((lenFilePath = strlen(buf)) + strlen(p) < MAX_PATH)
			strcpy(buf + lenFilePath, p);
	}
	if (pDupPath) free(pDupPath);
	return buf;
}

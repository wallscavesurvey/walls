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

	struct _stat st;
	char *pnam;
	BOOL bRet = TRUE;
	char cSav;

	pnam = trx_Stpnam(pathname);
	if (pnam - pathname < 3 || pnam[-1] != '\\') return TRUE;
	if (pnam[-2] != ':') pnam--;
	cSav = *pnam;
	*pnam = 0; //We are only examining the path portion
	if (!_stat(pathname, &st)) {
		//Does directory already exist?
		if (st.st_mode&_S_IFDIR) goto _restore;
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
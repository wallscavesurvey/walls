// UTILITY.CPP -- Custom helper functions for MFC programs

#include "stdafx.h"
#include "walls.h"
#include "dialogs.h"
#include "copyfile.h"
#include <stdarg.h>
#include <stdio.h>
#include <direct.h>
#include <commctrl.h>

static char BASED_CODE LoadErrorMsg[] = "Out of workspace. See About Dialog.";
static char BASED_CODE CustomColors[] = "Custom Colors";

int CDECL LoadCMsg(char *buf, int sizBuf, UINT nIDS, ...)
{
	char format[256];
	*buf = 0;
	if (!::LoadString(AfxGetInstanceHandle(), nIDS, format, 256)) return 0;
	va_list marker;
	va_start(marker, nIDS);
	buf[sizBuf - 1] = 0;
	int iRet = _vsnprintf(buf, sizBuf - 1, format, marker);
	va_end(marker);
	return iRet;
}

void CDECL SetWndTitle(CWnd *dlg, UINT nIDS, ...)
{
	char format[256];
	char buf[256];
	if (nIDS) {
		if (!::LoadString(AfxGetInstanceHandle(), nIDS, format, 256)) return;
	}
	else dlg->GetWindowText(format, 256);
	va_list marker;
	va_start(marker, nIDS);
	buf[255] = 0;
	_vsnprintf(buf, 255, format, marker);
	va_end(marker);
	dlg->SetWindowText(buf);
}

int CDECL CMsgBox(UINT nType, char *format, ...)
{
	char buf[1024];

	va_list marker;
	va_start(marker, format);
	buf[1023] = 0;
	_vsnprintf(buf, 1023, format, marker);
	va_end(marker);

	TRY{
		//May try to create temporary objects?
		nType = (UINT)AfxMessageBox(buf,nType);
	}
		CATCH_ALL(e) {
		nType = 0;
	}
	END_CATCH_ALL

		if (!nType) ::MessageBox(NULL, LoadErrorMsg, NULL, MB_ICONEXCLAMATION | MB_OK);
	return (int)nType;
}

int CDECL CMsgBox(UINT nType, UINT nIDS, ...)
{
	char buf[1024];
	CString format;

	//if((nType&MB_ICONEXCLAMATION)==MB_ICONEXCLAMATION) ::MessageBeep(MB_ICONEXCLAMATION);

	TRY{
		//Exceptions can be generated here --
		if (format.LoadString(nIDS)) {
			va_list marker;
			va_start(marker,nIDS);
			buf[1023] = 0;
			_vsnprintf(buf,1023,format,marker);
			va_end(marker);
			nType = (UINT)AfxMessageBox(buf,nType);
		}
		else nType = 0;
	}
		CATCH_ALL(e) {
		nType = 0;
	}
	END_CATCH_ALL

		if (!nType) ::MessageBox(NULL, LoadErrorMsg, NULL, MB_ICONEXCLAMATION | MB_OK);

	return (int)nType;
}

static BOOL bStatusMsg = FALSE;

void CDECL StatusMsg(UINT nIDS, ...)
{
	char buf[128];
	CString format;

	if (!format.LoadString(nIDS)) return;
	va_list marker;
	va_start(marker, nIDS);
	buf[127] = 0;
	_vsnprintf(buf, 127, format, marker);
	va_end(marker);
	AfxGetMainWnd()->SendMessage(WM_SETMESSAGESTRING, 0, (LPARAM)(LPSTR)buf);
	bStatusMsg = TRUE;
}

void StatusClear()
{
	if (bStatusMsg) {
		AfxGetMainWnd()->SendMessage(WM_SETMESSAGESTRING, AFX_IDS_IDLEMESSAGE, 0L);
		bStatusMsg = FALSE;
	}
}

void CDECL UtlTextOut(CDC *dc, int x, int y, char *format, ...)
{
	char buf[128];

	va_list marker;

	va_start(marker, format);
	buf[127] = 0;
	_vsnprintf(buf, 127, format, marker);
	va_end(marker);
	dc->TextOut(x, y, buf);
}

BOOL PASCAL AddFilter(CString &filter, int nIDS)
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

BOOL PASCAL DoPromptPathName(CString& pathName, DWORD lFlags,
	int numFilters, CString &strFilter, BOOL bOpen, UINT ids_Title, char *defExt)
{
#ifdef _DEBUG
	//bVistaStyle==TRUE causes exception the first time this is called while debugging!
	CFileDialog dlgFile(bOpen, 0, 0, OFN_HIDEREADONLY, 0, 0, 0, !IsDebuggerPresent());
#else
	CFileDialog dlgFile(bOpen, 0, 0, OFN_HIDEREADONLY, 0, 0, 0, TRUE);
#endif

	CString title;
	VERIFY(title.LoadString(ids_Title));

	char path[_MAX_PATH], initDir[_MAX_PATH];
	trx_Stncc(initDir, pathName, _MAX_PATH);
	LPSTR pName = trx_Stpnam(initDir);
	if (pName > initDir) {
		//path prefix exists
		pName[-1] = 0;
	}
	strcpy(path, pName);
	if (pName == initDir && !_getcwd(initDir, _MAX_PATH)) *initDir = 0;

	dlgFile.m_ofn.lpstrInitialDir = initDir;
	dlgFile.m_ofn.Flags |= (lFlags | OFN_ENABLESIZING);
	if (!(lFlags&OFN_OVERWRITEPROMPT)) dlgFile.m_ofn.Flags &= ~OFN_OVERWRITEPROMPT; //handled above

	dlgFile.m_ofn.nMaxCustFilter += numFilters;
	dlgFile.m_ofn.lpstrFilter = strFilter;
	dlgFile.m_ofn.hwndOwner = AfxGetApp()->m_pMainWnd->GetSafeHwnd();
	dlgFile.m_ofn.lpstrTitle = title;
	dlgFile.m_ofn.lpstrFile = path;
	dlgFile.m_ofn.nMaxFile = _MAX_PATH;
	if (defExt) dlgFile.m_ofn.lpstrDefExt = defExt + 1;
	//dlgFile.m_ofn.FlagsEx= OFN_EX_NOPLACESBAR; //fixes invalid handle error in DEBUG mode, but empties navigation pane!!!

	if (dlgFile.DoModal() == IDOK) {
		pathName.SetString(path);
		return TRUE;
	}
	//DWORD err=CommDlgExtendedError();
	return FALSE;
}

//Support for centering stock dialogs --
BEGIN_MESSAGE_MAP(CCenterFileDialog, CFileDialog)
	ON_MESSAGE(WM_COMMANDHELP, OnCommandHelp)
	//{{AFX_MSG_MAP(CCenterFileDialog)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CCenterFileDialog::OnInitDialog()
{
	CFileDialog::OnInitDialog();
	CenterWindow();
	return TRUE;
}
LRESULT CCenterFileDialog::OnCommandHelp(WPARAM wNone, LPARAM lParam)
{
	AfxGetApp()->WinHelp(15);
	return TRUE;
}

BEGIN_MESSAGE_MAP(CCenterColorDialog, CColorDialog)
	ON_MESSAGE(WM_COMMANDHELP, OnCommandHelp)
	//{{AFX_MSG_MAP(CCenterColorDialog)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

DWORD CCenterColorDialog::clrChecksum;

BOOL CCenterColorDialog::OnInitDialog()
{
	CColorDialog::OnInitDialog();
	SetWindowText(m_pTitle);
	CenterWindow();
	return TRUE;
}
LRESULT CCenterColorDialog::OnCommandHelp(WPARAM wNone, LPARAM lParam)
{
	AfxGetApp()->WinHelp(14);
	return TRUE;
}

void CCenterColorDialog::LoadCustomColors()
{
	CWinApp* pApp = AfxGetApp();
	char clr[8];
	CString str;
	int r, g, b;

	clrChecksum = 0;
	for (int i = 0; i < 16; i++) {
		sprintf(clr, "Color%u", i + 1);
		str = pApp->GetProfileString(CustomColors, clr);
		if (str.IsEmpty()) continue;
		tok_init(str.GetBuffer(1));
		r = tok_int();
		g = tok_int();
		b = tok_int();
		clrChecksum += (i + 1)*(clrSavedCustom[i] = RGB(r, g, b));
	}
}

void CCenterColorDialog::SaveCustomColors()
{
	CWinApp* pApp = AfxGetApp();
	char clr[8], buf[16];
	int i;
	DWORD chk = 0;

	for (i = 0; i < 16; i++)
		if (clrSavedCustom[i] != RGB_WHITE) {
			chk += (i + 1)*clrSavedCustom[i];
		}
	if (chk == clrChecksum) return;
	//NOTE: MFC version asserts if 3rd argument is NULL! --
	//::WritePrivateProfileString(CustomColors,NULL,NULL,pApp->m_pszProfileName);
	for (i = 0; i < 16; i++) {
		if ((chk = clrSavedCustom[i]) == RGB_WHITE) continue;
		sprintf(clr, "Color%u", i + 1);
		sprintf(buf, "%u %u %u", GetRValue(chk), GetGValue(chk), GetBValue(chk));
		pApp->WriteProfileString(CustomColors, clr, buf);
	}
}

void Pause()
{
	MSG msg;
	while (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		if (msg.message == WM_QUIT)
		{
			//bDoingBackgroundProcessing = FALSE;
			::PostQuitMessage(msg.wParam);
			break;
		}
		if (!AfxGetApp()->PreTranslateMessage(&msg))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}
	}
	AfxGetApp()->OnIdle(0);   // updates user interface
	AfxGetApp()->OnIdle(1);   // frees temporary objects 
}

int PASCAL GetWinPos(int *pWinPos, int siz_pos, CRect &rect, CSize &size, int &cxIcon)
{
	int offset;

	if (rect.Width() < 8 * cxIcon) cxIcon = 0;
	rect.InflateRect(-cxIcon, -cxIcon);


	for (offset = 0; offset < siz_pos; offset++)
		if (!pWinPos[offset]) break;
	if (offset == siz_pos) {
		memset(pWinPos, 0, siz_pos * sizeof(int));
		offset = 0;
	}
	pWinPos[offset]++;

	cxIcon *= offset;

	{
		int w = rect.Width();
		if (size.cx + cxIcon > w) size.cx = w - cxIcon;
		w = rect.Height();
		if (size.cy + cxIcon > w) size.cy = w - cxIcon;
	}

	return offset;
}

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

char * GetDateStr(int julday)
{
	static char buf[16];
	return trx_JulToDateStr(julday, buf);
}

char * GetFloatStr(double f, int dec)
{
	//Obtains the minimal width numeric string of f rounded to dec decimal places.
	//Any trailing zeroes (including trailing decimal point) are truncated.
	static char buf[40];
	BOOL bRound = TRUE;

	if (dec < 0) {
		bRound = FALSE;
		dec = -dec;
	}
	if (_snprintf(buf, 20, "%-0.*f", dec, f) == -1) {
		*buf = 0;
		return buf;
	}
	if (bRound) {
		char *p = strchr(buf, '.');
		ASSERT(p);
		for (p += dec; dec && *p == '0'; dec--) *p-- = 0;
		if (*p == '.') {
			*p = 0;
			if (!strcmp(buf, "-0")) strcpy(buf, "0");
		}
	}
	return buf;
}

char * GetIntStr(int i)
{
	static char buf[20];
	sprintf(buf, "%d", i);
	return buf;
}

BOOL CheckInt(const char *buf, int *pi, int iMin, int iMax)
{
	int i;
	if (!IsNumeric(buf) || (i = atoi(buf)) < iMin || i > iMax) {
		CMsgBox(MB_ICONEXCLAMATION, IDS_ERR_BADINT2, iMin, iMax);
		return FALSE;
	}
	*pi = i;
	return TRUE;
}

BOOL CheckFlt(const char *buf, double *pf, double fMin, double fMax, int dec)
{
	//if(!strcmp(buf,GetFloatStr(*pf,dec))) return TRUE;

	double f;
	if (!IsNumeric(buf) || (f = atof(buf)) < fMin || f > fMax) {
		CMsgBox(MB_ICONEXCLAMATION, IDS_ERR_BADFLT2, dec, fMin, dec, fMax);
		return FALSE;
	}
	*pf = f;
	return TRUE;
}

void PASCAL GetStrFromFld(char *s, const void *f, int len)
{
	while (len && ((char *)f)[len - 1] == ' ') len--;
	memcpy(s, f, len);
	s[len] = 0;
}

void PASCAL GetFldFromStr(void *f, const char *s, int len)
{
	memset(f, ' ', len);
	memcpy(f, s, strlen(s));
}

static int _tok_count;
static char *_tok_ptr;
void tok_init(char *p)
{
	_tok_count = 0;
	_tok_ptr = p;
}

double tok_dbl()
{
	char *p = strtok(_tok_ptr, " ");
	_tok_ptr = NULL;
	if (p) {
		_tok_count++;
		return atof(p);
	}
	return 0.0;
}

int tok_int()
{
	char *p = strtok(_tok_ptr, " ");
	_tok_ptr = NULL;
	if (p) {
		_tok_count++;
		return atoi(p);
	}
	return 0;
}

int tok_count()
{
	return _tok_count;
}

static int MakeNestedDir(char *path)
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


BOOL DirCheck(char *pathname, BOOL bPrompt)
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
		//Directory doesn't exist, select OK to create it --
		if (IDCANCEL == CMsgBox(MB_OKCANCEL, PRJ_ERR_NODIR1, pathname)) goto _fail;
		bRet++;
	}
	if (!MakeNestedDir(pathname)) goto _restore;

_failmsg:
	//Can't create directory -- possible file name conflict
	CMsgBox(MB_ICONEXCLAMATION, PRJ_ERR_DIRPROJECT, pathname);
_fail:
	bRet = FALSE;

_restore:
	*pnam = cSav;
	return bRet;
}

CString GetWallsPath()
{
	char path[_MAX_PATH];
	strcpy(path, AfxGetApp()->m_pszHelpFilePath);
	*trx_Stpnam(path) = 0;
	return path;
}

BOOL SetCurrentDir(const char *path)
{
	char buf[_MAX_PATH];
	int lenpath = trx_Stpnam((LPSTR)path) - (LPSTR)path;
	if (lenpath >= _MAX_PATH) return FALSE;
	if (lenpath > 0) {
		memcpy(buf, path, lenpath);
		buf[lenpath] = 0;
		if (_chdir(buf)) return FALSE;
		if (buf[1] == ':') _chdrive(toupper(*buf) - (int)('A') + 1); //shouldn't fail
	}
	return TRUE;
}

#ifdef _USE_ZONELETTER
char *ZoneStr(int zone, double abslat)
{
	//abslat is abs(latitude) ==
	static char buf[8];

	ASSERT(zone && abs(zone) <= 61);

	if (abs(zone) == 61) {
		strcpy(buf, (zone < 0) ? "UPS S" : "UPS N");
	}
	else {
		int i = ' ';
		if (zone < 0) {
			zone = -zone;
			if (abslat <= 80) {
				i = (int)((80 - abslat) / 8);
				if (i > 5) i++;
				i += 'C';
			}
		}
		else if (abslat < 84) {
			i = (int)(abslat / 8);
			if (i > 0) i++;
			if (i > 10) i = 10;
			i += 'N';
		}
		sprintf(buf, "UTM %u%c", zone, i);
	}
	return buf;
}
#else
char *ZoneStr(int zone)
{
	static char buf[8];

	ASSERT(zone && abs(zone) <= 61);

	if (abs(zone) == 61) {
		strcpy(buf, (zone < 0) ? "UPS S" : "UPS N");
	}
	else {
		sprintf(buf, "UTM %u%c", abs(zone), (zone < 0) ? 'S' : 'N');
	}
	return buf;
}
#endif

BOOL IsAbsolutePath(const char *path)
{
	return *path == '\\' || *path == '/' || (*path && path[1] == ':');
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

SYSTEMTIME *GetLocalFileTime(const char *pszPathName, long *pSize, BOOL *pbReadOnly /*=NULL*/)
{
	static SYSTEMTIME stLocal;
	memset(&stLocal, 0, sizeof(SYSTEMTIME));
	if (pbReadOnly) *pbReadOnly = 0;
	if (pSize) *pSize = 0;

#ifdef _CHKSTAT
	//_stat() fails under Windows XP due to a bug in VS 2015 - Windows XP (v140_xp) toolset !!!
	//Workaround is to use GetFileAttributesEx(). This tests the workaround in _DEBUG mode under Win7+ --
	static BOOL nFail = 0;
	struct _stat status;
	UINT size2, timesum2 = 0;
	BOOL bReadOnly2 = 2, bNotFound = 0;
	struct tm *ptm;

	if (_stat(pszPathName, &status)) {
		if (errno == ENOENT) {
			bNotFound = 1;
		}
		if (bNotFound || nFail) goto _cont;
		CMsgBox(MB_ICONASTERISK, "_stat() returns EINVAL with file %s", pszPathName);
		nFail++;
		goto _cont;
	}

	if (pbReadOnly) bReadOnly2 = (status.st_mode&_S_IWRITE) == 0;
	size2 = status.st_size;
	ptm = localtime(&status.st_mtime);
	if (ptm) timesum2 = ptm->tm_sec + ptm->tm_min + ptm->tm_hour + ptm->tm_mday + ptm->tm_mon + ptm->tm_year;
_cont:
#endif

	WIN32_FILE_ATTRIBUTE_DATA fdata;
	if (!GetFileAttributesEx(pszPathName, GetFileExInfoStandard, &fdata)) {
#ifdef _DEBUG
		DWORD e = GetLastError();
		if (e == 1 || e == 2 || e == 3) {
#ifdef _CHKSTAT
			ASSERT(bNotFound);
#endif
			return NULL;
		}
#ifdef _CHKSTAT
		ASSERT(!bNotFound || !_TraceLastError());
#endif
#endif
		return NULL;
	}
	if (pbReadOnly) *pbReadOnly = (fdata.dwFileAttributes&FILE_ATTRIBUTE_READONLY) != 0;
	if (pSize) *pSize = fdata.nFileSizeLow;

	// Convert the last-write time to local time.
	SYSTEMTIME stUTC;
	if (!FileTimeToSystemTime(&fdata.ftLastWriteTime, &stUTC))
		goto _eret;
	if (!SystemTimeToTzSpecificLocalTime(NULL, &stUTC, &stLocal))
		goto _eret;

#ifdef _DEBUG
#ifdef _CHKSTAT
	if (!nFail) {
		UINT timesum1 = stLocal.wSecond + stLocal.wMinute + stLocal.wHour + stLocal.wDay + stLocal.wMonth - 1 + stLocal.wYear - 1900;
		ASSERT(timesum1 == timesum2 && (!pSize || *pSize == size2));
		ASSERT(!pbReadOnly || *pbReadOnly == bReadOnly2);
	}
#endif
#endif

	return &stLocal;

_eret:
	ASSERT(0);
	return NULL;
}

char * GetTimeStr(SYSTEMTIME *ptm, long *pSize)
{
	static char buf[56];
	if (!ptm) *buf = 0;
	else {
		int len = _snprintf(buf, 56, "%4u-%02u-%02u %02u:%02u:%02u",
			ptm->wYear, ptm->wMonth, ptm->wDay,
			ptm->wHour, ptm->wMinute, ptm->wSecond);
		if (pSize) {
			_snprintf(buf + len, 56 - len, "  Size: %u KB", (*pSize + 500) / 1000);
		}
		buf[55] = 0;
	}
	return buf;
}

void TrackMouse(HWND hWnd)
{
	TRACKMOUSEEVENT tme;
	tme.cbSize = sizeof(tme);
	tme.dwFlags = TME_LEAVE;
	tme.hwndTrack = hWnd;
	_TrackMouseEvent(&tme);
}

void GetCoordinateFormat(CString &s, DPOINT &coord, BOOL bProfile, BOOL bFeetUnits)
{
	if (bProfile) {
		int inc = (coord.y >= 100.0) ? 0 : ((coord.y >= 10.0) ? 1 : 2);
		s.Format("%10.*f %s elev", inc, coord.y, bFeetUnits ? "ft" : "m");
	}
	else s.Format("%10.2f E%11.2f N", coord.x, coord.y);
}

void ElimNL(char *p)
{
	while (*p) if (*p++ == '\n') p[-1] = ' ';
}

void FixPathname(CString &path, char *ext)
{
	if (!*trx_Stpext(path)) {
		char *p = trx_Stpext(path.GetBuffer(path.GetLength() + 5));
		*p++ = '.';
		strcpy(p, ext);
		path.ReleaseBuffer();
	}
}

void DimensionStr(char *buf, double w, double h, BOOL bInches)
{
	char *punits = "in";

	if (!bInches) {
		w *= 2.54;
		h *= 2.54;
		punits = "cm";
	}
	_snprintf(buf, 60, "%.2f x %.2f %s", w, h, punits);
}

BOOL FilePatternExists(LPCSTR buf)
{
	WIN32_FIND_DATA fd;

	HANDLE h = ::FindFirstFile(buf, &fd);
	if (h == INVALID_HANDLE_VALUE) return FALSE;
	::FindClose(h);
	return TRUE;
}

int parse_name(LPCSTR name)
{
	LPCSTR p;

	if (p = strchr(name, ':')) p++;
	else p = name;
	if (*p == '#' || *name == '#') return IDS_ERR_NAMEPFX;
	if (!*p) return IDS_ERR_PFXNONAME;
	if (strlen(p) > 8) return IDS_ERR_NAMETOOLONG;
	return 0;
}


int parse_2names(char *names, BOOL bShowError)
{
	int i, e;

	if (*names == '#' && (e = strlen(names)) > 5 && !memicmp(names, "#FIX", 4) && isspace((BYTE)names[4]) && e < 63) {
		memmove(names + 5, names + 4, e - 3);
		memcpy(names, "<REF>", 5);
	}

	ASSERT(cfg_quotes == 0);
	cfg_quotes = 0;
	cfg_GetArgv(names, CFG_PARSE_ALL);

	if (cfg_argc != 2) {
		e = IDS_ERR_NOT2NAMES;
		goto _fail;
	}

	for (i = 0; i < 2; i++) {
		if (e = parse_name(cfg_argv[i])) goto _fail;
	}
	return 0;

_fail:
	if (bShowError) AfxMessageBox(e);
	return e;
}

void GetStrFromDateFld(char *s, void *date, LPCSTR delim)
{
	DWORD dw = *(DWORD *)date;
	if (!(dw &= 0xFFFFFF)) *s = 0;
	else sprintf(s, "%04u%s%02u%s%02u", ((dw >> 9) & 0x7FFF), delim, ((dw >> 5) & 0xF), delim, (dw & 0x1F));
}

LPSTR FixFlagCase(char *name)
{
	char *p;
	strlwr(name);
	while (TRUE) {
		while (isspace((BYTE)*name)) name++;
		if (!*name) break;
		*name = toupper(*name);
		if (!(p = strstr(name, ": ")) && !(p = strstr(name, "- "))) break;
		name = p + 2;
	}
	return name;
}

static pcre *CompileRegEx(LPCSTR pattern, LPCSTR *error, WORD wFlags)
{
	char buf[128 + 12];
	char *outFlags[3];

	outFlags[0] = outFlags[1] = outFlags[2] = "";
	if (!(wFlags&F_USEREGEX) || (wFlags&F_MATCHWHOLEWORD)) {
		if (!(wFlags&F_USEREGEX)) {
			//Quote the string --
			outFlags[1] = "\\Q";
			outFlags[2] = "\\E";
		}
		if (wFlags&F_MATCHWHOLEWORD) {
			//Test for word boundary --
			outFlags[0] = "\\b";
		}
		_snprintf(buf, sizeof(buf), "%s%s%s%s%s", outFlags[0], outFlags[1], pattern, outFlags[2], outFlags[0]);
		buf[sizeof(buf) - 1] = 0;
		pattern = buf;
	}

	int erroffset;
	return pcre_compile(
		pattern,          /* the pattern */
		(wFlags&F_MATCHCASE) ? 0 : PCRE_CASELESS,
		error,            /* for error message */
		&erroffset,       /* for error offset */
		NULL);            /* use default character tables */
}

pcre *GetRegEx(LPCSTR pattern, WORD wFlags)
{
	const char *error;
	return CompileRegEx(pattern, &error, wFlags);
}

BOOL CheckRegEx(LPCSTR pattern, WORD wFlags)
{
	const char *error;
	pcre *re = CompileRegEx(pattern, &error, wFlags);
	if (re) {
		free(re);
		return TRUE;
	}
	CMsgBox(MB_ICONINFORMATION, "Invalid regular expression: %s", error);
	return FALSE;
}

/*
void InsertDriveLetter(char *pathbuf,const char *drvpath)
{
	if(*pathbuf=='/') *pathbuf='\\';
	else if(*pathbuf!='\\') return;
	memmove(pathbuf+2,pathbuf,strlen(pathbuf)+1);
	*pathbuf=*drvpath;
	pathbuf[1]=':';
}
*/

BOOL BrowseFiles(CEdit *pw, CString &path, char *ext, UINT idType, UINT idBrowse)
{
	CString strFilter;
	CString Path;

	int numFilters = (idType == IDS_PRJ_FILES) ? 1 : 2;

	pw->GetWindowText(Path);

	if (!AddFilter(strFilter, idType) || numFilters == 2 &&
		!AddFilter(strFilter, AFX_IDS_ALLFILTER)) return FALSE;

	if (!DoPromptPathName(Path, OFN_HIDEREADONLY, numFilters, strFilter, FALSE, idBrowse, ext)) return FALSE;

	path = Path;
	FixPathname(path, ext + 1);
	pw->SetWindowText(path);
	pw->SetSel(0, -1);
	pw->SetFocus();
	return TRUE;
}

int CheckDirectory(CString &path, BOOL bOpen /*=FALSE*/)
{
	//Return 0 if directory was created (if necessary) and either the file
	//was absent or permission was given to overwrite an existing file.
	//Return 1 if operation cancelled
	//Return 2 if permission given to open existing file (bOpen=TRUE)

	//First, let's see if the directory needs to be created --
	int e = DirCheck((char *)(LPCSTR)path, TRUE);
	//e=1 if directory already exists,
	//e=2 if successfully created after prompting.
	//e=0 if directory was absent and not created after prompting

	if (!e) e = 1;
	else if (e == 1) {
		//The directory already existed. Check for file existence --
		if (_access(path, 6) != 0) {
			if (errno != ENOENT) {
				//file not rewritable --
				AfxMessageBox(IDS_ERR_FILE_RW);
				e = 1;
			}
			else e = 0;
		}
		else {
			//File exists and is rewritable.
			long filesize = 0;
			SYSTEMTIME *ptm = GetLocalFileTime(path, &filesize);
			if (!bOpen) e = (IDOK == CMsgBox(MB_OKCANCEL, IDS_FILE_EXISTDLG, (LPCSTR)path, GetTimeStr(ptm, &filesize))) ? 0 : 1;
			else {
				CString s;
				s.Format(IDS_FILE_OPENEXIST, (LPCSTR)path, GetTimeStr(ptm, &filesize));
				COpenExDlg dlg(s);
				if ((e = dlg.DoModal()) == IDOK) e = 0; //Overwrite
				else if (e == IDCANCEL) e = 1;
				else e = 2; //Open existing file
			}
		}
	}
	else e = 0;
	return e;
}

int CompareNumericLabels(LPCSTR p, LPCSTR p1)
{
	int i, j;
	while (*p && *p1) {
		if (isdigit((BYTE)*p) && isdigit((BYTE)*p1)) {
			i = atoi(p);
			j = atoi(p1);
			if (i != j) return (i < j) ? -1 : 1;
			while (isdigit((BYTE)*++p));
			while (isdigit((BYTE)*++p1));
			continue;
		}
		i = toupper(*p++) - toupper(*p1++);
		if (i) return (i < 0) ? -1 : 1;
	}
	return (*p1) ? -1 : (*p != 0);
}

int SetFileReadOnly(LPCSTR pPath, BOOL bReadOnly, BOOL bNoPrompt)
{
	BOOL bIsRO;
	if (!_stat_fix(pPath, NULL, &bIsRO)) {
		//File exists on disk --
		if (bReadOnly == bIsRO) return 1;  //status unchanged
		if (!_chmod(pPath, bReadOnly ? _S_IREAD : (_S_IWRITE | _S_IREAD))) return 2; //status changed
		//Unable to change file status on disk
		if (!bNoPrompt) AfxMessageBox(IDS_ERR_FILE_RW);
		return -1;
	}
	return 0; //file doesn't exist
}

double GetElapsedSecs(BOOL bInit)
{
	static LARGE_INTEGER swTimer;
	static LARGE_INTEGER swFreq;
	if (bInit) {
		QueryPerformanceFrequency(&swFreq);
		if (swFreq.LowPart == 0 && swFreq.HighPart == 0) swFreq.LowPart = 1;
		QueryPerformanceCounter(&swTimer);
		return 0.0;
	}
	LARGE_INTEGER lStop;
	QueryPerformanceCounter(&lStop);
	return (lStop.QuadPart - swTimer.QuadPart) / (double)swFreq.QuadPart;
}

BOOL ClipboardCopy(LPCSTR strData, UINT cf /*=CF_TEXT*/)
{
	BOOL bRet = FALSE;
	// test to see if we can open the clipboard first before
	// wasting any cycles with the memory allocation
	if (AfxGetMainWnd()->OpenClipboard())
	{
		// Empty the Clipboard. This also has the effect
		// of allowing Windows to free the memory associated
		// with any data that is in the Clipboard
		EmptyClipboard();

		// Ok. We have the Clipboard locked and it's empty. 
		// Now let's allocate the global memory for our data.

		// Here I'm simply using the GlobalAlloc function to 
		// allocate a block of data equal to the text in the
		// "to clipboard" edit control plus one character for the
		// terminating null character required when sending
		// ANSI text to the Clipboard.
		HGLOBAL hClipboardData;
		hClipboardData = GlobalAlloc(GMEM_DDESHARE, strlen(strData) + 1);

		// Calling GlobalLock returns to me a pointer to the 
		// data associated with the handle returned from GlobalAlloc
		char * pchData = (char*)GlobalLock(hClipboardData);

		// At this point, all I need to do is use the standard 
		// C/C++ strcpy function to copy the data from the local 
		// variable to the global memory.
		strcpy(pchData, strData);

		// Once done, I unlock the memory - remember you don't call
		// GlobalFree because Windows will free the memory 
		// automatically when EmptyClipboard is next called.
		GlobalUnlock(hClipboardData);

		// Now, set the Clipboard data by specifying that 
		// ANSI text is being used and passing the handle to
		// the global memory.
		bRet = (SetClipboardData(cf, hClipboardData) != NULL);

		// Finally, when finished I simply close the Clipboard
		// which has the effect of unlocking it so that other
		// applications can examine or modify its contents.
		CloseClipboard();
	}
	return bRet;
}

BOOL ClipboardPaste(LPSTR strData, UINT maxLen)
{
	// Test to see if we can open the clipboard first.
	*strData = 0;
	if (AfxGetMainWnd()->OpenClipboard())
	{
		if (::IsClipboardFormatAvailable(CF_TEXT))
		{
			// Retrieve the Clipboard data (specifying that 
			// we want ANSI text (via the CF_TEXT value).
			HANDLE hClipboardData = GetClipboardData(CF_TEXT);

			// Call GlobalLock so that to retrieve a pointer
			// to the data associated with the handle returned
			// from GetClipboardData.
			char *pchData = (char*)GlobalLock(hClipboardData);

			// Set the CString variable to the data
			if (strlen(pchData) <= maxLen)
				strcpy(strData, pchData);
			// Unlock the global memory.
			GlobalUnlock(hClipboardData);
		}
		// Finally, when finished I simply close the Clipboard
		// which has the effect of unlocking it so that other
		// applications can examine or modify its contents.
		CloseClipboard();
	}
	return (*strData != 0);
}

BOOL OpenContainingFolder(HWND hwnd, LPCSTR path)
{
	CString sParam;
	sParam.Format("/select,\"%s\"", path);
	if ((int)ShellExecute(hwnd, "OPEN", "explorer.exe", sParam, NULL, SW_NORMAL) <= 32) {
		AfxMessageBox("Unable to launch Explorer");
		return 0;
	}
	return 1;
}

LPCSTR TrimPath(LPSTR path, int maxlen)
{
	LPSTR pStart;
	int lenttl;
	if ((lenttl = strlen(path)) <= maxlen || !(pStart = strchr(path, '\\')))
		return path;

	if (*++pStart == '\\') pStart++;
	LPSTR pNext = pStart;
	lenttl += 2; //space for ".."

	for (int len = lenttl; len > maxlen; pNext++) {
		if (!(pNext = strchr(pNext, '\\')))
			return path;
		len = lenttl - (pNext - pStart);
	}
	*pStart++ = '.';
	*pStart++ = '.';
	strcpy(pStart, pNext - 1);
	return path;
}

LPCSTR TrimPath(CString &path, int maxlen)
{
	TrimPath(path.GetBuffer(), maxlen);
	path.ReleaseBuffer();
	return path;
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

void FixFilename(CString &name)
{
	static LPCSTR pBadChrs = "\\/:?\"<>|*";
	for (LPSTR pn = name.GetBuffer(); *pn; pn++) {
		if (strchr(pBadChrs, *pn)) *pn = '_';
	}
	name.ReleaseBuffer();
}

// RTL_OSVERSIONINFOEXW is defined in winnt.h
BOOL GetOsVersion(RTL_OSVERSIONINFOEXW* pk_OsVer)
{
	typedef LONG(WINAPI* tRtlGetVersion)(RTL_OSVERSIONINFOEXW*);

	memset(pk_OsVer, 0, sizeof(RTL_OSVERSIONINFOEXW));
	pk_OsVer->dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOEXW);

	HMODULE h_NtDll = GetModuleHandleW(L"ntdll.dll");
	tRtlGetVersion f_RtlGetVersion = (tRtlGetVersion)GetProcAddress(h_NtDll, "RtlGetVersion");

	ASSERT(f_RtlGetVersion); //All processes load ntdll.dll

	return f_RtlGetVersion && f_RtlGetVersion(pk_OsVer) == 0;
}

DWORD GetOsMajorVersion()
{
	RTL_OSVERSIONINFOEXW rtl;
	if (!GetOsVersion(&rtl)) return 0;
	return rtl.dwMajorVersion;
}

#ifdef _DEBUG
// Makes trace windows a little bit more informative...
BOOL _TraceLastError()
{
	LPVOID lpMsgBuf;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	TRACE1("Last error: %s\n", lpMsgBuf);
	LocalFree(lpMsgBuf);
	return TRUE;
}
#endif

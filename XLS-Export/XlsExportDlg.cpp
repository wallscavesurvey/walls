// XlsExportDlg.cpp : implementation file
//

#include "stdafx.h"
#include "XlsExport.h"
#include "PromptPath.h"
#include <dbfile.h>
#include <georef.h>
#include "XlsExportDlg.h"
#include "utility.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

static bool bDM = false;

static LPCSTR pFldSep;
static int nSep;
static UINT xlsline = 0, numRecs = 0;
static CString csLogPath;

CXlsExportDlg::CXlsExportDlg(int argc, char **argv, CWnd* pParent /*=NULL*/)
	: CDialog(CXlsExportDlg::IDD, pParent)
	, m_argc(argc)
	, m_argv(argv)
	, m_nLogMsgs(0)
	, m_bLogFailed(FALSE)
	, m_fbLog(2048)
	, m_bLatLon(TRUE)
	, m_psd(NULL)
	, m_bMinFldLen(FALSE)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	*m_szSourcePath = *m_szShpdefPath = *m_szShpPath = *m_szModule = 0;

	if (m_argc > 1) {
		strcpy(m_szModule, m_argv[1]);
		int len = GetFullPathName(m_argv[1], MAX_PATH, m_szModule, NULL);
		if (!len || len > MAX_PATH - 4) *m_szModule = 0;
		else if (m_szModule[len - 1] == '\\') {
			::GetModuleFileName(NULL, m_szShpPath, MAX_PATH);
			LPCSTR p = trx_Stpnam(m_szShpPath);
			if (len + strlen(p) < MAX_PATH) strcpy(m_szModule + len, p);
			else *m_szModule = 0;
			*m_szShpPath = 0;
		}
	}
	if (!*m_szModule)
		::GetModuleFileName(NULL, m_szModule, MAX_PATH - 4);

	strcpy(trx_Stpext(m_szModule), ".ini");

	::GetPrivateProfileString("Paths", "Source", "", m_szSourcePath, _MAX_PATH, m_szModule);
	if (*m_szSourcePath) {
		char table[80];
		::GetPrivateProfileString("Paths", "Table", "", table, 80, m_szModule);
		m_tableName = table;
		::GetPrivateProfileString("Paths", "Shpdef", "", m_szShpdefPath, _MAX_PATH, m_szModule);
		::GetPrivateProfileString("Paths", "Output", "", m_szShpPath, _MAX_PATH, m_szModule);
		m_bLatLon = ::GetPrivateProfileInt("Prefs", "UTM", 0, m_szModule) == 0;
		m_bMinFldLen = ::GetPrivateProfileInt("Prefs", "MinFld", 0, m_szModule) != 0;
	}

	m_sourcePath = m_szSourcePath;
	m_shpdefPath = m_szShpdefPath;
	m_shpPath = m_szShpPath;
}

void CXlsExportDlg::SaveOptions()
{
	::WritePrivateProfileString("Paths", "Source", m_szSourcePath, m_szModule);
	::WritePrivateProfileString("Paths", "Table", m_tableName, m_szModule);
	::WritePrivateProfileString("Paths", "Shpdef", m_szShpdefPath, m_szModule);
	::WritePrivateProfileString("Paths", "Output", m_szShpPath, m_szModule);
	::WritePrivateProfileString("Prefs", "UTM", m_bLatLon ? NULL : "1", m_szModule);
	::WritePrivateProfileString("Prefs", "MinFld", m_bMinFldLen ? "1" : NULL, m_szModule);
}

BEGIN_MESSAGE_MAP(CXlsExportDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BROWSE, OnBnClickedBrowse)
	ON_BN_CLICKED(IDC_BROWSE_TMP, &CXlsExportDlg::OnBnClickedBrowseShpdef)
	ON_BN_CLICKED(IDC_BROWSE_SHP, &CXlsExportDlg::OnBnClickedBrowseShp)
	ON_BN_CLICKED(IDC_EDIT_TMP, &CXlsExportDlg::OnBnClickedEditTmp)
END_MESSAGE_MAP()

void CDECL CXlsExportDlg::WriteLog(const char *format, ...)
{
	char buf[256];
	int len;

	if (m_bLogFailed) return;

	if (!m_fbLog.IsOpen()) {
		csLogPath = FixShpPath(".log");

		if ((m_bLogFailed = m_fbLog.OpenFail(csLogPath, CFile::modeCreate | CFile::modeReadWrite | CFile::shareExclusive)))
			return;
		CString s;
		if (m_bXLS <= 1) s.Format(", Table: %s", (LPCSTR)m_tableName);
		len = _snprintf(buf, 255, "Export Log - Source database: %s%s\r\nShapefile: %s\r\n\r\n", m_pSourceName, (LPCSTR)s, m_pShpName);
		m_fbLog.Write(buf, len);
	}

	m_nLogMsgs++;

	va_list marker;
	va_start(marker, format);
	//NOTE: Does the 2nd argument include the terminating NULL?
	//      The return value definitely does NOT.
	CString key;
	if (!m_fldKeyVal.IsEmpty()) key.Format(", Key: %s", (LPCSTR)m_fldKeyVal);
	len = _snprintf(buf, 255, "Rec:%7u%s - ", numRecs + 1, key); //also should show xlsline id reford is skipped
	m_fbLog.Write(buf, len);
	if ((len = _vsnprintf(buf, 255, format, marker)) < 0) len = 254;
	m_fbLog.Write(buf, len);
	m_fbLog.Write("\r\n", 2);
	va_end(marker);
}

static BOOL GetMemoFld(LPSTR pDest, CString &cs)
{
	UINT dbtrec = dbt_AppendRec(cs);
	if (!dbtrec) return FALSE;
	sprintf(pDest, "%010u", dbtrec);
	return TRUE;
}

LPCSTR CXlsExportDlg::FixShpPath(LPCSTR ext)
{
	m_pathBuf.Truncate(m_nPathLen);
	m_pathBuf += ext;
	return m_pathBuf;
}

static double getDeg(double y)
{
	y *= 0.0001;
	double deg = (UINT)y;
	y = (y - deg)*100.0;
	double min = (UINT)y;
	y = (y - min)*100.0;
	return deg + min / 60.0 + ((UINT)y) / 3600.0;
}

static double getDM(double x)
{
	double deg = (int)x;
	return deg + 100 * (x - deg) / 60;
}

int CXlsExportDlg::StoreFldData(int f)
{
	char buf[256];
	int iRet = 0, len = 0;
	DBF_FLDDEF &fd = m_psd->v_fldDef[f];
	int fSrc = m_psd->v_srcIdx[f];
	if (fSrc < 0) {
		LPCSTR pDefault = (LPCSTR)m_psd->v_srcDflt[f];
		if (*pDefault) {
			if (fd.F_Typ == 'M') {
				CString memo(pDefault);
				if (!GetMemoFld(buf, memo)) iRet = -memo.GetLength();
				else StoreText(f, buf);
			}
			else StoreText(f, pDefault);
		}
		return iRet;
	}
	switch (fd.F_Typ) {
	case 'N':
	{
		if (fd.F_Dec == 0 && fd.F_Len < 9) {
			long n = GetDbLong(fSrc);
			len = _snprintf(buf, fd.F_Len, "%*d", fd.F_Len, n);
		}
		else {
			double d = GetDbDouble(fSrc); //handles dbDecimal for now
			len = _snprintf(buf, fd.F_Len, "%*.*f", fd.F_Len, fd.F_Dec, d);
		}
		if (len < 0) {
			iRet = -1;
			len = fd.F_Len;
		}
		break;
	}
	case 'C':
	{
		if (fSrc >= 0) {
			len = GetDbText(buf, fSrc, fd.F_Len); //len = actual data length
		}
		if (!len) {
			len = strlen(strcpy(buf, m_psd->v_srcDflt[f]));
		}
		if (len > fd.F_Len) {
			iRet = fd.F_Len - len;
			len = fd.F_Len;
		}
		break;
	}
	case 'M':
	{
		CString memo;
		CString &rMemo = (m_bXLS <= 1) ? memo : m_vcsv[fSrc];
		if (m_bXLS <= 1) GetString(memo, m_rs, fSrc);
		if (!rMemo.IsEmpty()) {
			if (!GetMemoFld(buf, rMemo)) iRet = -rMemo.GetLength();
			else len = 10;
		}
		break;
	}
	case 'L':
	{
		GetDbBoolStr(buf, fSrc);
		len = *buf ? 1 : 0;
		break;
	}

	case 'D':
	{
		GetDbDateStr(buf, fSrc);
		len = *buf ? 1 : 0;
		break;
	}
	default: return 0;
	}
	if (len > 0) {
		StoreText(f, buf);
	}
	return iRet;
}

int CXlsExportDlg::GetCoordinates(double *pLat, double *pLon, double *pEast, double *pNorth, int *pZone)
{
	bool bDefX = false, bDefY = false;
	double y = GetDbDouble(m_psd->v_srcIdx[m_psd->iFldY]);
	if (!y) {
		bDefY = true;
		*pLat = m_psd->latDflt;
	}
	else if (!m_psd->iTypXY) {
		if (bDM) y = getDM(y);
		else if (y > 90.0) y = getDeg(y);
	}
	double x = GetDbDouble(m_psd->v_srcIdx[m_psd->iFldX]);
	if (!x) {
		bDefX = true;
		*pLon = m_psd->lonDflt;
	}
	else if (!m_psd->iTypXY) {
		if (bDM) x = getDM(x);
		else if (x > 180.0) x = -getDeg(x);
	}

	int e = 0;

	if (bDefX != bDefY) {
		int dec = m_psd->iTypXY ? 2 : 6;
		WriteLog("Partial coordinate set: %s = %.*f, %s = %.*f",
			m_psd->v_fldDef[m_psd->iFldX].F_Nam, dec, x, m_psd->v_fldDef[m_psd->iFldY].F_Nam, dec, y);
		*pLat = m_psd->latErr;
		*pLon = m_psd->lonErr;
		e = -1;
		goto _fix;
	}

	if (bDefX) {
		*pLat = m_psd->latDflt;
		*pLon = m_psd->lonDflt;
		*pZone = 0;
		geo_LatLon2UTM(*pLat, *pLon, pZone, pEast, pNorth, m_iDatum);
		return 1; //default location acceptable if both coordinates missing
	}

	*pZone = m_psd->iZoneDflt;

	if (m_bLatLon) {
		//creating lat/long shapefile -- we can either be reading Lat/Long,
		//in which case zone is always calculated, or reading UTM, in which case
		//zone is either initialized or read from source.

		//This condition tested when shpdef was processed --
		ASSERT(!m_psd->iTypXY || *pZone || m_psd->iFldZone >= 0 && m_psd->v_srcIdx[m_psd->iFldZone] >= 0);

		if (m_psd->iTypXY && !*pZone) {
			*pZone = GetDbLong(m_psd->v_srcIdx[m_psd->iFldZone]);
			if (!*pZone || *pZone<m_psd->zoneMin || *pZone>m_psd->zoneMax) {
				*pLat = m_psd->latErr;
				*pLon = m_psd->lonErr;
				WriteLog("Bad or missing UTM zone (%d): %.2fE, %.2fN", *pZone, x, y);
				e = -1;
				goto _fix;
			}
		}
	}

	if (m_psd->iTypXY) {
		//x,y are utm coordinates read from source --

		geo_UTM2LatLon(*pZone, x, y, pLat, pLon, m_iDatum);
		if (*pLat<m_psd->latMin || *pLat>m_psd->latMax || *pLon<m_psd->lonMin || *pLon>m_psd->lonMax) {
			*pLat = m_psd->latErr;
			*pLon = m_psd->lonErr;
			WriteLog("Location out of range: % .2fE, %.2fN, %u%c", x, y, abs(*pZone), (*pZone > 0) ? 'N' : 'S');
			e = -2;
		}
		else {
			*pEast = x;
			*pNorth = y;
		}
	}
	else {
		if (y<m_psd->latMin || y>m_psd->latMax || x<m_psd->lonMin || x>m_psd->lonMax) {
			*pLat = m_psd->latErr;
			*pLon = m_psd->lonErr;
			WriteLog("Location out of range: %.6f %.6f", y, x);
			e = -2;
		}
		else {
			if (!m_bLatLon && !*pZone) {
				ASSERT(!m_psd->iZoneDflt);
			}
			*pLat = y;
			*pLon = x;
			geo_LatLon2UTM(y, x, pZone, pEast, pNorth, m_iDatum);
			if (!m_bLatLon && !m_psd->iZoneDflt) {
				m_psd->iZoneDflt = *pZone;
			}
		}
	}

_fix:
	if (e) {
		if (m_bLatLon) *pZone = 0;
		geo_LatLon2UTM(*pLat, *pLon, pZone, pEast, pNorth, m_iDatum);
		if (!m_bLatLon && !m_psd->iZoneDflt) m_psd->iZoneDflt = *pZone;
	}

	return e;
}

static bool _check_sep(LPCSTR p, LPCSTR pSep)
{
	int n = strlen(pSep);
	for (int i = 0; i < 3; i++, p += n)
		if (!(p = strstr(p, pSep))) return false;
	pFldSep = pSep;
	nSep = n;
	return true;
}

static int _getFieldStructure(V_CSTRING &v_cs, LPCSTR p)
{
	if (!_check_sep(p, "\",\"") && !_check_sep(p, "\"|\"") && !_check_sep(p, "\"\t\"") &&
		!_check_sep(p, ",") && !_check_sep(p, "\t") && !_check_sep(p, "|")) return 0;

	if (nSep != 1) {
		if (*p++ != '"') return -1; //skip past first quote
	}
	LPCSTR p0 = p;
	for (; p = strstr(p, pFldSep); p += nSep, p0 = p) {
		v_cs.push_back(CString(p0, p - p0));
	}
	if (nSep == 1) {
		if (*p0)	v_cs.push_back(CString(p0));
	}
	else {
		int i = strlen(p0);
		if (!i || p0[--i] != '"') return -1;
		v_cs.push_back(CString(p0, i));
	}
	return v_cs.size();
}

static int _stopMsg(LPCSTR p)
{
	CString msg;
	int mb = MB_ICONINFORMATION;
	msg.Format("Export aborted at record %u (line %u): %s.", numRecs + 1, xlsline, p);
	if (numRecs) {
		mb |= MB_YESNO;
		msg += "\n\nDo you want to save the partially written shapefile?";
	}
	int i = MsgInfo(NULL, msg, NULL, mb);
	if (numRecs && i != IDOK) numRecs = 0;
	return -1;
}

static int _limitMsg(int f)
{
	CString s;
	s.Format("Data size for field %u exceeds allowed limit of 65535 bytes", f + 1);
	return _stopMsg(s);
}

static int _sepCount(LPCSTR p)
{
	int n = 0;
	while (*p && (p = strstr(p, pFldSep))) {
		n++;
		p += nSep;
	}
	return n;
}

int CXlsExportDlg::GetCsvRecord()
{
	CString s, s0;
	char cSep;
	int f = 0, maxf = m_psd->numDbFlds - 1;
	if (!m_csvfile.ReadString(s)) return 0;
	xlsline++;
#ifdef _DEBUG
	//if(xlsline==5980) {
		//int jj=0;
	//}
#endif
	LPCSTR p = s;
	if (nSep != 1) {
		cSep = pFldSep[1];
		if (*p++ != '"') {
			return _stopMsg("A quote (\") must precede the first field value");
		}
		while (true) {
			//field f data starts at p and ends at pFldSep, or at quote+eol if f==maxf --
			int fOff = p - (LPCSTR)s;
			while ((f < maxf && !(p = strstr(p, pFldSep))) || (f == maxf && (!(p = strchr(p, '"')) || p[1]))) {
				if (!m_csvfile.ReadString(s0)) {
					s.Format("File ended before data for field %u was read", f + 1);
					return _stopMsg(s);
				}
				xlsline++;
				s += "\r\n";
				int len = strlen(s);
				//check for max field length?
				if (len >= 64 * 1024) {
					return _limitMsg(f);
				}
				s += s0;
				p = (LPCSTR)s + len;
			}
			//p points at field terminator, possibly null
			s0.SetString((LPCSTR)s + fOff, p - (LPCSTR)s - fOff);
			s0.Trim();
			m_vcsv[f] = s0;
			if (f == maxf) {
				ASSERT(*p == '"' && !p[1]);
				break;
			}
			else {
				ASSERT(*p == '"' && p[1] == cSep);
			}
			p += nSep;
			f++;
		}
		ASSERT(_sepCount(s) == maxf);
	}
	else {
		// CSV Rule? - A field value can contain commas and/or quotes only if the entire
		// field is enclosed in a pair of quotes. (Also, any internal quote must be escaped
		// with another quote.) Therefore, if if a field  begins with a quote, it will end
		// with an unescaped quote followed by a comma or null. If a field value doesn't
		// begin with a quote, it ends with a comma or null. At least with an Excel CSV
		// export, a field value having no quotes or commas will not be encosed in quotes.

		cSep = *pFldSep;
		while (true) {
			//at start of line or at first char past separator
			LPCSTR p0 = p;
			if (*p == '"') {
				//The field must contain escaped quotes ("") and/or commas
				while (p = strchr(p + 1, '"')) {
					p++; //char after quote
					if (!*p || *p == cSep) break;
					if (*p != '"') {
						s.Format("Field %u - A quote (\") inside a quoted field must be escaped with a quote", f + 1);
						return _stopMsg(s);
					}
				}
				if (!p) {
					s.Format("Field %u is missing an ending quote (\")", f + 1);
					return _stopMsg(s);
				}
				s0.SetString(p0 + 1, p - p0 - 2);
				s0.Replace("\"\"", "\"");
			}
			else {
				p = strchr(p, cSep);
				if (!p) p = p0 + strlen(p0);
				s0.SetString(p0, p - p0);
			}
			//p is at separator (or null) past possibly quoted field value
			s0.Trim();
			m_vcsv[f] = s0;
			if (f == maxf) {
				if (*p)
					return _stopMsg("Data for too many fields encountered");
				break;
			}
			if (!*++p) {
				//blank fill remaining fields -
				for (++f; f <= maxf; f++) m_vcsv[f].Empty();
				break;
			}
			f++;
		}
	}
	return 1;
}

void CXlsExportDlg::InitFldKey()
{
	int f = m_psd->v_srcIdx[m_psd->iFldKey];
	DBF_FLDDEF &fd = m_psd->v_fldDef[m_psd->iFldKey];
	if (m_bXLS > 1) {
		m_fldKeyVal = m_vcsv[f];
	}
	else {
		if (fd.F_Typ == 'N') {
			__int64 n = GetInt64(m_rs, f);
			m_fldKeyVal.Format("%*I64d", fd.F_Len, n);
		}
		else {
			GetString(m_fldKeyVal, m_rs, f);
		}
	}
	if (m_fldKeyVal.GetLength() > fd.F_Len)
		m_fldKeyVal.Truncate(fd.F_Len);
}

static LPCSTR GetProgramPath(CString &path, LPCSTR pProgram)
{
	TCHAR pf[MAX_PATH];
	if (SHGetSpecialFolderPath(0, pf, CSIDL_PROGRAM_FILES, FALSE)) {
		strcat(pf, pProgram);
		if (!_access(pf, 0)) {
			path = pf;
			return (LPCSTR)path;
		}
	}
	return NULL;
}

void CXlsExportDlg::SetDataLen(int f)
{
	DBF_FLDDEF &fd = m_psd->v_fldDef[f];
	int fSrc = m_psd->v_srcIdx[f];
	UINT dlen;
	if (fSrc < 0) {
		dlen = m_psd->v_srcDflt[f].GetLength();
	}
	else {
		char buf[256];
		dlen = GetDbText(buf, fSrc, 254); //dlen = trimmed data length
		if (!dlen && !(dlen = m_psd->v_srcDflt[f].GetLength())) dlen = 1;
	}
	if (dlen > 254) dlen = 254;

	if ((WORD)dlen > fd.F_Off) {
		fd.F_Off = (WORD)dlen;
	}
}

bool CXlsExportDlg::AdjustFldLen()
{
	for (int f = 0; f < m_psd->numFlds; f++) {
		m_psd->v_fldDef[f].F_Off = 0;
	}

	int iNotEOF;
	try {
		if (m_bXLS > 1) {
			m_vcsv.assign(m_psd->numDbFlds, CString());
			iNotEOF = GetCsvRecord();
		}
		else iNotEOF = !m_rs.IsEOF();

		while (iNotEOF > 0) {
			numRecs++;
			for (int f = 0; f < m_psd->numFlds; f++) {
				DBF_FLDDEF &fd = m_psd->v_fldDef[f];
				int n = m_psd->GetLocFldTyp(fd.F_Nam) - 1;
				if (fd.F_Typ != 'C' || n >= LF_LAT && n <= LF_CREATED) {
					continue;
				}
				SetDataLen(f);
			}

			if (m_bXLS <= 1) {
				m_rs.MoveNext();
				iNotEOF = !m_rs.IsEOF();
				if (!iNotEOF) xlsline++;
			}
			else {
				iNotEOF = GetCsvRecord();
			}
		}

		if (m_bXLS > 1) {
			if (iNotEOF < 0) return false;
			CString s;
			m_csvfile.Seek(0, CFile::begin);
			m_csvfile.ReadString(s);
			xlsline = 1; //file lines read
		}
		else {
			m_rs.MoveFirst();
		}
		numRecs = 0; //records read

		for (int f = 0; f < m_psd->numFlds; f++) {
			DBF_FLDDEF &fd = m_psd->v_fldDef[f];
			if (fd.F_Off > 0) fd.F_Len = (BYTE)fd.F_Off;
		}
	}
	catch (CDaoException* pe)
	{
		AfxMessageBox(pe->m_pErrorInfo->m_strDescription, MB_ICONEXCLAMATION);
		pe->Delete();
		return false;
	}
	return true;
}

void CXlsExportDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_PATHNAME, m_sourcePath);
	DDV_MaxChars(pDX, m_sourcePath, _MAX_PATH);
	DDX_Text(pDX, IDC_TABLENAME, m_tableName);
	DDV_MaxChars(pDX, m_tableName, 50);
	DDX_Text(pDX, IDC_TMPNAME, m_shpdefPath);
	DDV_MaxChars(pDX, m_shpdefPath, _MAX_PATH);
	DDX_Text(pDX, IDC_SHPNAME, m_shpPath);
	DDV_MaxChars(pDX, m_shpPath, _MAX_PATH);
	DDX_Radio(pDX, IDC_UTM, m_bLatLon);
	DDX_Check(pDX, IDC_MIN_CLEN, m_bMinFldLen);

	if (pDX->m_bSaveAndValidate) {
		CWaitCursor wait;

		m_sourcePath.Trim();
		if (m_sourcePath.IsEmpty()) {
			pDX->m_idLastControl = IDC_PATHNAME;
			AfxMessageBox("A source pathname is required.");
			pDX->Fail();
			return;
		}

		::GetFullPathName(m_sourcePath, MAX_PATH, m_szSourcePath, &m_pSourceName);

		LPSTR pExt = trx_Stpext(m_szSourcePath);
		m_bXLS = !_stricmp(pExt, ".xls") || !_stricmp(pExt, ".xlsx");

		if (!m_bXLS && _stricmp(pExt, ".mdb")) {
			if (!_stricmp(pExt, ".csv")) m_bXLS = 2;
			else if (!_stricmp(pExt, ".txt")) m_bXLS = 3;
			else {
				pDX->m_idLastControl = IDC_PATHNAME;
				AfxMessageBox("Source extension must be one of .xls, .mdb, .csv. and .txt.");
				pDX->Fail();
				return;
			}
		}

		if (_access(m_szSourcePath, 4)) {
			pDX->m_idLastControl = IDC_PATHNAME;
			CMsgBox("Source file %s is absent or inaccessible.", m_pSourceName);
			pDX->Fail();
			return;
		}

		m_tableName.Trim();
		if (m_bXLS <= 1 && m_tableName.IsEmpty()) {
			pDX->m_idLastControl = IDC_TABLENAME;
			AfxMessageBox("A table or sheet name is required.");
			pDX->Fail();
			return;
		}

		m_shpdefPath.Trim();
		if (m_shpdefPath.IsEmpty() || stricmp(trx_Stpext(m_shpdefPath), ".shpdef")) {
			pDX->m_idLastControl = IDC_TMPNAME;
			AfxMessageBox("A template file with extension .shpdef must be specified.");
			pDX->Fail();
			return;
		}

		::GetFullPathName(m_shpdefPath, MAX_PATH, m_szShpdefPath, &m_pShpdefName);

		if (_access(m_szShpdefPath, 4)) {
			pDX->m_idLastControl = IDC_TMPNAME;
			CMsgBox("Template file %s is absent or inaccessible.", m_pShpdefName);
			pDX->Fail();
			return;
		}

		m_shpPath.Trim();
		if (m_shpPath.IsEmpty()) {
			pDX->m_idLastControl = IDC_SHPNAME;
			AfxMessageBox("An output shapefile pathname is required.");
			pDX->Fail();
			return;
		}

		::GetFullPathName(m_shpPath, MAX_PATH, m_szShpPath, &m_pShpName);
		pExt = trx_Stpext(m_szShpPath);
		strcpy(pExt, ".shp");
		m_nPathLen = pExt - m_szShpPath;
		m_pathBuf = m_szShpPath;
		FixShpPath(".dbf");

		if (!_access(m_pathBuf, 0)) {
			//file exists --
			bool ok = false;
			//dbf exists
			if (CheckAccess(m_pathBuf, 4)) {
				//not accesible for reaf/write --
				CMsgBox("%s is protected and can't be overwritten.", m_pShpName);
			}
			else if (IDYES == CMsgBox(MB_YESNO, "%s already exists.\nDo you want to overwrite it?", m_pShpName)) {
				ok = true;
			}
			if (!ok) {
				pDX->m_idLastControl = IDC_SHPNAME;
				pDX->Fail();
				return;
			}
		}

		ASSERT(!m_psd);

		xlsline = 0;

		if (m_bXLS > 1) {
			if (!m_csvfile.Open(m_szSourcePath, CFile::modeRead)) {
				CMsgBox("File %s missing or can't be opened.", m_pSourceName);
				pDX->m_idLastControl = IDC_PATHNAME;
				pDX->Fail();
				return;
			}

			m_psd = new CShpDef();

			//determine field names and separator --
			CString s;
			if (!m_csvfile.ReadString(s) || (m_psd->numDbFlds = _getFieldStructure(m_psd->v_srcNames, s)) <= 0) {
				m_csvfile.Close();
				s = m_psd->numDbFlds ? "Unrecognized format for field names" : "Fewer than 3 field names recognized";
				CMsgBox("%s in first line of %s.", (LPCSTR)s, m_pSourceName);
				delete m_psd;
				m_psd = NULL;
				pDX->m_idLastControl = IDC_PATHNAME;
				pDX->Fail();
				return;
			}
			xlsline++; //file lines read

			SaveOptions();
			if (!m_psd->Process(NULL, m_szShpdefPath, m_bLatLon)) {
				delete m_psd;
				m_psd = NULL;
				m_csvfile.Close();
				pDX->m_idLastControl = IDC_TMPNAME;
				pDX->Fail();
				return;
			}

		}
		else {
			try
			{
				m_database.Open(m_szSourcePath, TRUE, TRUE, m_bXLS ? "Excel 5.0" : "");
				ASSERT(m_database.IsOpen());
				CString table;
				table.Format("[%s%s]", (LPCSTR)m_tableName, m_bXLS ? "$" : "");
				m_rs.m_pDatabase = &m_database;
				m_rs.Open(dbOpenTable, table, dbReadOnly);

				m_psd = new CShpDef();
				m_psd->numDbFlds = m_rs.GetFieldCount();
			}
			catch (CDaoException* pe)
			{
				//AfxMessageBox(pe->m_pErrorInfo->m_strDescription, MB_ICONEXCLAMATION);
				pe->Delete();
				CString s;
				BOOL bOpen = m_database.IsOpen();
				if (bOpen) {
					s.Format("Unable to open table \"%s\" in %s.", (LPCSTR)m_tableName, m_pSourceName);
					if (m_rs.IsOpen()) m_rs.Close();
					m_database.Close();
				}
				else s.Format("Unable to access source: %s", m_pSourceName);
				if (m_psd) {
					delete m_psd;
					m_psd = NULL;
				}
				AfxMessageBox(s);
				pDX->m_idLastControl = IDC_PATHNAME;
				pDX->Fail();
				return;
			}

			SaveOptions();
			if (!m_psd->Process(&m_rs, m_szShpdefPath, m_bLatLon)) {
				CloseDB();
				delete m_psd;
				m_psd = NULL;
				pDX->m_idLastControl = IDC_TMPNAME;
				pDX->Fail();
				return;
			}
		}

		numRecs = 0;

		if (m_bMinFldLen && !AdjustFldLen()) {
			CloseDB();
			delete m_psd;
			m_psd = NULL;
			pDX->m_idLastControl = IDC_PATHNAME;
			pDX->Fail();
			return;
		}

		if (InitShapefile()) {
			//shouldn't happen since DBF is available for writing --
			CloseDB();
			delete m_psd; //done in constructor
			m_psd = NULL;
			CMsgBox("Error initializing %s.", m_pShpName);
			EndDialog(IDCANCEL);
			return;
		}

		{
			LPCSTR pd = (m_psd->iFldDatum >= 0) ? m_psd->v_srcDflt[m_psd->iFldDatum] : "WGS84";
			if (!strcmp(pd, "NAD83")) m_iDatum = geo_NAD83_datum;
			else if (!strcmp(pd, "NAD27")) m_iDatum = geo_NAD27_datum;
			else m_iDatum = geo_WGS84_datum;
		}

		UINT nLocErrors = 0, nLocMissing = 0, nLocErrRange = 0, nTruncated = 0;
		int iNotEOF = 0;

		try {
			if (m_bXLS > 1) {
				m_vcsv.assign(m_psd->numDbFlds, CString());
				iNotEOF = GetCsvRecord();
			}
			else iNotEOF = !m_rs.IsEOF();

			while (iNotEOF > 0) {

				if (m_psd->iFldKey >= 0) InitFldKey();

				double lat, lon, east, north;
				int zone = 0;
				int e = GetCoordinates(&lat, &lon, &east, &north, &zone);

				if (e) {
					if (e == 1) nLocMissing++;
					else if (e == -1) nLocErrors++;
					else if (e == -2) nLocErrRange++;
				}

				if (m_bLatLon) {
					e = WriteShpRec(lon, lat);
				}
				else {
					e = WriteShpRec(east, north);
				}

				if (e) {
					CString msg;
					msg.Format("Aborted: Error writing shp record %u.", numRecs + 1);
					WriteLog(msg);
					AfxMessageBox(msg);
					break;
				}

				for (int f = 0; f < m_psd->numFlds; f++) {
					DBF_FLDDEF &fd = m_psd->v_fldDef[f];
					int n = m_psd->GetLocFldTyp(fd.F_Nam) - 1;
					if (n >= LF_LAT && n <= LF_ZONE) {
						//Store already-computed location data --
						switch (n) {
						case LF_LAT: StoreDouble(fd, f, lat);
							break;
						case LF_LON: StoreDouble(fd, f, lon);
							break;
						case LF_EAST: StoreDouble(fd, f, east);
							break;
						case LF_NORTH: StoreDouble(fd, f, north);
							break;
						case LF_ZONE:
						{
							CString s;
							s.Format("%02u%c", (zone < 0) ? -zone : zone, (zone < 0) ? 'S' : 'N');
							StoreText(f, (LPCSTR)s);
							break;
						}
						}
						continue;
					}

					//normal field --

					int len = StoreFldData(f); //suuports only N, C, M, and L for now
					if (len < 0) {
						CString msg;
						if (fd.F_Typ == 'C') {
							msg.Format("Character data in field %s (length %u) truncated to %u.", fd.F_Nam, fd.F_Len - len, fd.F_Len);
						}
						else if (fd.F_Typ == 'M') {
							msg.Format("Field %s memo (length %u) couldn't be stored.", fd.F_Nam, fd.F_Len);
						}
						else {
							msg.Format("Numeric data in field %s truncated and may be corrupt.", fd.F_Nam);
						}
						WriteLog(msg);
						nTruncated++;
					}
				}

				numRecs++;
				if (m_bXLS <= 1) {
					m_rs.MoveNext();
					iNotEOF = !m_rs.IsEOF();
					if (!iNotEOF) xlsline++;
				}
				else {
					iNotEOF = GetCsvRecord();
				}
			}

			if (iNotEOF < 0) {
				ASSERT(m_bXLS > 1);
			}
		}
		catch (CDaoException* pe)
		{
			AfxMessageBox(pe->m_pErrorInfo->m_strDescription, MB_ICONEXCLAMATION);
			pe->Delete();
		}

		CloseDB();

		if (numRecs) {
			if (CloseFiles())
				AfxMessageBox("Error closing shapefile");
			else {
				CString msg, wmpath;

				msg.Format("Shapefile with %u records created.\nLocations missing: %u, Locations out-of-range: %u,\nPartial (bad) locations: %u, Truncated fields: %u",
					numRecs, nLocMissing, nLocErrRange, nLocErrors, nTruncated);

				if (m_nLogMsgs) {
					wmpath = msg;
					wmpath.Replace("\n", "\r\n");
					m_fbLog.Write("\r\n", 2);
					m_fbLog.Write((LPCSTR)wmpath, wmpath.GetLength());
					m_fbLog.Write("\r\n", 2);
					wmpath.Format("\n\nNOTE: %u diagnostic messages were written to %s.",
						m_nLogMsgs, trx_Stpnam(csLogPath));
					msg += wmpath;
				}

				if (GetProgramPath(wmpath, "\\WallsMap\\WallsMap.exe")) {
					MessageBeep(MB_ICONINFORMATION/*MB_OK*/);
					int i = MsgYesNoCancelDlg(0, msg, "Table2Shp - Successful Export", "Open Shapefile", "Exit", NULL);
					if (i == 6) {
						CString args;
						args.Format("\"%s\"", FixShpPath(".shp"));
						if ((int)ShellExecute(NULL, "OPEN", wmpath, args, NULL, SW_NORMAL) <= 32) {
							CMsgBox("Unable to launch %s.", trx_Stpnam(wmpath));
						}
					}
				}
				else AfxMessageBox(msg);
			}
		}
		else {
			DeleteFiles();
			if (iNotEOF >= 0) AfxMessageBox("No shapefile created.");
		}

		delete m_psd;
		m_psd = NULL;

		if (m_fbLog.IsOpen()) {
			m_fbLog.Close();
			OpenFileAsText(csLogPath);
		}
	}
}

// CXlsExportDlg message handlers

BOOL CXlsExportDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon

	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CXlsExportDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CXlsExportDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CXlsExportDlg::OnBnClickedBrowse()
{
	CString strFilter;
	if (!AddFilter(strFilter, IDS_ALL_FILES) || !AddFilter(strFilter, IDS_XLS_FILES) || !AddFilter(strFilter, IDS_MDB_FILES) || !AddFilter(strFilter, IDS_CSV_FILES)) return;
	CString s;
	if (DoPromptPathName(s, OFN_FILEMUSTEXIST, 3, strFilter, TRUE, IDS_XLS_EXPORT, ".xls")) {
		LPCSTR p = trx_Stpext(s);
		if (stricmp(p, ".xls") && stricmp(p, ".xlsx") && stricmp(p, ".csv") && stricmp(p, ".mdb") && stricmp(p, ".txt")) {
			s.Truncate(p - (LPCSTR)s);
			s += ".xls";
			p = trx_Stpext(s);
		}
		char typ = _toupper(p[1]);
		GetDlgItem(IDC_PATHNAME)->SetWindowText(s);
		if (_toupper(p[1]) != 'M') GetDlgItem(IDC_TABLENAME)->SetWindowText((typ == 'X') ? "Sheet1" : "");

		GetDlgItem(IDC_TMPNAME)->GetWindowText(strFilter);
		strFilter.Trim();
		if (strFilter.IsEmpty()) {
			s.Truncate(trx_Stpext(s) - (LPCSTR)s);
			s += ".shpdef";
			GetDlgItem(IDC_TMPNAME)->SetWindowText(s);
			GetDlgItem(IDC_SHPNAME)->GetWindowText(strFilter);
			strFilter.Trim();
			if (strFilter.IsEmpty()) {
				s.Truncate(strlen(s) - 3);
				GetDlgItem(IDC_SHPNAME)->SetWindowText(s);
			}
		}
	}
}


void CXlsExportDlg::OnBnClickedBrowseShpdef()
{
	CString strFilter;
	if (!AddFilter(strFilter, IDS_SHPDEF_FILES)) return;
	CString s;
	if (DoPromptPathName(s, OFN_FILEMUSTEXIST, 1, strFilter, TRUE, IDS_SEL_SHPDEF, ".shpdef")) {
		LPCSTR p = trx_Stpext(s);
		if (stricmp(p, ".shpdef")) {
			s.Truncate(p - (LPCSTR)s);
			s += ".shpdef";
		}
		GetDlgItem(IDC_TMPNAME)->SetWindowText(s);
		GetDlgItem(IDC_SHPNAME)->GetWindowText(strFilter);
		strFilter.Trim();
		if (strFilter.IsEmpty()) {
			s.Truncate(trx_Stpext(s) - (LPCSTR)s);
			s += ".shp";
			GetDlgItem(IDC_SHPNAME)->SetWindowText(s);
		}
	}
}

void CXlsExportDlg::OnBnClickedBrowseShp()
{
	CString strFilter;
	if (!AddFilter(strFilter, IDS_SHP_FILES)) return;
	CString s;
	if (DoPromptPathName(s, OFN_HIDEREADONLY, 1, strFilter, TRUE, IDS_SHP_FILES, ".shp")) {
		LPCSTR p = trx_Stpext(s);
		if (stricmp(p, ".shp")) {
			s.Truncate(p - (LPCSTR)s);
			s += ".shp";
		}
		GetDlgItem(IDC_SHPNAME)->SetWindowText(s);
	}
}

void CXlsExportDlg::OnBnClickedEditTmp()
{
	GetDlgItem(IDC_TMPNAME)->GetWindowText(m_shpdefPath);
	m_shpdefPath.Trim();
	if (m_shpdefPath.IsEmpty() || stricmp(trx_Stpext(m_shpdefPath), ".shpdef")) {
		AfxMessageBox("A template file with extension .shpdef hasn't been specified.");
		return;
	}

	::GetFullPathName(m_shpdefPath, MAX_PATH, m_szShpdefPath, &m_pShpdefName);

	if (_access(m_szShpdefPath, 4)) {
		CMsgBox("%s\n\nFile is absent or inaccessible.", m_szShpdefPath);
		return;
	}

	OpenFileAsText(m_szShpdefPath);
}

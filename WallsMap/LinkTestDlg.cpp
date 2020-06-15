// LinkTestDlg.cpp : implementation file
//

#include "stdafx.h"
#include "WallsMap.h"
#include "WallsMapDoc.h"
#include "ImageViewDlg.h"
#include "LinkTestDlg.h"

// CLinkTestDlg dialog

IMPLEMENT_DYNAMIC(CLinkTestDlg, CDialog)

CLinkTestDlg::CLinkTestDlg(CDBGridDlg *pDBGridDlg, UINT nFld, CWnd* pParent /*=NULL*/)
	: CDialog(CLinkTestDlg::IDD, pParent)
	, m_pDBGridDlg(pDBGridDlg)
	, m_pShp(pDBGridDlg->m_pShp)
	, m_pdb(m_pShp->m_pdb)
	, m_nFld(nFld)
	, m_bReplaceAll(false)
	, m_bInit(false)
{
	ASSERT(m_pdb->FldTyp(nFld) == 'M');
}

CLinkTestDlg::~CLinkTestDlg()
{
}

void CLinkTestDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//DDX_Text(pDX, IDC_REPLACEMENT, m_csReplacement);
	//DDV_MaxChars(pDX, m_csReplacement, 260);
	DDX_Control(pDX, IDC_REPLACEMENT, m_ceReplacement);
	DDX_Control(pDX, IDC_BROKEN_LINK, m_ceBrokenLink);
}

void CLinkTestDlg::SetCounts()
{
	CString s;
	if (m_nTotal) s.Format("Fixed %u of %u%s", m_nFixed, m_nTotal, m_pShp->IsEditable() ? "" : "  (Layer not editable)");
	GetDlgItem(IDC_ST_COUNT)->SetWindowText(s);
}

BEGIN_MESSAGE_MAP(CLinkTestDlg, CDialog)
	ON_WM_CTLCOLOR()
	ON_BN_CLICKED(IDC_SKIPNEXT, OnFindNext)
	ON_BN_CLICKED(IDC_REPLACE, OnBnClickedReplace)
	ON_BN_CLICKED(IDC_REPLACEALL, OnBnClickedReplaceAll)
	ON_CBN_SELCHANGE(IDC_LOOKIN, OnSelChangeLookIn)
	ON_BN_CLICKED(IDC_BROWSE, &CLinkTestDlg::OnBnClickedBrowse)
	ON_REGISTERED_MESSAGE(WM_PROPVIEWDOC, OnPropViewDoc)
	ON_EN_CHANGE(IDC_REPLACEMENT, OnEnChangeRepl)
	ON_MESSAGE(WM_COMMANDHELP, OnCommandHelp)
	ON_BN_CLICKED(IDC_CREATE_LOG, &CLinkTestDlg::OnBnClickedCreateLog)
END_MESSAGE_MAP()

void CLinkTestDlg::SetMsgText(LPCSTR format, ...)
{
	char buf[256];
	va_list marker;
	va_start(marker, format);
	_vsnprintf(buf, 256, format, marker); buf[255] = 0;
	va_end(marker);
	GetDlgItem(IDC_ST_MSG)->SetWindowText(buf);
}

BOOL CLinkTestDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	{
		CString sTitle;
		sTitle.Format("Find Broken Image Links in %s", m_pShp->Title());
		SetWindowText(sTitle);
	}

	VERIFY(m_cbLookIn.SubclassDlgItem(IDC_LOOKIN, this)); //subclass "Look in" edit control

	int ix = CB_ERR;
	for (UINT f = 1; f <= m_pdb->NumFlds(); f++) {
		if (m_pdb->FldTyp(f) == 'M') {
			int i = m_cbLookIn.AddString(m_pdb->FldNamPtr(f));
			if (f == m_nFld) ix = i;
		}
	}
	ASSERT(ix != CB_ERR && ix == m_cbLookIn.FindStringExact(0, m_pdb->FldNamPtr(m_nFld)));
	VERIFY(m_cbLookIn.SetCurSel(ix) != CB_ERR);

	InitStartMsg();

	return TRUE;  // return TRUE unless you set the focus to a control
}

void CLinkTestDlg::OnBnClickedReplaceAll()
{
	m_bReplaceAll = true;
	BeginWaitCursor();
	OnBnClickedReplace();
}

void CLinkTestDlg::OnBnClickedReplace()
{
	ASSERT((int)m_lp.nLinkPos > 0 && m_lp.nLinkPos <= m_lp.vBL.size());

	m_lp.vBL[m_lp.nLinkPos - 1].sRepl = m_sReplacement;

	StoreUnmatchedPrefixes(m_sBrokenText, m_sReplacement);

	m_lp.nNew++;
	m_nFixed++;
	OnFindNext();
}

bool CLinkTestDlg::CanAccess(CString &pathRel)
{
	char pathName[MAX_PATH];

	if (m_pShp->GetFullFromRelative(pathName, pathRel)) {
		if (!_access(pathName, 0)) {
			m_pShp->GetRelativePath(pathName, pathName, FALSE); //There *is* a name on the path
			pathRel = pathName; //might be shorter
			return true;
		}
	}
	return false;
}

static bool TrimDir(CString &s, LPSTR pathBuf)
{
	LPSTR p = pathBuf + strlen(pathBuf);
	if (p == pathBuf || *--p != '\\') return NULL;
	*p = 0;
	while (p > pathBuf && p[-1] != '\\') p--;
	s = p;
	*p = 0;
	return !s.IsEmpty() && s[0] != '.';
}

bool CLinkTestDlg::FindReplacement()
{
	/* algorithm
	1) Run through the vector of previously replaced bad path prefixes (s0) in reverse
	and find the longest one that matches a leading portion of sBrok. During the
	search, for each match found, append the remaining tail portion of sBrok to the
	correspopnding replacement prefix (s1) and test access, returning true
	if the file exists.

	2) If the existence tests all fail, do a deep folder search for the
	unprefixed file name in sBrok, starting in s1 and continuing upward
	through its chain of named parent directories.
	*/

	char pathBuf[MAX_PATH];
	*pathBuf = NULL;
	LPCSTR pName = trx_Stpnam(m_sBrokenText);

	if (m_vcs2.size()) {
		const CSTR2 *pcsmax = NULL;
		int blen = pName - (LPCSTR)m_sBrokenText;
		int rmax = 0;
		for (VEC_CSTR2::const_reverse_iterator rit = m_vcs2.rbegin(); rit != m_vcs2.rend(); rit++) {
			int rlen = rit->s0.GetLength();
			if (rlen <= blen && !_memicmp((LPCSTR)rit->s0, (LPCSTR)m_sBrokenText, rlen)) {
				if (rlen > rmax) {
					rmax = rlen;
					pcsmax = &*rit;
				}
				m_sReplacement = rit->s1;
				m_sReplacement += (LPCSTR)m_sBrokenText + rlen;
				if (CanAccess(m_sReplacement)) {
					return true;
				}
			}
		}
		if (pcsmax)
			strcpy(pathBuf, pcsmax->s0); //look in existing parent folders of a previously-examined bad link
	}

	if (!*pathBuf) {
		GetReplacementText(m_sReplacement); //look in parent folders of a browsed-to location
		strcpy(pathBuf, m_sReplacement);
		*trx_Stpnam(pathBuf) = 0;
	}
	if (*pathBuf) {
		bool bRet = false;
		//PVOID pvold; 
		//if(::Wow64DisableWow64FsRedirection(&pvold)) { --doesn't work with XP!
		if (MakeFileDirectoryCurrent(m_pShp->PathName())) {
			CString sSkipDir;
			do {
				if (SearchDirRecursively(pathBuf, pName, (LPCSTR)sSkipDir)) {
					//Make a proper relative name --
					sSkipDir = pathBuf;
					if (!m_pShp->GetFullFromRelative(pathBuf, sSkipDir)) break;
					if (!m_pShp->GetRelativePath(pathBuf, pathBuf, FALSE)) break;
					m_sReplacement = pathBuf;
					m_bReplaceAll = false; //stop automatic replacement
					bRet = true;
					break;
				}
			} while (TrimDir(sSkipDir, pathBuf));
		}
		else {
			ASSERT(0);
		}
		//VERIFY(Wow64RevertWow64FsRedirection(pvold));
	//}
		return bRet;
	}
	return false;
}

void CLinkTestDlg::OnFindNext()
{
	if (m_bScanAgain) {
		ASSERT(m_nFixed < m_nTotal && !GetDlgItem(IDC_BROWSE)->IsWindowEnabled());
		SetText(IDC_SKIPNEXT, "Skip to Next");
		InitStartMsg();
		return;
	}

	if (m_lp.nLinkPos == m_lp.vBL.size()) {
		if (m_lp.nNew) {
			//Update database --
			VERIFY(Search(2));
			m_lp.nNew = 0;
		}

		//Find next record with bad links, filling m_lp.vBL with links and offsets --
		m_lp.nRow++;
		if (!Search(1)) {
			//No more bad links found --
			if (m_bReplaceAll) {
				m_bReplaceAll = false;
				EndWaitCursor();
			}
			SetBrokenText("");
			SetMsgText("No more bad links were found in %s.", m_pdb->FldNamPtr(m_nFld));
			SetCounts();
			if (m_nFixed < m_nTotal) {
				m_bScanAgain = true;
				SetText(IDC_SKIPNEXT, "Scan Again");
			}
			else Enable(IDC_SKIPNEXT, 0);
			Enable(IDC_REPLACE, 0);
			Enable(IDC_REPLACEALL, 0);
			Enable(IDC_REPLACEMENT, 0);
			Enable(IDC_CREATE_LOG, 0);
			Enable(IDOK, 0);
			Enable(IDC_BROWSE, 0);
			return;
		}
		ASSERT(m_lp.nLinkPos == 0 && m_lp.nNew == 0);
	}

	//handle the next bad link --
	m_nCount++;

	ASSERT(m_lp.vBL[m_lp.nLinkPos].sRepl.IsEmpty());

	m_sBrokenText = m_lp.vBL[m_lp.nLinkPos++].sBrok;

	if (m_pShp->IsEditable() && FindReplacement()) {
		if (m_bReplaceAll) {
			PostMessage(WM_PROPVIEWDOC, IDC_REPLACE);  //invoke OnBnClickedReplace();
			return;
		}
		Enable(IDC_REPLACE);
		Enable(IDC_REPLACEALL);
	}
	else {
		if (m_bReplaceAll) {
			m_bReplaceAll = false;
			EndWaitCursor();
		}
		m_sReplacement.Empty();
		Enable(IDC_REPLACE, 0);
		Enable(IDC_REPLACEALL, 0);
	}
	SetBrokenText(m_sBrokenText); //normally an incorrect relative path
	SetReplacementText(m_sReplacement);
	Enable(IDC_SKIPNEXT); //enable skip
	Enable(IDC_CREATE_LOG);
	Enable(IDOK);
	if (m_pShp->IsEditable()) {
		Enable(IDC_REPLACEMENT);
		Enable(IDC_BROWSE);
	}
	VERIFY(!m_pdb->Go(m_lp.nRec));
	SetLabelMsg();
	SetCounts();
}

void CLinkTestDlg::SetLabelMsg()
{
	CString s;
	m_pdb->GetTrimmedFldStr(s, m_pShp->m_nLblFld);
	SetMsgText("Link %u, Record %u, %s: %s", m_nCount, m_lp.nRec, m_pdb->FldNamPtr(m_pShp->m_nLblFld), (LPCSTR)s);
}

void CLinkTestDlg::InitStartMsg()
{
	m_bReplaceAll = m_bScanAgain = false;
	Enable(IDC_SKIPNEXT, 0);
	Enable(IDC_REPLACE, 0);
	Enable(IDC_REPLACEALL, 0);
	Enable(IDC_REPLACEMENT, 0);
	Enable(IDC_CREATE_LOG, 0);
	Enable(IDOK, 0);
	Enable(IDC_BROWSE, 0);
	m_nFixed = m_nCount = 0;

	m_lp.nRow = m_lp.nLinkPos = m_lp.nNew = 0;

	m_lp.nFld = m_nFld;

	BeginWaitCursor();
	m_nTotal = Search(0); //total broken links
	EndWaitCursor();

	SetCounts();

	if (!m_nTotal) {
		CString s;
		LPCSTR pTitle = m_pdb->FldNamPtr(m_nFld);
		if (!m_lp.nNew)
			s.Format("No image links were found in %s.", pTitle);
		else
			s.Format("No broken links were found in %s. There are %u accessible links in %u records.", pTitle, m_lp.nLinkPos, m_lp.nNew);
		SetMsgText(s);
		SetReplacementText("");
	}
	else {
		m_lp.nRow--;
		PostMessage(WM_PROPVIEWDOC, IDC_SKIPNEXT);  //invoke OnFindNext();
	}
	m_lp.nLinkPos = m_lp.nNew = 0;
}

void CLinkTestDlg::OnSelChangeLookIn()
{
	int f = m_cbLookIn.GetCurSel();
	CString s;
	m_cbLookIn.GetLBText(f, s);
	if (!(f = m_pdb->FldNum(s))) {
		ASSERT(0);
		return;
	}
	if (m_nFld == (UINT)f) return;

	//write pending updates
	if (m_lp.nNew) {
		ASSERT(m_lp.vBL.size() >= m_lp.nLinkPos);
		VERIFY(Search(2));
	}

	m_nFld = f;
	InitStartMsg();
	SetText(IDC_SKIPNEXT, "Skip to Next");
}

void CLinkTestDlg::OnCancel()
{
	if (m_lp.nNew) {
		ASSERT(m_lp.vBL.size() >= m_lp.nLinkPos);
		VERIFY(Search(2));
	}
	CDialog::OnCancel();
}

LRESULT CLinkTestDlg::OnPropViewDoc(WPARAM wParam, LPARAM typ)
{
	if (wParam == IDC_REPLACE) {
		ASSERT(m_bReplaceAll);
		OnBnClickedReplace();
	}
	else if (wParam == IDC_SKIPNEXT) {
		OnFindNext();
	}
	return 0;
}

void CLinkTestDlg::OnEnChangeRepl()
{
	if (!m_bInit) {
		BOOL bFound = 0;
		CString s;
		GetReplacementText(s);
		if (CanAccess(s)) {
			bFound = 1;
			SetReplacementText(m_sReplacement = s);
		}
		Enable(IDC_REPLACE, bFound);
		Enable(IDC_REPLACEALL, bFound);
	}
}

HBRUSH CLinkTestDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
	if (pWnd->GetDlgCtrlID() == IDC_ST_MSG) {
		pDC->SetTextColor(RGB(0x80, 0, 0));
	}
	return hbr;
}

void CLinkTestDlg::OnBnClickedBrowse()
{
	char pathName[MAX_PATH];
	CString s;
	GetReplacementText(s);
	//if(s.IsEmpty()) s=m_pShp->PathName();
	if (s.IsEmpty()) {
		//GetBrokenText(s);
		s = m_sBrokenText;
	}
	int i;
	while (!s.IsEmpty() && (i = s.ReverseFind('\\')) > 0) {
		s.Truncate(i);
		if (!m_pShp->GetFullFromRelative(pathName, s)) {
			//too many /../ in s can crash ::GetFullPathName() called from dos_FullPath() in dis_io.c --
			break;
		}
		if (!_access(pathName, 0)) {
			CWallsMapDoc::m_csLastPath = pathName;
			break;
		}
	}

	LPCSTR pBrokenName = trx_Stpnam(m_sBrokenText);

	s.Format("Searching for %s", pBrokenName);

	//*pathName=0;
	strcpy(pathName, pBrokenName);
	if (!CWallsMapDoc::PromptForFileName(pathName, MAX_PATH, s, 0, FTYP_NONTL | FTYP_NOSHP))
		return;

	if (_access(pathName, 0)) {
		ASSERT(0);
		return;
	}

	LPSTR pName = trx_Stpnam(pathName);

	//Warn if file name is different!
	if (_stricmp(pName, pBrokenName)) {
		//don't prompt of only extensions differ
		int i = trx_Stpext(pName) - pName;
		if ((i != trx_Stpext(pBrokenName) - pBrokenName) || _memicmp(pName, pBrokenName, i))
			CMsgBox("CAUTION: The chosen file name, %s, "
				"doesn't match that of the linked file, which is %s.", pName, pBrokenName);
	}

	s = pathName; //avoid malloc
	m_pShp->GetRelativePath(pathName, s,/*bNoNameOnPath=*/FALSE);

	SetReplacementText(m_sReplacement = pathName); //new suggested relative pathname

	Enable(IDC_REPLACE);
	Enable(IDC_REPLACEALL);
}

void CLinkTestDlg::StoreUnmatchedPrefixes(LPCSTR p0, LPCSTR p1)
{

	LPCSTR pp0 = trx_Stpnam(p0);
	LPCSTR pp1 = trx_Stpnam(p1);

	int matches = pp0 - p0;
	if (matches == (pp1 - p1) && !memcmp(p0, p1, matches))
		return;

	for (matches = 0; pp0 > p0 && pp1 > p1 && pp0[-1] == pp1[-1]; pp0--, pp1--) {
		if (pp0[-1] == '\\') matches = 1;
		else ++matches;
	}
	//check if replacement string is already present --
	CString s1;
	s1.SetString(p1, pp1 - p1 + matches);
	for (VEC_CSTR2::reverse_iterator rit = m_vcs2.rbegin(); rit != m_vcs2.rend(); rit++) {
		if (!s1.CompareNoCase(rit->s1)) {
			return;
		}
	}
	m_vcs2.push_back(CSTR2());
	CSTR2 &cs = m_vcs2.back();
	cs.s1 = s1;
	cs.s0.SetString(p0, pp0 - p0 + matches);
}

void CLinkTestDlg::OnOK()
{
	BeginWaitCursor();
	m_lp.nRow = 0;
	m_lp.nRec = Search(-1); //total broken links
	EndWaitCursor();
	CDialog::OnOK();
}

LRESULT CLinkTestDlg::OnCommandHelp(WPARAM wNone, LPARAM lParam)
{
	AfxGetApp()->WinHelp(0);
	return TRUE;
}

void CLinkTestDlg::OnBnClickedCreateLog()
{
	static const LPCSTR bar = "===================================================================\r\n";
	char keybuf[256], pathName[MAX_PATH + 4];
	strcpy(pathName, m_pShp->PathName());
	strcpy(trx_Stpext(pathName), "_log.t$$");
	CTRXFile &trx = m_pDBGridDlg->m_trx;
	ASSERT(!trx.Opened());

	if (trx.Create(pathName) || !trx.Cache() && trx.AllocCache(64)) {
		trx.CloseDel(); //deletes cache
		CMsgBox("Failure creating temporary index.");
		return;
	}
	trx.SetUnique();

	strcpy(trx_Stpext(pathName), ".txt");
	CFile cFile;

	int e = CheckAccess(pathName, 0);

	if (!e) {
		//file exists --
		e = CheckAccess(pathName, 2); //write access?
		if (!e) {
			//we can optionally append or overwrite --
			e = CMsgBox(MB_YESNOCANCEL, "A broken links log already exists for this shapefile. Do you want to extend the "
				"log as opposed to replacing it?");
		}
		else {
			ASSERT(e == -1);
		}
	}
	else e = IDNO;

	if (e == IDCANCEL || e == -1 || !cFile.Open(pathName, (e == IDNO) ? (CFile::modeCreate | CFile::modeWrite) : CFile::modeWrite)) {
		trx.CloseDel(); //deletes cache
		if (e != IDCANCEL) CMsgBox("Failure creating or accessing log file. It may be open and protected.");
		return;
	}

	{
		CString s;
		if (e == IDNO) {
			s.Format("Broken Links in %s\r\n%s", trx_Stpnam(m_pShp->PathName()), bar);
			cFile.Write((LPCSTR)s, s.GetLength());
		}
		else {
			cFile.SeekToEnd();
		}
		s.Format("Field: %s\r\n", (LPCSTR)m_pShp->m_pdb->FldNamPtr(m_lp.nFld));
		cFile.Write((LPCSTR)s, s.GetLength());
	}

	BeginWaitCursor();
	m_lp.nRow = 0;
	m_lp.nRec = Search(-2); //build index
	ASSERT(trx.NumKeys());

	//Scan index and write log  --
	e = trx.First();
	while (!e) {
		VERIFY(!trx.GetKey(keybuf));
		int len = (BYTE)keybuf[0];
		cFile.Write(trx_Stxpc(keybuf), len);
		cFile.Write("\r\n", 2);
		e = trx.Next();
	}
	cFile.Write(bar, strlen(bar));
	cFile.Close();
	trx.CloseDel();
	EndWaitCursor();
	OpenFileAsText(pathName);
	CDialog::OnOK();
}

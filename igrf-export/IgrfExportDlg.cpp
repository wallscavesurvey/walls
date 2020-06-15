// IgrfExportDlg.cpp : implementation file
//

#include "stdafx.h"
#include "IgrfExport.h"
#include <ctype.h>
#include <io.h>
#include <dos_io.h>
#include <trx_str.h>
#include <trxfile.h>
#include "PromptPath.h"
#include "crack.h"
#include "IgrfExportDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CAboutDlg dialog used for App About

void CMsgBox(LPCSTR format, ...)
{
	char buf[512];

	va_list marker;
	va_start(marker, format);
	_vsnprintf(buf, 512, format, marker); buf[511] = 0;
	va_end(marker);
	AfxMessageBox(buf);
}

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

	// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()

#define MAX_SIZELINE (64*1024)
static CString csLogPath;

#ifdef _READ_TEXT
static CFile cfRTF;
static bool bEOF = false;
static char linebuf[MAX_SIZELINE];
static UINT linepos, linecount = 0;
static CString csRTF, csLogPath;
static char *pRecBuf = NULL;

static int ReadLine()
{
	if (bEOF) return -1;
	char *p = linebuf;
	linepos = (UINT)cfRTF.GetPosition();
	for (; !bEOF; p++) {
		if (cfRTF.Read(p, 1) != 1) {
			bEOF = true;
			break;
		}
		if (p == linebuf + MAX_SIZELINE - 1) {
			bEOF = true;
			return -1;
		}
		if (*p == '\n') {
			if (p > linebuf && p[-1] == '\r') p--;
			break;
		}
	}
	*p = 0;
	linecount++;
	return p - linebuf;
}

static void elim_fs(char *p)
{
	char *pf;
	while (pf = strstr(p, "\\fs")) {
		char *p0 = strchr(pf, ' ');
		if (!p0) break;
		p0++;
		memmove(pf, p0, strlen(p0) + 1);
	}
	if (pf = strchr(p, '\\')) *pf = 0;
}
#endif

// CIgrfExportDlg dialog

CIgrfExportDlg::CIgrfExportDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CIgrfExportDlg::IDD, pParent)
	, m_csPathName("igrf12.xls")
	, m_nLogMsgs(0)
	, m_bLogFailed(FALSE)
	, m_fbLog(2048)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

static BOOL GetBool(CDaoRecordset &rs, int nFld)
{
	COleVariant var;
	rs.GetFieldValue(nFld, var);
	return V_BOOL(&var);
}

static long GetLong(CDaoRecordset &rs, int nFld)
{
	COleVariant var;
	rs.GetFieldValue(nFld, var);
	if (var.vt == VT_R8) return (long)V_R8(&var);
	CString s(CCrack::strVARIANT(var));
	return s.IsEmpty() ? 0 : atol(s);
}

static void GetString(CString &s, CDaoRecordset &rs, int nFld)
{
	COleVariant var;
	rs.GetFieldValue(nFld, var);
	if (var.vt == VT_EMPTY || var.vt == VT_NULL)
		s.Empty();
	else {
		s = CCrack::strVARIANT(var);
		s.Trim();
	}
}

double GetDouble(CDaoRecordset &rs, int nFld)
{
	COleVariant var;
	rs.GetFieldValue(nFld, var);
	if (var.vt == VT_R8) return (double)V_R8(&var);
	return 0.0;
}

static BOOL GetText(LPSTR dst, CDaoRecordset &rs, int nFld, int nLen)
{
	COleVariant var;
	rs.GetFieldValue(nFld, var);
	CString s(CCrack::strVARIANT(var));
	if (var.vt == VT_EMPTY || var.vt == VT_NULL)
		*dst = 0;
	else {
		s.Trim();
		trx_Stncc(dst, s, nLen + 1);
		if (s.GetLength() > nLen) return -1;
	}
	return TRUE;
}

static BOOL GetVarText(LPSTR dst, CDaoRecordset &rs, int nFld, int nLen)
{
	COleVariant var;
	rs.GetFieldValue(nFld, var);
	if (var.vt == VT_EMPTY || var.vt == VT_NULL)
		*dst = 0;
	else {
		CString s;
		if (VT_R8 == var.vt) {
			int i = (int)((double)V_R8(&var));
			s.Format("%d", i);
		}
		else {
			s = CCrack::strVARIANT(var);
			s.Trim();
		}
		trx_Stncc(dst, s, nLen + 1);
		if (s.GetLength() > nLen) return -1;
	}
	return TRUE;
}

static BOOL GetLText(LPSTR dst, CDaoRecordset &rs, int nFld, int nLen)
{
	if (!GetText(dst, rs, nFld, nLen)) return FALSE;
	switch (*dst) {
	case 0: break;
	case 'Y':
	case 'y': *dst = 'Y'; break;
	case 'N':
	case 'n': *dst = 'N'; break;
	default: return FALSE;
	}
	return TRUE;
}

void CDECL CIgrfExportDlg::WriteLog(const char *format, ...)
{
	char buf[256];

	if (m_bLogFailed) return;

	if (!m_fbLog.IsOpen()) {
		strcpy(buf, m_csPathName);
		strcpy(trx_Stpext(buf), ".txt");
		csLogPath = buf;

		if ((m_bLogFailed =
			m_fbLog.OpenFail(csLogPath,
				CFile::modeCreate | CFile::modeReadWrite | CFile::shareExclusive))) return;

	}

	m_nLogMsgs++;

	va_list marker;
	va_start(marker, format);
	//NOTE: Does the 2nd argument include the terminating NULL?
	//      The return value definitely does NOT.
	int len;
	if ((len = _vsnprintf(buf, 255, format, marker)) < 0) len = 254;
	m_fbLog.Write(buf, len);
	va_end(marker);
}

void CIgrfExportDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_PATHNAME, m_csPathName);
	DDV_MaxChars(pDX, m_csPathName, 260);

#define BUF_LEN 31

	if (pDX->m_bSaveAndValidate) {
		try {
			CDaoRecordset rs(&m_database);

			rs.Open(dbOpenTable, "[Sheet1$]");
			if (!rs.IsEOF()) {
				char buf[BUF_LEN + 1];
				int n, m;

				//m_csPathName.Truncate(trx_Stpext(m_csPathName)-(LPCSTR)m_csPathName);
				//m_csPathName+=".shp";

				int nf = rs.GetFieldCount();
				int nYears = nf - 4;
				if (nYears < 2 || rs.IsEOF()) goto _close;

				nf = 3; //first year
				int mlast, nflast = nf + nYears - 1;

				UINT numRecs;

				char modname[10];

				for (int ny = 1900; nf <= nflast; ny += 5, nf++) {
					if (ny < 2000) {
						mlast = 10; m = 0;
					}
					else {
						mlast = 13;
						m = (ny < 2015) ? 0 : 8;
					}
					sprintf(modname, "%cGRF%0*u", (ny < 1945 || ny == 2015) ? 'I' : 'D', (ny < 2000) ? 2 : 4, (ny < 2000) ? ny - 1900 : ny);
					numRecs = 0;

					WriteLog("%11s  %4u.00%3u%3u  0 %u.00 %u.00   -1.0  600.0%17s%4u\r\n",
						modname, ny, mlast, m, ny, ny + 5, modname, numRecs++);
					rs.MoveFirst();
					bool bPending = false;
					m = n = 0;
					double f1 = 0, f2 = 0, sv1 = 0, sv2 = 0;
					while (!rs.IsEOF()) {
						GetText(buf, rs, 0, BUF_LEN);
						if (*buf == 'g') {
							if (bPending) {
								WriteLog("%2u %2u%10.2f%10.2f%10.2f%10.2f%31s%4u\r\n",
									n, m, f1, f2, sv1, sv2, modname, numRecs++);
								bPending = false;
								if (m == mlast) break;
							}
							n = GetLong(rs, 1);
							m = GetLong(rs, 2);
							f1 = GetDouble(rs, nf);
							sv1 = (nf == nflast) ? GetDouble(rs, nf + 1) : 0;
							f2 = sv2 = 0;
							bPending = true;
						}
						else {
							ASSERT(*buf == 'h' && bPending);
							f2 = GetDouble(rs, nf);
							sv2 = (nf == nflast) ? GetDouble(rs, nf + 1) : 0;
						}
						rs.MoveNext();
					}
					if (bPending)
						WriteLog("%2u %2u%10.2f%10.2f%10.2f%10.2f%31s%4u\r\n",
							n, m, f1, f2, sv1, sv2, modname, numRecs++);
				}
			}
		_close:
			rs.Close();
		}
		catch (CDaoException* e)
		{
			AfxMessageBox(e->m_pErrorInfo->m_strDescription);
			e->Delete();
		}

		if (m_fbLog.IsOpen()) {
			m_fbLog.Close();
			if ((int)::ShellExecute(NULL, "Open", csLogPath, NULL, NULL, SW_NORMAL) <= 32) {
				CMsgBox("Couldn't open log %s automatically.", csLogPath);
			}
		}
	}
}

BEGIN_MESSAGE_MAP(CIgrfExportDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BROWSE, OnBnClickedBrowse)
END_MESSAGE_MAP()


// CIgrfExportDlg message handlers

BOOL CIgrfExportDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	if (!_access(m_csPathName, 4)) {
		try
		{
			m_database.Open(m_csPathName, FALSE, TRUE, "Excel 5.0;");
		}
		catch (CDaoException* e)
		{
			AfxMessageBox(e->m_pErrorInfo->m_strDescription);
			e->Delete();
		}
	}

	if (!m_database.IsOpen())
	{
		// Now pop-up file-open dlg to ask for location
		CFileDialog dlgFile(
			TRUE,
			_T("xls"),
			NULL,
			OFN_ENABLESIZING | OFN_FILEMUSTEXIST,
			_T("Excel Files (*.xls)|*.xls|All Files (*.*)|*.*||"));

		if (dlgFile.DoModal() != IDCANCEL) {
			try
			{
				m_csPathName = dlgFile.GetFileName();
				m_database.Open(m_csPathName, FALSE, FALSE, "Excel 5.0;");
			}
			catch (CDaoException* e)
			{
				// Tell them the reason it failed to open
				AfxMessageBox(e->m_pErrorInfo->m_strDescription);
				e->Delete();
			}
		}
	}
	else {
		m_csPathName = dos_FullPath(m_csPathName, NULL);
	}

	if (!m_database.IsOpen())
		EndDialog(IDCANCEL);


	CDialog::OnInitDialog();

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CIgrfExportDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CIgrfExportDlg::OnPaint()
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
HCURSOR CIgrfExportDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CIgrfExportDlg::OnBnClickedBrowse()
{
	CString strFilter;
	if (!AddFilter(strFilter, IDS_SHP_FILES)) return;
	CString s(m_csPathName);
	if (DoPromptPathName(s, OFN_OVERWRITEPROMPT, 1, strFilter, FALSE, IDS_SHP_SAVEAS, ".shp")) {
		GetDlgItem(IDC_PATHNAME)->SetWindowText(s);
	}
}



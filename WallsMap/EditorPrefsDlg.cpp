// EditorPrefsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "WallsMap.h"
#include "MainFrm.h"
#include "EditorPrefsDlg.h"

static LPCSTR pTextSample = "AaBbYyZz 0123456789";

// CEditorPrefsDlg dialog

IMPLEMENT_DYNAMIC(CEditorPrefsDlg, CDialog)
CEditorPrefsDlg::CEditorPrefsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CEditorPrefsDlg::IDD, pParent)
	, m_hBrushOld(NULL)
	, m_hBmpOld(NULL)
{
}

CEditorPrefsDlg::~CEditorPrefsDlg()
{

	if (m_dcBmp.m_hDC) {
		if (m_hBmpOld)
			VERIFY(::SelectObject(m_dcBmp.m_hDC, m_hBmpOld));
		if (m_hBrushOld)
			::DeleteObject(::SelectObject(m_dcBmp.m_hDC, m_hBrushOld));
		VERIFY(m_dcBmp.DeleteDC());
	}
}

void CEditorPrefsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_RTFEMPTY, m_bEditRTF);
	DDX_Check(pDX, IDC_RTFTOOLBAR, m_bEditToolbar);
	DDX_Control(pDX, IDC_COLORFRM, m_StaticFrame);
	DDX_Control(pDX, IDC_BKGNDCOLOR, m_BtnBkg);
	DDX_Control(pDX, IDC_TEXTCOLOR, m_BtnTxt);
}

BEGIN_MESSAGE_MAP(CEditorPrefsDlg, CDialog)
	ON_BN_CLICKED(IDC_FONT, OnBnClickedFont)
	ON_MESSAGE(CPN_SELCHANGE, OnChgColor)
	ON_WM_PAINT()
	ON_MESSAGE(WM_COMMANDHELP, OnCommandHelp)
	ON_BN_CLICKED(IDC_TXTCOMP, &CEditorPrefsDlg::OnClickedTxtcomp)
END_MESSAGE_MAP()


// CEditorPrefsDlg message handlers

BOOL CEditorPrefsDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_BtnTxt.SetColor(m_clrTxt);
	m_BtnBkg.SetColor(m_clrBkg);

	m_StaticFrame.GetClientRect(&m_rectBox);
	m_sizeBox.cx = m_rectBox.right;
	m_sizeBox.cy = m_rectBox.bottom;

	m_StaticFrame.ClientToScreen(&m_rectBox);
	ScreenToClient(&m_rectBox);

	// Get temporary DC for dialog - Will be released in dc destructor
	CClientDC dc(this);
	VERIFY(m_cBmp.CreateCompatibleBitmap(&dc, m_sizeBox.cx, m_sizeBox.cy));
	VERIFY(m_dcBmp.CreateCompatibleDC(&dc));
	//Use API so as to keep m_hBmpOld valid --
	VERIFY(m_hBmpOld = (HBITMAP)::SelectObject(m_dcBmp.m_hDC, m_cBmp.GetSafeHandle()));
	{
		HBRUSH hbr = ::CreateSolidBrush(m_clrBkg);
		if (hbr) {
			VERIFY(m_hBrushOld = (HBRUSH)::SelectObject(m_dcBmp.m_hDC, hbr));
		}
	}
	m_dcBmp.SetTextAlign(TA_TOP | TA_CENTER);

	VERIFY(InitLabelDC(m_dcBmp.m_hDC, &m_font, m_clrTxt));
	GetLabelSize();
	UpdateBox();
	RefreshTextCompOptions();
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CEditorPrefsDlg::GetLabelSize()
{
	m_szLabel = m_dcBmp.GetTextExtent(pTextSample, strlen(pTextSample));
}

void CEditorPrefsDlg::OnBnClickedFont()
{
	if (m_font.GetFontFromDialog()) {
		InitLabelDC(m_dcBmp.m_hDC);
		VERIFY(InitLabelDC(m_dcBmp.m_hDC, &m_font, m_clrTxt));
		GetLabelSize();
		UpdateBox();
	}
}

void CEditorPrefsDlg::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	dc.BitBlt(m_rectBox.left, m_rectBox.top, m_rectBox.Width(), m_rectBox.Height(),
		&m_dcBmp, 0, 0, SRCCOPY);
}

void CEditorPrefsDlg::UpdateBox()
{
	VERIFY(m_dcBmp.PatBlt(0, 0, m_sizeBox.cx, m_sizeBox.cy, PATCOPY));

	//Place label on bitmap --
	m_dcBmp.TextOut(m_sizeBox.cx / 2, (m_sizeBox.cy - m_szLabel.cy) / 2, pTextSample, strlen(pTextSample));

	Invalidate(FALSE);
}

LRESULT CEditorPrefsDlg::OnChgColor(WPARAM clr, LPARAM id)
{
	if (id == IDC_TEXTCOLOR) {
		if (clr == m_clrTxt) return TRUE;
		::SetTextColor(m_dcBmp.m_hDC, m_clrTxt = clr);
	}
	else if (id == IDC_BKGNDCOLOR) {
		if (clr == m_clrBkg) return TRUE;
		HBRUSH hbr = ::CreateSolidBrush(clr);
		if (hbr) {
			m_clrBkg = clr;
			::DeleteObject(::SelectObject(m_dcBmp.m_hDC, hbr));
		}
	}
	UpdateBox();
	return TRUE;
}

LRESULT CEditorPrefsDlg::OnCommandHelp(WPARAM wNone, LPARAM lParam)
{
	AfxGetApp()->WinHelp(0);
	return TRUE;
}

void CEditorPrefsDlg::RefreshTextCompOptions()
{
	CString s("Text compare program:\n");
	s += CMainFrame::m_pComparePrg ? trx_Stpnam(CMainFrame::m_pComparePrg) : "None";

	GetDlgItem(IDC_ST_TXTCOMP)->SetWindowText(s);
}

void CEditorPrefsDlg::OnClickedTxtcomp()
{
	if (GetMF()->TextCompOptions()) RefreshTextCompOptions();
}

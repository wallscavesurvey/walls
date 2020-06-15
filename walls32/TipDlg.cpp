#include "stdafx.h"
#include "resource.h"
#include "TipDlg.h"

#include <winreg.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTipDlg dialog

#define TIP_OFFSET 21000

static const TCHAR szSection[] = _T("Tip");
static const TCHAR szIntID[] = _T("NextID");
static const TCHAR szIntStartup[] = _T("StartUp");
static const TCHAR szRelease[] = _T("Rel");

CTipDlg::CTipDlg(CWnd* pParent /*=NULL*/)
	: CDialog(IDD_TIP, pParent)
{
	//{{AFX_DATA_INIT(CTipDlg)
	//}}AFX_DATA_INIT

	m_iVersion = AfxGetApp()->GetProfileInt(szSection, szRelease, 0);
	m_bStartup = (m_iVersion != WALLS_RELEASE) || !AfxGetApp()->GetProfileInt(szSection, szIntStartup, 0);
	m_idMax = 0;
}

CTipDlg::~CTipDlg()
{
	// This destructor is executed whether the user had pressed the escape key
	// or clicked on the close button. If the user had pressed the escape key,
	// it is still required to update the filepos in the ini file with the 
	// latest position so that we don't repeat the tips! 

	if (m_idMax) {
		AfxGetApp()->WriteProfileInt(szSection, szIntID, m_id);
		if (m_iVersion != WALLS_RELEASE) AfxGetApp()->WriteProfileInt(szSection, szRelease, WALLS_RELEASE);
	}
}

void CTipDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTipDlg)
	DDX_Check(pDX, IDC_STARTUP, m_bStartup);
	DDX_Text(pDX, IDC_TIPSTRING, m_strTip);
	//}}AFX_DATA_MAP
	if (!pDX->m_bSaveAndValidate) GetDlgItem(IDC_TELLMEMORE)->EnableWindow(m_idHelp);
	else AfxGetApp()->WriteProfileInt(szSection, szIntStartup, !m_bStartup);
}

BEGIN_MESSAGE_MAP(CTipDlg, CDialog)
	ON_BN_CLICKED(IDC_TELLMEMORE, OnTellMeMore)
	ON_BN_CLICKED(IDC_PREVTIP, OnPrevTip)
	//{{AFX_MSG_MAP(CTipDlg)
	ON_BN_CLICKED(IDC_NEXTTIP, OnNextTip)
	ON_WM_CTLCOLOR()
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTipDlg message handlers

void CTipDlg::OnPrevTip()
{
	ASSERT(m_idMax && m_id);
	//m_id is the last shown tip -- it will be decremented by OnNextTip() --
	if ((m_id += 2) > m_idMax + 1) m_id = 2;
	OnNextTip();

}
void CTipDlg::OnNextTip()
{
	if (!m_idMax) {
		CString str;
		if (str.LoadString(TIP_OFFSET)) m_idMax = atoi(str);
		if (m_iVersion != WALLS_RELEASE) m_id = 0;
		else m_id = AfxGetApp()->GetProfileInt(szSection, szIntID, 0); //Last tip shown
	}

	m_idHelp = 0;
	m_strTip = "";

	for (int i = 0; i < m_idMax; i++) {
		if (--m_id < 1) m_id = m_idMax;
		if (m_strTip.LoadString(m_id + TIP_OFFSET)) {
			m_strTip.TrimRight();
			LPTSTR lpsz = m_strTip.GetBuffer(0);
			int len = strlen(lpsz) - 1;
			if (lpsz[len] == '#') {
				while (len && lpsz[--len] != ' ');
				if (len) {
					m_idHelp = atoi(lpsz + len + 1);
					lpsz[len] = 0; //may not be needed?
					m_strTip.ReleaseBuffer(-1);
				}
			}
			break;
		}
	}

	UpdateData(FALSE);
}

HBRUSH CTipDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	if (pWnd->GetDlgCtrlID() == IDC_TIPSTRING)
		return (HBRUSH)GetStockObject(WHITE_BRUSH);

	return CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
}

BOOL CTipDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// If Tips file does not exist then disable NextTip
	OnNextTip();

	return TRUE;  // return TRUE unless you set the focus to a control
}

void CTipDlg::OnTellMeMore()
{
	if (m_idHelp) AfxGetApp()->WinHelp(m_idHelp);
}

void CTipDlg::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	// Paint the background white.
	CRect rect;
	GetDlgItem(IDC_TIPFRAME)->GetWindowRect(&rect);
	ScreenToClient(&rect);
	CBrush brush;
	brush.CreateStockObject(WHITE_BRUSH);
	dc.FillRect(rect, &brush);

	// Get paint area for the big static control
	CWnd* pStatic = GetDlgItem(IDC_BULB);
	pStatic->GetWindowRect(&rect);
	ScreenToClient(&rect);

	// Load bitmap and get dimensions of the bitmap
	CBitmap bmp;
	bmp.LoadBitmap(IDB_LIGHTBULB);
	BITMAP bmpInfo;
	bmp.GetBitmap(&bmpInfo);

	// Draw bitmap in top corner and validate only top portion of window
	CDC dcTmp;
	dcTmp.CreateCompatibleDC(&dc);
	dcTmp.SelectObject(&bmp);
	rect.bottom = bmpInfo.bmHeight + rect.top;
	dc.BitBlt(rect.left, rect.top, rect.Width(), rect.Height(),
		&dcTmp, 0, 0, SRCCOPY);

	// Draw out "Did you know..." message next to the bitmap
	CString strMessage;
	strMessage.LoadString(CG_IDS_DIDYOUKNOW);
	rect.left += bmpInfo.bmWidth;
	dc.DrawText(strMessage, rect, DT_VCENTER | DT_SINGLELINE);

	// Do not call CDialog::OnPaint() for painting messages
}

// TableReplDlg.cpp : implementation file
//

#include "stdafx.h"
#include "WallsMap.h"
#include "DBGridDlg.h"
#include "TableReplDlg.h"


// CTableReplDlg dialog

IMPLEMENT_DYNAMIC(CTableReplDlg, CDialog)

static bool bPrefExtended=false;
static bool bPrefMatchCase=false;
static bool bPrefMatchWord=false;

CTableReplDlg::CTableReplDlg(WORD nFld,CWnd* pParent /*=NULL*/)
	: CDialog(CTableReplDlg::IDD, pParent)
	, m_nFld(nFld)
	, m_pGDlg((CDBGridDlg *)pParent)
	, m_csFindWhat(_T(""))
	, m_csReplWith(_T(""))
	, m_bExtended(bPrefExtended)
	, m_bMatchCase(bPrefMatchCase)
	, m_bMatchWord(bPrefMatchWord)
{
	if(m_pGDlg->m_pShp->m_pdb->FldTyp(m_nFld)=='M')
		m_fLen=-1;
	else m_fLen=m_pGDlg->m_pShp->m_pdb->FldLen(m_nFld);
}

CTableReplDlg::~CTableReplDlg()
{
}

HBRUSH CTableReplDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
	HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
	if(pWnd->GetDlgCtrlID()==IDC_ST_FIELD) {
		pDC->SetTextColor(RGB(0x80,0,0));
    }
	return hbr;
}

static void FixEscapes(CString &s)
{
	static char crlf[4]={0x0d,0x0a,0};
	static char tab[2]={0x09,0};
	static LPCSTR ph="0123456789ABCDEF";

	for(int i=0; (i=s.Find("\\x", i))>=0;) {
		LPCSTR ph0,ph1;
		if((ph1=strchr(ph, toupper(s[i+2]))) && (ph0=strchr(ph, toupper(s[i+3])))) {
			s.Delete(i,4);
			s.Insert(i, (char)(16*(ph1-ph)+(ph0-ph)));
			i++;
		}
		else i+=2;
	}
	s.Replace("\\n",crlf);
	s.Replace("\\t",tab);
}

void CTableReplDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_FIND_WHAT, m_csFindWhat);
	DDV_MaxChars(pDX, m_csFindWhat, 512);
	DDX_Check(pDX, IDC_EXTENDED, m_bExtended);
	DDX_Text(pDX, IDC_REPL_WITH, m_csReplWith);
	DDV_MaxChars(pDX, m_csReplWith, 512);
	DDX_Check(pDX, IDC_MATCH_CASE, m_bMatchCase);
	DDX_Check(pDX, IDC_MATCH_WORD, m_bMatchWord);

	if(pDX->m_bSaveAndValidate) {
		UINT id=0;
		CString msg;
		if(m_bMatchWord) m_csFindWhat.Trim();
		if(!m_bMatchCase) m_csFindWhat.MakeUpper();
		if(m_bExtended) {
			FixEscapes(m_csFindWhat);
			FixEscapes(m_csReplWith);
		}
		if(m_fLen>0) {
			if(m_fLen<m_csFindWhat.GetLength()) id=IDC_FIND_WHAT;
			else if(m_fLen<m_csReplWith.GetLength()) id=IDC_REPL_WITH;
			if(id) {
				msg.Format("The text entered for \"%s\" is too long for a field of length %u.",
					(id==IDC_FIND_WHAT)?"Find what":"Replace with", m_fLen);
			}
		}
		if(!id && !m_csFindWhat.Compare(m_csReplWith)) {
		    msg="The \"Find what\" and \"Replace with\" boxes can't ";
			msg+=(m_csFindWhat.IsEmpty() && m_csReplWith.IsEmpty())?"both be empty.":"have the same content.";
			id=IDC_FIND_WHAT;
		}
		if(id) {
			AfxMessageBox(msg);
			pDX->m_idLastControl=id;
			pDX->m_bEditLastControl=TRUE;
			pDX->Fail();
			return;
		}
		bPrefExtended=m_bExtended==1;
		bPrefMatchCase=m_bMatchCase==1;
		bPrefMatchWord=m_bMatchWord==1;
	}
}

BEGIN_MESSAGE_MAP(CTableReplDlg, CDialog)
	ON_WM_CTLCOLOR()
	ON_BN_CLICKED(IDC_SELECT_ALL, &CTableReplDlg::OnBnClickedSelectAll)
END_MESSAGE_MAP()

BOOL CTableReplDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	CString s;
	s.Format("Field %s - Replace Text in Selected Records", m_pGDlg->m_pShp->m_pdb->FldNamPtr(m_nFld));
	SetWindowText(s);
	s.Format("%u", m_pGDlg->m_nSelCount);
	GetDlgItem(IDC_ST_FIELD)->SetWindowText(s);

	if(m_pGDlg->m_nSelCount<=1 && m_pGDlg->NumRecs()>1) GetDlgItem(IDC_SELECT_ALL)->ShowWindow(SW_SHOW);

	GetDlgItem(IDC_FIND_WHAT)->SetFocus();

	return FALSE;  // return TRUE unless you set the focus to a control
}

// CTableReplDlg message handlers

void CTableReplDlg::OnBnClickedSelectAll()
{
	m_pGDlg->OnSelectAll();
	GetDlgItem(IDC_SELECT_ALL)->ShowWindow(SW_HIDE);
	CString s;
	s.Format("%u", m_pGDlg->m_nSelCount);
	GetDlgItem(IDC_ST_FIELD)->SetWindowText(s);
}

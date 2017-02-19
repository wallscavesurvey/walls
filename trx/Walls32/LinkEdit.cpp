// LinkEdit.cpp : implementation file
//

#include "stdafx.h"
#include "walls.h"
#include "MagDlg.h"
#include "dialogs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLinkEdit

#ifdef _DEBUG
	IMPLEMENT_DYNAMIC(CLinkEdit,CEdit)
#endif

BEGIN_MESSAGE_MAP(CLinkEdit, CEdit)
	ON_CONTROL_REFLECT(EN_CHANGE, OnChange)
	//ON_CONTROL_REFLECT(EN_SETFOCUS, OnSetfocus)
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_REGISTERED_MESSAGE(WM_RETFOCUS,OnRetFocus)
	ON_WM_CTLCOLOR_REFLECT()
    ON_WM_CONTEXTMENU()
	ON_MESSAGE(WM_PASTE, OnPaste)
	ON_MESSAGE(WM_CUT, OnCut)
	ON_MESSAGE(WM_CLEAR, OnCut)
END_MESSAGE_MAP()

CLinkEdit *CLinkEdit::m_pFocus=NULL;
int CLinkEdit::m_bSetting=0;
bool CLinkEdit::m_bNoContext=false;

/////////////////////////////////////////////////////////////////////////////
// CLinkEdit message handlers

BOOL CLinkEdit::PreTranslateMessage(MSG* pMsg)
{
	bool bsave=false;
	int c=pMsg->wParam;

	if(pMsg->message==WM_CHAR) {
		if(c>='0' && c<='9' || c=='.' || c=='-') bsave=true;
		else {
			if(isprint(c)) {
				MessageBeep(0);
				return 1;
			}
		}
	}
	else if(pMsg->message == WM_KEYDOWN) {
		switch(c) {
			case VK_DELETE :
				bsave=true;
				break;
		}
	}

	if(bsave) {
		m_dwSavedPos=GetSel();
		GetWindowText(m_csSaved);
	}

	return CEdit::PreTranslateMessage(pMsg);
}

LRESULT CLinkEdit::OnCut(WPARAM wParam, LPARAM lParam)
{
	m_dwSavedPos=GetSel();
	GetWindowText(m_csSaved);
	Default();
	return 0;
}
LRESULT CLinkEdit::OnPaste(WPARAM wParam, LPARAM lParam)
{
	char buf[32];
	ASSERT(::IsClipboardFormatAvailable(CF_TEXT));
	if(ClipboardPaste(buf,31)) {
		cfg_GetArgv(buf,CFG_PARSE_ALL);
		if(cfg_argc!=1 || strlen(cfg_argv[0])>=20 || !IsNumeric(cfg_argv[0])) {
			CErrorDlg dlg("The clipboard content is too large or not numeric.",GetParent(),"Paste Failure");
			dlg.DoModal();
			*buf=0;
		}
	}
	if(*buf) {
		m_dwSavedPos=GetSel();
		GetWindowText(m_csSaved);
		Default();
	}
	return 0;
}

void CLinkEdit::OnChange() 
{
	if(m_bSetting) {
		//Changed by UpdateText() --
		m_bSetting--;
		return;
	}
	BOOL oldInRange=InRange();
	BOOL newInRange=RetrieveText();
	if(newInRange==-1) return; //no change
	if(newInRange || oldInRange) {
		m_pFocus=this;
		m_pCB(newInRange?newInRange:-1);
		m_pFocus=NULL;
	}
}

void CLinkEdit::OnKillFocus(CWnd* pNewWnd) 
{
	if(m_pMagDlg && !InRange() && !m_pMagDlg->ErrorOK(this,pNewWnd)) {
		m_bNoContext=true;
		PostMessage(WM_RETFOCUS,TRUE);
		return;
	}
	CEdit::OnKillFocus(pNewWnd);
}

LRESULT CLinkEdit::OnRetFocus(WPARAM,LPARAM)
{
	ShowRangeErrMsg();
	VERIFY(SetFocus());
	m_bNoContext=false;
	return TRUE;
}

void CLinkEdit::ShowRangeErrMsg()
{
	char buf[80];
	CErrorDlg dlg(GetRangeErrMsg(buf),this);
	dlg.DoModal();
}

void CLinkEdit::OnSetFocus(CWnd* pOldWnd) 
{
	CWnd::OnSetFocus(pOldWnd);
	PostMessage(EM_SETSEL,0,-1);
	if(InRange()) {
		m_pFocus=this;
		m_pCB(0);
		m_pFocus=NULL;
	}
}

HBRUSH CLinkEdit::CtlColor(CDC* pDC, UINT nCtlColor) 
{

	if(m_bGrayed) {
		pDC->SetTextColor(RGB_LTGRAY);
		return (HBRUSH)::GetStockObject(WHITE_BRUSH);
	}
	// TODO: Return a non-NULL brush if the parent's handler should not be called
	return NULL;
}

void CLinkEdit::OnContextMenu(CWnd* pWnd, CPoint point)
{
    SetFocus();
	if(m_bNoContext) return;
    CMenu menu;
    menu.CreatePopupMenu();

	DWORD flags;
    DWORD sel = GetSel();

	if(m_bType!=BTYP_INT) {
		flags=(m_bGrayed || m_pMagDlg && !m_pMagDlg->IsDirEnabled())?MF_GRAYED:0;
		menu.InsertMenu(-1, MF_BYPOSITION|flags,ME_COPYCOORDS,MES_COPYCOORDS);
		flags = IsClipboardFormatAvailable(CF_TEXT) ? 0 : MF_GRAYED;
		menu.InsertMenu(-1, MF_BYPOSITION,ME_PASTECOORDS,MES_PASTECOORDS);
		menu.InsertMenu(-1, MF_BYPOSITION|MF_SEPARATOR);
	}

    //BOOL bReadOnly = GetStyle() & ES_READONLY;
    flags = CanUndo() ? 0 : MF_GRAYED;
    menu.InsertMenu(-1, MF_BYPOSITION|flags, EM_UNDO, MES_UNDO);

    flags = (LOWORD(sel) == HIWORD(sel)) ? MF_GRAYED : 0;
    menu.InsertMenu(-1, MF_BYPOSITION|flags, WM_CUT, MES_CUT);
    menu.InsertMenu(-1, MF_BYPOSITION|flags, WM_COPY, MES_COPY);

    flags = IsClipboardFormatAvailable(CF_TEXT) ? 0 : MF_GRAYED;
    menu.InsertMenu(-1, MF_BYPOSITION|flags, WM_PASTE, MES_PASTE);

    flags = (LOWORD(sel) == HIWORD(sel)) ? MF_GRAYED : 0;
    menu.InsertMenu(-1, MF_BYPOSITION|flags, WM_CLEAR, MES_DELETE);

	if (point.x == -1 || point.y == -1)	{
		CRect rc;
		GetClientRect(&rc);
		point = rc.CenterPoint();
		ClientToScreen(&point);
	}

    int nCmd = menu.TrackPopupMenu(TPM_LEFTALIGN|TPM_LEFTBUTTON|TPM_RETURNCMD|TPM_RIGHTBUTTON, point.x, point.y, this);

    menu.DestroyMenu();

    if (nCmd < 0) return;

	if(nCmd==ME_COPYCOORDS || nCmd==ME_PASTECOORDS) {
		GetParent()->PostMessage(nCmd,m_bType);
		return;
	}

    switch (nCmd) {
		case EM_UNDO:
		case WM_CUT:
		case WM_COPY:
		case WM_CLEAR:
		case WM_PASTE:
			SendMessage(nCmd);
			break;
		default:
			SendMessage(WM_COMMAND, nCmd);
    }
}

/////////////////////////////////////////////////////////////////////////////
// CLinkEditI members

#ifdef _DEBUG
	IMPLEMENT_DYNAMIC(CLinkEditI,CLinkEdit)
#endif

BEGIN_MESSAGE_MAP(CLinkEditI, CLinkEdit)
	//{{AFX_MSG_MAP(CLinkEditI)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CLinkEditI::SetMin()
{
	if(*m_pI!=m_min) {
		*m_pI=m_min;
		UpdateText(TRUE);
	}
	if(m_pLinkR) m_pLinkR->SetMin();
}

void CLinkEditI::IncVal(int dir)
{
	//return true if degree link set at maximum
	bool bmax=false;
	int val=*m_pI+dir;

	if(val>m_min && m_pLinkL && *m_pLinkL->m_pI==m_pLinkL->m_max) {
		ASSERT(val==m_min+1);
		m_pLinkL->SetMin(); //efect will rollover m_pLinkL and set this to m_min+1
	}
	else if(val<m_min) {
		ASSERT(val==m_min-1);
		val=m_max; //allways roll over -- ex. 59
		if(m_pLinkL) {
			if(*m_pLinkL->m_pI > m_pLinkL->m_min) {
				m_pLinkL->IncVal(-1); //will not roll to max
			}
			//else m_pLinkL already at min.
		}
		else if(m_pLinkR) m_pLinkR->SetMin();
	}
	else if(val>m_max) {
		ASSERT(val==m_max+1);
		val=m_min;
		if(m_pLinkL) {
			m_pLinkL->IncVal(1);
			//This can set m_pLinkL to max and minimize this redundantly
		}
	}
	else if(val==m_max) {
		if(!m_pLinkL && m_pLinkR)
			m_pLinkR->SetMin();
	}
	*m_pI=val;
	m_bGrayed=FALSE;
	m_bSetting=1;
	SetWindowText(GetIntStr(val));
}

void CLinkEditI::IncSpin(int dir)
{
	m_pFocus=this;
	IncVal(dir);
	m_pCB(1);
	m_pFocus=NULL;
}

void CLinkEditI::Init(CDialog *pdlg,int id,int min,int max,
					 int len,int *pI,CLinkEditI *pLinkL,CLinkEdit *pLinkR,LINKEDIT_CB pCB,CMagDlg *pDlg /*=NULL*/)
{
	VERIFY(SubclassDlgItem(id,pdlg));
	m_pCB=pCB;
	LimitText(len);
	m_min=min; m_max=max;
	m_pLinkL=pLinkL;
	m_pLinkR=pLinkR;
	m_pMagDlg=pDlg;
	m_pI=pI;
	m_bGrayed=FALSE;
	UpdateText(TRUE);
}

LPCSTR CLinkEditI::GetRangeErrMsg(char *buf)
{
	_snprintf(buf,80,"Please enter a number between %d and %d.",m_min,m_max);
	return buf;
}

BOOL CLinkEditI::InRange()
{
	return m_bZoneType?(*m_pI && abs(*m_pI)<=61):(m_min<=*m_pI && m_max>=*m_pI);
}

void CLinkEditI::UpdateText(BOOL bShow)
{
	if(m_pFocus==this) return;
	m_bGrayed=!bShow;
	m_bSetting++;
	int val=abs(*m_pI);
	SetWindowText((!m_bZoneType || val<61)?GetIntStr(val):((val<0)?"S":"N")); //handle zone
}

BOOL CLinkEditI::RetrieveText()
{
	//called only from CLinkEdit::OnChange() --
	//return 0 if out of range value stored, -1 if no changes in value, 1 or 100+diff if new in-range value stored,
	char buf[20];
	GetWindowText(buf,20);
	int val=atoi(buf);
	if(val<m_min || val>m_max || *buf=='-' && m_min>=0) {
		if(val>=1 && val<m_min) {
			*m_pI=val;
			return 0;
		}
		MessageBeep(0);
		m_bSetting++;
		SetWindowText(m_csSaved);
		SetSel(m_dwSavedPos);
		return -1;
	}

	if(*m_pI==val) return -1;

	if(m_pLinkR && !m_pLinkL) {
		if(val==m_max) {
			m_pLinkR->SetMin();
		}
	}

	if(m_bZoneType) {
		//handle zone --
		int oldval=abs(*m_pI);
		*m_pI=(*m_pI<0)?-val:val;
		return val-oldval+100;
	}
	*m_pI=val;
	return 1;
}

////////////////////////////////////////////////////////////////////
//CLinkEditF

#ifdef _DEBUG
	IMPLEMENT_DYNAMIC(CLinkEditF,CLinkEdit)
#endif

BEGIN_MESSAGE_MAP(CLinkEditF, CLinkEdit)
	//{{AFX_MSG_MAP(CLinkEditF)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CLinkEditF::Init(CDialog *pdlg,int id,double min,double max,int len,
		double *pF,BYTE bType,CLinkEditI *pLinkL,LINKEDIT_CB pCB,CMagDlg *pDlg /*=NULL*/)
{
	VERIFY(SubclassDlgItem(id,pdlg));
	m_bType=bType;
	m_pCB=pCB;
	LimitText(20);
	m_fmin=min;
	m_fmax=max;
	m_len=len;
	m_pF=pF;
	m_pLinkL=pLinkL;
	m_pMagDlg=pDlg;
	UpdateText(TRUE);
}

LPCSTR CLinkEditF::GetRangeErrMsg(char *buf)
{
	_snprintf(buf,80,"Please enter a number between %.0f and %.0f.",m_fmin,m_fmax);
	return buf;
}

BOOL CLinkEditF::InRange()
{
	return (m_fmin<=*m_pF && m_fmax>=*m_pF);
}

void CLinkEditF::UpdateText(BOOL bShow)
{
	if(m_pFocus==this && m_bGrayed!=bShow) return;
	m_bSetting=1;
	m_bGrayed=!bShow;
	LPCSTR p=GetFloatStr(*m_pF,-11); //no trimming
	double d=atof(p);
	if(d==atof(GetFloatStr(*m_pF,-m_len)))
		GetFloatStr(*m_pF,m_len); //trim redundant zeroes
	SetWindowText(p);
}

void CLinkEditF::SetMin()
{
	if(*m_pF!=m_fmin) {
		*m_pF=m_fmin;
		UpdateText(TRUE);
	}
}

BOOL CLinkEditF::RetrieveText()
{
	char buf[20];
	GetWindowText(buf,20);
	double f=atof(buf);
	if(!IsNumeric(buf) || (*buf=='-' && m_fmin>=0) || f<m_fmin || f>m_fmax) {
		if(f>=0 && f<m_fmin && IsNumeric(buf)) {
			*m_pF=f;
			return 0;
		}
		MessageBeep(0);
		m_bSetting++;
		SetWindowText(m_csSaved);
		SetSel(m_dwSavedPos);
		return -1;
	}
	if(f==*m_pF) return -1;
	*m_pF=f;
	return 1;
}

/////////////////////////////////////////////////////////////////////////////
// CSpinEdit

BEGIN_MESSAGE_MAP(CSpinEdit, CSpinButtonCtrl)
	//{{AFX_MSG_MAP(CSpinEdit)
	ON_NOTIFY_REFLECT(UDN_DELTAPOS, OnDeltapos)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CSpinEdit::OnDeltapos(NMHDR* pNMHDR, LRESULT* pResult) 
{
	*pResult = 0; //ALWAYS???
	if(CLinkEdit::m_bNoContext) {
		return;
	}
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
	if(GetFocus()==m_pEdit) m_pEdit->IncSpin(-pNMUpDown->iDelta);	
}

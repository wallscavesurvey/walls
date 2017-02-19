// GradDlg.cpp : implementation file
//

#include "stdafx.h"
#include "walls.h"
#include "prjdoc.h"
#include "segview.h"
#include "plotview.h"
#include "mapframe.h"
#include "mapview.h"
#include "compile.h"
#include "GradDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGradientDlg dialog


CGradientDlg::CGradientDlg(UINT id,COLORREF clr,CWnd* pParent /*=NULL*/)
	: CDialog(CGradientDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CGradientDlg)
	//}}AFX_DATA_INIT
	m_id=id;
	m_clr=clr;
	m_iTyp=CUSTOM_COLOR_IDX(m_clr);
	m_iInterpMethod = (int)CPrjDoc::GetGradientPtr(m_iTyp)->GetInterpolationMethod();
	m_bUpdating=m_bChanged=FALSE;
	m_SelPegIndex=PEG_NONE;
}

void CGradientDlg::UpdateFloatBox(UINT id,double value,int iTyp /*=-1*/)
{
	if(!iTyp && LEN_ISFEET()) value=(double)LEN_SCALE(value);
	m_bUpdating=TRUE;
	char *p=(iTyp==1)?GetDateStr((int)value):GetFloatStr(value,iTyp<0?4:2);
	if(!iTyp && id!=IDC_SELPEG_VALUE && id!=IDC_NEWPEG_VALUE) {
		CString s;
		s.Format("%s %s",p,LEN_ISFEET()?"ft":"m");
		GetDlgItem(id)->SetWindowText(s);
	}
	else GetDlgItem(id)->SetWindowText(p);
	m_bUpdating=FALSE;
}

void CGradientDlg::UpdateValueBox(UINT id,double position)
{
	UpdateFloatBox(id,m_wndGradientCtrl.GetGradient().ValueFromPos(position),m_iTyp);
	if(id==IDC_SELPEG_VALUE) m_bSelValid=TRUE;
}

void CGradientDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGradientDlg)
	DDX_Control(pDX, IDC_SELPEG_COLOR, m_SelPegColor);
	DDX_CBIndex(pDX, IDC_METHOD_COMBO, m_iInterpMethod);
	//}}AFX_DATA_MAP
	DDX_Control(pDX,IDC_GRADIENT,m_wndGradientCtrl);
}

BEGIN_MESSAGE_MAP(CGradientDlg, CDialog)
	ON_MESSAGE(CPN_SELCHANGE, OnSelEndOK)
	ON_CBN_SELCHANGE(IDC_METHOD_COMBO, OnSelchangeMethodCombo)
	ON_BN_CLICKED(IDC_ADD_PEG, OnAddPeg)
	ON_BN_CLICKED(IDC_DEL_PEG, OnDelPeg)
	ON_EN_CHANGE(IDC_SELPEG_POSITION, OnChangeSelpegPosition)
	ON_BN_CLICKED(IDC_RESET_PEGS, OnResetPegs)
	ON_EN_CHANGE(IDC_NEWPEG_POSITION, OnChangeNewpegPosition)
	ON_EN_CHANGE(IDC_NEWPEG_VALUE, OnChangeNewpegValue)
	ON_EN_CHANGE(IDC_SELPEG_VALUE, OnChangeSelpegValue)
	ON_EN_KILLFOCUS(IDC_NEWPEG_VALUE, OnKillfocusNewpegValue)
	ON_EN_KILLFOCUS(IDC_SELPEG_VALUE, OnKillfocusSelpegValue)
	ON_WM_LBUTTONDOWN()
	ON_BN_CLICKED(IDC_DEFAULT, OnDefault)
	ON_BN_CLICKED(ID_SAVEAS, OnSaveAs)
	ON_BN_CLICKED(ID_OPEN, OnOpen)
	ON_BN_CLICKED(IDC_APPLY, OnApply)
    ON_MESSAGE(WM_COMMANDHELP,OnCommandHelp)
	ON_NOTIFY(GC_SELCHANGE, IDC_GRADIENT, OnNotifyChangeSelPeg)
	ON_NOTIFY(GC_PEGMOVE, IDC_GRADIENT, OnNotifyPegMove)
	ON_NOTIFY(GC_PEGMOVED, IDC_GRADIENT, OnNotifyPegMove)
	ON_NOTIFY(GC_PEGREMOVED, IDC_GRADIENT, OnNotifyPegRemoved)
	ON_NOTIFY(GC_CREATEPEG, IDC_GRADIENT, OnNotifyDoubleClickCreate)
	//ON_NOTIFY(GC_EDITPEG, IDC_GRADIENT, OnNotifyEditPeg)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGradientDlg message handlers

BOOL CGradientDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	m_wndGradientCtrl.SetGradient(*CPrjDoc::GetGradientPtr(m_iTyp));
	m_wndGradientCtrl.SetPegSide(TRUE,FALSE); //right/up side: pegs_enable(FALSE)
	m_wndGradientCtrl.SetPegSide(FALSE,TRUE); //left/down side: pegs_enable(TRUE)
	//The control window dimensions determine gradient direction by default,
	//but this is more efficient --
	m_wndGradientCtrl.SetOrientation(CGradientCtrl::ForceHorizontal);

	//Required function call!
	//The default gradient width is that of the control window
	m_wndGradientCtrl.SetGradientWidth(86); //remaining width reserved for histogram

	CGradient &grad = m_wndGradientCtrl.GetGradient();

	//Set value range of current dataset --
	CPrjDoc::GetValueRange(&m_fMinValue,&m_fMaxValue,m_iTyp);
	if(grad.StartValue()>=grad.EndValue()) {
		CPrjDoc::InitGradValues(grad,m_fMinValue,m_fMaxValue,m_iTyp);
	}
	
	CPrjDoc::FillHistArray(m_wndGradientCtrl.GetHistValue(),m_wndGradientCtrl.GetHistLength(),
		grad.m_fStartValue,grad.m_fEndValue,m_iTyp);

	//Set static control text depending on m_iTyp --
	CString s(m_iTyp?((m_iTyp==1)?"Date":"Component F-Ratio"):"Elevation");
	SetWndTitle(this,0,(LPCSTR)s);
	SetWndTitle(GetDlgItem(IDC_ST_RANGETYPE),0,(LPCSTR)s);

	if(!m_iTyp) s+=LEN_ISFEET()?" (ft):":" (m):";
	else s+=':';
	GetDlgItem(IDC_ST_SELPOS)->SetWindowText(s);
	GetDlgItem(IDC_ST_NEWPOS)->SetWindowText(s);

	//Update static values --
	UpdateFloatBox(IDC_ST_MINVALUE,m_fMinValue,m_iTyp);
	UpdateFloatBox(IDC_ST_MAXVALUE,m_fMaxValue,m_iTyp);
	UpdateFloatBox(IDC_ST_STARTVALUE,grad.StartValue(),m_iTyp);
	UpdateFloatBox(IDC_ST_ENDVALUE,grad.EndValue(),m_iTyp);

	UpdateFloatBox(IDC_NEWPEG_POSITION,m_NewPegPosition=0.5);
	UpdateValueBox(IDC_NEWPEG_VALUE,m_NewPegPosition);

	m_SelPegPosition=0.0;
	SelectPeg(PEG_START);

	m_ToolTips.Create(this);
	m_SelPegColor.AddToolTip(&m_ToolTips);

	Enable(IDC_APPLY,CPrjDoc::IsActiveFrame());

	//GetDlgItem(IDCANCEL)->SetFocus();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CGradientDlg::OnSelchangeMethodCombo() 
{
	int sel = ((CComboBox *)GetDlgItem(IDC_METHOD_COMBO))->GetCurSel();
	if(sel == -1) return;
	m_wndGradientCtrl.GetGradient().SetInterpolationMethod((CGradient::InterpolationMethod)sel);
	m_wndGradientCtrl.Invalidate();
	m_bChanged=TRUE;
}

void CGradientDlg::OnAddPeg() 
{
	AddAndSelectPeg(m_wndGradientCtrl.GetGradient().ColorFromPosition(m_NewPegPosition),m_NewPegPosition);
}

void CGradientDlg::OnDelPeg() 
{
	CGradient &grad = m_wndGradientCtrl.GetGradient();
	int selindex = m_wndGradientCtrl.GetSelIndex();

	if(selindex<PEG_NONE) {
		//We are deleting an end peg. Let it absorb the adjacent peg's value and color --
		ASSERT(grad.GetPegCount()>0);
		int index=(selindex==PEG_START)?0:(grad.GetPegCount()-1);
		CPeg peg = grad.GetPeg(index);
		grad.SetPeg(selindex,peg.color,0); //last param (position) ignored here
		m_SelPegColor.SetColor(peg.color);
		double f=peg.position;
		grad.RemovePeg(index);
		//Reposition the remaining interior pegs (f=deleted pos)
		index=grad.GetPegCount();
		while(index--) {
			double fPos=grad.GetPeg(index).position;
			if(selindex==PEG_START) {
				if(f<1.0) fPos=(fPos-f)/(1.0-f);
			}
			else if(f>0.0) fPos/=f;
			grad.ShiftPegToPosition(index,fPos);
		}
		f=grad.ValueFromPos(f);
		UpdateRangeEnd(selindex,f);
		UpdateFloatBox(IDC_SELPEG_VALUE,f,m_iTyp);
		if(!grad.GetPegCount()) Enable(IDC_DEL_PEG,FALSE);
	}
	else {
		grad.RemovePeg(selindex);
		if(selindex==grad.GetPegCount()) {
			//deleting rightmost peg --
			if(--selindex<0) selindex=PEG_START;
		}
		//else id stays the same

		SelectPeg(selindex);
		m_wndGradientCtrl.Invalidate();
	}
	m_bChanged=TRUE; //done in notify msg also
}

long CGradientDlg::OnSelEndOK(UINT /*lParam*/, LONG wParam)
{
	CGradient &gradient = m_wndGradientCtrl.GetGradient();
	int selindex = m_wndGradientCtrl.GetSelIndex();

	if(wParam!=IDC_SELPEG_COLOR || selindex==PEG_NONE || selindex==PEG_BACKGROUND) return 0;
	const CPeg &peg = gradient.GetPeg(selindex);
	gradient.SetPeg(selindex, m_SelPegColor.GetColor(), peg.position);
	
	m_wndGradientCtrl.Invalidate();
	m_bChanged=TRUE;

	return 0;
}

void CGradientDlg::UpdateSelPegText(int index)
{
	CString str("Selected Color Peg ");
	LPCSTR p=NULL;
	switch(index) {
		case PEG_NONE: p="[None]"; break;
		case PEG_START: p="[Start]"; break;
		case PEG_END: p="[End]"; break;
		default: break;
	}
	if(!p) {
		CString append;
		append.Format("- %d",index);
		str+=append;
	}
	else str+=p;

	GetDlgItem(IDC_ST_SELPEG)->SetWindowText(str);
	m_SelPegIndex=index;
}

void CGradientDlg::OnNotifyChangeSelPeg(NMHDR * pNotifyStruct, LRESULT *result)
{
	PegNMHDR* pPegNotifyStruct = (PegNMHDR*)pNotifyStruct;
	
	if(pPegNotifyStruct->index == PEG_NONE)
	{
		ASSERT(FALSE);
		m_SelPegColor.EnableWindow(FALSE);
		Enable(IDC_SELPEG_POSITION,FALSE);
		Enable(IDC_SELPEG_VALUE,FALSE);
		Enable(IDC_DEL_PEG,FALSE);
	}
	else
	{	
		ASSERT(pPegNotifyStruct->peg.position!=-1);
		m_SelPegColor.EnableWindow(TRUE);
		m_SelPegColor.SetColor(pPegNotifyStruct->peg.color);
		UpdateFloatBox(IDC_SELPEG_POSITION,m_SelPegPosition=pPegNotifyStruct->peg.position);
		UpdateValueBox(IDC_SELPEG_VALUE,m_SelPegPosition);

		Enable(IDC_SELPEG_POSITION,pPegNotifyStruct->index>=0);
		Enable(IDC_SELPEG_VALUE,TRUE);
		Enable(IDC_DEL_PEG,m_wndGradientCtrl.GetGradient().GetPegCount()>0);

	}
	UpdateSelPegText(pPegNotifyStruct->index);

	*result = 0;
}

void CGradientDlg::OnNotifyPegMove(NMHDR * pNotifyStruct, LRESULT *result)
{
	PegNMHDR* pPegNotifyStruct = (PegNMHDR*)pNotifyStruct;
	m_SelPegPosition=pPegNotifyStruct->peg.position;
	UpdateFloatBox(IDC_SELPEG_POSITION,m_SelPegPosition);
	UpdateValueBox(IDC_SELPEG_VALUE,m_SelPegPosition);
	if(m_SelPegIndex!=pPegNotifyStruct->index) UpdateSelPegText(pPegNotifyStruct->index); 
	m_bChanged=TRUE;
	*result = 0;
}

void CGradientDlg::OnNotifyPegRemoved(NMHDR* , LRESULT *result)
{
	OnDelPeg();
	*result = 0;
}

void CGradientDlg::SelectPeg(int idx)
{
	m_wndGradientCtrl.SetSelIndex(idx);
	LRESULT result;
	PegNMHDR nmhdr;
	nmhdr.peg=m_wndGradientCtrl.GetSelPeg();
	nmhdr.index = idx;
	OnNotifyChangeSelPeg((NMHDR*) &nmhdr, &result);
}

void CGradientDlg::AddAndSelectPeg(COLORREF color,double position)
{
	SelectPeg(m_wndGradientCtrl.GetGradient().AddPeg(color,position));
	m_wndGradientCtrl.Invalidate();
	m_bChanged=TRUE;
}

void CGradientDlg::OnNotifyDoubleClickCreate(NMHDR * pNotifyStruct, LRESULT *result)
{
	PegCreateNMHDR* pPegCreateNotifyStruct = (PegCreateNMHDR*)pNotifyStruct;
	AddAndSelectPeg(pPegCreateNotifyStruct->color,pPegCreateNotifyStruct->position);
	*result = 0;
}

BOOL CGradientDlg::ChangePosition(UINT idPos,UINT idVal,double &fNewPos) 
{
	//We've manually editied a position field --
	CString buf;
	double dPos;
	GetDlgItem(idPos)->GetWindowText(buf);
	if(!CheckFlt(buf,&dPos,0.0,1.0,4)) {
		UpdateFloatBox(idPos,fNewPos);
		return FALSE;
	}
	UpdateValueBox(idVal,fNewPos=dPos);
	return TRUE;
}

void CGradientDlg::OnChangeSelpegPosition() 
{
	if(m_bUpdating) return;
	if(ChangePosition(IDC_SELPEG_POSITION,IDC_SELPEG_VALUE,m_SelPegPosition)) {
		//We've successfully edited the position field --
		UpdateSelPegText(m_wndGradientCtrl.MoveSelected(m_SelPegPosition,TRUE));
		m_bChanged=TRUE;
	}
}

void CGradientDlg::OnChangeNewpegPosition() 
{
	if(m_bUpdating) return;
	ChangePosition(IDC_NEWPEG_POSITION,IDC_NEWPEG_VALUE,m_NewPegPosition);
}

void CGradientDlg::OnResetPegs() 
{
	CGradient &grad = m_wndGradientCtrl.GetGradient();

	if(m_iTyp==2) {
		CPrjDoc::InitGradValues(grad,m_fMinValue,m_fMaxValue,2);
		m_SelPegPosition=0.0;
		SelectPeg(PEG_START);
	}
	else if(m_fMinValue==grad.StartValue() && m_fMaxValue==grad.EndValue()) return;

	m_bChanged=TRUE;

	grad.SetStartValue(m_fMinValue);
	UpdateFloatBox(IDC_ST_STARTVALUE,m_fMinValue,m_iTyp);
	grad.SetEndValue(m_fMaxValue);
	UpdateFloatBox(IDC_ST_ENDVALUE,m_fMaxValue,m_iTyp);

	UpdateHistArray();
	m_wndGradientCtrl.Invalidate(); //Tooltips synchronized

	int index=m_wndGradientCtrl.GetSelIndex();
	UpdateFloatBox(IDC_SELPEG_VALUE,grad.ValueFromPos(grad.GetPeg(index).position),m_iTyp);
	CString buf;
	GetDlgItem(IDC_NEWPEG_POSITION)->GetWindowText(buf);
	UpdateFloatBox(IDC_NEWPEG_VALUE,grad.ValueFromPos((double)atof(buf)),m_iTyp);
}

void CGradientDlg::ShowRangeError()
{
	int index=m_wndGradientCtrl.GetSelIndex();

	if(index>=PEG_NONE) {
		CString start,end;
		GetDlgItem(IDC_ST_STARTVALUE)->GetWindowText(start);
		GetDlgItem(IDC_ST_ENDVALUE)->GetWindowText(end);
		CMsgBox(MB_ICONINFORMATION,IDS_ERR_VALUERANGE2,(LPCSTR)start,(LPCSTR)end);
	}
	else CMsgBox(MB_ICONINFORMATION,IDS_ERR_VALUELIMIT);
}

void CGradientDlg::UpdateHistArray()
{
	CGradient &grad = m_wndGradientCtrl.GetGradient();
	CPrjDoc::FillHistArray(m_wndGradientCtrl.GetHistValue(),m_wndGradientCtrl.GetHistLength(),
	grad.m_fStartValue,grad.m_fEndValue,m_iTyp);
}

int CGradientDlg::UpdateRangeEnd(int index,double fVal)
{
	//We're updating the value of a selected endpoint peg --
	CGradient &grad = m_wndGradientCtrl.GetGradient();
	int iRet=0;

	if(index==PEG_START) {
	  if(fVal>=grad.EndValue()) iRet=-1;
	}
	else if(fVal<=grad.StartValue()) iRet=-1;

	if(!iRet) {
		if(index==PEG_START) {
			grad.SetStartValue(fVal);
			UpdateFloatBox(IDC_ST_STARTVALUE,fVal,m_iTyp);
		}
		else {
			grad.SetEndValue(fVal);
			UpdateFloatBox(IDC_ST_ENDVALUE,fVal,m_iTyp);
		}

		UpdateHistArray();
		m_wndGradientCtrl.Invalidate();

		CString buf;
		GetDlgItem(IDC_NEWPEG_POSITION)->GetWindowText(buf);
		UpdateFloatBox(IDC_NEWPEG_VALUE,grad.ValueFromPos(atof(buf)),m_iTyp);
	}
	return iRet;
}

BOOL CGradientDlg::ChangeValue(UINT idVal,UINT idPos,double &fNewPos,int index) 
{
	CGradient &grad = m_wndGradientCtrl.GetGradient();
	CString buf;
	GetDlgItem(idVal)->GetWindowText(buf);
	double fPos;
	int j=0;

	if(m_iTyp==1) {
		j=trx_DateStrToJul(buf);
		if(j>=0) {
			if(index<PEG_NONE) {
				ASSERT(idVal==IDC_SELPEG_VALUE);
				//display error if not correct endpoint --
				j=UpdateRangeEnd(index,(double)j);
			}
			else if(j<grad.IntStartValue() || j>grad.IntEndValue()) {
				j=-1;
			}
			else fPos=grad.PosFromValue(j);
		}
	}
	else {
		double f;
		buf.TrimRight();
		if(!IsNumeric(buf)) j=-1;
		else {
			f=atof(buf);
			if(!m_iTyp && LEN_ISFEET()) f=LEN_INVSCALE(f);
			if(index<PEG_NONE) {
				ASSERT(idVal==IDC_SELPEG_VALUE);
				//display error if not correct endpoint --
				j=UpdateRangeEnd(index,f);
			}
			else if(f<grad.StartValue() || f>grad.EndValue()) {
				j=-1;
			}
			else fPos=grad.PosFromValue(f);
		}
	}

	if(index>=PEG_NONE) {
		Enable(idPos,j>=0);
		if(j>=0) UpdateFloatBox(idPos,fNewPos=fPos);
	}

	return j>=0;
}

void CGradientDlg::OnChangeNewpegValue() 
{
	if(m_bUpdating) return;

	BOOL bValid=ChangeValue(IDC_NEWPEG_VALUE,IDC_NEWPEG_POSITION,m_NewPegPosition,PEG_NONE);

	Enable(IDC_ADD_PEG,bValid);
}

void CGradientDlg::OnChangeSelpegValue() 
{
	if(m_bUpdating) return;
	int index=m_wndGradientCtrl.GetSelIndex();
	if(m_bSelValid=ChangeValue(IDC_SELPEG_VALUE,IDC_SELPEG_POSITION,m_SelPegPosition,index)) {
		if(index>PEG_NONE) m_wndGradientCtrl.MoveSelected(m_SelPegPosition,TRUE);
		m_bChanged=TRUE;
	}
}

void CGradientDlg::OnKillfocusNewpegValue() 
{
	if(!GetDlgItem(IDC_NEWPEG_POSITION)->IsWindowEnabled()) {
		Enable(IDC_NEWPEG_POSITION,TRUE);
		Enable(IDC_ADD_PEG,TRUE);
		CString buf;
		GetDlgItem(IDC_NEWPEG_POSITION)->GetWindowText(buf);
		UpdateValueBox(IDC_NEWPEG_VALUE,(double)atof(buf));
	}
}

void CGradientDlg::OnKillfocusSelpegValue() 
{
	if(!m_bSelValid) {
		m_bSelValid=TRUE;
		ShowRangeError();
		if(m_wndGradientCtrl.GetSelIndex()>=0) Enable(IDC_SELPEG_POSITION,TRUE);
		CString buf;
		GetDlgItem(IDC_SELPEG_POSITION)->GetWindowText(buf);
		UpdateValueBox(IDC_SELPEG_VALUE,(double)atof(buf));
		GetDlgItem(IDC_SELPEG_VALUE)->SetFocus();
	}
}

void CGradientDlg::OnLButtonDown(UINT nFlags, CPoint point) 
{
	OnKillfocusNewpegValue();
	OnKillfocusSelpegValue();
	CDialog::OnLButtonDown(nFlags, point);
}

LRESULT CGradientDlg::OnCommandHelp(WPARAM wNone, LPARAM lParam)
{
	AfxGetApp()->WinHelp(22);
	return TRUE;
}

void CGradientDlg::OnDefault() 
{
	CGradient &grad = m_wndGradientCtrl.GetGradient();
	for(int i=grad.GetPegCount();i;) {
		grad.RemovePeg(--i);
	}

	CPrjDoc::InitGradient(&grad,m_iTyp);
	m_iInterpMethod = grad.GetInterpolationMethod();
	((CComboBox *)GetDlgItem(IDC_METHOD_COMBO))->SetCurSel(m_iInterpMethod);
	m_SelPegPosition=0.0;
	SelectPeg(PEG_START);

	OnResetPegs();
	m_wndGradientCtrl.Invalidate(); //Tooltips synchronized
	m_bChanged=TRUE;
}

CFileDialog * CGradientDlg::GetFileDialog(BOOL bSave)
{
	LPSTR pathName;
	CString fspec;
	char ext[]="ntae";
	if(m_iTyp) ext[3]=(m_iTyp==1)?'d':'s';

	pathName=CPrjDoc::m_pReviewDoc->WorkPath(CPrjDoc::m_pReviewNode,CPrjDoc::TYP_NTAC);
	_strlwr(trx_Stpnam(pathName));
	pathName[strlen(pathName)-1]=ext[3];

	char *name=m_iTyp?((m_iTyp==1)?"Date":"Statistics"):"Elevation";
	fspec.Format("%s gradient (*.%s)|*.%s||",
		m_iTyp?((m_iTyp==1)?"Date":"Statistics"):"Elevation",
		ext,ext);

	return new CFileDialog(bSave,ext,pathName,
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_EXPLORER,fspec);

}

void CGradientDlg::OnSaveAs() 
{
	CFileDialog *pdlg;

	VERIFY(pdlg=GetFileDialog(FALSE));
	if(!pdlg) return;

	if(pdlg->DoModal()==IDOK)
	{
		CFile file(pdlg->GetPathName(), CFile::modeWrite | CFile::modeCreate | CFile::shareDenyWrite );
		CArchive ar(&file, CArchive::store);
		m_wndGradientCtrl.GetGradient().Serialize(ar);
	}
	delete pdlg;
}

void CGradientDlg::OnOpen() 
{
	CFileDialog *pdlg;
		
	VERIFY(pdlg=GetFileDialog(TRUE));
	if(!pdlg) return;

	*(pdlg->m_ofn.lpstrFile)=0;

	if(pdlg->DoModal()==IDOK)
	{
		CFile file(pdlg->GetPathName(), CFile::modeRead | CFile::shareDenyWrite );
		CArchive ar(&file, CArchive::load);

		CGradient &grad = m_wndGradientCtrl.GetGradient();
	
		grad.Serialize(ar);

		m_iInterpMethod = grad.GetInterpolationMethod();
		((CComboBox *)GetDlgItem(IDC_METHOD_COMBO))->SetCurSel(m_iInterpMethod);
		UpdateValueBox(IDC_NEWPEG_VALUE,m_NewPegPosition);
		UpdateFloatBox(IDC_ST_STARTVALUE,grad.StartValue(),m_iTyp);
		UpdateFloatBox(IDC_ST_ENDVALUE,grad.EndValue(),m_iTyp);

		UpdateHistArray();

		m_SelPegPosition=0.0;
		SelectPeg(PEG_START);

		m_wndGradientCtrl.Invalidate(); //Tooltips synchronized
		m_bChanged=TRUE;
	}
	delete pdlg;
}

void CGradientDlg::OnApply() 
{
	CGradient *pOldGradient=pSV->m_Gradient[m_iTyp];
	pSV->m_Gradient[m_iTyp]=&m_wndGradientCtrl.GetGradient();
	pSV->ApplyClrToFrames(m_clr,m_id);
	pSV->m_Gradient[m_iTyp]=pOldGradient;
}

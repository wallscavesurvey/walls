// AdvSearchDlg.cpp : implementation file
//

#include "stdafx.h"
#include "WallsMap.h"
#include "WallsMapDoc.h"
#include "ShowShapeDlg.h"
#include "AdvSearchDlg.h"

// CAdvSearchDlg dialog

IMPLEMENT_DYNAMIC(CAdvSearchDlg, CDialog)

bool CAdvSearchDlg::m_bLastViewOnly=false;

CAdvSearchDlg::CAdvSearchDlg(CWallsMapDoc *pDoc,CWnd* pParent /*=NULL*/)
	: CDialog(CAdvSearchDlg::IDD, pParent)
	, m_pDoc(pDoc)
	, m_rbSelMatched(0)
	, m_iSelOpt(NULL)
	, m_bSelFldChange(false)
	, m_wFlags(0)
{
	if(app_pShowDlg) m_pDoc=app_pShowDlg->m_pDoc;

	m_phist_find=pDoc->m_phist_find;
	if(m_phist_find) {
		m_wFlags=m_phist_find->GetFirstFlags();
		m_FindString=m_phist_find->GetFirstString();
	}
	if(m_bViewOnly=m_bLastViewOnly) m_wFlags|=F_VIEWONLY;
	m_bMatchCase=(m_wFlags&F_MATCHCASE)!=0;
	m_bMatchWholeWord=(m_wFlags&F_MATCHWHOLEWORD)!=0;
}

CAdvSearchDlg::~CAdvSearchDlg()
{
}

void CAdvSearchDlg::ShowNoMatches()
{
	CString s;
	if(!CWallsMapDoc::m_bSelectionLimited) {
		LPCSTR p1="un";
		LPCSTR p2="";
		switch(GET_FTYP(m_wFlags)) {
			case FTYP_SELMATCHED:
				p1="";
				break;

			case FTYP_SELUNMATCHED:
				break;

			case FTYP_ADDMATCHED:
				p1=p2;

			case FTYP_ADDUNMATCHED:
				p2=" to add";
				break;

			case FTYP_KEEPUNMATCHED:
				p1=p2;

			case FTYP_KEEPMATCHED:
				p2=" to remove";
				break;
		}
		s.Format("No %smatching items found%s!",p1,p2);
	}
	else s.SetString("Search aborted.");
	GetDlgItem(IDC_ST_NOMATCHES)->ShowWindow(SW_SHOW);
	GetDlgItem(IDC_ST_NOMATCHES)->SetWindowText(s);
}

void CAdvSearchDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_SEL_MATCHCASE, m_bMatchCase);
	DDX_Check(pDX, IDC_SEL_WHOLEWORDS, m_bMatchWholeWord);
	DDX_Control(pDX, IDC_FIND_COMBO, m_cbFind);
	DDX_Text(pDX, IDC_FIND_COMBO, m_FindString);
	DDV_MaxChars(pDX, m_FindString, 80);
	DDX_Radio(pDX, IDC_SELMATCHED, m_rbSelMatched);
	DDX_Control(pDX, IDC_LIST1, m_list1);
	DDX_Control(pDX, IDC_LIST2, m_list2);
	DDX_Control(pDX, IDC_LIST3, m_list3);
	DDX_Control(pDX, IDC_LIST4, m_list4);
	DDX_Check(pDX, IDC_SEL_VIEWONLY, m_bViewOnly);

	if(pDX->m_bSaveAndValidate) {
		bool bFail=false;
		if(m_bSelFldChange) UpdateSelFldChange();

		if(!bFail && !m_pDoc->IsSetSearchable(TRUE)) {
			CMsgBox(MB_ICONINFORMATION,
				"%s --\nYour layer and field settings either exclude all layers from searches "
				"or exclude all fields from being searched!",(LPCSTR)m_pDoc->GetTitle());
			bFail=true;
		}
		else if(m_bMatchWholeWord) m_FindString.Trim();
		else {
			CString s(m_FindString);
			s.Trim();
			if(s.IsEmpty()) m_FindString.Empty();
			else if(s.Compare(m_FindString) && (!m_phist_find || m_FindString.Compare(m_phist_find->GetFirstString()))) {
				UINT id=AfxMessageBox("Your target string has leading or trailing spaces. "
					"Should they be removed prior to the search?",MB_YESNOCANCEL);
				if(id==IDYES){
					m_cbFind.SetWindowText(m_FindString=s);
				}
				else if(id!=IDNO) bFail=true;
			}
		}

		if(!bFail && m_FindString.IsEmpty() && 
			IDOK!=CMsgBox(MB_OKCANCEL|MB_ICONQUESTION,
			"The Find box has no text. Select OK to match %sempty fields with this search.",(m_rbSelMatched&1)?"non":""))
			 bFail=true;
		
		if(bFail) {
			pDX->m_idLastControl=IDC_FIND_COMBO;
			pDX->m_bEditLastControl=TRUE;
			pDX->Fail();
			return;
		}

		m_wFlags=F_MATCHCASE*m_bMatchCase +
			     F_MATCHWHOLEWORD*m_bMatchWholeWord +
				 F_VIEWONLY*m_bViewOnly +
				 (m_rbSelMatched<<FTYP_SHIFT);

		BeginWaitCursor();
		BOOL bVecModified=m_pDoc->FindByLabel(m_FindString,m_wFlags+F_USEACTIVE);
		EndWaitCursor();

		if(!bVecModified) {
			ShowNoMatches();
			pDX->m_idLastControl=IDC_FIND_COMBO;
			pDX->m_bEditLastControl=TRUE;
			m_cbFind.SetEditSel(0,-1);
			pDX->Fail();
			return;
		}
		//Save advanced search options --
		m_bLastViewOnly=(m_bViewOnly==TRUE);
		SaveLayerOptions();
		ClearVecShpopt(); //not required
		if(!m_phist_find)
			m_phist_find=m_pDoc->m_phist_find=new CTabComboHist(MAX_COMBOHISTITEMS);
		m_phist_find->InsertAsFirst(m_FindString,m_wFlags&F_MATCHMASK);
		m_pDoc->m_bHistoryUpdated=true;
		m_pDoc->SetChanged(1); //for saving search history, no prompt needed to save NTL
	}
}

BEGIN_MESSAGE_MAP(CAdvSearchDlg, CDialog)
	ON_BN_CLICKED(IDC_MOVEALL_LEFT, &CAdvSearchDlg::OnBnClickedMoveallLeft)
	ON_BN_CLICKED(IDC_MOVEALL_RIGHT, &CAdvSearchDlg::OnBnClickedMoveallRight)
	ON_BN_CLICKED(IDC_MOVELEFT, &CAdvSearchDlg::OnBnClickedMoveLayerLeft)
	ON_BN_CLICKED(IDC_MOVERIGHT, &CAdvSearchDlg::OnBnClickedMoveLayerRight)
	ON_LBN_SELCHANGE(IDC_LIST2,OnSelChangeList2)
	ON_LBN_DBLCLK(IDC_LIST1,OnListDblClk)
	ON_LBN_DBLCLK(IDC_LIST2,OnListDblClk)
	ON_LBN_DBLCLK(IDC_LIST3,OnListDblClk)
	ON_LBN_DBLCLK(IDC_LIST4,OnListDblClk)
	ON_BN_CLICKED(IDC_MOVERIGHT2, &CAdvSearchDlg::OnBnClickedMoveFldRight)
	ON_BN_CLICKED(IDC_MOVELEFT2, &CAdvSearchDlg::OnBnClickedMoveFldLeft)
	ON_CBN_SELCHANGE(IDC_FIND_COMBO, OnSelChangeFindCombo)
	ON_CBN_EDITCHANGE(IDC_FIND_COMBO, OnEditChangeFindCombo)
	ON_MESSAGE(WM_CTLCOLORLISTBOX, OnColorListbox)
	ON_MESSAGE(WM_CTLCOLORSTATIC, OnColorListbox)
	ON_BN_CLICKED(IDCANCEL, &CAdvSearchDlg::OnBnClickedCancel)
	ON_BN_CLICKED(IDC_MOVEALL_LEFT2, &CAdvSearchDlg::OnBnClickedMoveAllFldsLeft)
	ON_BN_CLICKED(IDC_MOVEALL_RIGHT2, &CAdvSearchDlg::OnBnClickedMoveAllFldsRight)
	ON_BN_CLICKED(IDC_SAVE_DEFAULT,&CAdvSearchDlg::OnBnClickedSaveDefault)
	ON_BN_CLICKED(IDC_LOAD_DEFAULT,&CAdvSearchDlg::OnBnClickedLoadDefault)
	ON_MESSAGE(WM_COMMANDHELP,OnCommandHelp)
	//ON_BN_CLICKED(IDC_CLEAR_HISTORY, &CAdvSearchDlg::OnBnClickedClearHistory)
	ON_REGISTERED_MESSAGE(WM_CLEAR_HISTORY,OnClearHistory)
END_MESSAGE_MAP()

BOOL CAdvSearchDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	SetWndTitle(this,IDS_ADVSRCH_TITLE,m_pDoc->GetTitle());

	if(m_phist_find)
		m_cbFind.SetCustomCmd(WM_CLEAR_HISTORY,"Clear search history...",this);

	m_cbFind.LoadHistory(m_phist_find); //adds at least one empty string

	m_bActiveChange=m_pDoc->InitVecShpopt(true);

	InitLayerSettings();

	ASSERT(!app_pShowDlg || app_pShowDlg->m_pDoc==m_pDoc);

	EnableCurrent(app_pShowDlg && app_pShowDlg->NumSelected());

	GetDlgItem(IDC_FIND_COMBO)->SetFocus();

	return FALSE;  // return TRUE unless you set the focus to a control
}

void CAdvSearchDlg::UpdateButtons()
{
	BOOL b=m_list1.GetCount()>0;
	Enable(IDC_MOVERIGHT,b);
	Enable(IDC_MOVEALL_RIGHT,b);

	b=m_list2.GetCount()>0;
	Enable(IDC_MOVELEFT,b);
	Enable(IDC_MOVEALL_LEFT,b);

	b=m_list3.GetCount()>0;
	Enable(IDC_MOVERIGHT2,b);
	Enable(IDC_MOVEALL_RIGHT2,b);

	b=m_list4.GetCount()>0;
	Enable(IDC_MOVELEFT2,b);
	Enable(IDC_MOVEALL_LEFT2,b);

	b=(m_bActiveChange==true);
	Enable(IDC_SAVE_DEFAULT,b);
	Enable(IDC_LOAD_DEFAULT,b);
}

void CAdvSearchDlg::UpdateSelFldChange()
{
	ASSERT(m_iSelOpt>=0);
	int n=m_list4.GetCount();
	VEC_BYTE flds(n,0);
	while(n--) flds[n]=(BYTE)m_list4.GetItemData(n);
	vec_shpopt[m_iSelOpt].ReplaceFlds(flds);
	m_bSelFldChange=false;
	m_bActiveChange=true;
}

void CAdvSearchDlg::InitFlds()
{
	if(m_bSelFldChange) UpdateSelFldChange();

	m_list3.ResetContent();
	m_list4.ResetContent();

	m_iSelOpt=-1;

	int i=m_list2.GetCurSel();
	if(i<0) return;

	m_list3.SetRedraw(0);
	m_list4.SetRedraw(0);

	m_iSelOpt=m_list2.GetItemData(i);
	ASSERT(m_iSelOpt>=0);
	
	VEC_BYTE *pFlds=vec_shpopt[m_iSelOpt].pvSrchFlds;
	CShpDBF *pdb=vec_shpopt[m_iSelOpt].pShp->m_pdb;
	int nFlds=pdb->NumFlds();
	VEC_BYTE vSrchFlgs(nFlds+1,0);

	for(VEC_BYTE::iterator it=pFlds->begin();it!=pFlds->end();it++) {
		vSrchFlgs[*it]=1;  //field *it is searchable
		VERIFY((i=m_list4.AddString(pdb->FldNamPtr(*it)))>=0);
		m_list4.SetItemData(i,(DWORD_PTR)*it);
	}

	int nFldsUsed=0,nFldSel=-1;

	for(int n=1;n<=nFlds;n++) {
		if(!vSrchFlgs[n]) {
			VERIFY((i=m_list3.AddString(pdb->FldNamPtr(n)))>=0);
			m_list3.SetItemData(i,(DWORD_PTR)n);
		}
	}
	m_list3.SetCurSel(0); //ignore possible errors
	m_list3.SetRedraw(1);
	m_list4.SetCurSel(0);
	m_list4.SetRedraw(1);
}

void CAdvSearchDlg::MoveAllLayers(bool bExclude)
{
	m_list1.SetRedraw(0);
	m_list1.ResetContent();
	m_list2.SetRedraw(0);
	m_list2.ResetContent();
	CListBox &list=bExclude?m_list1:m_list2;

	int i=0;
	CString path;
	for(it_shpopt it=vec_shpopt.begin();it!=vec_shpopt.end();it++,i++) {
		it->bSearchExcluded=bExclude;
		VERIFY(i==list.AddString(it->pShp->LayerPath(path)));
		list.SetItemData(i,(DWORD_PTR)i);
	}
	list.SetCurSel(0);
	m_list1.SetRedraw(1);
	m_list2.SetRedraw(1);
	InitFlds();
	m_bActiveChange=true;
	UpdateButtons();
}

void CAdvSearchDlg::OnBnClickedMoveallLeft()
{
	MoveAllLayers(true); //exclude all layers
}

void CAdvSearchDlg::OnBnClickedMoveallRight()
{
	MoveAllLayers(false); //include all layers
}

void CAdvSearchDlg::MoveOne(CListBox &list1,CListBox &list2,int iSel1)
{
	if(iSel1<0) return;
	CString s;
	list1.GetText(iSel1,s);
	int iSeq=list1.GetItemData(iSel1);
	ASSERT(iSeq!=-1);
	VERIFY(list1.DeleteString(iSel1)!=LB_ERR);
	if(iSel1==list1.GetCount()) iSel1--;
	list1.SetCurSel(iSel1);
	iSel1=list2.GetCount();
	for(;iSel1;iSel1--) {
		if(iSeq>(int)list2.GetItemData(iSel1-1)) break;
	}
	if(iSel1==list2.GetCount()) iSel1=-1;
	VERIFY((iSel1=list2.InsertString(iSel1,s))>=0);
	list2.SetItemData(iSel1,iSeq);
	list2.SetCurSel(iSel1);
	vec_shpopt[iSeq].bSearchExcluded=!vec_shpopt[iSeq].bSearchExcluded;
	InitFlds();
	m_bActiveChange=true;
	UpdateButtons();
}

void CAdvSearchDlg::OnBnClickedMoveLayerLeft()
{
	MoveOne(m_list2,m_list1,m_list2.GetCurSel());
}

void CAdvSearchDlg::OnBnClickedMoveLayerRight()
{
	MoveOne(m_list1,m_list2,m_list1.GetCurSel());
}

void CAdvSearchDlg::OnSelChangeList2()
{
	InitFlds();
}

void CAdvSearchDlg::OnBnClickedMoveAllFldsLeft()
{
	ASSERT(m_iSelOpt>=0);
	vec_shpopt[m_iSelOpt].ReplaceFlds(VEC_BYTE());
	m_bSelFldChange=false;
	InitFlds();
	m_bActiveChange=true;
	UpdateButtons();
}

void CAdvSearchDlg::OnBnClickedMoveAllFldsRight()
{
	ASSERT(m_iSelOpt>=0);
	SHPOPT &so=vec_shpopt[m_iSelOpt];
	int nf=so.pShp->m_pdb->NumFlds();
	VEC_BYTE vf(nf,0);
	for(int f=0;f<nf;f++) vf[f]=f+1;
	so.ReplaceFlds(vf);
	m_bSelFldChange=false;
	InitFlds();
	m_bActiveChange=true;
	UpdateButtons();
}

void CAdvSearchDlg::OnBnClickedMoveFldRight()
{
	int i=m_list3.GetCurSel();
	ASSERT(i>=0 && m_iSelOpt>=0);
	if(i<0) return;
	CString s1;
	m_list3.GetText(i,s1);
	int f=m_list3.GetItemData(i);
	m_list3.DeleteString(i);
	if(i==m_list3.GetCount()) i--;
	m_list3.SetCurSel(i);
	i=m_list4.GetCurSel()+1;
	if(i==m_list4.GetCount()) i=-1;
	VERIFY((i=m_list4.InsertString(i,s1))>=0);
	m_list4.SetItemData(i,f);
	m_list4.SetCurSel(i);
	m_bActiveChange=true;
	UpdateButtons();
	m_bSelFldChange=true;
}

void CAdvSearchDlg::OnBnClickedMoveFldLeft()
{
	int i=m_list4.GetCurSel();
	if(i<0) return;
	CString s;
	m_list4.GetText(i,s);
	int f=m_list4.GetItemData(i);
	m_list4.DeleteString(i);
	if(i==m_list4.GetCount()) i--;
	m_list4.SetCurSel(i);
	i=m_list3.GetCount();
	for(;i;i--) {
		if(f>(int)m_list3.GetItemData(i-1)) break;
	}
	if(i==m_list3.GetCount()) i=-1;
	VERIFY((i=m_list3.InsertString(i,s))>=0);
	m_list3.SetItemData(i,f);
	m_list3.SetCurSel(i);
	m_bActiveChange=true;
	UpdateButtons();
	m_bSelFldChange=true;
}

void CAdvSearchDlg::OnListDblClk()
{
	const MSG* pMsg=GetCurrentMessage();
	switch(pMsg->wParam&0xFFFF) {
		case IDC_LIST1 : OnBnClickedMoveLayerRight(); break;
		case IDC_LIST2 : OnBnClickedMoveLayerLeft(); break;
		case IDC_LIST3 : OnBnClickedMoveFldRight(); break;
		case IDC_LIST4 : OnBnClickedMoveFldLeft(); break;
		default : ASSERT(0);
	}
}

LRESULT CAdvSearchDlg::OnColorListbox(WPARAM wParam,LPARAM lParam)
{
	LRESULT lret=Default();
	bool bSet=(HWND)lParam==GetDlgItem(IDC_ST_NOMATCHES)->m_hWnd;

	if(bSet || (HWND)lParam==m_list1.m_hWnd || (HWND)lParam==m_list3.m_hWnd) {
		//lret=(LRESULT)CShpLayer::m_hBrushBkg;
		lret=(LRESULT)::GetSysColorBrush(COLOR_BTNFACE);
		::SetBkColor((HDC)wParam,::GetSysColor(COLOR_BTNFACE));
		if(bSet) ::SetTextColor((HDC)wParam,RGB(0x80,0,0));
	}
	return lret;
}

void CAdvSearchDlg::OnEditChangeFindCombo() 
{
	// TODO: Add your control notification handler code here
	//CString s;
	//m_cbFind.GetWindowText(s);
	//GetDlgItem(IDOK)->EnableWindow(!s.IsEmpty());
	HideNoMatches();
}


void CAdvSearchDlg::OnSelChangeFindCombo() 
{
	if(m_phist_find)
		OnSelChangeFindStr(this,m_phist_find);
	HideNoMatches();
}

void CAdvSearchDlg::InitLayerSettings()
{
	m_list1.SetRedraw(0);
	m_list1.ResetContent();
	m_list2.SetRedraw(0);
	m_list2.ResetContent();
	
	CString path;
	int i,seq=0;
	for(it_shpopt it=vec_shpopt.begin();it!=vec_shpopt.end();it++) {
		it->pShp->LayerPath(path);
		if(it->bSearchExcluded) {
			VERIFY((i=m_list1.AddString(path))>=0);
			m_list1.SetItemData(i,(DWORD_PTR)seq);
		}
		else {
			VERIFY((i=m_list2.AddString(path))>=0);
			m_list2.SetItemData(i,(DWORD_PTR)seq);
		}
		seq++;
	}
	m_list1.SetCurSel(0); //ignore possible errors
	m_list1.SetRedraw(1);
	m_list2.SetCurSel(0);
	m_list2.SetRedraw(1);
	m_bSelFldChange=false;
	InitFlds();
	UpdateButtons();
}

void CAdvSearchDlg::OnBnClickedCancel()
{
	ClearVecShpopt();
	EndDialog(IDCANCEL);
}

void CAdvSearchDlg::SaveLayerOptions()
{
	for(it_shpopt it=vec_shpopt.begin();it!=vec_shpopt.end();it++) {
		ASSERT(it->bChanged || it->pShp->m_pvSrchFlds==it->pvSrchFlds);
		if(it->bChanged) {
			if(it->pShp->m_pvSrchFlds!=&it->pShp->m_vSrchFlds)
				delete it->pShp->m_pvSrchFlds;
			it->pShp->m_pvSrchFlds=it->pvSrchFlds;
			it->bChanged=false; // avoid delete in destructor
		}
		it->pShp->m_bSearchExcluded=it->bSearchExcluded;
	}
}

void CAdvSearchDlg::OnBnClickedSaveDefault()
{
	if(m_bSelFldChange) UpdateSelFldChange(); //obtained from m_list4
	SaveLayerOptions();
	bool bChg=false;
	for(it_shpopt it=vec_shpopt.begin();it!=vec_shpopt.end();it++) {
		ASSERT(!it->bChanged);
		if(it->pShp->m_pvSrchFlds!=&it->pShp->m_vSrchFlds) {
			if(it->pShp->SwapSrchFlds(*it->pvSrchFlds)) bChg=true;
			delete it->pvSrchFlds;
			it->pvSrchFlds=it->pShp->m_pvSrchFlds=&it->pShp->m_vSrchFlds;
		}
		bChg=bChg || it->bSearchExcluded!=it->pShp->IsSearchExcluded();
		it->pShp->SetSearchExcluded(it->bSearchExcluded);
	}
	if(bChg) m_pDoc->SetChanged();
	m_bActiveChange=false;
	NextDlgCtrl();
	Enable(IDC_SAVE_DEFAULT,0);
	Enable(IDC_LOAD_DEFAULT,0);
}

void CAdvSearchDlg::OnBnClickedLoadDefault()
{
	m_pDoc->InitVecShpopt(false);
	for(it_shpopt it=vec_shpopt.begin();it!=vec_shpopt.end();it++) {
		if(it->pShp->m_pvSrchFlds!=it->pvSrchFlds) {
			delete it->pShp->m_pvSrchFlds;
			it->pShp->m_pvSrchFlds=it->pvSrchFlds;
		}
		it->pShp->m_bSearchExcluded=it->bSearchExcluded;
	}
	InitLayerSettings();
	m_bActiveChange=false;
	NextDlgCtrl();
	NextDlgCtrl();
	Enable(IDC_SAVE_DEFAULT,0);
	Enable(IDC_LOAD_DEFAULT,0);
}

LRESULT CAdvSearchDlg::OnCommandHelp(WPARAM,LPARAM)
{
	AfxGetApp()->WinHelp(1012);
	return TRUE;
}

LRESULT CAdvSearchDlg::OnClearHistory(WPARAM fcn,LPARAM)
{
	if(fcn==2) {
		OnOK();
		return TRUE;
	}
	if(m_phist_find && !m_phist_find->IsEmpty()) {
		if(IDYES==CMsgBox(MB_YESNO,"Are you sure you want to clear the list of items previously searched for?\n"
			"(Document preferences are set to %ssave search history in the NTL file.)",m_pDoc->IsSavingHistory()?"":"NOT "))
		{ 
			m_pDoc->SetChanged(1);
			m_pDoc->m_bHistoryUpdated=true;
			delete m_pDoc->m_phist_find;
			m_phist_find=m_pDoc->m_phist_find=NULL;
			m_cbFind.ClearListItems();
		}
	}
	return TRUE;
}

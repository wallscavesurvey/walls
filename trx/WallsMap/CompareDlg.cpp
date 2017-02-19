// CompareDlg.cpp : implementation file
//

#include "stdafx.h"
#include "WallsMap.h"
#include "WallsMapDoc.h"
#include "ShpLayer.h"
#include "FldsIgnoreDlg.h"
#include "CompareDlg.h"

// CCompareDlg dialog

IMPLEMENT_DYNAMIC(CCompareDlg, CDialog)

BOOL CCompareDlg::bLogOutdated = TRUE;

static void InitLogPathName(CShpLayer *pShp,CString &path)
{
	LPSTR p=(LPSTR)pShp->PathName();
	path.SetString(p,trx_Stpnam(p)-p);
	int i=SHP_DBFILE::csEditorName.ReverseFind(' ')+1;
	CString name;
	name.Format("UPD_%s_",(LPCSTR)SHP_DBFILE::csEditorName+i);
	p=GetLocalTimeStr();
	if(p) {
		p[10]=0;
		name+=p;
	}
	path+=name;
}

CCompareDlg::CCompareDlg(CShpLayer *pShp,CString &path,CString &summary,VEC_COMPSHP &vCompShp,WORD **ppwFld,CWnd* pParent /*=NULL*/)
	: CDialog(CCompareDlg::IDD, pParent)
	, m_pShpRef(pShp)
	, m_pathName(path)
	, m_summary(summary)
	, m_vCompShp(vCompShp)
	, m_ppwFld(ppwFld)
	, m_iFlags(0)
	, m_nNumSel(0)
	, m_bIncludeShp(FALSE)
	, m_bIncludeLinked(FALSE)
	, m_bIncludeAll(FALSE)
	, m_bLogOutdated(bLogOutdated)
{
	InitLogPathName(pShp,m_pathName);
	PVEC_XCOMBO pxc=pShp->m_pdbfile->pvxc_ignore;
	if(pxc) {
		m_vFldsIgnored.reserve(pxc->size());
		for(it_vec_xcombo it=pxc->begin(); it!=pxc->end(); it++) {
			m_vFldsIgnored.push_back(it->nFld);
		}
	}
}

CCompareDlg::~CCompareDlg()
{
}

HBRUSH CCompareDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
	HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
	UINT id=pWnd->GetDlgCtrlID();
    if(id==IDC_ST_SCANNING || id==IDC_ST_IGNORED && !m_vFldsIgnored.empty() ) {
		pDC->SetTextColor(RGB(0x80,0,0));
    }
	return hbr;
}

void CCompareDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SHP_CAND, m_lbShpCand);
	DDX_Control(pDX, IDC_SHP_COMP, m_lbShpComp);
	DDX_Control(pDX, IDC_PATHNAME, m_cePathName);
	DDX_Text(pDX, IDC_PATHNAME, m_pathName);
	DDX_Control(pDX, IDC_INCLUDE_SHP, m_cbIncludeShp);
	DDX_Check(pDX, IDC_INCLUDE_SHP, m_bIncludeShp);
	DDX_Control(pDX, IDC_INCLUDE_LINKED, m_cbIncludeLinked);
	DDX_Check(pDX, IDC_INCLUDE_LINKED, m_bIncludeLinked);
	DDX_Control(pDX, IDC_INCLUDE_ALL, m_cbIncludeAll);
	DDX_Check(pDX, IDC_INCLUDE_ALL, m_bIncludeAll);
	DDX_Check(pDX, IDC_LOGOUTDATED, m_bLogOutdated);
	DDV_MaxChars(pDX, m_pathName, 255);

	if(pDX->m_bSaveAndValidate) {

		m_pathName.Trim();
		LPSTR fullpath=0;

		if(m_pathName.IsEmpty() || !(fullpath=dos_FullPath(m_pathName, ".dbf"))) {
			AfxMessageBox("The output pathname is empty or invalid!");
			goto _pathFail;
		}

		if(!DirCheck(fullpath, TRUE))
			goto _pathFail;

		LPCSTR perr=0;

		if(m_bIncludeShp) {
			if(CShpLayer::DbfileIndex(fullpath)>=0 || !_access(fullpath, 0) && _access(fullpath, 6)) {
				*trx_Stpext(fullpath)=0;
				perr="Shapefile";
				goto _pathProtected;
			}
		}

		if(m_bIncludeShp || m_bIncludeLinked) {
			//Is zip protected?
			strcpy(trx_Stpext(fullpath), ".zip");
			if(!_access(fullpath, 0) && _access(fullpath, 6)) {
				perr="Archive";
				goto _pathProtected;
			}
		}

		strcpy(trx_Stpext(fullpath), ".txt");
		if(!_access(fullpath, 0) && _access(fullpath, 6)) {
			perr="Log file";
			goto _pathProtected;
		}

		m_pathName=fullpath;
		{
			//remove unselected layers
			it_compshp it0=m_vCompShp.begin();
			it_compshp it=it0;
			for(; it!=m_vCompShp.end(); it++) {
				if(it->bSel) {
					it->bSel=0;
					if(it!=it0) {
						it0->pShp=it->pShp;
					}
					it0++;
				}
			}
			ASSERT(m_nNumSel && m_nNumSel==(it0-m_vCompShp.begin()));
			if(it0!=it) m_vCompShp.resize(m_nNumSel);
		}

		GetDlgItem(IDOK)->ShowWindow(SW_HIDE);
		GetDlgItem(IDCANCEL)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_LOGOUTDATED)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_ST_SCANNING)->SetWindowText("Scanning records. Please wait...");
		Pause();

		*m_ppwFld=(WORD *)calloc(m_vCompShp.size()*m_pShpRef->m_pdb->NumFlds(), sizeof(WORD));

		bLogOutdated=m_bLogOutdated;

		m_iFlags=m_pShpRef->CompareShp(m_pathName, m_summary, m_vCompShp, *m_ppwFld, m_vFldsIgnored, m_bIncludeAll);

		//m_iFlags=-1 if error, no more prompting or summary display required
		//0 if no log generated (no changes), "No log generated" is in summary, no other prompting needed
		//0x4000000 if log only (nothing to zip, but some messages)
		//0x4000001 if just shp to zip
		//0x4000003 if both shp and linked files to zip

		if(m_iFlags>0) {
			if(!m_bIncludeShp) m_iFlags&=~1;
			else if((m_iFlags&1) && m_bIncludeAll) m_iFlags|=4;
			if(!m_bIncludeLinked) m_iFlags&=~2;
			m_summary+=((m_iFlags&3)?"\nProceed with ZIP creation?":"\nWould you like to open the log?");
		}

		//		EndDialog((m_iFlags<0)?IDCANCEL:IDOK);
		return;

	_pathProtected:
		{
			CString s(" You may need to remove it as a layer.");
			if(*perr!='S') s.Empty();
			CMsgBox(MB_ICONINFORMATION, "Please choose another output path or filename. %s %s is protected and can't be replaced.%s",
				perr, trx_Stpnam(fullpath), (LPCSTR)s);
		}
	_pathFail:
		pDX->m_idLastControl=IDC_PATHNAME;
		pDX->m_bEditLastControl=TRUE;
		pDX->Fail();

	}
}

BEGIN_MESSAGE_MAP(CCompareDlg, CDialog)
	ON_WM_CTLCOLOR()
	ON_BN_CLICKED(IDC_MOVEALL_LEFT, &CCompareDlg::OnBnClickedMoveallLeft)
	ON_BN_CLICKED(IDC_MOVEALL_RIGHT, &CCompareDlg::OnBnClickedMoveallRight)
	ON_BN_CLICKED(IDC_MOVELEFT, &CCompareDlg::OnBnClickedMoveLayerLeft)
	ON_BN_CLICKED(IDC_MOVERIGHT, &CCompareDlg::OnBnClickedMoveLayerRight)
	ON_LBN_DBLCLK(IDC_SHP_CAND,&CCompareDlg::OnBnClickedMoveLayerRight)
	ON_LBN_DBLCLK(IDC_SHP_COMP,&CCompareDlg::OnBnClickedMoveLayerLeft)
	ON_BN_CLICKED(IDC_BROWSE, &CCompareDlg::OnBnClickedBrowse)
	ON_BN_CLICKED(IDC_SEL_IGNORED, &CCompareDlg::OnBnClickedSelIgnored)
	ON_BN_CLICKED(IDC_INCLUDE_ALL, &CCompareDlg::OnBnClickedIncludeAll)
	ON_BN_CLICKED(IDC_INCLUDE_SHP, &CCompareDlg::OnBnClickedIncludeShp)
	ON_MESSAGE(WM_COMMANDHELP,OnCommandHelp)
END_MESSAGE_MAP()

BOOL CCompareDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	CString s;

#if 0
	double dW=MetersPerDegreeLon(36.5);
	dW*=0.01; 
	double dH=MetersPerDegreeLat(36.5);
	dH*=0.01; //cell dimensions
	dW*=dH;

	dW=MetersPerDegreeLon(25.837);
	dW*=0.01;
	dH=MetersPerDegreeLat(25.837);
	dH*=0.01; //cell
	dW*=dH;
#endif

	GetDlgItem(IDC_SHP_REF)->SetWindowText(m_pShpRef->LayerPath(s));
	GetDlgItem(IDC_SHP_KEY)->SetWindowText(m_pShpRef->m_pdb->FldNamPtr(m_pShpRef->KeyFld()));
	if(!m_vFldsIgnored.empty()) ShowIgnored();
	
	MoveAllLayers(FALSE);

	if(m_pShpRef->HasLocFlds() && m_pShpRef->m_bConverted) {
		m_cbIncludeShp.EnableWindow(0);
		m_cbIncludeAll.EnableWindow(0);
	}

	if(!m_pShpRef->HasMemos())
		m_cbIncludeLinked.EnableWindow(0);

	m_lbShpCand.SetFocus();

	return FALSE;  // return TRUE unless you set the focus to a control
}

void CCompareDlg::UpdateButtons()
{
	BOOL b=m_lbShpCand.GetCount()>0;
	Enable(IDC_MOVERIGHT,b);
	Enable(IDC_MOVEALL_RIGHT,b);

	b=(m_nNumSel=m_lbShpComp.GetCount())>0;
	Enable(IDC_MOVELEFT,b);
	Enable(IDC_MOVEALL_LEFT,b);
	Enable(IDOK,b);
}

void CCompareDlg::MoveAllLayers(BOOL bToRight)
{
	m_lbShpCand.SetRedraw(0);
	m_lbShpCand.ResetContent();
	m_lbShpComp.SetRedraw(0);
	m_lbShpComp.ResetContent();
	CHScrollListBox &list=bToRight?m_lbShpComp:m_lbShpCand;

    CString path;
	int i=0;
	for(it_compshp it=m_vCompShp.begin();it!=m_vCompShp.end();it++,i++) {
		it->bSel=bToRight;
		VERIFY(i==list.AddString(it->pShp->LayerPath(path)));
		list.SetItemData(i,(DWORD_PTR)i);
	}
	list.SetCurSel(0);
	m_lbShpCand.SetRedraw(1);
	m_lbShpComp.SetRedraw(1);
	UpdateButtons();
}

void CCompareDlg::OnBnClickedMoveallLeft()
{
	MoveAllLayers(FALSE); //exclude all layers
}

void CCompareDlg::OnBnClickedMoveallRight()
{
	MoveAllLayers(TRUE); //include all layers
}

void CCompareDlg::MoveOne(CListBox &list1,CListBox &list2,int iSel1,BOOL bToRight)
{
	if(iSel1<0) return;
	CString s;
	list1.GetText(iSel1,s);
	int iSeq=list1.GetItemData(iSel1);
	ASSERT(iSeq!=-1 && m_vCompShp[iSeq].bSel==bToRight?0:1);
	m_vCompShp[iSeq].bSel=bToRight;
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
	UpdateButtons();
}

void CCompareDlg::OnBnClickedMoveLayerLeft()
{
	MoveOne(m_lbShpComp,m_lbShpCand,m_lbShpComp.GetCurSel(),FALSE);
}

void CCompareDlg::OnBnClickedMoveLayerRight()
{
	MoveOne(m_lbShpCand,m_lbShpComp,m_lbShpCand.GetCurSel(),TRUE);
}

void CCompareDlg::OnBnClickedBrowse()
{
	CString newPath(m_pathName);
	CString strFilter;
	if(!AddFilter(strFilter,IDS_TXT_FILES) || !AddFilter(strFilter,IDS_SHP_FILES) || !AddFilter(strFilter,IDS_ZIP_FILES)) return;
	if(!DoPromptPathName(newPath,
		OFN_HIDEREADONLY,
		1,strFilter,FALSE,IDS_ZIP_SAVEAS,".txt")) return;

	LPSTR fullpath=dos_FullPath(newPath,".");
	if(fullpath) {
		*trx_Stpext(fullpath)=0;
		m_pathName=fullpath;
		m_cePathName.SetWindowText(fullpath);
	}
	else {
		ASSERT(0);
		AfxMessageBox("Invalid pathname!");
	}
}

void CCompareDlg::ShowIgnored()
{
	CString s;
	s.Format("%u", m_vFldsIgnored.size());
	GetDlgItem(IDC_ST_IGNORED)->SetWindowText(s);
}

void CCompareDlg::OnBnClickedSelIgnored()
{
	CFldsIgnoreDlg dlg(m_pShpRef,m_vFldsIgnored);
	if(IDOK==dlg.DoModal()) ShowIgnored();
}


void CCompareDlg::OnBnClickedIncludeAll()
{
	if(m_cbIncludeAll.GetCheck()!=0) m_cbIncludeShp.SetCheck(TRUE);
}


void CCompareDlg::OnBnClickedIncludeShp()
{
	if(!m_cbIncludeShp.GetCheck()) m_cbIncludeAll.SetCheck(FALSE);
}

LRESULT CCompareDlg::OnCommandHelp(WPARAM,LPARAM)
{
	AfxGetApp()->WinHelp(1020);
	return TRUE;
}

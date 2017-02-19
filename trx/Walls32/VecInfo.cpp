// VecInfo.cpp : implementation file
//

#include "stdafx.h"
#include "walls.h"
#include "prjdoc.h"
#include "magdlg.h"
#include "plotview.h"
#include "segview.h"
#include "compile.h"
#include "VecInfo.h"
#include "wall_shp.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define pLB(id) ((CListBox *)GetDlgItem(id))

/////////////////////////////////////////////////////////////////////////////
// CVecInfoDlg dialog


CVecInfoDlg::CVecInfoDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CVecInfoDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CVecInfoDlg)
	//}}AFX_DATA_INIT
	m_pNode=NULL;
	m_hMagDlg=0;
}

void CVecInfoDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CVecInfoDlg, CDialog)
	ON_WM_CTLCOLOR()
	ON_BN_CLICKED(IDC_PROPERTIES, OnProperties)
	ON_BN_CLICKED(IDC_FILE_EDIT, OnFileEdit)
	ON_BN_CLICKED(IDC_VIEWSTATS, OnViewStats)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_FR_MAGDLG, OnFrMagDlg)
	ON_BN_CLICKED(IDC_TO_MAGDLG, OnToMagDlg)
    ON_MESSAGE(WM_COMMANDHELP,OnCommandHelp)
	ON_BN_CLICKED(IDC_VIEWSEG, OnBnClickedViewseg)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CVecInfoDlg message handlers

void CVecInfoDlg::SetCoordinate(UINT id,int i)
{
	CString s;
	s.Format("%.2f",LEN_SCALE(pPLT->xyz[i]));
	SetText(id,s);
}

void CVecInfoDlg::SetNote(UINT id)
{
	char note[TRX_MAX_KEYLEN+1];
	
	if(!pPLT->end_id) {
		if(!CPrjDoc::m_pReviewNode->InheritedState(FLG_GRIDMASK)) *note=0;
		else CPrjDoc::GetDatumStr(note,CPrjDoc::m_pReviewNode);
	}
	else if(!CPrjDoc::FindNoteTreeRec(note)) *note=0;
	if(*note) SetText(id,note);
}

void CVecInfoDlg::SetFlagnames(UINT idf,UINT idm)
{

	if(!pPLT->end_id) {
		Show(idm,FALSE);
		return;
	}

	if(!CSegView::pFlagNamePos) return;

	UINT count=0,numf=pixSEGHDR->numFlagNames;
	CListBox *plb=pLB(idf);

	BYTE key[8];
	char flagname[TRX_MAX_KEYLEN+1];
	bool bPending;

	ixSEG.UseTree(NTA_NOTETREE);

	*key=5;
	key[1]=(char)NTA_CHAR_FLAGPREFIX;
	*((long *)&key[2])=pPLT->end_id;
	bPending=ixSEG.Find(key)==0;

	while(bPending) {
		VERIFY(ixSEG.GetKey(key,8)==0);
		idf=(DWORD)*(WORD *)&key[6];
		ASSERT(idf<numf);
		CSegView::pFlagNameSeq[idf]|=0x40000000;
		count++;
		*key=5;
		bPending=ixSEG.FindNext(key)==0;
	}

	if(count) {
		for(idf=0;idf<numf;idf++) {
			if(CSegView::pFlagNameSeq[idf]&0x40000000) {
				count--;
				CSegView::pFlagNameSeq[idf]&=~0x40000000;
				VERIFY(!ixSEG.SetPosition(CSegView::pFlagNamePos[CSegView::pFlagNameSeq[idf]&0x7FFFFFFF]));
				VERIFY(!ixSEG.GetKey(flagname));
				if(*flagname) {
					FixFlagCase(trx_Stxpc(flagname));
					VERIFY(plb->AddString(flagname)>=0);
				}
			}
		}
		ASSERT(!count);
	}
	ixSEG.UseTree(NTA_SEGTREE);
}

void CVecInfoDlg::SetButtonImage(UINT id)
{
	if(m_hMagDlg) GetDlgItem(id)->SendMessage(BM_SETIMAGE,(WPARAM)IMAGE_ICON,(LPARAM)m_hMagDlg);
	//if(m_hMagDlg) GetDlgItem(id)->SendMessage(BM_SETIMAGE,(WPARAM)IMAGE_BITMAP,(LPARAM)m_hMagDlg);
}

BOOL CVecInfoDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	char fname[SHP_SIZ_BUF_ATTRIBUTES];
	CString str("Properties for Vector ");

	CPrjDoc::AppendVecStationNames(str);
	SetWindowText(str);

	//Assumes pPLT and pVEC already positioned at vector --
	ASSERT((pPLT->vec_id+1)/2==dbNTV.Position());

	pSV->GetAttributeNames(pPLT->seg_id,fname,SHP_SIZ_BUF_ATTRIBUTES);
	SetText(IDC_ST_ATTR,fname);

	GetStrFromDateFld(fname,pVEC->date,"-");
	SetText(IDC_ST_SURVEYDATE,fname);

	ASSERT(CPrjDoc::m_pReviewNode);
	GetStrFromFld(fname,pVEC->filename,8);
	if(CPrjDoc::m_pReviewNode && (m_pNode=CPrjDoc::m_pReviewNode->FindName(fname))) {
		SYSTEMTIME *ptm=GetLocalFileTime(CPrjDoc::m_pReviewDoc->SurveyPath(m_pNode),NULL);
		SetText(IDC_ST_FILEDATE,GetTimeStr(ptm,NULL));
		SetText(IDC_ST_FILENAME,trx_Stpnam(CPrjDoc::m_pathBuf));
		str.Empty();
		m_pNode->GetBranchPathName(str);
		SetText(IDC_ST_TITLE,str);
	}
	ASSERT(m_pNode);
	
	Enable(IDC_VIEWSTATS,pPLT->str_id!=0);

	//Enable or disable calculator buttons --

	//VERIFY(m_hMagDlg=LoadImage(AfxGetInstanceHandle(),MAKEINTRESOURCE(IDB_MAGDLG),IMAGE_BITMAP,0,0,LR_DEFAULTCOLOR));
	VERIFY(m_hMagDlg=LoadImage(AfxGetInstanceHandle(),
				MAKEINTRESOURCE(IDI_MAGDLG),IMAGE_ICON,0,0,LR_DEFAULTCOLOR));

	SetButtonImage(IDC_FR_MAGDLG);
	SetButtonImage(IDC_TO_MAGDLG);

	if(!CPrjDoc::m_pReviewNode->InheritedState(FLG_GRIDMASK)) {
		Enable(IDC_FR_MAGDLG,FALSE);
		Enable(IDC_TO_MAGDLG,FALSE);
	}

	CSegView::GetFlagNameRecs();
	
	BOOL bAtToStation=(pPLT->vec_id&1)==0;

	if(bAtToStation) VERIFY(dbNTP.Prev()==0);

	//From station data --
	str.Empty();
	CPrjDoc::AppendStationName(str,pVEC->pfx[0],pVEC->nam[0]);
	SetText(IDC_ST_FR_NAME,str);
	memcpy(m_enu_fr,pPLT->xyz,sizeof(m_enu_fr));
	SetCoordinate(IDC_FR_EAST,0);
	SetCoordinate(IDC_FR_NORTH,1);
	SetCoordinate(IDC_FR_UP,2);
	SetNote(IDC_FR_NOTE);
	SetFlagnames(IDC_FR_FLAGS,IDC_FR_MAGDLG);

	if(bAtToStation) VERIFY(dbNTP.Next()==0);
	else VERIFY(dbNTP.Prev()==0);

	//To station data --
	str.Empty();
	CPrjDoc::AppendStationName(str,pVEC->pfx[1],pVEC->nam[1]);
	SetText(IDC_ST_TO_NAME,str);
	memcpy(m_enu_to,pPLT->xyz,sizeof(m_enu_to));
	SetCoordinate(IDC_TO_EAST,0);
	SetCoordinate(IDC_TO_NORTH,1);
	SetCoordinate(IDC_TO_UP,2);
	SetNote(IDC_TO_NOTE);
	SetFlagnames(IDC_TO_FLAGS,IDC_TO_MAGDLG);

	if(!bAtToStation) VERIFY(dbNTP.Next()==0);

	CSegView::FreeFlagNameRecs();

	if(LEN_ISFEET()) {
		SetText(IDC_ST_EAST,"East (ft)");
		SetText(IDC_ST_NORTH,"North (ft)");
		SetText(IDC_ST_UP,"Up (ft)");
	}
	return TRUE;
}

HBRUSH CVecInfoDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
	if(nCtlColor==CTLCOLOR_STATIC) {
		UINT id=pWnd->GetDlgCtrlID();
		switch(id) {
			case IDC_STATIC :
			case IDC_ST_EAST :
			case IDC_ST_NORTH :
			case IDC_ST_UP : break;
			default: nCtlColor=CTLCOLOR_LISTBOX;
		}
	}

    if(nCtlColor==CTLCOLOR_LISTBOX) {
        pDC->SetBkMode(TRANSPARENT);
		pDC->SetTextColor(RGB_DKRED);
		return (HBRUSH)::GetSysColorBrush(COLOR_3DFACE);
    }
	
	// TODO: Return a different brush if the default is not desired
	return CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
}

void CVecInfoDlg::OnProperties() 
{
	EndDialog(IDRETRY);
}

void CVecInfoDlg::OnFileEdit() 
{
	EndDialog(IDIGNORE);
}

void CVecInfoDlg::OnViewStats() 
{
	EndDialog(IDYES);
}

void CVecInfoDlg::OnBnClickedViewseg()
{
	EndDialog(IDNO);
}

static void LoadMagDlg(double *enu)
{

	PRJREF *pr=CPrjDoc::m_pReviewNode->GetAssignedRef();
	ASSERT(pr);

	PRJREF prjref=*pr;

	prjref.east=enu[0];
	prjref.north=enu[1];
	prjref.elev=(int)enu[2];
	prjref.zone=pr->zone;
	CMainFrame::InitRef(&prjref);
	if(MF(MAG_CVTUTM2LL3)) return;
	DWORD date=*(DWORD *)pVEC->date&0xFFFFFF;
	if(date) {
		MD.y=((date>>9)&0x7FFF);
		MD.m=((date>>5)&0xF);
		MD.d=(date&0x1F);
	}

	if(app_pMagDlg) app_pMagDlg->DestroyWindow();
    ASSERT(!app_pMagDlg);

	VERIFY(app_pMagDlg=new CMagDlg(CWnd::GetDesktopWindow(),true));
}

void CVecInfoDlg::OnFrMagDlg() 
{
	LoadMagDlg(m_enu_fr);
}

void CVecInfoDlg::OnToMagDlg() 
{
	LoadMagDlg(m_enu_to);
}

void CVecInfoDlg::OnDestroy() 
{
	if(m_hMagDlg) VERIFY(DestroyIcon((HICON)m_hMagDlg));
	CDialog::OnDestroy();
	ASSERT(pPLT->vec_id!=0);
}

LRESULT CVecInfoDlg::OnCommandHelp(WPARAM wNone, LPARAM lParam)
{
	AfxGetApp()->WinHelp(111,HELP_CONTEXT);
	return TRUE;
}

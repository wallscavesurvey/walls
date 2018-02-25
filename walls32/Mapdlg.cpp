// mapdlg.cpp : implementation file
//

#include "stdafx.h"
#include "walls.h"
#include "Prjdoc.h"
#include "compview.h"
#include "plotview.h"
#include "pageview.h" 
#include "mapdlg.h"
#include "mapframe.h"
#include "wall_srv.h"
#include "compile.h"
#include "dialogs.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

BOOL CMapDlg::m_bConfirmedReset=0;

/////////////////////////////////////////////////////////////////////////////
// CMapDlg dialog

CMapDlg::CMapDlg(BOOL bPrinter,CWnd* pParent /*=NULL*/)
	: CDialog(CMapDlg::IDD, pParent)
	, m_nLblPrefix(0)
{
	m_iLabelType = 1;
	if(bPrinter<2) {
		m_MF[0]=CPlotView::m_mfFrame;
		m_MF[1]=CPlotView::m_mfPrint;
		m_bExport=FALSE;
	}
	else {
	    m_MF[0]=CPlotView::m_mfExport;
		m_bExport=TRUE;
		bPrinter=0;
	}
	m_pMF=&m_MF[bPrinter];
	m_bSwapping=m_bFlagChange=FALSE;
	m_bLineChg=m_bLabelChg=m_bApplyActive=FALSE;
}

void CMapDlg::RefreshFrameSize()
{
	double fWidth,fHeight;

	CWallsApp::GetDefaultFrameSize(fWidth,fHeight,m_bUsePageSize);
	m_FrameWidthInches=GetFloatStr(fWidth,4);
	m_FrameHeightInches=GetFloatStr(fHeight,4);
}

void CMapDlg::InitData()
{
	m_bUsePageSize=m_pMF->bPrinter && (m_pMF->fFrameWidthInches<0.1 || m_pMF->fFrameHeightInches<0.1);
	if(m_pMF->bPrinter) RefreshFrameSize();
	else m_FrameWidthInches=GetFloatStr(m_pMF->fFrameWidthInches,4);

	m_FrameThick=GetIntStr(m_pMF->iFrameThick);
	m_MarkerSize=GetIntStr(m_pMF->iMarkerSize);
	m_MarkerThick=GetIntStr(m_pMF->iMarkerThick);
	m_FlagSize=GetIntStr(m_pMF->iFlagSize);
	m_FlagThick=GetIntStr(m_pMF->iFlagThick);
	m_VectorThin=GetIntStr(m_pMF->iVectorThin);
	m_VectorThick=GetIntStr(m_pMF->iVectorThick);
	m_OffLabelX=GetIntStr(m_pMF->iOffLabelX);
	m_OffLabelY=GetIntStr(m_pMF->iOffLabelY);
	m_TickLength=GetIntStr(m_pMF->iTickLength);
	m_TickThick=GetIntStr(m_pMF->iTickThick);
	m_LabelSpace=GetIntStr(m_pMF->iLabelSpace);
	m_LabelInc=GetIntStr(m_pMF->iLabelInc);
}

void CMapDlg::SetSolidText()
{
	static char *name[3]={"Clear","Solid","Opaque"};
	GetDlgItem(IDC_FLAGSOLID)->SetWindowText(name[GetCheck(IDC_FLAGSOLID)]);
}

void CMapDlg::InitControls()
{
    char buf[40];
	static char *ptyp[2]={"Screen...","Printer..."};
	static char *pbtyp[3]={"SCREEN","PRINTER","METAFILES"};
	
	sprintf(buf,"Map Format Options: %s",pbtyp[m_pMF->bPrinter+m_bExport]);
	SetWindowText(buf);

	GetDlgItem(IDC_ST_FRAME)->SetWindowText(m_pMF->bPrinter?"Default Initial Frame Size":"Window Width");
	if(!m_bExport) {
		GetDlgItem(IDC_SWAPSETS)->SetWindowText(ptyp[1-m_pMF->bPrinter]);
		m_bApplyActive=m_bMapsActive && !m_pMF->bPrinter;
		GetDlgItem(IDC_APPLYLBL)->ShowWindow(m_bApplyActive?SW_SHOW:SW_HIDE);
		if(m_bApplyActive) Enable(IDC_APPLYLBL,m_bLabelChg|m_bLineChg);
	}
	else GetDlgItem(IDC_SWAPSETS)->ShowWindow(SW_HIDE);

	Enable(IDC_FRAMETHICK,m_pMF->bPrinter);
	Enable(IDC_ST_FRAMETHICK,m_pMF->bPrinter);

	Enable(IDC_USEPAGESIZE,m_pMF->bPrinter);

	Enable(IDC_FRAMEHEIGHT,m_pMF->bPrinter && !m_bUsePageSize);
	Enable(IDC_ST_FRAMEHEIGHT,m_pMF->bPrinter);
	Enable(IDC_FRAMEWIDTH,!m_bUsePageSize);

	m_iLabelType=(m_pMF->bLblFlags&1)?0:1; //heights or names

	switch(m_pMF->bLblFlags&(8+4+2)) {
		case 2: m_nLblPrefix=1; break;
		case 4: m_nLblPrefix=2; break;
		case 8: m_nLblPrefix=3; break; //prefixed names
		default: m_nLblPrefix=0;
	}

	SetCheck(IDC_MONOCHROME,m_pMF->bPrinter && !m_pMF->bUseColors);
	SetCheck(IDC_FLAGSOLID,m_pMF->bFlagSolid);
	SetSolidText();
    ((CComboBox *)GetDlgItem(IDC_FLAGSTYLE))->SetCurSel(m_pMF->iFlagStyle);

	Enable(IDC_FRAMEFONT,m_pMF->bPrinter);
	GetDlgItem(IDC_MONOCHROME)->ShowWindow(m_pMF->bPrinter?SW_SHOW:SW_HIDE);
}

UINT CMapDlg::CheckData()
{
	MAPFORMAT mf=*m_pMF;
	
	if(m_bUsePageSize) {
		mf.fFrameWidthInches=mf.fFrameHeightInches=0.0;
	}
	else {
		if(!CheckFlt(m_FrameWidthInches,&mf.fFrameWidthInches,
			mf.bPrinter?1.0:MIN_SCREEN_FRAMEWIDTH,
			mf.bPrinter?MAX_FRAMEWIDTHINCHES:MAX_SCREEN_FRAMEWIDTH,
			4)) return IDC_FRAMEWIDTH;
		if(mf.bPrinter && !CheckFlt(m_FrameHeightInches,&mf.fFrameHeightInches,1.0,MAX_FRAMEWIDTHINCHES,4)) return IDC_FRAMEHEIGHT;
	}
	if(m_pMF->bPrinter) {
		if(!CheckInt(m_FrameThick,&mf.iFrameThick,0,100)) return IDC_FRAMETHICK;
	}
	if(!CheckInt(m_MarkerSize,&mf.iMarkerSize,1,SRV_MAX_FLAGSIZE)) return IDC_MARKERSIZE;
	if(!CheckInt(m_MarkerThick,&mf.iMarkerThick,1,10)) return IDC_MARKERTHICK;
	if(!CheckInt(m_FlagSize,&mf.iFlagSize,1,SRV_MAX_FLAGSIZE)) return IDC_FLAGSIZE;
	if(!CheckInt(m_FlagThick,&mf.iFlagThick,1,10)) return IDC_FLAGTHICK;
	if(!CheckInt(m_VectorThin,&mf.iVectorThin,1,10)) return IDC_VECTORTHIN;
	if(!CheckInt(m_VectorThick,&mf.iVectorThick,1,10)) return IDC_VECTORTHICK;
	if(!CheckInt(m_OffLabelX,&mf.iOffLabelX,-SRV_MAX_FLAGSIZE,SRV_MAX_FLAGSIZE)) return IDC_MARKERXOFF;
	if(!CheckInt(m_OffLabelY,&mf.iOffLabelY,-SRV_MAX_FLAGSIZE,SRV_MAX_FLAGSIZE)) return IDC_MARKERYOFF;
	if(!CheckInt(m_TickLength,&mf.iTickLength,1,SRV_MAX_FLAGSIZE)) return IDC_TICKLENGTH;
	if(!CheckInt(m_TickThick,&mf.iTickThick,0,20)) return IDC_TICKTHICK;
	if(!CheckInt(m_LabelSpace,&mf.iLabelSpace,0,1000)) return IDC_LABELSPACE;
	if(!CheckInt(m_LabelInc,&mf.iLabelInc,1,1000)) return IDC_LABELINC;

	mf.bLblFlags=m_iLabelType?0:1;
	switch(m_nLblPrefix) {
		case 1 : mf.bLblFlags|=2; break;
		case 2 : mf.bLblFlags|=4; break;
		case 3 : mf.bLblFlags|=8; break;
	}
	mf.bUseColors=!GetCheck(IDC_MONOCHROME);
	mf.bFlagSolid=GetCheck(IDC_FLAGSOLID);
	mf.iFlagStyle=((CComboBox *)GetDlgItem(IDC_FLAGSTYLE))->GetCurSel();
	
	if(memcmp(&mf,m_pMF,sizeof(MAPFORMAT))) {
	   m_bFlagChange=(mf.bFlagSolid!=m_pMF->bFlagSolid) || (mf.iFlagStyle!=m_pMF->iFlagStyle) ||
		   (mf.iFlagSize!=m_pMF->iFlagSize) || (mf.iMarkerSize!=m_pMF->iMarkerSize);
	   mf.bChanged=TRUE;
	   *m_pMF=mf;
	}
    return 0;
}

static void NoteChange(MAPFORMAT *pMF)
{
	if(pCV!=NULL &&
		(CPlotView::m_mfPrint.fFrameWidthInches!=pMF->fFrameWidthInches ||
		 CPlotView::m_mfPrint.fFrameHeightInches!=pMF->fFrameHeightInches))
	CMsgBox(MB_ICONINFORMATION,IDS_PRJ_FRAMECHG);
}

void CMapDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	if(!pDX->m_bSaveAndValidate) {
		InitData();
		InitControls();
	}

	DDX_Text(pDX, IDC_FRAMEWIDTH, m_FrameWidthInches);
	DDV_MaxChars(pDX, m_FrameWidthInches, 10);
	DDX_Text(pDX, IDC_FRAMETHICK, m_FrameThick);
	DDV_MaxChars(pDX, m_FrameThick, 4);
	DDX_Text(pDX, IDC_MARKERSIZE, m_MarkerSize);
	DDV_MaxChars(pDX, m_MarkerSize, 4);
	DDX_Text(pDX, IDC_MARKERTHICK, m_MarkerThick);
	DDV_MaxChars(pDX, m_MarkerThick, 4);
	DDX_Text(pDX, IDC_VECTORTHICK, m_VectorThick);
	DDV_MaxChars(pDX, m_VectorThick, 4);
	DDX_Text(pDX, IDC_MARKERXOFF, m_OffLabelX);
	DDV_MaxChars(pDX, m_OffLabelX, 4);
	DDX_Text(pDX, IDC_MARKERYOFF, m_OffLabelY);
	DDV_MaxChars(pDX, m_OffLabelY, 4);
	DDX_Text(pDX, IDC_TICKLENGTH, m_TickLength);
	DDV_MaxChars(pDX, m_TickLength, 4);
	DDX_Text(pDX, IDC_TICKTHICK, m_TickThick);
	DDV_MaxChars(pDX, m_TickThick, 4);
	DDX_Text(pDX, IDC_FRAMEHEIGHT, m_FrameHeightInches);
	DDV_MaxChars(pDX, m_FrameHeightInches, 10);
	DDX_Text(pDX, IDC_LABELSPACE, m_LabelSpace);
	DDV_MaxChars(pDX, m_LabelSpace, 3);
	DDX_Text(pDX, IDC_FLAGSIZE, m_FlagSize);
	DDV_MaxChars(pDX, m_FlagSize, 4);
	DDX_Text(pDX, IDC_FLAGTHICK, m_FlagThick);
	DDV_MaxChars(pDX, m_FlagThick, 4);
	DDX_Text(pDX, IDC_VECTORTHIN, m_VectorThin);
	DDV_MaxChars(pDX, m_VectorThin, 4);
	DDX_Text(pDX, IDC_LABELINC, m_LabelInc);
	DDV_MaxChars(pDX, m_LabelInc, 3);
	DDX_Check(pDX, IDC_USEPAGESIZE, m_bUsePageSize);
	DDX_Radio(pDX, IDC_LBLHEIGHTS, m_iLabelType);
	DDX_CBIndex(pDX, IDC_LBLPREFIXES, m_nLblPrefix);

	if(pDX->m_bSaveAndValidate) {
		UINT id=CheckData();
		if(id) {
			pDX->m_idLastControl=id;
			//***7.1 pDX->m_hWndLastControl=GetDlgItem(id)->m_hWnd;
			pDX->m_bEditLastControl=TRUE;
			pDX->Fail();
		}
		else if(m_pMF->bChanged) {
			//Any default flag symbol change other than size and line thickness affects all output types --

			if(m_bSwapping) {
				if(m_bFlagChange) {
					m_MF[1-m_pMF->bPrinter].bFlagSolid=m_pMF->bFlagSolid;
					m_MF[1-m_pMF->bPrinter].iFlagStyle=m_pMF->iFlagStyle;
					m_MF[1-m_pMF->bPrinter].iFlagSize=m_pMF->iFlagSize;
					m_MF[1-m_pMF->bPrinter].iMarkerSize=m_pMF->iMarkerSize;
					m_MF[1-m_pMF->bPrinter].bChanged=TRUE;
				}
			}
			else {
				if(m_bExport) {
					NoteChange(&m_MF[0]);
					CPlotView::m_mfExport=m_MF[0];
					CPlotView::m_mfPrint.fFrameWidthInches=m_MF[0].fFrameWidthInches;
					CPlotView::m_mfPrint.fFrameHeightInches=m_MF[0].fFrameHeightInches;
					CPlotView::m_mfPrint.iFrameThick=m_MF[0].iFrameThick;
					if(m_bFlagChange) {
						CPlotView::m_mfPrint.bFlagSolid=CPlotView::m_mfFrame.bFlagSolid=m_MF[0].bFlagSolid;
						CPlotView::m_mfPrint.iFlagStyle=CPlotView::m_mfFrame.iFlagStyle=m_MF[0].iFlagStyle;
						CPlotView::m_mfPrint.iFlagSize=CPlotView::m_mfFrame.iFlagSize=m_MF[0].iFlagSize;
						CPlotView::m_mfPrint.iMarkerSize=CPlotView::m_mfFrame.iMarkerSize=m_MF[0].iMarkerSize;
						CPlotView::m_mfPrint.bChanged=TRUE;
					}
				}
				else {
					NoteChange(&m_MF[1]);
					if(m_bFlagChange) {
						CPlotView::m_mfExport.bFlagSolid=m_MF[1-m_pMF->bPrinter].bFlagSolid=
							m_pMF->bFlagSolid;
						CPlotView::m_mfExport.iFlagStyle=m_MF[1-m_pMF->bPrinter].iFlagStyle=
							m_pMF->iFlagStyle;
						CPlotView::m_mfExport.iFlagSize=m_MF[1-m_pMF->bPrinter].iFlagSize=
							m_pMF->iFlagSize;
						CPlotView::m_mfExport.iMarkerSize=m_MF[1-m_pMF->bPrinter].iMarkerSize=
							m_pMF->iMarkerSize;

						m_MF[1].bChanged=TRUE;
					}
					CPlotView::m_mfFrame=m_MF[0];
					CPlotView::m_mfPrint=m_MF[1];
					CPlotView::m_mfExport.fFrameWidthInches=m_MF[1].fFrameWidthInches;
					CPlotView::m_mfExport.fFrameHeightInches=m_MF[1].fFrameHeightInches;
					CPlotView::m_mfExport.iFrameThick=m_MF[1].iFrameThick;

					
				}
			}
		}
		if(!m_bSwapping && m_bApplyActive && (m_bLineChg || m_bLabelChg)) {
			ASSERT(CPrjDoc::m_pReviewDoc);
			CPrjDoc::m_pReviewDoc->UpdateMapViews(FALSE,
				m_bLineChg?M_TOGGLEFLAGS:(M_NOTES|M_NAMES|M_HEIGHTS|M_PREFIXES));
		}
	} 
} 

IMPLEMENT_DYNAMIC(CMapDlg,CDialog)

BEGIN_MESSAGE_MAP(CMapDlg, CDialog)
    ON_MESSAGE(WM_COMMANDHELP,OnCommandHelp)
	ON_BN_CLICKED(IDC_FRAMEFONT, OnFrameFont)
	ON_BN_CLICKED(IDC_LABELFONT, OnLabelFont)
	ON_BN_CLICKED(IDC_NOTEFONT, OnNoteFont)
	ON_BN_CLICKED(IDC_SWAPSETS, OnSwapSets)
	ON_BN_CLICKED(IDC_USEPAGESIZE, OnUsePageSize)
	ON_BN_CLICKED(IDC_FLAGSOLID, OnFlagSolid)
	ON_BN_CLICKED(IDC_RESET, OnReset)
	ON_BN_CLICKED(IDC_APPLYLBL, OnApplyLbl)

	ON_EN_CHANGE(IDC_LABELINC, OnLabelChg)
	ON_EN_CHANGE(IDC_LABELSPACE, OnLabelChg)
	ON_EN_CHANGE(IDC_MARKERXOFF, OnLabelChg)
	ON_EN_CHANGE(IDC_MARKERYOFF, OnLabelChg)
	ON_CBN_SELCHANGE(IDC_LBLPREFIXES, OnLabelChg)

	ON_EN_CHANGE(IDC_VECTORTHIN, OnLineChg)
	ON_EN_CHANGE(IDC_VECTORTHICK, OnLineChg)
	ON_EN_CHANGE(IDC_FLAGTHICK, OnLineChg)
	ON_EN_CHANGE(IDC_MARKERTHICK, OnLineChg)

END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMapDlg message handlers and helpers

CMapDlg::~CMapDlg()
{
#ifdef _BOLD_HDR
	if(m_boldFont.GetSafeHandle()) m_boldFont.DeleteObject();
#endif
}

BOOL CMapDlg::OnInitDialog()
{
#ifdef _BOLD_HDR
	static UINT hdrCtls[]={
	 IDC_ST_FRAME,IDC_ST_LABELS,IDC_ST_FLAGS,IDC_ST_TICKMARKS,
	 IDC_ST_MARKERS,IDC_ST_LINEWIDTHS,IDC_ST_FONTS};
	
	LOGFONT lf;
	CFont *pcf=GetDlgItem(IDC_ST_FRAME)->GetFont();
	pcf->GetLogFont(&lf);
	lf.lfWeight = FW_BOLD;
	if(m_boldFont.CreateFontIndirect(&lf)) {
		for(int i=0;i<sizeof(hdrCtls)/sizeof(UINT);i++)
			GetDlgItem(hdrCtls[i])->SetFont(&m_boldFont);
	}
#endif
	CDialog::OnInitDialog();
	CenterWindow();
	//GetDlgItem(IDC_FRAMEWIDTH)->SetFocus();
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CMapDlg::OnFrameFont()
{
	 ASSERT(m_pMF->bPrinter);
	 CMainFrame::ChangeLabelFont(m_pMF->pfFrameFont);
}

void CMapDlg::OnLabelFont()
{
	 CMainFrame::ChangeLabelFont(m_pMF->pfLabelFont);
	 OnLabelChg();
}

void CMapDlg::OnNoteFont()
{
     CMainFrame::ChangeLabelFont(m_pMF->pfNoteFont);
	 OnLabelChg();
}

LRESULT CMapDlg::OnCommandHelp(WPARAM wNone, LPARAM lParam)
{
	AfxGetApp()->WinHelp(8);
	return TRUE;
}

void CMapDlg::OnFlagSolid()
{
	SetSolidText();
}

void CMapDlg::OnSwapSets()
{
	ASSERT(!m_bExport);
	m_bSwapping=TRUE;
	if(UpdateData(TRUE)) {
	  m_pMF=&m_MF[1-m_pMF->bPrinter];
	  UpdateData(FALSE);
	}
	m_bSwapping=FALSE;
}

void CMapDlg::OnUsePageSize() 
{
	ASSERT(m_pMF->bPrinter);
	m_bUsePageSize=IsDlgButtonChecked(IDC_USEPAGESIZE);
	if(m_bUsePageSize) {
		RefreshFrameSize();
		SetText(IDC_FRAMEWIDTH,m_FrameWidthInches);
		SetText(IDC_FRAMEHEIGHT,m_FrameHeightInches);
	}
	//else GetDlgItem(IDC_FRAMEWIDTH)->SetFocus();
	Enable(IDC_FRAMEWIDTH,!m_bUsePageSize);
	Enable(IDC_FRAMEHEIGHT,!m_bUsePageSize);
}

void CMapDlg::OnLabelTypeChg(UINT id)
{
	OnLabelChg();
}

void CMapDlg::OnLabelChg()
{
	if(!m_bApplyActive) return;
	if(!m_bLabelChg) {
		if(!m_bLineChg) Enable(IDC_APPLYLBL, TRUE);
		m_bLabelChg=TRUE;
	}
}

void CMapDlg::OnLineChg()
{
	if(!m_bApplyActive) return;
	if(!m_bLineChg) {
		if(!m_bLabelChg) Enable(IDC_APPLYLBL, TRUE);
		m_bLineChg=TRUE;
	}
}

void CMapDlg::OnApplyLbl()
{
	ASSERT(m_bApplyActive && (m_bLineChg || m_bLabelChg));
	if(UpdateData(1)) {
		m_bLineChg=m_bLabelChg=FALSE;
		Enable(IDC_APPLYLBL,FALSE);
	}
}

void CMapDlg::OnReset()
{
	BOOL bChg=m_pMF->bChanged;
	m_pMF->bChanged=0;
	if(memcmp(m_pMF,&CPlotView::m_mfDflt[m_pMF->bPrinter],sizeof(MAPFORMAT))) {
		if(!m_bConfirmedReset) {
			CString s;
			s.Format("This will restore format preferences for %s maps to their original default values. If you do this "
			"you might lose changes you've made in earlier program sessions. Font settings, however, are not affected.",m_bExport?"EXPORTED":(m_pMF->bPrinter?"PRINTED":"SCREEN"));
			CApplyDlg dlg(IDD_MOVE_CONFIRM,&m_bConfirmedReset,"Comfirm Reset to Defaults",s);
			if(dlg.DoModal()!=IDOK) goto _ret;
		}
		ASSERT(m_pMF=&m_MF[m_bExport?0:m_pMF->bPrinter]);
		*m_pMF=CPlotView::m_mfDflt[m_pMF->bPrinter];
		m_pMF->bChanged=TRUE;
		UpdateData(0);
		if(m_bApplyActive) {
			m_bLineChg=m_bLabelChg=TRUE;
			Enable(IDC_APPLYLBL, TRUE);
		}
		return;
	}
_ret:
	m_pMF->bChanged=bChg;
}

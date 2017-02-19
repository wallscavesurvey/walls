// ExportNTI.cpp : implementation file
//

#include "stdafx.h"
#include "WallsMap.h"
#include "mainfrm.h"
#include "WallsMapDoc.h"
#include "WallsMapView.h"
#include "GdalLayer.h"
#include "NtiLayer.h"
#include "ExportNTI.h"
#include "ExportNTI_Adv.h"
#include <gdal_priv.h>
#include <ntifile.h>
#include <webp31/encode.h>
/*
  WEBP_PRESET_DEFAULT = 0,  // default preset.
  WEBP_PRESET_PICTURE,      // digital picture, like portrait, inner shot
  WEBP_PRESET_PHOTO,        // outdoor photograph, with natural lighting
  WEBP_PRESET_DRAWING,      // hand or line drawing, with high-contrast details
  WEBP_PRESET_ICON,         // small-sized colorful images
  WEBP_PRESET_TEXT          // text-like
*/

// CExportNTI dialog

BOOL CExportNTI::m_bReplWithExported = FALSE;
BOOL CExportNTI::m_bAddExported = FALSE;
BOOL CExportNTI::m_bOpenExported = TRUE;

static BOOL bExtentWindow=0;
char *webp_preset[NTI_WEBP_NUM_PRESETS] = { "dflt", "pict", "photo", "draw", "icon", "text" };
static char *sFilter[4]={"(G)","(R)","(B)",""};

IMPLEMENT_DYNAMIC(CExportNTI, CDialog)
CExportNTI::CExportNTI(CImageLayer *pGLayer,CWnd* pParent /*=NULL*/)
	: CDialog(CExportNTI::IDD, pParent),m_pGLayer(pGLayer)
{
	m_iHint=0;
	CExportNTI_Adv::m_bNoOverviews=FALSE;
	m_pDoc=m_pGLayer->m_pDoc;
	m_job.SetLayer(m_pGLayer);
	m_job.SetExpanding(FALSE);
	m_pathName=m_pGLayer->PathName();
	m_pathName+=NTI_EXT;
	m_bProcessing=FALSE;
	m_bAddToProject=m_bAddExported && m_pGLayer->IsTransformed();
	theApp.m_bBackground=FALSE;
	m_crWindow.SetRectEmpty();
	CWallsMapView *pView=m_pDoc->GetFirstView();
	ASSERT(pView);
	if(pView) {
		CFltRect viewExt;
		pView->GetViewExtent(viewExt);
		if(m_pDoc->TrimExtent(viewExt))
			m_pGLayer->GeoRectToImgRect(viewExt,m_crWindow);
	}
	m_bExtentWindow=(bExtentWindow!=1 || !m_crWindow.IsRectEmpty())?bExtentWindow:0;
}

CExportNTI::~CExportNTI()
{
	m_job.nti.NullifyHandle();
}

UINT CExportNTI::CheckInt(UINT id,int *pi,int iMin,int iMax)
{
	int i;
	CString buf;
	GetDlgItem(id)->GetWindowText(buf);
	if(!IsNumeric(buf) || (i=atoi(buf))<iMin || i>iMax) {
		CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_BADINT2,iMin,iMax);
		return id;
	}
	*pi=i;
	return 0;
}

UINT CExportNTI::Get_crWindow()
{
	if (m_bExtentWindow) {
		if (m_bExtentWindow == 2) {
			int left, top, width, height;
			UINT id;
			if ((id = CheckInt(IDC_EDGE_LEFT, &left, 0, m_pGLayer->Width() - 1)) ||
				(id = CheckInt(IDC_EDGE_TOP, &top, 0, m_pGLayer->Height() - 1)) ||
				(id = CheckInt(IDC_WIDTH, &width, 1, m_pGLayer->Width() - left)) ||
				(id = CheckInt(IDC_HEIGHT, &height, 1, m_pGLayer->Height() - top))) {
				return id;
			}
			crWindow.SetRect(left, top, left + width, top + height);
		}
		else crWindow = m_crWindow;
	}
	else crWindow.SetRect(0, 0, m_pGLayer->Width(), m_pGLayer->Height());
	return 0;
}

void CExportNTI::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_PATHNAME, m_pathName);
	DDV_MaxChars(pDX, m_pathName, 260);
	DDX_Control(pDX, IDC_PATHNAME, m_cePathName);
	DDX_Control(pDX, IDC_PROGRESS, m_Progress);
	DDX_Radio(pDX, IDC_EXTENT_ENTIRE, m_bExtentWindow);
	DDX_Check(pDX, IDC_REPLW_EXPORTED, m_bReplWithExported);
	DDX_Check(pDX, IDC_ADD_EXPORTED, m_bAddToProject);
	DDX_Check(pDX, IDC_OPEN_EXPORTED, m_bOpenExported);

	if(pDX->m_bSaveAndValidate) {
		if(!m_bProcessing) {
			//Now let's see if the directory needs to be created --
			if(ForceExtension(m_pathName,NTI_EXT))
				SetText(IDC_PATHNAME,m_pathName);
			LPCSTR p=trx_Stpext(m_pathName);
			if(CheckDirectory(m_pathName,CExportNTI_Adv::m_Adv.bOverwrite)) {
				pDX->m_idLastControl=IDC_PATHNAME;
				pDX->m_bEditLastControl=TRUE;
				pDX->Fail();
				return;
			}

			UINT id = Get_crWindow();
			if (id) {
				pDX->m_idLastControl = id;
				pDX->m_bEditLastControl = TRUE;
				pDX->Fail();
				return;
			}

			NTI_LVL lvl;
			_nti_InitLvl(&lvl,crWindow.Width(),crWindow.Height(),m_pGLayer->NumColorEntries(),m_pGLayer->NumBands());

			double sz=(double)(lvl.nRecTotal*sizeof(DWORD)+SZ_NTI_HDR+m_pGLayer->NumColorEntries()*sizeof(RGBQUAD))+
				(double)(lvl.rec[1]*m_pGLayer->NumBands()+(lvl.nRecTotal-lvl.rec[1])*lvl.nOvrBands)*NTI_BLKSIZE;

			if(sz>=(double)(1024*1024*1024)) {
				if(IDYES!=CMsgBox(MB_YESNO|MB_ICONINFORMATION,IDS_ERR_SIZEIMAGE2,sz/(1024*1024*1024))) {
					pDX->m_idLastControl=IDC_EXTENT_CUSTOM;
					pDX->m_bEditLastControl=TRUE;
					pDX->Fail();
					return;
				}
			}
			bExtentWindow=m_bExtentWindow;

			/* tested in DoWork() --
			if (CExportNTI_Adv::m_Adv.bGet_ssim && (m_pGLayer->NumColorEntries() ||
				crWindow.Width()< NTI_BLKWIDTH || crWindow.Height() < NTI_BLKWIDTH))
				CExportNTI_Adv::m_Adv.bGet_ssim = FALSE;
			*/
		}
		else {
			//Processing done on another thread --
			ASSERT(m_pGLayer->LayerType()!=TYP_GDAL || !((CGdalLayer *)m_pGLayer)->m_pGDAL);
			bool bFileCreated=m_job.nti.Opened();
			if(bFileCreated) {
				m_bAddExported=m_bAddToProject;
				if(m_bReplWithExported && !CExportNTI_Adv::m_bNoOverviews) {
					ASSERT(!m_bAddExported && !m_bOpenExported);
					try {
						CNtiLayer *pNLayer=new CNtiLayer(m_job.nti);
						if(!pNLayer->SetNTIProperties()) {
							delete pNLayer;
							throw 0;
						}
						pNLayer->m_bUseTransColor=pNLayer->ParseTransColor(pNLayer->m_crTransColor);
						//pNLayer->m_bUseTransColor=m_pGLayer->m_bUseTransColor;
						//pNLayer->m_crTransColor=m_pGLayer->m_crTransColor;
						pNLayer->m_bAlpha=m_pGLayer->m_bAlpha;
						pNLayer->m_pDoc=m_pDoc;

						if(!strcmp(m_pGLayer->Title(),m_pGLayer->FileName()))
                            pNLayer->SetTitle(trx_Stpnam(m_pathName));
						else
							pNLayer->SetTitle(m_pGLayer->Title());

						pNLayer->SetVisible(m_pGLayer->IsVisible());

						//Can't throw --
						m_iHint=(pNLayer->Width()!=m_pGLayer->Width() || pNLayer->Height()!=m_pGLayer->Height())?LHINT_FITIMAGE:LHINT_REFRESH;
						m_pDoc->ReplaceLayer(pNLayer); //replace layer at selected position
					}
					catch(...) {
					   m_job.nti.Close();
					   m_pDoc->OnCloseDocument();
					}
					return;
				}
				m_job.nti.Close(); //shouldn't fail since flushed
				ASSERT(!m_job.nti.Errno());
			}

			if(m_pGLayer->LayerType()==TYP_GDAL) {
				//Reopen GDAL file --
				CGdalLayer *pGL=(CGdalLayer *)m_pGLayer;
				ASSERT(pGL && !pGL->m_pGDAL);
				ASSERT(pGL->IsInitialized());

				if(CGdalLayer::m_uRasterIOActive==2 || !(pGL->m_pGDAL=(GDALDataset *)GDALOpen(pGL->PathName(),GA_ReadOnly))) {
					m_pDoc->OnCloseDocument();
				}
			}

			if(bFileCreated && !CExportNTI_Adv::m_bNoOverviews) {
				ASSERT(!m_bReplWithExported);
				if (m_bAddToProject) {
					ASSERT(!m_bOpenExported);
					m_pDoc->AddNtiLayer(m_pathName);
				}
				else if (m_bOpenExported) {
					VERIFY(theApp.OpenDocumentFile(m_pathName));
				}
			}
		}
	}
}

void CExportNTI::UpdateAdvSettings()
{
	CString text,args;

	int iTyp=CExportNTI_Adv::m_Adv.bMethod;

	bool bGetSSIM = CExportNTI_Adv::m_Adv.bGet_ssim && (iTyp==NTI_FCNJP2K || iTyp==NTI_FCNWEBP)  && !m_pGLayer->NumColorEntries() &&
		crWindow.Width() >= NTI_BLKWIDTH && crWindow.Height() >= NTI_BLKWIDTH;

	switch(iTyp) {
#ifdef _USE_LZMA
		case NTI_FCNLZMA :
#endif
		case NTI_FCNZLIB :
		case NTI_FCNLOCO : args=sFilter[CExportNTI_Adv::m_Adv.bFilter];
			break;
		case NTI_FCNJP2K : if(!CExportNTI_Adv::GetRateStr(args)) bGetSSIM=false;
			break;
		case NTI_FCNWEBP : args.Format("%s %s", webp_preset[CExportNTI_Adv::m_Adv.bPreset],GetFloatStr(CExportNTI_Adv::m_Adv.fQuality,1));
			break;
		default :
			ASSERT(0);
	}

	if (bGetSSIM) args += " S";

	text.Format("%s %s",nti_xfr_name[iTyp],(LPCSTR)args);
	if(CExportNTI_Adv::m_bNoOverviews) text+=" Trunc";
	SetText(IDC_ST_ADVSETTINGS,text);
}

BOOL CExportNTI::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	SetWndTitle(this,0,m_pGLayer->FileName());
	SetWndTitle(GetDlgItem(IDC_ST_REGION),0,m_pGLayer->Width(),m_pGLayer->Height());
	if(m_crWindow.IsRectEmpty()) {
		ASSERT(m_bExtentWindow!=1);
		Disable(IDC_EXTENT_WINDOW);
	}

	ShowResolution(m_bExtentWindow);

	VERIFY(!Get_crWindow());
	UpdateAdvSettings();

	m_cePathName.SetSel(0,-1);
	m_cePathName.SetFocus();
	
	return FALSE;  //We set the focus to a control
}

BEGIN_MESSAGE_MAP(CExportNTI, CDialog)
	//{{AFX_MSG_MAP(CExpSefDlg)
	ON_BN_CLICKED(IDBROWSE, OnBrowse)
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDCANCEL, OnBnClickedCancel)
	ON_BN_CLICKED(IDC_EXPORT, OnBnClickedExport)
	ON_MESSAGE(WM_MYPROGRESS,  OnProgress)
	ON_BN_CLICKED(IDC_ADVANCED, OnBnClickedAdvanced)
	ON_BN_CLICKED(IDC_EXTENT_ENTIRE, OnBnClickedExtentEntire)
	ON_BN_CLICKED(IDC_EXTENT_WINDOW, OnBnClickedExtentWindow)
	ON_BN_CLICKED(IDC_EXTENT_CUSTOM, OnBnClickedExtentCustom)
    ON_MESSAGE(WM_COMMANDHELP,OnCommandHelp)
	ON_BN_CLICKED(IDC_ADD_EXPORTED, &CExportNTI::OnBnClickedAddExported)
	ON_BN_CLICKED(IDC_REPLW_EXPORTED, &CExportNTI::OnBnClickedReplWithExported)
	ON_BN_CLICKED(IDC_OPEN_EXPORTED, &CExportNTI::OnBnClickedOpenExported)
END_MESSAGE_MAP()

// CExportNTI message handlers
void CExportNTI::OnBrowse() 
{
	if(BrowseFiles(&m_cePathName,m_pathName,NTI_EXT,IDS_NTI_FILES,IDS_NTI_BROWSE)) {
		if(ForceExtension(m_pathName,NTI_EXT))
			SetText(IDC_PATHNAME,m_pathName);
	}
}

void CExportNTI::OnBnClickedAdvanced()
{
	UINT id = 0;
	if (!m_pGLayer->NumColorEntries()) {
		id = Get_crWindow();
		if (id) {
			GetDlgItem(id)->SetFocus();
			return;
		}
		id = (crWindow.Width() >= NTI_BLKWIDTH && crWindow.Height() >= NTI_BLKWIDTH); //true if exporting a whole tile
	}
	CExportNTI_Adv dlg(m_pGLayer, id); //allow SSIM computation if id!=0
	if(IDOK==dlg.DoModal()) 
		UpdateAdvSettings();
}

void CExportNTI::OnBnClickedCancel()
{
	if(!m_bProcessing) {
		OnCancel();
		return;
	}
	if(m_bProcessing==1) {
		m_job.Abort();
	}
	else {
		//m_bProcessing==2 means operation completed --
		OnOK();
	}
}

void CExportNTI::OnBnClickedExport()
{
	if(m_bProcessing) {
		(CMainFrame *)theApp.m_pMainWnd->ShowWindow(SW_MINIMIZE);
		theApp.m_bBackground=TRUE;
		//m_job.SetPriority(THREAD_PRIORITY_LOWEST);
		VERIFY(::SetPriorityClass(::GetCurrentProcess(),IDLE_PRIORITY_CLASS));
		return;
	}

	if(UpdateData(TRUE)) {
		if(!_access((LPCSTR)m_pathName,0) && CheckAccess((LPCSTR)m_pathName,2)) {
			CMsgBox("%s\nThis file is already open and/or protected. You can choose a different name.",(LPCSTR)m_pathName);
			m_cePathName.SetFocus();
			return;
		}
		Disable(IDC_PATHNAME);
		Disable(IDBROWSE);
		Disable(IDC_ADVANCED);
		Disable(IDC_EXTENT_ENTIRE);
		Disable(IDC_EXTENT_WINDOW);
		Disable(IDC_EXTENT_CUSTOM);
		Disable(IDC_EDGE_LEFT);
		Disable(IDC_EDGE_TOP);
		Disable(IDC_WIDTH);
		Disable(IDC_HEIGHT);
		//Disable(IDC_EXPORT);
		SetText(IDC_EXPORT,"Minimize");
		SetText(IDCANCEL,"Abort");
		//Hide(IDC_ST_NOTE);
		Show(IDC_PROGRESS);
		Show(IDC_ST_PERCENT);

		//Can't share open ECW between different threads!
		if(m_pGLayer->LayerType()==TYP_GDAL) {
			GDALClose(((CGdalLayer*)m_pGLayer)->m_pGDAL);
			((CGdalLayer*)m_pGLayer)->m_pGDAL=NULL;
		}
		m_job.SetWindow(crWindow,bExtentWindow);
		m_job.SetPathName(m_pathName);

		nti_def_timing=true; //for now
		nti_tm_def=0;
		nti_tm_def_cnt=0;

		m_bProcessing=TRUE;
		//IDOK is labeled "Exit" and is currently hidden
		m_job.Begin(this, WM_MYPROGRESS);	 // Start the job
	}
}

//////////////////
// Handle progress notification from thread.
//    wp = 0   ==> done
//	  p = -1  ==> aborted
//	  else wp  =  percentage of work completed.
//
LRESULT CExportNTI::OnProgress(WPARAM wp, LPARAM lp)
{
	if((CMainFrame *)theApp.m_pMainWnd->IsIconic()) {
		if(wp<=0 || !theApp.m_bBackground) {
#ifdef _DEBUG
			int pr=m_job.GetPriority();
#endif
//			m_job.SetPriority(THREAD_PRIORITY_NORMAL);
			VERIFY(::SetPriorityClass(::GetCurrentProcess(),NORMAL_PRIORITY_CLASS));
			GetMF()->UpdateFrameTitleForDocument(m_pDoc->GetTitle());
			(CMainFrame *)theApp.m_pMainWnd->ShowWindow(SW_RESTORE);
		}
	}

	if((int)wp<0) {
		//Thread has terminated via CExportJob::ExitNTI() --
		theApp.m_bBackground=FALSE;
		OnOK();
		CGdalLayer::m_uRasterIOActive=0;
		return 0;
	}

	CString s;
	if(wp) {
		//Thread still active  --
		if(theApp.m_bBackground) {
			s.Format("%.0f%%",(100.0*wp)/lp);
			GetMF()->UpdateFrameTitleForDocument(s);
			return 0;
		}
		s.Format("Records: %u of %u",wp,lp);
		SetText(IDC_ST_PERCENT,s);
		m_Progress.SetPos((100*wp)/lp);
		return 0;
	}

	//Thread has finished successfully --

	CGdalLayer::m_uRasterIOActive=0;

	Hide(IDC_ST_PERCENT);
	Hide(IDC_EXPORT);
	Hide(IDCANCEL);
	Hide(IDC_ST_PROGRESS);
	Hide(IDC_PROGRESS);

	__int64 feSize,fdSize;
	char dSize[20],eSize[20];

	if(CExportNTI_Adv::m_bNoOverviews) {
		fdSize=m_job.nti.SizeDecoded(0);
		feSize=m_job.nti.SizeEncoded(0);
	}
	else {
		fdSize=m_job.nti.SizeDecoded();
		feSize=m_job.nti.SizeEncoded();
	}
	CString ssim,stimes;
	if(ssim_count) {
#ifdef SSIM_RGB
		ssim.Format("  SSIM: %.2f %.2f %.2f (%.2f)", ssim_total[2], ssim_total[1], ssim_total[0],
			0.2126*ssim_total[2]+0.7152*ssim_total[1]+0.0722*ssim_total[0]);
#else
		ssim.Format("  SSIM: %.2f", ssim_total);
#endif
	}
	if(nti_def_timing) {
		stimes.Format(", Encoding: %.2f (%u tiles)",GetPerformanceSecs(&nti_tm_def),nti_tm_def_cnt);
	}
	s.Format("Completed - Elapsed time: %.2f secs%s\n"
		"Size: %s  Encoded: %s (%.2f%%)%s",
		TIMER_SECS(),(LPCSTR)stimes,CommaSep(dSize,fdSize),CommaSep(eSize,feSize),100.0*(double)feSize/(double)fdSize,
		(LPCSTR)ssim
	);

	if(CExportNTI_Adv::m_bNoOverviews) {
		s+="\nNOTE: No overviews were created. File will expand when opened.";
	}
	SetText(IDC_ST_STATS,s);
	Show(IDC_ST_STATS);

	if(!CExportNTI_Adv::m_bNoOverviews) {
		Show(IDC_REPLW_EXPORTED);
		Show(IDC_OPEN_EXPORTED);
		Show(IDC_ADD_EXPORTED);
		if (!m_pGLayer->IsTransformed()) {
			SetCheck(IDC_ADD_EXPORTED, 0);
			Disable(IDC_ADD_EXPORTED);
		}
	}
	
	Hide(IDC_ADVANCED);
	Show(IDOK); //exit button

	m_bProcessing=2;
	theApp.m_bBackground=FALSE;
	MessageBeep(-1);

	return 0;
}

void CExportNTI::SetUintText(UINT id,UINT uVal)
{
	static char buf[24];
	sprintf(buf,"%u",uVal);
	SetText(id,buf);
}

void CExportNTI::ShowResolution(BOOL bExtent)
{
	CSize res(m_pGLayer->Width(),m_pGLayer->Height());
	CPoint pos(0,0);

	if(bExtent==2) {
		if(m_bExtentWindow==2 && !crWindow.IsRectEmpty()) {
			//Custom option is the first one displayed. Let's use crWindow of the last export
			//provided it's inside the current image --
			CRect rect;
			rect.IntersectRect(crWindow,CRect(pos,res));
			if(rect.EqualRect(crWindow)) {
				res=crWindow.Size();
				pos=crWindow.TopLeft();
			}
		}
		else if(m_bExtentWindow!=2) goto _enable;
	}
	else if(bExtent==1 && !m_crWindow.IsRectEmpty()) {
		res=m_crWindow.Size();
		pos=m_crWindow.TopLeft();
	}

	SetUintText(IDC_EDGE_LEFT,pos.x);
	SetUintText(IDC_EDGE_TOP,pos.y);
	SetUintText(IDC_WIDTH,res.cx);
	SetUintText(IDC_HEIGHT,res.cy);

_enable:
	if(bExtent==2) {
		Enable(IDC_EDGE_LEFT);
		Enable(IDC_EDGE_TOP);
		Enable(IDC_WIDTH);
		Enable(IDC_HEIGHT);
	}
	else {
		Disable(IDC_EDGE_LEFT);
		Disable(IDC_EDGE_TOP);
		Disable(IDC_WIDTH);
		Disable(IDC_HEIGHT);
	}
	m_bExtentWindow=bExtent;
	UINT id = Get_crWindow();
	if(!id) UpdateAdvSettings();
}

void CExportNTI::OnBnClickedExtentEntire()
{
	if(m_bExtentWindow) {
		ShowResolution(0);
	}
}

void CExportNTI::OnBnClickedExtentWindow()
{
	if(m_bExtentWindow!=1) {
		ShowResolution(1);
	}
}

void CExportNTI::OnBnClickedExtentCustom()
{
	if(m_bExtentWindow!=2) {
		ShowResolution(2);
	}
}

LRESULT CExportNTI::OnCommandHelp(WPARAM wNone, LPARAM lParam)
{
	AfxGetApp()->WinHelp(1017);
	return TRUE;
}

void CExportNTI::OnBnClickedAddExported()
{
	//Add exported as layer --
	ASSERT(m_pGLayer->IsTransformed());
	m_bAddToProject=!m_bAddToProject;
	if(m_bAddToProject) {
		m_bReplWithExported=m_bOpenExported=FALSE;
		SetCheck(IDC_REPLW_EXPORTED,0);
		SetCheck(IDC_OPEN_EXPORTED,0);
	}
}

void CExportNTI::OnBnClickedReplWithExported()
{
	//Replace source with exported
	m_bReplWithExported=!m_bReplWithExported;
	if(m_bReplWithExported) {
		m_bAddToProject=m_bOpenExported=FALSE;
		SetCheck(IDC_ADD_EXPORTED,0);
		SetCheck(IDC_OPEN_EXPORTED,0);
	}
}

void CExportNTI::OnBnClickedOpenExported()
{
	//Open exported as separate document
	m_bOpenExported = !m_bOpenExported;
	if (m_bOpenExported) {
		m_bAddToProject = m_bReplWithExported = FALSE;
		SetCheck(IDC_ADD_EXPORTED,0);
		SetCheck(IDC_REPLW_EXPORTED,0);
	}
}


// ImageViewDlg.cpp : implementation file
//

#include "stdafx.h"
#include "WallsMap.h"
#include "WallsMapDoc.h"
#include "MainFrm.h"
#include "WallsMapView.h"
#include "MapLayer.h"
#include "GdalLayer.h"
#include "NtiLayer.h"
#include "ShowShapeDlg.h"
#include "DBGridDlg.h"
#include "ImageViewDlg.h"
#include "EditImageDlg.h"
#include "TitleDlg.h"
#include "ImageZoomDlg.h"
#include "LayerSetSheet.h"
#include "ListCtrlEx.h"
#include "ListCtrlSelect.h"

using namespace ListCtrlEx;

CRect CImageViewDlg::m_rectSaved;

//Globals --
CImageViewDlg * app_pImageDlg=NULL;

static LPCSTR validExt[]=
{
	".jpg",
	".tif",
	".bmp",
	".gif",
	".png",
	".nti",
	".tiff",
	".webp"
};
#define SIZ_VALID_EXT (sizeof(validExt)/sizeof(LPCSTR))

const UINT WM_THUMBTHREADMSG = ::RegisterWindowMessage("THUMBTHREADMSG");

CPreviewPane::~CPreviewPane()
{
}

BEGIN_MESSAGE_MAP(CPreviewPane, CStatic)
	//{{AFX_MSG_MAP(CImageArea)
	ON_WM_PAINT()
	//ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPreviewPane message handlers
void CPreviewPane::OnPaint() 
{
	CImageViewDlg*  pDlg = static_cast<CImageViewDlg*>(GetParent());
	if(pDlg->m_bResizing) {
		CStatic::OnPaint();
		return;
	}

	CPaintDC dc(this);

	//First, clear the pane --
	CRect cr;
	GetClientRect(&cr);

	CImageLayer *pLayer=pDlg->GetSelectedImage();
	if(pLayer) {
#ifdef _ADJ_SCROLL
		if(pLayer==m_pLastLayer && cr.right!=m_iLastClientWidth) {
			pDlg->AdjustControls();
			GetClientRect(&cr);
		}
		m_pLastLayer=pLayer;
		m_iLastClientWidth=cr.right;
#endif
		BeginWaitCursor();
		pLayer->CopyAllToDC(&dc,cr,::GetSysColorBrush(COLOR_3DFACE));
		EndWaitCursor();
	}
	else {
#ifdef _ADJ_SCROLL
		m_pLastLayer=NULL;
#endif
		::FillRect(dc.m_hDC,&cr,::GetSysColorBrush(COLOR_3DFACE));
		if(!m_bInfoLoaded) {
			if(!m_bmInfo.LoadBitmap(IDB_DRAGINFO))
				return;
			BITMAP bm;
			VERIFY(m_bmInfo.GetBitmap(&bm));
			m_bmWidth=bm.bmWidth;
			m_bmHeight=bm.bmHeight;
			m_bInfoLoaded=true;
		}
		CDC memDC;
		VERIFY(memDC.CreateCompatibleDC(&dc));
		CBitmap *pOldBM=memDC.SelectObject(&m_bmInfo);
		ASSERT(pOldBM);

		int nX = cr.left + (cr.Width() - m_bmWidth) / 2;
		int nY = cr.top + (cr.Height() - m_bmHeight) / 3;

		dc.BitBlt(nX, nY, m_bmWidth, m_bmHeight, &memDC,0,0, SRCCOPY);
		memDC.SelectObject(&m_bmInfo);
	}
}

/*
void CPreviewPane::OnSize(UINT nType, int cx, int cy) 
{
	CStatic::OnSize(nType, cx, cy);

	CImageViewDlg*  pDlg = static_cast<CImageViewDlg*>(GetParent());
	if(!pDlg->m_bResizingX && nType==SIZE_RESTORED) {
		int i=0; //not called when finished!
	}
}
*/

/////////////////////////////////////////////////////////////////////////////
// CLVEdit

BEGIN_MESSAGE_MAP(CLVEdit, CEdit)
	//{{AFX_MSG_MAP(CLVEdit)
	ON_WM_WINDOWPOSCHANGING()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CLVEdit::OnWindowPosChanging(WINDOWPOS FAR* lpwndpos) 
{
#define EDIT_TITLE_CX 144
	if(lpwndpos->cx) {
		lpwndpos->x+=(lpwndpos->cx-EDIT_TITLE_CX)/2;
		lpwndpos->cx=EDIT_TITLE_CX;
		//lpwndpos->cy=2*CImageViewDlg::m_iTextHeight+2;
	}
	CEdit::OnWindowPosChanging(lpwndpos);
}

/////////////////////////////////////////////////////////////////////////////
// CImageData members
LPCSTR CImageData::InitFromText(LPCSTR p,CShpLayer *pShp)
{
	//p is positioned at the first byte following [~] ...
	csPath.Empty();

	while(xisspace((BYTE)*p)) p++;

	LPCSTR p0=strstr(p,"\r\n");
	if(!p0) return p;

	if(p0>p)
		csTitle.SetString(p,p0-p);
	else csTitle.Empty();

	p0+=2;
	while(xisspace((BYTE)*p0)) p0++;

	char buf[MAX_PATH];
	if((p=strstr(p0,"\r\n"))) {
		if(p-p0>=MAX_PATH) return p+2;
		memcpy(buf,p0,p-p0);
		buf[p-p0]=0;
		p+=2;
		p0=buf;
	}
	else {
		p=p0+strlen(p0);
		if(p-p0>MAX_PATH) return p;
	}

	if(*p0) {
		char buf2[MAX_PATH];
		LPCSTR pFull=pShp->GetFullFromRelative(buf2,p0);
		if(!pFull || !CImageViewDlg::IsValidExt(trx_Stpext(p0=pFull)) || _access(p0,4)) {
			csPath="*";
			csPath+=p0;
			return p;
		}
		csPath=p0;
		if(csTitle.IsEmpty()) {
			p0=trx_Stpnam(p0);
			csTitle.SetString(p0,trx_Stpext(p0)-p0);
		}
		if(!(p0=CDBTData::NextImage(p))) p0=p+strlen(p);
		if(p0>p && p0[-1]=='\n') p0--;
		if(p0>p && p0[-1]=='\r') p0--;
		csDesc.SetString(p,p0-p);
		p=p0;
	}
	return p;
}

/////////////////////////////////////////////////////////////////////////////
// CImageViewDlg dialog

IMPLEMENT_DYNAMIC(CImageViewDlg, CMemoEditDlg)

CImageViewDlg::CImageViewDlg(CDialog *pParentDlg,CShpLayer *pShp,EDITED_MEMO &memo,CWnd* pParent /*=NULL*/)
	: CMemoEditDlg(CImageViewDlg::IDD,pParentDlg,pShp,memo,pParent)
	, m_iListSel(-1)
	, m_bResizing(false)
	, m_bThreadActive(false)
	, m_bChanged(memo.bChg>1)
	, m_pThread(NULL)
	, m_iLastIncHeight(INT_MAX)
	, m_pwchTip(NULL)
	, m_pOldFont(NULL)
	, m_bEditing(false)
	, m_nBadLinks(0)
{
	ASSERT(!app_pImageDlg && memo.pData);
	VERIFY(m_memo.pData=_strdup(memo.pData));
	memo.bChg=0;
	if(Create(CImageViewDlg::IDD,pParent)) {
		app_pImageDlg=this;
		RefreshTitle();
	}
}

CImageViewDlg::~CImageViewDlg()
{
	ASSERT(!app_pImageDlg);
	TerminateThread();
	for(it_ImageData it=m_vImageData.begin();it!=m_vImageData.end();it++) {
		delete it->pLayer; //will close handle if open
	}
	free(m_pwchTip);
	free(m_memo.pData);
	if(m_pOldFont) {
		m_dc.SelectObject(m_pOldFont);
		m_dc.DeleteDC();
	}
}

void CImageViewDlg::PostNcDestroy() 
{
	ReturnFocus();
	app_pImageDlg=NULL;
	delete this;
}

void CImageViewDlg::ClearImageData()
{
	for(it_int it=m_vIndex.begin();it!=m_vIndex.end();it++) {
		delete m_vImageData[*it].pLayer; //will close handle if open
	}
	m_vIndex.clear();
	m_vImageData.clear();
}

void CImageViewDlg::ReInit(CShpLayer *pShp,EDITED_MEMO &memo)
{
	ASSERT(app_pShowDlg || pShp->m_pdbfile->pDBGridDlg);
	ASSERT(memo.pData);
	if(!memo.pData) {
		ASSERT(0);
		return;
	}

	TerminateThread();

	m_iListSel=-1;
	m_ThumbList.SetRedraw(FALSE);
	VERIFY(m_ThumbList.DeleteAllItems());
	VERIFY(m_ImageList.SetImageCount(0));
	m_ThumbList.SetRedraw(TRUE);
	m_ThumbList.UpdateWindow();
	m_bResizing=false;
#ifdef _ADJ_SCROLL
	m_PreviewPane.m_pLastLayer=NULL;
#endif
	m_PreviewPane.Invalidate();
	m_PreviewPane.UpdateWindow();

	m_pShp=pShp;
	free(m_memo.pData);
	m_bChanged=(memo.bChg>1); //if from editor, bChg>0
	memo.bChg=0;
	m_memo=memo;
	m_memo.pData=_strdup(memo.pData);
	RefreshTitle();

	ClearImageData();
	if(m_memo.pData && !InitImageData()) {
		Destroy();
		return;
	}
	DisplayData();
	AdjustControls();
}

bool CImageViewDlg::DenyClose(BOOL bForceClose /*=FALSE*/)
{
	if(!m_bChanged) return false;

	int id=m_pShp->ConfirmSaveMemo(m_hWnd,m_memo,bForceClose);

	if(id==IDYES) {
		//save
		if(m_pShp->SaveAllowed(1)) SaveImageData();
		else if(!bForceClose) id=IDCANCEL;
	}

	if(id==IDCANCEL) {
		//not possible with forced close
		return true;
	}

	//else id==IDNO -- will discard changes

	return false;
}

BOOL CImageViewDlg::PreTranslateMessage(MSG* pMsg) 
{
	if(pMsg->message == WM_KEYDOWN) {
		if(pMsg->wParam==VK_RETURN || pMsg->wParam==VK_ESCAPE) {
			return m_bEditing?FALSE:TRUE;
		}
	}
	return CResizeDlg::PreTranslateMessage(pMsg);
}

void CImageViewDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizeDlg::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_THUMB_LIST, m_ThumbList);
	DDX_Control(pDX, IDC_PREVIEW_PANE, m_PreviewPane);
}

BEGIN_MESSAGE_MAP(CImageViewDlg, CResizeDlg)
	ON_NOTIFY(LVN_BEGINLABELEDIT, IDC_THUMB_LIST, OnBeginLabelEdit)
	ON_NOTIFY(LVN_ENDLABELEDIT, IDC_THUMB_LIST, OnEndLabelEdit)
	ON_NOTIFY(LVN_GETDISPINFO, IDC_THUMB_LIST, GetDispInfo)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_THUMB_LIST, OnItemChangedList)
	ON_NOTIFY(LVN_ITEMCHANGING, IDC_THUMB_LIST, OnItemChanging)
	ON_NOTIFY(NM_DBLCLK, IDC_THUMB_LIST, OnItemDblClick)
	ON_REGISTERED_MESSAGE(WM_THUMBTHREADMSG,OnThumbThreadMsg)
	//ON_MESSAGE(WM_EXITSIZEMOVE, OnExitSizeMove)
	ON_WM_NCMOUSEMOVE()
	ON_BN_CLICKED(IDC_BUTTONLEFT, OnBnClickedButtonleft)
	ON_BN_CLICKED(IDC_BUTTONRIGHT, OnBnClickedButtonright)
	ON_WM_CONTEXTMENU()
    ON_COMMAND(ID_EDIT_PASTE, OnEditPaste)
	ON_COMMAND(ID_IMG_PROPERTIES, &CImageViewDlg::OnImgProperties)
	ON_COMMAND(ID_IMG_REMOVE, &CImageViewDlg::OnImgRemove)
	ON_COMMAND(ID_IMG_INSERT, &CImageViewDlg::OnImgInsert)
	ON_UPDATE_COMMAND_UI(ID_IMG_COLLECTION, &CImageViewDlg::OnUpdateImgCollection)
	ON_BN_CLICKED(IDC_SAVEANDEXIT, &CImageViewDlg::OnBnClickedSaveandexit)
	ON_COMMAND(ID_REMOVEALL, &CImageViewDlg::OnRemoveall)
	ON_COMMAND(ID_OPENFOLDER, &CImageViewDlg::OnOpenfolder)
    ON_MESSAGE(WM_COMMANDHELP,OnCommandHelp)
	ON_WM_NCLBUTTONDOWN()
	//ON_WM_NCLBUTTONUP()
	//ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
END_MESSAGE_MAP()

void CImageViewDlg::WaitForThread()
{
	if(m_bThreadActive) {
		//WaitForSingleObject(pThread->m_hThread, INFINITE); //doesn't work
		for( ; ; )
		{
			if(!m_pThread || ::WaitForSingleObject(m_pThread->m_hThread, 0) == WAIT_OBJECT_0 )
				break;

			MSG msg;
			while (::PeekMessage(&msg,NULL,0,0,PM_NOREMOVE)) 
			{ 
				if (!AfxGetApp()->PumpMessage()) 
				break; 
			} 
		}
		//::CloseHandle(m_hThread);
		m_bThreadActive=false;
	}
	delete m_pThread;
	m_pThread=NULL;
}

void CImageViewDlg::TerminateThread()
{
	if(m_bThreadActive) {
		m_bThreadTerminating=true;
		WaitForThread();
	}
}

UINT CImageViewDlg::LoadThumbNails(LPVOID lpParam)
{
	CImageViewDlg* pDlg=(CImageViewDlg*)lpParam;

	CListCtrl& ListCtrl=pDlg->m_ThumbList;
	CImageList& ImgList=pDlg->m_ImageList;
	std::vector<CImageData> &ImgData=pDlg->m_vImageData;

	// Create Brushes for Border and BackGround
	//HBRUSH hBrushBorder=(HBRUSH)::GetStockObject(BLACK_BRUSH);
	//HBRUSH hBrushBk=(HBRUSH)::GetStockObject(GRAY_BRUSH);
	CBrush cbrBkgrnd(::GetSysColor(COLOR_BTNFACE));
	CRect rcBorder(0,0,THUMBNAIL_WIDTH,THUMBNAIL_HEIGHT);
	CClientDC dc(&ListCtrl);

	int nIndex=pDlg->m_iListImages;
	ASSERT((int)pDlg->m_vIndex.size()>nIndex);

	for(it_int it=pDlg->m_vIndex.begin()+nIndex;!pDlg->m_bThreadTerminating && it!=pDlg->m_vIndex.end();it++,nIndex++) {
		CDC memDC;
		VERIFY(memDC.CreateCompatibleDC(&dc));
		CBitmap cbm;
		VERIFY(cbm.CreateCompatibleBitmap(&dc, THUMBNAIL_WIDTH, THUMBNAIL_HEIGHT));
		CBitmap *pOldBM=memDC.SelectObject(&cbm);
		ASSERT(pOldBM);
		// Draw Background
		::FillRect(memDC.m_hDC,&rcBorder,(HBRUSH)cbrBkgrnd.m_hObject);
		// Draw image
		CImageData &image=ImgData[*it];
		CImageLayer *pLayer=image.pLayer;
		ASSERT(pLayer);
		if(pLayer && pLayer->OpenHandle(image.csPath)) {
			pLayer->CopyAllToDC(&memDC,rcBorder); //draw thumbnail
			pLayer->CloseHandle();
		}
		// Draw Border
		//rcBorder.InflateRect(-1,-1,-1,-1);
		//::FrameRect(memDC.m_hDC,&rcBorder,(HBRUSH)::GetStockObject(GRAY_BRUSH));
		memDC.SelectObject(pOldBM);

		if(!pDlg->m_hWnd) break;

		// Replace image in CImageList.
		VERIFY(ImgList.Replace(*it,&cbm,NULL));
		pDlg->PostMessage(WM_THUMBTHREADMSG,(WPARAM)nIndex);
		if(pDlg->m_bThreadTerminating) break;
	}

	pDlg->m_bThreadActive=false;
	return 0;
}

#define MAXDATALEN 139

bool CImageViewDlg::ImageFound(LPCSTR path)
{
	for(it_int it=m_vIndex.begin();it!=m_vIndex.end();it++) {
		if(!_stricmp(m_vImageData[*it].csPath,path)) return true;
	}
	return false;
}

bool CImageViewDlg::InitThumbnail(CImageData &image)
{
	CImageLayer *pLayer=NULL;

	try {
		if(CLayerSet::GetLayerType(image.csPath)==TYP_NTI)
			pLayer=new CNtiLayer();
		else
			pLayer=new CGdalLayer();

		if(!pLayer->Open(image.csPath,NTL_FLG_SILENT)) {
			throw 0;
		}

		image.pLayer=pLayer;
	}
	catch(...) {
		delete pLayer;
		return false;
	}

	return true;
}

void CImageViewDlg::NoDropMsg()
{
	CMsgBox("%s\n\nSince this record is not currently editable, images can't be added or rearranged.",
		m_pShp->Title());
}

void CImageViewDlg::OnDrop()
{
	if(!IsEditable()) {
		NoDropMsg();
		return;
	}

	WaitForThread(); //Insure all thumbnails are loaded (shouldn't have come this far!)

	UINT nFiles=CFileDropTarget::m_vFiles.size();
	ASSERT(m_vIndex.size()==m_nImages);

	try {
		m_vImageData.reserve(m_vImageData.size()+nFiles);
		m_vIndex.reserve(m_nImages+nFiles);
	}
	catch(...) {
		ASSERT(0); //for now;
		return;
	}

	m_ThumbList.SetRedraw(FALSE);

	CImageData image;
	int nFailures=0,nDups=0,nAdded=0;

	for(V_FILELIST::iterator itf=CFileDropTarget::m_vFiles.begin();itf!=CFileDropTarget::m_vFiles.end();itf++) {
		if(ImageFound(*itf)) {
			nDups++;
			continue;
		}
		if(_access(*itf,4)) {
			nFailures++;
			continue;
		}
		image.csTitle=trx_Stpnam(image.csPath=*itf);
		LPCSTR p=(LPCSTR)image.csTitle;
		image.csTitle.Truncate(trx_Stpext(p)-p);
		int ht=FixIconTitle(image.csTitle);

		if(!InitThumbnail(image)) {
			nFailures++;
			continue;
		}
		if(nAdded)
			image.pLayer->CloseHandle(); //Keep existing selection open

		int nIndex=m_vImageData.size();
		m_vImageData.push_back(image);
		m_vIndex.push_back(nIndex);
		m_ThumbList.InsertItem(m_nImages,LPSTR_TEXTCALLBACK,I_IMAGECALLBACK);
		nAdded++;
		m_nImages++;
		if(ht>m_iMaxDataHt) {
			m_iMaxDataHt=ht;
		}
	}

	m_ThumbList.SetRedraw(TRUE);
	if(nAdded) {
		EnableSave(m_bChanged=true);
		AdjustControls();
		m_ImageList.SetImageCount(m_vImageData.size());
		m_ThumbList.UpdateWindow();
		//Now fill thumbnail rectangles --
		PostMessage(WM_THUMBTHREADMSG,0,m_nImages-nAdded); //index of first new image
	}
}

void CImageViewDlg::RefreshIconTitleSize()
{
	CRect rect0(0,0,MAXDATALEN,0);
	m_iMaxDataHt=m_iTextHeight;
	for(it_ImageData it=m_vImageData.begin();it!=m_vImageData.end();it++) {
		CRect rect(rect0);
		int cy=m_dc.DrawText(it->csTitle,it->csTitle.GetLength(),&rect,DT_CENTER|DT_WORDBREAK|DT_CALCRECT);
		ASSERT(cy==rect.Height() && cy>=m_iTextHeight && cy<=2*m_iTextHeight && rect.Width()<=MAXDATALEN);
		if(cy>m_iTextHeight) {
			m_iMaxDataHt*=2;
			break;
		}
	}
	AdjustControls();
}

int CImageViewDlg::FixIconTitle(CString &title)
{
	//Truncate title if too long and return size of enclosing retangle,
	//which may contain two lines --
	char buf[MAX_PATH];

	title.Trim();
	title.Replace('_',' ');
	strcpy(buf,title);

	int len1=strlen(buf);
	int len0=len1;

	CRect rect0(0,0,MAXDATALEN,0);

	int maxHt=m_iTextHeight;

	while(len1>=0) {
		buf[len1]=0;
		CRect rect(rect0);
		if((maxHt=m_dc.DrawText(buf,len1,&rect,DT_CENTER|DT_WORDBREAK|DT_CALCRECT))<=2*m_iTextHeight && rect.Width()<=MAXDATALEN)
			break;
		maxHt=m_iTextHeight;
		len1-=4;
	}
	if(len1<len0) {
		if(len1<3) len1=3;
		strcpy(buf+len1-3,"...");
		title.SetString(buf,len1);
	}
	return maxHt;
}

LPCSTR CImageViewDlg::GetSetTitle()
{
	m_SetTitle.Empty();
	LPCSTR p=CDBTData::NextImage(m_memo.pData);
	if(p>m_memo.pData && (p=(LPCSTR)memchr(m_memo.pData,'\r',p-m_memo.pData))) {
		m_SetTitle.SetString(m_memo.pData,p-m_memo.pData);
		m_SetTitle.Trim();
	}
	return p?p:m_memo.pData;
}

BOOL CImageViewDlg::InitImageData()
{
	CSize sz;
	CImageData image;
	CString badPath;
	int nLinks=0;
	
	m_iMaxDataHt=m_nBadLinks=0;

	LPCSTR p=GetSetTitle();

	while((p=CDBTData::NextImage(p))) {
		nLinks++;
		p=image.InitFromText(p+3,m_pShp);
		if(image.IsPathValid()) {
			int ht=FixIconTitle(image.csTitle);
			if(ht>m_iMaxDataHt)	m_iMaxDataHt=ht;
			m_vImageData.push_back(image);
		}
		else if(!image.csPath.IsEmpty()) {
			if(!m_nBadLinks) {
				badPath=(LPCSTR)image.csPath+1;
			}
			m_nBadLinks++;
		}
	}

	if(m_nBadLinks) {
		CString title;

		LPCSTR pmsg=(m_nBadLinks!=nLinks)?
			"If you revise content from within the viewer, any bad links will be discarded along with their properties":
			"If the problem is due to many files being relocated, you can more easily fix the links "
					"by opening a table view and selecting \"Find broken image links\" in the header's context menu";

		CMsgBox(MB_ICONINFORMATION,
			"%s\n\nNOTE: %u (of %u) linked image files could not be found. One such file is\n\n"
			"%s\n\nTo view the links click the image field's button while pressing the CTRL key. %s.",
			m_pShp->GetMemoTitle(title,m_memo),m_nBadLinks,nLinks,(LPCSTR)badPath,pmsg);

		if(m_nBadLinks==nLinks)
			return FALSE;
	}

	m_bEditable=m_pShp->IsEditable() && (!m_memo.uRec || !m_pShp->IsRecDeleted(m_memo.uRec))
		/*&& !CDBGridDlg::m_bNotAllowingEdits*/ && !m_pShp->IsFldReadOnly(m_memo.fld);
	EnableSave(m_bEditable && (m_bChanged || m_nBadLinks));
	return TRUE;
}

void CImageViewDlg::InitThumbnails()
{
	m_ThumbList.SetRedraw(FALSE);
	int nIndex=0,nImage=0;

	for(it_ImageData it=m_vImageData.begin();it!=m_vImageData.end();it++,nImage++)
	{
		if(!InitThumbnail(*it))
			continue;

		if(nIndex)
			it->pLayer->CloseHandle(); //Keep the first file open

		m_ThumbList.InsertItem(nIndex,LPSTR_TEXTCALLBACK,I_IMAGECALLBACK);
		m_vIndex.push_back(nImage);
		nIndex++;
	}

	m_nImages=nIndex;
	ASSERT(nIndex==m_vIndex.size());
	m_ThumbList.SetRedraw(TRUE);
	m_ThumbList.UpdateWindow();
}

void CImageViewDlg::DisplayData()
{
	//m_iListSel=-1; //set in constructor
	m_iListImages=m_nImages=0;

#ifdef _USE_GDTIMER
	secs_rasterio=secs_stretch=0.0;
#endif

	if(m_vImageData.size()) {
		//if(m_vImageData.size()>1)
			//std::sort(m_vImageData.begin(),m_vImageData.end(),vec_imagedata_pred);

		ASSERT(!m_ImageList.GetImageCount());

		m_ImageList.SetImageCount(m_vImageData.size());

		InitThumbnails();

		if(m_nImages) {

			PostMessage(WM_THUMBTHREADMSG); //Will draw first thumbnail and SetCurSel()

		}
	}
	else {
		m_bResizing=false;
		m_PreviewPane.Invalidate();
		m_PreviewPane.UpdateWindow();
	}

	m_bEmptyOnEntry=(m_nImages==0);

	m_ThumbList.SetFocus();
}

bool CImageViewDlg::ScrollBarPresent()
{
#ifdef _ADJ_SCROLL
	//return (m_ThumbList.GetStyle()&WS_HSCROLL)!=0; //fails when count>4 and initial window width is large
	if(m_vImageData.size()<=4) return false;
	CRect crect;
	m_PreviewPane.GetClientRect(&crect);
	int needed=(124+143*m_vImageData.size());
	return crect.right+20 <= needed;
#else
	return m_vImageData.size()>4;
#endif
}

void CImageViewDlg::AdjustControls()
{
	if(!m_iMaxDataHt) m_iMaxDataHt=13; //kluge to fix squashed list window when empty!

	int incHeight=(ScrollBarPresent()?16:0)-(2*m_iTextHeight-m_iMaxDataHt); //increase due to hz scrollbar and titles

	int incDiff=(m_iLastIncHeight==INT_MAX)?incHeight:(incHeight-m_iLastIncHeight);

	if(incDiff) {
		//Adjust listbox height and preview pane for scroll bars and or text wrap while
		//keeping the dialog box the same height --
		ASSERT(m_iLastIncHeight!=INT_MAX || !m_Items.size());
		m_Items.clear();
		CRect r;
		m_ThumbList.GetWindowRect(&r); //no conversion necessary since we're only resizing listbox --
		m_ThumbList.SetWindowPos(NULL,0,0,r.Width(),r.Height()+incDiff,SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);
		m_PreviewPane.GetWindowRect(&r);
		ScreenToClient(&r.TopLeft());
		ScreenToClient(&r.BottomRight());
		m_PreviewPane.SetWindowPos(NULL,r.left,r.top+incDiff,r.Width(),r.Height()-incDiff,SWP_NOZORDER|SWP_NOACTIVATE);
	}

	if(incDiff || (m_iLastIncHeight==INT_MAX)) {
		AddControl(IDC_THUMB_LIST, CST_RESIZE, CST_RESIZE, CST_NONE, CST_NONE, 0);
		AddControl(IDC_PREVIEW_PANE, CST_RESIZE, CST_RESIZE, CST_RESIZE, CST_RESIZE, 0);
		AddControl(IDOK, CST_REPOS, CST_REPOS, CST_NONE, CST_NONE, 0);
		AddControl(IDC_ST_COUNTER,CST_REPOS, CST_REPOS, CST_NONE, CST_NONE, 0);
		AddControl(IDC_BUTTONLEFT,CST_REPOS, CST_REPOS, CST_NONE, CST_NONE, 0);
		AddControl(IDC_BUTTONRIGHT,CST_REPOS, CST_REPOS, CST_NONE, CST_NONE, 0);
		AddControl(IDC_SAVEANDEXIT,CST_REPOS, CST_REPOS, CST_NONE, CST_NONE, 0);
	}

	GetDlgItem(IDOK)->EnableWindow(m_nImages>0);
	GetDlgItem(IDC_BUTTONLEFT)->EnableWindow(m_nImages>1);
	GetDlgItem(IDC_BUTTONRIGHT)->EnableWindow(m_nImages>1);

	m_iLastIncHeight=incHeight;
}

void CImageViewDlg::Destroy()
{
	GetWindowRect(&m_rectSaved);
	DestroyWindow();
}

BOOL CImageViewDlg::OnInitDialog()
{
	CResizeDlg::OnInitDialog();

	//CWnd *parent=GetParent();
	//CRect rect;
	//parent->GetWindowRect(&rect);
	//SetWindowPos(0,rect.left+100,rect.top-100,0,0,SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOSIZE|SWP_NOREDRAW);
	ASSERT(m_memo.pData);

	if(m_dc.CreateCompatibleDC(NULL)) {
		if(!(m_pOldFont=m_dc.SelectObject(m_ThumbList.GetFont()))) {
			m_dc.DeleteDC();
		}
		else {
			TEXTMETRIC tm;
			m_dc.GetTextMetrics(&tm);
			m_iTextHeight=tm.tmHeight;
			CRect r;
			m_ThumbList.GetWindowRect(&r);
			m_iListHeight=r.Height(); //space for two lines /w no scrollbar
			m_ThumbList.ModifyStyle(WS_HSCROLL | WS_VSCROLL, 0 ); 
		}
	}

	if(!m_pOldFont || m_memo.pData && !InitImageData()) {
		EndDialog(IDCANCEL);
		return FALSE;
	}

	ListView_SetExtendedListViewStyleEx(m_ThumbList.m_hWnd,LVS_EX_BORDERSELECT,LVS_EX_BORDERSELECT);

	// Initialize Imaget List and Attach it to ListCtrl
	m_ImageList.Create(THUMBNAIL_WIDTH, THUMBNAIL_HEIGHT, ILC_COLOR24, 0, 1);
	m_ThumbList.SetImageList(&m_ImageList, LVSIL_NORMAL);
	m_ThumbList.SetBkColor(GetSysColor(COLOR_BTNFACE));
	m_ThumbList.SetTextBkColor(GetSysColor(COLOR_BTNFACE));

#ifdef _ADJ_SCROLL
	m_PreviewPane.m_pLastLayer=NULL;
#endif

	DisplayData();
	AdjustControls();

	if(!m_rectSaved.IsRectEmpty()) {
		Reposition(m_rectSaved);
#ifdef _ADJ_SCROLL
		AdjustControls();
#endif
	}

#ifdef _DEBUG
	m_dropTarget.m_bTesting=true;
#endif

    VERIFY(m_dropTarget.Register(this,DROPEFFECT_LINK));     

	return FALSE;  // return TRUE unless you set the focus to a control
}

LPSTR CImageViewDlg::GetTooltipText(int index,UINT code)
{
	if((UINT)index>=m_vIndex.size()) return FALSE;
	CImageData &data=m_vImageData[m_vIndex[index]];
	ASSERT(data.pLayer);
	data.pLayer->GetNameAndSize(m_csToolTipText,data.csPath);
	if(!data.csDesc.IsEmpty()) {
		m_csToolTipText+="\r\n";
		m_csToolTipText+=data.csDesc;
	}
	UINT len=m_csToolTipText.GetLength()+1;
	if(len<=1) return NULL;

	if(code==TTN_NEEDTEXTA) {
		ASSERT(0); //is this possible?
		return (LPSTR)(LPCSTR)m_csToolTipText;
	}
	if(!m_pwchTip || m_wchTipSiz<len) {
		m_wchTipSiz=80*(1+(len-1)/80);
		m_pwchTip=(WCHAR *)realloc(m_pwchTip,m_wchTipSiz*sizeof(WCHAR));
	}
	if(m_pwchTip) { 
		_mbstowcsz(m_pwchTip,m_csToolTipText,len);
		ASSERT(m_pwchTip[len-1]==(WCHAR)0);
		return (LPSTR)m_pwchTip;
	}
	return NULL;
}

void CImageViewDlg::RefreshIconTitles()
{
	int i=0;
	for(it_int it=m_vIndex.begin();it!=m_vIndex.end();it++,i++) {
		m_ThumbList.SetItemText(i, 0,m_vImageData[*it].csTitle);
	}
}

void CImageViewDlg::MoveSelected(int nDragIndex,int nDropIndex)
{
	//Move item nDragIndex to just below nDropIndex
	ASSERT(nDragIndex==m_iListSel);

	if(!IsEditable()) {
		NoDropMsg();
		return;
	}

	int iTemp=m_vIndex[nDragIndex];
	if(nDropIndex>nDragIndex) nDropIndex--;
	ASSERT(nDropIndex!=nDragIndex);
	if(abs(nDropIndex-nDragIndex)==1) {
		m_vIndex[nDragIndex]=m_vIndex[nDropIndex];
		m_vIndex[nDropIndex]=iTemp;
	}
	else {
		m_vIndex.erase(m_vIndex.begin()+nDragIndex);
		m_vIndex.insert(m_vIndex.begin()+nDropIndex,iTemp);
	}
	EnableSave(m_bChanged=true);
	RefreshIconTitles();

	SetCurSel(m_ThumbList,nDropIndex);
}

void CImageViewDlg::OnOK()
{
	//GetWindowRect(&m_rectSaved); //why?

	if(m_iListSel<0) return;

	CImageData *pndat=NULL,*pidat=&m_vImageData[m_vIndex[m_iListSel]];
	ASSERT(pidat->pLayer);

	CString sPath(pidat->csPath);
	sPath.Truncate(sPath.GetLength()-strlen(trx_Stpext(sPath)));
	BOOL bLocalTyp;
	LPCSTR pExt=GetMediaTyp(sPath,&bLocalTyp);
	if(pExt && !_access(sPath,0)) {
		if(bLocalTyp) {
			//m_pShp->OpenLinkedDoc(sPath);
			pndat=new CImageData(sPath);
			if(!InitThumbnail(*pndat)) {
				ASSERT(0);
				delete pndat;
				CMsgBox("Unable to open %s.",(LPCSTR)sPath);
				return;
			}
			pidat=pndat;
		}
		else {
			//sPath.Insert(0,'\"'); sPath+='\"';
			int i=(int)ShellExecute(m_hWnd,NULL /*"open"*/,(LPCSTR)sPath, NULL, NULL, SW_NORMAL);
			if(i<=32) {
#ifdef _DEBUG
				switch(i) {
					case SE_ERR_NOASSOC:
						break; //=31, caused by "open", go to def to see other codes
				}
#endif
				CMsgBox("Unable to open %s, which was found alongside %s. "
					"The extension %s may not have an associated application.",
					trx_Stpnam(sPath),trx_Stpnam(pidat->csPath),pExt);
			}
			return;
		}
	}

	CWallsMapDoc *pDoc=m_pShp->m_pDoc;
	int id;
	if(pidat->pLayer && ((id=pDoc->IsLayer(pidat->csPath)-1)>=0 || pDoc->LayerSet().IsLayerCompatible(pidat->pLayer,TRUE))) { //silent check for compatibility
		if(pndat) {
			delete pndat->pLayer;
			pndat->pLayer=NULL;
		}
		bool bZoom=false,bNew=false;
		if(id>=0) {
			//exists as layer at tree position id
			int i=AfxMessageBox("This file already exists as a layer. "
				"Do you want to zoom to its image? Select NO to open it as a separate document.",MB_YESNOCANCEL);
			if(i==IDCANCEL) {
				goto _exit;
			}
			if(i!=IDYES) id=-1; //open as doc
			else bZoom=true;
		}
		else {
			//compatible image not in layerset --
			bNew=true;
			CImageZoomDlg dlg(trx_Stpnam(pidat->csPath));
			id=dlg.DoModal();
			if(id==IDCANCEL) {
				goto _exit;
			}
			if(id==IDNO) id=-1; //open as doc
			else {
				CWallsMapView::SyncStop();
				//Creates and inserts CMapLayer object without updating displayed tree --
				if(!pDoc->AddLayer(pidat->csPath,NTL_FLG_VISIBLE)) {
					//user denied coordinate conversion
					goto _exit;
				}
				if(hPropHook && pDoc==pLayerSheet->GetDoc()) {
					//Calls PageLayers::UpdateLayerData(pDoc);
					pLayerSheet->SendMessage(WM_PROPVIEWDOC,(WPARAM)pDoc,(LPARAM)LHINT_NEWLAYER);
				}
				id=pDoc->SelectedLayerPos();
				ASSERT(id>=0);
				bZoom=(dlg.m_bZoomToAdded==TRUE);
			}
		}

		if(id>=0) {
			//Either found image or successfilly added it if bNew==true
			//Zoom to layer or refresh views
			ASSERT(bNew || bZoom);
			if(!bNew) {
				if(!hPropHook) {
					pDoc->SetSelectedLayerPos(id);
					pDoc->GetPropStatus().nTab=0; //Next invocation will show layers page
				}
				else if(pDoc==pLayerSheet->GetDoc()){
					pLayerSheet->SetActivePage(0);
					pLayerSheet->SelectLayer(id);
				}
			}
			if(bZoom) {
				POSITION pos=pDoc->GetFirstViewPosition();
				CWallsMapView *pView=(CWallsMapView *)pDoc->GetNextView(pos);
				if(pView) pView->FitImage(LHINT_FITIMAGE);
			}
			else if(bNew)
				pDoc->RefreshViews();
			goto _exit;
		}
	}

	if(pndat) {
		delete pndat->pLayer;
		pndat->pLayer=NULL;
	}

	m_pShp->OpenLinkedDoc(pidat->csPath);

_exit:
	if(pndat) {
		delete pndat->pLayer;
		delete pndat;
	}
}

// CImageViewDlg message handlers

void CImageViewDlg::OnCancel()
{
	if(m_bChanged) {
		CString title;
		CString msg;
		msg.Format("%s\n\nYou've made changes to the image collection.  Save them before exiting?",m_pShp->GetMemoTitle(title,m_memo));
		int id=MsgYesNoCancelDlg(m_hWnd,msg,m_pShp->Title(),"Save","Discard Changes","Continue Editing");
		if(id==IDCANCEL) return;
		if(id==IDYES && m_pShp->SaveAllowed(1))
			SaveImageData();
	}
	Destroy();
}

bool CImageViewDlg::IsValidExt(LPCSTR pExt)
{
	for(int i=0;i<SIZ_VALID_EXT;i++)
		if(!_stricmp(pExt,validExt[i])) return true;
	return false;
}

CImageLayer * CImageViewDlg::GetSelectedImage()
{
	if(m_iListSel<0 || m_iListSel>=m_iListImages) return NULL;
	CImageData &data=m_vImageData[m_vIndex[m_iListSel]];
	ASSERT(data.pLayer);
	if(!data.pLayer->OpenHandle(data.csPath))
		return NULL;
	return data.pLayer;
}

LRESULT CImageViewDlg::OnThumbThreadMsg(UINT wParam, LONG lParam)
{
	//Called with Postmessage() after changing bitmap in m_ImageList
	if(!wParam) {

		ASSERT((LONG)m_vIndex.size()>lParam);

		CImageLayer *pLayer=m_vImageData[m_vIndex[lParam]].pLayer;
		bool bNTI=(pLayer->LayerType()==TYP_NTI);

		ASSERT(pLayer->IsOpen());

		//Draw first thumbnail --
		CRect rcBorder(0,0,THUMBNAIL_WIDTH,THUMBNAIL_HEIGHT);
		CClientDC dc(&m_ThumbList);
		CDC memDC;
		VERIFY(memDC.CreateCompatibleDC(&dc));
		CBitmap cbm;
		VERIFY(cbm.CreateCompatibleBitmap(&dc, THUMBNAIL_WIDTH, THUMBNAIL_HEIGHT));
		CBitmap *pOldBM=memDC.SelectObject(&cbm);
		ASSERT(pOldBM);
		// Draw Background
		{
			CBrush cbr(::GetSysColor(COLOR_BTNFACE));
			::FillRect(memDC.m_hDC,&rcBorder,(HBRUSH)cbr.m_hObject);
		}
		// Draw image
		pLayer->CopyAllToDC(&memDC,rcBorder); //draw thumbnail
		// Draw Border
		//rcBorder.InflateRect(-1,-1,-1,-1);
		//::FrameRect(memDC.m_hDC,&rcBorder,(HBRUSH)::GetStockObject(GRAY_BRUSH));

		memDC.SelectObject(pOldBM);
		VERIFY(m_ImageList.Replace(lParam,&cbm,NULL));

		m_ThumbList.Invalidate();

		m_iListImages=lParam+1;

		if(m_nImages>m_iListImages) {
			//Initiate thread to draw remaining thumbnails
			if(!bNTI) {
				SetCurSel(m_ThumbList,lParam); //draws preview image while drawing thumbnails

				GetDlgItem(IDC_BUTTONLEFT)->EnableWindow(TRUE);
				GetDlgItem(IDC_BUTTONRIGHT)->EnableWindow(TRUE);
				GetDlgItem(IDOK)->EnableWindow(TRUE);

			}
			else BeginWaitCursor();

			m_pThread=AfxBeginThread(
				LoadThumbNails,			//AFX_THREADPROC pfnThreadProc,
				this,					//LPVOID pParam,
				bNTI?THREAD_PRIORITY_NORMAL:THREAD_PRIORITY_LOWEST,
				0,						//UINT nStackSize = 0,
				CREATE_SUSPENDED		//DWORD dwCreateFlags = 0,
										//LPSECURITY_ATTRIBUTES lpSecurityAttrs = NULL 
			);

			if(!m_pThread) {
				if(bNTI) EndWaitCursor(); 
				EndDialog(IDCANCEL);
				return TRUE;
			}
			m_pThread->m_bAutoDelete=FALSE;
			m_bThreadActive=true;
			m_bThreadTerminating=false;
			m_pThread->ResumeThread(); //should rtn 1

			if(bNTI) {
				WaitForThread();
				EndWaitCursor(); 
				SetCurSel(m_ThumbList,lParam); //draws preview image after thumbnails are drawn

				GetDlgItem(IDC_BUTTONLEFT)->EnableWindow(TRUE);
				GetDlgItem(IDC_BUTTONRIGHT)->EnableWindow(TRUE);
				GetDlgItem(IDOK)->EnableWindow(TRUE);
			}
		} 
		else {
			SetCurSel(m_ThumbList,lParam); //draws preview image
		}

	}
	else {
		m_ThumbList.RedrawItems(wParam,wParam);
		m_ThumbList.UpdateWindow();
		m_iListImages++;
		UpdateCounter();
	}
    return 0; // I handled this message
}

void CImageViewDlg::UpdateCounter()
{
	CString s;
	s.Format("%u of %u",m_iListSel+1,m_iListImages);
	GetDlgItem(IDC_ST_COUNTER)->SetWindowText(s);
}

void CImageViewDlg::OnItemChangedList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	if (pNMListView && pNMListView->uOldState != pNMListView->uNewState)
	{
		if ((pNMListView->uNewState & LVIS_SELECTED) == LVIS_SELECTED)
		{
			ASSERT(pNMListView->iItem>=0 && pNMListView->iItem<(int)m_vIndex.size());
			if(pNMListView->iItem!=m_iListSel) {
				if(!m_ThumbList.IsMovingSelected()) {
					//Update preview pane
					if(m_iListSel>=0) m_vImageData[m_vIndex[m_iListSel]].pLayer->CloseHandle();
					m_bResizing=false;
					m_PreviewPane.Invalidate();
				}
				else m_ThumbList.Invalidate();
				m_iListSel=pNMListView->iItem;
				UpdateCounter();
				m_ThumbList.UpdateWindow();
			}
		}
	}
	*pResult=0;
}

void CImageViewDlg::OnItemChanging(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	if (pNMListView && pNMListView->uOldState != pNMListView->uNewState)
	{
		if ((pNMListView->uNewState & LVIS_SELECTED) == LVIS_SELECTED)
		{
			ASSERT(pNMListView->iItem>=0 && pNMListView->iItem<(int)m_vIndex.size());
			if(pNMListView->iItem>=m_iListImages) {
				*pResult=TRUE;
				return;
			}
		}
	}
	*pResult=FALSE;
}

void CImageViewDlg::OnItemDblClick(NMHDR* pNMHDR, LRESULT* pResult)
{
	OnOK();
}

/*
LRESULT CImageViewDlg::OnExitSizeMove( WPARAM wp, LPARAM lp)
{
	if(wp==599)
		m_PreviewPane.Invalidate();
    return 0;
}

void CImageViewDlg::OnNcLButtonUp(UINT nFlags, CPoint point)
{
	CMemoEditDlg::OnNcLButtonUp(nFlags, point);
}
*/
void CImageViewDlg::OnNcLButtonDown(UINT nHitTest, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	switch(nHitTest) {
		case HTRIGHT :
		case HTBOTTOM :
		case HTBOTTOMRIGHT :
		case HTBOTTOMLEFT :
		case HTTOPRIGHT :
		case HTTOP :
		case HTLEFT :
			m_bResizing=true;
			break;
	}

	CDialog::OnNcLButtonDown(nHitTest, point);
}

void CImageViewDlg::OnNcMouseMove(UINT nHitTest, CPoint point)
{
	if(m_bResizing) {
		m_bResizing=false;
		m_PreviewPane.Invalidate();
	}
	CDialog::OnNcMouseMove(nHitTest, point);
}

void CImageViewDlg::OnMouseMove(UINT nFlags, CPoint point)
{
	if(m_bResizing) {
		m_bResizing=false;
		m_PreviewPane.Invalidate();
	}
	CMemoEditDlg::OnMouseMove(nFlags, point);
}

void CImageViewDlg::OnBnClickedButtonleft()
{
	SendMessage(WM_NEXTDLGCTL,(WPARAM)m_ThumbList.GetSafeHwnd(),TRUE);
	if(m_iListImages<=1 || m_iListSel<0) {
		ASSERT(0);
		return;
	}
	if(m_iListSel>0) SetCurSel(m_ThumbList,m_iListSel-1);
	else SetCurSel(m_ThumbList,m_iListImages-1);
}

void CImageViewDlg::OnBnClickedButtonright()
{
	SendMessage(WM_NEXTDLGCTL,(WPARAM)m_ThumbList.GetSafeHwnd(),TRUE);
	if(m_iListImages<=1 || m_iListSel<0) {
		ASSERT(0);
		return;
	}
	if(m_iListSel<m_iListImages-1) SetCurSel(m_ThumbList,m_iListSel+1);
	else SetCurSel(m_ThumbList,0);
}

void CImageViewDlg::OnContextMenu(CWnd *pWnd, CPoint pt)
{
	if(m_bThreadActive || m_ThumbList.IsMovingSelected())
		return;

	/*
	if(pWnd!=&m_ThumbList) return;

	CPoint ptc(pt);
	m_ThumbList.ScreenToClient(&ptc);
	m_iListContext=m_ThumbList.HitTest(ptc);
	*/

	m_iListContext=m_iListSel;

	CMenu menu;
	if(menu.LoadMenu(IDR_IMAGE_CONTEXT)) {
		CMenu* pPopup = menu.GetSubMenu(0);
		ASSERT(pPopup != NULL);
		if(!m_nImages || !IsEditable()) {
			pPopup->EnableMenuItem(ID_IMG_REMOVE,MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
			pPopup->EnableMenuItem(ID_REMOVEALL,MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
			if(m_nImages) {
				pPopup->EnableMenuItem(ID_IMG_INSERT,MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
			}
		}

		if(m_iListContext<0) {
			pPopup->EnableMenuItem(ID_IMG_PROPERTIES,MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
			pPopup->EnableMenuItem(ID_IMG_REMOVE,MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
		}

		{
			COleDataObject odj;
			if(!IsEditable() || !odj.AttachClipboard() || !odj.IsDataAvailable(CF_HDROP))
				pPopup->EnableMenuItem(ID_EDIT_PASTE,MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
		}


		pPopup->TrackPopupMenu(TPM_LEFTALIGN|TPM_RIGHTBUTTON,pt.x,pt.y,this);
	}
}

void CImageViewDlg::GetDispInfo(NMHDR* pNMHDR, LRESULT* pResult) 
{
	LV_ITEM* pItem= &((LV_DISPINFO*)pNMHDR)->item;

	if(pItem->mask & LVIF_TEXT) {
		lstrcpyn(pItem->pszText,(LPCSTR)m_vImageData[m_vIndex[pItem->iItem]].csTitle,pItem->cchTextMax);
	}

	if(pItem->mask & LVIF_IMAGE) {
		pItem->iImage=m_vIndex[pItem->iItem];
	}

    *pResult=0;
}

void CImageViewDlg::SaveImageData()
{
	EnableSave(m_bChanged=false);
	m_nBadLinks=0;

	CString sData;
	ASSERT(m_nImages==m_vIndex.size());

	if(m_nImages) {
		char pathBuf[MAX_PATH];

		CString s; 
		if(!m_SetTitle.IsEmpty())
			sData.Format("%s\r\n",(LPCSTR)m_SetTitle);

		for(it_int it=m_vIndex.begin();it!=m_vIndex.end();it++) {
			CImageData &iData=m_vImageData[*it];
			m_pShp->GetRelativePath(pathBuf,iData.csPath,FALSE); //last arg true if no file name at end
			s.Format("[~] %s\r\n%s\r\n%s",(LPCSTR)iData.csTitle,pathBuf,(LPCSTR)iData.csDesc);
			sData+=s;
			if(!iData.csDesc.IsEmpty()) sData+="\r\n";
		}
	}
	m_pShp->SaveEditedMemo(m_memo,sData);
}

void CImageViewDlg::OnImgProperties()
{
	ASSERT(m_iListContext>=0 && m_iListContext==m_iListSel);
	CImageData &iData=m_vImageData[m_vIndex[m_iListContext]];
	CEditImageDlg dlg;
	dlg.m_csTitle=iData.csTitle;
	dlg.m_csDesc=iData.csDesc;
	dlg.m_csPath=iData.csPath;
	ASSERT(iData.pLayer);
	iData.pLayer->GetNameAndSize(dlg.m_csSize,iData.csPath,true);
	dlg.m_bIsEditable=IsEditable();
	if(dlg.DoModal()==IDOK && IsEditable()) {
		bool bChg=false;
		FixIconTitle(dlg.m_csTitle);
		if(dlg.m_csTitle.Compare(iData.csTitle)) {
			m_ThumbList.SetItemText(m_iListSel, 0, iData.csTitle=dlg.m_csTitle);
	    	RefreshIconTitleSize();
			bChg=true;
		}
		if(dlg.m_csDesc.Compare(iData.csDesc)) {bChg=true; iData.csDesc=dlg.m_csDesc;}
		if(dlg.m_csPath.Compare(iData.csPath)) {bChg=true; iData.csPath=dlg.m_csPath;}
		if(bChg) EnableSave(m_bChanged=true);
	}
}

void CImageViewDlg::OnImgRemove()
{
	ASSERT(IsEditable());
	ASSERT(m_nImages==m_vIndex.size() && m_iListImages==m_nImages && m_nImages==m_ThumbList.GetItemCount());
	ASSERT(m_iListContext>=0 && m_iListContext==m_iListSel && m_iListContext<m_nImages);

	//m_vImageData[m_v_Index[i]] associates with m_ImageMap item m_vIndex[i]
	//where i is the control item we are deleting.

	//Interestingly, experimentation indicates we must manually shift
	//down any higher positions in the control. What a mess!!
	if(m_iListContext<m_nImages-1) {
		int xInc,y;
		m_ThumbList.GetItemSpacing(0,&xInc,&y);
		CPoint pt0;
		m_ThumbList.GetItemPosition(m_iListContext,&pt0);
		//m_ThumbList.GetItemPosition(m_iListContext+1,&pt1);
		//ASSERT(xInc==pt1.x-pt0.x);
		for(int i=m_iListContext+1;i<m_nImages;i++) {
			VERIFY(m_ThumbList.SetItemPosition(i,pt0));
			pt0.x+=xInc;
		}
	}

	//Now delete control item m_iListContext, realizing this alone wouldn't
	//have revised item display positions. Only indexes will shift.
	m_ThumbList.DeleteItem(m_iListContext);
	ASSERT(m_ThumbList.GetItemCount()==m_nImages-1);

	it_int it=m_vIndex.begin()+m_iListContext;
	int iDel=*it; //index of m_ImageData and m_ImageList item no longer needed
	m_vIndex.erase(it);
	ASSERT(m_vIndex.size()==m_nImages-1);
	for(it=m_vIndex.begin();it!=m_vIndex.end();it++) {
		if(*it>iDel) *it=*it-1;
	}

	//delete m_vImageData item (moving others down) 
	CImageData &iData=m_vImageData[iDel];
	delete iData.pLayer;
	iData.pLayer=NULL;
	m_vImageData.erase(m_vImageData.begin()+iDel);

	//Delete corresponding m_ImageList item (moving others down) 
	VERIFY(m_ImageList.Remove(iDel));
	ASSERT(m_ImageList.GetImageCount()==m_nImages-1);
	m_iListImages--;
	m_nImages--;

	RefreshIconTitleSize();

	m_iListSel=-1; //trigger m_PreviewPane update in OnItemChangedList()

	if(m_nImages) {
		SetCurSel(m_ThumbList,(m_iListContext<m_nImages)?m_iListContext:(m_iListContext-1));
	}
	else {
		m_PreviewPane.Invalidate();
		m_PreviewPane.UpdateWindow();
	}

	ASSERT(m_iListSel==GetCurSel(m_ThumbList));
	UpdateCounter();
	m_bChanged=(!m_bEmptyOnEntry || m_nImages>0);
	EnableSave(m_bChanged || m_nBadLinks);
}

void CImageViewDlg::OnRemoveall()
{
	ASSERT(IsEditable() && m_nImages);
	// confirm --

	if(m_nImages>1 && IDOK!=CMsgBox(MB_OKCANCEL,"Confirm that you want to clear the collection of all %u items.",m_nImages))
		return;

	TerminateThread();

	GetDlgItem(IDC_BUTTONLEFT)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTONRIGHT)->EnableWindow(FALSE);
	GetDlgItem(IDOK)->EnableWindow(FALSE);

	m_iListSel=-1;
	m_ThumbList.SetRedraw(FALSE);
	VERIFY(m_ThumbList.DeleteAllItems());
	VERIFY(m_ImageList.SetImageCount(0));
	m_ThumbList.SetRedraw(TRUE);
	m_ThumbList.UpdateWindow();
	m_PreviewPane.Invalidate();
	m_PreviewPane.UpdateWindow();

	free(m_memo.pData);
	m_memo.pData=_strdup("");
	m_bChanged=(!m_bEmptyOnEntry);
	EnableSave(m_bChanged || m_nBadLinks); //if from editor, bChg>0
	ClearImageData();
	m_iListImages=m_nImages=0;
}

void CImageViewDlg::OnImgInsert()
{
	ASSERT(IsEditable());
	if(CFileDropTarget::m_vFiles.size()) {
		ASSERT(0);
		CFileDropTarget::m_vFiles.clear();
	}

	char pathName[20*MAX_PATH];
	*pathName=0;
	if(!CWallsMapDoc::PromptForFileName(pathName,20*MAX_PATH,
		"Adding Image Files to Viewer",OFN_ALLOWMULTISELECT,FTYP_NONTL|FTYP_NOSHP)) return;

	LPCSTR p=pathName+strlen(pathName)+1;
	if(*p) {
		//multiple files selected --
		CString s;
		while(*p) {
			s.Format("%s\\%s",pathName,p);
			CFileDropTarget::m_vFiles.push_back(s);
			p+=strlen(p)+1;
		}
	}
	else CFileDropTarget::m_vFiles.push_back(CString(pathName));

	OnDrop();

	V_FILELIST().swap(CFileDropTarget::m_vFiles);

}

void CImageViewDlg::OnUpdateImgCollection(CCmdUI *pCmdUI)
{
	CTitleDlg dlg(m_SetTitle,"Collection Title (displayed right of button)");
    if(!IsEditable()) dlg.m_bRO=TRUE;
	if(IDOK==dlg.DoModal()) {
		if(IsEditable() && strcmp(dlg.m_title,m_SetTitle)) {
			m_SetTitle=dlg.m_title;
			EnableSave(m_bChanged=(!m_bEmptyOnEntry || m_nImages>0));
		}
	}
}

void CImageViewDlg::OnBnClickedSaveandexit()
{
	ASSERT(m_bEditable && (m_bChanged || m_nBadLinks));

#if 0 
	{ //probably unnec --
		CString title;
		if(app_pShowDlg && app_pShowDlg->IsRecOpen(m_pShp,m_memo.uRec))
			CMsgBox(MB_ICONINFORMATION,"%s\n\nNOTE: Since this record is currently open in the "
				"Selected Points dialog, you can still discard your changes via the Reset button in that dialog.",
				m_pShp->GetMemoTitle(title,m_memo));
	}
#endif

	if(!m_bChanged) {
		CString title;
		if(IDOK!=CMsgBox(MB_OKCANCEL,"%s\n\nCAUTION: You've made no changes, but this field still has %u broken link(s). "
			"Saving will remove all broken links and their properties. OK to proceed?",
			m_pShp->GetMemoTitle(title,m_memo),m_nBadLinks)) return;
	}

	if(m_pShp->SaveAllowed(1))
		SaveImageData();
	Destroy();
}

void CImageViewDlg::SetEditable(bool bEditable) {
	ASSERT(!m_bChanged);
	m_bEditable=bEditable;
	EnableSave(m_bEditable);
}

void CImageViewDlg::EnableSave(bool bEnable)
{
	GetDlgItem(IDC_SAVEANDEXIT)->EnableWindow(bEnable);
	GetDlgItem(IDOK)->EnableWindow(m_nImages>0);
}

void CImageViewDlg::OnEditPaste() 
{
	COleDataObject oData;

	if(!oData.AttachClipboard() || !oData.IsDataAvailable(CF_HDROP)) {
		ASSERT(0);
		return;
	}
	if(m_dropTarget.ReadHdropData(&oData)) {
		OnDrop();
		V_FILELIST().swap(CFileDropTarget::m_vFiles);
	}
	else
		CFileDropTarget::FileTypeErrorMsg();
}

void CImageViewDlg::OnOpenfolder()
{
	OpenContainingFolder((m_iListContext>=0)?
		m_vImageData[m_vIndex[m_iListContext]].csPath:m_pShp->PathName());
}

LRESULT CImageViewDlg::OnCommandHelp(WPARAM wNone, LPARAM lParam)
{
	AfxGetApp()->WinHelp(0);
	return TRUE;
}

void CImageViewDlg::OnBeginLabelEdit(NMHDR* pNMHDR, LRESULT* pResult)
{
	if(IsEditable()) {
		//LV_DISPINFO* pDispInfo = (LV_DISPINFO*)pNMHDR;
		//CString title(pDispInfo->item.pszText);
		//ASSERT(m_iListSel==pDispInfo->item.iItem);
		m_bOneLine=(m_iMaxDataHt<=m_iTextHeight);
		if(m_bOneLine) {
			ASSERT(m_iMaxDataHt==m_iTextHeight);
			m_iMaxDataHt=2*m_iTextHeight;
			AdjustControls();
		}
		m_bEditing=true;
		HWND hWnd=m_ThumbList.GetEditControl()->m_hWnd;
		ASSERT(hWnd!=NULL);
		VERIFY(m_LVEdit.SubclassWindow(hWnd));
		//CImageData &iData=m_vImageData[m_vIndex[m_iListSel]];
		//m_LVEdit.SetWindowTextA(iData.csTitle);
		*pResult = FALSE;
	}
	else *pResult = TRUE;
}

void CImageViewDlg::OnEndLabelEdit(NMHDR* pNMHDR, LRESULT* pResult)
{
	LV_DISPINFO* pDispInfo = (LV_DISPINFO*)pNMHDR;
	*pResult = 0;
	ASSERT(m_bEditing);
	if(pDispInfo->item.pszText && pDispInfo->item.iItem != -1) {
		*pResult=1;
		ASSERT(m_iListSel==pDispInfo->item.iItem);
		CString title(pDispInfo->item.pszText);
		CImageData &iData=m_vImageData[m_vIndex[m_iListSel]];
		if(title.Compare(iData.csTitle)) {
			m_bOneLine=false;
			FixIconTitle(title);
			strcpy(pDispInfo->item.pszText,title);
			m_ThumbList.SetItemText(m_iListSel, 0, iData.csTitle=title);
			//m_LVEdit.SetWindowTextA(NULL);
			RefreshIconTitleSize();
			EnableSave(m_bChanged=true);
		}
		VERIFY(m_LVEdit.UnsubclassWindow());
	}
	if(m_bOneLine) {
		m_iMaxDataHt=m_iTextHeight;
		AdjustControls();
	}
	m_bEditing=false;
}

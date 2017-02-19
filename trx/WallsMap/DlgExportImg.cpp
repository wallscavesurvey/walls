// DlgExportImg.cpp : implementation file
//

#include "stdafx.h"
#include "WallsMap.h"
#include "Mainfrm.h"
#include "MapLayer.h"
#include "GdalLayer.h"
#include "WallsMapDoc.h"
#include "WallsMapView.h"
#include "DibWrapper.h"
#include "DlgExportImg.h"
#include <png.h>
#include <io.h>
#include <errno.h>

// CDlgExportImg dialog

IMPLEMENT_DYNAMIC(CDlgExportImg, CDialog)

static BOOL bOpen=TRUE,bIncScale=TRUE;

CDlgExportImg::CDlgExportImg(CWallsMapView *pView,CWnd* pParent /*=NULL*/)
	: m_pView(pView)
	, CDialog(CDlgExportImg::IDD, pParent)
	, m_bOpenImage(bOpen)
	, m_nRatio(100)
{
	m_csPathname=GetDocument()->GetPathName();
	m_csPathname.Truncate(trx_Stpext(m_csPathname)-(LPCSTR)m_csPathname);
	m_csPathname+=".png";
	m_bIncScale=bIncScale && pView->GetDocument()->IsTransformed();

	CRect client;
	m_pView->GetClientRect(&client);
	m_uWidth=m_uClientW=client.Width();
	m_uClientH=client.Height();
}

CDlgExportImg::~CDlgExportImg()
{
}

void CDlgExportImg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_OPEN_IMAGE, m_bOpenImage);
	DDX_Check(pDX, IDC_INC_SCALE, m_bIncScale);
	DDX_Control(pDX, IDC_PATHNAME, m_cePathName);
	DDX_Text(pDX, IDC_PATHNAME, m_csPathname);
	DDV_MaxChars(pDX, m_csPathname, 260);
	DDX_Text(pDX, IDC_WIDTH, m_uWidth);
	DDX_Control(pDX, IDC_PROGRESS, m_Progress);
	DDX_Control(pDX, IDC_RATIO, m_sliderRatio);

	if(pDX->m_bSaveAndValidate) {

		pDX->m_idLastControl=IDC_WIDTH;
		if(m_uWidth<m_uClientW) {
			CMsgBox("The image width must be no less than the view width, which is %u pixels.",m_uClientW);
			goto _fail;
		}

		int height=(int)(((double)m_uClientH/m_uClientW)*m_uWidth);
		__int64 kp=(((__int64)m_uWidth*height)+1023)/1024;
		if(kp<0 || kp>(__int64)(100*1024)) {
			if(kp>(__int64)1024*200) {
				AfxMessageBox("Image files with more than 200 megapixels can't be exported. You'll need to reduce the width.");
				goto _fail;
			}
			if(IDYES!=CMsgBox(MB_ICONQUESTION|MB_YESNO,"Are you sure you want to create a file this large? Its size would be "
				"%u MB assuming 50%% compression.",(UINT)((3*kp+2047)/2048)))
				goto _fail;
		}

		m_csPathname.Trim();
		if(m_csPathname.IsEmpty()) {
			AfxMessageBox("A non-empty output file name is required.");
			goto _badPath;
		}

		LPCSTR p=trx_Stpext(m_csPathname);
		if(!*p)	{
			m_csPathname+=".png";
		}
		else if(_stricmp(p,".png")) {
			AfxMessageBox("Only the PNG format is supported.");
			goto _badPath;
		}

		if(!_access(m_csPathname,0)) {
			if(IDYES!=CMsgBox(MB_YESNO,"File %s already exists. Do you want to overwrite it?",trx_Stpnam(m_csPathname)))
				goto _badPath;
			if(_unlink(m_csPathname)) {
				AfxMessageBox("The file is currently open or protected.");
				goto _badPath;
			}
		}

		BeginWaitCursor();

		{
			CDIBWrapper dib;
			if(!dib.Init(m_uWidth,height))  {
				goto _memFail;
			}

			dib.Fill(GetDocument()->m_hbrBack);

			CFltRect geoExtDIB;
			m_pView->GetViewExtent(geoExtDIB);

			double imgScale=m_uWidth/geoExtDIB.Width(); //DIB pixels/geo-units
			//needed to scale symbology -- varies from 1 to imgScale/m_pView->m_fScale:
			CLayerSet::m_fExportScaleRatio=((100-m_nRatio)+ m_nRatio*(imgScale/m_pView->m_fScale))*0.01;
			//m_uScaleDenom -- used for determining layer and label visibility,
			CLayerSet::m_uScaleDenom=GetImageScaleDenom();

			//if(!GetDocument()->LayerSet().CopyToImageDIB(&dib,geoExtDIB,m_uWidth/geoExtDIB.Width()))
			if(!GetDocument()->LayerSet().CopyToImageDIB(&dib,geoExtDIB))
				goto _memFail;

			if(m_bIncScale)
				CScaleWnd::CopyScaleBar(dib,m_pView->MetersPerPixel(imgScale),
				imgScale/m_pView->m_fScale,
				GetDocument()->IsFeetUnits());

			CreatePng(dib);
		}
		EndWaitCursor();
		bOpen=m_bOpenImage;
		bIncScale=m_bIncScale;
		return;

_memFail:
		EndWaitCursor();
		AfxMessageBox("Memory insufficient for bitmap creation. Either reduce width or change view extent!");
		goto _fail;

_badPath:
		pDX->m_idLastControl=IDC_PATHNAME;
_fail:
		pDX->m_bEditLastControl=TRUE;
		pDX->Fail();
		return;
	}
}

BEGIN_MESSAGE_MAP(CDlgExportImg, CDialog)
	ON_BN_CLICKED(IDBROWSE, &CDlgExportImg::OnBnClickedBrowse)
	ON_EN_CHANGE(IDC_WIDTH,OnEnChangeWidth)
    ON_MESSAGE(WM_COMMANDHELP,OnCommandHelp)
END_MESSAGE_MAP()

// CDlgExportImg message handlers

void CDlgExportImg::OnBnClickedBrowse()
{
	CString strFilter;
	if(!AddFilter(strFilter,IDS_PNG_FILES)) return;
	if(!DoPromptPathName(m_csPathname,OFN_HIDEREADONLY,
		1,strFilter,FALSE,IDS_PNG_SAVEAS,".png")) return;
	SetText(IDC_PATHNAME,m_csPathname);
}

void CDlgExportImg::InitWidth()
{
	UINT h=(UINT)(((double)m_uClientH/m_uClientW)*m_uWidth);
	CString s;
	s.Format("%u",h);
	SetText(IDC_HEIGHT,s);
	__int64 kp=((__int64)m_uWidth*h+1023)/1024;
	if(kp<(__int64)1024) {
		s.Format("%u KP",(UINT)kp);
	}
	else if(kp<(__int64)1024*1024) {
		s.Format("%.1f MP",(double)kp/1024);
	}
	else {
		s.Format("%.1f GP",(double)kp/(1024*1024));
	}
	SetText(IDC_KBSIZE,s);
}

void CDlgExportImg::UpdateRatioLabel()
{
	CString s;
	s.Format("%3d %%",m_nRatio);
	GetDlgItem(IDC_ST_RATIO)->SetWindowText(s);
}

UINT CDlgExportImg::GetImageScaleDenom()
{
	double scrnScale=m_pView->m_fScale;
	CFltRect geoExt;
	m_pView->GetViewExtent(geoExt);

	if(m_uWidth>(int)m_uClientW) {

		double imgScale=m_uWidth/geoExt.Width(); //image pixels per geounit
		scrnScale=imgScale/(((100-m_nRatio)+m_nRatio*(imgScale/scrnScale))*0.01);
		GetDlgItem(IDC_RATIO)->EnableWindow(TRUE);
		GetDlgItem(IDC_ST_RATIO)->EnableWindow(TRUE);

		//double fExportScaleRatio=((100-m_nRatio)+m_nRatio*(imgScale/scrnScale))*0.01;
		//When rendered, <size of symbol in geounits> = (m_fExportScaleRatio * symSize)/imgScale.
		//must have <size of symbol in screen pixels> = <size of symbol in geounits) * scrnScale == symSize, or
		//scrnScale == symSize / ((m_fExportScaleRatio * symSize)/imgScale) = imgScale / m_fExportScaleRatio
	}
	return CLayerSet::GetScaleDenom(geoExt,scrnScale,m_pView->GetDocument()->IsProjected());
}

void CDlgExportImg::UpdateDenomLabel()
{
	CString s;
	s.Format("1 : %u",GetImageScaleDenom());
	GetDlgItem(IDC_ST_DENOM)->SetWindowText(s);
	if(m_uWidth<=(int)m_uClientW) {
		GetDlgItem(IDC_RATIO)->EnableWindow(FALSE);
		GetDlgItem(IDC_ST_RATIO)->EnableWindow(FALSE);
	}
}

BOOL CDlgExportImg::OnInitDialog()
{
	CDialog::OnInitDialog();

	{
		CString s;
		s.Format("Symbol Size Ratio for Image Widths > %u",m_uClientW);
		GetDlgItem(IDC_ST_RATIO_BOX)->SetWindowText(s);

		if(!m_pView->GetDocument()->IsTransformed())
			GetDlgItem(IDC_INC_SCALE)->EnableWindow(0);

		CFltRect geoExt;
		m_pView->GetViewExtent(geoExt);
		UINT w=GetDocument()->LayerSet().MaxPixelWidth(geoExt);
		if(w) {
			s.Format("NOTE: A width of %u would be needed to preserve detail in the view's highest resolution image.",w);
			SetText(IDC_ST_FULLRES,s);
		}
	}

	InitWidth();
	m_sliderRatio.SetPos(m_nRatio);
	UpdateRatioLabel();
	UpdateDenomLabel();

	return TRUE;  // return TRUE unless you set the focus to a control
}

BOOL CDlgExportImg::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	// Slider Control
	if(wParam == IDC_RATIO)
	{
		int i=m_sliderRatio.GetPos();
		if(i!=m_nRatio) {
			m_nRatio=i;
			UpdateRatioLabel();
			UpdateDenomLabel();
		}
	}
	return CDialog::OnNotify(wParam, lParam, pResult);
}

void CDlgExportImg::OnEnChangeWidth()
{
	CString s;
	GetDlgItem(IDC_WIDTH)->GetWindowTextA(s);
	UINT i=atoi(s);
	if(m_uWidth!=i) {
		m_uWidth=i;
		InitWidth();
		UpdateDenomLabel();
	}
}

void CDlgExportImg::CreatePng(const CDIBWrapper &dib)
{
    FILE *fp = fopen(m_csPathname,"wb");
	if(!fp)
		goto _fail;

    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL,NULL,NULL);

	if (!png_ptr)
		goto _fail;

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr)
	   goto _fail0;

    if(setjmp(png_jmpbuf(png_ptr)))
		goto _fail0;

	png_init_io(png_ptr,fp);

	//png_set_write_status_fn(png_ptr, (png_write_status_ptr)write_row_callback);

	UINT w=dib.GetWidth();
	UINT h=dib.GetHeight();

	png_set_IHDR(png_ptr, info_ptr, w, h, 8, PNG_COLOR_TYPE_RGB, 
       PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

	png_write_info(png_ptr, info_ptr);

	png_set_bgr(png_ptr);

	UINT wi=dib.GetImageWidth();
	LPBYTE pData=(LPBYTE)dib.GetDIBits()+h*wi;

	for(UINT r=0;r<h;r++) {
		pData-=wi;
		png_write_row(png_ptr,pData);
		m_Progress.SetPos(((r+1)*100)/h);
	}

	//If you are interested in writing comments or time, you should pass an appropriately filled png_info pointer.
	//f you are not interested, you can pass NULL.
    png_write_end(png_ptr,NULL);

	//When you are done, you can free all memory used by libpng like this:
    png_destroy_write_struct(&png_ptr, &info_ptr);

	fclose(fp);

	if(m_pView->GetDocument()->IsTransformed()) {
		try {
			CString s(m_csPathname);
			s.Truncate(s.GetLength()-3); s+="pgw";
			CFile file(s,CFile::modeCreate|CFile::modeWrite);
			CFltRect geoExt;
			m_pView->GetViewExtent(geoExt);
			double fw=geoExt.Width()/w;
			s.Format("%.12f\r\n0.0\r\n0.0\r\n%.12f\r\n%.12f\r\n%.12f\r\n",
				fw,-fw,geoExt.l+fw/2,geoExt.t-fw/2);
			file.Write((LPCSTR)s,s.GetLength());
			file.Close();
		}
		catch(...) {
			return;
		}
	}

	if(m_bOpenImage) {
		CWallsMapApp::m_bNoAddRecent=TRUE;
		CLayerSet::m_pLsetImgSource=&m_pView->GetDocument()->LayerSet();
		VERIFY(theApp.OpenDocumentFile(m_csPathname));
		CLayerSet::m_pLsetImgSource=NULL;
		CWallsMapApp::m_bNoAddRecent=FALSE;
	}

	return;

_fail0:
	png_destroy_write_struct(&png_ptr, (png_infopp)NULL);

_fail:
	if(fp) {
		fclose(fp);
		_unlink(m_csPathname);
	}
	AfxMessageBox("File creation failure.");
}

LRESULT CDlgExportImg::OnCommandHelp(WPARAM wNone, LPARAM lParam)
{
	AfxGetApp()->WinHelp(0);
	return TRUE;
}

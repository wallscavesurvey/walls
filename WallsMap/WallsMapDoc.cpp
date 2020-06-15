// WallsMapDoc.cpp : implementation of the CWallsMapDoc class
//

#include "stdafx.h"
#include "filecfg.h"
#include "MainFrm.h"
#include "WallsMap.h"
#include "WallsMapDoc.h"
#include "WallsMapView.h"
#include "MapLayer.h"
#include "NtiLayer.h"
#include "ShpLayer.h"
#include "ExportNti.h"
#include "ExportNTI_Adv.h"
#include "showshapedlg.h"
#include "dbteditdlg.h"
#include "imageviewdlg.h"
#include "Layersetsheet.h"
#include "DlgSymbolsImg.h"
#include "ZipDlg.h"
#include "TabCombo.h"
#include "MsgCheck.h"
#include "GPSPortSettingDlg.h"
#include "FileIsInUse.h"
#include "QuadTree.h"

#undef _USE_XFILEDLG
#ifdef _USE_XFILEDLG
#include "XFileDialog.h"
#endif

#include <direct.h>
#include <io.h>
#include ".\wallsmapdoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CWallsMapDoc

IMPLEMENT_DYNCREATE(CWallsMapDoc, CDocument)

BEGIN_MESSAGE_MAP(CWallsMapDoc, CDocument)
	ON_COMMAND(ID_FILE_PROPERTIES, OnFileProperties)
	ON_UPDATE_COMMAND_UI(ID_FILE_PROPERTIES, OnUpdateFileProperties)
	ON_COMMAND(ID_FILE_SAVE_AS, OnFileSaveAs)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVE_AS, OnUpdateFileSaveAs)
	ON_COMMAND(ID_FILE_SAVE, OnFileSave)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVE, OnUpdateFileSave)
	ON_COMMAND(ID_FILE_TRUNCATE, OnFileTruncate)
	ON_UPDATE_COMMAND_UI(ID_FILE_TRUNCATE, OnUpdateFileTruncate)
	ON_COMMAND(ID_FILE_CONVERT, OnFileConvert)
	ON_UPDATE_COMMAND_UI(ID_FILE_CONVERT, OnUpdateFileConvert)
	ON_COMMAND(ID_TOOLSELECT, OnToolSelect)
	ON_UPDATE_COMMAND_UI(ID_TOOLSELECT, OnUpdateToolSelect)
	ON_COMMAND(ID_SELECT_ADD, OnSelectAdd)
	ON_UPDATE_COMMAND_UI(ID_SELECT_ADD, OnUpdateSelectAdd)

	ON_COMMAND(ID_VIEW_DEFAULT, OnViewDefault)
	ON_COMMAND(ID_VIEW_ZOOMIN, OnViewZoomin)
	ON_COMMAND(ID_VIEW_ZOOMOUT, OnViewZoomout)
	ON_COMMAND(ID_VIEW_PAN, OnViewPan)
	ON_COMMAND(ID_TOOLMEASURE, OnToolMeasure)

	ON_UPDATE_COMMAND_UI(ID_VIEW_DEFAULT, OnUpdateViewPan)
	ON_UPDATE_COMMAND_UI(ID_VIEW_ZOOMIN, OnUpdateViewPan)
	ON_UPDATE_COMMAND_UI(ID_VIEW_ZOOMOUT, OnUpdateViewPan)
	ON_UPDATE_COMMAND_UI(ID_VIEW_PAN, OnUpdateViewPan)
	ON_UPDATE_COMMAND_UI(ID_TOOLMEASURE, OnUpdateViewPan)

	ON_COMMAND(ID_EXPORT_ZIP, OnExportZip)
END_MESSAGE_MAP()

// CWallsMapDoc construction/destruction

UINT CWallsMapDoc::m_nDocs = 0;
BOOL CWallsMapDoc::m_bOpenAsEditable = FALSE;
bool CWallsMapDoc::m_bSelectionLimited = false;

CString CWallsMapDoc::m_csLastPath;

LPSTR CWallsMapDoc::ntltypes[NTL_CFG_NUMTYPES] = {
   "CRS",
   "FOLDER",
   "LAYER",
   "LEND",
   "LFONT",
   "LLBLFLD",
   "LMSTYLE",
   "LNAME",
   "LOPACITY",
   "LPATH",
   "LQTDEPTH",
   "LSCALES",
   "LSRCHFLD",
   "LSTATUS",
   "SLAYERS",
   "STRFIND",
   "VBKRGB",
   "VFLAGS",
   "VVIEW"
};

CWallsMapDoc::CWallsMapDoc()
{
	m_pFileNTL = NULL;
	m_bExtensionNTL = 0;
	m_bDrawOutlines = FALSE;
	m_bSelecting = 0;
	m_uActionFlags = 0;
	m_bMarkSelected = TRUE;
	m_uEnableFlags = NTL_FLG_MARKERS;
	m_bReadOnly = m_bShowAltNad = m_bShowAltGeo = false;
	m_clrBack = m_clrPrev = RGB(0, 0, 0);
	m_hbrBack = (HBRUSH)GetStockObject(BLACK_BRUSH);
	m_layerset.m_pDoc = this;
	m_pRestorePosition = NULL;
	m_phist_find = NULL;
	m_bHistoryUpdated = m_bLayersUpdated = false;
	m_pfiu = NULL;
}

void CWallsMapDoc::Register_fiu(LPCSTR pPathName)
{
	DWORD dwCookie = 0;
	ASSERT(!m_pfiu);
	if (S_OK != CFileIsInUseImpl::s_CreateInstance(AfxGetMainWnd()->m_hWnd, pPathName, FUT_EDITING, OF_CAP_CANSWITCHTO, IID_PPV_ARGS(&m_pfiu), &dwCookie)) {
		ASSERT(!m_pfiu);
		ASSERT(0);
	}
	else m_dwCookie = dwCookie;
}

void CWallsMapDoc::Release_fiu()
{
	IRunningObjectTable *prot;
	HRESULT hr = GetRunningObjectTable(NULL, &prot);
	if (SUCCEEDED(hr))
	{
		hr = prot->Revoke(m_dwCookie);
		prot->Release();
	}
	m_pfiu->Release();
	m_pfiu = NULL;
}

CWallsMapDoc::~CWallsMapDoc()
{
	if (m_pfiu) Release_fiu();

	if (m_pFileNTL) {
		if (m_pFileNTL->IsOpen()) {
			m_pFileNTL->Close();
		}
		delete m_pFileNTL;
	}

	delete m_phist_find;

	if (app_pDbtEditDlg && app_pDbtEditDlg->m_pShp->m_pDoc == this) {
		app_pDbtEditDlg->DenyClose(TRUE); //Force save or discard
		app_pDbtEditDlg->Destroy();
	}
	if (app_pImageDlg && app_pImageDlg->m_pShp->m_pDoc == this) {
		app_pImageDlg->DenyClose(TRUE); //Force save or discard
		app_pImageDlg->Destroy();
	}
	if (app_pShowDlg && app_pShowDlg->m_pDoc == this) {
		app_pShowDlg->Destroy();
		//m_vec_shprec.clear(); //unnecesary
	}
	::DeleteObject(m_hbrBack);
	delete m_pRestorePosition;
}

//////////////////////////////////////////////////////////////////////////////
//Utilities related to file types --
WallsDocType CWallsMapDoc::doctypes[MAX_WALLS_FORMAT] =
{
	{ WALLS_FORMAT_ALL, TRUE, FALSE, "All supported formats",
		"*.bmp;*.gif;*.jpg;*.jpeg;*.png;*.tif;*.tiff;*.pcx;*.j2k;*.jp2;*.jpc;*.sid;*.ecw;*.ppm;*.pgm;*.webp;*.nti;*.shp;*.ntl"}
	,{ WALLS_FORMAT_NTL, TRUE, TRUE,  "WallsMap layer files (.ntl)", "*.ntl" }
	,{ WALLS_FORMAT_NTI, TRUE, TRUE,  "WallsMap image files (.nti)", "*.nti" }
	,{ WALLS_FORMAT_BMP, TRUE, FALSE, "BMP files", "*.bmp" }
	,{ WALLS_FORMAT_ECW, TRUE, FALSE, "ECW files", "*.ecw" }
	,{ WALLS_FORMAT_GIF, TRUE, FALSE, "GIF files", "*.gif" }
	,{ WALLS_FORMAT_JPG, TRUE, FALSE, "JPEG files", "*.jpg;*.jpeg" }
	,{ WALLS_FORMAT_J2K, TRUE, FALSE, "JPEG2000 files (.j2k,.jp2,.jpc)", "*.j2k;*.jp2;*.jpc" }
	,{ WALLS_FORMAT_PNG, TRUE, FALSE, "PNG files", "*.png" }
	,{ WALLS_FORMAT_PPM, TRUE, FALSE, "PPM and PGM files", "*.ppm;*.pgm" }
	,{ WALLS_FORMAT_SHP, TRUE, TRUE,  "ESRI shape files (.shp)", "*.shp" }
	,{ WALLS_FORMAT_SID, TRUE, FALSE, "MrSID files (.sid)", "*.sid" }
	,{ WALLS_FORMAT_TIF, TRUE, FALSE, "TIFF files (.tif,.tiff)", "*.tif;*.tiff" }
	,{ WALLS_FORMAT_WEBP, TRUE, FALSE, "WebP files", "*.webp" }
	//,{ WALLS_FORMAT_NTF, TRUE, FALSE, "NITF files (.ntf,.nsf)","*.ntf;*.nsf" }
};

//Currently used when reading only -- 
CString CWallsMapDoc::GetFileTypes(UINT fTyp)
{
	CString str;
	if (fTyp&FTYP_ALLTYPES) {
		str += "All files";
		str += (TCHAR)NULL;
		str += "*.*";
		str += (TCHAR)NULL;
	}
	else {
		for (int i = 0; i < MAX_WALLS_FORMAT; i++) {
			if ((fTyp&FTYP_NONTL) && i == WALLS_FORMAT_NTL ||
				(fTyp&FTYP_NOSHP) && i == WALLS_FORMAT_SHP
				) continue;
			str += doctypes[i].description;
			str += (TCHAR)NULL;
			str += doctypes[i].ext;
			if (!i && fTyp) {
				str.Truncate(str.GetLength() - ((fTyp&FTYP_NOSHP) ? 12 : 6));
			}
			str += (TCHAR)NULL;
		}
	}
	return str;
}

//Currently used when reading only -- 
BOOL CWallsMapDoc::PromptForFileName(char *pFileName, UINT maxFileNameSiz, LPCSTR pTitle, DWORD dwFlags, UINT fTyp)
{

#ifdef _USE_XFILEDLG
	CFileDialog *pDlg;

	if (fTyp&FTYP_NOSHP) pDlg = new CFileDialog(TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_ENABLESIZING | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_EXPLORER | OFN_LONGNAMES | dwFlags);
	else pDlg = new CXFileDialog(TRUE, NULL, NULL, OFN_ENABLESIZING | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_EXPLORER | OFN_LONGNAMES | dwFlags);

	CString strFilter = GetFileTypes(fTyp);
	pDlg->m_ofn.lpstrFilter = strFilter;
	pDlg->m_ofn.nFilterIndex = 0; //start with all supported files
	pDlg->m_ofn.lpstrDefExt = NULL;
	pDlg->m_ofn.lpstrTitle = pTitle;
	pDlg->m_ofn.lpstrFile = pFileName;
	pDlg->m_ofn.lpstrInitialDir = (LPCSTR)m_csLastPath;
	pDlg->m_ofn.nMaxFile = maxFileNameSiz;
	if (!(fTyp&FTYP_NOSHP) && CMainFrame::IsPref(PRF_OPEN_EDITABLE)) {
		((CXFileDialog *)pDlg)->m_bEnableEdit = true;
	}

	BOOL bRet = (pDlg->DoModal() == IDOK);

	if (bRet) {
		if (!(fTyp&FTYP_NOSHP) && ((CXFileDialog *)pDlg)->m_bEnableEdit) bRet++;
		SetLastPath(pFileName);
	}
	delete pDlg;
	return bRet;
#else
	//CFileDialog *pDlg;

	//if(fTyp&FTYP_NOSHP) 
		//pDlg=new CFileDialog(TRUE,NULL,NULL,OFN_HIDEREADONLY | OFN_ENABLESIZING | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_EXPLORER | OFN_LONGNAMES | dwFlags);
	//else pDlg=new CXFileDialog(TRUE,NULL,NULL,OFN_ENABLESIZING | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_EXPLORER | OFN_LONGNAMES | dwFlags);

	CFileDialog dlg(TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_ENABLESIZING | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_EXPLORER | OFN_LONGNAMES | dwFlags);
	CString strFilter = GetFileTypes(fTyp);
	dlg.m_ofn.lpstrFilter = strFilter;
	dlg.m_ofn.nFilterIndex = 0; //start with all supported files
	dlg.m_ofn.lpstrDefExt = NULL;
	dlg.m_ofn.lpstrTitle = pTitle;
	dlg.m_ofn.lpstrFile = pFileName;
	dlg.m_ofn.lpstrInitialDir = (LPCSTR)m_csLastPath;
	dlg.m_ofn.nMaxFile = maxFileNameSiz;
	/*
	if(!(fTyp&FTYP_NOSHP) && CMainFrame::IsPref(PRF_OPEN_EDITABLE)) {
		((CXFileDialog *)pDlg)->m_bEnableEdit = true;
	}
	*/

	BOOL bRet = (dlg.DoModal() == IDOK);

	//if(bRet && !(fTyp&FTYP_NOSHP) && ((CXFileDialog *)pDlg)->m_bEnableEdit) bRet++;
	if (bRet) {
		if (!(fTyp&FTYP_NOSHP) && CMainFrame::IsPref(PRF_OPEN_EDITABLE)) bRet++;
		SetLastPath(pFileName);
	}
	//delete pDlg;
	return bRet;
#endif
}

//////////////////////////////////////////////////////////////////////////////

BOOL CWallsMapDoc::OnNewDocument()
{
	// if (!CDocument::OnNewDocument()) return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return FALSE;
}

BOOL CWallsMapDoc::OnOpenDocument(LPCSTR lpszPathName)
{
	BOOL bRet;

	if (!_stricmp(trx_Stpext(lpszPathName), NTL_EXT)) {
		m_bExtensionNTL = 2;
		bRet = OpenLayerSet(lpszPathName);
		if (bRet) Register_fiu(lpszPathName);
	}
	else {
		m_layerset.CreateImageList();
		if (CLayerSet::m_pLsetImgSource) {
			m_uEnableFlags |= (CLayerSet::m_pLsetImgSource->m_pDoc->m_uEnableFlags&NTL_FLG_FEETUNITS); //for scale bar
		}

		bRet = OpenLayer(lpszPathName, trx_Stpnam(lpszPathName),
			(m_bOpenAsEditable ? (NTL_FLG_VISIBLE | NTL_FLG_EDITABLE) : NTL_FLG_VISIBLE));
	}
	if (bRet) {
		//m_layerset.ComputeExtent();
		GetPropStatus().nTab = IsTransformed() ? 0 : 1;
	}
	if (IsTransformed()) {
		CWallsMapView::SyncStop();
	}
	if (bRet && m_csLastPath.IsEmpty())
		SetLastPath(lpszPathName);

	return bRet;
}

static UINT getLabelField(CString &csLabel, LPCSTR pArg)
{
	int iRet = 1;
	for (LPCSTR p = pArg; *p; p++) {
		if (!isdigit(*p)) {
			iRet = 0;
			break;
		}
	}
	if (iRet && ((iRet = atoi(pArg)) <= 0 || iRet > 128)) iRet = 0;
	if (!iRet) csLabel = pArg;
	return iRet;
}

BOOL CWallsMapDoc::OpenLayerSet(LPCTSTR lpszPathName)
{
	ASSERT(!m_pFileNTL);
	m_pFileNTL = new CFileCfg(ntltypes, NTL_CFG_NUMTYPES);

	CFileException e;

	if (!m_pFileNTL->Open(lpszPathName, CFile::modeReadWrite | CFile::shareExclusive, &e)) {
		if (!m_pFileNTL->BufferSize()) return FALSE;
		//Either read-only or protected --
		if (e.m_cause != CFileException::fileNotFound) {
			if (!m_pFileNTL->Open(lpszPathName, CFile::modeRead | CFile::shareExclusive)) {
				if (m_pFileNTL->BufferSize())
					CMsgBox("File: %s\nThis file is protected by another application.", lpszPathName);
				return 0;
			}
			m_bReadOnly = true;
		}
		else {
			CMsgBox("File \"%s\" no longer exists!", lpszPathName);
			return 0;
		}
	}

	m_layerset.CreateImageList();

	char ftitle[_MAX_FNAME];
	char fname[_MAX_FNAME];
	char fpath[_MAX_PATH];
	UINT uFlags = NTL_FLG_VISIBLE;
	UINT vFlags = 0;
	UINT nLblFld = 1;
	UINT uDepth = 0, uQTnodes = 0, nImage = 0;
	std::vector<CString> vcsSrchFld;
	CString csLabel;
	COLORREF crLbl = RGB(255, 255, 255), crTransColor = 0;
	BYTE bQTdepth = 0, bQTusing = 0;
	BYTE bAlpha = 0xff;
	bool bUseTransColor = false, bSrchFlds = false;
	CScaleRange scaleRange;

	SHP_MRK_STYLE mstyle, mstyleDef;
	CLogFont lf, lfDef;

	CNtiLayer::m_bExpanding = FALSE;
	CShpLayer::m_bShpReeditMsg = CLayerSet::m_bReadWriteConflict = false;
	CLayerSet::m_bOpenConflict = 0;

	m_layerset.m_iNad = -1;

	bool bSkipMissing = false;

	*fname = *fpath = *ftitle = 0;
	COLORREF crBack = RGB(0, 0, 0);
	int typ;
	bool bFolder = false, bInLayer = false;
#ifdef _DEBUG
	int layercnt = 0;
#endif

	//read layers in order of rendering, bottom of displayed tree to top, or from beginning of layers vector --

	while ((typ = m_pFileNTL->GetLine()) >= 0 || typ == CFG_ERR_KEYWORD) {
		switch (typ) {

		case NTL_CFG_SLAYERS:
			ASSERT(vFlags&NTL_FLG_SAVELAYERS); //width,height,wcol1,wcol2,wcol3
			if (m_pFileNTL->Argc() > 1) {
				int nargs = m_pFileNTL->GetArgv(m_pFileNTL->Argv(1));
				if (nargs >= 5) {
					SetLayersWindowSize(atoi(m_pFileNTL->Argv(0)), atoi(m_pFileNTL->Argv(1)));
					for (int i = 0; i < 3; i++)
						m_layerset.m_PropStatus.nColWidth[i] = atoi(m_pFileNTL->Argv(i + 2));
				}
			}
			break;

		case NTL_CFG_STRFIND:
			ASSERT(vFlags&NTL_FLG_SAVEHISTORY);
			if (m_pFileNTL->Argc() > 1) {
				LPCSTR p = m_pFileNTL->Argv(1); //string may leading/trailing spaces, quotes, etc.
				WORD w = (WORD)atoi(p); //starts with a flags integer followed by a space
				//following the space, string may have leading/trailing spaces, quotes, etc.
				while (*p && *p != ' ') p++;
				if (*p && *++p) {
					if (!m_phist_find)
						m_phist_find = new CTabComboHist(MAX_COMBOHISTITEMS);
					if (m_phist_find->m_nItems < m_phist_find->m_nMaxItems)
						m_phist_find->InsertAsLast(p, w);
				}
			}
			break;

		case NTL_CFG_FOLDER:
			ASSERT(!bInLayer);
			bInLayer = bFolder = true;
			*ftitle = 0;
			uDepth = 0;
			nImage = 1; //open initially
			uFlags = NTL_FLG_VISIBLE; //default status
			if (m_pFileNTL->Argc() > 1)
				trx_Stncc(ftitle, m_pFileNTL->Argv(1), _MAX_FNAME);
			break;

		case NTL_CFG_LAYER:
			ASSERT(!bInLayer && !bFolder);
			bInLayer = true;
			bFolder = false;
			uFlags = NTL_FLG_VISIBLE; //default status
			*fname = *fpath = *ftitle = 0;
			uDepth = 0; //to be determined
			nLblFld = 1;
			bSrchFlds = false;
			scaleRange.Clear();
			vcsSrchFld.clear();
			csLabel.Empty();
			crLbl = RGB(255, 255, 255);
			crTransColor = 0;
			bUseTransColor = false;
			bAlpha = 0xff;
			bQTdepth = bQTusing = 0;
			uQTnodes = 0;
			mstyle = mstyleDef;
			lf = lfDef;
			if (m_pFileNTL->Argc() > 1)
				trx_Stncc(ftitle, m_pFileNTL->Argv(1), _MAX_FNAME);
			break;

		case NTL_CFG_LSTATUS:
			ASSERT(bInLayer);
			if (m_pFileNTL->Argc() > 1) {
				int nargs = m_pFileNTL->GetArgv(m_pFileNTL->Argv(1));
				uFlags = atoi(m_pFileNTL->Argv(0));
				if (nargs > 1)
					uDepth = atoi(m_pFileNTL->Argv(1));
				if (bFolder) {
					ASSERT(nargs == 3);
					if (nargs > 2) nImage = atoi(m_pFileNTL->Argv(2));
				}
				else {
					if (m_bReadOnly) uFlags &= ~NTL_FLG_EDITABLE;
				}
			}
			break;

		case NTL_CFG_VFLAGS:
			if (m_pFileNTL->Argc() > 1) vFlags = (atoi(m_pFileNTL->Argv(1))&NTL_FLG_ALLSAVED);
			break;

		case NTL_CFG_VBKRGB:
			if (m_pFileNTL->Argc() > 1 && m_pFileNTL->GetArgv(m_pFileNTL->Argv(1)) >= 3) {
				crBack = RGB(atoi(m_pFileNTL->Argv(0)), atoi(m_pFileNTL->Argv(1)), atoi(m_pFileNTL->Argv(2)));
			}
			break;

		case NTL_CFG_VVIEW:
			ASSERT(vFlags&NTL_FLG_RESTOREVIEW);
			if (m_pFileNTL->Argc() > 1 && m_pFileNTL->GetArgv(m_pFileNTL->Argv(1)) == 3) {
				CRestorePosition *pRP = GetRestorePosition();
				if (pRP) {
					ASSERT(!pRP->uStatus);
					pRP->geoHeight = atof(m_pFileNTL->Argv(0));
					pRP->ptGeo.x = atof(m_pFileNTL->Argv(1));
					pRP->ptGeo.y = atof(m_pFileNTL->Argv(2));
					pRP->uStatus = 1;
				}
			}
			break;

		case NTL_CFG_LMSTYLE:
		{
			if (m_pFileNTL->Argc() > 1) {
				CString s(m_pFileNTL->Argv(1));
				mstyle.ReadProfileStr(s);
			}
			break;
		}

		case NTL_CFG_LFONT:
			if (m_pFileNTL->Argc() > 1) lf.GetFontFromStr(m_pFileNTL->Argv(1));
			break;

		case NTL_CFG_LLBLFLD:
			if (m_pFileNTL->Argc() > 1 && m_pFileNTL->GetArgv(m_pFileNTL->Argv(1)) >= 2) {
				nLblFld = getLabelField(csLabel, m_pFileNTL->Argv(0));
				crLbl = atoi(m_pFileNTL->Argv(1));
			}
			break;

		case NTL_CFG_LOPACITY:
			if (m_pFileNTL->Argc() > 1 && m_pFileNTL->GetArgv(m_pFileNTL->Argv(1)) >= 3) {
				bAlpha = (BYTE)atoi(m_pFileNTL->Argv(0));
				bUseTransColor = atoi(m_pFileNTL->Argv(1)) != 0;
				crTransColor = atoi(m_pFileNTL->Argv(2));
			}
			break;

		case NTL_CFG_LSCALES:
			if (m_pFileNTL->Argc() > 1 && m_pFileNTL->GetArgv(m_pFileNTL->Argv(1)) >= 6) {
				scaleRange.bLayerNoShow = (short)atoi(m_pFileNTL->Argv(0));
				scaleRange.uLayerZoomIn = atoi(m_pFileNTL->Argv(1));
				scaleRange.uLayerZoomOut = atoi(m_pFileNTL->Argv(2));
				scaleRange.bLabelNoShow = (short)atoi(m_pFileNTL->Argv(3));
				scaleRange.uLabelZoomIn = atoi(m_pFileNTL->Argv(4));
				scaleRange.uLabelZoomOut = atoi(m_pFileNTL->Argv(5));
			}
			break;

		case NTL_CFG_LSRCHFLD:
			bSrchFlds = true;
			if (m_pFileNTL->Argc() > 1 && m_pFileNTL->GetArgv(m_pFileNTL->Argv(1)) >= 1) {
				for (int i = 0; i < m_pFileNTL->Argc(); i++) {
					vcsSrchFld.push_back(m_pFileNTL->Argv(i));
				}
			}
			break;

		case NTL_CFG_LQTDEPTH:
			if (m_pFileNTL->Argc() > 1) {
				if (m_pFileNTL->GetArgv(m_pFileNTL->Argv(1)) > 2) {
					bQTdepth = (BYTE)atoi(m_pFileNTL->Argv(0));
					bQTusing = (BYTE)atoi(m_pFileNTL->Argv(1));
					uQTnodes = atoi(m_pFileNTL->Argv(2));
				}
			}
			break;

		case NTL_CFG_LEND:
			ASSERT(bInLayer);
			bInLayer = false;
			if (bFolder) {
				bFolder = false;
				PTL ptl = new CTreeLayer(ftitle);
				m_layerset.PushBackPTL(ptl);
#ifdef _DEBUG
				layercnt++;
#endif
				ptl->m_uFlags = uFlags;
				ptl->m_uLevel = uDepth;
				ptl->m_nImage = nImage;

				//Fix links of immediate children, they have m_uLevel==ptl->m_uLevel+1 and parent==0
				PTL pLast = 0;
				rit_Layer it = BeginLayerRit();
				ASSERT(*it == ptl);
				while (++it != EndLayerRit()) {
					PTL pChild = *it;
					if (!pChild->parent && pChild->m_uLevel == uDepth + 1) {
						ASSERT(pChild->nextsib == 0 && pChild->prevsib == 0);
						if (!pLast) {
							ptl->child = pChild;

						}
						else {
							pLast->nextsib = pChild;
							pChild->prevsib = pLast;
						}
						pChild->parent = ptl;
						pLast = pChild;
					}
				}
				if (!ptl->HasVisibleChildren()) {
					if (ptl->IsVisible()) ptl->m_uFlags |= NTL_FLG_TRISTATE;
					ptl->m_uFlags &= ~NTL_FLG_VISIBLE;
					if (!ptl->child) ptl->m_nImage = 1; //empty folders are open
				}
			}
			else {
				//Map layer ==
				if (*fname) {
					if (*fpath) {
						strcat(fpath, "\\");
						strcat(fpath, fname);
						//if(_access(fpath,0)) *fpath=0;
					}
					else {
						//if(!*fpath) {
						strcpy(fpath, lpszPathName);
						strcpy(trx_Stpnam(fpath), fname);
					}

					if (OpenLayer(fpath, (*ftitle) ? ftitle : fname, uFlags | NTL_FLG_LAYERSET)) {
						PML pLayer = (PML)LayerPtr(0);
						pLayer->m_uLevel = uDepth;
						pLayer->scaleRange = scaleRange;
						if (pLayer->LayerType() == TYP_SHP) {
							CShpLayer *pShp = (CShpLayer *)pLayer;
							if (!nLblFld && !csLabel.IsEmpty())
								nLblFld = pShp->m_pdb->FldNum(csLabel);
							if (nLblFld) {
								VERIFY(pShp->SetLblFld(nLblFld)); //override tmpshp
								if (!bSrchFlds) pShp->InitSrchFlds(nLblFld);  //override tmpshp
							}
							if (bSrchFlds) pShp->SetSrchFlds(vcsSrchFld); //override tmpshp
							pShp->m_crLbl = crLbl;
							pShp->m_mstyle = mstyle;
							pShp->m_font = lf;
							pShp->UpdateImage(m_layerset.m_pImageList);
#ifdef _USE_QTFLG
							if (bQTdepth && pShp->ShpType() != CShpLayer::SHP_POINT) {
#else
							if (bQTdepth && pShp->ShpType() == CShpLayer::SHP_POLYGON) {
#endif
								pShp->m_bQTdepth = bQTdepth;
								pShp->m_bQTusing = bQTusing;
								pShp->m_uQTnodes = uQTnodes;
							}
							}
						else {
							CImageLayer *pImg = (CImageLayer *)pLayer;
							pImg->m_crTransColor = crTransColor;
							pImg->m_bUseTransColor = bUseTransColor;
							pImg->m_bAlpha = bAlpha;
						}
						ASSERT(!pLayer->nextsib && !pLayer->prevsib && !pLayer->child && !pLayer->parent);
#ifdef _DEBUG			
						layercnt++;
#endif
						}
					else {
						if (CLayerSet::m_bOpenConflict) {
							LPCSTR p = (CLayerSet::m_bOpenConflict < 0) ? "missing or" : "protected by another process and";
							CLayerSet::m_bOpenConflict = 0;

							if (!bSkipMissing) {
								LPCSTR pProject = trx_Stpnam(lpszPathName);
								CString msg;
								msg.Format("%s\n\nThis file is %s can't be opened. You can Skip this or the remaining\n"
									"missing layers, or Cancel to abort opening project %s.\n\n"
									"NOTE: After opening the available layers you may want to protect the original\n"
									"project file from overwriting by saving with a new name (File -> Save As).",
									fpath, p, pProject);

								MessageBeep(MB_ICONINFORMATION);

								int i = MsgYesNoCancelDlg(NULL, msg, pProject, "Skip", "Skip All", "Cancel");


								if (i == IDCANCEL) {
									m_pFileNTL->Close();
									return FALSE;
								}
								if (i == IDNO) bSkipMissing = true;
							}
						}
						SetChanged();
						if (CNtiLayer::m_bExpanding) {
							m_pFileNTL->Close();
							return FALSE;
						}
					}
					}
				*fname = *fpath = *ftitle = 0;
				}
			break;

		case NTL_CFG_LNAME:
			if (m_pFileNTL->Argc() > 1) trx_Stncc(fname, m_pFileNTL->Argv(1), _MAX_FNAME);
			break;

		case NTL_CFG_LPATH:
			if (m_pFileNTL->Argc() > 1) {
				char *p = m_pFileNTL->Argv(1);
				if (!GetFullFromRelative(fpath, p, lpszPathName)) {
					ASSERT(0);
					*fpath = 0;
				}
			}
			break;
			}
		}

	m_pFileNTL->FreeCFG();

	ASSERT(layercnt == NumLayers());

	if (CLayerSet::m_bReadWriteConflict) {
		//CLayerSet::m_bReadWriteConflict=false;
		CMsgBox(MB_ICONINFORMATION, "NOTE: At least one shapefile layer was set non-editable due to being protected "
			"by another process.");
	}

	if (!LayerSet().InitPML()) {
		CMsgBox("File %s has no map layers.", lpszPathName);
		m_pFileNTL->Close();
		return FALSE;
	}

	if (crBack != RGB(0, 0, 0)) {
		m_hbrBack = CreateSolidBrush(m_clrBack = m_clrPrev = crBack);
	}
	m_uEnableFlags |= vFlags;

	//Finally fix sibling links of depth 0 layers and replace depth with tree sequence numbers --
	{
		uDepth = 0;
		PTL pLast = 0;
		for (rit_Layer it = BeginLayerRit(); it != EndLayerRit(); it++) {
			PTL ptl = *it;
			if (!ptl->m_uLevel) {
				if (ptl->prevsib = pLast) pLast->nextsib = ptl;
				ASSERT(pLast || !ptl->parent || ptl->parent->child == ptl);
				pLast = ptl;
			}
			ptl->m_uLevel = uDepth++;
		}
	}

	return TRUE;
	}

void CWallsMapDoc::OnFileProperties()
{
	//CDlgProperties dlg(this);
	//dlg.DoModal();
	if (hPropHook) return;
	CLayerSetSheet *pDlg;
	try {
		pDlg = new CLayerSetSheet(this, NULL, GetPropStatus().nTab);
	}
	catch (...) {
		AfxMessageBox("No memory");
		return;
	}
	pDlg->Create(AfxGetApp()->m_pMainWnd);
}

void CWallsMapDoc::OnUpdateFileProperties(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(!hPropHook);
}

void CWallsMapDoc::OnCloseDocument()
{
	if (hPropHook && GetMF()->NumDocsOpen() < 2) {
		hPropHook = FALSE;
		pLayerSheet->DestroyWindow();
	}
	if (CMainFrame::IsPref(PRF_OPEN_LAST) && !_stricmp(trx_Stpext(m_strPathName), NTL_EXT))
		theApp.UpdateRecentMapFile(m_strPathName);

	CDocument::OnCloseDocument();
}

// CWallsMapDoc diagnostics

#ifdef _DEBUG
void CWallsMapDoc::AssertValid() const
{
	CDocument::AssertValid();
}
#endif //_DEBUG

void CWallsMapDoc::ExportNTI()
{
	int iHint = 0;
	PML pLayer = (PML)SelectedLayerPtr();

	if (pLayer->LayerType() == TYP_SHP) {
		/* probably not needed since we will save a valid set before exporting --
		if(app_pShowDlg && app_pShowDlg->m_pSelLayer==pLayer &&
			app_pShowDlg->CancelSelChange()) {
				app_pShowDlg->BringWindowToTop();
				return;
		}
		*/
		DWORD nRecs = ((CShpLayer *)pLayer)->m_nNumRecs;
		VEC_DWORD vRec(nRecs);
		for (DWORD n = 0; n < nRecs; n++) vRec[n] = n + 1;
		//iHint=((CShpLayer *)pLayer)->ExportRecords(vRec,false)?LHINT_FITIMAGE:0; //see below
		((CShpLayer *)pLayer)->ExportRecords(vRec, false);
		return;
	}
	//else
	{
		CExportNTI dlg((CImageLayer *)pLayer);
		m_pExportNTI = &dlg;
		CExportNTI_Adv::LoadAdvParams();
		dlg.DoModal();
		iHint = dlg.m_iHint; // 0 if not replacing layer, else LHINT_FITIMAGE or LHINT_REFRESH
	}

	if (iHint) {
		//Layer was replaced!
		if (NumLayers() == 1) {
			pLayer = (PML)LayerPtr(0);
			SetTitle(pLayer->FileName());
			theApp.AddToRecentFileList(m_strPathName = pLayer->PathName());
		}
		else iHint = LHINT_REFRESH;

		//CWnd *pWnd=hPropHook?pLayerSheet:this;
		if (hPropHook) {
			pLayerSheet->SendMessage(WM_PROPVIEWDOC, (WPARAM)this, (LPARAM)LHINT_NEWLAYER);
		}
		if (iHint == LHINT_REFRESH) {
			RefreshViews();
		}
		else {
			ASSERT(iHint == LHINT_FITIMAGE);
			//Zoom to layer
			POSITION pos = GetFirstViewPosition();
			CWallsMapView *pView = (CWallsMapView *)GetNextView(pos);
			if (pView)
				pView->FitImage(LHINT_FITIMAGE);
		}
	}
}

bool CWallsMapDoc::HasZipContent(PTL pFolder, BOOL bIncHidden, BOOL bShpsOnly)
{
	PTL pend = pFolder->nextsib;
	PTL pnext = pFolder->child;
	while (pnext && pnext != pend) {
		if (pnext->IsFolder()) {
			if (HasZipContent(pnext, bIncHidden, bShpsOnly))
				return true;
		}
		else {
			bool bShp = (pnext->LayerType() == TYP_SHP);
			if ((bIncHidden || pnext->IsVisible()) && (bShp || !(bShpsOnly & 1)) &&
				(!bShp || !(bShpsOnly & 2) || ((CShpLayer *)pnext)->ShpType() == CShpLayer::SHP_POINT))
				return true;
		}
		pnext = pnext->nextsib;
	}
	return false;
}


BOOL CWallsMapDoc::WriteNTL(CFileCfg *pFile, const CString &csPathName, BOOL bIncHidden /*=TRUE*/, BOOL bShpsOnly /*=FALSE*/)
{

	if (CImageLayer::m_pDlgSymbolsImg && CImageLayer::m_pDlgSymbolsImg->m_pLayer->m_pDoc == this) {
		CImageLayer::m_pDlgSymbolsImg->NewLayer(0);
	}
	TRY
	{
		pFile->Write(";WallsMap Layer File\r\n");

		if (m_clrBack != RGB(0,0,0)) {
			pFile->WriteArgv(".VBKRGB\t%u %u %u\r\n",GetRValue(m_clrBack),GetGValue(m_clrBack),GetBValue(m_clrBack));
		}

		if (m_uEnableFlags&NTL_FLG_ALLSAVED) {
			pFile->WriteArgv(".VFLAGS\t%u\r\n",m_uEnableFlags&NTL_FLG_ALLSAVED);

			if ((m_uEnableFlags&NTL_FLG_RESTOREVIEW) && m_pRestorePosition && m_pRestorePosition->uStatus) {
				int decs = IsProjected() ? 1 : 6;
				pFile->WriteArgv(".VVIEW\t%.*f %.*f %.*f\r\n",
					decs,m_pRestorePosition->geoHeight,
					decs,m_pRestorePosition->ptGeo.x,
					decs,m_pRestorePosition->ptGeo.y);
				m_pRestorePosition->uStatus = 1;
			}
		}

		if (m_phist_find && IsSavingHistory()) {
			CComboHistItem *pItem = m_phist_find->m_pFirst;
			while (pItem) {
				pFile->WriteArgv(".STRFIND\t%u %s\r\n",pItem->m_wFlags,(LPCSTR)pItem->m_Str);
				pItem = pItem->m_pNext;
			}
		}

		if (LayersWidth() && IsSavingLayers()) {
			CPropStatus &prop = m_layerset.m_PropStatus;
			pFile->WriteArgv(".SLAYERS\t%u %u %u %u %u\r\n",prop.nWinWidth,prop.nWinHeight,prop.nColWidth[0],prop.nColWidth[1],prop.nColWidth[2]);
		}

		char pathBuf[MAX_PATH];
		CScaleRange scaleRange; //for testing if non-zero

		//write layers in order of rendering, bottom of displayed tree to top, or from beginning of layers vector
		for (it_Layer it = BeginLayerIt(); it != EndLayerIt(); it++) {
			CMapLayer &layer = *(PML)*it;

			UINT uDepth = 0;
			for (PTL ptl = &layer; ptl = ptl->parent; uDepth++);

			if (layer.IsFolder()) {
				//Exclude if no map layers would be included (!bIncHidden /*=TRUE*/,BOOL bShpsOnly
				if (bIncHidden && !bShpsOnly || HasZipContent(&layer,bIncHidden,bShpsOnly))
					pFile->WriteArgv(".FOLDER\t%s\r\n.LSTATUS\t%u %u %u\r\n.LEND\r\n",layer.Title(),layer.m_uFlags,uDepth,layer.m_nImage);
			}
			else {
				bool bShp = (layer.LayerType() == TYP_SHP);
				if (!bIncHidden && !layer.IsVisible() || !bShp && (bShpsOnly & 1) != 0 ||
						bShp && (bShpsOnly & 2) != 0 && ((CShpLayer *)&layer)->ShpType() != CShpLayer::SHP_POINT)
					continue;

				pFile->WriteArgv(".LAYER\t%s\r\n.LNAME\t%s\r\n",layer.Title(),layer.FileName());
				if (uDepth) pFile->WriteArgv(".LDEPTH\t%u\r\n",uDepth);

				GetRelativePath(pathBuf,layer.PathName(),csPathName,FALSE);
				LPSTR p0 = trx_Stpnam(pathBuf);
				if (p0 > pathBuf) {
					*--p0 = 0;
					pFile->WriteArgv(".LPATH\t%s\r\n",pathBuf);
				}
				if (bShp) {
					const CShpLayer &shp = (const CShpLayer &)layer;
					SHP_MRK_STYLE const &mstyleS = shp.m_mstyle;
					SHP_MRK_STYLE mstyle;
					CLogFont font;
					if (memcmp(&shp.m_mstyle,&mstyle,sizeof(mstyle))) {
						CString s;
						pFile->WriteArgv(".LMSTYLE\t%s\r\n",shp.m_mstyle.GetProfileStr(s));
					}

					if (shp.m_nLblFld != 1 || shp.m_crLbl != RGB(255,255,255)) {
						LPCSTR p = shp.m_pdb->FldNamPtr(shp.m_nLblFld);
						if (p) pFile->WriteArgv(".LLBLFLD \"%s\" %u\r\n",p,shp.m_crLbl);
					}

					if (shp.ShpType() == CShpLayer::SHP_POINT) {
						CString s;
						if (shp.GetSrchFlds(s)) {
							pFile->WriteArgv(".LSRCHFLD%s\r\n",(LPCSTR)s);
						}
					}
					else if (shp.m_bQTusing || shp.m_uQTnodes || shp.m_bQTdepth != QT_DFLT_DEPTH) {
						ASSERT(shp.ShpType() == CShpLayer::SHP_POLYGON);
						pFile->WriteArgv(".LQTDEPTH %u %u %u\r\n",shp.m_bQTdepth,shp.m_bQTusing,shp.m_uQTnodes);
					}

					if (memcmp(&shp.m_font,&font,sizeof(font))) {
						CString s;
						shp.m_font.GetStrFromFont(s);
						pFile->WriteArgv(".LFONT\t%s\r\n",(LPCSTR)s);
					}
				}
				else {
					const CImageLayer &img = (const CImageLayer &)layer;
					if (img.m_crTransColor || img.IsTransparent())
						pFile->WriteArgv(".LOPACITY\t%u %u %u\r\n",img.m_bAlpha,(img.m_bUseTransColor == true),
							img.m_crTransColor);
				}

				if (memcmp(&layer.scaleRange,&scaleRange,sizeof(CScaleRange))) {
					pFile->WriteArgv(".LSCALES\t%u %u %u %u %u %u\r\n",
						layer.scaleRange.bLayerNoShow,
						layer.scaleRange.uLayerZoomIn,
						layer.scaleRange.uLayerZoomOut,
						layer.scaleRange.bLabelNoShow,
						layer.scaleRange.uLabelZoomIn,
						layer.scaleRange.uLabelZoomOut);
				}

				//***Bug removed 10/4/2013!
				//if(layer.m_uFlags!=NTL_FLG_VISIBLE)
				//  pFile->WriteArgv(".LSTATUS\t%u %u\r\n",layer.m_uFlags,uDepth);
				//pFile->Write(".LEND\r\n");
				pFile->WriteArgv(".LSTATUS\t%u %u\r\n.LEND\r\n",layer.m_uFlags,uDepth);
			}
		}
		pFile->Flush();
		pFile->SetLength(pFile->GetPosition());
		pFile->FreeCFG();
	}
		CATCH(CFileException, e)
	{
		pFile->ReportException(csPathName, e, TRUE, AFX_IDP_FAILED_TO_SAVE_DOC);

		return FALSE;
	}
	END_CATCH

		return TRUE;
}

BOOL CWallsMapDoc::SaveLayerSet(BOOL bSaveAs /*=FALSE*/)
{
	if (app_pShowDlg && app_pShowDlg->m_pDoc == this && app_pShowDlg->CancelSelChange()) {
		app_pShowDlg->BringWindowToTop();
		return FALSE;
	}

	if (bSaveAs && m_pfiu) Release_fiu();

	if (!bSaveAs && LayerSet().HasEditedShapes())
		LayerSet().SaveShapefiles();

	if (hPropHook && IsSavingLayers() && pLayerSheet->GetDoc() == this) {
		pLayerSheet->SaveWindowSize();
	}

	if (!IsChanged() && !bSaveAs) return TRUE;

	CString csPathName = m_strPathName;

	if (m_bExtensionNTL != 2 || m_bReadOnly) bSaveAs = TRUE;

	if (bSaveAs) {
		CString strFilter;
		if (!AddFilter(strFilter, IDS_NTL_FILES)) return FALSE;
		if (!m_bExtensionNTL) {
			csPathName.Truncate(csPathName.GetLength() - strlen(trx_Stpext(csPathName)));
		}
		if (m_bExtensionNTL != 2) csPathName += NTL_EXT;
		if (!DoPromptPathName(csPathName,
			OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
			1, strFilter, FALSE, IDS_NTL_SAVEAS, NTL_EXT)) return FALSE;

		CWallsMapDoc *pDoc = GetMF()->FindOpenDoc(csPathName);
		if (pDoc) {
			if (pDoc != this) {
				CMsgBox("%s\nThis file is currently open and can't be overwritten.", (LPCSTR)csPathName);
				return FALSE;
			}
			if (!IsChanged()) return TRUE;
			bSaveAs = FALSE;
		}
	}

	CFileCfg *pFile;

	if (bSaveAs) {
		pFile = new CFileCfg(ntltypes, NTL_CFG_NUMTYPES);

		CFileException ex;

		if (!pFile->Open(csPathName, CFile::modeCreate | CFile::modeReadWrite | CFile::shareExclusive, &ex)) {
			if (pFile->BufferSize()) {
				//file related, not memory alloc failure --
				if (_access(csPathName, 0))
					pFile->ReportException(csPathName, &ex, TRUE, AFX_IDP_FAILED_TO_CREATE_DOC);
				else
					CMsgBox("%s\nError replacing file. It may be protected by another program instance.",
					(LPCSTR)csPathName);
			}
			delete pFile;
			return FALSE;
		}
	}
	else {
		pFile = m_pFileNTL;
		if (!pFile->OpenCFG()) {
			//seek 0 failure!!
			ASSERT(0);
			return FALSE;
		}
	}

	if (!WriteNTL(pFile, csPathName)) {
		if (bSaveAs) {
			pFile->Abort(); // will not throw an exception
			delete pFile;
		}
		else {
			pFile->FreeCFG(); //file remains open
		}
		return FALSE;
	}

	if (bSaveAs) {
		if (m_pFileNTL) {
			m_pFileNTL->Abort(); //caused problem with mirrored file
			delete m_pFileNTL;
		}
		m_pFileNTL = pFile;
		m_bReadOnly = false;
		SetPathName(csPathName);
		m_bExtensionNTL = 2;
		if (hPropHook) pLayerSheet->SetPropTitle();
		Register_fiu(csPathName);
	}

	m_bHistoryUpdated = m_bLayersUpdated = false;
	SetUnChanged();
	return TRUE;
}

void CWallsMapDoc::OnFileConvert()
{
	ASSERT(NumLayers() == 1 && !m_bExtensionNTL);
	ExportNTI();
}

void CWallsMapDoc::OnUpdateFileConvert(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(NumLayers() == 1 && !m_bExtensionNTL &&
		(LayerPtr(0)->LayerType() != TYP_SHP || ((CShpLayer *)LayerPtr(0))->ShpType() == CShpLayer::SHP_POINT));
}

void CWallsMapDoc::OnFileSave()
{
	SaveLayerSet();
}

void CWallsMapDoc::OnUpdateFileSave(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(!m_bReadOnly && (IsChanged() || m_layerset.HasEditedShapes()));
}

void CWallsMapDoc::OnFileSaveAs()
{
	SaveLayerSet(TRUE);
}

void CWallsMapDoc::OnUpdateFileSaveAs(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(TRUE);
}

BOOL CWallsMapDoc::SaveModified()
{
	//if (IsModified()) return DoSave(m_strPathName);
	if (m_bReadOnly) {
		ASSERT(!m_layerset.HasEditedShapes());
		return TRUE;
	}

	if (app_pShowDlg && app_pShowDlg->m_pDoc == this && app_pShowDlg->CancelSelChange()) {
		app_pShowDlg->BringWindowToTop();
		return FALSE;
	}

	if (hPropHook && pLayerSheet->GetDoc() == this) {
		pLayerSheet->DestroyWindow(); //may save layers window layout
	}

	if (app_pDbtEditDlg && app_pDbtEditDlg->m_pShp->m_pDoc == this) {
		if (app_pDbtEditDlg->DenyClose())
			return FALSE;
		app_pDbtEditDlg->Destroy();
	}

	if (app_pImageDlg && app_pImageDlg->m_pShp->m_pDoc == this) {
		if (app_pImageDlg->DenyClose())
			return FALSE;
		app_pImageDlg->Destroy();
	}

	if (m_layerset.HasEditedShapes()) {
		LayerSet().SaveShapefiles();
	}

	if (IsChanged() >= 2) { //Prompt to save changes upon close (if 1 and m_pRestorePosition->uStatus>1, save without prompting
		CString name = m_strPathName;
		if (!m_bExtensionNTL) {
			name.Truncate(name.GetLength() - strlen(trx_Stpext(name)));
		}
		ASSERT(!name.IsEmpty());		//*** CDocument version loads AFX_IDS_UNTITLED

		CString prompt;
		AfxFormatString1(prompt, IDS_NTL_ASK_TO_SAVE, name);

		switch (AfxMessageBox(prompt, MB_YESNOCANCEL))
		{
		case IDCANCEL:
			return FALSE;       // don't continue

		case IDYES:
			// If so, either Save or Update, as appropriate
			break;

		case IDNO:
			// If not saving changes, revert the document
			return TRUE;

		default:
			ASSERT(FALSE);
			break;
		}
	}

	if (m_bHistoryUpdated || m_bLayersUpdated || ((m_uEnableFlags&NTL_FLG_RESTOREVIEW) && m_pRestorePosition && m_pRestorePosition->uStatus > 1)) {
		SetChanged();
	}
	return IsChanged() < 2 || SaveLayerSet();    // allow close
}

BOOL CWallsMapDoc::OnSaveDocument(const char* pszPathName)
{
	ASSERT(FALSE);
	return FALSE;
}

BOOL CWallsMapDoc::AddLayer(CString &pathName, UINT uFlags, int pos /*=-1*/, int posSib /*=0*/)
{
	//Creates and adds visible CMapLayer object.
	//(no change yet to tree display if present)

	//Open and insert single layer --
	//pos==-1 for default insertion pos, else pos==predetermined tree position of newly-opened layer.
	//In the latter case, posSib==-1 if adding a first child at pos+1, or posSib==current position of sibling
	//OpenLayer() returns 0 (fail) or tree position+1.

	BOOL bPos = OpenLayer(pathName, trx_Stpnam(pathName), uFlags, pos, posSib);
	if (!bPos)
		return FALSE;

	SetChanged();
	SetSelectedLayerPos(bPos - 1); //sets m_PropStatus.nPos=bPos=1 -- last added addem will be selected
	if (!m_bExtensionNTL) {
		pathName.Format("LayerSet%u", m_nDocs++);
		m_bExtensionNTL++;
		SetPathName(pathName, FALSE); //Do not add to recent file list
		if (hPropHook) {
			pLayerSheet->SetWindowText(pathName);
		}
	}
	return TRUE;
}

void CWallsMapDoc::AddFileLayers(bool bFromTree /*=false*/, int posSib /*=0*/)
{
	// prompt the user (with all document templates)
	char pathName[10 * MAX_PATH];
	*pathName = 0;
	BOOL bEditable = PromptForFileName(pathName, 10 * MAX_PATH,
		String128("Add layers to %s", (LPCSTR)GetTitle()), OFN_ALLOWMULTISELECT, FTYP_NONTL);

	if (!bEditable) return; // open cancelled
	bEditable--; //True if NOT opened in ReadOnly mode
	UINT uFlags = (bEditable && CMainFrame::IsPref(PRF_OPEN_EDITABLE)) ? NTL_FLG_EDITABLE : 0;
	UINT nAdded = 0;
	CNtiLayer::m_bExpanding = FALSE;
	char *p = pathName + strlen(pathName) + 1;
	CString path = pathName;
	int pos;

	if (!posSib) {
		uFlags |= NTL_FLG_VISIBLE;
		pos = -1;
	}
	else {
		//Determine pos=position of first insertion wrt selected layer --
		//Intially: posSib==2 if first child, posSib==-1 if prevsib, posSib==1 if nextsib
		//For Addlayer(), posSib==-1 if adding first child of selected layer, else posSib==current position of sibling
		ASSERT(bFromTree && posSib >= -1 && posSib <= 2);
		PTL ptl = SelectedLayerPtr();
		ASSERT(ptl->m_uLevel == SelectedLayerPos());

		if (posSib == 2) {
			pos = ptl->m_uLevel + 1;
			posSib = -1;
			uFlags |= (ptl->m_uFlags&NTL_FLG_VISIBLE);
		}
		else {
			if (!ptl->parent || ptl->parent->IsVisible())
				uFlags |= NTL_FLG_VISIBLE;
			if (posSib == -1) {
				pos = posSib = ptl->m_uLevel;
			}
			else {
				posSib = ptl->m_uLevel;
				pos = ptl->nextsib ? ptl->nextsib->m_uLevel : (posSib + 1);
			}
		}
	}

	if (*p) {
		while (*p) {
			path.AppendChar('\\');
			path += p;
			//AddLayer() calls OpenLayer() which Calls InsertLayer() --
			if (AddLayer(path, uFlags, pos, posSib)) {
				nAdded++;
				if (pos >= 0) posSib = pos++;
			}
			path = pathName;
			p += strlen(p) + 1;
		}
	}
	else {
		if (AddLayer(path, uFlags, pos, posSib)) nAdded++;
		//pos=SelectedLayerPos(); 
	}

	if (nAdded) {
		SetChanged(); //prompt for save upon closing
		CWallsMapView::SyncStop();
		if (!hPropHook) {
			GetPropStatus().nTab = 0; //Next invocation will show layers page
		}
		else {
			ASSERT(this == pLayerSheet->GetDoc());
			pLayerSheet->SendMessage(WM_PROPVIEWDOC, (WPARAM)this, (LPARAM)LHINT_NEWLAYER);
		}
		if (uFlags&NTL_FLG_VISIBLE)
			RefreshViews();
	}
}

void CWallsMapDoc::OnFileTruncate()
{
	if (IDYES == CMsgBox(MB_YESNO | MB_ICONQUESTION, IDS_NTI_TRUNCATE1, ((PML)LayerPtr(0))->FileName())) {
		nti_bTruncateOnClose = TRUE;
		OnCloseDocument();
		nti_bTruncateOnClose = FALSE;
	}
}

void CWallsMapDoc::OnUpdateFileTruncate(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(NumLayers() == 1 && LayerPtr(0)->LayerType() == TYP_NTI);
}

void CWallsMapDoc::OnToolSelect()
{
	m_bSelecting = (m_bSelecting == 1) ? 0 : 1;
	m_uActionFlags = (m_bSelecting ? ACTION_SELECT : 0);
}

void CWallsMapDoc::OnUpdateToolSelect(CCmdUI *pCmdUI)
{
	if (HasShpPtLayers()) {
		pCmdUI->Enable(TRUE);
		pCmdUI->SetCheck(m_bSelecting == 1);
	}
	else {
		if (m_bSelecting == 1) m_bSelecting = 0;
		pCmdUI->SetCheck(FALSE);
		pCmdUI->Enable(FALSE);
	}
}
void CWallsMapDoc::OnSelectAdd()
{
	m_bSelecting = (m_bSelecting == 2) ? 0 : 2;
	m_uActionFlags = (m_bSelecting ? ACTION_SELECT : 0);
}

void CWallsMapDoc::OnUpdateSelectAdd(CCmdUI *pCmdUI)
{
	if (HasShpPtLayers()) {
		pCmdUI->Enable(TRUE);
		pCmdUI->SetCheck(m_bSelecting == 2);
	}
	else {
		if (m_bSelecting == 2) m_bSelecting = 0;
		pCmdUI->SetCheck(FALSE);
		pCmdUI->Enable(FALSE);
	}
}

void CWallsMapDoc::ClearSelectMode()
{
	if (m_bSelecting) {
		m_bSelecting = 0;
		m_uActionFlags = 0;
	}
}

void CWallsMapDoc::OnViewPan()
{
	ClearSelectMode();
	m_uActionFlags ^= ACTION_PAN;
	if (m_uActionFlags&ACTION_PAN) m_uActionFlags = ACTION_PAN; //panning only
}

void CWallsMapDoc::OnUpdateViewPan(CCmdUI *pCmdUI)
{
	BOOL bEnable = (0 != AfxGetApp()->GetOpenDocumentCount());
	UINT flag = 0;
	if (bEnable) {
		switch (pCmdUI->m_nID) {
			//case ID_TOOLSELECT : flag=ACTION_SELECT; break;
		case ID_TOOLMEASURE: flag = ACTION_MEASURE; break;
		case ID_VIEW_ZOOMIN: flag = ACTION_ZOOMIN; break;
		case ID_VIEW_ZOOMOUT: flag = ACTION_ZOOMOUT; break;
		case ID_VIEW_PAN: flag = ACTION_PAN;
		}
		pCmdUI->SetCheck((!flag && !m_uActionFlags) || (flag & m_uActionFlags) != 0);
	}
	pCmdUI->Enable(bEnable);
}

void CWallsMapDoc::OnViewZoomin()
{
	ClearSelectMode();
	m_uActionFlags ^= ACTION_ZOOMIN;
	if (m_uActionFlags&ACTION_ZOOMIN) m_uActionFlags = ACTION_ZOOMIN;
}

void CWallsMapDoc::OnViewZoomout()
{
	ClearSelectMode();
	m_uActionFlags ^= ACTION_ZOOMOUT;
	if (m_uActionFlags&ACTION_ZOOMOUT) m_uActionFlags = ACTION_ZOOMOUT;
}

void CWallsMapDoc::OnToolMeasure()
{
	ClearSelectMode();
	m_uActionFlags ^= ACTION_MEASURE;
	if (m_uActionFlags&ACTION_MEASURE) m_uActionFlags = ACTION_MEASURE;
}

void CWallsMapDoc::OnViewDefault()
{
	ClearSelectMode();
	m_uActionFlags = 0;
}

bool CWallsMapDoc::SelectionLimited(UINT size)
{
	if (size > MAX_VEC_SHPREC_SIZE) {
		AfxMessageBox("The number of points matched or chosen would exceed the capacity of "
			"the Selected Points dialog, which is 10000.",
			MAX_VEC_SHPREC_SIZE);
		return m_bSelectionLimited = true;
	}
	return false;
}

void CWallsMapDoc::ReplaceVecShprec()
{
	ClearSelChangedFlags();
	UnflagSelectedRecs();
	m_vec_shprec.swap(CLayerSet::vec_shprec_srch);
	CLayerSet::Empty_shprec_srch(); //free memory
	FlagSelectedRecs();
	RefreshTables();
}

void CWallsMapDoc::ClearVecShprec()
{
	ClearSelChangedFlags();
	UnflagSelectedRecs();
	VEC_SHPREC().swap(m_vec_shprec);
	m_layerset.m_bSelectedSetFlagged = true;
	RefreshTables();
}

void CWallsMapDoc::OnExportZip()
{
	CZipDlg dlg(this);
	dlg.DoModal();
}

bool CWallsMapDoc::GetConvertedGPSPoint(CFltPoint &fp)
{
	int zone = LayerSet().m_iZone;
	if (zone != 0 || LayerSet().m_iNad != 1) {
		return GetConvertedPoint(fp, LayerSet().m_iNad, &zone, 1, 0);
	}
	return true;
}

BOOL CWallsMapDoc::DynamicAddLayer(CString &pathName)
{

	if (AddLayer(pathName, NTL_FLG_VISIBLE | NTL_FLG_EDITABLE)) { //editable layer
		CWallsMapView::SyncStop();
		if (!hPropHook) {
			GetPropStatus().nTab = 0; //Next invocation will show layers page
			RefreshViews();
		}
		//Else Let the prop dialog refresh views after its own screen update.
		else if (this == pLayerSheet->GetDoc()) {
			pLayerSheet->SendMessage(WM_PROPVIEWDOC, (WPARAM)this, (LPARAM)LHINT_NEWLAYER);
			pLayerSheet->RefreshViews();
		}
		return TRUE;
	}
	return FALSE;
}

CShpLayer *CWallsMapDoc::FindShpLayer(LPCSTR pName, UINT *pFldNum, UINT *pFldPfx/*=NULL*/)
{
	char buf[MAX_PATH];
	char namPfx[16], namFld[16];
	trx_Stncc(buf, pName, MAX_PATH - 6);

	LPSTR p = NULL, pfx = NULL;

	if (pFldNum) {
		if (p = strstr(buf, "::")) {
			*p = 0;
			p = buf + (p - buf) + 2; //p now points to field name
			if (pFldPfx) {
				pfx = strchr(p, ',');
				if (pfx) {
					*pfx++ = 0;
					pfx = trx_Stncc(namPfx, pfx, 11); //prefix field name
				}
				else return NULL;
			}
			trx_Stncc(namFld, p, 11);
		}
		else return NULL;
	}

	if (_stricmp(SHP_EXT_SHP, trx_Stpext(buf)))
		strcat(buf, SHP_EXT_SHP);

	for (PML pml = m_layerset.InitTreePML(); pml; pml = m_layerset.NextTreePML()) {
		if (pml->LayerType() == TYP_SHP && (pFldPfx || ((CShpLayer *)pml)->ShpType() == CShpLayer::SHP_POLYGON)) {

			if (!_stricmp(buf, pml->FileName())) {
				if (p) {
					if (!(*pFldNum = ((CShpLayer *)pml)->m_pdb->FldNum(namFld)) || pfx && !(*pFldPfx = ((CShpLayer *)pml)->m_pdb->FldNum(namPfx)))
						return NULL;
				}
				return (CShpLayer *)pml;
			}
		}
	}
	return NULL;
}

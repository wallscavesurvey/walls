// ExportShp.cpp : implementation file
//

#include "stdafx.h"
#include "WallsMap.h"
#include "MainFrm.h"
#include "WallsMapDoc.h"
#include "WallsMapView.h"
#include "ShpLayer.h"
#ifndef __SAFEMIRRORFILE_H
#include "SafeMirrorFile.h"
#endif
#include "Layersetsheet.h"
#include "shpdef.h"
#include "ExportShpDlg.h"

// CExportShpDlg dialog

IMPLEMENT_DYNAMIC(CExportShpDlg, CDialog)
CExportShpDlg::CExportShpDlg(CShpLayer *pShp,VEC_DWORD &vRec,bool bSelected,CWnd* pParent /*=NULL*/)
	: CDialog(CExportShpDlg::IDD, pParent)
	, m_pShp(pShp)
	, m_vRec(vRec)
	, m_bSelected(bSelected)
	, m_bUseViewCoord(FALSE)
	, m_tempName(_T(""))
	, m_bRetainDeleted(FALSE)
	, m_bInitEmpty(FALSE)
	, m_bExcludeMemos(FALSE)
	, m_bGenTemplate(FALSE)
	, m_bAddLayer(pShp->m_pDoc->m_bReadOnly?0:1)
	, m_bUseLayout(FALSE)
	, m_bMake2D(TRUE)
{
	char buf[_MAX_PATH];
	strcpy(buf,pShp->PathName());
	char *p=trx_Stpext(buf);
	if(p-buf+8<_MAX_PATH) strcpy(p,"_exp.shp");
	m_pathName=buf;
	m_pDoc=pShp->m_pDoc;
}

CExportShpDlg::~CExportShpDlg()
{
}

bool CExportShpDlg::Check_template_name(LPCSTR tmppath,LPCSTR shppath)
{
	if(_stricmp(trx_Stpext(tmppath),SHP_EXT_TMPSHP)) {
		CMsgBox("Template files must have extension %s.",SHP_EXT_TMPSHP);
		return false;
	}
	if(!shppath) {
		LPCSTR p=trx_Stpnam(tmppath);
		if(strlen(p)>11 && !_memicmp(p, "EXPORT_", 7)) {
			m_pathName.Truncate(trx_Stpnam(m_pathName)-(LPCSTR)m_pathName);
			m_pathName+=(p+7);
			VERIFY(ForceExtension(m_pathName,SHP_EXT_SHP));
			SetText(IDC_PATHNAME,m_pathName);
		}
		return true;
	}

	if(shppath) {
		if(_access(tmppath,0)) {
			CMsgBox("Template file %s not found.",trx_Stpnam(tmppath));
			return false;
		}
		int lentmp=trx_Stpext(tmppath)-tmppath;
		int lenshp=trx_Stpext(shppath)-shppath;
		if(lentmp==lenshp && !_memicmp(tmppath,shppath,lenshp)) {
			CMsgBox("The file you've chosen as a template, %s, can't have the same path "
				"and base name of the shapefile you're creating. Please rename it or choose "
				"a different output name or location.",trx_Stpnam(tmppath));
			return false;
		}
	}
	return true;
}

void CExportShpDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	DDX_Radio(pDX, IDC_COORD_SOURCE, m_bUseViewCoord);
	DDX_Text(pDX, IDC_PATHNAME, m_pathName);
	DDX_Control(pDX, IDC_PATHNAME, m_cePathName);
	DDV_MaxChars(pDX, m_pathName, MAX_PATH);
	DDX_Text(pDX, IDC_PATH_SHPDEF, m_tempName);
	DDX_Control(pDX, IDC_PATH_SHPDEF, m_ceTempName);
	DDV_MaxChars(pDX, m_tempName, MAX_PATH);
	DDX_Check(pDX, IDC_RETAIN_DELETED, m_bRetainDeleted);
	DDX_Check(pDX, IDC_INIT_EMPTY, m_bInitEmpty);
	DDX_Check(pDX, IDC_EXCLUDE_MEMOS, m_bExcludeMemos);
	DDX_Check(pDX, IDC_GEN_TEMPLATE, m_bGenTemplate);
	DDX_Check(pDX, IDC_ADD_LAYER, m_bAddLayer);
	DDX_Check(pDX, IDC_USE_LAYOUT, m_bUseLayout);
	DDX_Check(pDX, IDC_MAKE_2D, m_bMake2D);

	if(pDX->m_bSaveAndValidate) {

		//First, process shpdef --
		CShpDef shpdef;
		shpdef.uFlags=m_bUseViewCoord*SHPD_USEVIEWCOORD +
			m_bRetainDeleted*SHPD_RETAINDELETED + m_bExcludeMemos*SHPD_EXCLUDEMEMOS +
			m_bInitEmpty*SHPD_INITEMPTY + m_bGenTemplate*SHPD_GENTEMPLATE + m_bMake2D*SHPD_MAKE2D;


		LPSTR p=NULL;

		m_pathName.Trim();
		if(!m_pathName.IsEmpty()) {
			p=dos_FullPath(m_pathName,SHP_EXT_SHP+1); //don't force extension
			if(p && _stricmp(trx_Stpext(p),SHP_EXT_SHP)) {
				p=NULL;
			}
		}

		if(!p || !CShpLayer::check_overwrite(p)) {
			if(!p) AfxMessageBox("An output file name with extension .shp is required.");
			pDX->m_idLastControl=IDC_PATHNAME;
			pDX->m_bEditLastControl=TRUE;
			pDX->Fail();
			return;
		}
		SetText(IDC_PATHNAME,p);

		//static fcn -- delete any supported components plus .qpj (also called after an export failure) --
		if(!CShpLayer::DeleteComponents(p)) {
			pDX->m_idLastControl=IDC_PATHNAME;
			pDX->m_bEditLastControl=TRUE;
			pDX->Fail();
			return;
		}
	
		m_pathName=p;

		m_tempName.Trim();
		if(!m_tempName.IsEmpty()) {
			p=dos_FullPath(m_tempName,SHP_EXT_TMPSHP+1); //don't force extension
			if(!p || !Check_template_name(p,m_pathName)) {
				if(!p) CMsgBox("Invalid template pathname specified: %s",(LPCSTR)m_tempName);
				pDX->m_idLastControl=IDC_PATH_SHPDEF;
				pDX->m_bEditLastControl=TRUE;
				pDX->Fail();
				return;
			}
			SetText(IDC_PATH_SHPDEF,p);
			m_tempName=p;
		}

		bool bUsingTemplate=false;
		if(!m_tempName.IsEmpty()) {
			ASSERT(!m_bUseLayout);
			bUsingTemplate=true;

			if(!shpdef.Process(m_pShp,m_tempName)) {
				pDX->m_idLastControl=IDC_PATH_SHPDEF;
				pDX->m_bEditLastControl=TRUE;
				pDX->Fail();
				return;
			}
			if(!shpdef.numSrcFlds) {
				SetCheck(IDC_INIT_EMPTY, m_bInitEmpty=TRUE);
				shpdef.uFlags|=SHPD_INITEMPTY;
			}

			if((shpdef.uFlags&(SHPD_FILTER+SHPD_INITEMPTY))==SHPD_FILTER) {
				//filter m_vRec!
				VEC_DWORD vRec;
				CString sVal;
				for(it_dword it=m_vRec.begin(); it!=m_vRec.end(); it++) {
					if(!m_pShp->m_pdb->Go(*it)) {
						bool bInc=false;
						//check field content --
						for(it_filter itf=shpdef.v_filter.begin(); itf!=shpdef.v_filter.end(); itf++) {
							m_pShp->m_pdb->GetTrimmedFldStr(sVal,itf->fld);
							if(!itf->pVals) bInc=!sVal.IsEmpty();
							else {
								for(LPCSTR p=itf->pVals; *p; p+=strlen(p)+1) {
									if(!sVal.CompareNoCase(p)) {
										bInc=true;
										break;
									}
								}
							}
							if(itf->bNeg) bInc=!bInc;
							if(bInc) {
								break;
							}
						}
						if(bInc) vRec.push_back(*it);
					}
				}
				if(vRec.empty()) {
					AfxMessageBox("No records in source meet the export template's filter criteria.");
					pDX->Fail();
					return;
				}
				m_vRec.swap(vRec);
			}
		}
		else if(!m_pShp->InitShpDef(shpdef,m_bUseLayout)) {
			pDX->Fail();
			return;
		}

		if(m_bInitEmpty) m_vRec.clear();
		//Output pathName is now full, valid, and any existing components have been deleted --

		if(shpdef.uFlags&(SHPD_DESCFLDS|SHPD_COMBOFLDS)) {
			//combo options are present for exported fields --
			shpdef.uFlags|=SHPD_GENTEMPLATE;
		}

		if(!m_pShp->ExportShapes(m_pathName,shpdef,m_vRec)) {
			pDX->Fail();
			return;
		}

		if(m_bAddLayer) {
			m_bAddLayer=m_pShp->m_pDoc->DynamicAddLayer(m_pathName);
		}

		//Save certain settings --
		//bUseLayout=m_bUseLayout; //should be layer-specific!

	}
}

BEGIN_MESSAGE_MAP(CExportShpDlg, CDialog)
	ON_BN_CLICKED(IDC_BROWSE_SHPDEF, OnBnClickedBrowseShpdef)
	ON_BN_CLICKED(IDBROWSE, OnBnClickedBrowse)
	ON_BN_CLICKED(IDC_USE_LAYOUT, &CExportShpDlg::OnBnClickedUseLayout)
    ON_MESSAGE(WM_COMMANDHELP,OnCommandHelp)
END_MESSAGE_MAP()

// CExportShpDlg message handlers

void CExportShpDlg::OnBnClickedBrowseShpdef()
{
	CString strFilter;
	if(!AddFilter(strFilter,IDS_SHPDEF_FILES)) return;
	if(!DoPromptPathName(m_tempName,
		OFN_FILEMUSTEXIST,
		1,strFilter,TRUE,IDS_SHPDEF_OPEN,
		SHP_EXT_TMPSHP)) return;

	if(Check_template_name(m_tempName,NULL))
		SetText(IDC_PATH_SHPDEF,m_tempName);
}

void CExportShpDlg::OnBnClickedBrowse()
{
	CString strFilter;
	if(!AddFilter(strFilter,IDS_SHP_FILES)) return;
	if(!DoPromptPathName(m_pathName,
		OFN_HIDEREADONLY,
		1,strFilter,FALSE,IDS_SHP_EXPORT,SHP_EXT)) return;
	SetText(IDC_PATHNAME,m_pathName);
}

BOOL CExportShpDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	CString title;
	title.Format("Export of %s (%u %srecord%s)",m_pShp->Title(),m_vRec.size(),
		m_bSelected?"selected ":"",(m_vRec.size()>1)?"s":"");

	SetWindowText(title);

	SetText(IDC_ST_MSGHDR,"This operation creates a new shapefile from the content of the one selected. "
		                  "If a template file name is specified, it will be used to revise the attribute "
						  "structure while possibly adding special editing options. A table view's layout "
						  "(column order and visibility) can also serve as a template.");

	Enable(IDC_USE_LAYOUT,m_pShp->IsGridDlgOpen());

	CLayerSet &lSet=m_pDoc->LayerSet();
	SetText(IDC_ST_SOURCE2,CMapLayer::GetShortCoordSystemStr(lSet.Nad(),lSet.Zone()));
	SetText(IDC_ST_SOURCE,CMapLayer::GetShortCoordSystemStr(m_pShp->m_iNadOrg,m_pShp->m_iZoneOrg));
	//Enable(IDC_COORD_SOURCE,m_pShp->m_bConverted==true);
	//Enable(IDC_RETAIN_DELETED,m_pShp->ShpType()==CShpLayer::SHP_POINT);
	Enable(IDC_ADD_LAYER,!m_pShp->m_pDoc->m_bReadOnly);
	int typ=m_pShp->ShpTypeOrg();
	Enable(IDC_MAKE_2D,typ!=CShpLayer::SHP_POINT && typ!=CShpLayer::SHP_POLYGON && typ!=CShpLayer::SHP_POLYLINE);

	//if(m_bGenTemplate) Enable(IDC_GEN_TEMPLATE,FALSE); //force generation if m_pShp->HasXCFlds()?

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CExportShpDlg::OnBnClickedUseLayout()
{
	ASSERT(m_pShp->IsGridDlgOpen());
	if(!m_bUseLayout) {
		CString s;
		GetText(IDC_PATH_SHPDEF,s);
		if(!s.IsEmpty()) {
			if(IDOK!=AfxMessageBox(
				"You've already specified a file to be used as a template.\n"
				"Select OK to clear the edit box and allow use of the table layout.",MB_OKCANCEL))
			{
				SetCheck(IDC_USE_LAYOUT,FALSE);
				return;
			}
			SetText(IDC_PATH_SHPDEF,"");
		}
		m_bUseLayout=TRUE;
	}
	else m_bUseLayout=FALSE;
	Enable(IDC_BROWSE_SHPDEF,!m_bUseLayout);
	Enable(IDC_PATH_SHPDEF,!m_bUseLayout);
}

LRESULT CExportShpDlg::OnCommandHelp(WPARAM wNone, LPARAM lParam)
{
	AfxGetApp()->WinHelp(1003);
	return TRUE;
}

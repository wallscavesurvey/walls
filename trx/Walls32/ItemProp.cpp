// ItemProp.cpp : implementation file
//

#include "stdafx.h"
#include "walls.h"
#include "prjdoc.h"
#include "prjview.h"
#include "ItemProp.h"
#include "copyfile.h"
#include "DirDialog.h"
#include "itemprop.h"
#include "MagDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CItemProp

static UINT gen_id[]=	{12,IDOK,IDC_TITLE,IDC_NAME,IDC_PATH,IDC_SURVEY,IDC_SURVEYBOOK,
						IDC_FEETUNITS,IDC_METERUNITS,IDC_DEFINESEG,IDBROWSE,IDC_SURVEYNOT,IDCANCEL};

static UINT opt_id[]=	{13,IDOK,IDC_OPTIONS,IDC_NORTHEAST,IDC_NORTHWEST,IDC_NORTH,IDC_EAST,IDC_WEST,
						IDC_INHERIT_VIEW,IDC_INHERIT_LENGTH,IDC_PRESERVE_LENGTH,
						IDC_INHERIT_VERTICAL,IDC_PRESERVE_VERTICAL,IDCANCEL};

static UINT ref_id[]=	{9,IDOK,IDC_REF_INHERIT,IDC_REF_CLEAR,IDC_REF_CHANGE,
						IDC_DATE_METHOD,IDC_DATE_INHERIT,IDC_GRID_TYPE,IDC_GRID_INHERIT,
						IDCANCEL};

static UINT view_id[]=	{IDC_NORTHEAST,IDC_NORTHWEST,IDC_NORTH,IDC_EAST,IDC_WEST};

static UINT checkid(UINT id,UINT *p)
{
	for(UINT i=*p;i;i--) if(*++p==id) return id;
	//ASSERT(FALSE);
	return IDOK;
}

_inline CItemProp * CItemPage::Sheet()	{return STATIC_DOWNCAST(CItemProp,GetParent());}

IMPLEMENT_DYNAMIC(CItemProp, CPropertySheet)

BOOL hPropHook;
HWND hPropWnd;
CPrjView *pPropView;
CItemProp *pItemProp;

CItemProp::CItemProp(CPrjDoc *pDoc,CPrjListNode *pNode,BOOL bNew)
:CPropertySheet("", NULL, bNew?0:pDoc->m_nActivePage)
{
	m_psh.dwFlags |= PSH_NOAPPLYNOW;
	//propDlg.m_psh.dwFlags &= ~PSH_HASHELP; /*doesn't work!*/
	AddPage(&m_genDlg);
	AddPage(&m_optDlg);
	AddPage(&m_refDlg);
	m_bNew=bNew;
	m_pDoc=pDoc;
	m_pNode=pNode;
	m_pParent=pNode->Parent();
	m_uFlags=pNode->m_uFlags;
	m_genDlg.Init(this);
	m_optDlg.Init(this);
	m_refDlg.Init(this);
	hPropHook=FALSE;
	pItemProp=this;
	pPropView=m_pDoc->GetPrjView();
}

BEGIN_MESSAGE_MAP(CItemProp, CPropertySheet)
	ON_REGISTERED_MESSAGE(WM_PROPVIEWLB,OnPropViewLB)
	ON_REGISTERED_MESSAGE(WM_RETFOCUS,OnRetFocus)
	//{{AFX_MSG_MAP(CItemProp)
	ON_WM_SYSCOMMAND()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CItemProp message handlers

BOOL CItemProp::OnInitDialog() 
{
	BOOL bResult;

	if(!m_bNew) {
		m_bModeless = false;
		m_nFlags |= WF_CONTINUEMODAL;
 		bResult = CPropertySheet::OnInitDialog();
		m_nFlags &= ~WF_CONTINUEMODAL;
		m_bModeless = true;
	}
	else bResult = CPropertySheet::OnInitDialog();
	SetPropTitle();

	CWnd *pw=NULL;
	if(pw=GetDlgItem(IDHELP)) pw->EnableWindow(FALSE);
	if(pw=GetDlgItem(ID_APPLY_NOW)) pw->EnableWindow(FALSE);
	if(pw=GetDlgItem(IDCANCEL)) pw->EnableWindow(FALSE);
	if(pw=GetDlgItem(IDOK)) pw->EnableWindow(FALSE);
	
	CRect rectWnd;
	GetWindowRect(rectWnd);	
	SetWindowPos(&wndNoTopMost,0,0,rectWnd.Width(),
		rectWnd.Height()-32,
		SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
	CenterWindow();
	hPropWnd=m_hWnd;
	hPropHook=m_bNew?FALSE:TRUE;
	return bResult;
}

LRESULT CItemProp::OnRetFocus(WPARAM,LPARAM)
{
	VERIFY(SetFocus());
	return TRUE;
}

void CItemProp::SetPropTitle()
{
	if(m_bNew==2) SetWindowText("Properties: New Project");
	else {
		char szTitle[150];
		CPrjListNode *pNode=m_bNew?m_pNode->Parent():m_pNode;
		if(m_pNode->IsRoot()) _snprintf(szTitle,150,"Project properties: %s",pNode->Title()); 
		else {
			_snprintf(szTitle,150,m_bNew?"New item under %s: %s":"Properties for %s: %s",
				m_pDoc->m_pszName,pNode->Title());
			szTitle[15]=toupper(szTitle[15]);
		}
		SetWindowText(szTitle);
	}
}

//***7.1 LRESULT CItemProp::OnPropViewLB(CPrjDoc *pDoc,LPARAM)
LRESULT CItemProp::OnPropViewLB(WPARAM wParam,LPARAM)
{
	/*At this point, the object's variables have already been updated with
	  The control contents (UpdateData(TRUE) already called).
	  UpdateProperties() has *not* been called.*/

	CPrjListNode *pNode=((CPrjDoc *)wParam)->GetSelectedNode();

	if((CPrjDoc *)wParam!=m_pDoc || pNode!=m_pNode) {

		//Don't update old node if this is a move or copy to a new tree.
		//In this case, CPrjList::OnDrop() will have set m_pNode=NULL --
		if(m_pNode) {
			//Update if this is a local move or a *copy* to a new tree.
			pPropView->UpdateProperties(this);
		}

		//Init property dialog with new data.

		m_pDoc=(CPrjDoc *)wParam;
		m_pNode=pNode;
		m_pParent=pNode->Parent();
		pPropView=m_pDoc->GetPrjView();
		SetPropTitle();
	}
	else if(pNode->Parent()!=m_pParent) m_pParent=pNode->Parent();
	else return TRUE;

	m_uFlags=m_pNode->m_uFlags;
	m_genDlg.Init(this);
	m_optDlg.Init(this);
	m_refDlg.Init(this);
	GetActivePage()->UpdateData(FALSE);
	return TRUE;
}

void CItemProp::PostNcDestroy() 
{
	if(hPropHook) {
		hPropHook=FALSE;
		delete(this);
	}
}

void CItemProp::EndItemProp(int id)
{
	if(hPropHook) {
		hPropHook=FALSE;
		DestroyWindow();
		if(id==IDOK) pPropView->UpdateProperties(this);
		delete(this);
	}
	else {
		if(m_bNew && id==IDOK)
			pPropView->UpdateProperties(this);
		EndDialog(id);
	}
}

void CItemProp::OnSysCommand(UINT nID, LPARAM lParam) 
{
	WPARAM wCmdType = nID & 0xFFF0;

	if(wCmdType==SC_KEYMENU && !HIWORD(lParam)) {
		char mnemonic = (char)(DWORD)CharUpper((LPSTR)lParam);
		switch(mnemonic) {
			case 'G' :  SetActivePage(0);
						return;
			case 'C' :	SetActivePage(1);
						return;
			case 'R' :	SetActivePage(2);
						return;
		}
	}
	else if(nID==SC_CLOSE) {
		SetLastIndex();
		EndItemProp(IDCANCEL);
		return;
	}
	CPropertySheet::OnSysCommand(nID, lParam);
}


/////////////////////////////////////////////////////////////////////////////
// CItemPage property page

//IMPLEMENT_DYNCREATE(CItemPage, CPropertyPage)

//BOOL CItemPage::m_bChanged;
//BOOL CItemPage::m_bChangeAllow;

BEGIN_MESSAGE_MAP(CItemPage, CPropertyPage)
	ON_REGISTERED_MESSAGE(WM_RETFOCUS,OnRetFocus)
	//{{AFX_MSG_MAP(CItemPage)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CItemPage::OnSetActive() 
{
	BOOL bRet=CPropertyPage::OnSetActive();
	if(bRet) {
		PostMessage(WM_RETFOCUS);
	}
	return bRet;
}

LRESULT CItemPage::OnRetFocus(WPARAM,LPARAM)
{
	if(m_idLastFocus) GetDlgItem(m_idLastFocus)->SetFocus();
	else m_idLastFocus=checkid(GetFocus()->GetDlgCtrlID(),getids());
	return TRUE;
}

BOOL CItemPage::OnKillActive() 
{
	TRACE0("OnKillActive\n");
	m_idLastFocus=checkid(GetFocus()->GetDlgCtrlID(),getids());
	return UpdateData(TRUE) && (!hPropHook || ValidateData());
}

void CItemPage::OnCancel() 
{
	CItemProp *p=Sheet();
	p->SetLastIndex();
	p->EndItemProp(IDCANCEL);
}

void CItemPage::OnOK() 
{
	CItemProp *p=Sheet();
	if(UpdateData(TRUE)) {
		if(hPropHook && !ValidateData()) {
			PostMessage(WM_RETFOCUS);
			return;
		}
		p->SetLastIndex();
		p->EndItemProp(IDOK);
	}
}

/////////////////////////////////////////////////////////////////////////////
// CItemGeneral property page

IMPLEMENT_DYNCREATE(CItemGeneral, CPropertyPage)

UINT * CItemGeneral::getids() {return gen_id;} 

void CItemGeneral::Init(CItemProp *pSheet)
{
	CPrjListNode *pNode=pSheet->m_pNode;

	m_szTitle = pNode->m_Title;
	m_szName = pNode->m_Name;
	m_szPath = pNode->m_Path;
	m_bDefineSeg = pNode->IsDefineSeg();
	m_iLaunch=0;
	if(pNode->m_uFlags&FLG_LAUNCHOPEN) m_iLaunch=1;
	else if(pNode->m_uFlags&FLG_LAUNCHEDIT) m_iLaunch=2;
	if(pNode->IsLeaf()) {
		m_bOther=pNode->IsOther();
		m_bSurvey=!m_bOther;
		m_bBook=FALSE;
	}
	else {
		m_bBook=TRUE;
		m_bOther=m_bSurvey=FALSE;
	}
	m_bFeetUnits = pNode->IsFeetUnits();
	m_idLastFocus=0;
	m_bFilesHaveMoved=FALSE;
	m_bClickedReadonly=FALSE;
}

BOOL CItemGeneral::ValidateData()
{
	TRACE0("Validating Gen\n");
	if(hPropHook) hPropHook=TRUE;
	CItemProp *pSheet=Sheet();
	CPrjListNode *pNode=pSheet->m_pNode;
	CPrjDoc *pDoc=pSheet->m_pDoc;
	BOOL bNew=pSheet->m_bNew;

	//m_bChanged=FALSE;

	char pathbuf[_MAX_PATH];
	UINT idsErr=0;
	int	 len;
	int  idErr;
	
	if(m_szTitle.IsEmpty()) {
		idErr=IDC_TITLE;
		idsErr=IDS_PRJ_ERR_NOTITLE;
		goto _idsErr;
	}

	if(!m_szName.IsEmpty()) {

		//The name field is non-empty --
	
	    idErr=IDC_NAME;

		if((LPCSTR)trx_Stpnam(m_szName)!=(LPCSTR)m_szName) {
			idsErr=IDS_PRJ_ERR_NAMEPFX;
			goto _idsErr;
		}

		LPSTR p=trx_Stpext(strcpy(pathbuf,m_szName));
		//if(!m_bOther) _strupr(pathbuf);

		if(!m_bOther && !_stricmp(p,PRJ_PROJECT_EXTW)) {
			idsErr=IDS_PRJ_ERR_PRJEXT;
			goto _idsErr;
		}

		if(m_bBook && *p) {
			idsErr=IDS_PRJ_ERR_BASEEXT;
			goto _idsErr;
		}
		
		len=p-pathbuf;
		if(!len) {
			idsErr=IDS_PRJ_ERR_BADNAME;
			goto _idsErr;
		}

		if(len>LEN_BASENAME && !m_bOther) {
			idsErr=IDS_PRJ_ERR_BASELEN;
			goto _idsErr;
		}

		//Survey files should have extension .SRV --
		if(m_bSurvey && *p) {
			if(_stricmp(p,PRJ_SURVEY_EXT)) {
				if(IDYES!=CMsgBox(MB_YESNO,IDS_PRJ_NOTSURVEY)) {
					idsErr=IDS_PRJ_ERR_BADNAME1;
					goto _idsErr;
				}
			}
			*p=0;
		}
		
		//Check for base name compatibility --
		//bNew = 0 (old), 1 (new), 2 (new root)

		//m_bChanged=_stricmp(pathbuf,pNode->BaseName())!=0;
		
		if(bNew<2 && !m_bOther && (bNew || pNode->IsOther() || _stricmp(pathbuf,pNode->BaseName()))) {
			//Validity check on name is required --
			if(pDoc->m_pRoot->FindName(pathbuf)) {
			   CMsgBox(MB_ICONEXCLAMATION,idsErr=IDS_PRJ_ERR_BADNAME1,pathbuf);
			   goto _idsErr;
			}
		}

		if(!bNew && !m_bBook && !m_szPath.CompareNoCase(pNode->m_Path)) {
			if(pNode->NameEmpty()) goto _idsErr;
			if(m_bSurvey && !*p) strcpy(p,PRJ_SURVEY_EXT);

			if(stricmp(pathbuf,trx_Stpnam(p=pDoc->SurveyPath(pNode)))) {
				//No change in path prefix but the file name is different.
				//Let's allow renaming of an existing file --

				//m_bChanged++;

				//Test R/W access --
				if((len=_access(p,6))==0 || errno==EACCES) {

					if(CLineDoc::GetLineDoc(p)) {
						//"This file is currently being edited. You must close
						//its edit window before changing its name or path property." (Why not allow this?)
						CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_SURVOPEN1,p);
						m_idLastFocus=idErr;
						return FALSE;       // don't continue
					}

					strcpy(pathbuf,p); //pathbuf - original complete pathname

					switch(CMsgBox(MB_YESNOCANCEL,IDS_PRJ_SRVRENAME1,p)) {
						/*"A file with the original name exists. Would you like to rename
						  it as opposed to associating a different file?
						  Select NO to associate a new file while not deleting the old one."*/

						case IDCANCEL:
							m_idLastFocus=idErr;
							return FALSE;       // don't continue

						case IDYES:
							//Rename the existing file. If len!=0 it's write protected or inaccessible!
							p=trx_Stpext(strcpy(trx_Stpnam(pathbuf),m_szName));
							if(m_bSurvey && !*p) strcpy(p,PRJ_SURVEY_EXT);

							//Now pathbuf contains the new name while CPrjDoc::m_pathBuf
							//still has the original pathname --

							if(len) {
								//Current file is read-only, let's try removing the protection
								if(_chmod(CPrjDoc::m_pathBuf,_S_IWRITE|_S_IREAD)) {
									//The system won't allow temporary removal of write protection
									//on this file. It can't be renamed.
									CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_CHMODRW1,CPrjDoc::m_pathBuf);
									m_idLastFocus=idErr;
									return FALSE; // don't continue
								}
							}
							if(rename(CPrjDoc::m_pathBuf,pathbuf)) {
								//pathbuf could be the name of an "other" file or of a file
								//not associated with the project. Let's check if that file is open.

								if(CLineDoc::GetLineDoc(pathbuf)) {
									//"The new name is that of an existing file you currently have open.
									//You must close it before it can be replaced or attached to this project."
									CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_DELETEOPEN,pathbuf);
									idsErr=IDS_PRJ_ERR_BADNAME1;
								}
								else
								//"The new name matches that of an existing file.
								//Choose OK to replace that file with the one you've chosen to rename.
								//CAUTION: This operation will discard all data in the file being replaced."
								if(IDOK==CMsgBox(MB_OKCANCEL,IDS_PRJ_DELETEORG1,pathbuf)) {
									if(_unlink(pathbuf) || rename(CPrjDoc::m_pathBuf,pathbuf)) {
										//"We could not rename the current file to %s,
										//probably because the existing file with that name
										//is protected and can't be deleted."
										CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_BADRENAME1,pathbuf);
										idsErr=IDS_PRJ_ERR_BADNAME1;
									}
								}
								else idsErr=IDS_PRJ_ERR_BADNAME1;
							}
							if(len) {
								//Keep this file item read-only --
								VERIFY(_chmod(idsErr?CPrjDoc::m_pathBuf:pathbuf,_S_IREAD)==0);
							}
							break;

						case IDNO:
							// Accept name change without renaming ==
							break;
					}
				} /*else file doesn't exist*/

			} /*else pathname same after expansion*/

			goto _idsErr;

		} /*else: book or new or changed path*/

	} /*Name not empty*/
	
    m_szPath.Replace('/','\\');

	if(m_szPath.CompareNoCase(pNode->m_Path)) {

		//m_bChanged++;

		idErr=IDC_PATH;
		len=m_szPath.GetLength();

		//First, store absolute version in pathbuf --
		if(len && IsAbsolutePath(m_szPath)) {
			strcpy(pathbuf,m_szPath);
			//InsertDriveLetter(pathbuf,pDoc->m_pPath);
			strcat(pathbuf,"\\");
		}
		else {
			pDoc->GetPathPrefix(pathbuf,pNode->Parent());
			if(len) {
			   strcat(pathbuf,m_szPath);
			   strcat(pathbuf,"\\");
			}
		}
		
		//get old path in CPrjDoc::m_pathBuf --
		*trx_Stpnam(pDoc->SurveyPath(pNode))=0;

		if(!_stricmp(CPrjDoc::m_pathBuf,pathbuf)) goto _idsErr;

        //A new path was specified.
		//m_bChanged++;
		if((m_bBook || !m_szName.CompareNoCase(pNode->m_Name)) && pNode->HasDependentPaths(pDoc)) {
			   
			   if(IDOK != CMsgBox(MB_OKCANCEL,IDS_PRJ_COPYITEM)) {
					idsErr=IDS_PRJ_ERR_BADNAME1;
					goto _idsErr;
			   }
			   if(hPropHook) hPropHook++;
			   //Be sure to close all open files before moving them!
			   CLineDoc::SaveAllOpenFiles();
			   bMoveFile=TRUE;
			   pDoc->CopyBranchSurveys(pNode,pathbuf,1);
			   m_bFilesHaveMoved=TRUE;
		}
		else if(len) {
			if(!(len=DirCheck(pathbuf,TRUE)))  idsErr=IDS_PRJ_ERR_BADNAME1;
			else if(len!=TRUE && hPropHook) hPropHook++;
		}
	}
		
_idsErr:	    
	if(idsErr) {
	    if(idsErr!=IDS_PRJ_ERR_BADNAME1) AfxMessageBox(idsErr);
		m_idLastFocus=idErr;
		return FALSE;
	}
	//if(m_bClickedReadonly && !m_bBook) { ????
	//	idErr=0;
	//}
	return TRUE;
}

void CItemGeneral::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);

	//{{AFX_DATA_MAP(CItemGeneral)
	DDX_Text(pDX, IDC_NAME, m_szName);
	DDV_MaxChars(pDX, m_szName, 128);
	DDX_Text(pDX, IDC_TITLE, m_szTitle);
	DDV_MaxChars(pDX, m_szTitle, 128);
	DDX_Check(pDX, IDC_DEFINESEG, m_bDefineSeg);
	DDX_Text(pDX, IDC_PATH, m_szPath);
	DDV_MaxChars(pDX,m_szPath,128);
	//}}AFX_DATA_MAP

	DDX_Radio(pDX,IDC_LAUNCHPROMPT,m_iLaunch);

	if(!pDX->m_bSaveAndValidate) {
		Check(IDC_SURVEY,m_bSurvey);
		Check(IDC_SURVEYBOOK,m_bBook);
		Check(IDC_SURVEYNOT,m_bOther);
		Check(IDC_FEETUNITS,m_bFeetUnits);
		Check(IDC_METERUNITS,!m_bFeetUnits);
		m_ePath.Init(this,IDC_PATH,IDC_PATHPREFIX);
		UpdateGenCtrls();
	}
	else {
		ASSERT(m_bOther+m_bSurvey+m_bBook==1);
		m_szTitle.Trim();
	    m_szName.Trim();
		m_szPath.Trim();
		m_bFeetUnits=IsDlgButtonChecked(IDC_FEETUNITS)!=0;
		m_bDefineSeg=IsDlgButtonChecked(IDC_DEFINESEG)!=0;

		int len=m_bFeetUnits*FLG_FEETUNITS+m_bDefineSeg*FLG_DEFINESEG+m_bOther*FLG_SURVEYNOT;
		if(m_iLaunch==1) len+=FLG_LAUNCHOPEN;
		else if(m_iLaunch==2) len+=FLG_LAUNCHEDIT;
		Sheet()->m_uFlags &= ~FLG_MASK_GENERAL;
		Sheet()->m_uFlags |= len;

		if(!m_bOther) {
			m_szName.MakeUpper();
			if(m_bSurvey) {
				char * pExt=trx_Stpext(m_szName);
				if(*pExt && !strcmp(pExt,PRJ_SURVEY_EXT)) {
					m_szName.Delete(pExt-(LPCSTR)m_szName,4);
				}
			}
		}

		len=m_szPath.GetLength();
		if(len>1 && m_szPath[len-1]=='\\') m_szPath.Delete(--len,1);

		if(!hPropHook && !ValidateData()) {
			pDX->m_idLastControl=m_idLastFocus;
			//***7.1 pDX->m_hWndLastControl=GetDlgItem(m_idLastFocus)->m_hWnd;
			pDX->m_bEditLastControl=TRUE;
			pDX->Fail();
		}
	}
}

BEGIN_MESSAGE_MAP(CItemGeneral, CItemPage)
    ON_MESSAGE(WM_COMMANDHELP,OnCommandHelp)
	//{{AFX_MSG_MAP(CItemGeneral)
	ON_BN_CLICKED(IDC_SURVEY, OnSurvey)
	ON_BN_CLICKED(IDBROWSE, OnBrowse)
	ON_WM_CTLCOLOR()
	ON_BN_CLICKED(IDC_SURVEYBOOK, OnSurvey)
	ON_BN_CLICKED(IDC_SURVEYNOT, OnSurvey)
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_READONLY, OnBnClickedReadonly)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CItemGeneral message handlers
static char *pSurveyText="Name:";
static char *pOtherText= "File:";

void CItemGeneral::UpdateSurveyOther()
{
    ASSERT(m_bBook+m_bSurvey+m_bOther==1);
	SetDlgItemText(IDC_NAMETEXT,m_bOther?pOtherText:pSurveyText);
	Show(IDC_ST_REVIEW,!m_bOther);
	Show(IDC_FEETUNITS,!m_bOther);
	Show(IDC_METERUNITS,!m_bOther);
	Show(IDC_DEFINESEG,!m_bOther);
	Show(IDC_ST_LAUNCH,m_bOther);
	Show(IDC_LAUNCHPROMPT,m_bOther);
	Show(IDC_LAUNCHEDIT,m_bOther);
	Show(IDC_LAUNCHOPEN,m_bOther);
}

void CItemGeneral::OnSurvey() 
{
   
   m_bSurvey=(IsDlgButtonChecked(IDC_SURVEY)!=0);
   if(m_bBook!=(IsDlgButtonChecked(IDC_SURVEYBOOK)!=0)) {
	   m_bBook=!m_bBook;
	   Show(IDC_READONLY,!m_bBook);
   }
   if(m_bOther!=(IsDlgButtonChecked(IDC_SURVEYNOT)!=0)) {
	    m_bOther=!m_bOther;
		UpdateSurveyOther();
   }
   HWND hw=Sheet()->m_optDlg.m_hWnd;
   if(::IsWindow(hw)) ::EnableWindow(::GetDlgItem(hw,IDC_SVG_PROCESS),m_bBook);
}

void CItemGeneral::OnBrowse() 
{
	char pathbuf[_MAX_PATH];
	CString Path;
	CEdit *pw=(CEdit *)GetDlgItem(IDC_PATH);
	BOOL bLongName=FALSE;
	char *p;

	if(m_bBook) {
		CDirDialog dlg;
		Sheet()->m_pDoc->GetPathPrefix(pathbuf,Sheet()->m_pNode->Parent());
		pathbuf[strlen(pathbuf)-1]=0;
		dlg.m_strSelDir=pathbuf;
		GetDlgItem(IDC_TITLE)->GetWindowText(Path);
		LoadCMsg(pathbuf,sizeof(pathbuf),IDS_PRJ_SELDIR,Path.IsEmpty()?"this branch":(LPCSTR)Path);
		dlg.m_strTitle=pathbuf;
		if(!dlg.DoBrowse(m_hWnd)) {
			pw->SetSel(0x7FFF,0x7FFF);
			pw->SetFocus();
			return;
		}
		strcpy(pathbuf,dlg.m_strPath);
		if(*pathbuf && pathbuf[strlen(pathbuf)-1]!='\\') strcat(pathbuf,"\\");
	}
	else {
		CString strFilter;
		Path=Sheet()->m_pDoc->GetPathPrefix(pathbuf,Sheet()->m_pNode);

		if(m_bSurvey && !AddFilter(strFilter,IDS_SRV_FILES)) return;
		if(!AddFilter(strFilter,AFX_IDS_ALLFILTER)) return;
		
		if(!DoPromptPathName(Path,OFN_FILEMUSTEXIST,
			m_bSurvey?2:1,strFilter,TRUE,
			m_bSurvey?IDS_ITMBROWSE:IDS_OTHERBROWSE,
			m_bSurvey?PRJ_SURVEY_EXT:NULL)) return;

		strcpy(pathbuf,Path);
	}

	Sheet()->m_pDoc->GetRelativePathName(pathbuf,Sheet()->m_pNode->Parent());

	p=trx_Stpnam(pathbuf);
	char *pExt=trx_Stpext(p);

	if(p>pathbuf) {
		ASSERT(p[-1]=='\\');
		p[-1]=0;
		m_szPath=pathbuf;
	}
	else m_szPath.Empty();

	pw->SetWindowText(m_szPath);

	if(m_bBook) {
		pw->SetSel(0x7FFF,0x7FFF);
		pw->SetFocus();
		return;
	}

	GetDlgItem(IDC_NAME)->SetWindowText(p);
	pw=(CEdit *)GetDlgItem(IDC_TITLE);

	if(m_bSurvey && !_stricmp(pExt,PRJ_SURVEY_EXT)) *pExt=0;

	if(m_bOther || (pExt-p)>LEN_BASENAME) {
		pw->SetWindowText(p);
		if(m_bSurvey) {
			Check(IDC_SURVEY,m_bSurvey=FALSE);
			Check(IDC_SURVEYNOT,m_bOther=TRUE);
			UpdateSurveyOther();
			if(!*pExt) {
				strcpy(pExt,PRJ_SURVEY_EXT);
				GetDlgItem(IDC_NAME)->SetWindowText(p);
			}
			CMsgBox(MB_ICONINFORMATION,IDS_PRJ_LONGNOTIFY,p);
		}
	}
	else {
	  CString csTitle;
	  pw->GetWindowText(csTitle);
	  csTitle.Trim();
	  if(csTitle.IsEmpty()) {
		  //If first line is a comment, use it as title --
		  FILE *fp=NULL;
		  if((fp=fopen(Path,"r")) && fgets(pathbuf,128,fp) && *pathbuf==';') {
			int len=strlen(pathbuf)-1;
			if(pathbuf[len]=='\n') pathbuf[len]=0;
			csTitle.SetString(pathbuf+1);
			csTitle.Trim();
			pw->SetWindowText(csTitle);
		  }
		  if(fp) fclose(fp);
	  }
	}

	Sheet()->m_pDoc->GetPathPrefix(pathbuf,Sheet()->m_pNode->Parent());
	GetDlgItem(IDC_NAME)->GetWindowText(m_szName);
	UpdateSizeDate(pathbuf);

	pw->SetSel(0,-1);
	pw->SetFocus();
}

LRESULT CItemGeneral::OnCommandHelp(WPARAM wNone, LPARAM lParam)
{
	AfxGetApp()->WinHelp(6,HELP_CONTEXT);
	return TRUE;
}

void CItemGeneral::UpdateSizeDate(char *pathbuf)
{
	GetDlgItem(IDC_SIZEDATE)->SetWindowText("");
	BOOL bReadOnly=!m_bBook;

	Show(IDC_READONLY,m_bReadOnly=bReadOnly);

	if(bReadOnly) {
		long filesize;
		SYSTEMTIME *ptm;
		if(!m_szPath.IsEmpty()) {
		   if(m_ePath.m_bAbsolute) strcpy(pathbuf,m_szPath);
		   else strcat(pathbuf,m_szPath);
		   if(pathbuf[1]) strcat(pathbuf,"\\");
		}
		strcat(pathbuf,m_szName);
		if(m_bSurvey && !*trx_Stpext(m_szName)) strcat(pathbuf,PRJ_SURVEY_EXT);
		Enable(IDC_READONLY,TRUE);
		if(ptm=GetLocalFileTime(pathbuf,&filesize,&bReadOnly)) {
		   sprintf(pathbuf,"Last updated: %s",GetTimeStr(ptm,&filesize));
		   GetDlgItem(IDC_SIZEDATE)->SetWindowText(pathbuf);
		   Check(IDC_READONLY,m_bReadOnly=bReadOnly);
		}
		else {
		   //File doesn't exist yet --
		   Check(IDC_READONLY,m_bReadOnly=FALSE);
		   Enable(IDC_READONLY,FALSE);
		}
	}
}

void CItemGeneral::UpdateGenCtrls(void)
{
	CItemProp *pSheet=Sheet();
	CPrjListNode *pNode=pSheet->m_pNode;
	BOOL bNew=pSheet->m_bNew;
	char pathbuf[_MAX_PATH];
	
	//Disable unmodifiable items --
	BOOL bEnable = bNew!=2 && (bNew || (!pNode->FirstChild() && !pNode->IsRoot()));
	Enable(IDC_SURVEY,bEnable);
	Enable(IDC_SURVEYBOOK,bEnable);
	Enable(IDC_SURVEYNOT,bEnable);
	//bEnable=bEnable || (pNode->IsRoot() && !strcmp(pNode->Name(),"PROJECT"));
	//GetDlgItem(IDC_NAME)->EnableWindow(bEnable);
	  
	UpdateSurveyOther();

	Enable(IDC_DEFINESEG,!pNode->IsRoot());

	Sheet()->m_pDoc->GetPathPrefix(pathbuf,pNode->Parent());
	GetDlgItem(IDC_PATHPREFIX)->SetWindowText(TrimPath(CString(pathbuf),50));
	m_ePath.OnChange();

	UpdateSizeDate(pathbuf);
}

BOOL CItemGeneral::OnInitDialog() 
{
	CItemPage::OnInitDialog();
	CPrjListNode *pNode=Sheet()->m_pNode;
	
	//New items require a title, otherwise we probably want to change the name if
	//this field is empty or this is a leaf with no existing file
	
    m_idLastFocus=(!Sheet()->m_bNew && (m_szName.IsEmpty() || (pNode->IsLeaf() &&
		!pNode->m_dwFileChk)))?IDC_NAME:IDC_TITLE;
	((CEdit *)GetDlgItem(m_idLastFocus))->SetSel(0,-1);
	
	return TRUE; // return TRUE unless you set the focus to a control
}

/////////////////////////////////////////////////////////////////////////////
// CItemReference property page

IMPLEMENT_DYNCREATE(CItemReference, CPropertyPage)

UINT * CItemReference::getids() {return ref_id;}
 
void CItemReference::Init(CItemProp *pSheet)
{
	CPrjListNode *pNode=pSheet->m_pNode;
	UINT flags=pSheet->m_uFlags;

	m_pParent=pNode->Parent();

	m_bRefInherit = m_pParent?((flags&FLG_REFMASK)==0):FALSE;
	m_bRefClear = !pNode->InheritedState(FLG_REFMASK);
	m_Ref=*pNode->GetDefinedRef();

	CMainFrame::CheckDatumName(&m_Ref);

	m_bGridInherit = m_pParent?((flags&FLG_GRIDMASK)==0):FALSE; 
	m_bGridType = pNode->InheritedState(FLG_GRIDMASK);

	m_bDateInherit = m_pParent?((flags&FLG_DATEMASK)==0):FALSE;
	m_bDateMethod = pNode->InheritedState(FLG_DATEMASK);
	m_idLastFocus=0;
}

BOOL CItemReference::ValidateData()
{
	if(hPropHook) hPropHook=TRUE;
	if(!m_bRefInherit && !m_bRefClear) {
		char buf1[20];
		char buf2[20];
		if(!GetText(IDC_CONV,buf1,20)) *buf1=0;
		_snprintf(buf2,30,"%.4f",m_Ref.conv);
		if(strcmp(buf1,buf2)) {
			double f=atof(buf1);
			if(f>-180.0 && f<=180.0) m_Ref.conv=(float)f;
			else {
			   AfxMessageBox(IDS_ERR_CONVRANGE);
			   m_idLastFocus=IDC_CONV;
			   if(hPropHook) hPropHook++;
			   return FALSE;
			}
		}
	}
	return TRUE;
}

void CItemReference::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CItemReference)
	DDX_Check(pDX, IDC_DATE_INHERIT, m_bDateInherit);
	DDX_Check(pDX, IDC_DATE_METHOD, m_bDateMethod);
	DDX_Check(pDX, IDC_GRID_INHERIT, m_bGridInherit);
	DDX_Check(pDX, IDC_GRID_TYPE, m_bGridType);
	DDX_Check(pDX, IDC_REF_INHERIT, m_bRefInherit);
	DDX_Check(pDX, IDC_REF_CLEAR, m_bRefClear);
	//}}AFX_DATA_MAP

	if(pDX->m_bSaveAndValidate) {
		UINT flags=0;
		Sheet()->m_uFlags &= ~FLG_MASK_REFERENCE;
		if(!m_bDateInherit)		flags |= (m_bDateMethod?FLG_DATECHK:FLG_DATEUNCHK);
		if(!m_bGridInherit)		flags |= (m_bGridType?FLG_GRIDCHK:FLG_GRIDUNCHK);
		if(!m_bRefInherit) 		flags |= (m_bRefClear?FLG_REFUSEUNCHK:FLG_REFUSECHK);
		Sheet()->m_uFlags |= flags;

		if(!hPropHook && !ValidateData()) {
			pDX->m_idLastControl=m_idLastFocus;
			//***7.1 pDX->m_hWndLastControl=GetDlgItem(m_idLastFocus)->m_hWnd;
			pDX->m_bEditLastControl=TRUE;
			pDX->Fail();
		}
	}
	else {
		if(!m_idLastFocus) {
			Enable(IDC_DATE_INHERIT,m_pParent!=NULL && !m_bRefClear);
			Enable(IDC_GRID_INHERIT,m_pParent!=NULL && !m_bRefClear);
			Enable(IDC_REF_INHERIT,m_pParent!=NULL);
			Enable(IDC_DATE_METHOD,!m_bRefClear && (!m_pParent || !m_bDateInherit));
			Enable(IDC_GRID_TYPE,!m_bRefClear && (!m_pParent || !m_bGridInherit));
			Enable(IDC_REF_CLEAR,!m_bRefInherit);
			Enable(IDC_REF_CHANGE,!m_bRefInherit);
			m_bConvUpdated=FALSE;
			UpdateRefCtrls();
		}
	}
}

BEGIN_MESSAGE_MAP(CItemReference, CItemPage)
    ON_MESSAGE(WM_COMMANDHELP,OnCommandHelp)
	//{{AFX_MSG_MAP(CItemReference)
	ON_WM_CTLCOLOR()
	ON_BN_CLICKED(IDC_DATE_INHERIT, OnDateInherit)
	ON_BN_CLICKED(IDC_GRID_INHERIT, OnGridInherit)
	ON_BN_CLICKED(IDC_REF_CHANGE, OnRefChange)
	ON_BN_CLICKED(IDC_REF_CLEAR, OnRefClear)
	ON_BN_CLICKED(IDC_REF_INHERIT, OnRefInherit)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CItemReference message handlers


BOOL CItemReference::OnInitDialog() 
{
	CItemPage::OnInitDialog();
	
    m_idLastFocus=IDCANCEL;
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

LRESULT CItemReference::OnCommandHelp(WPARAM wNone, LPARAM lParam)
{
	AfxGetApp()->WinHelp(119,HELP_CONTEXT);
	return TRUE;
}

HBRUSH CItemReference::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
	HBRUSH hbr = CPropertyPage::OnCtlColor(pDC, pWnd, nCtlColor);
    if(nCtlColor==CTLCOLOR_STATIC || nCtlColor==CTLCOLOR_EDIT) {
		switch(pWnd->GetDlgCtrlID()) {
			case IDC_ST_UTM:
			case IDC_ST_LATLON:
			case IDC_ST_DATUM:
			case IDC_ST_ELEV:
			case IDC_CONV:
			pDC->SetTextColor(RGB_DKRED);
		}
    }
	//else if(nCtlColor==CTLCOLOR_EDIT && pWnd->GetDlgCtrlID()==IDC_CONV) {
	//}
	return hbr;
}

void CItemReference::OnDateInherit() 
{
	if(m_bDateInherit=!m_bDateInherit)
		Check(IDC_DATE_METHOD,m_bDateMethod=m_pParent->InheritedState(FLG_DATEMASK));
	Enable(IDC_DATE_METHOD,!m_bDateInherit);
}

void CItemReference::OnGridInherit() 
{
	if(m_bGridInherit=!m_bGridInherit)
		Check(IDC_GRID_TYPE,m_bGridType=m_pParent->InheritedState(FLG_GRIDMASK));
	Enable(IDC_GRID_TYPE,!m_bGridInherit);
}

void CItemReference::UpdateRefCtrls()
{
	char buf[40];

	CPrjListNode *pNode=m_bRefInherit?m_pParent->GetRefParent():Sheet()->m_pNode;
	SetText(IDC_ST_TITLE,pNode->Title());

	if(m_bRefClear) {
		char *p="";
		SetText(IDC_ST_UTM,p);
		SetText(IDC_ST_LATLON,p);
		SetText(IDC_ST_DATUM,p);
		Enable(IDC_CONV,TRUE);
		SetText(IDC_CONV,p);
		Enable(IDC_CONV,FALSE);
		m_bConvUpdated=FALSE;
		SetText(IDC_ST_ELEV,p);
	}
	else {
		char latdir=(m_Ref.flags&REF_LATDIR)?'S':'N';
		char londir=(m_Ref.flags&REF_LONDIR)?'W':'E';
		double abslat=m_Ref.latdeg+(m_Ref.latmin*60.0+m_Ref.latsec)/3600;
		switch(m_Ref.flags&REF_FMTMASK) {
			case 0 :
					sprintf(buf,"%.7f\xB0 %c,  %.7f\xB0 %c",
					abslat, latdir,
					m_Ref.londeg+(m_Ref.lonmin*60.0+m_Ref.lonsec)/3600, londir);
				break;
			case 1 :
					sprintf(buf,"%u\xB0 %.5f' %c,  %u\xB0 %.5f' %c",
					m_Ref.latdeg,(m_Ref.latmin+m_Ref.latsec/60.0), latdir,
					m_Ref.londeg,(m_Ref.lonmin+m_Ref.lonsec/60.0), londir);
				break;
			case 2 :
				sprintf(buf,"%u\xB0 %u' %.3f\" %c,  %u\xB0 %u' %.3f\" %c",
					m_Ref.latdeg,m_Ref.latmin,m_Ref.latsec, latdir,
					m_Ref.londeg,m_Ref.lonmin,m_Ref.lonsec, londir);
				break;
		}
		SetText(IDC_ST_LATLON,buf);
		LPSTR p=ZoneStr(m_Ref.zone);
		p[3]=0;
		sprintf(buf,"%s: ",p);
		SetText(IDC_ST_UTMUPS,buf);
		sprintf(buf,"%s  %.2f E,  %.2f N",p+4,m_Ref.east,m_Ref.north);
		SetText(IDC_ST_UTM,buf);

		Enable(IDC_CONV,TRUE);
		if(!m_bConvUpdated) {
			sprintf(buf,"%.3f",m_Ref.conv);
			SetText(IDC_CONV,buf);
			m_bConvUpdated=TRUE;
		}
		Enable(IDC_CONV,!m_bRefInherit);
		sprintf(buf,"%d m",m_Ref.elev);
		SetText(IDC_ST_ELEV,buf);
		SetText(IDC_ST_DATUM,m_Ref.datumname);
	}
}

void CItemReference::OnRefInherit() 
{
	m_bRefInherit=!m_bRefInherit;

	if(m_bRefInherit) {
		BOOL bNewClear=!m_pParent->InheritedState(FLG_REFMASK);
		Check(IDC_REF_CLEAR,bNewClear);
		Enable(IDC_REF_CLEAR,FALSE);
		if(!bNewClear) {
			m_Ref=*m_pParent->GetDefinedRef();
			m_bConvUpdated=FALSE;
		}
		if(m_bRefClear!=bNewClear) {
			OnRefClear();
			return;
		}
	}
	else {
		//No longer inheriting reference --
		Enable(IDC_REF_CLEAR,TRUE);
	}
	Enable(IDC_REF_CHANGE,!m_bRefInherit);
	UpdateRefCtrls();
}

void CItemReference::OnRefClear() 
{
	m_bRefClear=!m_bRefClear;

	ASSERT(m_pParent || (!m_bDateInherit && !m_bGridInherit));

	if(m_bRefClear) {
		if(m_bDateMethod || m_bGridType) {
			AfxMessageBox(IDS_ERR_NOGEOREF);
		}

		if(m_pParent) {
			Enable(IDC_DATE_INHERIT,TRUE);
			Check(IDC_DATE_INHERIT,m_bDateInherit=FALSE);
			Enable(IDC_DATE_INHERIT,FALSE);

			Enable(IDC_GRID_INHERIT,TRUE);
			Check(IDC_GRID_INHERIT,m_bGridInherit=FALSE);
			Enable(IDC_GRID_INHERIT,FALSE);
		}

		if(m_bDateMethod) {
			Enable(IDC_DATE_METHOD,TRUE);
			Check(IDC_DATE_METHOD,m_bDateMethod=FALSE);
		}
		Enable(IDC_DATE_METHOD,FALSE);

		if(m_bGridType) {
			Enable(IDC_GRID_TYPE,TRUE);
			Check(IDC_GRID_TYPE,m_bGridType=FALSE);
		}
		Enable(IDC_GRID_TYPE,FALSE);
	}
	else {
		if(m_pParent) {
			Enable(IDC_DATE_INHERIT,TRUE);
			Enable(IDC_GRID_INHERIT,TRUE);
		}

		Enable(IDC_DATE_METHOD,TRUE);
		if(m_bDateInherit) {
			Check(IDC_DATE_METHOD,m_bDateMethod=m_pParent->InheritedState(FLG_DATEMASK));
			Enable(IDC_DATE_METHOD,FALSE);
		}

		Enable(IDC_GRID_TYPE,TRUE);
		if(m_bGridInherit) {
			Check(IDC_GRID_TYPE,m_bGridType=m_pParent->InheritedState(FLG_GRIDMASK));
			Enable(IDC_GRID_TYPE,FALSE);
		}
		m_bConvUpdated=FALSE;
	}
	//Enable(IDC_REF_CHANGE,!m_bRefClear);
	UpdateRefCtrls();
}

void CItemReference::OnRefChange() 
{
	if(app_pMagDlg) app_pMagDlg->DestroyWindow();
	CMainFrame::InitRef(&m_Ref);
	app_pMagDlg=new CMagDlg();
	if(app_pMagDlg) {
		char buf[100];
		_snprintf(buf,100,"Reference: %s",Sheet()->m_pNode->Title());
		app_pMagDlg->m_pTitle=buf;
		if(app_pMagDlg->DoModal()==IDOK) {
			MD=app_pMagDlg->m_MD;
			m_bConvUpdated=FALSE;
			CMainFrame::GetRef(&m_Ref);
			MF(MAG_GETDATUMNAME);
			strcpy(m_Ref.datumname,MD.modname);
			ASSERT(m_Ref.datum<=REF_MAXDATUM);
			if(m_bRefClear) {
				Check(IDC_REF_CLEAR,FALSE);
				OnRefClear();
			}
			else UpdateRefCtrls();
		}
		delete app_pMagDlg; //will null the pointer
	}
}

/////////////////////////////////////////////////////////////////////////////
// CItemOptions property page

IMPLEMENT_DYNCREATE(CItemOptions, CPropertyPage)

UINT * CItemOptions::getids() {return opt_id;}
 
CItemOptions::CItemOptions() : CItemPage(CItemOptions::IDD)
{
	//{{AFX_DATA_INIT(CItemOptions)
	m_bSVG_process = FALSE;
	//}}AFX_DATA_INIT
}

BOOL CItemOptions::OnInitDialog() 
{
	CItemPage::OnInitDialog();
	//Enable(IDC_SVG_PROCESS,!Sheet()->m_pNode->IsLeaf());
    m_idLastFocus=IDC_OPTIONS;
	//((CEdit *)GetDlgItem(m_idLastFocus))->SetSel(0,-1);
	
	return TRUE; // return TRUE unless you set the focus to a control
}

void CItemOptions::Init(CItemProp *pSheet)
{
	CPrjListNode *pNode=pSheet->m_pNode;
	m_szOptions = pNode->m_Options;

	UINT flags=pSheet->m_uFlags;
	m_bSVG_process = (flags&FLG_SVG_PROCESS)!=0;
	m_bPreserveLength = pNode->InheritedState(FLG_VTLENMASK);
	m_bPreserveVertical = pNode->InheritedState(FLG_VTVERTMASK);
	m_iNorthEast = pNode->InheritedView();
	pNode=pNode->Parent();
	m_bInheritLength = pNode?((flags&FLG_VTLENMASK)==0):FALSE; 
	m_bInheritVertical = pNode?((flags&FLG_VTVERTMASK)==0):FALSE;
	m_bInheritView = pNode?((flags&FLG_VIEWMASK)==0):FALSE;
	m_idLastFocus=0;
}

void CItemOptions::EnableView(BOOL bEnable)
{
	Enable(IDC_NORTHEAST,bEnable);
	Enable(IDC_NORTHWEST,bEnable);
	Enable(IDC_NORTH,bEnable);
	Enable(IDC_EAST,bEnable);
	Enable(IDC_WEST,bEnable);
}

BOOL CItemOptions::ValidateData()
{
	if(hPropHook) hPropHook=TRUE;
	return TRUE;
}

void CItemOptions::InsertComboOption(CPrjListNode *pNode)
{
	if(pNode) {
		m_DisCombo.InsertString(-1,pNode->m_Name+": "+pNode->m_Options);
		InsertComboOption(pNode->Parent());
	}
}

void CItemOptions::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);

	//if(!pDX->m_bSaveAndValidate) m_DisCombo.ResetContent();

	//{{AFX_DATA_MAP(CItemOptions)
	DDX_Control(pDX, IDC_OPTIONS, m_DisCombo);
	DDX_Text(pDX, IDC_OPTIONS, m_szOptions);
	DDV_MaxChars(pDX, m_szOptions, 256);
	DDX_Check(pDX, IDC_INHERIT_LENGTH, m_bInheritLength);
	DDX_Check(pDX, IDC_INHERIT_VERTICAL, m_bInheritVertical);
	DDX_Check(pDX, IDC_PRESERVE_LENGTH, m_bPreserveLength);
	DDX_Check(pDX, IDC_PRESERVE_VERTICAL, m_bPreserveVertical);
	DDX_Check(pDX, IDC_INHERIT_VIEW, m_bInheritView);
	DDX_Radio(pDX, IDC_NORTHEAST, m_iNorthEast);
	DDX_Check(pDX, IDC_SVG_PROCESS, m_bSVG_process);
	//}}AFX_DATA_MAP

	if(pDX->m_bSaveAndValidate) {
		UINT flags=(m_bSVG_process?FLG_SVG_PROCESS:0);
		if(hPropHook) hPropHook=TRUE;
		if(!m_bInheritLength)   flags |= (m_bPreserveLength?FLG_VTLENCHK:FLG_VTLENUNCHK);
		if(!m_bInheritVertical) flags |= (m_bPreserveVertical?FLG_VTVERTCHK:FLG_VTVERTUNCHK);
		if(!m_bInheritView) {
			UINT f=m_iNorthEast;	
			if(Sheet()->m_pParent) f++; 
			flags |= (f<<FLG_VIEWSHIFT);
		}
		Sheet()->m_uFlags &= ~FLG_MASK_OPTIONS;
		Sheet()->m_uFlags |= flags;
	}
	else {
		if(!m_idLastFocus) {
			BOOL bIsRoot=(Sheet()->m_pParent==NULL);
			m_DisCombo.ResetContent();
			m_DisCombo.SetWindowText(m_szOptions);
			if(!bIsRoot) {
				//Fill combo box with inerited compile options --
				//m_DisCombo.InsertString(0,"Inherited Options --");
				InsertComboOption(Sheet()->m_pNode->Parent()); //Recursive
			}
			Enable(IDC_INHERIT_VERTICAL,!bIsRoot);
			Enable(IDC_INHERIT_LENGTH,!bIsRoot);
			Enable(IDC_INHERIT_VIEW,!bIsRoot);
			Enable(IDC_PRESERVE_LENGTH,bIsRoot || !m_bInheritLength);
			Enable(IDC_PRESERVE_VERTICAL,bIsRoot || !m_bInheritVertical);
			EnableView(bIsRoot || !m_bInheritView);
			Enable(IDC_SVG_PROCESS,!Sheet()->m_pNode->IsLeaf());
		}
	}
}

BEGIN_MESSAGE_MAP(CItemOptions, CItemPage)
    ON_MESSAGE(WM_COMMANDHELP,OnCommandHelp)
	//{{AFX_MSG_MAP(CItemOptions)
	ON_BN_CLICKED(IDC_INHERIT_LENGTH, OnInheritLength)
	ON_BN_CLICKED(IDC_INHERIT_VERTICAL, OnInheritVertical)
	ON_BN_CLICKED(IDC_INHERIT_VIEW, OnInheritView)
	//}}AFX_MSG_MAP
	ON_CBN_SELCHANGE(IDC_OPTIONS, OnCbnSelchangeOptions)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CItemOptions message handlers

LRESULT CItemOptions::OnCommandHelp(WPARAM wNone, LPARAM lParam)
{
	AfxGetApp()->WinHelp(131,HELP_CONTEXT);
	return TRUE;
}

void CItemOptions::OnInheritLength() 
{
	m_bInheritLength=!m_bInheritLength;

	if(m_bInheritLength) 
		Check(IDC_PRESERVE_LENGTH,Sheet()->m_pNode->Parent()->InheritedState(FLG_VTLENMASK));
	Enable(IDC_PRESERVE_LENGTH,!m_bInheritLength);
	
}

void CItemOptions::OnInheritVertical() 
{
	m_bInheritVertical=!m_bInheritVertical;

	if(m_bInheritVertical)
		Check(IDC_PRESERVE_VERTICAL,Sheet()->m_pNode->Parent()->InheritedState(FLG_VTVERTMASK));
	Enable(IDC_PRESERVE_VERTICAL,!m_bInheritVertical);
}


void CItemOptions::OnInheritView() 
{
	m_bInheritView=!m_bInheritView;

	if(m_bInheritView)
		CheckRadioButton(IDC_NORTHEAST,IDC_WEST,
		  view_id[Sheet()->m_pNode->Parent()->InheritedView()]);
	EnableView(!m_bInheritView);
}

/////////////////////////////////////////////////////////////////////////////
// CChangeEdit

#ifdef _DEBUG
	IMPLEMENT_DYNAMIC(CChangeEdit,CEdit)
#endif

BEGIN_MESSAGE_MAP(CChangeEdit, CEdit)
	//{{AFX_MSG_MAP(CChangeEdit)
	ON_CONTROL_REFLECT(EN_CHANGE, OnChange)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CChangeEdit::Init(CDialog *pDlg,UINT idEdit,UINT idText)
{
	if(!m_pDlg) {
		VERIFY(SubclassDlgItem(idEdit,m_pDlg=pDlg));
		m_idText=idText;
		m_bAbsolute=FALSE;
	}
}

void CChangeEdit::OnChange() 
{
	GetWindowText(m_cBuf);
	if(m_bAbsolute!=IsAbsolutePath(m_cBuf)) {
		m_pDlg->GetDlgItem(m_idText)->EnableWindow(!(m_bAbsolute=!m_bAbsolute));
	}
}

void CItemGeneral::OnBnClickedReadonly()
{
	m_bClickedReadonly=TRUE;
	m_bReadOnly=((CButton *)GetDlgItem(IDC_READONLY))->GetCheck()!=0;
}

void CItemOptions::OnCbnSelchangeOptions()
{
	// TODO: Add your control notification handler code here
}

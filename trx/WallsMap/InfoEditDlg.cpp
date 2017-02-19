// InfoEditDlg.cpp : implementation file
//

#include "stdafx.h"
#include "WallsMap.h"
#include "MainFrm.h"
#include "WallsMapDoc.h"
#include "InfoEditDlg.h"

// CInfoEditDlg dialog

//Globals --
CInfoEditDlg * app_pInfoEditDlg=NULL;
CRect CInfoEditDlg::m_rectSaved;

IMPLEMENT_DYNAMIC(CInfoEditDlg, CResizeDlg)

CInfoEditDlg::CInfoEditDlg(UINT nID,CWallsMapDoc *pDoc,CString &csData,CWnd* pParent = NULL)
		: CResizeDlg(nID, pParent)
		, m_pDoc(pDoc)
		, m_csData(csData)
	    , m_bEditable(!pDoc->m_bReadOnly)
{
	if(Create(CInfoEditDlg::IDD,pParent)) {
		app_pInfoEditDlg=this;
		RefreshTitle();
	}
}

void CInfoEditDlg::RefreshTitle()
{
	CString title;
	title.Format("Project %s Information",(LPCSTR)m_pDoc->GetTitle());
	SetWindowText(title);
}

bool CInfoEditDlg::DenyClose(BOOL bForceClose /*=FALSE*/)
{
	if(!HasEditedText()) return false;

	UINT mbtyp;
	CString s;
	
	s.Format("%s\n\nYou've edited the notes for %s and the changes haven't been saved. "
			  "Select YES to save, NO to discard changes",m_pDoc->GetTitle());
	if(bForceClose) {
		//Discard or save
		s.AppendChar('.');
		mbtyp=MB_YESNO;
	}
	else {
		//Discard, save, or continue --
		s+=", or CANCEL to continue editing.";
		mbtyp=MB_YESNOCANCEL;
	}

	mbtyp=AfxMessageBox(s,mbtyp);
	if(mbtyp==IDYES) {
		SaveEditedText();
	}
	else if(mbtyp==IDCANCEL) {
		m_rtf.SetFocus();
		return true;
	}
	return false;
}

bool CInfoEditDlg::HasEditedText()
{
	return m_bEditable && m_rtf.GetRichEditCtrl().GetModify() &&
			(!m_csData.GetLength() || m_rtf.GetTextLength());
}

void CInfoEditDlg::ReInit(CWallsMapDoc *pDoc,CString &csData)
{
	//m_pParent=pParent;
	free(m_memo.pData);
	m_memo=memo;
	m_memo.pData=_strdup(memo.pData);
	RefreshTitle();

	InitData();
}

CInfoEditDlg::~CInfoEditDlg()
{
	ASSERT(!app_pInfoEditDlg);
	free(m_memo.pData);
}

void CInfoEditDlg::PostNcDestroy() 
{
	ASSERT(app_pInfoEditDlg==this);
	app_pInfoEditDlg=NULL;
	delete this;
}

BOOL CInfoEditDlg::PreTranslateMessage(MSG* pMsg) 
{
	if(pMsg->message == WM_KEYDOWN) {
		if(pMsg->wParam==VK_ESCAPE) return TRUE;
	}
	return CResizeDlg::PreTranslateMessage(pMsg);
}

void CInfoEditDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizeDlg::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_STATIC_PLACEMENT, m_placement);
	if(pDX->m_bSaveAndValidate) {
		ASSERT(0);
	}
}

BEGIN_MESSAGE_MAP(CInfoEditDlg, CResizeDlg)
	ON_BN_CLICKED(IDC_CHK_TOOLBAR, &CInfoEditDlg::OnBnClickedToolbar)
	ON_REGISTERED_MESSAGE(urm_DATACHANGED,OnDataChanged)
	ON_REGISTERED_MESSAGE(urm_LINKCLICKED,OnLinkClicked)
    ON_MESSAGE(WM_COMMANDHELP,OnCommandHelp)
END_MESSAGE_MAP()

// CInfoEditDlg message handlers

void CInfoEditDlg::InitData()
{
	GetDlgItem(IDCANCEL)->SetWindowText(m_bEditable?"Cancel":"Close");
	GetDlgItem(IDOK)->ShowWindow(m_bEditable?SW_SHOW:SW_HIDE);
	GetDlgItem(IDC_IMAGEMODE)->ShowWindow(FALSE);

	m_rtf.SetReadOnly(m_bEditable==false);
	bool bFormatRTF=CDBTData::IsTextRTF(m_memo.pData);
	bool bStartRTF=m_bEditable && !*m_memo.pData && CMainFrame::IsPref(PRF_EDIT_INITRTF);

	m_rtf.ShowToolbar(bStartRTF || (bFormatRTF && m_bEditable && CMainFrame::IsPref(PRF_EDIT_TOOLBAR)));
	m_rtf.SetText(m_memo.pData,bFormatRTF||bStartRTF); //This will init toolbar button

	m_rtf.SetFocus();
}

BOOL CInfoEditDlg::OnInitDialog()
{
	CResizeDlg::OnInitDialog();

	CRect rect;
	m_placement.GetWindowRect( rect );
	ScreenToClient( rect );
	m_rtf.Create( WS_TABSTOP | WS_CHILD | WS_VISIBLE, rect, this, 100, &CMainFrame::m_fEditFont,RGB(0,0,0));

	m_xMin = 520; // x minimum resize
	m_yMin = 326; // y minimum resize
	AddControl(100, CST_RESIZE, CST_RESIZE, CST_RESIZE, CST_RESIZE,1); //flickerfree

	AddControl(IDCANCEL, CST_REPOS, CST_REPOS, CST_REPOS, CST_REPOS, 0);
	AddControl(IDOK, CST_REPOS, CST_REPOS, CST_REPOS, CST_REPOS, 0);
	AddControl(IDC_CHK_TOOLBAR, CST_REPOS, CST_REPOS, CST_REPOS, CST_REPOS, 0);

	if(!m_rectSaved.IsRectEmpty()) {
		Reposition(m_rectSaved);
	}

	m_rtf.SetWordWrap(TRUE);
	m_rtf.SetBackgroundColor(RGB(255,255,225));
	m_rtf.GetRichEditCtrl().LimitText(128*1024);

	InitData();

	//VERIFY(m_dropTarget.Register(this));

	return FALSE;  // return TRUE unless you set the focus to a control
}

void CInfoEditDlg::SaveEditedText()
{

	//Text has been edited --
    //TODO: fix this to stream rtf directly into an allocated m_memo.pData!!

	CString csText;

	if(m_rtf.GetTextLength()) {
		m_rtf.GetText(csText);
		ASSERT(csText.GetLength());
	}
	m_pShp->SaveEditedMemo(m_memo,csText);
}

void CInfoEditDlg::OnOK()
{
	if(HasEditedText()) {
		SaveEditedText();
	}
	Destroy();
}

LRESULT CInfoEditDlg::OnDataChanged(WPARAM bFormatRTF, LPARAM)
{
	BOOL bShow=m_bEditable && m_rtf.IsFormatRTF();
	GetDlgItem(IDC_CHK_TOOLBAR)->ShowWindow(bShow);
	if(bShow)
		ChkToolbarButton(m_rtf.IsToolbarVisible());

	GetDlgItem(IDC_IMAGEMODE)->EnableWindow(!m_rtf.GetTextLength() || !m_rtf.IsFormatRTF());
	return TRUE; //rtn not used
}

void CInfoEditDlg::OnCancel()
{
	if(DenyClose()) return;
	Destroy();
}

void CInfoEditDlg::OnLinkToFile()
{
	char pathName[20*MAX_PATH];
	*pathName=0;
	CString s;
	s.LoadString(IDS_INSERT_FILELINK);
	BOOL bEditable=CWallsMapDoc::PromptForFileName(pathName,20*MAX_PATH,s,0,FTYP_ALLTYPES|FTYP_NOSHP);

	if(!bEditable) {
		m_rtf.SetFocus();
		return; // open cancelled
	}

	char pathBuf[MAX_PATH];
	//last arg FALSE if pathName contains a file name before and after function call
	m_pShp->GetRelativePath(pathBuf,pathName,FALSE);

	s.Format(" <file://%s> ",pathBuf);
	s.Replace('\\','/');
	//s.Replace(" ","%20"); //brackets prevent need for this!

	m_rtf.SetFocus();
	m_rtf.GetRichEditCtrl().ReplaceSel(s,TRUE);
}

// My callback procedure that writes the rich edit control contents
// to a file.
static DWORD CALLBACK 
MyStreamOutCallback(DWORD dwCookie, LPBYTE pbBuff, LONG cb, LONG *pcb)
{
   CFile* pFile = (CFile*) dwCookie;

   pFile->Write(pbBuff, cb);
   *pcb = cb;

   return 0;
}

void CInfoEditDlg::OnSaveToFile()
{
	CString csPath;
	CString strFilter;
	bool bRTF=(m_rtf.IsFormatRTF()==TRUE);

	if(!AddFilter(strFilter,bRTF?IDS_RTF_FILES:IDS_TXT_FILES)) return;
	if(bRTF && !AddFilter(strFilter,IDS_TXT_FILES)) return;
	if(!AddFilter(strFilter,AFX_IDS_ALLFILTER)) return;

	GetWindowText(csPath);
	csPath.Replace(": ","-");

	char namBuf[60];
	LPSTR pDefExt=LabelToValidFileName(csPath,namBuf,60-4);
	strcpy(pDefExt,bRTF?".rtf":".txt");
	csPath.SetString(namBuf);

	if(!DoPromptPathName(csPath,OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT,
		1,strFilter,FALSE,IDS_SAVETOFILE,pDefExt)) return;

	if(bRTF && _stricmp(trx_Stpext(csPath),pDefExt)) bRTF=false;

	CFile cFile(csPath,CFile::modeCreate|CFile::modeWrite);
	EDITSTREAM es;
	es.dwCookie = (DWORD) &cFile;
	es.pfnCallback = MyStreamOutCallback;
	BOOL flags=bRTF?SF_RTF:SF_TEXT;
	if(m_rtf.GetRichEditCtrl().GetSelectionType()&SEL_TEXT)
		flags|=SFF_SELECTION;
	m_rtf.GetRichEditCtrl().StreamOut(flags,es);
	cFile.Close();
}

static bool ConfirmNoLinks()
{
	return IDOK==AfxMessageBox("CAUTION: Since text is present, but without recognized image links, it "
		"will be discarded if you switch to image mode and save changes.\nSelect OK if this is your intent.",MB_OKCANCEL);
}

void CInfoEditDlg::OnBnClickedToolbar()
{
	BOOL bNewChk=((CButton*)GetDlgItem(IDC_CHK_TOOLBAR))->GetCheck()!=0;
	ASSERT(m_rtf.IsFormatRTF() && bNewChk!=m_rtf.IsToolbarVisible());
	m_rtf.ShowToolbar(bNewChk);
	//hkToolbarButton(m_rtf.IsToolbarVisible());
}

void CInfoEditDlg::OnBnClickedImageMode()
{
	ASSERT(m_bEditable || CDBTData::IsTextIMG(m_memo.pData));

	LPCSTR p="";
	CString csText;

	if(HasEditedText()) {
		m_rtf.GetText(csText);
		if(!csText.IsEmpty() && !CDBTData::IsTextIMG(csText)) {
			if(!ConfirmNoLinks()) return;
		}
		else p=csText;
	}
	else if(*m_memo.pData && !CDBTData::IsTextIMG(m_memo.pData)) {
		if(!ConfirmNoLinks()) return;
	}
	else p=NULL; //memo is unedited and either empty or with valid links

	if(app_pImageDlg && app_pImageDlg->DenyClose())
		return;

	if(p) {
		ASSERT(m_bEditable);
		if(!m_pShp->SaveAllowed(1)) {
			m_pShp->CancelMemoMsg(m_memo);
			return;
		}
		free(m_memo.pData);
		m_memo.pData=_strdup(p);
		m_memo.bChg=2;
	}
	else m_memo.bChg=1;
	EDITED_MEMO *pMemo=new EDITED_MEMO(m_memo);
	m_memo.pData=NULL;
	GetMF()->PostMessage(WM_SWITCH_EDITMODE,(WPARAM)m_pShp,(LPARAM)pMemo);
	Destroy();
}

LRESULT CInfoEditDlg::OnLinkClicked(WPARAM, LPARAM lParam)
{
		ENLINK *p_enlink=(ENLINK *)lParam; 
		ASSERT(p_enlink && p_enlink->msg==WM_LBUTTONUP && p_enlink->chrg.cpMin<p_enlink->chrg.cpMax);

		if(p_enlink->chrg.cpMin<p_enlink->chrg.cpMax) {
			CString csLink;
			m_rtf.GetRichEditCtrl().GetTextRange(p_enlink->chrg.cpMin,p_enlink->chrg.cpMax,csLink);

			CShpLayer *pShp=NULL;
			if(!csLink.Find("file://",0)) { //csLink does not include any < or trailing >
				csLink.Delete(0,7);
				csLink.Replace("%20"," ");
				csLink.Replace('/','\\');
				//pShp=((CInfoEditDlg *)GetParent())->m_pShp; //bug fixed 10/26/2009
				pShp=m_pShp;
				LPCSTR pExt=trx_Stpext(csLink);
				if(!_stricmp(pExt,".nti") || !_stricmp(pExt,".ntl")) {
					if(!pShp->OpenLinkedDoc(csLink))
						goto _errmsg;
				}
				else {
					char buf[MAX_PATH];
					if(!(pShp->GetFullFromRelative(buf,(LPCSTR)csLink)))
						goto _errmsg;
					pShp=NULL;
					csLink=buf;
				}
			}

			if(!pShp) {
				//second param is NULL rather than "open" in order to invoke the default action,
				//otherwise PDF files would return SE_ERR_NOASSOC
				if((int)ShellExecute(m_hWnd, NULL, csLink, NULL, NULL, SW_SHOW)<=32)
					goto _errmsg;
				/*
				if(e<=32) {
					switch(e) {
						case ERROR_FILE_NOT_FOUND :
							e=100; break;
						case ERROR_PATH_NOT_FOUND :
							e=101; break;
						case ERROR_BAD_FORMAT :
							e=102; break;
						case SE_ERR_ACCESSDENIED :
							e=103; break;
						case SE_ERR_NOASSOC :
							e=104; break;
						case SE_ERR_SHARE :
							e=105; break;
					}
					goto _errmsg;
				}
				*/
			}
			return TRUE;
_errmsg:
			CMsgBox("Unable to open %s.\nThe file may be missing or the link improperly formatted.",(LPCSTR)csLink);
		}
				
		return TRUE; //rtn not used
}

LRESULT CInfoEditDlg::OnCommandHelp(WPARAM wNone, LPARAM lParam)
{
	AfxGetApp()->WinHelp(0);
	return TRUE;
}


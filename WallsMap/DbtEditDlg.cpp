// DbtEditDlg.cpp : implementation file
//

#include "stdafx.h"
#include "WallsMap.h"
#include "MainFrm.h"
#include "WallsMapDoc.h"
#include "ShowShapeDlg.h"
#include "ImageViewDlg.h"
#include "DBGridDlg.h"
#include "dbteditdlg.h"

// CDbtEditDlg dialog

//Globals --
CDbtEditDlg * app_pDbtEditDlg=NULL;
CRect CDbtEditDlg::m_rectSaved;
CString CDbtEditDlg::m_csLastSaved;

IMPLEMENT_DYNAMIC(CMemoEditDlg, CResizeDlg)
IMPLEMENT_DYNAMIC(CDbtEditDlg, CMemoEditDlg)

CDbtEditDlg::CDbtEditDlg(CDialog *pParentDlg,CShpLayer *pShp,EDITED_MEMO &memo,CWnd* pParent /*=NULL*/)
	: CMemoEditDlg(CDbtEditDlg::IDD,pParentDlg,pShp,memo,pParent)
{
	ASSERT(!app_pDbtEditDlg && memo.pData);
	VERIFY(m_memo.pData=_strdup(memo.pData));
	if(Create(CDbtEditDlg::IDD,pParent)) {
		app_pDbtEditDlg=this;
		RefreshTitle();
	}
}

bool CDbtEditDlg::DenyClose(BOOL bForceClose /*=FALSE*/)
{
	if(!HasEditedText()) return false;

	int id=m_pShp->ConfirmSaveMemo(m_hWnd,m_memo,bForceClose);

	if(id==IDYES) {
		if(m_pShp->SaveAllowed(1)) SaveEditedText();
		else if(!bForceClose) id=IDCANCEL;
	}

	if(id==IDCANCEL) {
		m_rtf.SetFocus();
		return true;
	}

	return false;
}

bool CDbtEditDlg::HasEditedText()
{
	return m_bEditable && m_rtf.GetRichEditCtrl().GetModify() &&
			(*m_memo.pData || m_rtf.GetTextLength());
}

void CDbtEditDlg::SetEditable(bool bEditable) {
	if(m_bEditable==bEditable) return;
	ASSERT(!HasEditedText());
	ASSERT(bEditable==m_pShp->IsEditable());
	InitData();
}

void CDbtEditDlg::ReInit(CShpLayer *pShp,EDITED_MEMO &memo)
{
	ASSERT(app_pShowDlg || pShp->m_pdbfile->pDBGridDlg);
	ASSERT(memo.pData);
	if(!memo.pData) {
		ASSERT(0);
		return;
	}

	//m_pParent=pParent;
	m_pShp=pShp;
	free(m_memo.pData);
	m_memo=memo;
	m_memo.pData=_strdup(memo.pData);
	RefreshTitle();

	InitData();
}

CDbtEditDlg::~CDbtEditDlg()
{
	ASSERT(!app_pDbtEditDlg);
	free(m_memo.pData);
}

void CDbtEditDlg::PostNcDestroy() 
{
	ReturnFocus();
	ASSERT(app_pDbtEditDlg==this);
	app_pDbtEditDlg=NULL;
	delete this;
}

BOOL CDbtEditDlg::PreTranslateMessage(MSG* pMsg) 
{
	if(pMsg->message == WM_KEYDOWN) {
		if(pMsg->wParam==VK_ESCAPE) return TRUE;
		if(pMsg->wParam==0x56) {
			if(GetKeyState(VK_CONTROL)<0) {
				//^V
				if(m_bEditable && m_rtf.CanPaste())
					m_rtf.PostMessage(WM_COMMAND,ID_EDIT_PASTE);
				return TRUE;
			}
		}
	}
	return CResizeDlg::PreTranslateMessage(pMsg);
}

void CDbtEditDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizeDlg::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_STATIC_PLACEMENT, m_placement);
	if(pDX->m_bSaveAndValidate) {
		ASSERT(0);
	}
}

BEGIN_MESSAGE_MAP(CDbtEditDlg, CResizeDlg)
	ON_BN_CLICKED(IDC_IMAGEMODE, &CDbtEditDlg::OnBnClickedImageMode)
	ON_BN_CLICKED(IDC_CHK_TOOLBAR, &CDbtEditDlg::OnBnClickedToolbar)
	ON_REGISTERED_MESSAGE(urm_DATACHANGED,OnDataChanged)
	ON_REGISTERED_MESSAGE(urm_LINKCLICKED,OnLinkClicked)
    ON_MESSAGE(WM_COMMANDHELP,OnCommandHelp)
END_MESSAGE_MAP()

// CDbtEditDlg message handlers

void CDbtEditDlg::InitData()
{
	m_bEditable=m_pShp->IsEditable() && (!m_memo.uRec || !m_pShp->IsRecDeleted(m_memo.uRec)) 
		/*&& !CDBGridDlg::m_bNotAllowingEdits*/ && !m_pShp->IsFldReadOnly(m_memo.fld);

	GetDlgItem(IDCANCEL)->SetWindowText(m_bEditable?"Cancel":"Close");
	GetDlgItem(IDOK)->ShowWindow(m_bEditable?SW_SHOW:SW_HIDE);

	m_rtf.SetReadOnly(m_bEditable==false);
	bool bFormatRTF=CDBTData::IsTextRTF(m_memo.pData);
	bool bStartRTF=m_bEditable && !*m_memo.pData && CMainFrame::IsPref(PRF_EDIT_INITRTF);

	m_rtf.ShowToolbar(bStartRTF || (bFormatRTF && m_bEditable && CMainFrame::IsPref(PRF_EDIT_TOOLBAR)));
	m_rtf.SetText(m_memo.pData,bFormatRTF||bStartRTF); //This will init toolbar button

	if(!m_bEditable && (m_rtf.IsFormatRTF() || !CDBTData::IsTextIMG(m_memo.pData)))
		GetDlgItem(IDC_IMAGEMODE)->ShowWindow(FALSE);

	m_rtf.SetFocus();
}

BOOL CDbtEditDlg::OnInitDialog()
{
	CResizeDlg::OnInitDialog();

	CRect rect;
	m_placement.GetWindowRect( rect );
	ScreenToClient( rect );
	m_rtf.Create( WS_TABSTOP | WS_CHILD | WS_VISIBLE, rect, this, 100, &CMainFrame::m_fEditFont,CMainFrame::m_clrTxt);

	m_xMin = 520; // x minimum resize
	m_yMin = 326; // y minimum resize
	AddControl(100, CST_RESIZE, CST_RESIZE, CST_RESIZE, CST_RESIZE,1); //flickerfree

	AddControl(IDC_IMAGEMODE, CST_NONE, CST_NONE, CST_REPOS, CST_REPOS, 0);
	AddControl(IDCANCEL, CST_REPOS, CST_REPOS, CST_REPOS, CST_REPOS, 0);
	AddControl(IDOK, CST_REPOS, CST_REPOS, CST_REPOS, CST_REPOS, 0);
	AddControl(IDC_CHK_TOOLBAR, CST_REPOS, CST_REPOS, CST_REPOS, CST_REPOS, 0);

	if(!m_rectSaved.IsRectEmpty()) {
		Reposition(m_rectSaved);
	}
	else if(app_pShowDlg) {
		app_pShowDlg->GetWindowRect(&rect);
		//Offset, but make sure new TL position is on screen --
		rect.left+=100;
		rect.top+=120;
		Reposition(rect,SWP_NOSIZE);
	}

	m_rtf.SetWordWrap(TRUE);
	m_rtf.SetBackgroundColor(CMainFrame::m_clrBkg);
	m_rtf.GetRichEditCtrl().LimitText(128*1024);
	m_rtf.m_pParentDlg=this;
	m_rtf.m_idContext=IDR_RTF_CONTEXT;
	
	InitData();

	VERIFY(m_dropTarget.Register(this,DROPEFFECT_LINK));

	return FALSE;  // return TRUE unless you set the focus to a control
}

void CDbtEditDlg::SaveEditedText()
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

void CDbtEditDlg::OnOK()
{
	if(HasEditedText()) {
		if(!m_pShp->SaveAllowed(1)) {
			m_pShp->CancelMemoMsg(m_memo);
			return;
		}
		SaveEditedText();
	}
	Destroy();
}

LRESULT CDbtEditDlg::OnDataChanged(WPARAM bFormatRTF, LPARAM)
{
	BOOL bShow=m_bEditable && m_rtf.IsFormatRTF();
	GetDlgItem(IDC_CHK_TOOLBAR)->ShowWindow(bShow);
	if(bShow)
		ChkToolbarButton(m_rtf.IsToolbarVisible());

	GetDlgItem(IDC_IMAGEMODE)->EnableWindow(!m_rtf.GetTextLength() || !m_rtf.IsFormatRTF());
	return TRUE; //rtn not used
}

void CDbtEditDlg::OnCancel()
{
	if(DenyClose()) return;
	Destroy();
}

void CDbtEditDlg::OnLinkToFile()
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
static DWORD CALLBACK MyStreamOutCallback(DWORD dwCookie, LPBYTE pbBuff, LONG cb, LONG *pcb)
{
   ((CFile*)dwCookie)->Write(pbBuff, cb);
   *pcb = cb;
   return 0;
}

// My callback procedure that reads the rich edit control contents from a file. 
static DWORD CALLBACK MyStreamInCallback(DWORD dwCookie, LPBYTE pbBuff, LONG cb, LONG *pcb)
{
   *pcb=((CFile*)dwCookie)->Read(pbBuff, cb);
   return 0;
}

LPCSTR CDbtEditDlg::GetMemoFileName(CString &csPath)
{
	static LPCSTR pExt[2]={".txt",".rtf"};
	int iExt=m_rtf.IsFormatRTF();
	GetWindowText(csPath);
	csPath.Replace(": ","-");
	char namBuf[80];
	LPSTR pDefExt=LabelToValidFileName(csPath,namBuf,80-4);
	strcpy(pDefExt,pExt[iExt]);
	LPCSTR p=trx_Stpnam(m_pShp->PathName());
	csPath.SetString(p, trx_Stpext(p)-p);
	csPath.AppendFormat("_#%u_%s",m_memo.uRec,namBuf);
	return pExt[iExt];
}

bool CDbtEditDlg::WriteToFile(CString &path)
{
	bool bRTF=!_stricmp(trx_Stpext(path),".rtf");

	try {
		CFile cFile(path,CFile::modeCreate|CFile::modeWrite);
		EDITSTREAM es;
		es.dwCookie = (DWORD) &cFile;
		es.pfnCallback = MyStreamOutCallback;
		BOOL flags=_stricmp(trx_Stpext(path),".rtf")?SF_TEXT:SF_RTF;
		if(m_rtf.GetRichEditCtrl().GetSelectionType()&SEL_TEXT)
			flags|=SFF_SELECTION;
		m_rtf.GetRichEditCtrl().StreamOut(flags,es);
		cFile.Close();
	}
	catch(...) {
		CMsgBox("%s\n\nThis file is in use and can't be rewritten.",(LPCSTR)path);
		return false;
	}
	return true;
}

void CDbtEditDlg::OnSaveToFile()
{
	CString strFilter;
	bool bRTF=(m_rtf.IsFormatRTF()==TRUE);

	if(!AddFilter(strFilter,bRTF?IDS_RTF_FILES:IDS_TXTRTF_FILES)) return;
	//if(!AddFilter(strFilter,bRTF?IDS_TXT_FILES:IDS_RTF_FILES)) return;
	if(!AddFilter(strFilter,AFX_IDS_ALLFILTER)) return;

	CString path,name;
	LPCSTR p;
	if(!m_csLastSaved.IsEmpty()) {
		//Use same location as saved file --
		p=m_csLastSaved;
	}
	else p=m_pShp->m_pDoc->PathName();
	path.SetString(p, trx_Stpnam(p)-p);
	p=GetMemoFileName(name);
	path+=name;

	if(!DoPromptPathName(path,OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT,
		2,strFilter,FALSE,IDS_SAVETOFILE,p)) return;

	if(!WriteToFile(path)) {
		return;
	}
	m_csLastSaved=path;
}

void CDbtEditDlg::OnCompareWithFile()
{
	CString nameSrc,pathSrc;
	GetMemoFileName(nameSrc); //will not have path prefix
	ASSERT(CMainFrame::m_pComparePrg && !m_csLastSaved.IsEmpty());

	/*
	if(!_stricmp(trx_Stpnam(m_csLastSaved), nameSrc)) {
		AfxMessageBox("The current memo was the last one saved to file.");
		return;
	}
	*/
	if(_access(m_csLastSaved, 0)) {
		AfxMessageBox("The last saved memo file is no longer available.");
		return;
	}
	//Use same location as saved file --
	LPCSTR p=m_csLastSaved;
	pathSrc.SetString(p, trx_Stpnam(p)-p);
	pathSrc+=nameSrc;
	if(!_access(pathSrc, 0) && CheckAccess(pathSrc, 2)) {
		CMsgBox("%s\n\nThis file, needed for storing this memo, already exists and is protected.");
		return;
	}
	//Save this memo in same folder and launch compare fcn --
	if(!WriteToFile(pathSrc))
		return;
	CMainFrame::LaunchComparePrg(pathSrc,m_csLastSaved);
}

void CDbtEditDlg::OnReplaceWithFile()
{
	bool bRTF=(m_rtf.IsFormatRTF()==TRUE);

	//bool be=HasEditedText();

	CString path,name;
	LPCSTR p;
	bool bSaved=!m_csLastSaved.IsEmpty();
	if(bSaved) {
		//Use same location as saved file --
		p=m_csLastSaved;
	}
	else p=m_pShp->m_pDoc->PathName();
	path.SetString(p, trx_Stpnam(p)-p);
	if(bSaved) {
		p=GetMemoFileName(name);
		path+=name; //by default, load file with edited version of current memo
	}
	else p=bRTF?".rtf":".txt";

	CString strFilter;
	if(!AddFilter(strFilter,IDS_TXTRTF_FILES)) return;
	if(!AddFilter(strFilter,AFX_IDS_ALLFILTER)) return;
	if(!DoPromptPathName(path,OFN_FILEMUSTEXIST,
		2,strFilter,TRUE,IDS_REPLWITHFILE,p)) return;

	m_rtf.SetWindowText("");
	if(bRTF != (_stricmp(trx_Stpext(path),".rtf")==0)) {
		m_rtf.OnFormatRTF();
		bRTF=(m_rtf.IsFormatRTF()==TRUE);
	}

	CFile cFile(path,CFile::modeRead);
	EDITSTREAM es;
	es.dwCookie = (DWORD) &cFile;
	es.dwError = 0;
	es.pfnCallback = MyStreamInCallback;
	BOOL flags=bRTF?SF_RTF:SF_TEXT;
	//if(m_rtf.GetRichEditCtrl().GetSelectionType()&SEL_TEXT)
		//flags|=SFF_SELECTION;
	long nCount=m_rtf.GetRichEditCtrl().StreamIn(flags,es);
	//be=HasEditedText();
	cFile.Close();
	if(es.dwError!=0) {
		CMsgBox("Error reading %s.",(LPCSTR)path);
	}
}

static bool ConfirmNoLinks()
{
	return IDOK==AfxMessageBox("CAUTION: Since text is present, but without recognized image links, it "
		"will be discarded if you switch to image mode and save changes.\nSelect OK if this is your intent.",MB_OKCANCEL);
}

void CDbtEditDlg::OnBnClickedToolbar()
{
	BOOL bNewChk=((CButton*)GetDlgItem(IDC_CHK_TOOLBAR))->GetCheck()!=0;
	ASSERT(m_rtf.IsFormatRTF() && bNewChk!=m_rtf.IsToolbarVisible());
	m_rtf.ShowToolbar(bNewChk);
	//hkToolbarButton(m_rtf.IsToolbarVisible());
}

void CDbtEditDlg::OnBnClickedImageMode()
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
	CMainFrame::m_pMemoParentDlg=m_pParentDlg;
	GetMF()->PostMessage(WM_SWITCH_EDITMODE,(WPARAM)m_pShp,(LPARAM)pMemo);
	m_pParentDlg=NULL; //no need to reset focus upon PostNcDestroy()
	Destroy();
}

LRESULT CDbtEditDlg::OnLinkClicked(WPARAM, LPARAM lParam)
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
				//pShp=((CDbtEditDlg *)GetParent())->m_pShp; //bug fixed 10/26/2009
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

LRESULT CDbtEditDlg::OnCommandHelp(WPARAM wNone, LPARAM lParam)
{
	AfxGetApp()->WinHelp(0);
	return TRUE;
}

// wedview.cpp : implementation of the CWedView class
///////////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "walls.h"
#include "prjdoc.h"
#include "prjview.h"
#include "txtfrm.h"
#include "linedoc.h"
#include "compview.h"
#include "plotview.h"
#include "dialogs.h"
#include "lineview.h"
#include "wedview.h"
#include "netlib.h"
#include "wall_srv.h"
#include "tabedit.h"
#include "compile.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWedView

IMPLEMENT_DYNCREATE(CWedView,CLineView)

BEGIN_MESSAGE_MAP(CWedView,CLineView)
    //{{AFX_MSG_MAP(CWedView)
	ON_WM_CHAR()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_WM_LBUTTONDBLCLK()
	ON_COMMAND(ID_EDIT_COPY,OnEditCopy)
	ON_COMMAND(ID_EDIT_CUT,OnEditCut)
	ON_COMMAND(ID_EDIT_PASTE, OnEditPaste)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE, OnUpdateEditPaste)
	ON_COMMAND(ID_EDIT_FIND, OnEditFind)
	ON_COMMAND(ID_EDIT_REPLACE, OnEditReplace)
	ON_WM_DESTROY()
	ON_COMMAND(ID_GOTOLINE, OnGotoline)
	ON_COMMAND(ID_FIND_NEXT, OnFindNext)
	ON_COMMAND(ID_FIND_PREV, OnFindPrev)
	ON_COMMAND(ID_EDIT_AUTOSEQ, OnEditAutoSeq)
	ON_UPDATE_COMMAND_UI(ID_EDIT_AUTOSEQ, OnUpdateEditAutoSeq)
	ON_COMMAND(ID_EDIT_REVERSE, OnEditReverse)
	ON_UPDATE_COMMAND_UI(ID_EDIT_REVERSE, OnUpdateEditReverse)
	ON_COMMAND(ID_ZIPBACKUP, OnZipBackup)
	ON_WM_RBUTTONUP()
	ON_COMMAND(ID_EDIT_PROPERTIES, OnEditProperties)
	ON_COMMAND(ID_LOCATEINGEOM, OnLocateInGeom)
	//ON_UPDATE_COMMAND_UI(ID_LOCATEINGEOM, OnUpdateLocateInGeom)
	ON_COMMAND(ID_LOCATEONMAP, OnLocateOnMap)
	//ON_UPDATE_COMMAND_UI(ID_LOCATEONMAP, OnUpdateLocateInGeom)
	ON_COMMAND(ID_REVIEWSEG, OnViewSegment)
	//ON_UPDATE_COMMAND_UI(ID_REVIEWSEG, OnUpdateLocateInGeom)
	ON_COMMAND(ID_VECTORINFO, OnVectorInfo)
	//ON_UPDATE_COMMAND_UI(ID_VECTORINFO, OnUpdateLocateInGeom)
	//}}AFX_MSG_MAP
    ON_MESSAGE(WM_COMMANDHELP,OnCommandHelp)
	ON_UPDATE_COMMAND_UI(ID_FIND_PREV,OnUpdateFindNext)
	ON_UPDATE_COMMAND_UI(ID_FIND_NEXT,OnUpdateFindNext)
	ON_UPDATE_COMMAND_UI(ID_EDIT_FIND,OnUpdateFindNext)
	ON_UPDATE_COMMAND_UI(ID_EDIT_REPLACE,OnUpdateEditReplace)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_PFXNAM,OnUpdatePfxNam)
	ON_UPDATE_COMMAND_UI(ID_EDIT_COPY,OnUpdateSel)
	ON_UPDATE_COMMAND_UI(ID_EDIT_CUT,OnUpdateSel)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_SEL,OnUpdateSel)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_COL,OnUpdateCol)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_LINE,OnUpdateLine)
	ON_COMMAND(ID_EDIT_UNDO, OnEditUndo)
	ON_UPDATE_COMMAND_UI(ID_EDIT_UNDO, OnUpdateEditUndo)
	ON_COMMAND(ID_EDIT_REDO, OnEditRedo)
	ON_UPDATE_COMMAND_UI(ID_EDIT_REDO, OnUpdateEditRedo)
	ON_COMMAND(ID_POPUP_WRITEPROTECT, OnPopupWriteprotect)
	ON_UPDATE_COMMAND_UI(ID_POPUP_WRITEPROTECT, OnUpdatePopupWriteprotect)
END_MESSAGE_MAP()

////////////////////////////////////////////////////////////////////////////
// CWedView construction/destruction

CWedView::CWedView()
{
	// Init everything to zero (except for base class, CLineView) --
	//AFX_ZERO_INIT_OBJECT(CLineView);
	m_nWinOffset=m_nTabCols=m_nTabStop=0;
	m_bCaretShown=false;
    m_uFlags=0;
}

CWedView::~CWedView()
{
	m_nEditWinPos[m_nWinOffset]=0;
}
/////////////////////////////////////////////////////////////////////////////
// Static initialization/termination

CWedView *CWedView::m_pFocusView=NULL;
CPen CWedView::m_hPenGrid;
PRJFONT CWedView::m_EditFont;
LPINT CWedView::m_CharWidthTable=NULL;
int CWedView::m_nMaxWidth;
int CWedView::m_nAveWidth;
int CWedView::m_nSpcWidth;
int CWedView::m_nTabWidth;
bool CWedView::m_bClassRegistered=false;
int CWedView::m_nEditWinPos[SIZ_EDITWINPOS];

static char BASED_CODE szEditFont[] = "Editor Font";
static char BASED_CODE szEditWndClass[] = "WedViewWnd";

CFindTabDlg * CWedView::m_pSearchDlg=NULL;
LINENO CWedView::m_nFindLine;

BOOL CWedView::m_bFindOnly;
WORD CWedView::m_wTgtBufFlags=0;
BOOL CWedView::m_bReplaceAll=FALSE;
BOOL CWedView::m_bReplace=FALSE;
BOOL CWedView::m_bSearchDown=TRUE;
char * CWedView::m_pTgtBuf=NULL;
BOOL CWedView::m_bSelectTrailingWS=FALSE;

int CWedView::IncFindLine()
{
	int lenLine=0;

    if(m_nLineTotal<2) return 0;
	while(!lenLine) {
		if(m_bSearchDown) {
			if(++m_nFindLine>m_nLineTotal) {
				m_nFindLine=1;
			}
		}
		else {
			if(--m_nFindLine<1) m_nFindLine=m_nLineTotal;
		}
		if(m_nActiveLine==m_nFindLine) break;
		lenLine=LineLen(m_nFindLine);
	}
	return lenLine;
} 

static char RegExConvert(char c,char &cConvert)
{
	switch(cConvert) {
		case 'u' : cConvert=0;
		case 'U' : c=toupper(c); break;
		case 'l' : cConvert=0;
		case 'L' : c=tolower(c); break;
	}
	return c;
}

static void RegExMove(LPSTR pDest,LPCSTR pSrc,int len,char &cConvert)
{
	if(cConvert) {
		while(len--) *pDest++=RegExConvert(*pSrc++,cConvert);
	}
	else memcpy(pDest,pSrc,len);
}

int CWedView::RegExReplaceWith(int *ovector,int numsubstr)
{
	char repstr[CLineDoc::MAXLINLEN+1];
	char *p=repstr;
	int lenttl=0;
	char c,cConvert=0;
	LPCSTR reppat=m_pSearchDlg->GetReplaceStringPtr();
	LPSTR pLine=(LPSTR)LinePtr(m_nFindLine)+1;

	if(!numsubstr) numsubstr++;

	while(c=*reppat++) {
		if(c=='\\') {
			switch(c=*reppat++) {
				case 't' : c='\t'; break;
				case 'l' :
				case 'L' :
				case 'u' :
				case 'U' : cConvert=c; continue;
				case 'e' :
				case 'E' : cConvert=0; continue;
				default  :
					if(isdigit((BYTE)c)) {
						int len,i=(int)(c-'0');
						if(i<numsubstr) {
							i*=2;
							if((len=ovector[i+1]-ovector[i])>0 && lenttl+len<=CLineDoc::MAXLINLEN) {
								RegExMove(repstr+lenttl,pLine+ovector[i],len,cConvert);
								lenttl+=len;
							}
							continue;
						}
					}
					else if(!c) {
						reppat--;
						continue;
					}
			}
		}
		if(cConvert) c=RegExConvert(c,cConvert);
		if(lenttl<CLineDoc::MAXLINLEN) repstr[lenttl++]=c;
	}
	repstr[lenttl]=0;

	CLineRange range(m_nFindLine,ovector[0],m_nFindLine,ovector[1]);
	//A string insertion is either all or nothing --
	if(!GetDocument()->Update(lenttl?
	       CLineDoc::UPD_INSERT|CLineDoc::UPD_TRUNCABORT:CLineDoc::UPD_DELETE,
	       repstr,lenttl,range)) return -1;
	return lenttl;
}

void CWedView::DoRegExFindReplace()
{
	LPSTR pSel,pLine;
	int lenSel,lenLine,firstFindLineOffset;
	int nFindR0,nFindR1;
	int flags,numsubstr,ovector[30];
	BOOL bReachedLast=FALSE;
    int findCount=0;

	pcre *re=GetRegEx(m_pTgtBuf,m_wTgtBufFlags);
	ASSERT(re);
	    
	firstFindLineOffset=m_nActiveChr;
	m_nFindLine=m_nActiveLine;

	pLine=(LPSTR)LinePtr(m_nFindLine)+1;
	lenLine=LineLen(m_nFindLine);

    //First, establish the start search position. We set nFindR0 and nFindR1 to
    //the starting pLine offset and ending pLine offset respectively.

	if(flags=GetSelectedString(pSel,lenSel,FALSE/*m_bMatchWhole*/)) {
		if(m_nActiveLine!=m_nSelectLine) flags=0; //Selected an entire line
		ClearSelection();
	}

	if(flags) {
		if(m_bReplace) {
			numsubstr=pcre_exec(re,NULL,pLine,lenLine,pSel-pLine,
				PCRE_ANCHORED|PCRE_NOTEMPTY,ovector,30);
			if(numsubstr>=0) {
				//UpdateDoc() replaces the selection and advances the caret to the
				//point following the replacement string --
				if((lenSel=RegExReplaceWith(ovector,numsubstr))<0)  goto _repAbt;
				findCount++;
				firstFindLineOffset=pSel-pLine; //Don't search past this offset if m_bReplaceAll
				pLine=(LPSTR)LinePtr(m_nFindLine)+1;
				ASSERT(LineLen(m_nFindLine)==lenLine+lenSel-(ovector[1]-ovector[0]));
				lenLine=LineLen(m_nFindLine);
				//continue search at first char past replacement, which is m_nActiveChr --
				flags=0;
				ASSERT(m_bSearchDown);
			}
			else m_bReplaceAll=FALSE;
		}
	}
	else {
		m_bReplaceAll=FALSE;
	}

	//If flags==TRUE, previously-selected text of length lenSel exists at offset pSel-pLine --

 	if(m_bSearchDown) {
		//If selection, one char past first selected char
		nFindR0=flags?(pSel-pLine+1):m_nActiveChr;
		nFindR1=lenLine;
	}
	else {
		nFindR1=flags?(pSel+lenSel-pLine-1):m_nActiveChr; //offset of last char of selection
		nFindR0=nFindR1-1; //first offset we'll check if we have to iterate backward
	}
   
	while(TRUE) {

		if(m_bSearchDown) {
#ifdef _DEBUG
			if(nFindR1<=nFindR0) {
				ASSERT(TRUE);
			}
#endif
			if(nFindR0<lenLine &&
					(numsubstr=pcre_exec(re,NULL,pLine,nFindR1,nFindR0,
					(nFindR1<lenLine)?(PCRE_NOTEMPTY|PCRE_NOTEOL):PCRE_NOTEMPTY,
					ovector,30))>=0) {

				if(!m_bReplaceAll) goto _match_found;
				if(m_bReplaceAll>1) {
					if(IDOK==CMsgBox(MB_OKCANCEL,IDS_PRJ_REPLWRAP1,findCount)) {
						//Select and scroll to the found string --
						SetPosition(m_nFindLine,ovector[1],POS_CHR|POS_NOTRACK,FALSE);
						SetPosition(m_nFindLine,ovector[0],POS_CHR,TRUE);
					}
					break;
				}
				if((lenSel=RegExReplaceWith(ovector,numsubstr))<0) goto _repAbt;
				pLine=(LPSTR)LinePtr(m_nFindLine)+1;
				lenLine=LineLen(m_nFindLine);
				nFindR0=ovector[0]+lenSel; //continue search at first char past replacement --
				findCount++;
				continue;
			}
		}
		else {
			int firstMatchOffset=-1,firstMatchEndOffset;
			flags=(nFindR1<lenLine)?(PCRE_NOTEOL|PCRE_NOTEMPTY):PCRE_NOTEMPTY;
			if(nFindR0>0) {
				//If going backward we can dismiss this line if there are no matches at all --
				if(pcre_exec(re,NULL,pLine,nFindR1,0,flags,ovector,3)<0) goto _next_line;
				firstMatchOffset=ovector[0];
				firstMatchEndOffset=ovector[1];
			}
			while(nFindR0>firstMatchOffset) {
				if(pcre_exec(re,NULL,pLine,nFindR1,nFindR0,flags,ovector,3)>=0) goto _match_found;
				nFindR0--;
			}
			if(firstMatchOffset>=0) {
				ovector[0]=firstMatchOffset;
				ovector[1]=firstMatchEndOffset;
				goto _match_found;
			}
		}

_next_line:
		if(bReachedLast) break;
		if(!(lenLine=IncFindLine())) {
			lenLine=LineLen(m_nFindLine);
			bReachedLast=TRUE;
		}
		pLine=(LPSTR)LinePtr(m_nFindLine)+1;
		if(m_bSearchDown) {
			nFindR1=(bReachedLast && m_bReplaceAll)?firstFindLineOffset:lenLine;
			nFindR0=0;
			if(m_bReplaceAll && m_nFindLine==1) m_bReplaceAll++;
		}
		else {
			nFindR1=lenLine;
			nFindR0=lenLine-1; //first offset we'll check if we have to iterate backward
		}
	}

	//No match found or all replacements made --

	free(re);
	if(!m_bReplaceAll || !findCount) {
		//char tgtbuf[SIZ_TGTMSG];
		StatusMsg(IDS_ERR_REGEXNOFIND,m_pTgtBuf);
	}
	else StatusMsg(IDS_ERR_REPLACE1,findCount);
	return;

_repAbt:
	free(re);
	StatusMsg(IDS_ERR_REPABORT1,findCount);
	return;

_match_found:
	free(re);
	
	//Select and scroll to the found string --
	SetPosition(m_nFindLine,ovector[1],POS_CHR|POS_NOTRACK,FALSE);
	SetPosition(m_nFindLine,ovector[0],POS_CHR,TRUE);

	//For a successful find, if this is not a find/replace operation get rid
	//of the dialog which is probably just in the way. The toolbar or keyboard
	//is more convenient hereafter --
	if(m_bFindOnly && m_pSearchDlg)
		m_pSearchDlg->PostMessage(WM_COMMAND,IDCANCEL,0L); //Also clears statusbar
	else StatusClear();
}


void CWedView::OnSearch()
{
    //This function is called by CMainFrame::OnFindReplace(), the handler
    //for the modeless CFindReplaceDialog object created in
    //CWedView:DoFindReplaceDialog(). This dialog may or may not have been
    //initiated while this view was active.
    
    //NOTE: For ReplaceCurrent() or ReplaceAll() SearchDown() will always
    //be TRUE.

	if(!m_bFindOnly) {
		m_bReplaceAll=m_pSearchDlg->ReplaceAll();
		m_bReplace=m_bReplaceAll || m_pSearchDlg->ReplaceCurrent();
	}
	else m_bReplace=m_bReplaceAll=FALSE;
    
	if(m_bReplace || m_pSearchDlg->FindNext()) {
		LPCSTR p=m_pSearchDlg->GetFindStringPtr();
	    if(!*p) return;

		WORD flags=F_MATCHCASE * m_pSearchDlg->MatchCase() +
	  			F_MATCHWHOLEWORD * m_pSearchDlg->MatchWholeWord() +
				F_USEREGEX * m_pSearchDlg->GetUseRegEx();

		if((flags&F_USEREGEX) && !CheckRegEx(p,flags)) {
			m_pSearchDlg->SetFocusToEdit();
			return;
		}
		m_wTgtBufFlags=flags;
	    strcpy(m_pTgtBuf,p);
		hist_prjfind.InsertAsFirst(CString(p),flags);
		if(m_bReplace) {
			p=m_pSearchDlg->GetReplaceStringPtr();
			if(*p) hist_prjrepl.InsertAsFirst(CString(p));
		}
	    m_bSearchDown=!m_bFindOnly || m_pSearchDlg->SearchDown();
	    DoRegExFindReplace();
	}
}

BOOL CWedView::AllocTgtBuf()
{
	if(!m_pTgtBuf) {
	  if((m_pTgtBuf=(char *)malloc(SIZ_TGTBUF))==NULL)
	    return FALSE;
	  *m_pTgtBuf=0;
	  m_wTgtBufFlags=0;
	}
	return TRUE;
}

BOOL CWedView::InitTgtBuf(BOOL bUseSelection)
{
	LPSTR pSel;
	int lenSel;
	
    if(!AllocTgtBuf()) return FALSE;
	//Get the default search string --
	if(bUseSelection && GetSelectedString(pSel,lenSel) && lenSel<SIZ_TGTBUF) {
		//If the selected string matches a regular expression in m_pTgtBuf,
		//don't replace m_pTgtBuf with it --
		if(*m_pTgtBuf && (m_wTgtBufFlags&F_USEREGEX)) {
			pcre *re=GetRegEx(m_pTgtBuf,m_wTgtBufFlags);
			if(re) {
				int ovector[3];
				LPCSTR pLine=(LPSTR)LinePtr(m_nActiveLine)+1;
				if(pcre_exec(re,NULL,pLine,LineLen(m_nActiveLine),pSel-pLine,
					PCRE_ANCHORED|PCRE_NOTEMPTY,ovector,3)>=0 &&
					ovector[1]-ovector[0]==lenSel) bUseSelection=FALSE;
				free(re);
			}
		}
		if(bUseSelection) {
			memcpy(m_pTgtBuf,pSel,lenSel);
			m_pTgtBuf[lenSel]=0;
			m_wTgtBufFlags&=~F_USEREGEX;
		}
	}
	if(!*m_pTgtBuf) {
		strcpy(m_pTgtBuf,hist_prjfind.GetFirstString());
		if(*m_pTgtBuf) m_wTgtBufFlags=hist_prjfind.GetFirstFlags();
	}
	return TRUE;
}

void CWedView::DoFindReplaceDialog(BOOL bFindOnly)
{
	if(m_pSearchDlg != NULL) {
	  m_pSearchDlg->SetActiveWindow();
	  return;
	}
	//Initialize with selected text, but only if the selected text
	//doesn't already match m_pTgtBuf --
	if(!InitTgtBuf(TRUE)) return;
	
	m_pSearchDlg = new CFindTabDlg(bFindOnly);
	
	DWORD dwFlags=0;
	if(m_wTgtBufFlags&F_MATCHWHOLEWORD) dwFlags |= FR_WHOLEWORD;
	if(m_wTgtBufFlags&F_MATCHCASE) dwFlags |= FR_MATCHCASE;
	if(m_bSearchDown) dwFlags |= FR_DOWN;
	dwFlags |= FR_ENABLETEMPLATE;

	/*
	if(!bFindOnly) {
		//Only if this is a find/replace operation do we make the current
		//selected string the first candidate to search --
		if(CLineDoc::NormalizeRange(m_nActiveLine,m_nActiveChr,m_nSelectLine,m_nSelectChr))          
			ClearSelection();
	}
	*/

	//Last param specifies parent as application window;
	//callback is in CMainFrame --
	
	m_bFindOnly=bFindOnly;
	m_pSearchDlg->Create(
		bFindOnly,
		m_pTgtBuf,
		bFindOnly?NULL:hist_prjrepl.GetFirstString(),
		dwFlags,
		NULL);
	m_pSearchDlg->SetUseRegEx((m_wTgtBufFlags&F_USEREGEX)!=0);
}

void CWedView::DoFind(BOOL bSearchDown)
{
	//Menu or toolbar (not dialog) invocation of Find Next/Prev --
	//Move to next or previous match --
	if(!InitTgtBuf(FALSE)) return; //Don't initialize with selected text
	m_bSearchDown=bSearchDown;
	if(!*m_pTgtBuf) {
		DoFindReplaceDialog(TRUE); //find only, not replace
	}
	else {
	  m_bReplace=m_bReplaceAll=FALSE;
	  m_bFindOnly=TRUE;
	  DoRegExFindReplace();
	}
}

void CWedView::OnFindNext()
{
  DoFind(TRUE);
}

void CWedView::OnFindPrev()
{
  DoFind(FALSE);
}

void CWedView::OnUpdateFindNext(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_pSearchDlg==NULL);
}

void CWedView::OnUpdateEditReplace(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(!IsReadOnly() && m_pSearchDlg==NULL);
}

void CWedView::InitCharWidthTable()
{
    m_nAveWidth=m_EditFont.AveCharWidth;
    if(!m_CharWidthTable) m_nSpcWidth=m_nTabWidth=m_nMaxWidth=m_nAveWidth;
    else {
	  CDC dc;
	  dc.CreateCompatibleDC(NULL);
	  dc.SelectObject(&m_EditFont.cf);
	  ::GetCharWidth(dc.m_hDC,0,0xFF,m_CharWidthTable);
      m_nSpcWidth=m_CharWidthTable[' '];
      m_nTabWidth=m_CharWidthTable['E'];
      m_nMaxWidth=m_nAveWidth;
	  for(int i=0;i<256;i++) if(m_nMaxWidth<m_CharWidthTable[i])
	    m_nMaxWidth=m_CharWidthTable[i];
	}
}

void CWedView::Initialize()
{
	CLogFont lf;
    //Initialization of CLineView static data only --
    m_cbBackground.CreateStockObject(WHITE_BRUSH);
    m_cbHighlight.CreateSolidBrush(RGB_SKYBLUE);
    m_cbFlag.CreateStockObject(LTGRAY_BRUSH);
    m_TextColor=RGB_BLACK;
    m_hPenGrid.CreatePen(PS_SOLID,0,RGB_LTGRAY);
	lf.Init("Arial",90,700);
    m_EditFont.Load(szEditFont,FALSE,&lf);
    m_CharWidthTable=(LPINT)malloc(256*sizeof(int));
    InitCharWidthTable();
    CMainFrame::LoadEditFlags(0);
    CMainFrame::LoadEditFlags(1);
}

void CWedView::Terminate()
{
    free(m_pTgtBuf); //null argument ignored
    //Uninitialize static data --
    m_EditFont.Save();
    if(m_CharWidthTable) {
      free(m_CharWidthTable);
      m_CharWidthTable=NULL;
    }
    CMainFrame::SaveEditFlags(0);
    CMainFrame::SaveEditFlags(1);
}

/////////////////////////////////////////////////////////////////////////////
// Overrides of CView and CLineView

void CWedView::OnInitialUpdate()
{

	m_uFlags=GetDocument()->m_EditFlags;
	m_nTabCols=m_uFlags>>8;
    m_nTabStop = (m_nTabCols+1) * m_nTabWidth;
	
	CLineView::OnInitialUpdate();
	   
	//Lets set the initial window size and position pased on the size
	//of the document and the possible existence of other windows.
	//We actually do this by resizing the parent frame which has not
	//yet been activated or made visible. (This will be done
	//by ActivateFrame(), which is one of the last operations of the
	//template's OpenDocumentFile()).
	
    //First, let's check if another view is active. If so we will minimize it
	//and use the same size and position for the new window --

    CRect rect;	
	CWnd *pTxtFrame=GetParentFrame();
    
    ((CTxtFrame *)pTxtFrame)->m_bClosed=TRUE;
    
	CWnd *pMdiClient=pTxtFrame->GetParent();
	    
	int iLines=m_nLineTotal;
	int iSrv=GetDocument()->m_bSrvFile!=0;
	
	if(iLines<MIN_WINDOW_LINES) iLines=MIN_WINDOW_LINES;
	else {
		if((UINT)iLines>CMainFrame::m_EditHeight[iSrv])
			iLines=CMainFrame::m_EditHeight[iSrv];
	}

	CSize size((CMainFrame::m_EditWidth[iSrv]&~E_SIZECHG)*m_nAveWidth,m_nHdrHeight+iLines*m_nLineHeight);
		
    //NOTE: We will use rect.right and rect.bottom for width and height --
	size.cx+=2*GetSystemMetrics(SM_CXFRAME)+GetSystemMetrics(SM_CXVSCROLL);
	size.cy+=2*GetSystemMetrics(SM_CYFRAME)+GetSystemMetrics(SM_CYSIZE)+
	  GetSystemMetrics(SM_CYHSCROLL);
	
	//Let's check that the frame size is reasonable given the mainframe's size--
	
	pMdiClient->GetClientRect(&rect);

	int offset=GetSystemMetrics(SM_CYCAPTION);
	rect.left=rect.right/3-offset;
	
	m_nWinOffset=GetWinPos(m_nEditWinPos,SIZ_EDITWINPOS,rect,size,offset);
	
	pTxtFrame->SetWindowPos(NULL,rect.left+offset,rect.top+offset,size.cx,size.cy,
	 SWP_HIDEWINDOW|SWP_NOACTIVATE);
}

BOOL CWedView::PreCreateWindow(CREATESTRUCT& cs)
{
   if(!CLineView::PreCreateWindow(cs)) return FALSE;
   cs.dwExStyle&=~(WS_EX_CLIENTEDGE|WS_EX_WINDOWEDGE);
   if(!m_bClassRegistered) {
      WNDCLASS wc;
      if(::GetClassInfo( AfxGetInstanceHandle(),cs.lpszClass,&wc))
      {
         wc.lpszClassName=(LPCSTR)szEditWndClass;
         wc.hCursor=::LoadCursor(NULL,IDC_IBEAM);
         wc.hbrBackground=NULL; //We will handle all background painting
         if(::RegisterClass(&wc)) m_bClassRegistered=true;
      }
   }
   if(m_bClassRegistered) cs.lpszClass=(LPCSTR)szEditWndClass;
   return TRUE;
}

void CWedView::GetLineMetrics(int& nLineMinWidth,int& nLineMaxWidth,int& nLineHeight,
    int& nHdrHeight,int& nMargin)
{
    //Recalculate line and header dimensions needed by CLineView.

    nLineMaxWidth=m_nMaxWidth*(CLineDoc::MAXLINLEN-CLineDoc::MAXLINTABS) +
                  m_nTabStop*CLineDoc::MAXLINTABS + CaretWidth();

    nLineMinWidth=m_nMaxWidth; //Min Horiz scroll

    nLineHeight=m_EditFont.LineHeight; // 1 line of text
    if(GridLines()) nLineHeight++;
    nMargin=m_nAveWidth/2;

    nHdrHeight=0;

#ifdef _SURVEY
    //nHdrHeight=TabLines()?nLineHeight:0;
#endif
}

void CWedView::OnDrawHdr(CDC* pDC)
{
#ifdef _SURVEY
#endif
}

CFont * CWedView::SelectTextFont(CDC* pDC)
{
   //This sets intitial state of CDC upon entry to OnDrawLine() and OnDrawHdr() --
   pDC->SelectObject(&m_hPenGrid);
   pDC->SetTextColor(m_TextColor);
   pDC->SetBkMode(TRANSPARENT);
   return pDC->SelectObject(&m_EditFont.cf);
}

int CWedView::NextTabPos(LPBYTE& pLine,int& len,int x)
{
    //Returns next tabstop, assuming x is 0 or at a tabstop --

    BOOL bTable=m_CharWidthTable!=NULL;
	while(len) {
      UINT c=(UINT)*pLine++;
	  len--;
      if(c=='\t') return x+m_nTabStop-(x%m_nTabStop);
      x+=bTable?m_CharWidthTable[c]:m_nAveWidth;
    }
    return 0;
}

void CWedView::OnDrawLine(CDC* pDC,LINENO nLine,CRect rect)
{
	int chr0,chr1;
	bool caretLine=m_bCaretShown && (m_nActiveLine==nLine);
	bool bGrayed=(nLine==m_nFlagLine); // || nLine==m_nFlagLine2);
	LPBYTE pLine=LinePtr(nLine);
    int len=*pLine++;
    int Xmax=rect.right;
    int Xmin=rect.left;

    if(GridLines()) rect.bottom--;
    if(caretLine) HideCaret();

    if(GetSelectedChrs(nLine,chr0,chr1)) {
      //Part of the line is selected. The selection includes the EOL
      //if chr1==INT_MAX. We will highlight the margin if chr0==0.
      //We will also highlight all characters whose line offsets
      //are >=chr0 and <=chr1. If chr1==INT_MAX, we'll show the EOL
      //as a highlighted region of width margin().

      //Compute rectangle for highlighted region --
      CRect hirect=rect;  //absolute (not client relative) clip rectangle
      if(chr0) {
        GetPosition(nLine,chr0,hirect.left);
        //For appearance sake, we will fill tab boxes with highlight --
        if(!TabLines() || pLine[chr0-1]!='\t') hirect.left+=Margin();
        if(hirect.left<Xmin) hirect.left=Xmin;
      }
      if(chr1==INT_MAX) hirect.right=Xmax;
      else {
        GetPosition(nLine,chr1,hirect.right);
        if(!TabLines() || pLine[chr1-1]!='\t') hirect.right+=Margin();
        if(hirect.right>Xmax) hirect.right=Xmax;
		}

      if(hirect.left<hirect.right) {
        if(Xmin<hirect.left) {
          rect.right=hirect.left;
          pDC->FillRect(rect,bGrayed?&m_cbFlag:&m_cbBackground);
        }
        pDC->FillRect(hirect,&m_cbHighlight);
        rect.left=hirect.right;
        rect.right=Xmax;
      }
    }

    if(rect.left<rect.right) pDC->FillRect(rect,bGrayed?&m_cbFlag:&m_cbBackground);

    if(len) {
      GetPosition(nLine,INT_MAX,rect.right);
      if(rect.right+Margin()>Xmin)
        pDC->TabbedTextOut(Margin(),rect.top,(LPSTR)pLine,len,1,(LPINT)&m_nTabStop,Margin());
      else len=0;
    }

    if(GridLines()) {
      pDC->MoveTo(Xmin,rect.bottom);
      pDC->LineTo(Xmax,rect.bottom);
    }
    if(TabLines() && len) {
        int x=0;
        while((x=NextTabPos(pLine,len,x)) && x<Xmax) {
          if(x>=Xmin) {
            pDC->MoveTo(x,rect.top);
            pDC->LineTo(x,rect.bottom);
          }
        }
    }

    if(caretLine) ShowCaret();
}

int CWedView::GetPosition(LINENO nLine,int nChr,long& xPos)
{
    //Return line-relative X coordinate position, xPos,
    //for the character whose offset in line nLine is nChr.
    //If nChr>(line length), set nChr to line length first.

    BOOL bTable=m_CharWidthTable!=NULL;
    LPBYTE pLine=LinePtr(nLine);
    if(nChr>*pLine) nChr=*pLine;
    int x=0;
    for(int i=nChr;i;i--) {
      UINT c=(UINT)*++pLine;
      if(c=='\t') x+=m_nTabStop-(x%m_nTabStop);
      else x+=bTable?m_CharWidthTable[c]:m_nAveWidth;
    }
    xPos=x;
    return nChr;
}

void CWedView::GetNearestPosition(LINENO nLine,int& nChr,long& xPos)
{
    // Determine position of the character cell whose left margin is nearest the
    // line-relative X position, xPos, in the specified line. The character cell
    // positions and widths are based on the active line's contents and current
    // value of m_nTabStop.
    int chr,x,c,xPrev;
    BOOL bTable=m_CharWidthTable!=NULL;
    LPBYTE pLine=LinePtr(nLine);
    int linlen=*pLine++;

    //Note: xPos = absolute position (when not scrolled horizontally)
    x=xPrev=c=0;

    for(chr=0;x<xPos && chr<linlen;chr++) {
         xPrev=x;
         c=(UINT)*pLine++;
         if(c=='\t') x+=m_nTabStop-(x%m_nTabStop);
         else x+=bTable?m_CharWidthTable[c]:m_nAveWidth;
    }
    if((x-xPos)>(c=='\t'?Margin():(xPos-xPrev))) {
      x=xPrev;
      chr--;
    }
    xPos=x;
    nChr=chr;
}

void CWedView::PositionCaret()
{
  if(m_bOwnCaret) {
    ::SetCaretPos(m_caretX-m_scrollX+Margin(),m_caretY);
    DisplayCaret(true);
  }
}

void CWedView::DisplayCaret(bool bOn)
{
  if(m_bOwnCaret) {
    if(m_bCaretVisible) {
      if(bOn) {
        if(!m_bCaretShown) ShowCaret();
      }
      else {
        if(m_bCaretShown) HideCaret();
      }
      m_bCaretShown=bOn;
    }
    else if(m_bCaretShown) {
      HideCaret();
      m_bCaretShown=false;
    }
  }
}

void CWedView::SetNextTabPosition(BOOL bPrev)
{
	//pPrev = 1 : Backtab due to a SHIFT-TAB or SHIFT-ENTER
	//      = 0 : ENTER key with EnterTabs()==TRUE;

	LPBYTE pLine=LinePtr(m_nActiveLine);
	int len=*pLine;
	LINENO line=m_nActiveLine;
	int c=m_nActiveChr;

	if(!bPrev) {
	  if(c==len && pLine[len]=='\t') {
	    ProcessCtrlN();
	    return;
	  }
	  //Move to next tabstop --
	  while(++c<=len) if(pLine[c]=='\t') goto _set;

	  //We are at the end of the line and there are no more tabstops
	  if(len && pLine[c-1]=='\t') {
	     SetPosition(line,len-1,POS_CHR|POS_NOTRACK);
	     DeleteChar();
	     if(line==m_nLineTotal) {
	       InsertEOL();
	       return;
	     }
	     c=0;
	     line++;
	  }
	  else {
	    SetPosition(line,c);
	    if(len) InsertChar(VK_TAB);
	    else InsertEOL();
	    return;
	  }
	}
	else {
	  //Move to previous tabstop
	  if(c) {
	    if(--c>len) c=len;
	  }
	  else {
	    if(line==1) return;
	    line--;
	    pLine=LinePtr(line);
	    c=*pLine;
	  }
	  while(c && pLine[c]!='\t') c--;
	}

_set:
	SetPosition(line,c);
}

void CWedView::OnChar(UINT nChar,UINT nRepCnt,UINT nFlags)
{
	if(!iscntrl(nChar)) {
		if(!IsReadOnly()) InsertChar(nChar);
		else {
			MessageBeep(-1);
			StatusMsg(IDS_ERR_READONLY);
		}
	}
}

void CWedView::ProcessCtrlN(void)
{
	LPBYTE pLine=LinePtr(m_nActiveLine);
	//If positioned at the last char of a line, and it is a tab,
	//first delete it before inserting the EOL --
	if(m_nActiveChr && *pLine==m_nActiveChr && pLine[m_nActiveChr]=='\t') {
	  SetPosition(m_nActiveLine,m_nActiveChr-1,POS_CHR|POS_NOTRACK);
	  DeleteChar();
	}
	BOOL bEmptyAbove=!m_nActiveChr;
	InsertEOL();
	if(bEmptyAbove) SetPosition(m_nActiveLine-1,0);
}

void CWedView::EraseLn(BOOL bEOL)
{
	 int len=LineLen(m_nActiveLine);       
     if(len || bEOL) {
    	SetPosition(m_nActiveLine,0);
    	UpdateDoc(CLineDoc::UPD_DELETE,NULL,len+bEOL);
    }
}

void CWedView::EraseWord()
{
	int nChr=m_nActiveChr+1;
	int len=GetStartOfWord(LinePtr(m_nActiveLine),nChr,1);
	UpdateDoc(CLineDoc::UPD_DELETE,NULL,len+1);
}

BOOL CWedView::ProcessVKey(UINT nChar,BOOL bShift,BOOL bControl)
{
    //Called by CLineDoc::OnKeyDown() --
    //Return FALSE if key is not processed here.

	if(IsReadOnly()) {
		if(nChar==VK_RETURN) OnChar('A',0,0);
		if(!bControl || nChar!='G') return bControl;
		//Allow GoToLine
	}

	switch (nChar)
	{
		case VK_INSERT:
			CMainFrame::ToggleIns();
			AdjustCaret(true);
			break;

	    case VK_DELETE:
	        if(!bControl) DeleteChar();
	        else EraseLn(FALSE); //Do not delete EOL
	        break;

	    case VK_TAB:
	         if(!bShift) InsertChar(VK_TAB);
	         else SetNextTabPosition(TRUE); //backtab
		     break;

		case 'G':
		case 'R':     
		case 'Y':
		case 'T':
		case 'N':	if(!bControl) return FALSE;
					if(nChar!='N') {
						 if(nChar=='Y') EraseLn(TRUE);
						 else if(nChar=='T') EraseWord();
						 else if(nChar=='G') OnGotoline();
						 else {
						   if(!IsSrvFile()) return FALSE;
						   OnEditReverse();
						 }
					     break;
					}

		case VK_RETURN:

		    if(bControl) {
		      //Let's inplement WordStar's CTRL-N behavior for CTRL-ENTER --
		      ProcessCtrlN();
		      break;
		    }
		    
		    if(EnterTabs()) {
				SetNextTabPosition(bShift);
				if(!m_nActiveChr && AutoSeq()) AutoSequence();
			}
			else InsertEOL();
		    break;

		default: return bControl;  //do not send CTRL keys to OnChar
    }
    return TRUE;
}

CPrjListNode * CWedView::FindName(CPrjDoc **ppDoc,LPCSTR name)
{
    CPrjDoc *pDoc=CPrjDoc::m_pReviewDoc;
	CPrjListNode *pNode=NULL;
	if(pDoc) pNode=pDoc->m_pRoot->FindName(name);
	if(!pNode) {
		CMultiDocTemplate *pTmp=CWallsApp::m_pPrjTemplate;
	    POSITION pos = pTmp->GetFirstDocPosition();
	    while(pos) {
	      pDoc=(CPrjDoc *)pTmp->GetNextDoc(pos);
		  if(pDoc && pDoc->IsKindOf(RUNTIME_CLASS(CPrjDoc)) &&
			pDoc!=CPrjDoc::m_pReviewDoc &&
			(pNode=pDoc->m_pRoot->FindName(name))) break;
	    }
	}
	*ppDoc=pDoc;
	return pNode;
}


void CWedView::OpenProjectFile()
{
	char buf[16];
	int c1, len;
	CPrjDoc *pDoc;
	CPrjListNode *pNode;

	if(!GetSelectedChrs(m_nActiveLine, c1, len)) return;
	PBYTE pLine=LinePtr(m_nActiveLine);
	len=*pLine++;
	PBYTE pb=pLine+c1; //*pb first char of selection
	while(pb>pLine && pb[-1]!='\t') {
		pb--; c1--;
	}
	pb=pLine+c1;
	while(pb<pLine+len && *pb!=':') pb++;
	if(*pb!=':') return;
	len=pb-(pLine+c1); //length of name
	if(len>15) return;
	memcpy(buf,pLine+c1, len);
	buf[len]=0;

	if(pNode=FindName(&pDoc, buf)) {
		m_nLineStart=atoi((char *)(pb+1));
		m_nCharStart=0;
		pDoc->OpenSurveyFile(pNode);
	}
}

void CWedView::OnLButtonDblClk(UINT nFlags, CPoint point)
{
    //Select current tab or space-delimited field.
    //Note that two WM_LBUTTONDOWN / WM_LBUTTONUP pairs have been sent --

    LPBYTE pLine=LinePtr(m_nActiveLine);
    int c0=m_nActiveChr;

    if(GetStartOfWord(pLine,c0,-1)) SetPosition(m_nActiveLine,c0);
    else ClearSelection();
    c0=m_nActiveChr+1;
    GetStartOfWord(pLine,c0,1);
    if(!m_bSelectTrailingWS) while(IsWS(pLine[c0])) c0--;
    SetPosition(m_nActiveLine,c0,POS_CHR,TRUE);
    
    if(GetDocument()->IsLstFile()) {
	    OpenProjectFile();
	}
}

void CWedView::OnSetFocus(CWnd* pOldWnd)
{
	CLineView::OnSetFocus(pOldWnd);

	//The next Find/Replace operation will operate on this window --
	m_pFocusView=this;

	ASSERT(!m_bOwnCaret);
	::CreateCaret(m_hWnd,NULL,CaretWidth(),m_nLineHeight);
	m_bOwnCaret=true;
	m_bCaretShown=false;
	PositionCaret();
}

void CWedView::OnKillFocus(CWnd* pNewWnd)
{
    ASSERT(m_bOwnCaret);
    StatusClear();
	::DestroyCaret();
	m_bOwnCaret=m_bCaretShown=false;
	CLineView::OnKillFocus(pNewWnd);
}

void CWedView::OnDestroy()
{
    //If another view is present, m_pFocusView will already have been
    //set to it in OnSetFocus() --
    if(m_pSearchDlg && m_pFocusView==this) {
      //If we are closing the last view, remove the Find/Replace dialog --
      m_pFocusView=NULL;
      m_pSearchDlg->PostMessage(WM_COMMAND,IDCANCEL,0L);
    }
	CLineView::OnDestroy();
}

static void UpdateStatusInt(CCmdUI *pCmdUI,char *format,UINT n)
{
  char temp[8];
  sprintf(temp,format,n);
  pCmdUI->SetText(temp);
  pCmdUI->Enable(TRUE);
}

void CWedView::OnUpdateCol(CCmdUI* pCmdUI)
{
  if(IsMinimized()) pCmdUI->Enable(FALSE);
  else UpdateStatusInt(pCmdUI,"%03u",(UINT)(m_caretX/m_nAveWidth+1));
}

void CWedView::OnUpdateLine(CCmdUI* pCmdUI)
{
  if(IsMinimized()) pCmdUI->Enable(FALSE);
  else {
    UpdateStatusInt(pCmdUI,"%05u",(UINT)m_nActiveLine);
    if(GetDocument()->m_bActiveChange && m_nActiveLine!=GetDocument()->m_nActiveLine) {
      GetDocument()->SaveActiveBuffer();
    }
  }
}

void CWedView::OnEditPaste()
{
   UpdateDoc(CLineDoc::UPD_CLIPPASTE);
}

void CWedView::OnUpdateEditPaste(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(!IsReadOnly() && ::IsClipboardFormatAvailable(CF_TEXT));
}

void CWedView::OnEditCut()
{
   UpdateDoc(CLineDoc::UPD_CLIPCUT);
}

void CWedView::OnEditCopy()
{
   UpdateDoc(CLineDoc::UPD_CLIPCOPY);
}

void CWedView::OnUpdateSel(CCmdUI* pCmdUI)
{
	BOOL bEnable=(pCmdUI->m_nID!=ID_EDIT_CUT || !IsReadOnly()) && !IsSelectionCleared();
	pCmdUI->Enable(bEnable);
	if(pCmdUI->m_nID==ID_INDICATOR_SEL)
		pCmdUI->SetText(bEnable?"SEL":"");
}

void CWedView::AdjustCaret(bool bShow)
{
	if(m_bOwnCaret) {
		//Adjust caret size --
		::DestroyCaret();
		::CreateCaret(m_hWnd,NULL,CaretWidth(),m_EditFont.LineHeight);
		m_bCaretShown=false;
		if(bShow) PositionCaret();
	}
}

void CWedView::OnUpdate(CView *pView,LPARAM lHint,CObject* pHint)
{
	// OnUpdate() is called by CDocument::UpdateAllViews() when
	// either the grid settings, font or a range of text has changed.
	if(lHint==LHINT_GRID) {
	    UINT uFlags=m_uFlags&~(E_ENTERTABS+E_AUTOSEQ);
	    m_uFlags=GetDocument()->m_EditFlags;
	    if(uFlags==(m_uFlags&~(E_ENTERTABS+E_AUTOSEQ))) return;
	    
		//Assume line dimensions have changed --
	    m_nTabCols=m_uFlags>>8;
        m_nTabStop = (m_nTabCols+1) * m_nTabWidth;
		lHint=LHINT_FONT;
    }
    else if(lHint==LHINT_FONT) {
        m_nTabStop = (m_nTabCols+1) * m_nTabWidth;
		AdjustCaret(false);
	}
    CLineView::OnUpdate(pView,lHint,pHint);
}

void CWedView::OnEditFind()
{
    DoFindReplaceDialog(TRUE); //find only
}

void CWedView::OnEditReplace()
{
    DoFindReplaceDialog(FALSE); //find and replace
}

void CWedView::ChangeFont()
{
	 if(!m_EditFont.Change(
	   CF_ANSIONLY|CF_NOOEMFONTS|CF_NOSIMULATIONS|CF_SCREENFONTS|CF_INITTOLOGFONTSTRUCT)) return;

     InitCharWidthTable();

	 //We must update views of ALL documents, not just the one attached.
	 CLineDoc *pd=((CMainFrame *)AfxGetMainWnd())->m_pFirstLineDoc;
	 while(pd) {
        pd->UpdateAllViews(NULL,LHINT_FONT,NULL); //Updates bars and caret positions for all views
	    pd=pd->m_pNextLineDoc;
	 }
}

/////////////////////////////////////////////////////////////////////////////
#ifdef _DEBUG
CLineDoc* CWedView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CLineDoc)));
	return (CLineDoc*) m_pDocument;
}
#endif

void CWedView::OnGotoline()
{
    CLineNoDlg dlg(NULL,m_nActiveLine,m_nLineTotal);
	if(dlg.DoModal()==IDOK) {
	  SetPosition(dlg.m_nLineNo,0);
	  SetFlagLine(dlg.m_nLineNo);
	}
}

void CWedView::OnEditAutoSeq()
{
   CLineDoc *pDoc=GetDocument();
   ASSERT(pDoc->IsSrvFile());
   pDoc->m_EditFlags^=E_AUTOSEQ; //Toggle auto-sequence mode
   pDoc->UpdateAllViews(NULL,LHINT_GRID);	
}

void CWedView::OnUpdateEditAutoSeq(CCmdUI* pCmdUI)
{
   BOOL bSrvFile=IsSrvFile();
   pCmdUI->Enable(bSrvFile=(bSrvFile && !IsReadOnly()));
   if(bSrvFile) pCmdUI->SetCheck((m_uFlags&E_AUTOSEQ)!=0);	
}

void CWedView::OnEditReverse()
{
	//Reverse positions of first two tokens if they don't contain ';' or '#' --
	//If the caret is within this ranch, move it to the start of the third
	//token (or EOL).
	
	#define MAX_LEN_REVERSE 64
	LPBYTE pLine=LinePtr(m_nActiveLine);
	int len=*pLine;
	int nWrd0,nWrd1,nWrd2;
	
	for(nWrd0=0;nWrd0<len && IsWS(pLine[nWrd0+1]);nWrd0++);
	nWrd1=nWrd0+1;
	if(nWrd1>=len) return;
	GetStartOfWord(pLine,nWrd1,1);
	
	//nWrd1 now points to the second word or is at EOL 
	//nWrd0 is the length of leading whitespace --
	if((nWrd2=nWrd1+1)>=len) return;
	
	GetStartOfWord(pLine,nWrd2,1);
	//Two words are present --
	//nWrd2 is the char offset of the third word or is at EOL.
	
	if(nWrd2>=MAX_LEN_REVERSE) return;
	pLine++;
	ClearSelection();
	
	//First, move the first word (including trailing WS to a buffer --
	char buf[MAX_LEN_REVERSE];
	char *p=buf;
	
	if(nWrd0) {
	  memcpy(p,pLine,nWrd0);
	  p+=nWrd0;
	}
	
	memcpy(p,pLine+nWrd1,nWrd2-nWrd1);
	memcpy(p+nWrd2-nWrd1,pLine+nWrd0,nWrd1-nWrd0);
	
	//Now let's exclude comments and commands --
	buf[nWrd2]=0;
	if(strchr(buf,SRV_CHAR_COMMENT) || strchr(buf,SRV_CHAR_COMMAND)) return;
	
	if((nWrd1=m_nActiveChr)<nWrd2) nWrd1=nWrd2;
	SetPosition(m_nActiveLine,0);
	UpdateDoc(CLineDoc::UPD_REPLACE,buf,nWrd2);
	//SetPosition(m_nActiveLine,nWrd1);
}

void CWedView::OnUpdateEditReverse(CCmdUI* pCmdUI)
{
   pCmdUI->Enable(!IsReadOnly() && IsSrvFile());
}

static int PASCAL GetSeqName(char *pName,LPBYTE pSrc,int len)
{
	if(len>NET_SIZNAME) return -1;
	memcpy(pName,pSrc,len);
	pName[len]=0;
	while(*pName && !isdigit((BYTE)*pName)) pName++;
	if(*pName) {
		len=atoi(pName);
		*pName++='0';
		char *p=pName;
		while(*p && isdigit((BYTE)*p)) p++;
		while(*p) {
			if(isdigit((BYTE)*p)) return -1;
			*pName++=*p++;
		};
		*pName=0;
		return len<INT_MAX?len:-1;
	}
	return -1;
}

static char * PASCAL StoreSeqName(char *buf,char *name,int seq)
{
	char numbuf[12];  //Good enough for LONG_MAX
	seq=sprintf(numbuf,"%u",seq);
	ASSERT(seq+strlen(name)<=NET_SIZNAME+2);
	for(;*name;name++) {
	  if(*name=='0') {
	    memcpy(buf,numbuf,seq);
	    buf+=seq;
	  }
	  else *buf++=*name;
	}
	*buf++='\t';
	return buf;
} 

void CWedView::AutoSequence()
{
  if(m_nActiveLine<=1) return;
  LPBYTE pLine=LinePtr(m_nActiveLine-1);
  int linlen=*pLine++;
  int nChr1,len1;
  int nChr2,len2;
  
  for(nChr1=0;nChr1<linlen && IsWS(pLine[nChr1]);nChr1++);
  for(nChr2=nChr1;nChr2<linlen && !IsWS(pLine[nChr2]);nChr2++);
  len1=nChr2-nChr1;
  
  for(;nChr2<linlen && IsWS(pLine[nChr2]);nChr2++);
  if(nChr2==linlen) return;
  for(len2=0;nChr2+len2<linlen && !IsWS(pLine[nChr2+len2]);len2++);

  char linbuf[2*NET_SIZNAME+4],name[NET_SIZNAME+2];  //space for tabs *and* increment!
  
  if((len1=GetSeqName(name,pLine+nChr1,len1))<0 ||
     (len2=GetSeqName(linbuf,pLine+nChr2,len2))<0 ||
     (len2-len1!=1 && len1-len2!=1) || strcmp(name,linbuf)) return;
  
  if(len1<len2) {
     len1=len2;
     len2++;
  }
  else {
     len2=len1;
     len1++;
  }
  len1=StoreSeqName(StoreSeqName(linbuf,name,len1),name,len2)-linbuf;
  UpdateDoc(CLineDoc::UPD_INSERT,linbuf,len1);
}
  
LRESULT CWedView::OnCommandHelp(WPARAM wNone, LPARAM lParam)
{
	AfxGetApp()->WinHelp(IsSrvFile()?7:17,HELP_CONTEXT);
	return TRUE;
}

void CWedView::OnZipBackup() 
{
	CPrjDoc *pDoc=CPrjDoc::m_pReviewDoc;
	if(!pDoc) {
		pDoc=((CWallsApp *)AfxGetApp())->GetOpenProject();
	}
	if(pDoc) pDoc->GetPrjView()->OnZipBackup();
}

void CWedView::OnRButtonUp(UINT nFlags, CPoint point) 
{
	if(IsSelectionCleared()) {
		CLineView::OnLButtonDown(0,point);
		CLineView::OnLButtonUp(0,point);
	}
	CMenu menu;
	if(menu.LoadMenu(IDR_EDIT_CONTEXT))
	{
		CMenu* pPopup = menu.GetSubMenu(0);
		ASSERT(pPopup != NULL);
		if(!IsLocateEnabled()) {
			pPopup->DeleteMenu(8, MF_BYPOSITION);
			pPopup->DeleteMenu(8, MF_BYPOSITION);
			pPopup->DeleteMenu(8, MF_BYPOSITION);
			pPopup->DeleteMenu(8, MF_BYPOSITION);
			pPopup->DeleteMenu(8, MF_BYPOSITION);
		}
		ClientToScreen(&point);
		pPopup->TrackPopupMenu(TPM_LEFTALIGN|TPM_RIGHTBUTTON,point.x,point.y,GetParentFrame());
	}
}

void CWedView::OnEditProperties() 
{
	extern BOOL hPropHook;
	CPrjDoc *pFindDoc;
	CPrjListNode *pNode=CPrjDoc::FindFileNode(&pFindDoc,GetPathName(),TRUE);

	if(pNode) {
		CPrjView *pView=pFindDoc->GetPrjView();
		pView->m_PrjList.SelectNode(pNode);
		if(!hPropHook) pView->OpenProperties();
	}
	else {
		CFileStatDlg dlg(GetPathName());
		dlg.DoModal();
	}
}

CPrjListNode * CWedView:: GetFileNode()
{
	CPrjDoc *pFindDoc;
    return CPrjDoc::FindFileNode(&pFindDoc,GetPathName(),FALSE);
}

BOOL CWedView::GetFirstTwoWords(char *buf)
{
	LPBYTE pLine=LinePtr(m_nActiveLine);
	int len=*pLine;
	int nWrd0,nWrd1;
	
	for(nWrd0=0;nWrd0<len && IsWS(pLine[nWrd0+1]);nWrd0++);
	nWrd1=++nWrd0;
	if(nWrd1>=len) goto _badline;

	GetStartOfWord(pLine,nWrd1,1);
	//pLine[nWrd1] is the last WS prior to the second word or is at EOL.
	//pLine[nWrd0] is the first char of the first word --
	if((++nWrd1)>=len) goto _badline;
	GetStartOfWord(pLine,nWrd1,1);
	//Two words are present --
	//pLine[nWrd1] is the last WS prior to the third word or is at EOL.
	if(!IsWS(pLine[nWrd1]) || (nWrd1-nWrd0)>=MAX_LEN_REVERSE) goto _badline;
	memcpy(buf,&pLine[nWrd0],nWrd1-nWrd0);
	buf[nWrd1-nWrd0]=0;
	if(parse_2names(buf,FALSE)) goto _badline;
	return TRUE;

_badline:
	AfxMessageBox(IDS_ERR_NEEDNAMEPAIR,MB_ICONINFORMATION);
	return FALSE;
}

void CWedView::OnLocateInGeom() 
{
	char buf[MAX_LEN_REVERSE+2];
	if(!GetFirstTwoWords(buf)) return;
	VERIFY(CPrjDoc::m_pErrorNode=GetFileNode());
	CPrjDoc::m_nLineStart=m_nActiveLine;
	if(CPrjDoc::FindVector(FALSE,2)) {
		if(GetParentFrame()->IsZoomed()) GetParentFrame()->ShowWindow(SW_RESTORE);

		//Assumes pPLT is positioned at vector that was found --
		DWORD rec=dbNTP.Position();
		pCV->GoToComponent(); //may change component (pCV->OnNetChg), update preview map, and refresh map frames!
		VERIFY(!dbNTP.Go(rec));
		CPrjDoc::PositionReview(); //position a specific traverse and update map frames, and float buttons
	}
	CPrjDoc::m_nLineStart=0;
	CPrjDoc::m_pErrorNode=NULL;
}

BOOL CWedView::IsLocateEnabled()
{
	BOOL bEnable=(CPrjDoc::m_pReviewNode!=NULL/* && CPrjDoc::m_pReviewNode->IsReviewable()==2*/);

	if(bEnable) {
		bEnable=FALSE;
		CPrjListNode *pNode=GetFileNode();
		while(pNode) {
			if(pNode==CPrjDoc::m_pReviewNode) {
				bEnable=TRUE;
				break;
			}
			if(pNode->IsFloating()) break;
			pNode=pNode->Parent();
		}
	}
	return bEnable;
}

BOOL CWedView::LocateVector()
{
	BOOL bRet=FALSE;
	char buf[MAX_LEN_REVERSE+2];

	if(GetFirstTwoWords(buf)) {
		VERIFY(CPrjDoc::m_pErrorNode=GetFileNode());
		CPrjDoc::m_nLineStart=m_nActiveLine;
		bRet=CPrjDoc::FindVector(FALSE,3);
		CPrjDoc::m_nLineStart=0;
		CPrjDoc::m_pErrorNode=NULL;
	}
	return bRet;
}

void CWedView::OnLocateOnMap() 
{
	if(LocateVector()) {
		CPrjDoc::LocateOnMap();
		ASSERT(pPV);
		if(GetParentFrame()->IsZoomed()) GetParentFrame()->ShowWindow(SW_RESTORE);
		pPV->GetParentFrame()->ActivateFrame();
	}
}

void CWedView::OnViewSegment()
{
	if(LocateVector()) {
		CPrjDoc::ViewSegment();
	}
}

void CWedView::OnVectorInfo()
{
	if(LocateVector()) {
		ASSERT(pPLT->vec_id);
		VERIFY(!dbNTV.Go((pPLT->vec_id+1)/2));
		CPrjDoc::VectorProperties();
	}
}

void CWedView::OnUpdatePfxNam(CCmdUI* pCmdUI)
{
	if(pCV) pCV->OnUpdatePfxNam(pCmdUI);
	else pCmdUI->Enable(FALSE);
}

void CWedView::OnEditUndo()
{
	GetDocument()->m_Hist.Undo();
}

void CWedView::OnUpdateEditUndo(CCmdUI *pCmdUI)
{
	CLineDoc *pDoc=GetDocument();
	pCmdUI->Enable(!IsReadOnly() && GetDocument()->m_Hist.IsUndoable());
}

void CWedView::OnEditRedo()
{
	GetDocument()->m_Hist.Redo();
}

void CWedView::OnUpdateEditRedo(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(!IsReadOnly() && GetDocument()->m_Hist.IsRedoable());
}

void CWedView::OnPopupWriteprotect()
{
	GetDocument()->ToggleWriteProtect();
}

void CWedView::OnUpdatePopupWriteprotect(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(IsReadOnly());
}

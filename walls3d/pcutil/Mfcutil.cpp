//<copyright>
// 
// Copyright (c) 1993,94
// Institute for Information Processing and Computer Supported New Media (IICM),
// Graz University of Technology, Austria.
// 
//</copyright>

//<file>
//
// Name:        mfcutil.cpp
//
// Purpose:     collection of MFC helper classes and functions
//               
//
// Created:            94    Thomas Dietinger
//
// Modified:     2 Mar 95    Thomas Dietinger
//
//
//
// Description:
//
//
//</file>

#include "../stdafx.h"

#include "mfcutil.h"
#include "pcstuff.h"
#include <direct.h>
#include <iostream>
#include <fstream>
using namespace std;

#ifdef LOCAL_DB_20
#else
#ifdef __DISPATCHER__
//#include <Dispatch/dispatcher.h>
#endif
#endif

#include "../utils/types.h"
#include "../apps/hglinks.h"

#include "mdiobjrec.h"

extern void cleanup(int i);

/////////////////////////////////////////////////////////////////////////////
// CViewerDocument

IMPLEMENT_SERIAL(CViewerDocument, CDocument, 0 /* schema number*/ )

#if defined(_MSC_VER) & defined(_DEBUG)
#define new DEBUG_NEW
#endif

#include "debug.h"

#ifdef LOCAL_DB_20
#undef __DISPATCHER__
#endif

CViewerDocument::CViewerDocument()
{
    hgObj_ = ObjectPtr();
    annotations_ = FALSE;
}

CViewerDocument::~CViewerDocument()
{
    if (hgObj_.Obj() && 
        hgObj_.Obj()->getReceivers() &&
        hgObj_.Obj()->getReceivers()->count())
    {
        ReceiverList* pRecList = hgObj_.Obj()->getReceivers();
        for (MDIObjReceiver* node = (MDIObjReceiver*)pRecList->getFirst(); 
             node; 
             node = (MDIObjReceiver*)pRecList->getNext(node)) 
        {
            if (node->getMDIObjInfo() && 
                node->getMDIObjInfo()->m_pDocument_ == this)
            {
                hgObj_.Obj()->removeReceiver(node);
                break;
            }
         }
    }
}

BOOL CViewerDocument::OnNewDocument()
{
    if (!CDocument::OnNewDocument())
        return FALSE;
    return TRUE;
}

BEGIN_MESSAGE_MAP(CViewerDocument, CDocument)
    //{{AFX_MSG_MAP(CViewerDocument)
        // NOTE - the ClassWizard will add and remove mapping macros here.
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


BOOL CViewerDocument::annotations()
{
    // check if there are annotations:
    return (boolean)hgObj_.Obj()->existAnnotations();
} // annotations


BOOL CViewerDocument::createLinkListFromFile(const char* pFileName)
{
#ifdef LOCAL_DB_20
	return false;
#else
    char buffer[1024];

    ifstream fs;
    fs.open(pFileName, ios::in);

    Document* hgDoc = (Document*)hgObj_.Obj();

    Object o = "";

    while ( !(fs.eof()) && !(fs.fail()) )
    {
        fs.getline(buffer, 1023);
        if (strlen(buffer))            
            o += RString(buffer) + "\n";
        else 
        {
            Anchor a(o);
//            BaseLinkNode* ln = makeLinkNode( &a, hgDoc );
            (hgDoc->linkList())->insert( &a, hgDoc );
            o = "";
        }
    }
    fs.close();
    return (fs.good() != 0);
#endif
}

/////////////////////////////////////////////////////////////////////////////
// CViewerDocument serialization

void CViewerDocument::Serialize(CArchive& ar)
{
    if (ar.IsStoring())
    {
        // TODO: add storing code here
    }
    else
    {
        // TODO: add loading code here
    }
}

/////////////////////////////////////////////////////////////////////////////
// CViewerDocument commands

// try to open a Hyper-G Link (*.HGL) file in addition to data file 
// in order to test correct link handling for stand-alone-viewers
BOOL CViewerDocument::OnOpenDocument(const char* pszPathName)
{
    if (!CDocument::OnOpenDocument(pszPathName))
        return FALSE;

    CString tmpName(pszPathName);
    int foundPoint = tmpName.ReverseFind('.');
    if ( (foundPoint != -1) && (foundPoint >= tmpName.GetLength() - 4) ) 
    {
        tmpName = tmpName.Left(foundPoint + 1);
        tmpName += "hgl";
    }
    createLinkListFromFile(tmpName);

    return TRUE;
}


ObjectPtr CViewerDocument::getCurrentObject()
{
//    ObjectPtr thisObject(hgObj_);
//    return thisObject;
    return hgObj_;
}


RString CViewerDocument::getLinkPosStr()
{
    return newLinkPos_;
}


/////////////////////////////////////////////////////////////////////////////
// CMDITemplate:
//

CMDITemplate::~CMDITemplate()
{
//    if (m_hMenuShared)
//    {
//        ::DestroyMenu(m_hMenuShared);
//        m_hMenuShared = NULL;
//    }
//    if (docInfo.m_pDocument_
}


CMDIObjectInfo* CMDITemplate::OpenNewObject(const char* pszPathName,    // document file name
                                            const char* pszTitle,       // window title
                                            int cmdShow,                // windows opening style
                                            ObjectPtr hgObj,            // related Hyper-G object
                                            ObjectPtr destAnch)         // related destination anchor
{
    docInfo_.m_pDocument_ = CreateNewDocument();
    if (docInfo_.m_pDocument_ == NULL)
    {
        TRACE0("CDocTemplate::CreateNewDocument returned NULL\n");
        AfxMessageBox(AFX_IDP_FAILED_TO_CREATE_DOC);
        return NULL;
    }
    ASSERT_VALID(docInfo_.m_pDocument_);
#ifdef __AMADEUS__
    ((CViewerDocument*)docInfo_.m_pDocument_)->hgObj_ = hgObj; 
    ((CViewerDocument*)docInfo_.m_pDocument_)->destAnch_ = destAnch; 
#endif

    BOOL bOldAuto = docInfo_.m_pDocument_->m_bAutoDelete;
    docInfo_.m_pDocument_->m_bAutoDelete = FALSE;   // don't destroy if something goes wrong
    docInfo_.m_pFrameWnd_ = CreateNewFrame(docInfo_.m_pDocument_, NULL);
    docInfo_.m_pDocument_->m_bAutoDelete = bOldAuto;
    if (docInfo_.m_pFrameWnd_ == NULL)
    {
        AfxMessageBox(AFX_IDP_FAILED_TO_CREATE_DOC);
        delete docInfo_.m_pDocument_;       // explicit delete on error
        docInfo_.m_pDocument_ = NULL;
        return NULL;
    }
    ASSERT_VALID(docInfo_.m_pFrameWnd_);

    if (pszPathName == NULL)
    {
        // create a new document - with default document name
/*#if (_MFC_VER > 0x0250)
		SetDefaultTitle(docInfo_.m_pDocument_);
#else*/
        UINT nUntitled = m_nUntitledCount + 1;

        CString strDocName;
        if (pszTitle)
            strDocName = CString(pszTitle);
        else
        {
            if (GetDocString(strDocName, CDocTemplate::docName) &&
                !strDocName.IsEmpty())
            {
                char szNum[8];
                wsprintf(szNum, "%d", nUntitled);
                strDocName += szNum;
            }
            else
            {
                // use generic 'untitled' - ignore untitled count
                VERIFY(strDocName.LoadString(AFX_IDS_UNTITLED));
            }
        }
        docInfo_.m_pDocument_->SetTitle(strDocName);
// #endif

        if (!docInfo_.m_pDocument_->OnNewDocument())
        {
            // user has be alerted to what failed in OnNewDocument
            TRACE0("CDocument::OnNewDocument returned FALSE\n");
            docInfo_.m_pFrameWnd_->DestroyWindow();
            docInfo_.m_pFrameWnd_ = NULL;
            return NULL;
        }

        // it worked, now bump untitled count
        m_nUntitledCount++;
    }
    else
    {
        // open an existing document
		BeginWaitCursor();
        if (!docInfo_.m_pDocument_->OnOpenDocument(pszPathName))
        {
            // user has be alerted to what failed in OnOpenDocument
            TRACE0("CDocument::OnOpenDocument returned FALSE\n");
            docInfo_.m_pFrameWnd_->DestroyWindow();
            docInfo_.m_pFrameWnd_ = NULL;
			EndWaitCursor();
            return NULL;
        }
#ifdef _MAC
		// if the document is dirty, we must have opened a stationery pad
		//  - don't change the pathname because we want to treat the document
		//  as untitled
		if (!docInfo_.m_pDocument_->IsModified())
#endif
			docInfo_.m_pDocument_->SetPathName(pszPathName);

		EndWaitCursor();
    }

    int oldCmdShow = AfxGetApp()->m_nCmdShow;
    if (cmdShow)
        AfxGetApp()->m_nCmdShow = cmdShow;

    InitialUpdateFrame(docInfo_.m_pFrameWnd_, docInfo_.m_pDocument_);

    if (cmdShow)
    {
        docInfo_.m_pFrameWnd_->ShowWindow(cmdShow);
        AfxGetApp()->m_nCmdShow = oldCmdShow;
    }

//    docInfo_.m_pView_ = (CView *)CWnd::GetFocus();

    // now try to find the corresponding view: should be the last in the list
    //  of open views in this document !
    docInfo_.m_pView_ = 0;
    POSITION viewPos = docInfo_.m_pDocument_->GetFirstViewPosition();
    while (viewPos)
        docInfo_.m_pView_ = docInfo_.m_pDocument_->GetNextView(viewPos);

    return &docInfo_;
}


BOOL CMDITemplate::LoadMenu( UINT nIDResource )
{
    if (DestroyMenu())
    {
        m_hMenuShared = ::LoadMenu(AfxGetResourceHandle(), MAKEINTRESOURCE(nIDResource));
        if (docInfo_.m_pFrameWnd_)
        {
            ((CMDIChildHelpWnd*)docInfo_.m_pFrameWnd_)->sharedMenu() = m_hMenuShared;
//            CMDIFrameWnd* pFrame = ((CMDIChildWnd*)docInfo_.m_pFrameWnd_)->GetMDIFrame();
            CMDIFrameWnd* pFrame = (CMDIFrameWnd*)((CMDIChildWnd*)docInfo_.m_pFrameWnd_)->GetMDIFrame();
            pFrame->m_hMenuDefault = m_hMenuShared;
        }

    }

    return m_hMenuShared != NULL ? TRUE : FALSE;
}


BOOL CMDITemplate::DestroyMenu()
{
    if (m_hMenuShared != NULL)
        return ::DestroyMenu(m_hMenuShared);
    else
        return TRUE;
}


void CApplication::DoBusyCursor(int nCode)
{
    // 0 => restore, 1=> begin, -1=> end
    ASSERT(nCode == 0 || nCode == 1 || nCode == -1);
    ASSERT(m_hbusyCursor_ != NULL);
    int oldCount_ = m_nBusyCursorCount_;
    m_nBusyCursorCount_ += nCode;

    if (m_nBusyCursorCount_ > 0)
    {
        HCURSOR hcurPrev;
        hcurPrev = ::SetCursor(m_hbusyCursor_);
        if (hcurPrev != NULL && hcurPrev != m_hbusyCursor_)
            m_hcurBusyCursorRestore_ = hcurPrev;
    }
    else
    {
        // turn everything off
        if (m_nBusyCursorCount_ != oldCount_)
        {
            ::SetCursor(m_hcurBusyCursorRestore_);
        }
        m_nBusyCursorCount_ = 0;     // prevent underflow
    }
}


CApplication::CApplication() : CWinApp()
{   
    m_pszPrivateProfileName[0] = '\0';
    m_nBusyCursorCount_ = 0;

// TODO: Port this feature to Macintosh
#ifdef _MAC
    strcpy(m_pszIniDir_,":");
#else
    GetCurrentDirectory(sizeof(m_pszIniDir_)-1, m_pszIniDir_);   // current working directory = ini-file dir.
#endif
}

BOOL CApplication::InitInstance()
{
// TODO: Port this feature to Macintosh
#ifdef _MAC
    strcpy(m_pszHomeDir_,":");
#else
    GetCurrentDirectory(sizeof(m_pszHomeDir_)-1, m_pszHomeDir_);
#endif

    ::GetPrivateProfileString(m_pszExeName, "HomeDir", m_pszHomeDir_, 
                              m_pszHomeDir_, sizeof(m_pszHomeDir_)-1, m_pszPrivateProfileName);

    if (m_pszHomeDir_[strlen(m_pszHomeDir_)-1] != '\\')
        strcat(m_pszHomeDir_, "\\");        // be sure that directory ends with a backslash !

// TODO: Port this feature to Macintosh
#ifndef _MAC
    SetCurrentDirectory(m_pszHomeDir_);     // set new home directory so that app can find req. binaries !
#endif

/*
    char helpPath[_MAX_PATH];
    _getcwd(helpPath, _MAX_PATH);            // get current working directory
    strcat(helpPath, "\\");
    strcat(helpPath, m_pszExeName);         // append help-filename
    strcat(helpPath, ".hlp");  
    m_pszHelpFilePath = _strdup(helpPath);
*/
    m_hbusyCursor_ = LoadCursor("BUSYCURSOR");

    return TRUE;
}


int CApplication::ExitInstance()
{
//    if (m_hbusyCursor_)
//        ::DeleteObject((HGDIOBJ)m_hbusyCursor_);

    return CWinApp::ExitInstance();
}


void CApplication::SetPrivateProfileName()
{
	strcpy(m_pszPrivateProfileName,m_pszHelpFilePath);
	ASSERT(*m_pszPrivateProfileName!=0);
	strcpy(m_pszPrivateProfileName+strlen(m_pszPrivateProfileName)-3,"INI");
    m_pszProfileName = _strdup(m_pszPrivateProfileName);
}


CString CApplication::GetPrivateProfileString(LPCSTR lpszSection, LPCSTR lpszEntry,
                                            LPCSTR lpszDefault /*= NULL */)
{
    ASSERT(lpszSection != NULL);
    ASSERT(lpszEntry != NULL);
    ASSERT(m_pszPrivateProfileName != NULL);

    if (lpszDefault == NULL)
        lpszDefault = "";                           // don't pass in NULL
        
    char szT[_MAX_PATH];
    szT[0] = '\0';
    
    ::GetPrivateProfileString(lpszSection, lpszEntry, lpszDefault, 
                              szT, sizeof(szT)-1, m_pszPrivateProfileName);
        
    return CString(szT);
} // GetPrivateProfileString


UINT CApplication::GetPrivateProfileInt(LPCSTR lpszSection, LPCSTR lpszEntry, int nDefault)
{
    ASSERT(lpszSection != NULL);
    ASSERT(lpszEntry != NULL);
    ASSERT(m_pszPrivateProfileName != NULL);

    return ::GetPrivateProfileInt(lpszSection, lpszEntry, nDefault, m_pszPrivateProfileName);
} // GetPrivateProfileInt


BOOL CApplication::WritePrivateProfileString(LPCSTR lpszSection, LPCSTR lpszEntry,
                                               LPCSTR lpszValue)
{
    ASSERT(lpszSection != NULL);
    ASSERT(lpszEntry != NULL);
    ASSERT(m_pszPrivateProfileName != NULL);

    return ::WritePrivateProfileString(lpszSection, lpszEntry, lpszValue, m_pszPrivateProfileName);
} // GetPrivateProfileString


BOOL CApplication::WritePrivateProfileInt(LPCSTR lpszSection, LPCSTR lpszEntry, int nValue)
{
    ASSERT(lpszSection != NULL);
    ASSERT(lpszEntry != NULL);
    ASSERT(m_pszPrivateProfileName != NULL);

    char text[40];  // just to be on the safe side ...
    wsprintf(text, "%d", nValue);

    return ::WritePrivateProfileString(lpszSection, lpszEntry, text, m_pszPrivateProfileName);
} // GetPrivateProfileInt


CString CApplication::getHomeDir()
{
    return m_pszHomeDir_;
}


CString CApplication::getIniDir()
{
    return m_pszIniDir_;
}


/////////////////////////////////////////////////////////////////////////////
// GUIControls

GUIControls* guiControls()
{
    CObject* myObject = AfxGetApp() && AfxGetApp()->m_pMainWnd ? AfxGetApp()->m_pMainWnd : 0;

    if ( myObject && myObject->IsKindOf( RUNTIME_CLASS( CMDIMainFrame ) ) )
        return ((CMDIMainFrame*)myObject)->guiControls_;
    else
    if ( myObject && myObject->IsKindOf( RUNTIME_CLASS( CSDIMainFrame ) ) )
        return ((CSDIMainFrame*)myObject)->guiControls_;
    else
    {   
        TRACE("Application mainframe NOT derived from CMDIMainFrame OR CSDIMainFrame !!");
        ASSERT(0);
        return 0;
    }
}


GUIControls::GUIControls()
{
    interrupt_ = 0;
    indicatorText_ = 0;
}


GUIControls::~GUIControls()
{
    delete indicatorText_;
}


// determines whether the execution of a certain action has been interrupted
boolean GUIControls::interrupted()
{
#ifdef __DISPATCHER__
    return Dispatcher::interrupted();
#else
    // give user chance to launch interrupt
    if (DoNothing(FALSE) == -1)
    {
        cleanup(0);
    }

    if (interrupt_)
    {
        interrupt_ = 0;     // reset flag for next operation
        return 1;
    }

    return 0;
#endif
}


boolean GUIControls::setInterrupt()
{
#ifdef __DISPATCHER__
    return Dispatcher::setInterrupt();
#else
    boolean oldVal = interrupt_;
    interrupt_ = 1;
    return oldVal;
#endif
}


boolean GUIControls::resetInterrupt()
{
#ifdef __DISPATCHER__
    return Dispatcher::resetInterrupt();
#else
    boolean oldVal = interrupt_;
    interrupt_ = 0;
    return oldVal;
#endif
}


// set indicator value (percentage)
void GUIControls::setProgIndicator(float percent)
{
    if (percent > 1.0f)
        percent = 1.0f;

    progIndicator_ = percent;

    showProgIndicator(percentageMethod);
}


// set indicator value
void GUIControls::setProgIndicator(unsigned int value)
{
    progIndicator_ = (float)value;

    showProgIndicator(valueMethod);
}


// set indicator value (integer counter)
void GUIControls::setProgIndicator(int count, int maxcount)
{
    if (!maxcount)      // special treatment, if we don't know maximum value !
    {
        progIndicator_ = 0.0f;
    }
    else
    {
        progIndicator_ = ((float)count)/((float)maxcount);
    }

    showProgIndicator(percentageMethod);
}


// set indicator value (text string)
void GUIControls::setProgIndicator(const char* text)
{
    delete indicatorText_;
    indicatorText_ = new char[strlen(text)+1];
    if (indicatorText_)
        strcpy(indicatorText_, text);

    showProgIndicator(textMethod);
}


void GUIControls::clearProgIndicator()
{
    showProgIndicator(clearMethod);
}


// indiactor methods: clearMethod ....... clear display
//                    textMethod ........ show text
//                    percentageMethod .. show percentage
//                    valueMethod ....... show integer value
//
void GUIControls::showProgIndicator(DisplayMethod method)
{
    if (AfxGetApp() && AfxGetApp()->m_pMainWnd)
    {
        CWnd* mainWnd = AfxGetApp()->m_pMainWnd;

        switch (method)
        {
            case clearMethod :
                mainWnd->SendMessage(WM_WRITE_STATUS, (UINT)method);
                break;

            case percentageMethod  :
                mainWnd->SendMessage(WM_WRITE_STATUS, (UINT)method, (LPARAM)&progIndicator_);
                break;

            case valueMethod :
                mainWnd->SendMessage(WM_WRITE_STATUS, (UINT)method, (LPARAM)(UINT)progIndicator_);
                break;

            case textMethod  :
                mainWnd->SendMessage(WM_WRITE_STATUS, (UINT)method, (LPARAM)indicatorText_);
                break;
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
// CMDIMainFrame

IMPLEMENT_DYNAMIC(CMDIMainFrame, CMDIFrameWnd)

BEGIN_MESSAGE_MAP(CMDIMainFrame, CMDIFrameWnd)
    //{{AFX_MSG_MAP(CMDIMainFrame)
	//ON_COMMAND(ID_HELP, OnHelp)
	//ON_COMMAND(ID_HELP_INDEX, OnHelpIndex)
	//ON_COMMAND(ID_CONTEXT_HELP, OnContextHelp)
	//ON_COMMAND(ID_HELP_USING, OnHelpUsing)
	//}}AFX_MSG_MAP
    // Global help commands
    ON_COMMAND(ID_DEFAULT_HELP, OnHelpIndex)
END_MESSAGE_MAP()

CMDIMainFrame::CMDIMainFrame() : CMDIFrameWnd()
{
    guiControls_ = new GUIControls();
}

CMDIMainFrame::~CMDIMainFrame()
{
    delete guiControls_;
}

/////////////////////////////////////////////////////////////////////////////
/*
void CMDIMainFrame::OnHelp() 
{
#if (_MFC_VER > 0x0250)
    CMDIFrameWnd::OnHelp();
#else
#endif
}

void CMDIMainFrame::OnHelpIndex() 
{
#if (_MFC_VER > 0x0250)
    CMDIFrameWnd::OnHelpIndex();
#else
#endif
}

void CMDIMainFrame::OnContextHelp() 
{
#if (_MFC_VER > 0x0250)
    CMDIFrameWnd::OnContextHelp();
#else
#endif
}

void CMDIMainFrame::OnHelpUsing() 
{
#if (_MFC_VER > 0x0250)
    CMDIFrameWnd::OnHelpUsing();
#else
#endif
}
*/
/////////////////////////////////////////////////////////////////////////////
// CSDIMainFrame

IMPLEMENT_DYNAMIC(CSDIMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CSDIMainFrame, CFrameWnd)
    //{{AFX_MSG_MAP(CSDIMainFrame)
	//ON_COMMAND(ID_HELP, OnHelp)
	//ON_COMMAND(ID_HELP_INDEX, OnHelpIndex)
	//ON_COMMAND(ID_CONTEXT_HELP, OnContextHelp)
	//ON_COMMAND(ID_HELP_USING, OnHelpUsing)
	//}}AFX_MSG_MAP
    // Global help commands
    ON_COMMAND(ID_DEFAULT_HELP, OnHelpIndex)
END_MESSAGE_MAP()

CSDIMainFrame::CSDIMainFrame() : CFrameWnd()
{
    guiControls_ = new GUIControls();
}

CSDIMainFrame::~CSDIMainFrame()
{
    delete guiControls_;
}

/////////////////////////////////////////////////////////////////////////////
/*
void CSDIMainFrame::OnHelp() 
{
#if (_MFC_VER > 0x0250)
    CFrameWnd::OnHelp();
#else
#endif
}

void CSDIMainFrame::OnHelpIndex() 
{
#if (_MFC_VER > 0x0250)
    CFrameWnd::OnHelpIndex();
#else
#endif
}

void CSDIMainFrame::OnContextHelp() 
{
#if (_MFC_VER > 0x0250)
    CFrameWnd::OnContextHelp();
#else
#endif
}

void CSDIMainFrame::OnHelpUsing() 
{
#if (_MFC_VER > 0x0250)
    CFrameWnd::OnHelpUsing();
#else
#endif
}
*/
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

void BeginBusyCursor()
{ 
    ((CApplication*)AfxGetApp())->BeginBusyCursor(); 
}

void EndBusyCursor()
{ 
    ((CApplication*)AfxGetApp())->EndBusyCursor(); 
}

void RestoreBusyCursor()
{ 
    ((CApplication*)AfxGetApp())->RestoreBusyCursor();
}






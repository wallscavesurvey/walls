//<copyright>
// 
// Copyright (c) 1993,94
// Institute for Information Processing and Computer Supported New Media (IICM),
// Graz University of Technology, Austria.
// 
//</copyright>

//<file>
//
// Name:        mfcutil.h
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


#ifndef HG_PCUTIL_MFCUTIL_H
#define HG_PCUTIL_MFCUTIL_H


//#if defined(__DISPATCHER__) || defined(__AMADEUS__)
//#include "apps/hgobject.h"
//#else
#include "hgdummy.h"
//#endif

#define WM_WRITE_STATUS (WM_USER + 556)

class GUIControls;

extern GUIControls* guiControls();

enum DisplayMethod { clearMethod = 0, percentageMethod = 1, 
                     valueMethod = 2, textMethod = 3};


/////////////////////////////////////////////////////////////////////////////
// CMDIObjectInfo:
//

struct CMDIObjectInfo
{
    CMDIObjectInfo() { m_pFrameWnd_ = 0; m_pDocument_ = 0; m_pView_ = 0; }

    CFrameWnd*  m_pFrameWnd_;
    CDocument*  m_pDocument_;
    CView*      m_pView_;
};


class CMDIChildHelpWnd : public CMDIChildWnd
{
public:
    HMENU& sharedMenu() { return m_hMenuShared; }
    void OnMDIActivate(BOOL bActivate, CWnd* pActivateWnd, CWnd*) 
    { CMDIChildWnd::OnMDIActivate(bActivate, pActivateWnd, NULL); } 
};


/////////////////////////////////////////////////////////////////////////////
// CMDITemplate:
//

class CMDITemplate : public CMultiDocTemplate
{
public:
    CMDITemplate( UINT nIDResource, CRuntimeClass* pDocClass, 
                  CRuntimeClass* pFrameClass, CRuntimeClass* pViewClass ) : 
                  CMultiDocTemplate( nIDResource, pDocClass, pFrameClass, pViewClass ) {}

    ~CMDITemplate();

    virtual CMDIObjectInfo* OpenNewObject(const char* pszPathName = NULL,   // document file name
                                          const char* pszTitle = NULL,      // window title
                                          int cmdShow = 0,                  // windows opening style
                                          ObjectPtr hgObj = ObjectPtr(),    // related Hyper-G object
                                          ObjectPtr destAnch = ObjectPtr());// related destination anchor

    BOOL LoadMenu( UINT nIDResource );
    BOOL DestroyMenu();

    CMDIObjectInfo docInfo_;

};


/////////////////////////////////////////////////////////////////////////////
// CMDITemplate:
//

class CApplication : public CWinApp
{
public:
    CApplication();

    void SetPrivateProfileName();

    CString GetPrivateProfileString(LPCSTR lpszSection, LPCSTR lpszEntry,
                                    LPCSTR lpszDefault = "" );
    UINT    GetPrivateProfileInt(LPCSTR lpszSection, LPCSTR lpszEntry, int nDefault = 0);
    BOOL    WritePrivateProfileString(LPCSTR lpszSection, LPCSTR lpszEntry, LPCSTR lpszValue);
    BOOL    WritePrivateProfileInt(LPCSTR lpszSection, LPCSTR lpszEntry, int nValue);

    void DoBusyCursor(int nCode);
    void BeginBusyCursor();
    void EndBusyCursor();
    void RestoreBusyCursor();

    CString getHomeDir();
    CString getBinDir()     { return getHomeDir(); }
    CString getIniDir();

    virtual BOOL InitInstance();
    virtual int  ExitInstance();

protected:
    char m_pszPrivateProfileName[_MAX_PATH];
    char m_pszIniDir_[_MAX_PATH];               // ini file directory
    char m_pszHomeDir_[_MAX_PATH];              // home directory where to look for binaries etc.
    HCURSOR m_hbusyCursor_;
    HCURSOR m_hcurBusyCursorRestore_;
    int m_nBusyCursorCount_;

};

/////////////////////////////////////////////////////////////////////////////
inline void CApplication::BeginBusyCursor()
{ 
    DoBusyCursor(1); 
}

inline void CApplication::EndBusyCursor()
{ 
    DoBusyCursor(-1); 
}

inline void CApplication::RestoreBusyCursor()
{ 
    DoBusyCursor(0); 
}



/////////////////////////////////////////////////////////////////////////////
// CViewerDocument document

class CViewerDocument : public CDocument
{
    DECLARE_SERIAL(CViewerDocument)
protected:
    CViewerDocument();          // protected constructor used by dynamic creation

// Attributes
public:
    ObjectPtr hgObj_;           // related Hyper-G object
    ObjectPtr destAnch_;        // related destination anchor

// Operations
public:
    virtual BOOL initDocument(const char* pszPathName) { return FALSE; }
    virtual ObjectPtr getCurrentObject();
    virtual RString getLinkPosStr();

    virtual BOOL annotations(); // returns whether we have annotations to this document

// Implementation
protected:
    virtual ~CViewerDocument();
    virtual void Serialize(CArchive& ar);   // overridden for document i/o
    virtual BOOL OnNewDocument();
    virtual BOOL OnOpenDocument(const char* pszPathName);

    BOOL createLinkListFromFile(const char* pFileName);

    RString newLinkPos_;        // position string for new link to be created
    BOOL annotations_;          // do we have annotations to this document?

    // Generated message map functions
protected:
    //{{AFX_MSG(CViewerDocument)
        // NOTE - the ClassWizard will add and remove member functions here.
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};



/////////////////////////////////////////////////////////////////////////////
// GUIControls

class GUIControls
{
public:
    GUIControls();
    ~GUIControls();

    virtual boolean interrupted();      // execution of certain action interrupted ?
    virtual boolean setInterrupt();
    virtual boolean resetInterrupt();

    virtual void setProgIndicator(float percent);           // set indicator value (percentage)
    virtual void setProgIndicator(unsigned int value);      // set indicator value (integer value)
    virtual void setProgIndicator(int count, int maxcount); // set indicator value (integer counter)
    virtual void setProgIndicator(const char* text);        // set indicator text
    virtual void clearProgIndicator();                      // clear indicator display
    float getProgIndicator() { return progIndicator_; }

protected:
    virtual void showProgIndicator(DisplayMethod method);

    boolean interrupt_;
    float   progIndicator_;
    char*   indicatorText_;
};


/////////////////////////////////////////////////////////////////////////////
// CMDIMainFrame

class CMDIMainFrame : public CMDIFrameWnd
{                                                                     
    DECLARE_DYNAMIC(CMDIMainFrame)
public:
    CMDIMainFrame();
    ~CMDIMainFrame();

    GUIControls* guiControls_;  // only pointer to enabled assigning of derived class

// Generated message map functions
protected:
	//{{AFX_MSG(CMDIMainFrame)
	//afx_msg void OnHelp();
	//afx_msg void OnHelpIndex();
	//afx_msg void OnContextHelp();
	//afx_msg void OnHelpUsing();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CSDIMainFrame

class CSDIMainFrame : public CFrameWnd
{
    DECLARE_DYNAMIC(CSDIMainFrame)
public:
    CSDIMainFrame();
    ~CSDIMainFrame();

    GUIControls* guiControls_;  // only pointer to enabled assigning of derived class

// Generated message map functions
protected:
	//{{AFX_MSG(CSDIMainFrame)
	//afx_msg void OnHelp();
	//afx_msg void OnHelpIndex();
	//afx_msg void OnContextHelp();
	//afx_msg void OnHelpUsing();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////


void BeginBusyCursor();
void EndBusyCursor();
void RestoreBusyCursor();

#endif

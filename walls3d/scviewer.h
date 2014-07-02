// scviewer.h : header file
// written in April 1995
// Gerbert Orasche
// Institute For Information Processing And Computer Supported New Media (IICM)
// Graz University Of Technology

#ifndef SC_VIEWER_SCVIEWER_H
#define SC_VIEWER_SCVIEWER_H

#ifndef __AFXEXT_H__
#include <afxext.h>
#endif

class CSceneViewer 
{

// Attributes
private:
  //char m_szActDirBuffer[256];

// Operations
public:
  // Hand cursor for showing links
  //HCURSOR linkCursor_;        
  //CSize m_csMagSize;
  int m_iUseTrueColor;
// Implementation
public:
  CSceneViewer();
  ~CSceneViewer();
  BOOL InitInstance();
  int  ExitInstance();
  //const char* GetPathBuffer();
};

#endif

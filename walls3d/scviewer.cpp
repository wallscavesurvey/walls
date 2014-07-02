// scviewer.cpp : implementation file
// written in April 1995
// Gerbert Orasche
// copyright: (c) 1994-95
// Institute For Information Processing And Computer Supported New Media (IICM)
// Graz University Of Technology

#include "stdafx.h"
#include "hgapp.h"
#include "scviewer.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPSViewer
// this class was created to do all initialization tasks


CSceneViewer::CSceneViewer()
{
}

CSceneViewer::~CSceneViewer()
{

}

BOOL CSceneViewer::InitInstance()
{
  // true color should be off at default
  // moved, because too late
  //m_iUseTrueColor=theApp.GetPrivateProfileInt("VRwebOptions","UseTrueColor",0);

  return TRUE;
}

int CSceneViewer::ExitInstance()
{
  //theApp.WritePrivateProfileInt("VRwebOptions","UseTrueColor",m_iUseTrueColor);
  return(0);
}

/*const char* CPSViewer::GetPathBuffer()
{
  return(m_szActDirBuffer);
}
*/

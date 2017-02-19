// scenedoc.cpp : Document class
// written in December 1994
// Gerbert Orasche
// copyright: (c) 1994-95
// Institute For Information Processing And Computer Supported New Media (IICM)
// Graz University Of Technology

#include "stdafx.h"
#include "ge3d.h"
#include "hgapp.h"
#include "camera.h"

#include "scenedoc.h"
#include "wscene.h"
#include "scenevw.h"
#include "pcutil/unzip.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

extern float backg_color[];

/////////////////////////////////////////////////////////////////////////////
// CSceneDoc

IMPLEMENT_DYNCREATE(CSceneDoc, CDocument)

BEGIN_MESSAGE_MAP(CSceneDoc, CDocument)
	//{{AFX_MSG_MAP(CSceneDoc)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSceneDoc construction/destruction

CSceneDoc::CSceneDoc()
{
	// create scene class
  m_pScene=new CWinScene3D(this);

  m_pszFileName=new char[512];
  m_pszFileName[0]=0;
  //SetMode(m_iDrawMode);
}

CSceneDoc::~CSceneDoc()
{
  m_pScene->clear();
  delete m_pScene;


  // clean up temp, if packed image
  if(m_iPacked)
  {
    _unlink(m_pszFileName);
    TRACE("deleting %s",m_pszFileName);
  }

  // free strings
  delete[] m_pszFileName;
}


/////////////////////////////////////////////////////////////////////////////
// CSceneDoc diagnostics

#ifdef _DEBUG
void CSceneDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CSceneDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CSceneDoc commands


BOOL CSceneDoc::OnOpenDocument(LPCTSTR lpszPathName) 
{
  // vars for unzipping
  CUnzip* pUnzip;
  char pUnzipFile[OUTNAME_SIZE];
  int iUnzipRes;
  // temporary path
  char pszTempPath[512];
  char szParseMess[768];
  FILE* hInputFile;
  int iRet;

	if (!CDocument::OnOpenDocument(lpszPathName))
		return(FALSE);
	
  pUnzip=new CUnzip;
  m_iPacked=FALSE;

#ifndef _MAC
  // look, if the data is compressed
  if(pUnzip->CheckMagic(lpszPathName)==DEFLATED)
  {
    GetTempPath(sizeof(pszTempPath),pszTempPath);
    if(strlen(pszTempPath) > 0 &&
       pszTempPath[strlen(pszTempPath)-1] != '\\' &&
       pszTempPath[strlen(pszTempPath)-1] != ':' )
    strcat(pszTempPath, "\\");
    m_iPacked=TRUE;

    iUnzipRes=pUnzip->Unzip(pUnzipFile,pszTempPath,".wrl");

    // did we have success?
    if(iUnzipRes==UNZIP_OK)
    {
      // OK, so we can go on with unzipped file
      strcpy(m_pszFileName,pUnzipFile);
    }
    else
      return(FALSE);
  }
  else
#endif // _MAC
  {
    // uncompressed postscript file
    strcpy(m_pszFileName,lpszPathName);
  }

  // free Unzip class (files are closed now!!)
  delete pUnzip;

	// read scene file from disk!
  TRACE("reading scene from -%s-\n",m_pszFileName);
  
  // write into parser output
  sprintf(szParseMess,"Parsing input file:%s\n",lpszPathName);
  m_pScene->errorMessage(szParseMess);


  //iRet=m_pScene->readScene(m_pszFileName);
  iRet=0;
#ifdef _MAC
  hInputFile=fopen("Macintosh HD:walls3d:cube.wrl","rb");
#else
  hInputFile=fopen(m_pszFileName,"r");    
#endif
  if(hInputFile)
  {
    iRet=m_pScene->readSceneFILE(hInputFile);
    /*int test;
    char buffer[1024];
    test=getc(hInputFile);
    //ungetc(test,hInputFile);
    fseek(hInputFile,0L,SEEK_SET);
    while(fgets(buffer,sizeof(buffer),hInputFile))
    ; */
    fclose(hInputFile);
  }
  else
    iRet=101;

  if(iRet)
  {
    AfxMessageBox("Error Reading Scene",MB_ICONSTOP|MB_OK);
    return(FALSE);
  }

	return(TRUE);
}

CWinScene3D* CSceneDoc::GetScene()
{
  return(m_pScene);
}

const char * CSceneDoc::GetFirstComment()
{
	return m_pScene->m_FirstComment; //DMcK
}

Camera* CSceneDoc::GetCamera()
{
  return(m_pScene->getCamera());
}

#ifdef _FILESAVE
// 140395
// Gerbert Orasche
// copies actual file to an alternate user defined
BOOL CSceneDoc::OnSaveDocument(LPCTSTR lpszPathName) 
{
  TRACE("On Save Document: %s\n",lpszPathName);

  CFileDialog saveDlg(FALSE,NULL,NULL, 
                OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST,
                "VRML files (*.wrl) | *.wrl | Hyper-G scenes (*.sdf) | *.sdf | All Files (*.*) | *.* ||",
                NULL );
    
  if(saveDlg.DoModal()==IDCANCEL)
    return(FALSE);

  CString csFName = saveDlg.GetPathName();

  TRACE("output filename:%s\n",csFName.GetBuffer(1));

  BeginWaitCursor();

  // we have only to copy 
  // conversion must be supplied in future
  BOOL bResult=CopyFile(m_pszFileName,csFName.GetBuffer(1),FALSE);

  if(!bResult)
    AfxMessageBox("Error at saving file",MB_OK);

  EndWaitCursor();
  return(bResult);
}
#endif

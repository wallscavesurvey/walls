// m_opengl.cpp : capsulates OpenGL contex handling for MESA OpenGL
// written in July 1995
// Gerbert Orasche
// copyright: (c) 1994-95
// Institute For Information Processing And Computer Supported New Media (IICM)
// Graz University Of Technology

#include "stdafx.h"
#include "hgapp.h"
#include "scviewer.h"

#pragma warning(disable:4244)
#include <gl\gl.h>
#include <gl\glu.h>
#include "scenedoc.h"
#include "scenevw.h"
#include "ge3d.h"
#include "scene3d.h"
#include "camera.h"
#include "color.h"
#include "vecutil.h"
#ifndef _MAC
//#include <wing/wing.h>
#endif


	
/////////////////////////////////////////////////////////////////////////////
// Static member data initialization


/////////////////////////////////////////////////////////////////////////////
// GL-related member functions
void CSceneView::GLCreateRGBPalette(HDC hDC)
{
  int iColors;

  iColors=::GetDeviceCaps(hDC,SIZEPALETTE);
  
  // we have to override for Mac!!!!!
#ifdef _MAC
  iColors=256;
#else
  if(theApp.scViewer_->m_iUseTrueColor==0)
    iColors=256;
#endif

  if(iColors<=256&&iColors>0)
  {
    // if we are creating a new palette delete the old one
    if(m_hPal!=NULL)
      return;
#ifdef _MAC
    {
      // here comes the Mac's palette alloc
      LOGPALETTE* pTempLog;
      int iCountRed, iCountGreen, iCountBlue,iIndex;
      pTempLog=(LOGPALETTE*)malloc(sizeof(LOGPALETTE)+255*sizeof(PALETTEENTRY));

      // we must design our WinG halftone palette
      pTempLog->palVersion=0x300;
      pTempLog->palNumEntries=256;
      iIndex=0;
      // Build the 6x6x6 cube for palette
      for(iCountRed=0;iCountRed<6;iCountRed++)
        for(iCountGreen=0;iCountGreen<6;iCountGreen++)
          for(iCountBlue=0;iCountBlue<6;iCountBlue++)
          {
            pTempLog->palPalEntry[iIndex].peRed=iCountRed*51;
            pTempLog->palPalEntry[iIndex].peGreen=iCountGreen*51;
            pTempLog->palPalEntry[iIndex].peBlue=iCountBlue*51;
            pTempLog->palPalEntry[iIndex].peFlags=0;
            iIndex++;
          }


      // now create palette
      m_hPal=CreatePalette(pTempLog);
      free(pTempLog);
    }
#else
    // create the WinG halfone palette
    m_hPal=WinGCreateHalftonePalette();
#endif
  }
} 

// 20.2.95
// gets color from component table
// obsolete for MESA
unsigned char CSceneView::GLComponentFromIndex(int i, UINT nbits, UINT shift)
{
  return(0);
}


// 2.3.95
// Destroys the rendering context, if there are no more references
void CSceneView::GLDestroy()
{
  // Before releasing we are flushing...
  glFlush();
  //Release the current GL rendering context and device context
  if(m_hmglrc)
  {
	  //nuke the rendering context
    WMesaDestroyContext(m_hmglrc);
    m_hmglrc=NULL;
    //delete m_pPal;
    if(m_hPal)
      DeleteObject(m_hPal);
  }	 
}

// 16.2.95
// Sets up the pixelformat
BOOL CSceneView::GLSetupPixelFormat(HDC hdc, PIXELFORMATDESCRIPTOR *pPFD)
{
  // not supported in MESA
  return FALSE;
}


// 020795
// sets up the GL rendering context
void CSceneView::GLSetupContext(CDC* pDC)
{
  GLboolean RGB_Flag;
  GLboolean DB_Flag;

  // Do we need palette??
  GLCreateRGBPalette(pDC->m_hDC);
  if(m_hPal)
    RGB_Flag=GL_FALSE;
  else
    RGB_Flag=GL_TRUE;

  // we need double buffer
  DB_Flag=GL_TRUE;
  //DB_Flag=GL_FALSE;
  
  m_hmglrc=WMesaCreateContext(m_hWnd,m_hPal,RGB_Flag,DB_Flag);

  // if we are painting via WING dithering is done there!
  if(m_hPal)
  {
    ::glDisable(GL_DITHER);
    //glEnable(GL_DITHER);
  }
}

void CSceneView::OnInitialUpdate() 
{
  CDC* pDC;
  static BOOL bStarted=FALSE;

  //theApp.scViewer_->m_iUseTrueColor=theApp.GetPrivateProfileInt("VRwebOptions","UseTrueColor",0);
  theApp.scViewer_->m_iUseTrueColor=0;
  //m_iDrawMode=ge3d_wireframe;
  m_iDrawMode=ge3d_flat_shading;
  // interactive mode is switched off (if same as normal drawing mode)
  m_iInteractiveMode=m_iDrawMode;

  pDC=GetDC();
  // no, so setup the context
  GLSetupContext(pDC);

  if(m_hmglrc==NULL)
  {
    AfxMessageBox("Fatal Error at getting Rendering Context",MB_OK|MB_ICONSTOP);
  }
  ReleaseDC(pDC);

  CView::OnInitialUpdate();

  //DMcK --	   
	//Let's show the initial view in a maximized state.
	//We actually do this by resizing the parent frame which has not
	//yet been activated or made visible. (This will be done
	//by ActivateFrame(), which is one of the last operations of the
	//template's OpenDocumentFile()).
	
    //First, let's check if another view is active. If so we will minimize it
	//and use the same size and position for the new window --

    //CRect rect;
    /*
    if(!bStarted) {
		CWnd *pMdiFrame=GetParentFrame();
		pMdiFrame->ShowWindow(SW_SHOWMAXIMIZED);
		bStarted=TRUE;
	}
	 */
	//CWnd *pMdiClient=pMdiFrame->GetParent();
	    
	//Let's check that the frame size is reasonable given the mainframe's size--
	//pMdiClient->GetClientRect(&rect);

	//pMdiFrame->SetWindowPos(NULL,rect.left+offset,rect.top+offset,size.cx,size.cy,
	 //SWP_HIDEWINDOW|SWP_NOACTIVATE);

}

// 130795
// make context current (don't need DC here!)
BOOL CSceneView::GLMakeCurrent(CDC* pDC)
{
  WMesaMakeCurrent(m_hmglrc,pDC->m_hDC);
  return TRUE;
}

// 130795
// test, if rendering context is valid
BOOL CSceneView::GLValidContext()
{
  if(m_hmglrc)
    return TRUE;
  else
    return FALSE;
}

// 130795
// swap buffers (don't need DC here!)
BOOL CSceneView::GLSwapBuffers(CDC* pDC)
{
  WMesaSwapBuffers(pDC->m_hDC);

  return TRUE;
}

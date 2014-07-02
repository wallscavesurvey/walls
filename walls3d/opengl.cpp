// opengl.cpp : capsulates OpenGL contex handling for MS OpenGL
// written in May 1995
// Gerbert Orasche
// copyright: (c) 1994-95
// Institute For Information Processing And Computer Supported New Media (IICM)
// Graz University Of Technology

#include "stdafx.h"
#include "pcutil/mfcutil.h"
#include "mainfrm.h"
#include "hgapp.h"

#include <gl\gl.h>
#include <gl\glu.h>
#include "ARB_Multisample.h"
#include "scenedoc.h"
#include "scenevw.h"
#include "ge3d.h"
#include "scene3d.h"
#include "camera.h"
#include "color.h"
#include "vecutil.h"

int bWglInit=0;
static const PIXELFORMATDESCRIPTOR st_pfd=
{
	sizeof (PIXELFORMATDESCRIPTOR),									// Size Of This Pixel Format Descriptor
	1,																// Version Number
	PFD_DRAW_TO_WINDOW |											// Format Must Support Window
	PFD_SUPPORT_OPENGL |											// Format Must Support OpenGL
	PFD_DOUBLEBUFFER,												// Must Support Double Buffering
	PFD_TYPE_RGBA,													// Request An RGBA Format
	24,																// Select Our Color Depth /*32? Nehe example*/
	0, 0, 0, 0, 0, 0,												// Color Bits Ignored
	0,																// Alpha Buffer /*1?*/
	0,																// Shift Bit Ignored
	0,																// No Accumulation Buffer
	0, 0, 0, 0,														// Accumulation Bits Ignored
	32,																// 32 Bit Z-Buffer (Depth Buffer) /*16?*/ 
	0,																// No Stencil Buffer
	0,																// No Auxiliary Buffer
	PFD_MAIN_PLANE,													// Main Drawing Layer
	0,																// Reserved
	0, 0, 0															// Layer Masks Ignored
};

/////////////////////////////////////////////////////////////////////////////
// Static member data initialization
  unsigned char CSceneView::m_threeto8[8] = {
    0, 0111>>1, 0222>>1, 0333>>1, 0444>>1, 0555>>1, 0666>>1, 0377
  };
  unsigned char CSceneView::m_twoto8[4] = {
    0, 0x55, 0xaa, 0xff
  };
  unsigned char CSceneView::m_oneto8[2] = {
    0, 255
  };
  int CSceneView::m_defaultOverride[13] = {
    0, 3, 24, 27, 64, 67, 88, 173, 181, 236, 247, 164, 91
  };

  PALETTEENTRY CSceneView::m_defaultPalEntry[20] = {
    { 0,   0,   0,    0 },
    { 0x80,0,   0,    0 },
    { 0,   0x80,0,    0 },
    { 0x80,0x80,0,    0 },
    { 0,   0,   0x80, 0 },
    { 0x80,0,   0x80, 0 },
    { 0,   0x80,0x80, 0 },
    { 0xC0,0xC0,0xC0, 0 },

    { 192, 220, 192,  0 },
    { 166, 202, 240,  0 },
    { 255, 251, 240,  0 },
    { 160, 160, 164,  0 },

    { 0x80,0x80,0x80, 0 },
    { 0xFF,0,   0,    0 },
    { 0,   0xFF,0,    0 },
    { 0xFF,0xFF,0,    0 },
    { 0,   0,   0xFF, 0 },
    { 0xFF,0,   0xFF, 0 },
    { 0,   0xFF,0xFF, 0 },
    { 0xFF,0xFF,0xFF, 0 }
  };


/////////////////////////////////////////////////////////////////////////////
// GL-related member functions
void CSceneView::GLCreateRGBPalette(HDC hDC)
{
  PIXELFORMATDESCRIPTOR pfd;
  int n;
  int iCount;
  LOGPALETTE* pBuffer;

  n=::GetPixelFormat(hDC);
  ::DescribePixelFormat(hDC,n,sizeof(PIXELFORMATDESCRIPTOR),&pfd);

  if(pfd.dwFlags & PFD_NEED_PALETTE)
  {
    n=1<<pfd.cColorBits;

    // if we are creating a new palette delete the old one
    if(m_hPal!=NULL)
      return;

    // allocate buffer for colortable
    pBuffer=(LOGPALETTE*)new(char[n*sizeof(PALETTEENTRY)+4]);
    pBuffer->palVersion=0x300;
    pBuffer->palNumEntries=n;

    // calculate colors
    for (iCount=0;iCount<n;iCount++)
    {
      pBuffer->palPalEntry[iCount].peRed=
            GLComponentFromIndex(iCount,pfd.cRedBits,pfd.cRedShift);
      pBuffer->palPalEntry[iCount].peGreen=
            GLComponentFromIndex(iCount, pfd.cGreenBits,pfd.cGreenShift);
      pBuffer->palPalEntry[iCount].peBlue=
            GLComponentFromIndex(iCount, pfd.cBlueBits,pfd.cBlueShift);
      pBuffer->palPalEntry[iCount].peFlags=0;
    }

        // fix up the palette to include the default GDI palette 
    if ((pfd.cColorBits == 8) &&
        (pfd.cRedBits   == 3) && (pfd.cRedShift   == 0) &&
        (pfd.cGreenBits == 3) && (pfd.cGreenShift == 3) &&
        (pfd.cBlueBits  == 2) && (pfd.cBlueShift  == 6))
    {
      for(iCount=1;iCount<=12;iCount++)
        pBuffer->palPalEntry[m_defaultOverride[iCount]]=m_defaultPalEntry[iCount];
    }

    m_hPal=CreatePalette(pBuffer);
    
    // release buffer
    delete[] pBuffer;
  }
} 

// 20.2.95
// gets color from component table
unsigned char CSceneView::GLComponentFromIndex(int i, UINT nbits, UINT shift)
{
  unsigned char val;

  val=(unsigned char)(i >> shift);
  switch(nbits)
  {
    case 1:
      val &= 0x1;
      return(m_oneto8[val]);
    case 2:
      val &= 0x3;
      return(m_twoto8[val]);
    case 3:
      val &= 0x7;
      return(m_threeto8[val]);
    default:
      return(0);
  }
}


// 2.3.95
// Destroys the rendering context, if there are no more references
void CSceneView::GLDestroy()
{
  // Before releasing we are flushing...
  glFlush();
  //Release the current GL rendering context and device context
  if(m_hglrc)
  {
    //make the rendering context not current.
    ::wglMakeCurrent(NULL, NULL);
	  //nuke the rendering context
    wglDeleteContext(m_hglrc);
    m_hglrc=NULL;
    //delete m_pPal;
    if(m_hPal)
      DeleteObject(m_hPal);
  }	 
}

void WglInit(CREATESTRUCT& cs)
{
	bWglInit=-1;
	HWND hWnd = CreateWindowEx(cs.dwExStyle, cs.lpszClass,
			cs.lpszName, cs.style, cs.x, cs.y, cs.cx, cs.cy,
			cs.hwndParent, cs.hMenu, cs.hInstance, cs.lpCreateParams);
	if(!hWnd) return;

	HGLRC hrc=0;
	HDC hdc=GetDC(hWnd);

	int pixelformat = ::ChoosePixelFormat(hdc, &st_pfd);

	if(!pixelformat || !::SetPixelFormat(hdc, pixelformat, &st_pfd)) {
		goto _destroy;
	}

	hrc=wglCreateContext(hdc);
	if(!hrc || !wglMakeCurrent (hdc, hrc)) {
		goto _destroy;
	}

	if(!WGLisExtensionSupported("WGL_ARB_multisample"))
		goto _destroy;

	// Get wgl address
	wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");	
	if (!wglChoosePixelFormatARB) 
	{
		goto _destroy;
	}

	if(InitMultisample(AfxGetInstanceHandle(),hWnd)) {
		bWglInit=1;
	}

_destroy:
#ifdef _DEBUG
	if(bWglInit!=1) {
		AfxMessageBox("Note: Multisampling is not supported by your graphics card.");
	}
#endif
	if(hrc) wglDeleteContext (hrc);	
	ReleaseDC(hWnd,hdc);
	::DestroyWindow(hWnd);
}

// Sets up the pixelformat
BOOL CSceneView::GLSetupPixelFormat(HDC hdc)
{
  BOOL bRet = TRUE;
  int pixelformat;

  if(bWglInit==1) {
	  if(InitMultisample(AfxGetInstanceHandle(),m_hWnd)) {
		pixelformat=arbMultisampleFormat;
	  }
	  else bRet=FALSE;
  }
  else {
	  if(!(pixelformat = ::ChoosePixelFormat(hdc,&st_pfd)))
		bRet = FALSE;
  }
  if(bRet) {
    GLCreateRGBPalette(hdc);
	if(!::SetPixelFormat(hdc, pixelformat,&st_pfd))
		bRet=FALSE;
  }

  return(bRet);
}

void CSceneView::GLSetupContext(CDC* pDC)
{
/*
#if 1
  PIXELFORMATDESCRIPTOR fdCreate;
  memset(&fdCreate,0,sizeof(PIXELFORMATDESCRIPTOR));
  fdCreate.nSize=sizeof(PIXELFORMATDESCRIPTOR);
  fdCreate.nVersion=1;
  fdCreate.dwFlags=PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER;
  //fdCreate.dwFlags=PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL;
  fdCreate.iPixelType=PFD_TYPE_RGBA;
  //fdCreate.cDepthBits=16;
  fdCreate.cDepthBits=32;
  fdCreate.cAccumBits=0;
  fdCreate.cColorBits=24;
#else
	PIXELFORMATDESCRIPTOR fdCreate=st_pfd;
#endif
*/

  // ChoosePixelFormat
  if(GLSetupPixelFormat(pDC->m_hDC))
  {
    if(m_hglrc=::wglCreateContext(pDC->m_hDC))
    {
      if (!::wglMakeCurrent(pDC->m_hDC,m_hglrc))
       {
         wglDeleteContext(m_hglrc);
         m_hglrc=NULL;
         AfxMessageBox("Error creating rendering context",MB_OK,0);
       }
       // if OK turn on dithering!
       glEnable(GL_DITHER);
    }
  }
  else 
  {
   AfxMessageBox("Error getting pixel format",MB_OK,0);
  }            
}

void CSceneView::OnInitialUpdate() 
{
	CDC* pDC;

	//m_iDrawMode=ge3d_wireframe;
	m_iDrawMode=ge3d_flat_shading;
	// interactive mode is switched off (if same as normal drawing mode)
	m_iInteractiveMode=m_iDrawMode;

    {
		const char *p=GetDocument()->GetFirstComment();
		if(*p) GetParent()->SetWindowText(p);
		else GetParent()->SetWindowText(GetDocument()->GetTitle());
	}

	pDC=GetDC();
	// so setup the context
	GLSetupContext(pDC);
	if(m_hglrc==NULL)
	{
		AfxMessageBox("Fatal Error at getting Rendering Context",MB_OK|MB_ICONSTOP);
	}
	ReleaseDC(pDC);

	CMDIChildWnd *pMdiChild=(CMDIChildWnd *)GetParentFrame();
	CMDIFrameWnd *pMdiFrame=(CMDIFrameWnd *)pMdiChild->GetParentFrame();

	if(!pMdiFrame->MDIGetActive()) {
		WINDOWPLACEMENT wp;
		pMdiChild->GetWindowPlacement(&wp);
		ASSERT(wp.length==sizeof(wp));
		wp.showCmd=SW_SHOWMAXIMIZED;
		VERIFY(pMdiChild->SetWindowPlacement(&wp));
		CSceneApp::m_bCmdLineOpen=TRUE;
	}

	CView::OnInitialUpdate();
}

// 130795
// make context current 
BOOL CSceneView::GLMakeCurrent(CDC* pDC)
{
  return ::wglMakeCurrent(pDC->m_hDC,m_hglrc);
}

// 130795
// test, if rendering context is valid
BOOL CSceneView::GLValidContext()
{
  if(m_hglrc)
    return TRUE;
  else
    return FALSE;
}

// 130795
// swap buffers
BOOL CSceneView::GLSwapBuffers(CDC* pDC)
{
  return ::SwapBuffers(pDC->m_hDC);
}

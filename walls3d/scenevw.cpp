// scenevw.cpp : implementation file
// written in December 1994
// Gerbert Orasche
// copyright: (c) 1994-95
// Institute For Information Processing And Computer Supported New Media (IICM)
// Graz University Of Technology

#include "stdafx.h"
#include "hgapp.h"
#include "mainfrm.h"

#include <gl\gl.h>
#include <gl\glu.h>
#include "ARB_Multisample.h"
#include "scenedoc.h"
#include "scenevw.h"                                                                                                
#include "ge3d.h"
#include "wscene.h"
#include "camera.h"
#include "color.h"
#include <math.h>
#include "vecutil.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

// mode "fly" (mouse position controls orientation, buttons speed) 

// dead zone: no movem. up to FlyDeadX_/FlyDeadY_ pixels away from middle  [20]
// FlySpeedInc_ 'speed' increment per mouse click                         [0.1]
// 'speed' is powered by 3, 1.0 = scene size
// mouse at left/right border = horicontal camera turn by FlyTurnHor_  [45 deg]
// mouse at top/bottom = vertical camera turn by FlyTurnVert_        [22.5 deg]

float CSceneView::m_fFlyDeadX=20.0F;
float CSceneView::m_fFlyDeadY=20.0F;
float CSceneView::m_fFlySpeedInc=0.1F;
//float CSceneView::m_fFlyTurnHor=M_PI/4.0F;
//float CSceneView::m_fFlyTurnVert=M_PI/8.0F;
#pragma warning(disable:4305)
float CSceneView::m_fFlyTurnHor=M_PI/8.0F;
float CSceneView::m_fFlyTurnVert=M_PI/16.0F;
float CSceneView::m_fWalkTurnHor3=4.0F*M_PI;
float CSceneView::m_fWalkTurnVert3=2.0F*M_PI;
float CSceneView::m_fWalkZoom=0.75F;
float CSceneView::m_fWalkZoom3=2.0F;
float CSceneView::m_fWalkFocal=0.5F;
float CSceneView::m_fFlipFocal=2.0F;
float CSceneView::m_fFlipZoom=1.0F;
float CSceneView::m_fFlipTurnHor=M_PI;
float CSceneView::m_fFlipTurnVert=M_PI/2.0F;

// mode "fly to" (targetted movement to point of interest) 
// movement per frame FlyToTran_ (fraction of current distance)          [0.15]
// rotation per frame FlyToRot_ (fraction of current angle)              [0.25]
// FlyToRot_ should be somewhat higher than FlyToTran_

float CSceneView::m_fFlyToTran=0.15F;
float CSceneView::m_fFlyToRot=0.25F;

// drag by window height = zooming by VelocityZoom_
// drag by window width = turn left-right by VelocityTurnHor_
// drag by window height = turn up-down by VelocityTurnVert_
// panning (horicontal/vertical translation) by VelocityPan_
// fast movements (3rd power): VelocityZoom3_,
// fast horicontal turn: VelocityTurnHor3_,
// fast vertical turn: VelocityTurnVert3_
// fast panning: VelocityPan3_

float CSceneView::m_fVelocityZoom=0.01F;
float CSceneView::m_fVelocityPan=0.1F;
float CSceneView::m_fVelocityTurnHor=M_PI/16.0F;
float CSceneView::m_fVelocityTurnVert=M_PI/32.0F;
float CSceneView::m_fVelocityZoom3=2.0F;
float CSceneView::m_fVelocityPan3=1.0F;
float CSceneView::m_fVelocityTurnHor3=4.0F*M_PI;
float CSceneView::m_fVelocityTurnVert3=2.0F*M_PI;


/////////////////////////////////////////////////////////////////////////////
// CSceneView

IMPLEMENT_DYNCREATE(CSceneView, CView)

BEGIN_MESSAGE_MAP(CSceneView, CView)
	//{{AFX_MSG_MAP(CSceneView)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEWHEEL()
	ON_WM_MOUSEMOVE()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_COMMAND(ID_VIEW_RESET, OnViewReset)
	ON_COMMAND(ID_NAVIGATION_FLY1, OnNavigationFly)
	ON_COMMAND(ID_NAVIGATION_FLYTO, OnNavigationFlyto)
	ON_COMMAND(ID_NAVIGATION_FLIP, OnNavigationFlip)
	ON_COMMAND(ID_NAVIGATION_WALK, OnNavigationWalk)
	ON_COMMAND(ID_NAVIGATION_HEADSUP, OnNavigationHeadsup)
	ON_UPDATE_COMMAND_UI(ID_NAVIGATION_KEEPTURNING, OnUpdateNavigationKeepturning)
	ON_COMMAND(ID_NAVIGATION_KEEPTURNING, OnNavigationKeepturning)
	ON_WM_KEYDOWN()
	ON_WM_KEYUP()
	ON_COMMAND(ID_NAVIGATION_INTERACTIVE_FLATSHADING, OnNavigationInteractiveFlatshading)
	ON_COMMAND(ID_NAVIGATION_INTERACTIVE_HIDDENLINE, OnNavigationInteractiveHiddenline)
	ON_COMMAND(ID_NAVIGATION_INTERACTIVE_SMOOTHSHADING, OnNavigationInteractiveSmoothshading)
	ON_COMMAND(ID_NAVIGATION_INTERACTIVE_WIREFRAME, OnNavigationInteractiveWireframe)
	ON_UPDATE_COMMAND_UI(ID_NAVIGATION_INTERACTIVE_FLATSHADING, OnUpdateNavigationInteractiveFlatshading)
	ON_UPDATE_COMMAND_UI(ID_NAVIGATION_INTERACTIVE_HIDDENLINE, OnUpdateNavigationInteractiveHiddenline)
	ON_UPDATE_COMMAND_UI(ID_NAVIGATION_INTERACTIVE_SMOOTHSHADING, OnUpdateNavigationInteractiveSmoothshading)
	ON_UPDATE_COMMAND_UI(ID_NAVIGATION_INTERACTIVE_WIREFRAME, OnUpdateNavigationInteractiveWireframe)
	ON_COMMAND(IDR_FLATSHADING, OnFlatshading)
	ON_COMMAND(IDR_HIDDENLINE, OnHiddenline)
	ON_COMMAND(IDR_SMOOTHSHADING, OnSmoothshading)
	ON_COMMAND(IDR_WIREFRAME, OnWireframe)
  ON_COMMAND(ID_VIEW_VRMLPARSEROUTPUT, OnShowParserOutput)
	ON_COMMAND(ID_VIEW_INTERACTIVE_ENABLE, OnViewInteractiveEnable)
	ON_UPDATE_COMMAND_UI(ID_VIEW_INTERACTIVE_ENABLE, OnUpdateViewInteractiveEnable)
	ON_UPDATE_COMMAND_UI(ID_NAVIGATION_FLIP, OnUpdateNavigationFlip)
	ON_UPDATE_COMMAND_UI(ID_NAVIGATION_FLY, OnUpdateNavigationFly)
	ON_UPDATE_COMMAND_UI(ID_NAVIGATION_FLYTO, OnUpdateNavigationFlyto)
	ON_UPDATE_COMMAND_UI(ID_NAVIGATION_HEADSUP, OnUpdateNavigationHeadsup)
	ON_UPDATE_COMMAND_UI(IDR_WIREFRAME, OnUpdateWireframe)
	ON_UPDATE_COMMAND_UI(IDR_SMOOTHSHADING, OnUpdateSmoothshading)
	ON_UPDATE_COMMAND_UI(IDR_HIDDENLINE, OnUpdateHiddenline)
	ON_UPDATE_COMMAND_UI(IDR_FLATSHADING, OnUpdateFlatshading)
	ON_UPDATE_COMMAND_UI(ID_NAVIGATION_WALK, OnUpdateNavigationWalk)
	ON_COMMAND(ID_VIEW_POLYGONS_AUTOMATIC, OnViewPolygonsAutomatic)
	ON_UPDATE_COMMAND_UI(ID_VIEW_POLYGONS_AUTOMATIC, OnUpdateViewPolygonsAutomatic)
	ON_COMMAND(ID_VIEW_POLYGONS_SINGLESIDED, OnViewPolygonsSinglesided)
	ON_UPDATE_COMMAND_UI(ID_VIEW_POLYGONS_SINGLESIDED, OnUpdateViewPolygonsSinglesided)
	ON_COMMAND(ID_VIEW_POLYGONS_TWOSIDED, OnViewPolygonsTwosided)
	ON_UPDATE_COMMAND_UI(ID_VIEW_POLYGONS_TWOSIDED, OnUpdateViewPolygonsTwosided)
	ON_COMMAND(ID_VIEW_LIGHTING_AUTOMATIC, OnViewLightingAutomatic)
	ON_UPDATE_COMMAND_UI(ID_VIEW_LIGHTING_AUTOMATIC, OnUpdateViewLightingAutomatic)
	ON_COMMAND(ID_VIEW_LIGHTING_LIGHTINGOFF, OnViewLightingLightingoff)
	ON_UPDATE_COMMAND_UI(ID_VIEW_LIGHTING_LIGHTINGOFF, OnUpdateViewLightingLightingoff)
	ON_COMMAND(ID_VIEW_LIGHTING_SWITCHON, OnViewLightingSwitchon)
	ON_UPDATE_COMMAND_UI(ID_VIEW_LIGHTING_SWITCHON, OnUpdateViewLightingSwitchon)
	ON_COMMAND(ID_TEXTURING, OnTexturing)
	ON_UPDATE_COMMAND_UI(ID_TEXTURING, OnUpdateTexturing)
	ON_COMMAND(ID_VIEW_LEVELVIEW, OnViewLevelview)
	ON_COMMAND(ID_VIEW_FRAMERATE, OnViewFramerate)
	ON_UPDATE_COMMAND_UI(ID_VIEW_FRAMERATE, OnUpdateViewFramerate)
	ON_COMMAND(ID_NAVIGATION_COLLISIONDETECTION, OnNavigationCollisiondetection)
	ON_UPDATE_COMMAND_UI(ID_NAVIGATION_COLLISIONDETECTION, OnUpdateNavigationCollisiondetection)
	ON_COMMAND(ID_NAVIGATION_FLY, OnNavigationFly)
	ON_COMMAND(ID_NAVIGATION_FREEROTATIONCONTROL, OnNavigationFreerotationcontrol)
	ON_UPDATE_COMMAND_UI(ID_NAVIGATION_FREEROTATIONCONTROL, OnUpdateNavigationFreerotationcontrol)
	//}}AFX_MSG_MAP
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CView::OnFilePrintPreview)
	ON_WM_MBUTTONDOWN()
	ON_WM_MBUTTONUP()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSceneView construction/destruction

CSceneView::CSceneView()
{
  //m_glptr=NULL;
  m_bLButton=FALSE;
  m_bRButton=FALSE;
  m_bMButton=FALSE;
  m_iInitialized=FALSE;
  m_hPal=NULL;
  m_bSizeChanged=FALSE;
  // standard mode is flipping objects
 	m_iNavigationMode=CSceneView::FLIP;
  // load initial help message
  m_csStatText.LoadString(IDS_STR_HELP_FLIP);

  m_hglrc=NULL;
  #ifdef __MESA__
  m_hmglrc=NULL;
  #endif
  m_bFlying=FALSE;
  m_fFlyspeed=0.0F;
  m_fHuIconpos[_icon_eyes]=72.0F;  // [_icon_body] + space + size 
  m_fHuIconpos[_icon_body]=24.0F;  // half space + half size 
  m_fHuIconpos[_icon_lift]=-24.0F;
  m_fHuIconpos[_icon_fly2]=-72.0F;
  initRGB(m_rgbColHudisplay,0.5F,0.0F,1.0F);  // purple
  m_iHuIcon=_icon_none;
  m_iWalkIcon=_icon_none;
  m_fDownx=0.0F;
  m_fDowny=0.0F;
  m_fDragx=0.0F;
  m_fDragy=0.0F;
  m_iPoiset=0;
  m_fktran=0.0F;
  m_fkrot=0.0F;
  m_bKeep=TRUE;
  m_iFlipMode=CSceneView::NONE;
  m_iFlyMode=CSceneView::NONE;
  m_iInteractEnabled=FALSE;
  m_iPolygons=Scene3D::twosided_auto;
  m_iLighting=Scene3D::lighting_auto;

  // init values for frame count
  m_lOldTime=0L;
  m_iFrameCnt=0;
  guiControls()->setProgIndicator("");
  m_bSlide=FALSE;
  m_bRecClipPLane=FALSE;
  m_bCollDetect=TRUE;
  m_bFreeRot=TRUE;
}

CSceneView::~CSceneView()
{
}

/////////////////////////////////////////////////////////////////////////////
// CSceneView drawing
// if iMode==0 then OS-redraw
// 1 left button
// 2 right button
// 3 both/middle button 
void CSceneView::DoDraw(int iMode,CPoint& cpNew)
{
  CSceneDoc* pDoc = GetDocument();
  ASSERT_VALID(pDoc);
  CDC* pDC;
  unsigned long lNewTime;

  pDC=GetDC();

  //TRACE("DoDraw Mode:%d\n",iMode);
  if(m_iFrameCnt)
  {
    unsigned long lTemp;
    GUIControls* pTemp=guiControls();
    char pBuffer[256];

    // get miliseconds since Windows was started!
    lNewTime=GetTickCount();
    lTemp=lNewTime-m_lOldTime;
    TRACE("frame rate: %ld\n",lTemp);
    if(lTemp>1000)
    {
      // sorry no frames per second, if more than one second
      sprintf(pBuffer,"time: %4.3f (<1 fps)",(float)lTemp/1000.0F);
      pTemp->setProgIndicator(pBuffer);
    }
    else
    {
      // calculate frames per second
      sprintf(pBuffer,"Time: %4.3fs (%1.2f fps)",(float)lTemp/1000.0F,1000.0F/(float)lTemp);
      pTemp->setProgIndicator(pBuffer);
    }
    m_lOldTime=lNewTime;
  }



  if(GLValidContext())
  {
    // do we need to realize a palette?
    if(m_hPal)
    { 
      SelectPalette(pDC->m_hDC,m_hPal,FALSE);
      int iReal=RealizePalette(pDC->m_hDC);
      TRACE("Colors changed:%d\n",iReal);
    }

    if (!m_iInitialized)
	  {
      // initialize ge3d library (multi matrix mode etc.)
      ge3d_init_();
      //ge3d_setbackgcolor(0.0F,0.0F,0.0F);

      // get initial rendering mode
      //m_iDrawMode=theApp.GetPrivateProfileInt("VRwebOptions","InitialMode",ge3d_flat_shading);
      m_iDrawMode=ge3d_flat_shading;
	  GetDocument()->GetScene()->mode(m_iDrawMode);

      // get initial interactive mode
      //m_iInteractiveMode=theApp.GetPrivateProfileInt("VRwebOptions","InteractiveMode",ge3d_wireframe);
	  m_iInteractiveMode=ge3d_flat_shading;
      GetDocument()->GetScene()->modeInteract(m_iInteractiveMode);

      m_iInitialized=TRUE;
    }

    if(!GLMakeCurrent(pDC))
    {
	    TRACE("Error was: %d\n",GetLastError());
      AfxMessageBox("Error making DC current",MB_OK,0);
      return;
    }

    // size changed, redefine viewport
    if(m_bSizeChanged)
    {
      ::glViewport(0,0,m_csSize.cx,m_csSize.cy);
      m_bSizeChanged=FALSE;
    }

    if(iMode!=REDRAW)
    {

      // are we in velocity mode?
      if(m_bFlying&&m_bKeep)
      {
        float dx=(float)(m_ptActual.x-m_ptDown.x)/(float)m_csSize.cx;
        float dy=(float)(m_ptActual.y-m_ptDown.y)/(float)m_csSize.cy;

        switch(m_iNavigationMode)
        {
          case CSceneView::FLY:
            // change mode buffer, if new mode
            if(iMode!=m_iFlyMode)
              m_iFlyMode=iMode;
            DrawFly(m_iFlyMode,cpNew);
          break;

          case CSceneView::FLYTO:
            DrawFlyto(iMode,cpNew);
          break;

          case CSceneView::FLIP:
          {
            DrawFlip(iMode,dx,dy);
            m_iFlipMode=iMode;
          }
          break;

          case CSceneView::WALK:
            hu_hold_button(m_iWalkIcon,dx,dy);
          break;

          case CSceneView::HEADSUP:
            hu_hold_button(m_iHuIcon,dx,dy);
          break;
        }
      }
      else
      {
        switch(m_iNavigationMode)
        {
          case CSceneView::FLY:
            // change mode buffer, if new mode
            if(iMode!=m_iFlyMode)
              m_iFlyMode=iMode;
            DrawFly(m_iFlyMode,cpNew);
          break;

          case CSceneView::FLYTO:
            DrawFlyto(iMode,cpNew);
          break;

          case CSceneView::FLIP:
          {
            float dx=(float)(m_ptActual.x-cpNew.x)/(float)m_csSize.cx;
            float dy=(float)(m_ptActual.y-cpNew.y)/(float)m_csSize.cy;

            DrawFlip(iMode,dx,dy);
            m_iFlipMode=iMode;
          }
          break;

          case CSceneView::WALK:
            DrawWalk(iMode,cpNew);
          break;

          case CSceneView::HEADSUP:
            DrawHeadsup(iMode,cpNew);
          break;
        }
      }
    }
    else
    {
      if(m_iInteractEnabled)
        GetDocument()->GetScene()->interact(0);
     }

    GetDocument()->GetScene()->draw();
    drawui();

    glFlush();

    if(!GLSwapBuffers(pDC))
      AfxMessageBox("Error swapping buffers!",MB_OK|MB_ICONSTOP,0);

    //  release the DC from rendering context
    #ifndef __MESA__
    if(!::wglMakeCurrent(NULL, NULL))
    {
	    TRACE("Error was: %d\n",GetLastError());
      AfxMessageBox("Error making DC not current",MB_OK|MB_ICONSTOP,0);
      return;
    }
    #endif
  }
  else
    AfxMessageBox("Error getting rendering context!",MB_OK|MB_ICONSTOP,0);

  ReleaseDC(pDC);
}


// 22.5.95
// Walk mode without velocity
void CSceneView::DrawWalk(int iMode,CPoint& cpNew)
{
  CSceneDoc* pDoc = GetDocument();
  ASSERT_VALID(pDoc);
  Camera* pcCamera=pDoc->GetCamera();

  // take action if there is one
  float fSize=GetDocument()->GetScene()->size();
  // calculate difference
  float dx=(float)(m_ptActual.x-cpNew.x)/(float)m_csSize.cx;
  float dy=(float)(m_ptActual.y-cpNew.y)/(float)m_csSize.cy;
  // calculate mouse position
  float drx=cpNew.x-m_csSize.cx/2.0F;
  float dry=(m_csSize.cy-cpNew.y)-m_csSize.cy/2.0F;
  float temp1=(drx-m_fDownx)/m_csSize.cx;
  float temp2=(m_fDragx-m_fDownx)/m_csSize.cx;
  float d3_x=temp1*temp1*temp1-temp2*temp2*temp2;
  temp1=(dry-m_fDowny)/m_csSize.cy;
  temp2=(m_fDragy-m_fDowny)/m_csSize.cy;
  float d3_y=temp1*temp1*temp1-temp2*temp2*temp2;

  switch(iMode)
  {
    // do nothing just redraw
    case CSceneView::NONE:
    break;

    case CSceneView::LEFT:
      pcCamera->rotate_camera_right(-dx*m_fFlyTurnHor+d3_x*m_fWalkTurnHor3);
      pcCamera->zoom_in((dy*m_fWalkZoom+d3_y*m_fWalkZoom3)*fSize,
                        pDoc->GetScene());
    break;

    case CSceneView::RIGHT:
      pcCamera->rotate_camera_right(-dx*m_fFlyTurnHor+d3_x*m_fWalkTurnHor3);
      pcCamera->rotate_camera_up(dy*m_fFlyTurnVert+d3_y*m_fWalkTurnVert3);
    break;

    case CSceneView::BOTH:
      pcCamera->translate (-dx,dy,pDoc->GetScene()->getWinAspect(),m_fWalkFocal*fSize,
        pDoc->GetScene());
    break;
  }
}

// 040595
// code for flying
void CSceneView::DrawFly(int iMode,CPoint& cpNew)
{
  TRACE("Drawfly: Mode %d flying: %d\n",iMode,m_bFlying);
  // left button do or stop translation

  if(m_bFlying)
    fly1_next_frame(iMode,cpNew);
}


void CSceneView::DrawFlyto(int iMode,CPoint& cpNew)
{
  // when left button was pressed set POI!
  if(iMode==LEFT)
  {
    CSceneDoc* pDoc = GetDocument();
    float fPickX=(float)cpNew.x/(float)m_csSize.cx;
    float fPickY=(float)(m_csSize.cy-cpNew.y)/(float)m_csSize.cy;
    m_iPoiset=(GetDocument()->GetScene()->pickObject(fPickX,fPickY,
       m_pPoitarget,m_pPoinormal)!=0);
    TRACE("Tried to set point %f - %f:%d\n",fPickX,fPickY,m_iPoiset);
  }
  // all other codes for flying (normal == NONE)
  else
  {
    if(m_iPoiset)
    {
      // now fly! 
      fly2_hold_button(NONE,cpNew);
    }
  }
}

void CSceneView::DrawHeadsup(int iMode,CPoint& cpNew)
{
  TRACE("DrawHeadsup x:%d y:%d\n",cpNew.x,cpNew.y);
  CSceneDoc* pDoc = GetDocument();
  ASSERT_VALID(pDoc);
  Camera* pcCamera=pDoc->GetCamera();

  // take action if there is one
  float fSize=GetDocument()->GetScene()->size();
  // calculate difference
  float dx=(float)(m_ptActual.x-cpNew.x)/(float)m_csSize.cx;
  float dy=(float)(m_ptActual.y-cpNew.y)/(float)m_csSize.cy;
  // calculate mouse position
  float drx=cpNew.x-m_csSize.cx/2.0F;
  float dry=(m_csSize.cy-cpNew.y)-m_csSize.cy/2.0F;
  float temp1=(drx-m_fDownx)/m_csSize.cx;
  float temp2=(m_fDragx-m_fDownx)/m_csSize.cx;
  float d3_x=temp1*temp1*temp1-temp2*temp2*temp2;
  temp1=(dry-m_fDowny)/m_csSize.cy;
  temp2=(m_fDragy-m_fDowny)/m_csSize.cy;
  float d3_y=temp1*temp1*temp1-temp2*temp2*temp2;


  switch(m_iHuIcon)
  {
    // 'eyes': turn left-right, bottom-top
    case _icon_eyes:  
      pcCamera->rotate_camera_right(-dx*m_fFlyTurnHor+d3_x*m_fWalkTurnHor3);
      pcCamera->rotate_camera_up(dy*m_fFlyTurnVert+d3_y*m_fWalkTurnVert3);
    break;

    // 'walk':  move forward-back, turn left-right
    case _icon_body:  
      // camera_->translate (delta_x, 0, scene_->getwinaspect (), WalkFocal_ * size_);
      pcCamera->rotate_camera_right(-dx*m_fFlyTurnHor+d3_x*m_fWalkTurnHor3);
      pcCamera->zoom_in((dy*m_fWalkZoom+d3_y*m_fWalkZoom3)*fSize,pDoc->GetScene());
    break;

    // 'pan' (earlier lift)
    case _icon_lift: 
      pcCamera->translate(-dx,dy,pDoc->GetScene()->getWinAspect(),
                         m_fWalkFocal*fSize,pDoc->GetScene());
    break;

    case _icon_fly2:
      if(m_iPoiset)
        fly2_hold_button(iMode,cpNew);
    break;
  }

  m_fDragx=drx;
  m_fDragy=dry;
}

void CSceneView::DrawFlip(int iMode,float dx,float dy)
{
  CSceneDoc* pDoc = GetDocument();
  ASSERT_VALID(pDoc);
  // take action if there is one
  float fSize=GetDocument()->GetScene()->size();

  switch(iMode)
  {
    // do nothing just redraw
    case CSceneView::NONE:
    break;

    case CSceneView::LEFT:
      pDoc->GetCamera()->translate(dx,-dy,pDoc->GetScene()->getWinAspect(),
                           fSize*m_fFlipFocal,pDoc->GetScene());
    break;

    case CSceneView::RIGHT:
      //pDoc->GetCamera()->zoom_in(-dy*FlipZoom_ * size_);
      pDoc->GetCamera()->zoom_in(-dy*fSize*m_fFlipZoom,pDoc->GetScene());
    break;

    case CSceneView::BOTH:
      // center of rotations is center of the scene
      point3D center;
      GetDocument()->GetScene()->getCenter(center);  
      pDoc->GetCamera()->rotate_camera(dx*m_fFlipTurnHor,-dy*m_fFlipTurnVert,center);
    break;
  }
}

//////////////////////////////////////////////////////////////////////////////////////
// routines for velocity control

// hu_hold_button
// called when velocityControl option is true for walk or heads up
// icon tells which function to do (_icon...; necessary for walk)
// dx, dy are the distances from the current point to press position
// (whole window width/height is 1.0)
void CSceneView::hu_hold_button(int icon,float dx,float dy)
{
  CSceneDoc* pDoc = GetDocument();
  ASSERT_VALID(pDoc);
  Camera* pcCamera=pDoc->GetCamera();
  float fSize=GetDocument()->GetScene()->size();

  float dx3=dx*dx*dx;
  float dy3=dy*dy*dy;

  switch (icon)
  {
    // 'eyes': turn left-right, bottom-top
    case _icon_eyes:  
      pcCamera->rotate_camera_right(dx*m_fVelocityTurnHor+dx3*m_fVelocityTurnHor3);
      pcCamera->rotate_camera_up(-dy*m_fVelocityTurnVert-dy3*m_fVelocityTurnVert3);
    break;
    // 'walk':  move forward-back, turn left-right
    case _icon_body:  
      pcCamera->rotate_camera_right(dx*m_fVelocityTurnHor+dx3*m_fVelocityTurnHor3);
      pcCamera->zoom_in((-dy*m_fVelocityZoom-dy3*m_fVelocityZoom3)*fSize,
                        pDoc->GetScene());
    break;
    // 'pan' (earlier lift)
    case _icon_lift: 
      pcCamera->translate (
        dx*m_fVelocityPan+dx3*m_fVelocityPan3,
        -dy*m_fVelocityPan-dy3*m_fVelocityPan3,
        pDoc->GetScene()->getWinAspect(),m_fWalkFocal*fSize,pDoc->GetScene());
    break;
    // not called for 'fly to'-icon
  }
} // hu_hold_button

// drawui
// draw ui components ("head mounted display" etc. atop the ge3d window)
void CSceneView::drawui()
{
  CSceneDoc* pDoc = GetDocument();
  ASSERT_VALID(pDoc);

/*  if (scene_->linksActive () && !scene_->anchorMotion ())  // pure pick mode
    return;*/

  //move_mode_ = scene_->navigationMode ();

  // be sure that SceneWindow::modeDrawsUI returns true
  // for movemodes that do drawings here

  /*if (!SceneWindow::modeDrawsUI (move_mode_))
    return;*/

  // for faster repaint also turns off z-buffer
  ge3d_setmode(ge3d_wireframe);

  // set up a simple orthogonal camera for 2d drawings:
  // screen coordinate units, origin at window center
  float vpwidth=(float)m_csSize.cx;
  float vpheight=(float)m_csSize.cy;
  float halfw=vpwidth/2.0F;
  float halfh=vpheight/2.0F;

  // do not set up color in walk mode
  if(m_iNavigationMode!=WALK)
  {
    ge3dLineColor(&(GetDocument()->GetScene()->col_hudisplay));
    ge3d_setlinestyle (0x7777);  // ----
  }

  // && dot3D (m_pPoinormal, m_pPoinormal) > 0.0
  if(m_iPoiset)  
  { 
    float fSize=GetDocument()->GetScene()->size();
    // still in 3D coordinate system
    const vector3D& n=m_pPoinormal;  // ass.: |m_pPoinormal| = 1

    ge3d_push_matrix();
    ge3d_translate(m_pPoitarget.x,m_pPoitarget.y,m_pPoitarget.z);
    if(fabs(n.z)<0.99999)
    {
      // double phi = atan2 (n.y, n.x);  // "x to y"
      ge3d_rotate_axis('z',atan2(n.y, n.x)*180.0F/M_PI);
    } 
    // otherwise normal vector is parallel to z-axis
    // double theta = acos (n.z);  // "z to x-y"
    ge3d_rotate_axis('y',acos(n.z)*180.0F/M_PI);
    ge3d_circle(0.0F,0.0F,fSize*0.05F);
    ge3d_circle(0.0F,0.0F,fSize*0.01F);
    float len=fSize*0.03F;
    ge3d_moveto(-len,0.0F,0.0F);
    ge3d_lineto(len,0.0F,0.0F);
    ge3d_moveto(0.0F,-len,0.0F);
    ge3d_lineto(0.0F,len,0.0F);
    ge3d_pop_matrix();
  }

  point3D eyepos,lookat;
  init3D (eyepos,0.0F,0.0F,1.0F);
  init3D (lookat,0.0F,0.0F,0.0F);
  ge3d_ortho_cam(&eyepos,&lookat,NULL,vpwidth,vpheight,0.0F,2.0F);

  // constants for fly_1
  const float cll=50.0F;  // cross line length
  const float sbw2=5.0F;  // half speed bar width
  const float sbh2=100.0F; // half speed bar height

  // speed bar origin
  float sbx=halfw-20.0F;
  float sby=-halfh+sbh2+20.0F;
  float temp;

  int i;
  static char speedlabel[] = "speed";
  char chs[2];  // string of 1 char
  chs [1]='\0';

  switch (m_iNavigationMode)
  {
    case WALK:
      // draw cross, when velocity control is enabled
      if(m_bKeep&&m_bFlying)
      {
        ge3d_setlinecolor(1.0F,0.0F,0.0F);  // red cross
        ge3d_line(m_fDownx-15.0F,m_fDowny,0.0F,m_fDownx+15.0F,m_fDowny,0.0F);
        ge3d_line(m_fDownx,m_fDowny-15.0F,0.0F,m_fDownx,m_fDowny+15.0F,0.0F);
      }
    break;


    case CSceneView::FLY:
      ge3d_setlinewidth(2);
      // cross indicating "dead zone"
      // use fast procedure for polylines of ge3d !!!@@@
      ge3d_moveto(m_fFlyDeadX+cll,m_fFlyDeadY,0.0F);  // upper right
      ge3d_lineto(m_fFlyDeadX,m_fFlyDeadY,0.0F);
      ge3d_lineto(m_fFlyDeadX,m_fFlyDeadY+cll,0.0F);
      ge3d_moveto(-m_fFlyDeadX-cll,m_fFlyDeadY,0.0F);  // upper left
      ge3d_lineto(-m_fFlyDeadX,m_fFlyDeadY,0.0F);
      ge3d_lineto(-m_fFlyDeadX,m_fFlyDeadY+cll,0.0F);
      ge3d_moveto(-m_fFlyDeadX-cll,-m_fFlyDeadY,0.0F);  // lower left
      ge3d_lineto(-m_fFlyDeadX,-m_fFlyDeadY,0.0F);
      ge3d_lineto(-m_fFlyDeadX,-m_fFlyDeadY-cll,0.0F);
      ge3d_moveto(m_fFlyDeadX+cll,-m_fFlyDeadY,0.0F);  // lower right
      ge3d_lineto(m_fFlyDeadX,-m_fFlyDeadY,0.0F);
      ge3d_lineto(m_fFlyDeadX,-m_fFlyDeadY-cll,0.0F);
      // circle and cross at center (decoration)
      ge3d_setlinewidth(1);
      ge3d_circle(0.0F,0.0F,m_fFlyDeadX*0.5F);
      ge3d_moveto(-m_fFlyDeadX,0.0F,0.0F);
      ge3d_lineto(m_fFlyDeadX,0.0F,0.0F);
      ge3d_moveto(0.0F,-m_fFlyDeadY,0.0F);
      ge3d_lineto(0.0F,m_fFlyDeadY,0.0F);
      // speed bar (in lower right window corner)
      ge3d_moveto(sbx,sby-sbh2,0.0F);
      ge3d_lineto(sbx,sby+sbh2,0.0F);
      ge3d_moveto(sbx-sbw2,sby+sbh2,0.0F);
      ge3d_lineto(sbx+sbw2,sby+sbh2,0.0F);
      ge3d_moveto(sbx-sbw2,sby-sbh2,0.0F);
      ge3d_lineto(sbx+sbw2,sby-sbh2,0.0F);
      // speed bar label (vertical)
      for(i=0;(*chs=speedlabel[i])!='\0';i++)
        ge3d_text(sbx-sbw2-5.0F,sby+sbh2-(i+1)*15,0.0F,chs);
      // speed bar thumb
      ge3d_setlinestyle(-1);  // solid
      ge3d_moveto(sbx-sbw2,sby,0.0F);
      ge3d_lineto(sbx+sbw2,sby,0.0F);
      ge3d_setlinecolor(1.0F,0.0F,0.0F);  // red
      ge3d_setlinewidth(3);
      temp=sby+m_fFlyspeed*sbh2;
      ge3d_moveto(sbx-sbw2,temp,0.0F);
      ge3d_lineto(sbx+sbw2,temp,0.0F);
      ge3d_setlinewidth(1);
    break;

    case CSceneView::HEADSUP:
      if(m_iHuIcon==_icon_fly2)  // hide icons during fly2
        break;

      // dashed, grey line
      ge3d_setlinestyle(-1);  // solid
      ge3d_setlinecolor(0.75F,0.75F,0.75F);

      // icon frames
      for(i=0;i<_icon_num;i++)
        ge3d_rect((float)-halficonsize_,m_fHuIconpos[i]-halficonsize_,
                    halficonsize_,m_fHuIconpos[i]+halficonsize_);

      //ge3d_setlinestyle(-1);  // solid

      ge3dLineColor(&m_rgbColHudisplay);

      float y0;
      // icon eyes
      y0=m_fHuIconpos[_icon_eyes];
      ge3d_setlinewidth(1);
      ge3d_arc(-8.0F,y0,6.0F,19.47F,160.53F);
      ge3d_arc(-8.0F,y0+4.0F,6.0F,199.47F,340.53F);
      ge3d_arc(8.0F,y0,6.0F,19.47F,160.53F);
      ge3d_arc(8.0F,y0+4.0F,6.0F,199.47F,340.53F);
      ge3d_setlinewidth(3);
      ge3d_circle(-8.0F,y0+2.0F,1.0F);
      ge3d_circle(8.0F,y0+2.0F,1.0F);

      // icon body
      y0=m_fHuIconpos[_icon_body];

      //left to right
      ge3d_moveto(8.0F,y0+3.0F,0.0F);
      ge3d_lineto(4.0F,y0+3.0F,0.0F);
      ge3d_lineto(-1.0F,y0+7.0F,0.0F);
      ge3d_lineto(-1.0F,y0-1.0F,0.0F);
      ge3d_lineto(3.0F,y0-6.0F,0.0F);
      ge3d_lineto(3.0F,y0-13.0F,0.0F);
      ge3d_moveto(-8.0F,y0-1.0F,0.0F);
      ge3d_lineto(-8.0F,y0+3.0F,0.0F);
      ge3d_lineto(-3.0F,y0+7.0F,0.0F);
      ge3d_lineto(-3.0F,y0-8.0F,0.0F);
      ge3d_lineto(-7.0F,y0-13.0F,0.0F);
      ge3d_circle(-2.0F,y0+12.0F,1.0F);

      y0=m_fHuIconpos[_icon_lift];
      // icon 'pan' (earlier icon lift)
      ge3d_setlinewidth(3);
      ge3d_moveto(-12.0F,y0,0.0F);
      ge3d_lineto(12.0F,y0,0.0F);  // hor
      ge3d_moveto(0.0F,y0-12.0F,0.0F);
      ge3d_lineto(0.0F,y0+12.0F,0.0F);  // vert
      ge3d_moveto(-8.0F,y0+4.0F,0.0F);
      ge3d_lineto(-12.0F,y0,0.0F);
      ge3d_lineto(-8.0F,y0-4.0F,0.0F);  // l
      ge3d_moveto(8.0F,y0+4.0F,0.0F);
      ge3d_lineto(12.0F,y0,0.0F);
      ge3d_lineto(8.0F,y0-4.0F,0.0F);  // r
      ge3d_moveto(-4.0F,y0-8.0F,0.0F);
      ge3d_lineto(0.0F,y0-12.0F,0.0F);
      ge3d_lineto(4.0F,y0-8.0F,0.0F);  // b
      ge3d_moveto(-4.0F,y0+8.0F,0.0F);
      ge3d_lineto(0.0F,y0+12.0F,0.0F);
      ge3d_lineto(4.0F,y0+8.0F,0.0F);  // t

      // icon fly2
      y0=m_fHuIconpos[_icon_fly2];
      ge3d_setlinewidth(2);
      ge3d_line(-2.0F,y0,0.0F,-12.0F,y0,0.0F);
      ge3d_line(2.0F,y0,0.0F,12.0F,y0,0.0F);
      ge3d_line(0.0F,y0-2.0F,0.0F,0.0F,y0-12.0F,0.0F);
      ge3d_line(0.0F,y0+2.0F,0.0F,0.0F,y0+12.0F,0.0F);
      ge3d_setlinewidth(1);
      ge3d_circle(0.0F,y0,8.0F);

      // dragged line
      if (m_iHuIcon!=_icon_none)  // && hu_icon_ != _icon_fly2)
      { 
        if(m_bFlying)
        {
          m_fDragx=m_ptActual.x-m_csSize.cx/2.0F;
          // mirror because of 0.0 in lower left corner
          m_fDragy=(m_csSize.cy-m_ptActual.y)-m_csSize.cy/2.0F;
        }
        ge3d_setlinecolor (1.0F,0.0F,0.0F);  // red
        ge3d_moveto(m_fDownx,m_fDowny,0.0F);
        ge3d_lineto(m_fDragx,m_fDragy,0.0F);
      }
    break;
  }
  ge3d_setlinestyle(-1);  // solid
} // drawui

void CSceneView::OnDraw(CDC* pDC)
{
  DoDraw(CSceneView::REDRAW,CPoint(0,0));
}

/////////////////////////////////////////////////////////////////////////////
// CSceneView printing

BOOL CSceneView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CSceneView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void CSceneView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}

/////////////////////////////////////////////////////////////////////////////
// CSceneView diagnostics

#ifdef _DEBUG
void CSceneView::AssertValid() const
{
	CView::AssertValid();
}

void CSceneView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CSceneDoc* CSceneView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CSceneDoc)));
	return (CSceneDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CSceneView message handlers

int CSceneView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
  // MFC OnCreate
  if(CView::OnCreate(lpCreateStruct) == -1)
    return -1;

  return(0);
}

BOOL CSceneView::PreCreateWindow(CREATESTRUCT& cs) 
{
  // the view windows style bits must include WS_CLIPSIBLINGS and
  // WS_CLIPCHILDREN so that the wgl functions will work.
  cs.style=cs.style|WS_CLIPSIBLINGS|WS_CLIPCHILDREN;
           //|CS_CLASSDC|CS_OWNDC|CS_PARENTDC;

  BOOL bRet=CView::PreCreateWindow(cs);

  if(bRet && !bWglInit) {
	  WglInit(cs);
  }
  
  return(bRet);
}

void CSceneView::OnDestroy() 
{
  CView::OnDestroy();
  GLDestroy();
}

void CSceneView::OnSize(UINT nType, int cx, int cy) 
{
  // resize outputwindow
  CSceneDoc* pDoc = GetDocument();
  RECT rect;

  TRACE("OnSize cx:%d cy:%d\n",cx,cy);
  GetClientRect(&rect);

  // tell scene class the aspect ratio
  if(cx>0&&cy>0)
    GetDocument()->GetScene()->setWinAspect((float)cx/(float)cy);

  // save size
  m_csSize.cx=cx;
  m_csSize.cy=cy;
  m_bSizeChanged=TRUE;

  if(m_iInteractEnabled)
    GetDocument()->GetScene()->interact(0);
}


BOOL CSceneView::OnEraseBkgnd(CDC* pDC) 
{
  return 1;
}

BOOL CSceneView::OnIdle(LONG lCount)
{
  // in flymodes call always!
  if(m_iNavigationMode==FLY&&m_bFlying)
  {
    DoDraw(CSceneView::NONE,m_ptActual);
    return(TRUE);
  }

  // in flyto mode call only selective
  if(m_iNavigationMode==FLYTO&&m_iPoiset)
  {
    if(m_fktran!=0.0||m_fkrot!=0.0)
    {
      DoDraw(CSceneView::NONE,m_ptActual);
      return(TRUE);
    }
    else
      return(FALSE);
  }
  // is velocity important?
  if(m_bKeep&&m_bFlying)
  {
    DoDraw(CSceneView::NONE,m_ptActual);
    return(TRUE);
  }

  // we do not need time anymore!
  ((CMainFrame*)AfxGetMainWnd())->m_wndStatusBar.SetPaneText(0,m_csStatText,TRUE);
  return(FALSE);
}

/////////////////////////////////////////////////////////////////////////////
// handling of mouse events
void CSceneView::OnRButtonDown(UINT nFlags, CPoint point) 
{
  TRACE("right button down\n");

  if(!m_bRButton)
  {
    m_bRButton=TRUE;
    m_ptDown=point;
    m_ptActual=point;
    SetCapture();
    if(m_iInteractEnabled)
      GetDocument()->GetScene()->interact(1);
    //GetDocument()->GetScene()->mode(m_iInteractiveMode);
  }

  
  m_fDragx=m_fDownx=point.x-m_csSize.cx/2.0F;
  // mirror because of 0.0 in lower left corner
  m_fDragy=m_fDowny=(m_csSize.cy-point.y)-m_csSize.cy/2.0F;

  if(m_iNavigationMode==CSceneView::WALK)
  {
    m_bFlying=TRUE;
    if((nFlags&MK_RBUTTON)&&(nFlags&MK_LBUTTON))
      m_iWalkIcon=_icon_lift;
    else
      if(nFlags&MK_RBUTTON)
        m_iWalkIcon=_icon_eyes;
  }
}

void CSceneView::OnRButtonUp(UINT nFlags, CPoint point) 
{
  TRACE("right button up\n");

  if(m_bRButton)
  {
    m_iWalkIcon=_icon_none;
    m_fDragx=m_fDownx=0.0F;
    m_fDragy=m_fDowny=0.0F;

    m_bRButton=FALSE;
    if(m_iNavigationMode!=CSceneView::FLY)
      m_bFlying=FALSE;
    if(!m_bLButton)
    {
      ReleaseCapture();

      // redraw if interactive or remove walk cross
      if(m_iInteractEnabled||m_iNavigationMode==WALK)
      {
        GetDocument()->GetScene()->interact(0);
        DoDraw(NONE,m_ptActual);
      }
    }
  }
  m_iWalkIcon=_icon_none;
}

void CSceneView::OnMButtonDown(UINT nFlags, CPoint point) 
{
  TRACE("middle button down\n");

/*  if(!m_bMButton)
  {
    m_bMButton=TRUE;
    m_ptActual=point;
    SetCapture();
  }

  if(m_iNavigationMode==CSceneView::WALK)
    if(nFlags&MK_MBUTTON)
        m_iWalkIcon=_icon_lift;
*/
}

void CSceneView::OnMButtonUp(UINT nFlags, CPoint point) 
{
  TRACE("middle button up\n");

/*  if(m_bMButton)
  {
    m_bMButton=FALSE;
    if(!(m_bLButton||m_bMButton))
      ReleaseCapture();
  }
  m_iWalkIcon=_icon_none;
*/
}

void CSceneView::OnLButtonDown(UINT nFlags, CPoint point) 
{
  TRACE("left button down\n");
  int iCount;

  if(!m_bLButton)
  {
    m_iHuIcon=_icon_none;
    m_iWalkIcon=_icon_none;
    m_fDragx=m_fDownx=0.0F;
    m_fDragy=m_fDowny=0.0F;
    m_iFlyMode=NONE;

    m_bLButton=TRUE;
    m_ptActual=point;
    m_ptDown=point;
    GetDocument()->GetScene()->interact(0);
  

    // in flying mode capture is toggled!!
    if(m_iNavigationMode!=FLY)
      SetCapture();

    // do we have to redraw??
    if(m_iInteractEnabled)
      DoDraw(NONE,m_ptActual);
  }

  m_fDragx=m_fDownx=point.x-m_csSize.cx/2.0F;
  // mirror because of 0.0 in lower left corner
  m_fDragy=m_fDowny=(m_csSize.cy-point.y)-m_csSize.cy/2.0F;

  switch(m_iNavigationMode)
  {
    case CSceneView::FLYTO:
      // setting POI only possible when rendering context is actual!
      // so simply ondraw is called with left button mode
      if(nFlags&MK_LBUTTON)
      {
        // no interactive at settin POI
        GetDocument()->GetScene()->interact(0);
        //GetDocument()->GetScene()->mode(m_iDrawMode);
        DoDraw(LEFT,point);
      }
    break;  // fly2


    case CSceneView::HEADSUP:
      // do we have to turn flying on?
      if(m_bKeep)
      {
        m_bFlying=TRUE;
      }

      if(m_iHuIcon==_icon_fly2)
      {
        // stop POI picking (also with double click) 
        if((nFlags&MK_SHIFT)||(nFlags&MK_CONTROL))
        { // inactive since meta-click is used for object info (more or less debugging feature)
          m_iHuIcon=_icon_none;
          m_iPoiset=0;
          //scene_->statusMessage (STranslate::str (STranslate::NavHintHEADSUP));
          //gecontext_->resetCursor ();
          // show icons
          DoDraw(NONE,m_ptActual);
        }
        else
        {
          //click_=0;  // do not select anchor with this click
          fly2_hold_button(nFlags,point);
        }
      } 
      else
      {
        m_iHuIcon=_icon_none;

        // check for user click on an icon
        if(fabs(m_fDownx)<=halficonsize_)
        {
          for(iCount=0;iCount<_icon_num;iCount++)
            if(fabs(m_fDowny-m_fHuIconpos[iCount])<=halficonsize_)
            {
              m_iHuIcon=iCount;
              TRACE("selected icon: %d\n",m_iHuIcon);
            }
        }

        if(m_iHuIcon==_icon_fly2)
        {
          // set mode flyto!
          OnNavigationFlyto();
          // hide icons
          DoDraw(NONE,m_ptActual);
        }
      } // heads up
    break;

    case CSceneView::FLY:
      //left ??
      if(nFlags&MK_LBUTTON)
      {
        // if we are flying stop it!
        if(m_bFlying)  
        {
          TRACE("fly off\n");
          m_bFlying=FALSE;
          m_iFlyMode=CSceneView::NONE;
          // switch off interact
          GetDocument()->GetScene()->interact(0);
          // reset fly speed
          m_fFlyspeed=0.0F;
          // release mouse
          ReleaseCapture();
        }
        // click inside cross to turn on flying
        else  
        {
          // calc window center
          float x0=(float)m_csSize.cx/2.0F;
          float y0=(float)m_csSize.cy/2.0F;
          // coordinates shifted to center
          float x=point.x-x0;
          float y=point.y-y0;  

          if(x>=-m_fFlyDeadX && x <= m_fFlyDeadX && y >= -m_fFlyDeadY && y <= m_fFlyDeadY)
          {
    	      m_bFlying=TRUE;
            TRACE("fly on\n");
            // switch to interactive
            if(m_iInteractEnabled)
              GetDocument()->GetScene()->interact(1);
            SetCapture();
    	      //gecontext_->setCursor (HgCursors::instance ()->aeroplane ());
            //scene_->selectObj(0)  // no selection during flight
    	    }
        }
      }  // left mouse button
    break;

    case CSceneView::WALK:
      m_bFlying=TRUE;
      if((nFlags&MK_RBUTTON)&&(nFlags&MK_LBUTTON))
      {
        m_iWalkIcon=_icon_lift;
        break;
      }

      if(nFlags&MK_LBUTTON)
      {
        m_iWalkIcon=_icon_body;
      }
    break;
  }
}

void CSceneView::OnLButtonUp(UINT nFlags, CPoint point) 
{
  TRACE("left button up\n");

  if(m_bLButton)
  {
    m_bLButton=FALSE;
    m_iHuIcon=_icon_none;
    m_iWalkIcon=_icon_none;
    m_fDragx=m_fDownx=0.0F;
    m_fDragy=m_fDowny=0.0F;

    // stop velocity mode
    if(m_iNavigationMode==CSceneView::FLY)
      return;

    m_bFlying=FALSE;
    TRACE("reset flying\n");
    m_iFlipMode=CSceneView::NONE;

    // restore right drawing mode in all navigation modes != FLY
    if(!m_bRButton)
    {
      ReleaseCapture();
      if(m_iInteractEnabled)
      {
        TRACE("different modes: redraw!\n");
        GetDocument()->GetScene()->interact(0);
        //GetDocument()->GetScene()->mode(m_iDrawMode);
        DoDraw(NONE,m_ptActual);
      }
      if(m_iNavigationMode==HEADSUP||m_iNavigationMode==WALK)
        DoDraw(NONE,m_ptActual);
    }
  }
}

// 12.12.94
// called when the mouse moves
// no middle mouse button support from microsoft :-((
void CSceneView::OnMouseMove(UINT nFlags, CPoint point) 
{
  CSceneDoc* pDoc = GetDocument();

  // set drawing mode
  if(m_iInteractEnabled)
    GetDocument()->GetScene()->interact(1);
  //pDoc->GetScene()->mode(m_iInteractiveMode);

  // if we got this move twice forget it!
  if((m_ptActual==point)||(!(m_bLButton||m_bRButton)&&(m_iNavigationMode!=FLY)))
    return;

  // in mode fly we must handle also mouse move without button pressed!
  if(m_iNavigationMode==FLY&&m_bFlying)
  {
    DoDraw(CSceneView::NONE,point);
    m_ptActual=point;
    return;
  }  

  // I can't use a guarded if here, so I used the ugly returns...
  // m_ptActual must be set AFTER coll to draw
  // performance forever!!!!!
  if(nFlags&MK_RBUTTON&&nFlags&MK_LBUTTON)
  {
    DoDraw(CSceneView::BOTH,point);
    m_ptActual=point;
    return;
  }

  if(nFlags&MK_LBUTTON)
  {
    DoDraw(CSceneView::LEFT,point);
    m_ptActual=point;
    return;
  }

  if(nFlags&MK_RBUTTON)
  {
    DoDraw(CSceneView::RIGHT,point);
    m_ptActual=point;
    return;
  }
}

BOOL CSceneView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	if(zDelta) {
		CRect client;
		GetClientRect(&client);

		CPoint point=m_ptActual=client.CenterPoint();

		if(zDelta>0)
			point.y+=80;
		else
			point.y-=80;

		DoDraw(CSceneView::RIGHT,point);

		return TRUE;
	}
	return FALSE;
}

// fly1_next_frame
// does one step of movement/rotation in mode "fly I"
// event.pointer_[xy] and flyspeed_ determine the kind of movement
// returns 1 iff a redraw is necessary
// assumes gecontext_ and camera_ non null

void CSceneView::fly1_next_frame(int iMode,CPoint& cpNew)
{
  TRACE("fly1_next: Mode %d\n",iMode);
  if(!m_bFlying)
    return;

  CSceneDoc* pDoc = GetDocument();
  ASSERT_VALID(pDoc);

  // calc window center
  float x0=(float)m_csSize.cx/2.0F;
  float y0=(float)m_csSize.cy/2.0F;
  // coordinates shifted to center
  float x=m_ptActual.x-x0;
  float y=(m_csSize.cy-m_ptActual.y)-y0;  


  if(x<-m_fFlyDeadX || x>m_fFlyDeadX)  // horicontal turn
  {
    if(x>0)
      x-=m_fFlyDeadX;
    else
      x+=m_fFlyDeadX;
    x/=(x0-m_fFlyDeadX);  // 0 < x < 1

    // left to right (around position)
    pDoc->GetCamera()->rotate_camera_right(x*m_fFlyTurnHor);
  }

  if(y<-m_fFlyDeadY||y>m_fFlyDeadY)  // vertical turn
  {
    if(y>0)
      y-=m_fFlyDeadY;
    else  
      y+=m_fFlyDeadY;
    y/=(y0-m_fFlyDeadY);  // 0 < y < 1

    // up/down (around position)
    pDoc->GetCamera()->rotate_camera_up(y*m_fFlyTurnVert);
  }
  
  // zoom (nonlinear)
  if(m_fFlyspeed)  
  {
    float fSize=GetDocument()->GetScene()->size();

    pDoc->GetCamera()->zoom_in(m_fFlyspeed*m_fFlyspeed*m_fFlyspeed*fSize,
                               pDoc->GetScene());
  }

} // fly1_next_frame

// fly2_hold_button
// moves towards/away from point of interest while middle/right button_ is held
// also turn camera towards -m_pPoinormal
// redraws window if necessary
// assumes gecontext_ and camera_ non null

void CSceneView::fly2_hold_button(int iMode,CPoint& cpNew)
{
  CSceneDoc* pDoc = GetDocument();
  ASSERT_VALID(pDoc);
  Camera* pcCamera=pDoc->GetCamera();
  ASSERT(pcCamera);
  float fx=cpNew.x/m_csSize.cx;
  float fy=(m_csSize.cy-cpNew.y)/m_csSize.cy;

  // if no point was set...
  if (!m_iPoiset)
    return;

  float hither2=pDoc->GetCamera()->gethither();

  // square distance of near clipping plane
  hither2*=hither2;  

  // default: translation only
  // shift  : both translation and rotation (adjusting)
  // control: rotation (adjusting) only

  BOOL do_trans=TRUE;
  BOOL do_rotat=FALSE;
  if(m_iFlyMode==LEFT)
    do_rotat=TRUE;
  if(m_iFlyMode==RIGHT)
  {
    do_trans=FALSE;
    do_rotat=TRUE;
  }

  TRACE("do_rot:%d do_tran: %d mode:%d\n",do_rotat,do_trans,m_iFlyMode);

  if(do_rotat)
  {
    // rotate position around hit (towards -m_pPoinormal)
    point3D position;
    vector3D pos_hit;
    vector3D direction;
    point3D lookat;
    vector3D look_pos;

    pcCamera->getposition(position);
    sub3D(position, m_pPoitarget, pos_hit);
    pcCamera->getlookat(lookat);
    // line of sight
    sub3D(lookat,position,look_pos);  

    // ass.: |m_pPoinormal| == 1
    float norm_p_h=norm3D(pos_hit);
    // direction = |pos_hit| * m_pPoinormal - pos_hit
    lcm3D(norm_p_h,m_pPoinormal,-1,pos_hit,direction);

    // pos_hit += k * direction
    pol3D (pos_hit, m_fkrot, direction,pos_hit);  
    float temp=norm3D(pos_hit);  // new norm

    // preserve norm of pos_hit
    if(temp>0.0F)  
    {
      norm_p_h/=temp;
      scl3D(pos_hit,norm_p_h);
    }
    norm_p_h=temp;

    // new position
    add3D(pos_hit,m_pPoitarget,position);
    pcCamera->setposition(position);
  
    // rotate lookat around position (towards -m_pPoinormal)
    // direction = -m_pPoinormal - look_pos / |look_pos|
    temp=norm3D(look_pos);  // norm of look_pos
    if(temp> 0.0F)
    {
      temp=1.0F/temp;
      pol3D(m_pPoinormal,temp,look_pos,direction);
      scl3D(direction,-1.0F);
      // look_pos += k * direction
      pol3D(look_pos,m_fkrot,direction,look_pos);
    }

    //new norm
    temp=norm3D(look_pos);
    // set distance between position and lookat to 1.0
    if(temp)  
    { 
      temp=1/temp;
      scl3D(look_pos,temp);
    }

    //camera_->setlookat (m_pPoitarget);  // just a test
    add3D(look_pos,position,lookat);
    // new lookat
    pcCamera->setlookat (lookat);
  } // rotation

  if(do_trans)
  {
    // translation (towards m_pPoitarget)
    vector3D hit_pos;
    pcCamera->getposition(hit_pos);

    // hit_pos = POI hit - eye position
    sub3D(m_pPoitarget,hit_pos,hit_pos);  
    if(m_fktran<0.0F||(1-m_fktran)*(1-m_fktran)*dot3D(hit_pos, hit_pos)>hither2)
    {
     // move away or still away from near clipping plane
     // k * (POI - eye)
      scl3D(hit_pos,m_fktran);
      pcCamera->translate(hit_pos,0,m_bSlide,m_bRecClipPLane);
    }
  } // translation
} // fly2_hold_button



// 30.5.95
// fly and flyto are controlled by the plus and minus button on keyboard!
void CSceneView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{

  switch (nChar) 
  {
    case VK_ADD:
      // check, if key is interesting
      if(m_iNavigationMode==FLY)
      {
        // increase speed
        m_fFlyspeed+=m_fFlySpeedInc;
        if(m_fFlyspeed>1.0F)
          m_fFlyspeed=1.0F;
        DoDraw(NONE,m_ptActual);
      }
    break;

    case VK_SUBTRACT:
      // check, if key is interesting
      if(m_iNavigationMode==FLY)
      {
        // decrease speed
        m_fFlyspeed-=m_fFlySpeedInc;
        if(m_fFlyspeed<-1.0F)
          m_fFlyspeed=-1.0F;
        DoDraw(NONE,m_ptActual);
      }
    break;

    case VK_UP: 
      if(m_iNavigationMode==FLYTO)
      {
        // fly towards POI
        m_fktran=m_fFlyToTran;
        m_fkrot=m_fFlyToRot;
        m_iFlyMode=NONE;
        if(GetKeyState(VK_SHIFT)&0x8000)
          m_iFlyMode=LEFT;
        if(GetKeyState(VK_CONTROL)&0x8000)
          m_iFlyMode=RIGHT;
        fly2_hold_button(m_iFlyMode,m_ptActual);
        if(m_iInteractEnabled)
          GetDocument()->GetScene()->interact(1);
      }
    break;

    case VK_DOWN: 
      if(m_iNavigationMode==FLYTO)
      {
        // fly from
        m_iFlyMode=NONE;
        if(GetKeyState(VK_SHIFT)&0x8000)
          m_iFlyMode=LEFT;
        if(GetKeyState(VK_CONTROL)&0x8000)
          m_iFlyMode=RIGHT;
        m_fktran=-m_fFlyToTran;
        m_fkrot=-m_fFlyToRot;
        fly2_hold_button(m_iFlyMode,m_ptActual);
        if(m_iInteractEnabled)
          GetDocument()->GetScene()->interact(1);
     }
    break;

/*    case VK_HOME: 
    case VK_END:
    case VK_RETURN:
    case VK_RIGHT :
    case VK_LEFT  : 
    case VK_INSERT: break;
    case VK_DELETE: break;
    case VK_F1  : break;
    case VK_F2  : break;
    case VK_F3  : break;
    case VK_F4  : break;
    case VK_F5  : break;
    case VK_F6  : break;
    case VK_F7  : break;
    case VK_F8  : break;
    case VK_F9  : break;
    case VK_F10 : break;
    case VK_F11 : break;
    case VK_F12 : break;
*/
    default:
    break;
  } // switch
	CView::OnKeyDown(nChar, nRepCnt, nFlags);
}


// 9.6.95
// need this virtual function to stop flying in flyto mode
void CSceneView::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
  switch (nChar) 
  {
    case VK_UP: 
      if(m_iNavigationMode==FLYTO)
      {
        // stop flying
        m_fktran=0.0F;
        m_fkrot=0.0F;
        if(m_iInteractEnabled)
        {
          GetDocument()->GetScene()->interact(0);
          DoDraw(NONE,m_ptActual);
        }
      }
    break;

    case VK_DOWN: 
      if(m_iNavigationMode==FLYTO)
      {
        // stop flying
        m_fktran=0.0F;
        m_fkrot=0.0F;
        if(m_iInteractEnabled)
        {
          GetDocument()->GetScene()->interact(0);
          DoDraw(NONE,m_ptActual);
        }
      }
    break;

/*    case VK_HOME: 
    case VK_END:
    case VK_RETURN:
    case VK_RIGHT :
    case VK_LEFT  : 
    case VK_INSERT: break;
    case VK_DELETE: break;
    case VK_ADD     : break;
    case VK_SUBTRACT: break;
    case VK_F1  : break;
    case VK_F2  : break;
    case VK_F3  : break;
    case VK_F4  : break;
    case VK_F5  : break;
    case VK_F6  : break;
    case VK_F7  : break;
    case VK_F8  : break;
    case VK_F9  : break;
    case VK_F10 : break;
    case VK_F11 : break;
    case VK_F12 : break;
    case VK_ADD:
    case VK_SUBTRACT:
*/
    default:
    break;
  } // switch
	CView::OnKeyUp(nChar, nRepCnt, nFlags);
}





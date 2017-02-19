// scenemen.cpp : handles all menu events
// written in June 1995
// Gerbert Orasche
// copyright: (c) 1994-95
// Institute For Information Processing And Computer Supported New Media (IICM)
// Graz University Of Technology


#include "stdafx.h"
#include "scenedoc.h"
#include "hgapp.h"
#include "mainfrm.h"
#include "scenevw.h"                                                                                                
#include "wscene.h"
#include "scene3d.h"
#include "ge3d.h"
#include "camera.h"

/////////////////////////////////////////////////////////////////////////////
// handling of menu events


void CSceneView::OnNavigationFly() 
{
  m_iNavigationMode=CSceneView::FLY;
  m_csStatText.LoadString(IDS_STR_HELP_FLY);
  ((CMainFrame*)AfxGetMainWnd())->m_wndStatusBar.SetPaneText(0,m_csStatText,TRUE);
  m_iPoiset=0;
  if(m_iInteractEnabled)
    GetDocument()->GetScene()->interact(0);
  DoDraw(NONE,CPoint(0,0));  
}

void CSceneView::OnNavigationFlyto() 
{
  m_iNavigationMode=CSceneView::FLYTO;
  m_csStatText.LoadString(IDS_STR_HELP_FLYTO);
  ((CMainFrame*)AfxGetMainWnd())->m_wndStatusBar.SetPaneText(0,m_csStatText,TRUE);
  m_iPoiset=0;
  if(m_iInteractEnabled)
    GetDocument()->GetScene()->interact(0);
  DoDraw(NONE,CPoint(0,0));  
}

void CSceneView::OnNavigationFlip() 
{
  m_iNavigationMode=CSceneView::FLIP;
  m_csStatText.LoadString(IDS_STR_HELP_FLIP);
  ((CMainFrame*)AfxGetMainWnd())->m_wndStatusBar.SetPaneText(0,m_csStatText,TRUE);
  m_iPoiset=0;
  if(m_iInteractEnabled)
    GetDocument()->GetScene()->interact(0);
  DoDraw(NONE,CPoint(0,0));  
}

void CSceneView::OnNavigationWalk() 
{
  m_iNavigationMode=CSceneView::WALK;
  m_csStatText.LoadString(IDS_STR_HELP_WALK);
  ((CMainFrame*)AfxGetMainWnd())->m_wndStatusBar.SetPaneText(0,m_csStatText,TRUE);
  m_iPoiset=0;
  if(m_iInteractEnabled)
    GetDocument()->GetScene()->interact(0);
  DoDraw(NONE,CPoint(0,0));  
}

void CSceneView::OnNavigationHeadsup() 
{
  m_iNavigationMode=CSceneView::HEADSUP;
  m_csStatText.LoadString(IDS_STR_HELP_HEADUP);
  ((CMainFrame*)AfxGetMainWnd())->m_wndStatusBar.SetPaneText(0,m_csStatText,TRUE);
  m_iPoiset=0;
  if(m_iInteractEnabled)
    GetDocument()->GetScene()->interact(0);
  DoDraw(NONE,CPoint(0,0));  
}

void CSceneView::OnUpdateNavigationFlip(CCmdUI* pCmdUI) 
{
  if(m_iNavigationMode==CSceneView::FLIP)
    pCmdUI->SetCheck(1);
  else
    pCmdUI->SetCheck(0);
}

void CSceneView::OnUpdateNavigationWalk(CCmdUI* pCmdUI) 
{
  if(m_iNavigationMode==CSceneView::WALK)
    pCmdUI->SetCheck(1);
  else
    pCmdUI->SetCheck(0);
}

void CSceneView::OnUpdateNavigationFly(CCmdUI* pCmdUI) 
{
  if(m_iNavigationMode==CSceneView::FLY)
    pCmdUI->SetCheck(1);
  else
    pCmdUI->SetCheck(0);
}

void CSceneView::OnUpdateNavigationFlyto(CCmdUI* pCmdUI) 
{
  if(m_iNavigationMode==CSceneView::FLYTO)
    pCmdUI->SetCheck(1);
  else
    pCmdUI->SetCheck(0);
}

void CSceneView::OnUpdateNavigationHeadsup(CCmdUI* pCmdUI) 
{
  if(m_iNavigationMode==CSceneView::HEADSUP)
    pCmdUI->SetCheck(1);
  else
    pCmdUI->SetCheck(0);
}

// 17.3.95
// resets the camera
void CSceneView::OnViewReset() 
{
  CSceneDoc* pDoc = GetDocument();

  pDoc->GetScene()->restoreCamera();
  // if we draw last scene with interactive mode reset...
  if(m_iInteractEnabled)
    pDoc->GetScene()->interact(0);
  DoDraw(CSceneView::NONE,CPoint(0,0));
  m_fktran=0.0F;
  m_fkrot=0.0F;
  m_iPoiset=0;
  if(m_iNavigationMode!=FLY)
    m_bFlying=FALSE;
}

// 21.9.95
// levels the camera horizontally
void CSceneView::OnViewLevelview() 
{
  GetDocument()->GetCamera()->makeHoricontal();
  Invalidate();
}


void CSceneView::OnUpdateNavigationKeepturning(CCmdUI* pCmdUI) 
{
  if(m_bKeep)
    // uncheck menu item
    pCmdUI->SetCheck(TRUE);
  else
    // check menu item
    pCmdUI->SetCheck(FALSE);
}

void CSceneView::OnNavigationKeepturning() 
{
  if(m_bKeep)
    m_bKeep=FALSE;
  else
    m_bKeep=TRUE;
}

void CSceneView::OnNavigationInteractiveFlatshading() 
{
  m_iInteractiveMode=ge3d_flat_shading;
  GetDocument()->GetScene()->modeInteract(ge3d_flat_shading);
}

void CSceneView::OnNavigationInteractiveHiddenline() 
{
  m_iInteractiveMode=ge3d_hidden_line;
  GetDocument()->GetScene()->modeInteract(ge3d_hidden_line);
}

void CSceneView::OnNavigationInteractiveSmoothshading() 
{
  m_iInteractiveMode=ge3d_smooth_shading;
  GetDocument()->GetScene()->modeInteract(ge3d_smooth_shading);
}

void CSceneView::OnNavigationInteractiveWireframe() 
{
  m_iInteractiveMode=ge3d_wireframe;
  GetDocument()->GetScene()->modeInteract(ge3d_wireframe);
}

void CSceneView::OnUpdateNavigationInteractiveFlatshading(CCmdUI* pCmdUI) 
{
  if(m_iInteractiveMode==ge3d_flat_shading)
    pCmdUI->SetCheck(1);
  else
    pCmdUI->SetCheck(0);
}

void CSceneView::OnUpdateNavigationInteractiveHiddenline(CCmdUI* pCmdUI) 
{
  if(m_iInteractiveMode==ge3d_hidden_line)
    pCmdUI->SetCheck(1);
  else
    pCmdUI->SetCheck(0);
}

void CSceneView::OnUpdateNavigationInteractiveSmoothshading(CCmdUI* pCmdUI) 
{
  if(m_iInteractiveMode==ge3d_smooth_shading)
    pCmdUI->SetCheck(1);
  else
    pCmdUI->SetCheck(0);
}

void CSceneView::OnUpdateNavigationInteractiveWireframe(CCmdUI* pCmdUI) 
{
  if(m_iInteractiveMode==ge3d_wireframe)
    pCmdUI->SetCheck(1);
  else
    pCmdUI->SetCheck(0);
}

// sets the drawing mode for flatshading
void CSceneView::OnFlatshading() 
{
  m_iDrawMode=ge3d_flat_shading;
  GetDocument()->GetScene()->mode(m_iDrawMode);
  Invalidate();
}

// sets the drawing mode for hidden line
void CSceneView::OnHiddenline() 
{
  m_iDrawMode=ge3d_hidden_line;
  GetDocument()->GetScene()->mode(m_iDrawMode);
  Invalidate();
}

// sets the drawing mode for gouraud shading
void CSceneView::OnSmoothshading() 
{
  m_iDrawMode=ge3d_smooth_shading;
  GetDocument()->GetScene()->mode(m_iDrawMode);
  Invalidate();
}

// sets the drawing mode for wireframe
void CSceneView::OnWireframe() 
{
  m_iDrawMode=ge3d_wireframe;
  GetDocument()->GetScene()->mode(m_iDrawMode);
  Invalidate();
}

void CSceneView::OnTexturing() 
{
  m_iDrawMode=ge3d_texturing;
  GetDocument()->GetScene()->mode(m_iDrawMode);
  Invalidate();
}

void CSceneView::OnUpdateTexturing(CCmdUI* pCmdUI) 
{
  if(m_iDrawMode==ge3d_texturing)
    pCmdUI->SetCheck(1);
  else
    pCmdUI->SetCheck(0);
}

void CSceneView::OnUpdateWireframe(CCmdUI* pCmdUI) 
{
  if(m_iDrawMode==ge3d_wireframe)
    pCmdUI->SetCheck(1);
  else
    pCmdUI->SetCheck(0);
}

void CSceneView::OnUpdateSmoothshading(CCmdUI* pCmdUI) 
{
  if(m_iDrawMode==ge3d_smooth_shading)
    pCmdUI->SetCheck(1);
  else
    pCmdUI->SetCheck(0);
}

void CSceneView::OnUpdateHiddenline(CCmdUI* pCmdUI) 
{
  if(m_iDrawMode==ge3d_hidden_line)
    pCmdUI->SetCheck(1);
  else
    pCmdUI->SetCheck(0);
}

void CSceneView::OnUpdateFlatshading(CCmdUI* pCmdUI) 
{
  if(m_iDrawMode==ge3d_flat_shading)
    pCmdUI->SetCheck(1);
  else
    pCmdUI->SetCheck(0);
}

void CSceneView::OnShowParserOutput()
{
  GetDocument()->GetScene()->showResult();
}

// toggles interactive quality switchback	
void CSceneView::OnViewInteractiveEnable() 
{
  if(m_iInteractEnabled)
    m_iInteractEnabled=FALSE;
  else
    m_iInteractEnabled=TRUE;
  GetDocument()->GetScene()->interact(0);
}

void CSceneView::OnUpdateViewInteractiveEnable(CCmdUI* pCmdUI) 
{
  if(m_iInteractEnabled)
    pCmdUI->SetCheck(1);
  else
    pCmdUI->SetCheck(0);
}

void CSceneView::OnViewPolygonsAutomatic() 
{
  m_iPolygons=Scene3D::twosided_auto;
  GetDocument()->GetScene()->twosidedpolys(m_iPolygons);
  DoDraw(NONE,CPoint(0,0));  
}

void CSceneView::OnUpdateViewPolygonsAutomatic(CCmdUI* pCmdUI) 
{
  if(m_iPolygons==Scene3D::twosided_auto)
    pCmdUI->SetCheck(1);
  else
    pCmdUI->SetCheck(0);
}

void CSceneView::OnViewPolygonsSinglesided() 
{
  m_iPolygons=Scene3D::twosided_never;
  GetDocument()->GetScene()->twosidedpolys(m_iPolygons);
  DoDraw(NONE,CPoint(0,0));  
}

void CSceneView::OnUpdateViewPolygonsSinglesided(CCmdUI* pCmdUI) 
{
  if(m_iPolygons==Scene3D::twosided_never)
    pCmdUI->SetCheck(1);
  else
    pCmdUI->SetCheck(0);
}

void CSceneView::OnViewPolygonsTwosided() 
{
  m_iPolygons=Scene3D::twosided_always;
  GetDocument()->GetScene()->twosidedpolys(m_iPolygons);
  DoDraw(NONE,CPoint(0,0));  
}

void CSceneView::OnUpdateViewPolygonsTwosided(CCmdUI* pCmdUI) 
{
  if(m_iPolygons==Scene3D::twosided_always)
    pCmdUI->SetCheck(1);
  else
    pCmdUI->SetCheck(0);
}

void CSceneView::OnViewLightingAutomatic() 
{
  m_iLighting=Scene3D::lighting_auto;
  GetDocument()->GetScene()->dolighting(m_iLighting);
  DoDraw(NONE,CPoint(0,0));  
}

void CSceneView::OnUpdateViewLightingAutomatic(CCmdUI* pCmdUI) 
{
  if(m_iLighting==Scene3D::lighting_auto)
    pCmdUI->SetCheck(1);
  else
    pCmdUI->SetCheck(0);
}

void CSceneView::OnViewLightingLightingoff() 
{
  m_iLighting=Scene3D::lighting_never;
  GetDocument()->GetScene()->dolighting(m_iLighting);
  DoDraw(NONE,CPoint(0,0));  
}

void CSceneView::OnUpdateViewLightingLightingoff(CCmdUI* pCmdUI) 
{
  if(m_iLighting==Scene3D::lighting_never)
    pCmdUI->SetCheck(1);
  else
    pCmdUI->SetCheck(0);
}

void CSceneView::OnViewLightingSwitchon() 
{
  m_iLighting=Scene3D::lighting_always;
  GetDocument()->GetScene()->dolighting(m_iLighting);
  DoDraw(NONE,CPoint(0,0));  
}

void CSceneView::OnUpdateViewLightingSwitchon(CCmdUI* pCmdUI) 
{
  if(m_iLighting==Scene3D::lighting_always)
    pCmdUI->SetCheck(1);
  else
    pCmdUI->SetCheck(0);
}

void CSceneView::OnViewFramerate() 
{
  if(m_iFrameCnt)
    m_iFrameCnt=0;
  else
	  m_iFrameCnt=1;
}

void CSceneView::OnUpdateViewFramerate(CCmdUI* pCmdUI) 
{
  if(m_iFrameCnt)
    pCmdUI->SetCheck(1);
  else
    pCmdUI->SetCheck(0);
}

// toggles collision detection
void CSceneView::OnNavigationCollisiondetection() 
{
  if(m_bCollDetect)
    m_bCollDetect=0;
  else
	  m_bCollDetect=1;
  GetDocument()->GetScene()->collisionDetection(m_bCollDetect);
}

void CSceneView::OnUpdateNavigationCollisiondetection(CCmdUI* pCmdUI) 
{
  if(m_bCollDetect)
    pCmdUI->SetCheck(1);
  else
    pCmdUI->SetCheck(0);
}

void CSceneView::OnNavigationFreerotationcontrol() 
{
  if(m_bFreeRot)
    m_bFreeRot=0;
  else
	  m_bFreeRot=1;
  GetDocument()->GetScene()->arbitraryRotations(m_bFreeRot);
}

void CSceneView::OnUpdateNavigationFreerotationcontrol(CCmdUI* pCmdUI) 
{
  if(m_bFreeRot)
    pCmdUI->SetCheck(1);
  else
    pCmdUI->SetCheck(0);
}

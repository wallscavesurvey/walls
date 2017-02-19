// scenevw.h : interface of the CSceneView class
// written in December 1994
// Gerbert Orasche
// Institute For Information Processing And Computer Supported New Media (IICM)
// Graz University Of Technology

#ifndef SCENE_VIEWER_SCENEVW_H
#define SCENE_VIEWER_SCENEVW_H

#include "color.h"
#include "vecutil.h"

extern BOOL bWglInit;
void WglInit(CREATESTRUCT& cs);

class CSceneView : public CView
{
public:
protected: // create from serialization only
  CSceneView();
  DECLARE_DYNCREATE(CSceneView)

// Attributes
public:
  CSceneDoc* GetDocument();

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSceneView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual void OnInitialUpdate();
  virtual BOOL OnIdle(LONG lCount);
	protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
	void afx_msg OnTileVert();
public:
		virtual ~CSceneView();
	#ifdef _DEBUG
		virtual void AssertValid() const;
		virtual void Dump(CDumpContext& dc) const;
	#endif
	void DoDraw(int iMode,CPoint& cpNew);
	enum nav_mode
	{
	  WALK,FLY,FLYTO,FLIP,HEADSUP
	};
	enum button_state
	{
	  NONE,LEFT,RIGHT,BOTH,REDRAW
	};

	enum
	{
	  _icon_none=-1,_icon_eyes,_icon_body,_icon_lift,_icon_fly2,
	  _icon_num,  // number of icons
	  halficonsize_ = 16
	};


protected:
	BOOL m_bLButton;
	BOOL m_bRButton;
	BOOL m_bMButton;
	CPoint m_ptActual;
	CPoint m_ptDown;
	int m_iInitialized;
	CSize m_csSize;
	BOOL m_bSizeChanged;
	// basic Navigationmode
	int m_iNavigationMode;
	// navigation mode for interactivity
	int m_iInteractiveMode;
	// rendering mode
	int m_iDrawMode;
	BOOL m_bFlying;
	float m_fFlyspeed;
	// handle for MESA GL context
#ifdef __MESA__
  WMesaContext m_hmglrc;
#endif
	// handle for NT GL context
	HGLRC m_hglrc;
	HPALETTE m_hPal;
	// midpoint of the icons (y coord. in pixels from center)
	float m_fHuIconpos[_icon_num];      
	colorRGB m_rgbColHudisplay;
	colorRGB m_rgbBackGround;
	// buffer for coors head up
	float m_fDownx;
	float m_fDowny;
	float m_fDragx;
	float m_fDragy;
	// heads up: active icon
	int m_iHuIcon;
	// simulates head up for velocity mode
	int m_iWalkIcon;
	// fly to vars
	// target point (point of interest) for fly to
	point3D m_pPoitarget;
	// normal at target (POI) point
	vector3D m_pPoinormal;
	// int for setting point
	// gemetric object of hit
	//GeometricObject* m_gobjHit;
	// node hit
	//QvNode* m_nodeHit;
	// anchor node hit
	//QvWWWAnchor* m_anchorHit;
	// hit time
	//float m_fHitTime;

	// true if poitarget_ set
	int m_iPoiset;
	// true if keeping velocity
	BOOL m_bKeep; 
	// stores mode for velocity
	int m_iFlipMode;
	int m_iFlyMode;
	// stores values for translation
	float m_fktran;
	float m_fkrot;
	// Holds status line text
	CString m_csStatText;
	// holds interactive status
	int m_iInteractEnabled;
	// status of polygon rendering
	int m_iPolygons;
	// status for lighting
	int m_iLighting;
	// miliseconds since windows started
	unsigned long m_lOldTime;
	// frames / second * 100
	// bools for collision detection
	int m_iFrameCnt;
	int m_bSlide;
	int m_bRecClipPLane;
	int m_bCollDetect;
	// bool for alternate rotations
	int m_bFreeRot;

	// GL-related member functions
private:
	void GLDestroy();
	unsigned char GLComponentFromIndex(int i, UINT nbits, UINT shift);
	void GLCreateRGBPalette(HDC hDC);
	BOOL GLSetupPixelFormat(HDC hdc);
	BOOL GLMakeCurrent(CDC* pDC);
	BOOL GLValidContext();
	BOOL GLSwapBuffers(CDC* pDC);
	void GLSetupContext(CDC* pDC);

	void DrawFly(int iMode,CPoint& cpNew);
	void DrawFlyto(int iMode,CPoint& cpNew);
	void DrawWalk(int iMode,CPoint& cpNew);
	void DrawHeadsup(int iMode,CPoint& cpNew);
	void DrawFlip(int iMode,float dx,float dy);
	void fly1_next_frame(int iMode,CPoint& cpNew);
	void drawui();
	void fly2_hold_button(int iMode,CPoint& cpNew);
	void hu_hold_button(int icon,float dx,float dy);

#ifdef __MESA__
	#define NCOLORS 256
	//static float tkRGBMap[NCOLORS][3];
	//static BYTE tkRGBMap[NCOLORS][3];
#else
	// static variables
	static unsigned char   m_oneto8[2];
	static unsigned char   m_twoto8[4];
	static unsigned char   m_threeto8[8];
	static int             m_defaultOverride[13];
	static PALETTEENTRY    m_defaultPalEntry[20];
#endif
	// vars for flying mode
	static float m_fFlyDeadX;
	static float m_fFlyDeadY;
	static float m_fFlySpeedInc;
	static float m_fFlyTurnHor;
	static float m_fFlyTurnVert;
	static float m_fWalkTurnHor3;
	static float m_fWalkTurnVert3;
	static float m_fWalkZoom;
	static float m_fWalkZoom3;
	static float m_fWalkFocal;
	static float m_fFlipFocal;
	static float m_fFlipZoom;
	static float m_fFlipTurnHor;
	static float m_fFlipTurnVert;
	static float m_fFlyToTran;
	static float m_fFlyToRot;
	static float m_fVelocityZoom;
	static float m_fVelocityPan;
	static float m_fVelocityTurnHor;
	static float m_fVelocityTurnVert;
	static float m_fVelocityZoom3;
	static float m_fVelocityPan3;
	static float m_fVelocityTurnHor3;
	static float m_fVelocityTurnVert3;

// Generated message map functions
protected:
	//{{AFX_MSG(CSceneView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnViewReset();
	afx_msg void OnNavigationFly();
	afx_msg void OnNavigationFlyto();
	afx_msg void OnNavigationFlip();
	afx_msg void OnNavigationWalk();
	afx_msg void OnNavigationHeadsup();
	afx_msg void OnUpdateNavigationKeepturning(CCmdUI* pCmdUI);
	afx_msg void OnNavigationKeepturning();
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnNavigationInteractiveFlatshading();
	afx_msg void OnNavigationInteractiveHiddenline();
	afx_msg void OnNavigationInteractiveSmoothshading();
	afx_msg void OnNavigationInteractiveWireframe();
	afx_msg void OnUpdateNavigationInteractiveFlatshading(CCmdUI* pCmdUI);
	afx_msg void OnUpdateNavigationInteractiveHiddenline(CCmdUI* pCmdUI);
	afx_msg void OnUpdateNavigationInteractiveSmoothshading(CCmdUI* pCmdUI);
	afx_msg void OnUpdateNavigationInteractiveWireframe(CCmdUI* pCmdUI);
	afx_msg void OnFlatshading();
	afx_msg void OnHiddenline();
	afx_msg void OnSmoothshading();
	afx_msg void OnWireframe();
	afx_msg void OnShowParserOutput();
	afx_msg void OnViewInteractiveEnable();
	afx_msg void OnUpdateViewInteractiveEnable(CCmdUI* pCmdUI);
	afx_msg void OnUpdateNavigationFlip(CCmdUI* pCmdUI);
	afx_msg void OnUpdateNavigationFly(CCmdUI* pCmdUI);
	afx_msg void OnUpdateNavigationFlyto(CCmdUI* pCmdUI);
	afx_msg void OnUpdateNavigationHeadsup(CCmdUI* pCmdUI);
	afx_msg void OnUpdateWireframe(CCmdUI* pCmdUI);
	afx_msg void OnUpdateSmoothshading(CCmdUI* pCmdUI);
	afx_msg void OnUpdateHiddenline(CCmdUI* pCmdUI);
	afx_msg void OnUpdateFlatshading(CCmdUI* pCmdUI);
	afx_msg void OnUpdateNavigationWalk(CCmdUI* pCmdUI);
	afx_msg void OnViewPolygonsAutomatic();
	afx_msg void OnUpdateViewPolygonsAutomatic(CCmdUI* pCmdUI);
	afx_msg void OnViewPolygonsSinglesided();
	afx_msg void OnUpdateViewPolygonsSinglesided(CCmdUI* pCmdUI);
	afx_msg void OnViewPolygonsTwosided();
	afx_msg void OnUpdateViewPolygonsTwosided(CCmdUI* pCmdUI);
	afx_msg void OnViewLightingAutomatic();
	afx_msg void OnUpdateViewLightingAutomatic(CCmdUI* pCmdUI);
	afx_msg void OnViewLightingLightingoff();
	afx_msg void OnUpdateViewLightingLightingoff(CCmdUI* pCmdUI);
	afx_msg void OnViewLightingSwitchon();
	afx_msg void OnUpdateViewLightingSwitchon(CCmdUI* pCmdUI);
	afx_msg void OnTexturing();
	afx_msg void OnUpdateTexturing(CCmdUI* pCmdUI);
	afx_msg void OnViewLevelview();
	afx_msg void OnViewFramerate();
	afx_msg void OnUpdateViewFramerate(CCmdUI* pCmdUI);
	afx_msg void OnNavigationCollisiondetection();
	afx_msg void OnUpdateNavigationCollisiondetection(CCmdUI* pCmdUI);
	afx_msg void OnNavigationFreerotationcontrol();
	afx_msg void OnUpdateNavigationFreerotationcontrol(CCmdUI* pCmdUI);
	//}}AFX_MSG
	afx_msg void OnMButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMButtonUp(UINT nFlags, CPoint point);
	DECLARE_MESSAGE_MAP()
};


#ifndef _DEBUG  // debug version in scenevw.cpp
inline CSceneDoc* CSceneView::GetDocument()
   { return (CSceneDoc*)m_pDocument; }
#endif

#endif
/////////////////////////////////////////////////////////////////////////////

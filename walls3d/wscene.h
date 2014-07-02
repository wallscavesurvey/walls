// wcene.h : Windows scene3D
// written in August 1995
// Gerbert Orasche
// copyright: (c) 1994-95
// Institute For Information Processing And Computer Supported New Media (IICM)
// Graz University Of Technology

#ifndef SC_VIEWER_WSCENE_H
#define SC_VIEWER_WSCENE_H

#include "scene3d.h"
#include "resource.h"
class CSceneDoc;


#define PARSE_BUF 32768

class CWinScene3D: public Scene3D
{
public:
  CWinScene3D(CSceneDoc* pDoc);
  ~CWinScene3D();

  // originally virtual functions
  void progress(float fPercent, int iLabel);
  // error message (e.g. during parsing) 
  void errorMessage(const char* pszError) const;
  void showNumFaces();
  // request all textures
  void requestTextures();    
  // load a texture
  void loadTextureFile(Material* material,const char* filename);
  // read inline VRML data
  int readInlineVRMLFile(QvWWWInline* node,const char* filename);
  // read inline VRML data
  int readInlineVRMLFILE(QvWWWInline* node,FILE* file);
  // navigation hint from file
  void navigationHint(int);
  
  // *** selection ***
  // selected object has changed
  void selectionChanged();
  // define selected object as source anchor
  void setSourceAnchor();
  // delete a source anchor
  void deleteSourceAnchor(const SourceAnchor*);
  // define current view as destination anchor
  void setDestinationAnchor ();    
  // define the default destination anchor
  void setDefDestAnchor ();        

  // *** navigation ***
  // history functions
  void back();                    
  void forward();
  void history();
  // hold this viewer
  void hold();                    
  void docinfo(int);              
  // document information
  void anchorInfo(const SourceAnchor*);

  // *** anchors ***
  // activation of a link (view other document)
  int followLink(const GeometricObject*, const SourceAnchor*);

  // URL-request for inline VRML
  void requestInlineURL(QvWWWInline* node,const char* url,const char* docurl);

  // *** Windows only functions
  void showResult(void);
private:
  CSceneDoc* m_pDoc;
  char* m_pszParseOut;
};


#endif
/////////////////////////////////////////////////////////////////////////////
// CParseDlg dialog

class CParseDlg : public CDialog
{
// Construction
public:
	CParseDlg(char* pszEditText);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CParseDlg)
	enum { IDD = IDD_SCENE_OUTPUT };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CParseDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CParseDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

  char* m_pszEditText;
};

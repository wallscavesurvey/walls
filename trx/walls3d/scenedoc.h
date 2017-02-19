// scenedoc.h : interface of the CSceneDoc class
//
/////////////////////////////////////////////////////////////////////////////

class CWinScene3D;
class Camera;

#include <fstream>
using namespace std;


class CSceneDoc : public CDocument
{
protected: // create from serialization only
	CSceneDoc();
	DECLARE_DYNCREATE(CSceneDoc)

// Attributes
public:

// Operations
public:
  //void SetMode(int mode);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSceneDoc)
	public:
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CSceneDoc();
  CWinScene3D* GetScene();
  const char *GetFirstComment();
  Camera* GetCamera();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
  // handle to scene class
  CWinScene3D* m_pScene;
  char * m_pszFileName;
  // TRUE if document is packed
  int m_iPacked;

// Generated message map functions
protected:
	//{{AFX_MSG(CSceneDoc)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

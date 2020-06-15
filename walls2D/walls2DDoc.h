// walls2DDoc.h : interface of the CWalls2DDoc class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_WALLS2DDOC_H__6EDC27C5_3F1D_4809_9E48_365C0C00C923__INCLUDED_)
#define AFX_WALLS2DDOC_H__6EDC27C5_3F1D_4809_9E48_365C0C00C923__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CWalls2DDoc : public CDocument
{
protected: // create from serialization only
	CWalls2DDoc();
	DECLARE_DYNCREATE(CWalls2DDoc)

	// Attributes
public:

	// Operations
public:

	// Overrides
		// ClassWizard generated virtual function overrides
		//{{AFX_VIRTUAL(CWalls2DDoc)
		//}}AFX_VIRTUAL

	// Implementation
public:
	virtual ~CWalls2DDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

	// Generated message map functions
protected:
	//{{AFX_MSG(CWalls2DDoc)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WALLS2DDOC_H__6EDC27C5_3F1D_4809_9E48_365C0C00C923__INCLUDED_)

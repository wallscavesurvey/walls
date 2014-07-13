// reselectDoc.h : interface of the CReselectDoc class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_RESELECTDOC_H__A91B1A4D_19D9_47A5_9FD6_FBCD1E3398DB__INCLUDED_)
#define AFX_RESELECTDOC_H__A91B1A4D_19D9_47A5_9FD6_FBCD1E3398DB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CReselectDoc : public CDocument
{
protected: // create from serialization only
	CReselectDoc();
	DECLARE_DYNCREATE(CReselectDoc)

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CReselectDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CReselectDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CReselectDoc)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RESELECTDOC_H__A91B1A4D_19D9_47A5_9FD6_FBCD1E3398DB__INCLUDED_)

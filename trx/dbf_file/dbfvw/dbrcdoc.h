// dbrcdoc.h : interface of the CDBRecDoc and CDBRecHint classes
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __DBFILE_H
#include <dbfile.h>
#endif

class CDBRecHint : public CObject
{
	DECLARE_DYNAMIC(CDBRecHint)
public:
    BOOL m_recUpdated;
	CDBRecHint();
};


class CDBRecDoc : public CDocument
{
protected: // create from serialization only
	CDBRecDoc();
	DECLARE_DYNAMIC(CDBRecDoc)

public:
	static BOOL m_bInitialOpen;

protected:
	
	CDBFile m_dbf;  
	LONG m_nActiveRecNo;
    DBF_pFLDDEF m_pFldDef;
    int m_nNumFlds;
    PBYTE m_pRec;
    CDBRecHint m_hint;
    
// Overridables
	virtual void FillNewRecord(void *pRecord) = 0;   
	
// Operations
public:
	// Operations used by views
	BOOL CreateNewRecord(); // creates a new record (calls FillNewRecord())
	LONG GetRecordCount() {return (LONG)m_dbf.NumRecs();}
	WORD  GetSizRec() {return m_dbf.SizRec();}
	LONG GetActiveRecNo() {return m_nActiveRecNo;}
	void ActivateNext(BOOL bNext);
	void Activate(LONG RecNo);
    void GetFlds(DBF_pFLDDEF &pFldDef,int &nNumFlds) {
         pFldDef=m_pFldDef; nNumFlds=m_nNumFlds;}
	BOOL GetRecord(LONG nRecNo,void *pRecord);
	void *RecPtr(LONG nRecNo);
	virtual void UpdateRecord(CView* pSourceView,LONG nRecNo,void* pRecord); 

// Implementation
protected:
	// Overrides of CDocument member functions
	virtual BOOL OnOpenDocument(const char* pszPathName);  // opens a database
	virtual BOOL OnSaveDocument(const char* pszPathName);  // flushes the database
	virtual void DeleteContents(); 						   // closes the database
public:
	virtual ~CDBRecDoc();
	
// Generated message map functions
protected:
	//{{AFX_MSG(CDBRecDoc)
	afx_msg void OnNextRec();
	afx_msg void OnUpdateNextRec(CCmdUI* pCmdUI);
	afx_msg void OnPrevRec();
	afx_msg void OnUpdatePrevRec(CCmdUI* pCmdUI);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

// dbfdoc.h : interface of the CDbfDoc class
//
/////////////////////////////////////////////////////////////////////////////

typedef struct {
  int nCol;
  int nLen;
  int nFmt;
  void (*pfOutPut)(CDC *pDC,int x,int y,void *pSrc,int nLen);
} DBF_FLDFORMAT;
typedef DBF_FLDFORMAT *DBF_pFLDFORMAT;

class CDbfDoc : public CDBRecDoc
{
protected: // create from serialization only
        CDbfDoc();
        DECLARE_DYNCREATE(CDbfDoc)

// Attributes
        DBF_pFLDFORMAT m_pFldFormat;
        int m_nRecordCols;
        
// Overrides of CDBRecDoc and CDocument
    virtual void DeleteContents();
 	virtual void FillNewRecord(void *pRecord);   
	virtual BOOL OnOpenDocument(const char* pszPathName);
 
public:
        virtual ~CDbfDoc();
        void GetFldInfo(DBF_pFLDDEF &pDef,int &nNumFlds,
           DBF_pFLDFORMAT &pFormat,int &nRecordCols) {
           pDef=m_pFldDef;
           nNumFlds=m_nNumFlds;
           pFormat=m_pFldFormat;
           nRecordCols=m_nRecordCols;
        }
        
#ifdef _DEBUG
	virtual	void AssertValid() const;
	virtual	void Dump(CDumpContext& dc) const;
#endif

};

/////////////////////////////////////////////////////////////////////////////

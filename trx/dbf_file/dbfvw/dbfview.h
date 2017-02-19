// dbfview.h : interface of the CDbfView class
//
// Class CDBRecView implements a generic row-based scroll view.
// This derived class, CDbfView, implements the details specific
// to the DBFVW application.
/////////////////////////////////////////////////////////////////////////////
class CDbfView : public CDBRecView
{
        DECLARE_DYNCREATE(CDbfView)
public:
        CDbfView();
	    CDbfDoc* GetDocument();

// Attributes
protected:
   int m_nCharHeight;
   int m_nCharWidth;
   int m_nRecordCols;
   int m_nNumFlds;
   DBF_pFLDDEF m_pFldDef;
   DBF_pFLDFORMAT m_pFldFormat;

// Operations
   void DrawGrid(CDC *pDC,int y,int height,BOOL hdrType);
    
public:
    static int InitialFrameWidth();

// Required overrides of CDBRecView --
	virtual void GetRowDimensions(CDC* pDC,int& nRowWidth,int& nRowHeight,
	   int& nHdrCols,int& nRowLabelWidth);
	virtual void OnDrawRow(CDC* pDC,LONG nRow,int y,BOOL bSelected);
	virtual void GetFieldData(int idx,LPSTR &pText,int &fmt,int &len);

protected:
// standard overrides 
	void OnInitialUpdate();
    virtual ~CDbfView();
};

/////////////////////////////////////////////////////////////////////////////
#ifndef _DEBUG	// debug version in dbfview.cpp
inline CDbfDoc* CDbfView::GetDocument()
   { return (CDbfDoc*) m_pDocument; }
#endif

// dbfview.cpp : implementation of the CDbfView class
//

#include "stdafx.h"
#include "dbfvw.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
    
#define LABEL_COLS 7
/////////////////////////////////////////////////////////////////////////////
// CDbfView

IMPLEMENT_DYNCREATE(CDbfView, CDBRecView)

////////////////////////////////////////////////////////////////////////////
// CDbfView construction/destruction

CDbfView::CDbfView()
{
}

CDbfView::~CDbfView()
{
}
/////////////////////////////////////////////////////////////////////////////
// Overrides of CView and CDBRecView 

void CDbfView::OnInitialUpdate()
{
  GetDocument()->GetFldInfo(m_pFldDef,m_nNumFlds,m_pFldFormat,m_nRecordCols);
  CDBRecView::OnInitialUpdate();
}

void CDbfView::GetRowDimensions(CDC* pDC,int& nRowWidth,int& nRowHeight,
    int& nHdrCols,int& nRowLabelWidth)
{
	TEXTMETRIC tm;

    pDC->GetTextMetrics(&tm);
    m_nCharWidth=tm.tmAveCharWidth;
    m_nCharHeight=tm.tmHeight;
    nRowWidth = m_nCharWidth*(m_nRecordCols+LABEL_COLS);
	nHdrCols=m_nNumFlds+1;
    nRowLabelWidth=m_nCharWidth*LABEL_COLS-1;
	nRowHeight=m_nCharHeight+2; // 1 line of text (allow room for grid line)
} 

int CDbfView::InitialFrameWidth()
{
	TEXTMETRIC tm;
    HDC hdc=::GetDC(NULL);
    SelectObject(hdc,GetStockObject(ANSI_FIXED_FONT));
    ::GetTextMetrics(hdc,&tm);
    ::ReleaseDC(NULL,hdc);
    return tm.tmAveCharWidth*(LABEL_COLS/*+m_nRecordCols*/)+
      2*GetSystemMetrics(SM_CXFRAME)+GetSystemMetrics(SM_CXVSCROLL)-1;
}

void CDbfView::DrawGrid(CDC *pDC,int y,int height,BOOL hdrType)
{
     int x;
     CPen *oldpen;
     CPen pen;
     
     if(!hdrType) {
       pen.CreatePen(PS_SOLID,0,RGB(200,200,200));
       oldpen=(CPen *)pDC->SelectObject(&pen);
     }
     else oldpen=(CPen *)pDC->SelectStockObject(BLACK_PEN);
     //First, draw the right-bounding field bars, from right to left --
     x=m_nCharWidth*(LABEL_COLS+m_nRecordCols)-1; 
     for(int i=m_nNumFlds;i--;) {
       pDC->MoveTo(x,y);
       pDC->LineTo(x,y+height);
       x=m_nCharWidth*(m_pFldFormat[i].nCol+LABEL_COLS)-1;
     }
     pDC->MoveTo(x,y+height);
     pDC->LineTo(m_nCharWidth*(LABEL_COLS+m_nRecordCols)-1,y+height);
     if(!hdrType) pDC->SelectStockObject(BLACK_PEN); 
     pDC->MoveTo(x,y);
     pDC->LineTo(x,y+height);
     if(hdrType) {
       pDC->MoveTo(0,y+height);
       pDC->LineTo(x,y+height);
     }
     pDC->SelectObject(oldpen); 
}


void CDbfView::OnDrawRow(CDC* pDC,LONG nRow,int y,BOOL bSelected)
{
	CRect rect;
	

	pDC->GetClipBox(&rect);
	rect.top = y;
	rect.bottom = y+m_nRowHeight-1;

	if(m_nHdrHeight==y) {
			pDC->MoveTo(rect.left,y-1);
			pDC->LineTo(rect.right,y-1);
	}

	if(rect.left<m_nRowLabelWidth) {
	    {
			int rt=rect.right;
			rect.right=m_nRowLabelWidth;
			rect.top++;
			pDC->FillRect(&rect,CBrush::FromHandle((HBRUSH)GetStockObject(LTGRAY_BRUSH)));
			rect.top--;
			rect.right=rt;
        } 
        {
			int prevmode=pDC->SetBkMode(TRANSPARENT);
			char buffer[LABEL_COLS];
			sprintf(buffer,"%*lu",LABEL_COLS-1,nRow);
			TextOut(pDC->m_hDC,0,y,buffer,LABEL_COLS-1);
			pDC->SetBkMode(prevmode);
	    }
	    {
			//CPen *oldpen=(CPen *)pDC->SelectStockObject(BLACK_PEN);
			pDC->MoveTo(0,rect.bottom);
			pDC->LineTo(m_nRowLabelWidth,rect.bottom);
			//pDC->SelectObject(oldpen);
        } 
	    if(bSelected) pDC->PatBlt(0,y,m_nRowLabelWidth,m_nRowHeight,DSTINVERT);
	}
	
    if(rect.right>m_nRowLabelWidth) {
	    char *prec=(char *)(GetDocument())->RecPtr(nRow);
	    CBrush brushBackground;
  		brushBackground.CreateSolidBrush(::GetSysColor(COLOR_WINDOW));
	    rect.left=m_nRowLabelWidth;
		pDC->FillRect(&rect,&brushBackground);
        if(prec) {
          DBF_pFLDFORMAT pf=m_pFldFormat;
          if(*prec>' ') TextOut(pDC->m_hDC,m_nCharWidth*LABEL_COLS,y,(LPSTR)prec,1);
          for(int i=0;i<m_nNumFlds;i++,pf++)
            pf->pfOutPut(pDC,m_nCharWidth*(pf->nCol+LABEL_COLS+1),y,
              prec+m_pFldDef[i].F_Off,pf->nLen);
        }
    	DrawGrid(pDC,y,m_nRowHeight-1,FALSE);
    }
}

void CDbfView::GetFieldData(int idx, LPSTR & pText,int &fmt,int & len)
{
	if(!idx--) {
		pText="";
		fmt=HDF_CENTER|HDF_STRING;
		len=m_nRowLabelWidth+1;
	    return;
	}
	fmt=(m_pFldFormat[idx].nFmt|HDF_STRING);
	pText=m_pFldDef[idx].F_Nam;
	if(*pText==(char)-1) {
		pText+=2;
		if(*pText=='_') pText++;
	}
	len=strlen(pText);
	if(len<m_pFldFormat[idx].nLen) len=m_pFldFormat[idx].nLen;
	len=(len+2)*m_nCharWidth;
}

/////////////////////////////////////////////////////////////////////////////
#ifdef _DEBUG
CDbfDoc* CDbfView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CDbfDoc)));
	return (CDbfDoc*) m_pDocument;
}
#endif

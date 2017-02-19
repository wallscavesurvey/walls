// dbfdoc.cpp : implementation of the CDbfDoc class
//

#include "stdafx.h"
#include "dbfvw.h"
#include "utility.h"
#include <trx_str.h>

extern "C" TRXFCN_S trx_FileDate(LPSTR pstr3,UINT days); /*days since 12/31/77 to d3*/

#define FLT_DIGITS 10
#define DBL_DIGITS 10

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CDbfDoc

IMPLEMENT_DYNCREATE(CDbfDoc,CDBRecDoc)

CDbfDoc::CDbfDoc()
{
  m_pFldFormat=NULL;
}

CDbfDoc::~CDbfDoc()
{
}

/////////////////////////////////////////////////////////////////////////////
// CDbfDoc diagnostics

#ifdef _DEBUG
void CDbfDoc::AssertValid() const
{
	CDBRecDoc::AssertValid();
}
void CDbfDoc::Dump(CDumpContext& dc) const
{
	CDBRecDoc::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CDbfDoc commands

void CDbfDoc::DeleteContents()
{
  if(m_pFldFormat) {
    free(m_pFldFormat);
    m_pFldFormat=NULL;
  }
  CDBRecDoc::DeleteContents();
}

//Field display functions --

static void OutText(CDC *pDC,int x,int y,void *pSrc,int nLen)
{
  TextOut(pDC->m_hDC,x,y,(LPSTR)pSrc,nLen);
}

static void OutFloat(CDC *pDC,int x,int y,void *pSrc,int nLen)
{
  char buffer[FLT_DIGITS+20];
  #if FLT_DIGITS==12
	  double d=*(float *)pSrc;
	  if(d<0 || d>1.1) sprintf(buffer,"%*.2f",nLen,d);
	  else sprintf(buffer,"%*.*f",nLen,nLen-2,d);
  #else
  sprintf(buffer,"%*.2f",nLen,*(float *)pSrc);
  #endif
  TextOut(pDC->m_hDC,x,y,buffer,nLen);
}

static void OutDouble(CDC *pDC,int x,int y,void *pSrc,int nLen)
{
  char buffer[DBL_DIGITS+20];
  #if DBL_DIGITS==12
	  double d=*(double *)pSrc;
	  if(d<0 || d>1.1) sprintf(buffer,"%*.2f",nLen,d);
	  else sprintf(buffer,"%*.*f",nLen,nLen-2,d);
  #else
  sprintf(buffer,"%*.6f",nLen,*(double *)pSrc);
  #endif
  TextOut(pDC->m_hDC,x,y,buffer,nLen);
}

static void OutWord(CDC *pDC,int x,int y,void *pSrc,int nLen)
{
  char buffer[12];
  sprintf(buffer,"%*hu",nLen,*(WORD *)pSrc);
  TextOut(pDC->m_hDC,x,y,buffer,nLen);
}

static void OutDWord(CDC *pDC,int x,int y,void *pSrc,int nLen)
{
  char buffer[24];
  sprintf(buffer,"%*lu",nLen,*(DWORD *)pSrc);
  TextOut(pDC->m_hDC,x,y,buffer,nLen);
}

static void OutLong(CDC *pDC,int x,int y,void *pSrc,int nLen)
{
  char buffer[24];
  sprintf(buffer,"%*ld",nLen,*(long *)pSrc);
  TextOut(pDC->m_hDC,x,y,buffer,nLen);
}

static void OutByte(CDC *pDC,int x,int y,void *pSrc,int nLen)
{
  char buffer[12];
  sprintf(buffer,"%*hu",nLen,(UINT)(*(BYTE *)pSrc));
  TextOut(pDC->m_hDC,x,y,buffer,nLen);
}

static void OutShort(CDC *pDC,int x,int y,void *pSrc,int nLen)
{
  char buffer[12];
  sprintf(buffer,"%*hi",nLen,*(short *)pSrc);
  TextOut(pDC->m_hDC,x,y,buffer,nLen);
}

static void OutSKUK(CDC *pDC,int x,int y,void *pSrc,int nLen)
{
	char buffer[16];
	BYTE longval[4];

	longval[3]=0;
	longval[2]=((BYTE *)pSrc)[0];
	longval[1]=((BYTE *)pSrc)[1];
	longval[0]=((BYTE *)pSrc)[2];
	sprintf(buffer,"%07lu",*(DWORD *)longval);
    TextOut(pDC->m_hDC,x,y,buffer,7);
}

static void OutISKU(CDC *pDC,int x,int y,void *pSrc,int nLen)
{
	char buffer[16];
	sprintf(buffer,"%07lu",*(DWORD FAR *)pSrc);
    TextOut(pDC->m_hDC,x,y,buffer,7);
}

static UINT _inline _chk_range(UINT i,UINT limit)
{
	return (i>limit)?0:i;
}

static void OutDB2Date(CDC *pDC,int x,int y,void *pSrc,int nLen)
{
	char buffer[16];

	sprintf(buffer,"%4u/%02u/%02u",
		   1900+((BYTE *)pSrc)[0]-' ',
		   _chk_range(((BYTE *)pSrc)[1]-' ',12),
		   _chk_range(((BYTE *)pSrc)[2]-' ',31));
    TextOut(pDC->m_hDC,x,y,buffer,10);
}

static void OutDate2(CDC *pDC,int x,int y,void *pSrc,int nLen)
{
	char buffer[20];
	BYTE p[4];

	trx_FileDate((LPSTR)p,*(UINT *)pSrc);
	sprintf(buffer,"%4u/%02u/%02u",
		   1900+p[1]-' ',
		   _chk_range(p[2]-' ',12),
		   _chk_range(p[3]-' ',31));
	TextOut(pDC->m_hDC,x,y,buffer,10);
}

static void OutDate3(CDC *pDC,int x,int y,void *pSrc,int nLen)
{
  char buffer[20];
  DWORD dw=0;
  memcpy(&dw,pSrc,3);
  sprintf(buffer,"%04lu/%02lu/%02lu",(dw>>9)&4095,(dw>>5)&15,dw&31);
  TextOut(pDC->m_hDC,x,y,(LPSTR)buffer,nLen);
}

static void OutCost(CDC *pDC,int x,int y,void *pSrc,int nLen)
{
	char buf[20];
    int sizSrc=sprintf(buf,"%12ld ",*(DWORD FAR *)pSrc);
    buf[sizSrc-1]=buf[sizSrc-2];
    if((buf[sizSrc-2]=buf[sizSrc-3])==' ') buf[sizSrc-2]='0';
    buf[sizSrc-3]='.';
    if(buf[sizSrc-4]==' ') buf[sizSrc-4]='0';
    TextOut(pDC->m_hDC,x,y,buf,13);
}

BOOL CDbfDoc::OnOpenDocument(const char* pszPathName)
{
	// Upon opening the document, tell the application object
	// to save the path name in the private INI file.
	if (!CDBRecDoc::OnOpenDocument(pszPathName)) return FALSE;

    m_pFldFormat=(DBF_pFLDFORMAT)malloc(m_nNumFlds*sizeof(DBF_FLDFORMAT));
    if(!m_pFldFormat) {
    	DeleteContents();
        CMsgBox(MB_ICONEXCLAMATION,
            "%s not opened: No memory for field data",pszPathName);
        return FALSE;
    }

    int col=0;
    DBF_pFLDDEF pfd=m_pFldDef;
    DBF_pFLDFORMAT pff=m_pFldFormat;
    for(int i=0;i<m_nNumFlds;i++,pfd++,pff++) {
        int fldlen=strlen(pfd->F_Nam)+2;
        pff->pfOutPut=OutText;
		pff->nFmt=(pfd->F_Typ=='N')?HDF_CENTER:HDF_LEFT;
        pff->nLen=pfd->F_Len;

        if(pfd->F_Nam[0]=='_'&&fldlen>=5) {
            pfd->F_Nam[0]=(char)-1;
            fldlen-=3; 
            switch(pfd->F_Nam[1]) {
			  case '$' : if(fldlen<13+2) fldlen=13+2;
						 pff->pfOutPut=OutCost;
						 break;
			  case 'S' : if(fldlen<7+2) fldlen=7+2;
						 pff->pfOutPut=(pff->nLen==3)?OutSKUK:OutISKU;
						 break;
			  case 'B' : pff->pfOutPut=OutByte;
						 pff->nFmt=HDF_CENTER;
                         if(fldlen<3+2) fldlen=3+2;
                         break;
              case 'T' : pff->pfOutPut=OutDate3;
                         if(fldlen<10+2) fldlen=10+2;
                         break;
              case 'K' : pff->pfOutPut=OutDB2Date;
                         if(fldlen<10+2) fldlen=10+2;
                         break;
              case '2' : pff->pfOutPut=OutDate2;
                         if(fldlen<10+2) fldlen=10+2;
                         break;
              case 'F' : pff->pfOutPut=OutFloat;
						 pff->nFmt=HDF_CENTER;
                         if(fldlen<FLT_DIGITS+2) fldlen=FLT_DIGITS+2;
                         break;
              case 'D' : pff->pfOutPut=OutDouble;
						 pff->nFmt=HDF_CENTER;
                         if(fldlen<DBL_DIGITS+2) fldlen=DBL_DIGITS+2;
                         break;
              case 'W' : pff->pfOutPut=OutWord;
						 pff->nFmt=HDF_CENTER;
                         if(fldlen<5+2) fldlen=5+2;
                         break;
              case 'I' : pff->pfOutPut=OutShort;
						 pff->nFmt=HDF_CENTER;
                         if(fldlen<6+2) fldlen=6+2;
                         break;
              case 'U' : pff->pfOutPut=OutDWord;
						 pff->nFmt=HDF_CENTER;
                         if(fldlen<6+2) fldlen=10+2;
                         break;
              case 'L' : pff->pfOutPut=OutLong;
						 pff->nFmt=HDF_CENTER;
                         if(fldlen<6+2) fldlen=11+2;
                         break;
			}
            pff->nLen=fldlen-2;
        }
		else if(m_dbf.Type()==DBF2_FILEID) {
			switch(pfd->F_Typ) {
				case 'C' :  if(!strcmp(pfd->F_Nam,"SKUK")) {
								pff->pfOutPut=OutSKUK;
								pff->nLen=7;
							}
							break;

				case 'I' :  pff->pfOutPut=OutShort;
							pff->nFmt=HDF_CENTER;
							pff->nLen=6;
							break;
							
				case 'D' :  pff->pfOutPut=OutDB2Date;
							pff->nLen=10;
							break;
           
				case '$':	if(!strcmp(pfd->F_Nam,"ISKU")) {
							   pff->pfOutPut=OutISKU;
							   pff->nLen=7;
							   break;
							}
							pff->pfOutPut=OutCost;
							pff->nFmt=HDF_CENTER;
							pff->nLen=13;
							break;
							
			}
		}
        pff->nCol=col;
        if(fldlen<pff->nLen+2) fldlen=pff->nLen+2;
        col+=fldlen;
     }
     m_nRecordCols=col;

     theApp.UpdateIniFileWithDocPath(pszPathName);
	 return TRUE;
}

void CDbfDoc::FillNewRecord(void *pRecord)
{
	//Add code here!
}   

/////////////////////////////////////////////////////////////////////////////
// CDbfDoc commands


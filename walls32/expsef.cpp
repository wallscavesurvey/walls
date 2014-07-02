// expsef.cpp : Compilation/database opperations for CPrjDoc
//

#include "stdafx.h"
#include "walls.h"
#include "prjdoc.h"
#include "magdlg.h"
#include "expsef.h"
#include "expsefd.h"
#include "wall_srv.h"

#include <trxfile.h>

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define EXP_ERR_SETOPTIONS -1
#define EXP_ERR_DATEDECL -2
#define EXP_ERR_LOADDECL -3
#define EXP_ERR_CLOSE -4

#define EXP_DLL_FCN "_Export@4"

#define SIZ_PFX_CACHE 32

static PRJREF *pREF;

typedef int (FAR PASCAL *LPFN_EXPORT)(EXPTYP *);

static LPFN_EXPORT pExport;
static UINT uFLG;

#define EXP_FCN(id) (pEXP->fcn=id,pExport(pEXP))

extern DWORD numfiles;
extern double fTotalLength;

static EXPTYP *pEXP;
static CTRXFile *pixPFX;
static CExpSefDlg *pDLG;

static int PASCAL LogPrefix(char *name)
{
	int e;
	short pfx=0;
	
    if(!(e=pixPFX->Find((LPBYTE)name))) {
		pixPFX->GetRec(&pfx,sizeof(pfx));
	}
    else if(e==CTRXFile::ErrMatch || e==CTRXFile::ErrEOF) {
    	pfx=(short)pixPFX->NumRecs()+1;
    	if(e=pixPFX->InsertKey(&pfx,(LPBYTE)name)) pfx=0;
    }

    return pfx;
}

static int PASCAL SetOptions(CPrjListNode *pNode)
{
	if(pNode->Parent()) {
		if(SetOptions(pNode->Parent())) return EXP_ERR_SETOPTIONS;
	}
	else {
	  pEXP->title=NULL;
	  EXP_FCN(EXP_SETOPTIONS); //Set to default
	}

    if(!pNode->OptionsEmpty()) {
	  pEXP->title=pNode->Options();
	  if(EXP_FCN(EXP_SETOPTIONS)) return EXP_ERR_SETOPTIONS;
    }
    return 0;
} 

int PASCAL exp_CB(int fcn_id,LPSTR lpStr)
{
	int e=0;
	BYTE buf[TRX_MAX_KEYLEN+1];
	
	switch(fcn_id) {
		case FCN_DATEDECL:
		{
			ASSERT(pREF);
			CMainFrame::PutRef(pREF);
			DWORD dw=pEXP->date;
			// dw=(y<<9)+(m<<5)+d; //15:4:5
			MD.y=((dw>>9)&0x7FFF);
			MD.m=((dw>>5)&0xF);
			MD.d=(dw&0x1F);
			if(MF(MAG_CVTDATE)) return EXP_ERR_DATEDECL;
			*(double *)lpStr=(double)MD.decl;
			return 0;
		}

 		case FCN_PREFIX:
 		{
			ASSERT(pEXP->flags&EXP_CVTPFX);
			e=strlen(lpStr);
			if(e>128) return 0;
			memcpy(buf+1,lpStr,*buf=e); //Pascal string required
			e=LogPrefix((char *)buf);
			return e;
		}
		
	} /*switch*/
		
	//ASSERT(FALSE);
		
	return e;
}

int CPrjDoc::ExportBranch(CPrjListNode *pNode)
{
   int e;
   static char msg[32]="Converting ";

   if(!pNode->m_dwFileChk) return 0;
   
   if(pNode->m_bLeaf) {

	 pEXP->flags=uFLG;
   
	 if(pREF=pNode->GetAssignedRef()) {
		pEXP->flags |= EXP_USEDATES*pNode->InheritedState(FLG_DATEMASK);
		pEXP->grid=pREF->conv;
	 }
	 else pEXP->grid=0.0;

     //Set the compile options, if any --
	 //SetOptions must be called AFTER exp_Open()!
	 pEXP->path=SurveyPath(pNode);

     if(!(e=EXP_FCN(EXP_SRVOPEN)) && !(e=SetOptions(pNode))) {
		 //Show status msg --
		 strcpy(msg+11,pEXP->name=pNode->Name());
		 pDLG->StatusMsg(msg);
		 pEXP->title=pNode->Title();
		 e=EXP_FCN(EXP_SURVEY);
		 numfiles++;
	 }
     
	 if(e && e!=EXP_ERR_SETOPTIONS) {
			m_pErrorNode=pNode;
			m_nLineStart=(LINENO)pEXP->lineno;
			m_nCharStart=pEXP->charno;
	 }

   }
   else {
     //Branch node processing --
	 pEXP->title=pNode->Title();
	 if(!(e=EXP_FCN(EXP_OPENDIR))) {
		 pNode=(CPrjListNode *)pNode->m_pFirstChild;

		 while(pNode) {
		   if(((uFLG&EXP_INCDET) || !pNode->m_bFloating) && (e=ExportBranch(pNode))) break;
		   pNode=(CPrjListNode *)pNode->m_pNextSibling;
		 }

		 if(!e) EXP_FCN(EXP_CLOSEDIR);
	 }

   }
   return e;
}

int CPrjDoc::ExportSEF(CExpSefDlg *pDlg,CPrjListNode *pNode,const char *pathname,UINT flags)
{ 

    CTRXFile ixPFX;     //Branch node's original NTA file (segment index)
	EXPTYP exp;
	int e;

	m_pErrorNode=NULL;

	//Let's see if the directory needs to be created --
	if(!DirCheck((char *)pathname,TRUE)) return 0;

	//Load DLL --
	//UINT prevMode=SetErrorMode(SEM_NOOPENFILEERRORBOX);
	HINSTANCE hinst=LoadLibrary(EXP_DLL_NAME); //EXP_DLL_NAME
	pExport=NULL;
	
	if(hinst)
	  pExport=(LPFN_EXPORT)GetProcAddress(hinst,EXP_DLL_FCN);
	
	if(!pExport) {
	  if(hinst) FreeLibrary(hinst);
	  CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_IMPLOADDLL1,EXP_DLL_NAME);
	  return -1;
	}

	if(flags&EXP_CVTPFX) {
		//Create a temporary index file for converting name prefixes to integers --
		//Non-flushed mode is the default --
		if(!(e=ixPFX.Create(WorkPath(pNode,TYP_TMP),sizeof(short)))) { 
		  ixPFX.SetUnique(); //We will only insert unique names
		  ixPFX.SetExact(); //We will use "insert if not found"
		  //Assign cache and make it the default for indexes --
		  if(e=ixPFX.AllocCache(SIZ_PFX_CACHE,FALSE)) ixPFX.CloseDel();
		}
		if(e) {
			FreeLibrary(hinst);
			AfxMessageBox(IDS_ERR_MEMORY);
			return -1;
		}
		pixPFX=&ixPFX;
	}
	else pixPFX=NULL;

	uFLG=flags;
    numfiles=0;
	pEXP=&exp;
	pDLG=pDlg;
	exp.flags=EXP_VERSION;
	exp.path=pathname;
	exp.title=(char *)exp_CB;
    
	BeginWaitCursor();

	//Initialize SEF file --
	
	if(!(e=EXP_FCN(EXP_OPEN))) {
		pEXP->charno=e=ExportBranch(pNode);
		//Error status determines if SEF is to be deleted --
		if(EXP_FCN(EXP_CLOSE) && !e) e=EXP_ERR_CLOSE;
	}

	if(pixPFX) ixPFX.CloseDel(TRUE); 	//Delete cache

	EndWaitCursor();

	switch(e) {

		case 0:
			if(numfiles) pDLG->StatusMsg("");
			if(!exp.numvectors) AfxMessageBox(IDS_ERR_NTVEMPTY);
			else {
				char *pUnits;
				if(pNode->IsFeetUnits()) {
					pUnits="ft",
					exp.fTotalLength/=0.3048;
				}
				else pUnits="m";
				CMsgBox(MB_ICONEXCLAMATION,IDS_OK_EXPSEF5,
				pathname,
				numfiles,
				exp.numvectors,
				exp.fTotalLength,
				pUnits);
			}
			break;

		case EXP_ERR_SETOPTIONS: 
			CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_SRVOPTION,pNode->Title(),pEXP->title);
			break;

		case EXP_ERR_DATEDECL:
			CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_DATEDECL3,MD.y,MD.m,MD.d);
			break;

		case EXP_ERR_LOADDECL:
			ASSERT(0);
			//CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_IMPLOADDLL1,MAG_DLL_NAME);
			break;

		case EXP_ERR_CLOSE:
			CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_EXPCLOSE1,pathname);
			break;

		default: 	    
			//An error was found in the text file at line number pEXP->lineno --
			if(e!=EXP_ERR_DATEDECL && e!=EXP_ERR_LOADDECL) {
				if(m_pErrorNode) {
				  CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_EXPTEXT3,m_pErrorNode->Name(),m_nLineStart,pEXP->title);
				}
				else 
				  CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_EXPTEXT1,pEXP->title);
			}
			break;
	}

	FreeLibrary(hinst);
	return e;
}

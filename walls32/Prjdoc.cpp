// prjdoc.cpp : implementation of the CPrjDoc class
//

#include "stdafx.h"
#include "walls.h"
#include "prjframe.h"
#include "prjdoc.h"
#include "prjview.h"
#include "lineview.h"
#include "filecfg.h"
#include "prjtypes.h"
#include "itemprop.h"
#include "compview.h"
#include "plotview.h"
#include "mapframe.h"
#include "mapview.h"
#include "compile.h"
#include "ntfile.h"
#include "copyfile.h"
#include "wall_shp.h"
#include "vecinfo.h"
#include <stdarg.h>
#include <memory.h>
#include "launchfd.h"


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPrjDoc

IMPLEMENT_DYNCREATE(CPrjDoc, CDocument)

BEGIN_MESSAGE_MAP(CPrjDoc, CDocument)
	//{{AFX_MSG_MAP(CPrjDoc)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVE, OnUpdateFileSave)
	ON_COMMAND(ID_FILE_SAVE_AS, OnFileSaveAs)
	ON_COMMAND(ID_FILE_SAVE, OnFileSave)
	ON_COMMAND(ID_FILE_CLOSE, OnFileClose)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

CPrjListNode * CPrjDoc::m_pErrorNode;
CPrjListNode * CPrjDoc::m_pErrorNode2;
LINENO CPrjDoc::m_nLineStart;
LINENO CPrjDoc::m_nLineStart2;
int CPrjDoc::m_nCharStart;
BOOL CPrjDoc::m_bCheckNames=FALSE;
BOOL CPrjDoc::m_bComputing=FALSE;
BOOL CPrjDoc::m_bReviewing=FALSE;
BOOL CPrjDoc::m_bSwitchToMap=TRUE;
BOOL CPrjDoc::m_bSaveView=TRUE;
BOOL CPrjDoc::m_bPageActive=FALSE;
double CPrjDoc::m_ReviewScale=1.0;
double CPrjDoc::m_LnLimit=0.5;
double CPrjDoc::m_AzLimit=5.0;
double CPrjDoc::m_VtLimit=5.0;
BOOL CPrjDoc::m_bNewLimits=FALSE;
int CPrjDoc::m_iValueRangeNet=0;
CPrjDoc *CPrjDoc::m_pReviewDoc=NULL;
CPrjListNode *CPrjDoc::m_pReviewNode=NULL;
NET_DATA CPrjDoc::net_data;
char CPrjDoc::m_pathBuf[_MAX_PATH];

int CPrjDoc::m_iFindVector=0;
int CPrjDoc::m_iFindVectorDir=0;
int CPrjDoc::m_iFindVectorCnt=0;
int CPrjDoc::m_iFindVectorMax=0;
int CPrjDoc::m_iFindVectorIncr=1;
int CPrjDoc::m_iFindStation=0;
int CPrjDoc::m_iFindStationCnt=0;
int CPrjDoc::m_iFindStationMax=0;
int CPrjDoc::m_iFindStationIncr=1;
int CPrjDoc::m_bVectorDisplayed=0;

static CFileCfg *pFile;

CFileBuffer * CPrjDoc::m_pFileBuffer;

char *CPrjDoc::WorkTyp[CPrjDoc::TYP_COUNT]={
  NET_VEC_EXT,NET_NET_EXT,NET_PLT_EXT,NET_STR_EXT,
  NET_SEG_EXT,NET_NAM_EXT,NET_TMP_EXT,NET_LST_EXT,
  NET_3D_EXT,NET_WMF_EXT,NET_SEF_EXT,NET_LOG_EXT,NET_2D_EXT,NET_NTW_EXT,NET_NTAC_EXT};


static BOOL PrepareName(char *pName,BOOL bLeaf)
{
   //Prepare pName in case PRJ file was manually edited --
   char *p=trx_Stpnam(pName);
   
   if(p>pName) strcpy(pName,p); //Trim off path
   if(!bLeaf) *trx_Stpext(pName)=0; //Trim extension if workfile basename
   //_strupr(pName);
   return *pName!=0;
}

static void PrjFileErrMsg(UINT typ)  						
{
  //typ contains the resource ID of an appropriate error message --
  CPrjDoc *pDoc=((CMainFrame*)theApp.m_pMainWnd)->m_pFirstPrjDoc;         						
  CString errmsg;
  if(errmsg.LoadString(typ))
	CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_PROJECT4,pDoc->m_pPath,pDoc->m_pszName,pFile->LineCount(),(const char *)errmsg);
}

static BOOL LoadReference(PRJREF *pr)
{
  if(!pr || pFile->Argc()<2) return FALSE;

  ASSERT(cfg_quotes==0);
  cfg_quotes=1;
  pFile->GetArgv(pFile->Argv(1));
  cfg_quotes=0;
  int imax=pFile->Argc();
  if(imax<REF_NUMFLDS) return FALSE; /*REF_NUMFLDS==14*/
  pr->north=atof(pFile->Argv(0));
  pr->east=atof(pFile->Argv(1));
  pr->zone=(short)atoi(pFile->Argv(2));
  pr->conv=(float)atof(pFile->Argv(3));
  pr->elev=atoi(pFile->Argv(4));
  pr->flags=(BYTE)(UINT)atoi(pFile->Argv(5));
  pr->latdeg=(BYTE)(UINT)atoi(pFile->Argv(6));
  pr->latmin=(BYTE)(UINT)atoi(pFile->Argv(7));
  pr->latsec=(float)atof(pFile->Argv(8));
  pr->londeg=(BYTE)(UINT)atoi(pFile->Argv(9));
  pr->lonmin=(BYTE)(UINT)atoi(pFile->Argv(10));
  pr->lonsec=(float)atof(pFile->Argv(11));
  pr->datum=(BYTE)(UINT)atoi(pFile->Argv(12));

  char *p=(char *)pFile->Argv(13);
  if(strlen(p)>=REF_SIZDATUMNAME) p[REF_SIZDATUMNAME-1]=0;
  strcpy(pr->datumname,p);
  return TRUE;
}

static BOOL ReadBookNode(CPrjListNode *pParent)
{
 	//Process file lines successively, attaching a new leaf node to pParent if a
 	//SURVEY directive is encountered, or creating and attaching  new branch node whenever 
 	//BOOK is encountered. In the latter case, ReadBookNode(<new branch node>) is called
 	//recursively to process the branch. If pParent is *not* a book node (bLeaf==TRUE)
 	//this function simply returns TRUE. If pParent==NULL, FALSE is returned after
 	//displaying an error message IDS_ERR_FILERES.
 	//
 	//Otherwise, TRUE is returned upon reaching an ENDBOOK directive without
 	//first encountering an error condition. If an error occurs, or if a recursive
 	//call returns FALSE, we free all allocated memory and return FALSE. If an error
 	//is detected within this invocation, we will display a message box before returning.
    
	
	if(pParent->m_bLeaf) return TRUE;
	
    UINT typ;
    BOOL bSibling=FALSE;
    char *pArg;
    
    while(TRUE) {
      typ=pFile->GetLine();
      
      if((int)typ<0) {
        if(typ==CFG_ERR_EOF) typ=PRJ_ERR_EOF;
        else if(typ==CFG_ERR_KEYWORD) typ=PRJ_ERR_KEYWORD;
        else typ=PRJ_ERR_TRUNCATE;
        break;
      }
      
	  if(pFile->Argc()<2) {
	    if(typ!=PRJ_CFG_NAME && typ!=PRJ_CFG_ENDBOOK && typ!=PRJ_CFG_PATH) {
	      typ=PRJ_ERR_BADARG;
	 	  break;
	    }
	    pArg=0;
	  }
	  else pArg=pFile->Argv(1);
      
      switch(typ) {
         case PRJ_CFG_STATUS  : typ=(UINT)atoi(pArg);
								pParent->m_bOpen=(typ&1)!=0;
								pParent->m_bFloating=(typ&2)!=0;
#ifdef _GRAYABLE
								pParent->m_bGrayed=(typ&4)!=0;
#endif
								pParent->m_uFlags=typ>>3;
         						continue;
         						
         case PRJ_CFG_OPTIONS : pParent->SetOptions(pArg);
         						continue;

		 case PRJ_CFG_REF  : if(pParent->m_pRef) {
								   ASSERT(FALSE);
								   continue;
								}
								if(!LoadReference(pParent->m_pRef=new PRJREF)) {
								   ASSERT(FALSE);
								   delete pParent->m_pRef;
								   pParent->m_pRef=NULL;
								   pParent->m_uFlags&=~FLG_REFUSECHK;
								}
								else {
								   pParent->m_uFlags|=FLG_REFUSECHK;
								   pParent->m_uFlags&=~FLG_REFUSEUNCHK;
								}
								continue;
         						
         case PRJ_CFG_NAME	  :	if(pArg) {
                                  //In case text was directly edited --
                                  //Note we are not checking for the required uniqueness!
         						  if(PrepareName(pArg,pParent->m_bLeaf)) pParent->SetName(pArg);
                                }
         					    continue;
         					    
         case PRJ_CFG_PATH	  :	if(pArg) {
                                  //In case path was directly edited --
								  typ=strlen(pArg);
								  if(typ>1 && pArg[typ-1]=='\\') pArg[typ-1]=0;
         						  pParent->SetPath(pArg);
                                }
         					    continue;

         case PRJ_CFG_SURVEY  :
         case PRJ_CFG_BOOK    : {
									CPrjListNode *pNode=new CPrjListNode(typ==PRJ_CFG_SURVEY,pArg);
									//An exception would have been thrown upon failure.
									//We can assume pNode is valid --
									
									//NOTE: For efficiency, we use the CHierListNode (base
									//class) version of Attach() since we will handle the
									//file checksums, etc., after the entire project tree
									//is assembled (see OnOpenDocument()) --
									
									if(!(typ=ReadBookNode(pNode)) ||
									  (typ=pNode->CHierListNode::Attach(pParent,bSibling))) {
									    if(pNode) {
									      pNode->DeleteChildren();
										  delete pNode;
										}
										if(!typ) return FALSE; //ReadBookNode() failed
	    								if(typ==HN_ERR_MaxLevels) typ=PRJ_ERR_MAXLEVELS;
										else {
											typ=PRJ_ERR_COMPARE;
										}
	    								break;
									}
									pParent=pNode;
								}
								//From now on, we attach nodes at this level to pParent
								//as successive siblings --
								bSibling=TRUE;
								continue;
								
         case PRJ_CFG_ENDBOOK : return TRUE;

		 default			  : continue; //Ignore unsupported keywords
         
      } //switch(typ)
      
      //An error was detected within this invocation --
      break; 
      
    } //TRUE
    
	PrjFileErrMsg(typ);
    return FALSE; 						
}

static CPrjListNode *ReadRootNode()
{
    if(PRJ_CFG_BOOK!=pFile->GetLine() || pFile->Argc()<2) {
  		PrjFileErrMsg(PRJ_ERR_NOTAPRJ);
		return NULL;
    }
    CPrjListNode *pRoot=NULL;
    
	TRY {
		pRoot = new CPrjListNode(FALSE,pFile->Argv(1));
		pRoot->m_uFlags|=FLG_MASK_UNCHK;
		if(!ReadBookNode(pRoot)) {
		    pRoot->DeleteChildren();
		    delete pRoot;
		    return NULL;
		}
	}
    CATCH(CMemoryException,e) {
    	if(pRoot) {
		    pRoot->DeleteChildren();
		    delete pRoot;
    	}   
    	CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_PRJMEM);
    	return NULL;
    }
	END_CATCH
	
	//A root node is always "floating" --
	pRoot->m_bFloating=TRUE;
	 
    //For now, force open to first level.
	//This overrides what was in the PRJ file --
    if(pRoot->FirstChild()) pRoot->m_bOpen=TRUE;
    
	return pRoot;
}

static DWORD FileChecksum(const char *pszPathName)
{
    //Compute a 32-bit checksum based only on a file's name, size, and timestamp -- 
    UINT timesum;
	long filesize;

	struct tm *ptm=GetLocalFileTime(pszPathName,&filesize);
    
    if(!ptm) return 0;
    timesum=ptm->tm_sec+ptm->tm_min+ptm->tm_hour+ptm->tm_mday+ptm->tm_mon+ptm->tm_year;

    DWORD buf[3];
    memset(buf,0,sizeof(buf));
    char *pName=trx_Stpnam((char *)pszPathName);
    int len=strlen(pName);
    if(len>sizeof(buf)) len=sizeof(buf);
    memcpy((char *)buf+(sizeof(buf)-len),pName,len);
    return (DWORD)timesum+(DWORD)filesize+buf[0]+buf[1]+buf[2];
}
 
const char * CPrjDoc::UnitStr()
{
	return LEN_ISFEET()?"Feet":"Meters";
}

const char * CPrjDoc::ViewUnitStr()
{
	return m_pReviewNode->InheritedState(FLG_GRIDMASK)?"Grid":"True";
}

void CPrjDoc::BranchFileChk(CPrjListNode *pNode)
{
  //Compute checksum for all survey files in branch.
  //Call this function recursively for all child nodes.
  pNode->m_dwFileChk=0;
  if(pNode->m_bLeaf) {
    if(pNode->IsOther() || pNode->NameEmpty()) return;

#ifdef _USE_FILETIMER
	if(m_bCheckForChange) {
		DWORD dwChk=FileChecksum(SurveyPath(pNode));
		if(dwChk!=pNode->m_dwFileChk) {
			CLineDoc *pDoc=CLineDoc::GetLineDoc(CPrjDoc::m_pathBuf);
			if(pDoc) {
				CString msg;
				BOOL bSave=FALSE;
				msg.Format("File: %s\n\nCAUTION: The disk copy of this file has been modified by another application!\n"
					"This requires that any editor windows for this file be closed.",CPrjDoc::m_pathBuf);
				if(pDoc->m_bTitleMarked) {
					bSave=(IDYES==CMsgBox(MB_YESNO,"%s\n\nUpon closing do you want to save your pending changes, overwriting the disk copy?",(LPCSTR)msg));
				}
				else AfxMessageBox(msg);
				pDoc->Close(!bSave);
				if(bSave) dwChk=FileChecksum(CPrjDoc::m_pathBuf);
			}
			pNode->m_dwFileChk=dwChk;
		}
	}
	else
#endif
     pNode->m_dwFileChk=FileChecksum(SurveyPath(pNode));
  }
  else {
    CPrjListNode *pNext=pNode->FirstChild();
    while(pNext) {
      BranchFileChk(pNext);

      if(!pNext->m_bFloating) pNode->m_dwFileChk^=pNext->m_dwFileChk;

      pNext=pNext->NextSibling();
    }
  }
}

int CPrjDoc::AttachedFileChk(CPrjListNode *pNode)
{
  //Recompute checksums for all *attached* survey files in branch and return a file count.
  //Call this function recursively for all child nodes.
  pNode->m_dwFileChk=0;
  if(!pNode->m_bLeaf) {
    int count=0;
    CPrjListNode *pNext=pNode->FirstChild();
    while(pNext) {
      if(!pNext->m_bFloating) {
        count+=AttachedFileChk(pNext);
        pNode->m_dwFileChk^=pNext->m_dwFileChk;
      }
      pNext=pNext->NextSibling();
    }
    return count;
  }
  if(pNode->IsOther() || pNode->NameEmpty()) return 0;
  return 0!=(pNode->m_dwFileChk=FileChecksum(SurveyPath(pNode)));
}

CLineDoc * CPrjDoc::GetOpenLineDoc(CPrjListNode *pNode)
{
	if(pNode->m_bLeaf && !pNode->NameEmpty()) return CLineDoc::GetLineDoc(SurveyPath(pNode));
	return NULL;
}

int CPrjDoc::AllBranchEditsPending(CPrjListNode *pNode)
{
  int count=0;
  if(pNode->IsLeaf()) {
    CLineDoc *pDoc=GetOpenLineDoc(pNode);
    if(pDoc) count=(int)(pDoc->IsModified()||pDoc->m_bActiveChange);
  }
  else {
    CPrjListNode *pNext=pNode->FirstChild();
    while(pNext) {
      count+=AllBranchEditsPending(pNext);
      pNext=pNext->NextSibling();
    }
  }
  return count;
}

int CPrjDoc::BranchEditsPending(CPrjListNode *pNode)
{
  //Compute pNode->m_nEditsPending, the number of survey files in branch
  //that are open with CLineDoc::IsModified()==TRUE;
  
  pNode->m_nEditsPending=0;
  if(pNode->IsLeaf()) {
	if(pNode->IsOther()) return 0;
    CLineDoc *pDoc=GetOpenLineDoc(pNode);
    if(!pDoc) return 0;
	pNode->m_nEditsPending=(int)(pDoc->IsModified()||pDoc->m_bActiveChange);
    return pNode->m_bFloating?0:pNode->m_nEditsPending;
  }

  {
    CPrjListNode *pNext=pNode->FirstChild();
    while(pNext) {
      pNode->m_nEditsPending+=BranchEditsPending(pNext);
      pNext=pNext->NextSibling();
    }
	return pNode->m_nEditsPending;
  }
}

char * CPrjDoc::WorkPath(CPrjListNode *pNode,int typ)
{
    //Construct a complete pathname based on the node's workfile base name
    //which is assumed to be non-null.
#ifdef _DEBUG
	if(pNode && pNode->IsOther()) {
		ASSERT(FALSE);
	}
#endif
    char *p=m_pPath+m_lenPath;
    p+=strlen(strcpy(p,m_pszName));
    if(!pNode) *p=0;
    else {
		*p++='\\';
	    p+=strlen(strcpy(p,pNode->BaseName()));
	    #ifdef _DEBUG
	      if(p[-1]=='\\') {  //Should have been caught in CItemGeneral dialog, etc.
	        ASSERT(FALSE);
	      }
	    #endif
	    *p++='.';
	    strcpy(p,WorkTyp[typ]);
	}
    return m_pPath;
}

BOOL CPrjDoc::OnOpenDocument(const char* pszPathName)
{
	CFileCfg file(prjtypes,PRJ_CFG_NUMTYPES);
	
	if(!SetName(pszPathName) ||
	   file.OpenFail(pszPathName,CFile::modeReadWrite|CFile::shareDenyWrite)) return FALSE;
	  
	pFile=&file;

	cfg_quotes=0;

    ASSERT(((CMainFrame*)theApp.m_pMainWnd)->m_pFirstPrjDoc==this);

	TRY
	{
	    //Memory exceptions are handled in ReadRootNode() --
		if(!(m_pRoot=ReadRootNode())) {
			file.Abort(); 		// will not throw an exception
			return FALSE;
		}
		//Now read project-wide settings --
		//if(file.GetLine()==PRJ_CFG_REF) LoadReference(&m_ref);
		file.Close();
	}
	CATCH(CFileException,e)
	{
		TRY
			file.ReportException(pszPathName,e,FALSE, AFX_IDP_FAILED_TO_OPEN_DOC);
		END_TRY
		
		file.Abort(); 		// will not throw an exception
		return FALSE;
	}
	END_CATCH

	SetRootFlag(FLG_REFUSEUNCHK);
	SetRootFlag(FLG_DATEUNCHK);
	SetRootFlag(FLG_GRIDUNCHK);
	SetRootFlag(FLG_VTVERTUNCHK);
	SetRootFlag(FLG_VTLENUNCHK);
	
	//Compute survey and workfile checksums for all tree nodes --
	BranchFileChk(m_pRoot);
	BranchWorkChk(m_pRoot);
	
	//Initialize counts of open survey files with edits pending --
	BranchEditsPending(m_pRoot);
     
    SetPathName(pszPathName); //Necessary? **** Sets titles and updates MRU list
	MarkProject(FALSE);     // Start off with unmodified project
	
    theApp.UpdateIniFileWithDocPath(pszPathName);
	return TRUE;
}

int CPrjDoc::GetDataWidth(BOOL bIgnoreFlags)
{
   return m_pRoot?(m_pRoot->GetWidth(bIgnoreFlags)*CPrjView::m_LineHeight):0;
}   

/////////////////////////////////////////////////////////////////////////////
// CPrjDoc construction/destruction
CPrjDoc::CPrjDoc()
{
  CMainFrame* mf=(CMainFrame*)theApp.m_pMainWnd;
  m_pRoot=NULL;
  m_pNextPrjDoc=mf->m_pFirstPrjDoc;
  mf->m_pFirstPrjDoc=this;
  net_data.version=NET_VERSION;
  //Allocate buffer for "<lenName> 0 <m_lenPath><lenName>\8.3 0"
  m_pszName=(char *)malloc(_MAX_FNAME+_MAX_PATH+2); *m_pszName=0;
  m_pShapeFilePath=NULL;
  m_pFindString=NULL;
  m_uShapeTypes=(SHP_VECTORS|SHP_STATIONS|SHP_NOTES|SHP_FLAGS|SHP_WALLS|SHP_UTM);
  //m_uFindFlags=0;
  m_nActivePage=0;
#ifdef _USE_FILETIMER
  m_bCheckForChange=0;
#endif
}

BOOL CPrjDoc::SetName(const char *pszPathName)
{
	if(!m_pszName) return FALSE;
 	char *p=trx_Stpnam((char *)pszPathName);
 	m_lenPath=p-pszPathName;
	ASSERT(m_lenPath+13<=_MAX_PATH);
	if(m_lenPath+13>_MAX_PATH) return FALSE;
 	ASSERT(m_lenPath&& pszPathName[m_lenPath-1]=='\\');
 	
 	//Note we are ignoring the extension in pszPathName (PRJ assumed)
 	int lenName=trx_Stpext(p)-p;
 	memcpy(m_pszName,p,lenName);
 	m_pszName[lenName]=0;
 	m_pPath=m_pszName+lenName+1;
 	memcpy(m_pPath,pszPathName,m_lenPath);
	m_pPath[m_lenPath]=0;
 	return TRUE;
}

CPrjDoc::~CPrjDoc()
{
  if(m_pRoot) {
      m_pRoot->DeleteChildren();
      delete m_pRoot;
      m_pRoot=NULL;
  }
  
  free(m_pShapeFilePath);
  free(m_pFindString);
  
  CMainFrame* mf=(CMainFrame*)theApp.m_pMainWnd;
  CPrjDoc *pd=mf->m_pFirstPrjDoc;
  if(pd==this) mf->m_pFirstPrjDoc=m_pNextPrjDoc;
  else {
    while(pd && pd->m_pNextPrjDoc!=this) pd=pd->m_pNextPrjDoc;
    if(pd) pd->m_pNextPrjDoc=m_pNextPrjDoc;
  }
  free(m_pszName);
}

BOOL CPrjDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument()) return FALSE;
	//The above simply called DeleteContents(), emptied m_strPathName,
	//and called SetModified(FALSE). The current MFC version always returns TRUE.
		
    if(m_pszName) *m_pszName=0; //Setup for special handling in OnSaveDocument()
    
    //NOTE: We need to use an undocumented MFC function! 
    OnFileSaveAs(); //Will prompt for pathname and call OnSaveDocument()
    
    return m_pszName && *m_pszName;
}

static void CDECL PrjWrite(UINT nType,const char *format,...)
{
	char buf[256];
	int len=0;
	  
	if(nType) len=sprintf(buf,".%s\t",prjtypes[nType-1]);
	if(format) {
		va_list marker;
		va_start(marker,format);
		len+=_vsnprintf(buf+len,254-len,format,marker);
		va_end(marker);
	}
	else if(len) len--;
	strcpy(buf+len,"\r\n");
	CPrjDoc::m_pFileBuffer->Write(buf,len+2);
}

static void SaveReference(PRJREF *pr)
{
	PrjWrite(PRJ_CFG_REF,"%.3f %.3f %d %.3f %u %u %u %u %.3f %u %u %.3f %u \"%s\"",
	pr->north,
	pr->east,
	pr->zone,
	pr->conv,
	pr->elev,
	pr->flags,
	pr->latdeg,
	pr->latmin,
	pr->latsec,
	pr->londeg,
	pr->lonmin,
	pr->lonsec,
	pr->datum,
	pr->datumname);
}

static void WriteNode(CPrjListNode *pNode)
{
	PrjWrite(pNode->m_bLeaf?PRJ_CFG_SURVEY:PRJ_CFG_BOOK,pNode->Title());
	if(!pNode->NameEmpty()) PrjWrite(PRJ_CFG_NAME,pNode->Name());
	if(!pNode->PathEmpty()) PrjWrite(PRJ_CFG_PATH,pNode->Path());
	if(!pNode->OptionsEmpty()) PrjWrite(PRJ_CFG_OPTIONS,pNode->Options());
	
		UINT status=pNode->m_uFlags;

		//Clear all turned-off states (inherit overrides) for root --
		if(!pNode->Parent()) status &= ~FLG_MASK_UNCHK;
		//Turn off CHK flag for reference alone --
		status&=~FLG_REFUSECHK;

		status =

#ifdef _GRAYABLE
		           4*pNode->m_bGrayed+
#endif
				   2*pNode->m_bFloating+
		           1*pNode->m_bOpen+
		           (status<<3);

		
	if(status) PrjWrite(PRJ_CFG_STATUS,"%u",status);

	if(pNode->m_pRef) {
		ASSERT(pNode->m_uFlags&FLG_REFUSECHK);
		SaveReference(pNode->m_pRef);
	}

	if(pNode->m_bLeaf) return;
    pNode=pNode->FirstChild();
    while(pNode) {
      WriteNode(pNode);
      pNode=pNode->NextSibling();
    }
    PrjWrite(PRJ_CFG_ENDBOOK,NULL);
}

BOOL CPrjDoc::OnSaveDocument(const char* pszPathName)
{
    //Note: The only invocations of this function are from CPrjDoc::OnFileSave()
    //and CPrjDoc::OnFileSaveAs(). The return value is ignored --
    
    BOOL bSaveAs=stricmp(ProjectPath(),pszPathName)!=0;
    //Are we saving to a new filename?
    
    if(bSaveAs) {
      //We have come here from CPrjDoc::OnFileSaveAs().
      
      //We tentatively replace the project's name and pathname. 
      //This will have to be be reversed upon save failure --
      SetName(pszPathName);
      
      //This replaces any extension supplied by user with .WPJ --
      pszPathName=ProjectPath();
    }
	    
	CFileBuffer file(SAVEBUFSIZ);
	if (file.OpenFail(pszPathName,CFile::modeCreate|CFile::modeReadWrite|CFile::shareExclusive)) {
	    if(bSaveAs) SetName(m_strPathName);
		return FALSE;
    }
		
	BeginWaitCursor();
	m_pFileBuffer=&file;

	TRY
	{
	    PrjWrite(0,";WALLS Project file");
		WriteNode(m_pRoot);
	    file.Close();
	}

	CATCH(CFileException,e)
	{
		file.Abort(); // will not throw an exception
		EndWaitCursor();

		TRY
			file.ReportException(pszPathName,e,TRUE, AFX_IDP_FAILED_TO_SAVE_DOC);
		END_TRY
	    if(bSaveAs) SetName(m_strPathName);
		return FALSE;
	}
	END_CATCH
	
    SetPathName(pszPathName); //Clears any '*' mark on title
    MarkProject(FALSE);
 
    if(bSaveAs) {  
		//Since the project and/or work directory may have changed, we need to
		//recompute the file status of each node in the hierarchy.
		BranchFileChk(m_pRoot);
		BranchWorkChk(m_pRoot);
		BranchEditsPending(m_pRoot);
		UpdateAllViews(NULL);
    }
    
  	EndWaitCursor();
    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// CPrjDoc diagnostics

#ifdef _DEBUG
void CPrjDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CPrjDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CPrjDoc commands
void CPrjDoc::OnUpdateFileSave(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(IsModified());
}

void CPrjDoc::FixFileChecksums(const char *pszPathName,DWORD dwChk)
{
  //Called by CLineDoc::OnSaveDocument() after a successful save --
  
  CPrjListNode *pNode=m_pRoot->FindFileNode(this,pszPathName);
  if(!pNode) return;
  ASSERT(!_stricmp(SurveyPath(pNode),pszPathName));
  if(pNode->m_dwFileChk!=dwChk) {
    pNode->m_dwFileChk ^= dwChk;
    pNode->FixParentChecksums(); //Will take out old while putting in new
    pNode->m_dwFileChk=dwChk;
    //Refresh Compile/Review button (see CPrjView::OnUpdateCompileItem()) --
    GetPrjView()->RefreshDependentItems(pNode);
  }
}

void CPrjDoc::FixEditsPending(const char *pszPathName,BOOL bChg)
{
  CPrjListNode *pNode=m_pRoot->FindFileNode(this,pszPathName);
  if(pNode && bChg!=pNode->m_nEditsPending) {
    ASSERT(!_stricmp(SurveyPath(pNode),pszPathName));
    pNode->IncEditsPending(bChg?1:-1);
    //Refresh Compile/Review button (see CPrjView::OnUpdateCompileItem()) --
    GetPrjView()->RefreshDependentItems(pNode);
  }
}

void CPrjDoc::FixAllFileChecksums(const char *pszPathName)
{
    //Update node statuses of all projects to reflect an update
    //change in the file pszPathName (i.e., assume that the leaf
    //for that survey has an invalid checksum --
    
    DWORD dwChk=FileChecksum(pszPathName);
	CMultiDocTemplate *pTmp=CWallsApp::m_pPrjTemplate;
	CPrjDoc *pDoc;
	char namebuf[_MAX_PATH];

	strcpy(namebuf,pszPathName);
	
	POSITION pos = pTmp->GetFirstDocPosition();
	while(pos) {
	  pDoc=(CPrjDoc *)pTmp->GetNextDoc(pos);
	  if(pDoc && pDoc->IsKindOf(RUNTIME_CLASS(CPrjDoc))) pDoc->FixFileChecksums(namebuf,dwChk);
	}
}

CPrjListNode *CPrjDoc::FindFileNode(CPrjDoc **pFindDoc,LPCSTR pathname,BOOL bFindOther)
{
	CMultiDocTemplate *pTmp=CWallsApp::m_pPrjTemplate;
	CPrjDoc *pDoc;
	POSITION pos = pTmp->GetFirstDocPosition();
	CPrjListNode *pNode;
	while(pos) {
	  pDoc=(CPrjDoc *)pTmp->GetNextDoc(pos);
	  if(pDoc && pDoc->IsKindOf(RUNTIME_CLASS(CPrjDoc)) &&
		  (pNode=pDoc->m_pRoot->FindFileNode(pDoc,pathname,bFindOther))) {
		  if(pFindDoc) *pFindDoc=pDoc;
		  return pNode;
	  }
	}
	return NULL;
}

void CPrjDoc::FixAllEditsPending(const char *pszPathName,BOOL bChg)
{
	CMultiDocTemplate *pTmp=CWallsApp::m_pPrjTemplate;
	CPrjDoc *pDoc;
	char namebuf[_MAX_PATH];

	strcpy(namebuf,pszPathName);
	
	POSITION pos = pTmp->GetFirstDocPosition();
	while(pos) {
	  pDoc=(CPrjDoc *)pTmp->GetNextDoc(pos);
	  if(pDoc && pDoc->IsKindOf(RUNTIME_CLASS(CPrjDoc))) pDoc->FixEditsPending(namebuf,bChg);
	}
}

void CPrjDoc::OnFileSaveAs()
{
    //NOTE: This can be invoked by direct command or by our special
    //version of CPrjDoc::OnNewDocument(), in which case *m_pszName==0.
    //Note that CDocument::OnFileSaveAs() is simply
    //CDocument::Dosave(NULL==Prompt for name,TRUE==SetPathName).
    
    //In this version we want to enforce the file extension PRJ_PROJECT_EXTW
    //and handle the case of OnNewDocument(), in which we require a similar
    //dialog to obtain a file name and to replace the default title set up by
    //CWinApp::OnFileNew() --> CMultiDocTemplate::OpenDocumentFile(NULL).
    
	CString pathName;
	
	ASSERT(m_pszName);

	if(m_pszName) { 
		CString strFilter;
		if(!AddFilter(strFilter,IDS_PRJ_FILES)) return;
		if(!DoPromptPathName(pathName,
		  OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT,
		  1,strFilter,FALSE,
		  *m_pszName?IDS_PRJ_FILESAVEAS:IDS_PRJ_FILENEW,
		  PRJ_PROJECT_EXTW)) return;
		  
	    if(!DirCheck(pathName.GetBuffer(0),TRUE)) return;

		//Is project already open?
		if(theApp.FindOpenProject(pathName)) {
			CMsgBox(MB_ICONINFORMATION,"File %s is already open. You must choose a different name.",(LPCSTR)pathName);
			return;
		}
	}

    if(!m_pszName || !*m_pszName) {
       //If *m_pszName==0, this is being invoked via OnNewDocument() --> OnFileSaveAs() -->
       ASSERT(!m_pRoot);
	   CItemProp *pDlg=NULL;
	   ASSERT(m_strPathName.IsEmpty());
       
       //First initialize m_pszName and m_pPath with the name retrieved from the dialog --
       if(SetName(pathName)) {
			TRY {
				CString title=m_pszName;
				title+=" Project";
				m_pRoot = new CPrjListNode(FALSE,title,"PROJECT");
				m_pRoot->m_bFloating=TRUE;
				m_pRoot->m_uFlags|=FLG_MASK_UNCHK;
										 
				//This will allow the user to enter a title but NOT to revise the specified name --
				pDlg=new CItemProp(this,m_pRoot,2);

#ifdef _DEBUG
									     
				if(pDlg->DoModal()==IDOK) {
					CItemGeneral *pG=pDlg->GetGenDlg();
					CItemOptions *pO=pDlg->GetOptDlg();
					//NOTE: CString exceptions possible here!
					ASSERT(!strcmp(m_pRoot->m_Title,pG->m_szTitle));
					ASSERT(!strcmp(m_pRoot->m_Name,pG->m_szName));
					ASSERT(!strcmp(m_pRoot->m_Options,pO->m_szOptions));
					ASSERT(m_pRoot->m_uFlags==pDlg->m_uFlags);
					//m_pRoot->SetTitle(pG->m_szTitle);
					//m_pRoot->SetName(pG->m_szName);
					//m_pRoot->SetOptions(pO->m_szOptions);
		            //m_pRoot->m_uFlags=pDlg->m_uFlags;
				}
				else {
#else
				if(pDlg->DoModal()!=IDOK) {
#endif
					delete pDlg;
					delete m_pRoot;
					m_pRoot=NULL;
					if(m_pszName) *m_pszName=0;
					return;
				}
			}
			CATCH(CMemoryException,e) {
				if(pDlg) delete pDlg;
			    if(m_pRoot) {
			       delete m_pRoot;
				   m_pRoot=NULL;
				}
				if(m_pszName) *m_pszName=0;
				CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_MEMORY);
				return;
			}
			END_CATCH

			delete pDlg;
       }
       
	   if(!m_pRoot) {
		 CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_FILERES);
		 if(m_pszName) *m_pszName=0;
		 return;
	   }
	   
	   //SetPathName(ProjectPath()); //done in OnSaveDocument()!
	   
       theApp.UpdateIniFileWithDocPath(pathName);
       
	   MarkProject(TRUE); //This use to be FALSE (why??)
	   //We have created a new empty project with a user-specified name
	   //return;
	}
	
    OnSaveDocument(pathName);
}

void CPrjDoc::OnFileSave()
{
    //NOTE: We bypass CDocument::OnFileSave() --> DoSave() --
	OnSaveDocument(m_strPathName);
}

CLineDoc *CPrjDoc::OpenSurveyFile(CPrjListNode *pNode)
{
    if(!pNode) {
      pNode=m_pErrorNode;
      ASSERT(pNode);
	  CLineView::m_nLineStart=m_nLineStart;
	  CLineView::m_nCharStart=m_nCharStart;
	  if(pNode==m_pErrorNode2) CLineView::m_nLineStart2=m_nLineStart2;
	  else CLineView::m_nLineStart2=0;
    }
    else m_pErrorNode2=NULL;
     
	if(pNode->IsOther()) {
		CLineDoc::m_bNewEmptyFile=(_access(SurveyPath(pNode),0)==-1);
	}
	else CLineDoc::m_bNewEmptyFile=(pNode->m_dwFileChk==0);

	CLineDoc::m_pszInitFrameTitle=pNode->Title();
	CLineDoc::m_pszInitIconTitle=pNode->Name();

	CLineDoc *pDoc=CPrjDoc::GetOpenLineDoc(pNode);
	
	if(!pDoc) {
		LPCSTR path=SurveyPath(pNode);
		CLineDoc::m_bReadOnlyOpen=(_access(path,6)!=0 && errno==EACCES);
		pDoc=(CLineDoc *)theApp.m_pSrvTemplate->OpenDocumentFile(path);
		CLineDoc::m_bReadOnlyOpen=FALSE;
	}

	if(pDoc) pDoc->DisplayLineViews();
	else {
		CLineView::m_nLineStart=0;
		CLineDoc::m_pszInitFrameTitle=NULL;
	}
	
	if(!pDoc || !m_pErrorNode2) return pDoc;
	
	if(pNode==m_pErrorNode2) {
		LINENO nLast;
		POSITION pos=pDoc->GetFirstViewPosition();
		CLineView *pView=(CLineView *)pDoc->GetNextView(pos);
		pView->GetEditRectLastLine(NULL,nLast,FALSE); //nLast is last visible line
		if(nLast>=m_nLineStart2) {
		  //The two highlighted lines are in view --
		  return pDoc;
		}
		//The lines are far apart, so let's create a new view window --
		CDocTemplate* pTemplate = pDoc->GetDocTemplate();
		ASSERT_VALID(pTemplate);
		CFrameWnd* pFrame = pTemplate->CreateNewFrame(pDoc,(CFrameWnd *)pView->GetParent());
		if (pFrame == NULL) {
		   ASSERT(FALSE);
		   return pDoc;
		}
	    CLineView::m_nLineStart=m_nLineStart2;
	    CLineView::m_nCharStart=0;
	    CLineView::m_nLineStart2=m_nLineStart;
		pTemplate->InitialUpdateFrame(pFrame,pDoc);
		return pDoc;
	}
		  
	CLineView::m_nLineStart=m_nLineStart2;
	CLineView::m_nCharStart=0;
	return OpenSurveyFile(m_pErrorNode2);
}

BOOL CPrjDoc::CanCloseFrame(CFrameWnd* pFrameArg)
	// permission to close all views using this frame
	//  (at least one of our views must be in this frame)
{
    //The default SC_CLOSE processing results in a call to CDocument::CanCloseFrame()
    //which checks to see if m_nWindow>0 for the parent frame of any view. If so,
    //TRUE is returned, otherwise SaveModified() is called, which prompts the
    //user to save changes. This MFC behavior is incorrect! If more than one
    //document frames exist, SaveModified() should not be called.
    //
    //In our case, only three open view frame classes are possible: CPrjFrame, CCompFrame,
    //and CMapFrame. Since closing CPrjFrame will close CCompFrame, we need only call
    //SaveModified() for CPrjFrame!
    
	ASSERT_VALID(pFrameArg);
	
	//if(pCV && pCV->GetParentFrame()==pFrameArg ||
	//  pFrameArg->IsKindOf(RUNTIME_CLASS(CMapFrame))) return TRUE;
	
    if(!pFrameArg->IsKindOf(RUNTIME_CLASS(CPrjFrame))) return TRUE;
/*
	POSITION pos = GetFirstViewPosition();
	while (pos != NULL)
	{
		CView* pView = GetNextView(pos);
		ASSERT_VALID(pView);
		CFrameWnd* pFrame = pView->GetParentFrame();
		// assume frameless views are ok to close
		if (pFrame != NULL)
		{
			// assumes 1 document per frame
			ASSERT_VALID(pFrame);
			if (pFrame->m_nWindow > 0)
				return TRUE;        // more than one frame refering to us
		}
	}
*/

	// otherwise only one frame that we know about
	return SaveModified();
}

BOOL CPrjDoc::SaveModified()
{
	//if (IsModified()) return DoSave(m_strPathName);
	
	if (IsModified()) {
		if(!DoSave(m_strPathName)) OnFileSaveAs();
	}
	/*
		if (_access(m_strPathName, 6) != 0)
		{
			if (!DoSave(NULL)) return FALSE;   // don't continue 
			//ASSERT(FALSE);
		}
		else
		{
			if (!DoSave(m_strPathName))
				return FALSE;   // don't continue
		}
	}
	*/
	return TRUE;    // keep going
}

LINENO CPrjDoc::FindString(CPrjListNode *pNode,WORD findflags)
{
	//Save the file if it is already open and changed --
	{
		CLineDoc *pLineDoc=GetOpenLineDoc(pNode);
		if(pLineDoc) {
		  pLineDoc->SaveAndClose(FALSE);
		  Pause(); //Allow PostMessage(WM_COMMAND,ID_FILE_SAVE) to take effect
		}
	}
	
	char *tgt=SurveyPath(pNode);
	
	//Be silent when the file doesn't exist --
	if(_access(tgt,0) || m_pFileBuffer->OpenFail(tgt,CFile::modeRead|CFile::shareDenyWrite))
	  return 0;
	  
	char line[CLineDoc::MAXLINLEN+1];
	LINENO nResult=0;
	int tgtLen=strlen(m_pFindString);
	tgt=m_pFindString;
	int linLen,ovector[3];
	
	if(!(findflags&(F_USEREGEX|F_MATCHCASE))) tgt+=PRJ_SIZ_FINDBUF;
	
	//Hardly optimal, but fast enough for now --
	while((linLen=m_pFileBuffer->ReadLine(line,sizeof(line)))>=0) {
	  nResult++;
	  if(findflags&F_USEREGEX) {
		if(pcre_exec(m_pRegEx,NULL,line,strlen(line),0,PCRE_NOTEMPTY,ovector,3)>=0) goto _ret;
	  }
	  else {
		  if(linLen<tgtLen) continue;
		  if(!(findflags&F_MATCHCASE)) {
			  _strupr(line);
		  }
		  for(char *p=line;*p&&(p=strstr(p,tgt));p++)
			if(!(findflags&F_MATCHWHOLEWORD) ||
			  ((p==line||IsWordDelim(p[-1])) && IsWordDelim(p[tgtLen]))) goto _ret;
	  }
	}
	nResult=0;
	
_ret:     
	m_pFileBuffer->Close();
	return nResult;
}

static DWORD HdrChecksum(const char *pszPathName)
{
    //Return checksum stored at offest NTV_CHKOFFSET in the specified file,
    //or 0L if the file is of the wrong version or cannot be opened and read
	//(for whatever reason) --
    
	int f=_open(pszPathName,_O_BINARY|_O_RDONLY);
	if(f==-1) return 0L;
	
	BYTE hdr[NTV_CHKOFFSET+4];
	DWORD checksum;
	if(_read(f,&hdr,sizeof(hdr))!=sizeof(hdr) ||
	  *(WORD *)&hdr[NTV_VEROFFSET]!=NTV_VERSION) checksum=0L;
	else checksum=*(DWORD *)&hdr[NTV_CHKOFFSET];
	_close(f);
	return checksum;
}

static void deleteWork(LPCSTR path,int *perr)
{
	if(_unlink(path) && errno!=ENOENT) *perr=errno;
}

int CPrjDoc::PurgeWorkFiles(CPrjListNode *pNode,BOOL bIncludeNTA /*=FALSE*/)
{
	int err=0;
	if(pCV && m_pReviewNode==pNode) {
		pCV->Close();
	}
	pNode->m_dwWorkChk=0;
	if(pNode->IsOther() || pNode->m_Name.IsEmpty()) return 0;

	deleteWork(WorkPath(pNode,TYP_NTV),&err);
	deleteWork(WorkPath(pNode,TYP_NTN),&err);
	deleteWork(WorkPath(pNode,TYP_NTS),&err);
	deleteWork(WorkPath(pNode,TYP_NTP),&err);

	if(bIncludeNTA) {
		deleteWork(WorkPath(pNode,TYP_NTA),&err);
		deleteWork(WorkPath(pNode,TYP_NTAC),&err);
	}

	//AfxMessageBox(err?IDS_ERR_NTVPURGE:IDS_PRJ_GOODPURGE,MB_ICONINFORMATION);
	if(err) CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_NTVPURGE1,pNode->Title());

	return err;
}

void CPrjDoc::PurgeAllWorkFiles(CPrjListNode *pNode,
   BOOL bIncludeNTA/*=FALSE*/,BOOL bIgnoreFloating/*=FALSE*/)
{
    //if(pNode->m_dwWorkChk) 
    PurgeWorkFiles(pNode,bIncludeNTA);
    pNode=pNode->FirstChild();
    while(pNode) {
       if(bIgnoreFloating || !pNode->m_bFloating)
         PurgeAllWorkFiles(pNode,bIncludeNTA,bIgnoreFloating);
       pNode=pNode->NextSibling();
    }
}

void CPrjDoc::BranchWorkChk(CPrjListNode *pNode)
{
  //Read header checksums for all workfiles in branch --
  
  pNode->m_dwWorkChk=(pNode->IsOther()||pNode->NameEmpty())?0:HdrChecksum(WorkPath(pNode,TYP_NTV));
  
  if(!pNode->m_bLeaf) {
    CHierListNode *pNext=pNode->m_pFirstChild;
    while(pNext) {
      BranchWorkChk((CPrjListNode *)pNext);
      pNext=pNext->m_pNextSibling;
    }
  }
}

int CPrjDoc::CopyBranchSurveys(CPrjListNode *pNode,char *newPath,int bCopy)
{
   //This CPrjDoc is the current source tree
   //Assume newPath already incorporates pNode's new path property

   int len=strlen(newPath);
   
   if(pNode->IsLeaf()) {
     if(!pNode->NameEmpty()) {
	   strcpy(newPath+len,pNode->m_Name);
	   if(!pNode->IsOther() && !*trx_Stpext(pNode->m_Name)) strcat(newPath,PRJ_SURVEY_EXT);
       bCopy=MyCopyFile(SurveyPath(pNode),newPath,bCopy);
	   if(bCopiedFile) {
		   CLineDoc::UpdateOpenFilePath(m_pathBuf,newPath);
	   }
	   newPath[len]=0;
     }
   }
   else {
	   pNode=pNode->FirstChild();
	   while(bCopy>=0 && pNode) {
			if(!IsAbsolutePath(pNode->m_Path)) {
				if(!pNode->PathEmpty()) {
					strcpy(newPath+len,pNode->m_Path);
					strcat(newPath,"\\");
				}
				bCopy=CopyBranchSurveys(pNode,newPath,bCopy);
				newPath[len]=0;
			}
			pNode=pNode->NextSibling();
	   }
   }
   return bCopy;
}

int CPrjDoc::ResolveLaunchFile(char *pathname,int typ)
{
	CLaunchFindDlg dlg;

	strcat(pathname,(typ==TYP_2D)?"\\*.svg?":"\\*.wrl");
	if((dlg.m_hFile=_findfirst(pathname,&dlg.m_file))==-1L) {
		*pathname=0;
		return IDOK;
	}
	strcpy(dlg.m_fname,dlg.m_file.name);
	dlg.m_ftime=dlg.m_file.time_write;
	if(_findnext(dlg.m_hFile,&dlg.m_file)) {
		strcpy(trx_Stpnam(pathname),dlg.m_fname);
		_findclose(dlg.m_hFile);
		return IDOK;
	}
	dlg.m_strTitle=Title();
	dlg.m_pathname=pathname;
	typ=dlg.DoModal();
	_findclose(dlg.m_hFile);
	return typ;
}

void CPrjDoc::LaunchFile(CPrjListNode *pNode,int typ)
{
	char pathname[_MAX_PATH];
	struct _stat st;
	//time_t typ_time;

	if(!pNode) pNode=GetSelectedNode();

	if(pNode->IsOther() && pNode->IsType(typ)) {
		strcpy(pathname,SurveyPath(pNode));
		goto _open;
	}

	if(typ==TYP_3D || typ==TYP_2D) {
		strcpy(pathname,CMainFrame::Get3DFilePath(typ==TYP_3D));
		int iret=ResolveLaunchFile(pathname,typ);
		if(iret==IDCANCEL) return;
		if(iret==IDOK && *pathname) goto _open;
		*pathname=0;
	}
	else if(*pNode->BaseName()) {
		if(CMainFrame::LineDocOpened(strcpy(pathname,WorkPath(pNode,typ)))) goto _open;
	}
	else *pathname=0;
	
	if(!*pathname || _stat(pathname,&st)) {
		if(typ==TYP_3D || typ==TYP_2D) {
			if(IDOK==CMsgBox(MB_OKCANCEL|MB_ICONINFORMATION,
				(typ==TYP_3D)?IDS_PRJ_NO3DFILE:IDS_PRJ_NO2DFILE,Title())) {
				*pathname=0;
				goto _open;
			}
		}
		else CMsgBox(MB_OK|MB_ICONINFORMATION,(typ==TYP_LOG)?IDS_PRJ_LOGNONE1:IDS_PRJ_LSTNONE1,
	          pNode->Title());
	   return;
	}

	/*
	if(typ==TYP_LST) { 
		typ_time=st.st_mtime;
		
		if(_stat(WorkPath(pNode,TYP_NTV),&st) || typ_time<st.st_mtime) {
		  if(IDOK!=CMsgBox(MB_OKCANCEL|MB_ICONINFORMATION,IDS_PRJ_LSTDATE1,pNode->Title())) return;
		}
	}
	*/
	    
 _open:
 	if(typ==TYP_3D || typ==TYP_2D) CMainFrame::Launch3D(pathname,typ==TYP_3D);
	else theApp.OpenDocumentFile(pathname);
}

CPrjView * CPrjDoc::GetPrjView()
{
	POSITION pos=GetFirstViewPosition();
	CPrjView *pView=(CPrjView *)GetNextView(pos);
	ASSERT(pView && pView->IsKindOf(RUNTIME_CLASS(CPrjView)));
	return pView;
}

CPrjListNode * CPrjDoc::GetSelectedNode()
{
	return GetPrjView()->GetSelectedNode();
}

char * CPrjDoc::ProjectPath()
{
	strcpy(m_pPath+m_lenPath,m_pszName);
	return strcat(m_pPath,PRJ_PROJECT_EXTW);
}

char * CPrjDoc::GetPathPrefix(char *pathbuf,CPrjListNode *pNode)
{
	memcpy(pathbuf,m_pPath,m_lenPath);
	pathbuf[m_lenPath]=0;
	if(pNode) {
		pNode->GetPathPrefix(pathbuf);
		/*
		if(*pathbuf=='\\' || *pathbuf=='/') {
			InsertDriveLetter(pathbuf,m_pPath);
		}
		*/
	}
	return pathbuf;
}

char * CPrjDoc::SurveyPath(CPrjListNode *pNode)
{
    GetPathPrefix(m_pathBuf,pNode);
	if(pNode->IsLeaf() && !pNode->m_Name.IsEmpty()) {
		LPSTR p=m_pathBuf+strlen(m_pathBuf);
		strcpy(p,pNode->m_Name);
		if(!pNode->IsOther()) {
			p=trx_Stpext(p);
			if(!*p) strcpy(p,PRJ_SURVEY_EXT);
		}
	}
	return m_pathBuf;
}

char * CPrjDoc::GetRelativePathName(char *path,CPrjListNode *pNode)
{
	//If possible, transform an absolute path (or pathname)
	//into a path relative to pNode's path property. In other words,
	//chop off its front part if it matches pNode's path.
	int len=strlen(GetPathPrefix(m_pathBuf,pNode));
	if(len<=(trx_Stpnam(path)-path) && !_memicmp(m_pathBuf,path,len)) strcpy(path,path+len);
	return path;
}

void CPrjDoc::RefreshBranch(CPrjListNode *pNode)
{
	pNode->FixParentEditsAndSums(-1);
	BranchFileChk(pNode);
	BranchEditsPending(pNode);
	pNode->FixParentEditsAndSums(1);
	GetPrjView()->m_PrjList.RefreshBranch(pNode,TRUE);
}

void CPrjDoc::RefreshOtherDocs(CPrjDoc *pThisDoc)
{
	CMultiDocTemplate *pTmp=CWallsApp::m_pPrjTemplate;
	CPrjDoc *pDoc;
	
	POSITION pos = pTmp->GetFirstDocPosition();
	while(pos) {
	  pDoc=(CPrjDoc *)pTmp->GetNextDoc(pos);
	  if(pDoc!=pThisDoc) pDoc->RefreshBranch(pDoc->m_pRoot);
	}
}

void CPrjDoc::OnFileClose() 
{
	if(hPropHook && this==pPropView->GetDocument() && !CloseItemProp()) return;
	CDocument::OnFileClose();
}

void CPrjDoc::SendAsEmail(CString &csFile)
{
	CString strOldName = m_strPathName;
	m_strPathName=csFile;
	OnFileSendMail();
	m_strPathName=strOldName;
}

void CPrjDoc::PositionReview()
{
	//Position Review dialog page at pPLT vector.
	ASSERT(pCV && pPLT->vec_id!=0);
	if(pPLT->str_id) {
		pCV->GoToVector();
		UINT iTab=pCV->GetReView()->GetTabIndex();
		if(iTab!=TAB_TRAV) pCV->GetReView()->switchTab(TAB_TRAV);
		pCV->SetTraverse();
	}
	else LocateOnMap();
	pCV->GetParentFrame()->ActivateFrame();
}

void CPrjDoc::VectorProperties()
{
	//Assumes pPLT and pVEC is open and correctly positioned --
	extern BOOL hPropHook;

	CVecInfoDlg dlg;

	ASSERT(pPLT->vec_id!=0);
	UINT id=dlg.DoModal();
	ASSERT(pPLT->vec_id!=0);

	if(dlg.m_pNode) {
		if(id==IDRETRY) {
			CPrjView *pView=m_pReviewDoc->GetPrjView();
			pView->m_PrjList.SelectNode(dlg.m_pNode);
			if(!hPropHook) pView->OpenProperties();
		}
		else if(id==IDIGNORE) LoadVecFile((pPLT->vec_id+1)/2);
		else if(id==IDYES) PositionReview();
	}
}

LPCSTR CPrjDoc::GetDatumStr(char *buf,CPrjListNode *pNode)
{
	PRJREF * pr=pNode->GetAssignedRef();
	CMainFrame::CheckDatumName(pr);
	sprintf(buf,"%s Grid Conv: %.3f  Datum: %s",ZoneStr(pr->zone),pr->conv,pr->datumname);
	return buf;
}

BOOL CPrjDoc::IsActiveFrame()
{
	if(!m_pReviewNode) return FALSE;
	CMapFrame *pFrame=CPlotView::m_pMapFrame;
	while(pFrame && pFrame->m_pFrameNode!=m_pReviewNode) pFrame=pFrame->m_pNext;
	return pFrame!=NULL;
}

void CPrjDoc::RefreshActiveFrames()
{
	if(!m_pReviewNode) return;
	CMapFrame *pFrame=CPlotView::m_pMapFrame;
	while(pFrame) {
		if(pFrame->m_pFrameNode==m_pReviewNode)
			((CMapView *)pFrame->GetActiveView())->RefreshView();
		pFrame=pFrame->m_pNext;
	}
}

BOOL CPrjDoc::SetReadOnly(CPrjListNode *pNode,BOOL bReadOnly,BOOL bNoPrompt /*=0*/)
{
	if(!pNode->m_Name.IsEmpty()) {
		CLineDoc *pDoc=GetOpenLineDoc(pNode);
		if(pDoc) return pDoc->SetReadOnly(pNode,bReadOnly,bNoPrompt);
		//File is not open --
		return SetFileReadOnly(SurveyPath(pNode),bReadOnly,bNoPrompt)==2; //-1=error,0=absent,1-nochange,2=changed
	}
	return FALSE;
}

int CPrjDoc::SetBranchReadOnly(CPrjListNode *pNode,BOOL bReadOnly)
{
  int nFiles=0;
  if(pNode->m_bLeaf) {
	nFiles+=SetReadOnly(pNode,bReadOnly,TRUE); //No prompting
  }
  else {
    pNode=pNode->FirstChild();
    while(pNode) {
      nFiles+=SetBranchReadOnly(pNode,bReadOnly);
	  pNode=pNode->NextSibling();
    }
  }
  return nFiles;
}

int CPrjDoc::DeleteBranchSurveys(CPrjListNode *pNode)
{
   
	if(pNode->IsLeaf()) {
		if(!pNode->NameEmpty()) {
			SHFILEOPSTRUCT shf;
			SurveyPath(pNode);
			m_pathBuf[strlen(m_pathBuf)+1]=0;
			shf.pFrom=m_pathBuf;
			shf.hwnd=0;
			shf.pTo=NULL;
			shf.wFunc=FO_DELETE;
			shf.fFlags=FOF_ALLOWUNDO|FOF_NOERRORUI|FOF_SILENT|FOF_NOCONFIRMATION;
			if(!SHFileOperation(&shf)) return 1;
		}
		return 0;
	}

	UINT cnt=0;
	pNode=pNode->FirstChild();
	while(pNode) {
		cnt+=DeleteBranchSurveys(pNode);
		pNode=pNode->NextSibling();
	}
	return cnt;
}

int CPrjDoc::CountBranchReadOnly(CPrjListNode *pNode)
{
	if(pNode->m_bLeaf) 
		return (!pNode->NameEmpty() && !_access(SurveyPath(pNode),0) && _access(m_pathBuf,6))?1:0;

	int cnt=0;
	pNode=pNode->FirstChild();
	while(pNode) {
		cnt+=CountBranchReadOnly(pNode);
		pNode=pNode->NextSibling();
	}
	return cnt;
}

#ifdef _USE_FILETIMER
void CPrjDoc::TimerRefresh()
{
	m_bCheckForChange=TRUE;
	BranchFileChk(m_pRoot);
	m_bCheckForChange=FALSE;
	BranchEditsPending(m_pRoot);
	GetPrjView()->m_PrjList.RefreshBranch(m_pRoot,TRUE); //include floating branches
}
#endif

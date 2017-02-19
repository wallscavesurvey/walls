// compile.cpp : Compilation/database opperations for CPrjDoc
#include "stdafx.h"
#include "walls.h"
#include "prjdoc.h"
#include "lineview.h"
#include "ntfile.h"
#include "dupdlg.h"
#include "revframe.h"
#include "mapframe.h"
#include "Mapview.h"
#include "compview.h"
#include "plotview.h"
#include "traview.h"
#include "segview.h"
#include "magdlg.h"
#include "dialogs.h"
#include "wall_emsg.h"
#include <trxfile.h>
#include <direct.h>

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define __COMPILE__
#include "compile.h"

#define SIZ_INDEX_CACHE 256
#define SIZ_NTA_CACHE 32

static int (NEAR PASCAL *pAddNames)(vectyp *pVec);
static bool bStrFloating[2];
static bool bStrFixed[2];
static bool bStrBridge[2];
static bool bStrLowerc[2];
static int srv_iSeg;
static PRJREF *pREF;
static short root_zone; //for UTM, zone of compiled item

//For preserving old flag priorities during compilation --
static WORD OSEGwOldFlags,OSEGwNewFlags;
static WORD OSEGwFlagsMax;
static WORD *pOSEGFlagPrior;
static WORD *pOSEGFlagPriorPos;

//Used when calculating global F-ratios for statistics coloring: GetFRatio() --
static UINT net_nc;
static WORD net_ni[2];
static double net_ss[2];
static bool bLogNames;

#ifdef VISTA_INC
//Used for measure tooltip in Plotview & Mapview --
HWND CreateTrackingToolTip(HWND hWnd)
{
    // Create a tooltip.
    HWND hwndTT = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL, 
                                 WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP, 
                                 CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 
                                 hWnd, NULL, NULL, NULL);

    if (!hwndTT)
    {
      return NULL;
    }

    // Set up the tool information. In this case, the "tool" is the entire parent window.
    
	g_toolItem.cbSize   = TTTOOLINFOA_V2_SIZE;
    g_toolItem.uFlags   = TTF_IDISHWND | TTF_TRACK | TTF_ABSOLUTE;
    g_toolItem.hwnd     = hWnd;
    g_toolItem.hinst    = AfxGetInstanceHandle();
    g_toolItem.lpszText = "0,0";
    g_toolItem.uId      = (UINT_PTR)hWnd;
    
    GetClientRect (hWnd, &g_toolItem.rect);

    // Associate the tooltip with the tool window.
    
    SendMessage(hwndTT, TTM_ADDTOOL, 0, (LPARAM) (LPTOOLINFO) &g_toolItem);	
    
    return hwndTT;
}

void DestroyTrackingTT()
{
	if(g_hwndTrackingTT) {
		::SendMessage(g_hwndTrackingTT,TTM_TRACKACTIVATE, (WPARAM)FALSE, (LPARAM)&g_toolItem);
		::DestroyWindow(g_hwndTrackingTT);
		g_hwndTrackingTT = NULL;
	}
}

#endif

void GetDistanceFormat(CString &s,BOOL bProfile,bool bFeetUnits, bool bConvert)
{
	int dec=1;
	bool bFeetDisp=bConvert?!bFeetUnits:bFeetUnits;

	double dy=dpCoordinate.y-dpCoordinateMeasure.y;
	if(bConvert) dy *= (bFeetUnits?0.3048:(1/0.3048));
	LPCSTR pu=bFeetDisp?"ft":"m";

	if(bProfile) {
		if(dy<10) dec++; else if(dy>=1000) dec--;
		s.Format("%.*f %s elev chg", dec, dy, pu);
	}
	else {
		double dx=dpCoordinate.x-dpCoordinateMeasure.x;
		if(bConvert) dx *= (bFeetUnits?0.3048:(1/0.3048));
		double az=atan2(dx,dy)*(180.0/3.141592653589793);
		if(az<0.0) az+=360.0;
		dx=sqrt(dx*dx+dy*dy);
		if(bFeetDisp) {
			if(dx>5280.0) {
				dx/=5280.0; pu="mi";
			}
		}
		else if(dx>1000.0) {
			dx/=1000.0; pu="km";
		}
		if(dx<10) dec++; else if(dx>=1000) dec--;
		s.Format("%.*f %s %.1f°",dec,dx,pu,az);
	}
}

CNETData::CNETData(int nFiles)
{
	pNET=(NET_WORK *)malloc(nFiles*sizeof(NET_WORK));
}

CNETData::~CNETData() {
  free(pNET);
}

static apfcn_i GoWorkFile(CDBFile *pDB,UINT rec)
{
    if(pDB->Go(rec)) {
      CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_BADWORK);
      return FALSE;
    }
    return TRUE;
}

int CPrjDoc::InitCompView(CPrjListNode *pNode)
{
    m_ReviewScale=pNode->IsFeetUnits()?3.28084:1.0;
	m_iFindVector=m_iFindStation=m_bVectorDisplayed=0;
	m_iValueRangeNet=0;

    if(pCV) {
		if(m_bReviewing) pCV->GetParentFrame()->ShowWindow(SW_HIDE);
		m_pReviewNode=pNode;
		//CSegView::ResetContents() updates NTA file.
		//CCompView::ResetContents() uses m_pReviewNode to update window title --
		((CReView *)pCV->m_pTabView)->ResetContents();   //Invokes the above
		
    	CloseWorkFiles(); //Does not depend on m_pReviewNode
    	if(m_pReviewDoc!=this) {
    	  pCV->m_pTabView->ChangeDoc(this);
    	  m_pReviewDoc=this;
    	}
		pCV->BringToTop();
	}
    else {
        TRY {
	        ASSERT(!dbNTN.Opened() && !dbNTS.Opened());
	 		CRevFrame* pNewFrame = (CRevFrame *)CWallsApp::m_pRevTemplate->CreateNewFrame(this, NULL);
			if (pNewFrame == NULL) goto _err;

			ASSERT(pNewFrame->IsKindOf(RUNTIME_CLASS(CRevFrame)));
	        m_pReviewNode=pNode;
	        m_pReviewDoc=this;
			CWallsApp::m_pRevTemplate->InitialUpdateFrame(pNewFrame,this);
			pCV=(CCompView *)pNewFrame->GetActiveView();
			VERIFY(pPV=pCV->GetPlotView());
			VERIFY(pSV=pCV->GetSegView());
			VERIFY(pGV=pCV->GetPageView());
			VERIFY(pTV=pCV->GetTraView());

			ASSERT(pCV->IsKindOf(RUNTIME_CLASS(CCompView)));
		}
		CATCH(CMemoryException,e) {
			goto _err;
		}
		END_CATCH
    }
    return 0;

_err:
	CMsgBox(MB_ICONEXCLAMATION, IDS_ERR_MEMORY);
	m_pReviewNode=NULL;
	m_pReviewDoc=NULL;
	m_bPageActive=FALSE;
	return -1;
}

static LPBYTE PASCAL get_name_key(LPBYTE name,const LPBYTE namarr,DWORD pfx)
{
    //Retrieve pascal-type string from blank-extended field -- store up to NTV_SIZNAME+3 bytes
    LPBYTE p=name+1;
    if(pfx) {
      *p++=(BYTE)(0x80|(pfx>>8));
      *p++=(BYTE)(pfx&0xFF);
    }
    for(int i=0;i<NTV_SIZNAME && (*p=namarr[i])!=' ';i++) p++;
	name[0]=p-name-1; //prefix length byte
	return name;
}

static int PASCAL AddName(long *id,DWORD pfx,char *pName,DWORD dwFixed)
{
    DWORD w=0;
	
    if(!dwFixed && !memcmp(pCache,pName,NTV_SIZNAME) && pfx==pfxCache) w=wCache;
    else {
		BYTE name[NTV_SIZNAME+4];
		int e;
		numnames++;
		w=(numnames|dwFixed);
	    if(e=pixNAM->InsertKey(&w,get_name_key(name,(BYTE *)pName,pfx))) {
	        numnames--;
			if(e!=CTRXFile::ErrDup) return IDS_ERR_TRXBUILD;
			//The name was found (not inserted) in the index -- 
			pixNAM->GetRec(&w,sizeof(w));
			//Let's not bother caching an old name --
			if(dwFixed) {
				if(dwFixed&w) {
					//This is a duplicated fixed station with at least one of
					//the components having zero variance in each instance.
					//We'll completely ignore this shot and log a warning.
					GetStrFromFld((LPSTR)name,pName,NTV_SIZNAME);
					CString msg;
					msg.Format(IDS_ERR_FIXDUPLICATE,(LPCSTR)name);
					srv_ErrMsg((LPSTR)(LPCSTR)msg,SRV_ERR_LOGMSG);
					return SRV_ERR_LOGMSG;
				}
				//Let's accept the duplicate while updating the fixed flags as necessary --
				if((dwFixed|=w)!=w) pixNAM->PutRec(&dwFixed);
			}
			w&=0x3FFFFFFF;
	    }
	    else {
            ASSERT(!pixNAM->GetRec(&w,sizeof(w)) && (w&0x3FFFFFFF)==numnames);
		    //We inserted a new name in the index. Lets cache it --
		    memcpy(pCache,pName,NTV_SIZNAME);
		    wCache=w=numnames;
		    pfxCache=pfx;
		}
	}

	ASSERT((w&~0x3FFFFFFF)==0);
	*id=w;
	return 0;
}

static int NEAR PASCAL AddNames(vectyp *pVec)
{
  int e;
  DWORD dwFixed=0;
  
  if(pVec->flag&NET_VEC_FIXED) {
    pVec->id[0]=pVec->pfx[0]=0;
    memcpy(pVec->nam[0],"<REF>   ",8);
	//Check for a dublocated fixed point --
	dwFixed=((pVec->hv[0]==0.0)+2*(pVec->hv[1]==0.0))<<30;
  }
  else if(e=AddName(&pVec->id[0],pVec->pfx[0],pVec->nam[0],0)) return e; 
  
  //Add check for pVec->id[0]==pVec->id[1] in case DLL fails to do this?
  return AddName(&pVec->id[1],pVec->pfx[1],pVec->nam[1],dwFixed);
}

static int NEAR PASCAL CheckNames(vectyp *pVec)
{
	int e,off2;
	BYTE dblname[2*NTV_SIZNAME+8]; //was +4
	
	if(e=AddNames(pVec)) return e;
	
	//if(pVec->flag&NET_VEC_FIXED) return 0;
	
	//Construct a key consisting of a sorted name pair: <len><0><nam0><0><nam1>
	
	int n0=memcmp(pVec->nam[1],pVec->nam[0],NTV_SIZNAME);
	
	if(!n0) n0=pVec->pfx[1]<pVec->pfx[0];
	else n0=n0<0;

	//store up to NTV_SIZNAME+3 bytes at dblname+1, with dblname[1] as large as NTV_SIZNAME+2
	get_name_key(dblname+1,(LPBYTE)pVec->nam[n0],pVec->pfx[n0]);
	off2=dblname[1]+2; //offset of second name prefix byte
	dblname[1]=0;
	get_name_key(dblname+off2,(LPBYTE)pVec->nam[1-n0],pVec->pfx[1-n0]);
	*dblname=off2+dblname[off2];
	dblname[off2]=0; //dblname = <2*NTV_SIZNAME+4+2)<0><nn><abcdefgh><0><nn><abcdefgh>
	
    if(e=pixNAM->InsertKey(&numvectors,dblname)) {
      if(e!=CTRXFile::ErrDup) return IDS_ERR_TRXBUILD;
      //The name pair is a duplicate!
      pixNAM->GetRec(&e,sizeof(e)); //Get the database record number (less 1) of the first occurrence --
      if(dbNTV.Go(e+1)) return IDS_ERR_TRXBUILD; //Should not happen!
      
      DUPLOC loc[2];
      
      dblname[*dblname+1]=0; //Allow printing of 2nd name 
      {
	      char *p;
	      p=(char *)dblname+2;
	      if(*p&0x80) p+=2;
	      loc[0].pName=p;
	      p=(char *)dblname+off2+1;
	      if(*p&0x80) p+=2;
	      loc[1].pName=p;
      }
      GetStrFromFld(loc[1].File,pVec->filename,NTV_SIZNAME);
      loc[1].nLine=pVec->lineno;
      pVec=(vectyp *)dbNTV.RecPtr();
      GetStrFromFld(loc[0].File,pVec->filename,NTV_SIZNAME);
      loc[0].nLine=pVec->lineno;

	  if(!bLogNames) {
		CDupDlg dlg(&loc[0],&loc[1]);
		if((e=dlg.DoModal())==IDOK || e==IDCANCEL) {
			bLogNames=(e==IDCANCEL);
			e=0;
		}
	  }

	  if(bLogNames) {
		  CString cs;
		  //"FSB160:24 - Duplication of shot H3 to H4.  See FSB116:46."
		  cs.Format("%s:%u\tDuplication of shot %s to %s. See %s:%u\r\n",
			  loc[1].File,loc[1].nLine,
			  loc[0].pName,loc[1].pName,
			  loc[0].File,loc[0].nLine);
		  CPrjDoc::m_pLogDoc->log_write(cs);
		  return 0;
	  }

	  if(!e) return 0; //continue

	  /* No longer allowing this option
      if(e>IDABORT) {
		  //open file(s) for editing
	      CPrjDoc::m_pErrorNode=pBranch->FindName(loc[0].File);
	      CPrjDoc::m_pErrorNode2=pBranch->FindName(loc[1].File);
	      e=(CPrjDoc::m_pErrorNode==CPrjDoc::m_pErrorNode2 && loc[0].nLine>loc[1].nLine)?1:0;
		  CPrjDoc::m_nLineStart=(LINENO)loc[e].nLine;
		  CPrjDoc::m_nCharStart=0;
		  CPrjDoc::m_nLineStart2=(LINENO)loc[1-e].nLine;
	  }
	  */
	  ASSERT(e==IDABORT);

      return SRV_ERR_DISPLAYED;
    }
    
	ASSERT(!pixNAM->GetRec(&e,sizeof(e)) && (DWORD)e==numvectors);

	return 0;
}

static int PASCAL AddNameErrMsg(int e)
{
	if(e!=SRV_ERR_DISPLAYED && e!=SRV_ERR_LOGMSG) {
		CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_TRXBUILD,pixNAM->Errstr());
		e=SRV_ERR_DISPLAYED;
	}
	return e;
}

static int PASCAL AppendNTV(vectyp *pVec)
{
	int e;
   
	if(e=pAddNames(pVec)) {
		return AddNameErrMsg(e);
	}
	
	if(!dbNTV.AppendRec(pVec)) {
	  if(!(++numvectors%SRV_DLG_INCREMENT)) {
	    Pause();
	    if(!pCV) return SRV_ERR_DISPLAYED;
	    pCV->SetTotals(numvectors,fTotalLength);
	  }
	  return 0;
	}
	
	CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_NTVBUILD,dbNTV.Errstr());
	return SRV_ERR_DISPLAYED;
}

static BOOL PASCAL SetSurvey(char *pName)
{
     ASSERT(numfiles<maxfiles);
     
     if(numfiles==maxfiles) return TRUE;
     
     NET_WORK *pnet=pNET+numfiles++;
     
     if(pName) GetFldFromStr((BYTE *)pnet->name,pName,NTV_SIZNAME);
     pnet->length=(fTotalLength-fTotalLength0);
     pnet->vectors=numvectors-numvectors0;
     numvectors0=numvectors;
     fTotalLength0=fTotalLength;
     
	 Pause();
	 if(pCV) {
	   pCV->NetMsg(pnet,TRUE);
	   return TRUE;
	 }
	 return FALSE;
}

int CPrjDoc::OpenWork(PPVOID pRecPtr,CDBFile *pDB,CPrjListNode *pNode,int typ)
{
    int e;
    
    char *pathName=WorkPath(pNode,typ);
    
    if(pDB->Opened()) return TRUE;
	if((e=pDB->Open(pathName,CDBFile::ReadWrite)) || !pDB->Cache() &&
	    	(e=pDB->AllocCache(NTV_BUFRECS*pDB->SizRec(),NTV_NUMBUFS,TRUE))) {
	    if(e==CDBFile::ErrName) CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_MEMORY);
	    else CMsgBox(MB_ICONEXCLAMATION,"%s:  %s",pDB->Errstr(e),pathName);
	    if(pDB->Opened()) pDB->Close();
	    return FALSE;
	}
	if(typ==TYP_NTS && *(WORD *)pDB->HdrRes2()!=NET_NTS_VERSION) {
		pDB->Close();
		AfxMessageBox(IDS_ERR_NETLIB);
		return FALSE;
	};

    *pRecPtr=pDB->RecPtr();
	return TRUE;
}

//Non-inline may save space do to string litteral --
int CPrjDoc::OpenSegment(CTRXFile *pix,CPrjListNode *pNode,int mode)
{
	//If mode==0, OpenWorkFiles() is opening the segment for read-write access.
	//The CPanelDialogs should already have been created with InitCompView().

	UINT version=0;
	
	int e=pix->Open(WorkPath(pNode,TYP_NTA),mode);

#ifdef _DEBUG
	if(!e) {
		version=pix->SizExtra();
		version=sizeof(SEGHDR);
		version=*(WORD *)pix->ExtraPtr();
		version=NTA_VERSION;
		version=0;
	}
#endif
	
	if(e ||	(version=*(WORD *)pix->ExtraPtr())!=NTA_VERSION || pix->SizExtra()!=sizeof(SEGHDR)) {

		if(!e || e==CTRXFile::ErrName) {
			if(!e) {
				if(mode==CTRXFile::ReadOnly) {
					//Recompilation in which old NTA is being scanned -- At this
					//point, either the version is different or the header size doesn't match

					//If the major version number matches (in which case there is still a header
					//mismatch), use old segment records. The caller, Compile(), will detect the
					//mismatch and notify the user of some style losses as required --
					if((version&0xFF00)==(NTA_VERSION&0xFF00)) return 0;

					//Otherwise, the major version differs. Notify the user and either
					//cancel or discard old NTA completely --

					if(IDOK!=CMsgBox(MB_OKCANCEL,IDS_ERR_WORKFILE)) {
						pix->Close(FALSE);
						return -1; //Cancel compile
					}
					//Ignore old version completely --
					pix->Close(FALSE); //Do NOT delete default cache!
					return CTRXFile::ErrFind;
				}
				//We are opening the NTA for read/write access. If we are calling from Review()
				pix->Close();
				e=IDS_ERR_BADWORK;
			}
			else e=IDS_ERR_MEMORY;
			
			CMsgBox(MB_ICONEXCLAMATION,e);
			e=-1;
		}
		else if(mode==CTRXFile::ReadWrite || e!=CTRXFile::ErrFind) {
			goto _errmsg;
		}
		//Return ErrFind if we are opening in ReadOnly mode --
	}
	else if(mode==CTRXFile::ReadWrite) {
		if(e=pix->AllocCache(SIZ_NTA_CACHE,TRUE)) {
			goto _errmsg;
		}
		pixSEGHDR=(SEGHDR *)pix->ExtraPtr();
	}
	
	return e;
  
_errmsg:
	CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_SEGOPEN1,pix->Errstr(e));
	return -1;
}

static WORD AddSegment(/*statstyp *pStats*/)
{
  //Insert new segment path or position index at the matched
  //segment path. Return the resulting record number (seg_id),
  //or -1 in case of file error.
  //If it is a new segment we retrieve the style from the old index
  //if it exists.
  
  bSegChg=FALSE;
  memset(&SegRec,0,sizeof(styletyp)); //Assumes doubles will init to 0.0 
  CSegView::InitDfltStyle(&SegRec,pixSEGHDR);
  SegRec.seg_id=(WORD)ixSEG.NumRecs();
//  if(pStats) SegRec.stats=*pStats;
  
  //First, try insertion into ixSEG --
  int e=ixSEG.InsertKey(&SegRec,(LPBYTE)SegBuf);
  if(e) {
    if(e==CTRXFile::ErrDup &&
     !(e=ixSEG.GetRec(&SegRec,sizeof(styletyp)))) return SegRec.seg_id;
    goto _error;
  }
  
  //We successfully inserted a new segment. Let's retrieve the style
  //from the original NTA file if it exists --
  
  ASSERT(SegRec.seg_id+1==(WORD)ixSEG.NumRecs());
  
  if(pixOSEG && !pixOSEG->Find(SegBuf)) {
        styletyp segold;
        ASSERT(!ixSEG.GetRec(&segold,sizeof(styletyp)) && segold.seg_id==SegRec.seg_id);
        e=pixOSEG->GetRec(&segold,sizeof(styletyp));
        if(!e) {
		  segold.seg_id=SegRec.seg_id;
          SegRec=segold;
          e=ixSEG.PutRec(&SegRec);
        }
        if(e) goto _error;
  }
  return SegRec.seg_id;
  
_error:
  //Build error --
  CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_TRXBUILD,ixSEG.Errstr(e));
  return (WORD)-1;
}

static int InitSegmentPath(CPrjListNode *pNode,int iSeg)
{
	if(!iSeg) {
		ASSERT(pNode==pBranch);
		iSeg=1;
	}
	else if(pNode->IsDefineSeg()) {
		//A node with an empty name or a name too long simply doesn't contribute --
		char *p=pNode->BaseName();
		int len=strlen(p);
		if(len) {
			if(iSeg+len+(iSeg>1)>=SRV_SIZ_SEGBUF) {
				//Should be impossible --
				CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_SEGSIZE);
				return (WORD)-1;
			}
			if(iSeg>1) SegBuf[iSeg++]=SRV_SEG_DELIM;
			memcpy(SegBuf+iSeg,p,len);
			iSeg+=len;
		}
	}	
	
	//Add this component to the segment tree if required.
	//The index will remain positioned at this record
	//and SegRec will contain the current record --
	
    SegBuf[0]=iSeg-1; //make at a pascal string --
    
    return (AddSegment(/*NULL*/)==(WORD)-1)?-1:iSeg;  
}

static WORD AddSegmentPath(int iSeg,char FAR *key/*,statstyp *pStat*/)
{
   //Let's append longest possible prefix of key to SegBuf --
   int len=*key;
   ASSERT(iSeg>0 && iSeg<=SRV_SIZ_SEGBUF);
   if(len+iSeg+(iSeg>1)>SRV_SIZ_SEGBUF) {
      CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_SEGSIZE);
      return (WORD)-1;
   }
   if(len) {
     if(iSeg>1) SegBuf[iSeg++]=SRV_SEG_DELIM;
     memcpy(SegBuf+iSeg,key+1,len);
   }
   SegBuf[0]=iSeg+len-1;
   return AddSegment(/*pStat*/);  
}

int TrimPrefix(LPBYTE *argv,BOOL bMatchCase)
{
	//Trims prefix from string *argv, returning prefix id.
	//Returns -1 if no prefix present, -2 if prefix invalid.
	
	LPBYTE p=(LPBYTE)strrchr(*(LPSTR *)argv,SRV_CHAR_PREFIX);
	char buf[SIZ_EDITNAMEBUF];
	char key[SIZ_EDITNAMEBUF];
	short rec;
		
	if(!p) return -1;
	
	if(p==*argv) {
	  (*argv)++;
	  return 0;
	}
	//Empty name portion not allowed --
	if(!*(p+1)) goto _error;
	
	ASSERT(p-*argv<SIZ_EDITNAMEBUF);
	memcpy(buf+1,*argv,*buf=p-*argv);
	
	ixSEG.UseTree(NTA_PREFIXTREE);
	
	if(!ixSEG.Find(buf)) {
	  ixSEG.GetRec(&rec,sizeof(rec));
    }
    else {
      rec=0;
      if(!bMatchCase && !ixSEG.First()) {
        do {
        	ixSEG.GetKey(key,SIZ_EDITNAMEBUF);
        	if(*key==*buf && !memicmp(key+1,buf+1,*buf)) {
        	  ixSEG.GetRec(&rec,sizeof(rec));
	  		  *argv=p+1;
        	  break;
        	}
        }
        while(!ixSEG.Next());
      }
    }
	ixSEG.UseTree(NTA_SEGTREE);
	
	if(rec>0) {
	  *argv=p+1;
	  return rec;
	}
	
_error:
    CMsgBox(MB_ICONINFORMATION,IDS_ERR_PFXNOFIND,*argv);
	return -2;
}	

static int PASCAL LogPrefix(char *name)
{
	int e;
	short pfx=0;
	
    if(!(e=ixSEG.Find((LPBYTE)name))) ixSEG.GetRec(&pfx,sizeof(pfx));
    else if(e==CTRXFile::ErrMatch || e==CTRXFile::ErrEOF) {
    	pfx=(short)ixSEG.NumRecs()+1;
    	if(e=ixSEG.InsertKey(&pfx,(LPBYTE)name)) pfx=0;
    }
    return pfx;
}

static int PASCAL SetOptions(CPrjListNode *pNode)
{
	int e=0;
	if(pNode->Parent() && (e=SetOptions(pNode->Parent()))!=0) return e;

    if(!pNode->OptionsEmpty() && (e=srv_SetOptions(pNode->Options()))!=0) {
	  CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_SRVOPTION,pNode->Title(),srv_ErrMsg(errBuf,e));
 	  srv_Close();
      e=SRV_ERR_DISPLAYED;
    }
    return e;
}

static BOOL ParseFlagSymbol(NTA_FLAGTREE_REC *pnt,LPSTR lpStr)
{
	UINT shade,shape,size;
	char buf[4];
	
	ASSERT(lpStr && *lpStr);
	
	switch(*lpStr++) {
	  case '-' : shade=CPlotView::m_mfPrint.bFlagSolid; break;
	  case 'C' : shade=FST_CLEAR; break;
	  case 'S' : shade=FST_SOLID; break;
	  case 'O' : shade=FST_OPAQUE; break;
	  case 'T' : shade=FST_TRANSPARENT; break;
	  default  : return FALSE;
	}
	switch(*lpStr++) {
	  case  0  : lpStr--;
	  case '-' : shape=CPlotView::m_mfPrint.iFlagStyle; break;
	  case 'S' : shape=FST_SQUARES; break;
	  case 'C' : shape=FST_CIRCLES; break;
	  case 'T' : shape=FST_TRIANGLES; break;
	  case 'P' : shape=FST_PLUSES; break;
	  default  : return FALSE;
	}
	if(*lpStr && *lpStr!='-') {
	  if(strlen(lpStr)>3) return FALSE;
	  strcpy(buf,lpStr);
	  if((size=atoi(buf))>M_SIZEMASK) return FALSE;
	}
	else size=M_SIZEMASK;

    M_SETSHADE(pnt->seg_id,shade);
    M_SETSHAPE(pnt->seg_id,shape);
	if(size!=M_SIZEMASK) M_SETSIZE(pnt->seg_id,size);

	return TRUE;
}

static void UpdateFlagPriorities(void)
{
	UINT i,nPrior=0;
	NTA_FLAGTREE_REC rec;

	ASSERT((OSEGwOldFlags+OSEGwNewFlags)==pixSEGHDR->numFlagNames);

	for(i=0;i<OSEGwFlagsMax;i++) {
		if(pOSEGFlagPriorPos[i]) pOSEGFlagPrior[pOSEGFlagPriorPos[i]-1]=OSEGwNewFlags+nPrior++;
	}

	ixSEG.UseTree(NTA_NOTETREE);

	VERIFY(!ixSEG.First());

	i=0;
	do {
		VERIFY(!ixSEG.GetRec(&rec,sizeof(rec)));
		if(!M_ISFLAGNAME(rec.seg_id)) break;

		if(rec.iseq&0x8000) {
			rec.iseq=pOSEGFlagPrior[rec.iseq&0x7FFF];
			ASSERT(rec.iseq<OSEGwNewFlags+nPrior);
			VERIFY(!ixSEG.PutRec(&rec));
			if(++i==OSEGwOldFlags) break;
		}
	}
	while(!ixSEG.Next());

	ASSERT(i==OSEGwOldFlags);

	ixSEG.UseTree(NTA_SEGTREE);
}

static void Mag_ErrMsg()
{
	CString rootZoneStr(ZoneStr(pNTV->root_zone));
	MD.datum=pNTV->root_datum;
	MF(MAG_GETDATUMNAME);
	CString rootDatum(MD.modname);
	//MD.datum=pNTV->ref_datum;
	//MF(MAG_GETDATUMNAME);
	CMsgBox(MB_ICONINFORMATION,"Failure converting #FIX coordinates to %s %s coordinate system of branch being compiled. "
		" Coordinates are either invalid or out of range for such a conversion.",
		(LPCSTR)rootZoneStr,(LPCSTR)rootDatum);
}

static int PASCAL srv_CB(int fcn_id,LPSTR lpStr)
{
	int e=0;
	static NTA_NOTETREE_REC rec;
	BYTE buf[TRX_MAX_KEYLEN+1];
	
	switch(fcn_id) {
		case FCN_DATEDECL:
		{
			DWORD dw=*(DWORD *)pNTV->date;
			MD.y=((dw>>9)&0x7FFF);
			MD.m=((dw>>5)&0xF);
			MD.d=(dw&0x1F);
			if(e=MF(MAG_CVTDATE)) {
			  if(e==MAG_ERR_ELEVRANGE)CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_DATEELEV);
			  else CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_DATEDECL3,MD.y,MD.m,MD.d);
			  return SRV_ERR_MAGDLL;
			}
			*(double *)lpStr=(double)MD.decl;
			return 0;
		}

		case FCN_CVTDATUM:
			MD.east=pNTV->xyz[0];
			MD.north=pNTV->xyz[1];
			if(MF(MAG_CVTDATUM2+pNTV->root_datum)) {
			  Mag_ErrMsg();
			  return SRV_ERR_MAGDLL;
			}
			pNTV->xyz[0]=MD.east;
			pNTV->xyz[1]=MD.north;
			ASSERT(pREF);
			CMainFrame::PutRef(pREF); //restore MD
			return 0;

		case FCN_CVT2OUTOFZONE:
			ASSERT(pREF && pNTV->root_zone!=pNTV->ref_zone);
			MD.format=0;
			MD.east=pNTV->xyz[0];
			MD.north=pNTV->xyz[1];
			ASSERT(MD.zone==pNTV->ref_zone);
			MD.zone=pNTV->ref_zone;
			e=MF(MAG_CVTUTM2LL3); //returns whatever LL is in range, no conv or decl required here
			if(!e) {
				MD.zone=pNTV->root_zone; //force to root zone
				e=MF(MAG_CVTLL2UTM3);
				if(e==MAG_ERR_OUTOFZONE) e=0;
			}
			if(e) {
				Mag_ErrMsg();
				return SRV_ERR_MAGDLL;
			}
			pNTV->xyz[0]=MD.east;
			pNTV->xyz[1]=MD.north;
			CMainFrame::PutRef(pREF); //restore MD
			return 0;

		case FCN_CVTLATLON:
			ASSERT(pREF);
			MD.format=0;
			MD.lat.isgn=MD.lon.isgn=1;
			MD.lon.fdeg=pNTV->xyz[0];
			MD.lat.fdeg=pNTV->xyz[1];
			MD.lon.fmin=MD.lon.fsec=MD.lat.fmin=MD.lat.fsec=0.0;

			MD.zone=pNTV->root_zone; //force to root zone
			if((e=MF(MAG_CVTLL2UTM3)) && e!=MAG_ERR_OUTOFZONE) {
			   Mag_ErrMsg();
			   return SRV_ERR_MAGDLL;
			}
			pNTV->xyz[0]=MD.east;
			pNTV->xyz[1]=MD.north;
			CMainFrame::PutRef(pREF); //restore MD
			return 0;

		case FCN_SEGMENT:
		{
			//A new segment was encountered --
			if(bSegChg) ixSEG.PutRec(&SegRec);
			return ((pNTV->seg_id=AddSegmentPath(srv_iSeg,lpStr))==(WORD)-1)?SRV_ERR_DISPLAYED:0;
    	}
    	
 		case FCN_PREFIX:
 		{
			e=strlen(lpStr);
			if(e>SRV_LEN_PREFIX) return 0;
			memcpy(buf+1,lpStr,*buf=e); //Pascal string required
			ixSEG.UseTree(NTA_PREFIXTREE);
			e=LogPrefix((char *)buf);
			ixSEG.UseTree(NTA_SEGTREE);
			return e;
		}
		
		case FCN_FLAGNAME:
		{
			NTA_FLAGTREE_REC *pRec=(NTA_FLAGTREE_REC *)&rec;

			e=lpStr?strlen(lpStr):0;
			if(e>TRX_MAX_KEYLEN) return SRV_ERR_BADFLAGNAME;
			if(*buf=e) memcpy(buf+1,lpStr,e);
			ixSEG.UseTree(NTA_NOTETREE);
			//Initially set prority to same as ID (order names encountered)
			pRec->id=pixSEGHDR->numFlagNames; //zero-based!
			pRec->iseq=(OSEGwFlagsMax?OSEGwNewFlags:pRec->id); 
			pRec->color=RGB_WHITE;
			pRec->seg_id=M_STYLEINIT; //Default attributes
			M_SETSHADE(pRec->seg_id,CPlotView::m_mfPrint.bFlagSolid);
	    	if(e=ixSEG.InsertKey(pRec,buf)) {
	    	  if(e==CTRXFile::ErrDup) ixSEG.GetRec(pRec,sizeof(NTA_FLAGTREE_REC));
			  else rec.id=(DWORD)-1;  //Ignore other errors here
			}
			else {
				pixSEGHDR->numFlagNames++;
				//We successfully inserted a new flag name with default style attributes.
				//Let's retrieve the symbol style from the original NTA file if it exists.
				//We will need to clean up later to preserve priorities. The color, style,
				//and hide/show state (high byte of color) will be preserved now, however. 
				if(OSEGwFlagsMax) {
					pixOSEG->UseTree(NTA_NOTETREE);
					if(!pixOSEG->Find(buf)) {
						NTA_FLAGTREE_REC recold;
						VERIFY(!pixOSEG->GetRec(&recold,sizeof(recold)));
						pRec->iseq=(OSEGwOldFlags|0x8000);
						ASSERT(recold.iseq<OSEGwFlagsMax);
						pOSEGFlagPriorPos[recold.iseq]=++OSEGwOldFlags;
						pRec->seg_id=recold.seg_id;
						pRec->color=recold.color;
						VERIFY(!ixSEG.PutRec(pRec));
					}
					else OSEGwNewFlags++; 
					pixOSEG->UseTree(NTA_SEGTREE);
				}
			}
			ixSEG.UseTree(NTA_SEGTREE);
			pNTV->charno=pRec->id; //Used if this is a default flag name
			return 0;
	    }
	    
	    case FCN_SYMBOL:
	    {
	    	//srv_CB was just called with FCN_FLAGNAME --
	    	if(rec.id==(DWORD)-1) return SRV_ERR_NOFLAGNAME;
			if(!ParseFlagSymbol((NTA_FLAGTREE_REC *)&rec,lpStr)) return SRV_ERR_BADFLAGNAME;
			if(pNTV->lineno!=(DWORD)-1) rec.color=pNTV->lineno;
			ixSEG.UseTree(NTA_NOTETREE);
			ixSEG.PutRec(&rec);
			ixSEG.UseTree(NTA_SEGTREE);
	    	return 0;
	    }
	    
	    case FCN_FLAG:
	    case FCN_NOTE:
	    {
	    	vectyp *pVec=(vectyp *)((LPBYTE)pNTV+NTV_RECOFFSET);
			NTA_NOTETREE_REC frec;
			if(e=AddName((long *)&frec.id,pVec->pfx[0],pVec->nam[0],0)) return AddNameErrMsg(e);
			frec.seg_id=pVec->seg_id;
			*buf=1+sizeof(DWORD);
			*((DWORD *)(buf+2))=frec.id;
		
			ixSEG.UseTree(NTA_NOTETREE);

		    if(fcn_id==FCN_NOTE) {
			  buf[1]=NTA_CHAR_NOTEPREFIX;
		      frec.seg_id|=(M_NOTES<<M_NOTESHIFT);
			  e=strlen(lpStr);
	    	  ASSERT(e<=TRX_MAX_KEYLEN-5);
	    	  memcpy(buf+6,lpStr,e);
	    	  *buf+=e;
	        }
	        else {
			  buf[1]=NTA_CHAR_FLAGPREFIX;
		      frec.seg_id|=(M_FLAGS<<M_NOTESHIFT);
			  //This is a fixed station assignment or rec.id was initialized
			  //in a previous FCN_FLAGNAME call --
			  *((WORD *)(buf+6))=(lpStr?(*(WORD *)lpStr):((WORD)rec.id));
			  *buf+=sizeof(WORD);
		    }
	    
	    	if(e=ixSEG.InsertKey(&frec,(LPBYTE)buf)) {
	    	  if(e==CTRXFile::ErrDup) e=0;
	    	  else {
		    	  CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_NTVBUILD,ixSEG.Errstr());
		          e=SRV_ERR_DISPLAYED;
		      }
	        }
 	        ixSEG.UseTree(NTA_SEGTREE);
 	        return e;
		} /*case FCN_NOTE or FCN_FLAG*/
		
	    case FCN_LRUD:
	    {
	    	SRV_LRUD *pLrud=(SRV_LRUD *)lpStr;
			ASSERT((pLrud->az!=LRUD_BLANK) || (pLrud->dim[0]==LRUD_BLANK_NEG));
			if(e=AddName((long *)&buf[1],pLrud->pfx,pLrud->nam,0)) return AddNameErrMsg(e);
			*buf=3;
		
			ixSEG.UseTree(NTA_LRUDTREE);
			ASSERT(ixSEG.SizRec()==sizeof(NTA_LRUDTREE_REC));

	    	if(e=ixSEG.InsertKey(pLrud->dim,(LPBYTE)buf)) {
		      CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_NTVBUILD,ixSEG.Errstr());
		      e=SRV_ERR_DISPLAYED;
	        }

 	        ixSEG.UseTree(NTA_SEGTREE);
 	        return e;
		} /*case FCN_LRUD*/

		case FCN_ERROR:
		{
			return CPrjDoc::m_pLogDoc->log_write(lpStr);
		}

	} /*switch*/
		
	ASSERT(FALSE);
		
	return e;
}

int CPrjDoc::CompileBranch(CPrjListNode *pNode,int iSeg)
{
   if(!pNode->m_dwFileChk) return 0; //always the case with Other nodes

   //Initialize SegRec/SegBuf and obtain SegBuf insertion point (>=1) for new segments.
   if((iSeg=InitSegmentPath(pNode,iSeg))<0) return -1; 
     
   if(pNode->m_bLeaf) {
   
	 vectyp *pVec=(vectyp *)((LPBYTE)pNTV+NTV_RECOFFSET);

	 pNTV->charno = SRV_FIXVTLEN*pNode->InheritedState(FLG_VTLENMASK) +
					SRV_FIXVTVERT*pNode->InheritedState(FLG_VTVERTMASK);

	 if(pREF=pNode->GetAssignedRef()) {
		pNTV->charno |= SRV_USEDATES*pNode->InheritedState(FLG_DATEMASK);
		pNTV->grid=pREF->conv;
		if((pNTV->charno & SRV_USEDATES) || pNTV->root_datum!=255) {
			CMainFrame::PutRef(pREF);
			if(pNTV->root_datum>REF_MAXDATUM && pNTV->root_datum!=255) {
				pNTV->root_datum=(pNTV->root_datum==REF_NAD27)?MF_NAD27():MF_WGS84();
			}
			pNTV->ref_datum=MD.datum;
			pNTV->ref_zone=pREF->zone;
			if(pNTV->root_datum!=255 && root_zone!=pREF->zone) {
				//NOTE: pNTV->ref_zone and pNTV->root_zone will be needed to detect
				//when conversion of Lat/Longs and UTMs to out-of-zone UTMs is necessary!
				MD.zone=root_zone;
				int e=MF(MAG_CVTLL2UTM3); //get out-of-zone grid correction
				if(e!=MAG_ERR_OUTOFZONE) {
					CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_BADZONE2,pNode->Name(),pNode->Title());
					return -6;
				}
				pNTV->grid=MD.conv;
				CMainFrame::PutRef(pREF); //restore MD
			}
		}
	 }
	 else {
		if(pNTV->root_datum!=255) {
			CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_SRVNODATUM2,pNode->Name(),pNode->Title());
			return -6;
		}
		pNTV->ref_datum=255;
		pNTV->grid=0.0;
	 }

     int e=srv_Open(SurveyPath(pNode),pNTV,srv_CB);
     
     if(e) {
       //Should not normally happen since we have already confirmed file's existence --
	   CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_SRVOPEN,pNode->Name(),srv_ErrMsg(errBuf,e));
	   return -6;
     }
     
     //Make iSeg available to CB --
     srv_iSeg=iSeg;
     
     //Set the compile options, if any --
     if(e=SetOptions(pNode)) goto _error;

     //NOTE: SegRec is already initialized (and index positioned) for
     //accumulating statistics (if this were required) --
     
	 GetFldFromStr(pVec->filename,pNode->BaseName(),NTV_SIZNAME);
	 pVec->seg_id=SegRec.seg_id;
	 
	 while((e=srv_Next())==0) {
        
        bSegChg=TRUE;
        
	    pVec->fileno=(WORD)numfiles; //same as pVec->charno -- srv_Next() can change it!!!
		if(e=AppendNTV(pVec)) {
			if(e==SRV_ERR_LOGMSG) continue;
			break;
		}
		if(!(pVec->flag&NET_VEC_FIXED)) fTotalLength+=pNTV->len_ttl;    
		
		//Make sure that the From/To names aren't identical, which would only
		//add another condition to test for within WALLNET.DLL (which is
		//complicated enough). Let's not trust WALLSRV.DLL to trap it either --
				
		if(pVec->id[0]==pVec->id[1]) {
		  e=SRV_ERR_DUPNAMES;
		  break;
		}
	 }
	 
	 if(e==SRV_ERR_EOF && bSegChg) ixSEG.PutRec(&SegRec);
	 srv_Close();
	 
_error:
	 if(e!=SRV_ERR_EOF) {
	    
	    if(e!=SRV_ERR_DISPLAYED) {
			//An error was found in the text file at line number pVec->lineno --
			if(e!=SRV_ERR_MAGDLL) {
				LPSTR pMsg;
				CString cMsg;
				if(e==SRV_ERR_DUPNAMES) {
				  cMsg.LoadString(IDS_ERR_SRVDUP);
				  pMsg=(LPSTR)(const char *)cMsg;
				  pNTV->charno=0;
				}
				else pMsg=srv_ErrMsg(errBuf,e);
				CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_SRVTEXT,pNode->Name(),
				  pNTV->lineno,pMsg);
			}
			m_pErrorNode=pNode;
			m_nLineStart=(LINENO)pNTV->lineno;
			m_nCharStart=pNTV->charno;
		}
		return -7;
	 }
	 if(!SetSurvey(pNode->BaseName())) return -8;
   }
   else {
     int e;
     //Branch node processing --
     pNode=(CPrjListNode *)pNode->m_pFirstChild;
     while(pNode) {
       if(!pNode->m_bFloating && (e=CompileBranch(pNode,iSeg))) return e;
       pNode=(CPrjListNode *)pNode->m_pNextSibling;
     }
   }
   return 0;
}

char * CPrjDoc::GetNetNameOfOrigin(char *name)
{
    GetStrFromFld(name,pNTN->name,NET_SIZNAME);
    return name;
}

static int PASCAL WriteNTN(char *pathName)
{
    int e,i;
    CDBFile db;
    nettyp *pNet;

	if(e=db.Open(pathName,CDBFile::ReadWrite)) goto _error;
	
	pNet=(nettyp *)db.RecPtr();
	
	for(i=0;i<(int)numfiles;i++) {
	  if(e=db.AppendZero()) goto _errdel;
	  pNet->flag=NET_SURVEY_FLAG;
	  memcpy(pNet->name,pNET[i].name,NET_SIZNAME);
	  pNet->vectors=pNET[i].vectors;
	  pNet->length=pNET[i].length;
	  db.Mark();
	}
	if(e=db.Flush()) goto _errdel;
	return 0;
	
_errdel:
    db.CloseDel();
_error:
    CMsgBox(MB_ICONEXCLAMATION,"%s:  %s",db.Errstr(e),pathName);
    return -1;
}

static BOOL PASCAL CreateNTV(char *pathName)
{
    int e;

_retry:    
	e=dbNTV.Create(pathName);
	
	if(e) {
	  if(e==CNTVFile::ErrFind) {
	    char *p=trx_Stpnam(pathName)-1;
	    ASSERT(p>pathName);
	    *p=0;
	    //if(IDCANCEL==CMsgBox(MB_OKCANCEL,PRJ_ERR_NODIR,pathName)) return FALSE;
	    if(_mkdir(pathName)!=0) {
	    	CMsgBox(MB_ICONEXCLAMATION,PRJ_ERR_DIRWORK);
	    	return FALSE;
	    }
	    *p='\\';
	    goto _retry;
	  }
	}
	  
	if(e) {
      CMsgBox(MB_ICONEXCLAMATION,"%s:  %s",dbNTV.Errstr(e),pathName);
	  return FALSE;
	}
	
	return TRUE;
}

int PASCAL net_CB(int typ)
{
  char buf[128];
  static UINT sys;

  if(!pCV) return -1; //User aborted -- Causes NET_ErrCallBack to be returned
  
  switch(typ) {
    case NET_PostSortSys   :
		sprintf(buf,"Solving system %u..",sys=ND.sys_id);
		pCV->StatMsg(buf);
		Pause();
		break;

    case NET_PostSolveNet   :
		if(ND.sys_id&1) {
			ND.sys_id--;
			typ=1;
		}
		else typ=0;
		sprintf(buf,"System %u, %s traverse %u",sys,typ?"Vt":"Hz",ND.sys_id);
		pCV->StatMsg(buf);
		Pause();
		break;

    case NET_PostNetwork :
		pCV->NetMsg((pNET_WORK)&ND.vectors,FALSE);
		break;
  }
  
  return 0;
}

void CPrjDoc::InitLogPath(CPrjListNode *pNode)
{
	m_fplog=NULL;
	m_pLogDoc=this;
	m_pNodeLog=pNode;
	CMainFrame::CloseLineDoc(LogPath(), TRUE);
	_unlink(m_pPath);
}

int CPrjDoc::log_write(LPCSTR lpStr)
{
	if(!m_fplog) m_fplog=fopen(LogPath(), "w");
	return (!m_fplog || EOF==fputs(lpStr, m_fplog));
}

void CPrjDoc::ClearDefaultViews(CPrjListNode *pNode)
{
	CTRXFile xOSEG;     //Branch node's original NTA file (segment index)
	SEGHDR *pSH;

	ASSERT(pNode);
	if(m_pReviewNode==pNode) {
		pSH=pixSEGHDR;
	}
	else {
		if(xOSEG.Open(WorkPath(pNode, TYP_NTA), CTRXFile::ReadWrite))
			return;
		pSH=(SEGHDR *)xOSEG.ExtraPtr();
	}

	for(int i=0; i<=1; i++) {
		pSH->vf[i].iComponent=pSH->vf[i].bActive=0;
	}

	if(xOSEG.Opened()) {
		xOSEG.MarkExtra();
		xOSEG.Close();
	}
}

int CPrjDoc::Compile(CPrjListNode *pNode)
{ 
    //Return values:  0=Success, -1=Cancel, -2=Edit properties, -3 edit SRV 

	if(pNode->NameEmpty())
       return CMsgBox(MB_OKCANCEL|MB_ICONEXCLAMATION,PRJ_ERR_NOWRK1,pNode->Title())==IDOK?-2:-1;
       
    //Recompute checksums in case directory contents were somehow changed
    //outside of normal project operations. We have already insured
    //that all survey files opened as CLineDoc's are in a saved state.
	//We are assuming *workfile* checksums are being correctly maintained.
    {
	    DWORD dwChk=pNode->m_dwFileChk;

		//if(pNode->m_dwFileChk!=0x96ecf452) ASSERT(FALSE);

	    maxfiles=AttachedFileChk(pNode);

		//if(maxfiles!=0x31) ASSERT(FALSE);

		if(pNode->m_dwFileChk!=dwChk) {
		  pNode->m_dwFileChk ^= dwChk;
		  pNode->FixParentChecksums(); //removes the old and adds the new
		  pNode->m_dwFileChk ^= dwChk;
		}
	}
	  
	if(!pNode->m_dwFileChk) {
	  //The branch is now empty of files --
	  CMsgBox(MB_ICONEXCLAMATION,PRJ_ERR_ZEROCHK);
	  return -1;
	}
	
	if(pNode->m_dwFileChk==pNode->m_dwWorkChk) {
	  //Review existing data, do not compile --
      Review(pNode);
	  return -1;
	}

	//Close any workfiles already open. Any failure beyond this point
	//will require closing the CReView/CCompView dialogs with pCV->Close() --

	if(m_pMapFrame) {
	    //Remove from list frames displaying data for pNode

		CMapFrame *pf;
		while(pf=FrameNodeExists(pNode)) {
			pf->RemoveThis();
			pf->m_pDoc=NULL;
		}

		UpdateAllViews(0, LHINT_CLOSEMAP,(CObject *)pNode); //Close map frames displaying data for m_pReviewNode
		#ifdef _DEBUG
		for(pf=m_pMapFrame; pf; pf=pf->m_pNext) {
			if(pf->m_pDoc!=this || pf->m_pFrameNode==pNode) {
				ASSERT(0);
			}
		}
		#endif
	}

	ASSERT(!m_pMapFrame || m_pMapFrame->m_pFrameNode!=pNode);

	if(InitCompView(pNode)) //Initializes m_pReviewNode
		return -1;

	pPV->Enable(IDC_UPDATEFRAME,FrameNodeExists(pNode)!=NULL);

	pCV->ShowAbort(TRUE);

	//We will overwrite any existing workfile, so we start with an
    //empty checksum --
    
  	pNode->m_dwWorkChk=0;

    //Stack variables whose addresses will be passed for public access --
    CTRXFile xNAM;		//Name index file (temporary)	
    CTRXFile xOSEG;     //Branch node's original NTA file (segment index)
	NTVRec ntv;   		//Database record being assembled
	
	BYTE cache[NTV_SIZNAME];	//Holds previously established name
	
    //Use a class to simply initialize pointer pNET --	 
	CNETData net(maxfiles);
	
	if(!pNET || !CreateNTV(WorkPath(pNode,TYP_NTV))) {
	  //Display error message --*/
	  pCV->Close();
	  if(!pNET) CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_MEMORY);
	  return -1;
	}

	//Open index files --	
	int e;
	pixOSEG=NULL;
	OSEGwFlagsMax=0;

	//Create a temporary index file for checking station names, and for computing
	//a sequential 4-byte ID number for each unique name --
	//Non-flushed mode is the default --
	if(!(e=xNAM.Create(WorkPath(pNode,TYP_NAM),sizeof(long)))) { 
      xNAM.SetUnique(); //We will only insert unique names
	  pixNAM=&xNAM;
	  //Assign cache and make it the default for indexes --
	  if(e=xNAM.AllocCache(SIZ_INDEX_CACHE,TRUE)) xNAM.CloseDel();
	}
	
	//Open segment indexes as required --
	if(!e) {
	   //First, open old NTA for retrieving segments and attributes --
	   pixOSEG=&xOSEG;
	   if(e=OpenSegment(pixOSEG,pNode,CTRXFile::ReadOnly)) {
	     if(e==CTRXFile::ErrFind) e=0;
	     pixOSEG=NULL;
	   }
	   else xOSEG.SetExact();

	   //Create(pathname,size_rec,mode,numtrees,size_extra,...)
	   //We are using tree 0 for segments and tree 1 for prefixes.
	   if(!e) e=ixSEG.Create(WorkPath(pNode,pixOSEG?TYP_TMP:TYP_NTA),
	      sizeof(styletyp),0,NTA_NUMTREES,sizeof(SEGHDR));

	   if(!e) {
	     //Set bkgnd color, grid options, etc., stored in index file header --
	     //ASSERT(ixSEG.SizExtra()==sizeof(SEGHDR));
	     pixSEGHDR=(SEGHDR *)ixSEG.ExtraPtr();

	     //We preserve SEGHDR styles only for completely compatible versions
		 //OR if this version is 8 bytes smaller --
		 if(pixOSEG && (*(WORD *)pixOSEG->ExtraPtr()==NTA_VERSION || pixOSEG->SizExtra()==sizeof(SEGHDR_7P))) {
			if(pixOSEG->SizExtra()==sizeof(SEGHDR_7P)) {
				//Transfer all but HDR_OUTLINES and initialize the latter --
				CSegView::CopySegHdr_7P(pixSEGHDR,(SEGHDR_7P *)pixOSEG->ExtraPtr());
			}
			else {
				memcpy(pixSEGHDR,pixOSEG->ExtraPtr(),sizeof(SEGHDR));
				pixSEGHDR->style[HDR_FLOOR].SetSVGAvail(FALSE);
			}

			//When recompiling, inactivate saved views for any network component
			//but the first --
			for(int i=0;i<=1;i++) {
				if(pixSEGHDR->vf[i].iComponent>1) {
					pixSEGHDR->vf[i].iComponent=0;
					pixSEGHDR->vf[i].iComponent=pixSEGHDR->vf[i].bActive=FALSE;
				}
			}

			if((OSEGwFlagsMax=pixSEGHDR->numFlagNames) &&
				(pOSEGFlagPrior=(WORD *)calloc(2*OSEGwFlagsMax,sizeof(WORD)))) {

				pOSEGFlagPriorPos=pOSEGFlagPrior+OSEGwFlagsMax;
				OSEGwNewFlags=OSEGwOldFlags=0;
			}
			else OSEGwFlagsMax=0;
		 }
	     else CSegView::InitSegHdr(pixSEGHDR);
	     
	     pixSEGHDR->numFlagNames=0;
	     
	     //Header already "marked"
	     ixSEG.SetUnique();
	     
	     //Initialize #PREFIX tree --
	     ixSEG.UseTree(NTA_PREFIXTREE);  //Tree 1
 		 //Max 128-char length for prefixes
 		 ixSEG.InitTree(sizeof(WORD),SRV_LEN_PREFIX,CTRXFile::KeyUnique);
 		 ixSEG.SetExact(); //We will use "insert if not found"
 		 
 		 //Initialize #NOTE and #FLAG 
 		 ixSEG.UseTree(NTA_NOTETREE);  //Tree 2
 		 ixSEG.InitTree(sizeof(NTA_NOTETREE_REC),TRX_MAX_KEYLEN,CTRXFile::KeyUnique);
 		 
 		 //Initialize LRUD tree -- 
 		 ixSEG.UseTree(NTA_LRUDTREE);  //Tree 3
 		 ixSEG.InitTree(sizeof(NTA_LRUDTREE_REC),TRX_MAX_KEYLEN,CTRXFile::KeyExact);

 		 ixSEG.UseTree(NTA_SEGTREE);  //Tree 0
	   }
	   else xNAM.CloseDel();
	}
	
	if(e) {
	  if(pixOSEG) xOSEG.Close(FALSE);
	  dbNTV.DeleteCache();
	  dbNTV.CloseDel();  
	  pCV->Close();
      if(e>0) CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_TRXBUILD,xNAM.Errstr(e));
	  return -1;
	}
    
    //Initialize additional pointers for use by static
    //recursive fcn, CompileBranch() --
    pAddNames=m_bCheckNames?CheckNames:AddNames;
	bLogNames=false;
    pBranch=pNode;
    memset(pNTV=&ntv,0,sizeof(ntv));
    wCache=NULL_ID;
    memset(pCache=cache,0,sizeof(cache));
    
    numfiles=numvectors=numnames=numvectors0=0;
    fTotalLength=fTotalLength0=0.0;
	m_pErrorNode=NULL;
    //m_pErrorNode=m_pErrorNode2=NULL;                           
    
    ASSERT(pCV);

	pNTV->root_datum=255;
	pNTV->root_zone=0; //should be unnec
	pREF=NULL;

	if(pNode->InheritedState(FLG_GRIDMASK)) {
		if(pREF=pNode->GetAssignedRef()) {
			pNTV->root_datum=pREF->datum;
			pNTV->root_zone=root_zone=pREF->zone;
		}
		else {
			//Turn off grid_relative (correction for bug in B6 version) --
			pNode->m_uFlags&=~FLG_GRIDCHK;
			pNode->m_uFlags|=FLG_GRIDUNCHK;
		}
	}

	//pNTV->root_datum=255 indicates to wallsrv.dll that UTM is NOT being generated.
	//The individual files may have their own references for decl generation
	
	InitLogPath(m_pReviewNode);

	e=CompileBranch(pNode,0); //This is the biggie!

	if(m_fplog) fclose(m_fplog);

	ASSERT_PLNODE(pNode);
	
	xNAM.CloseDel(FALSE); 	//Does not delete cache
	
	//Close segment indexes --
	if(pixOSEG) {
		xOSEG.Close(FALSE);
		//Adjust priorities of preexisting flag names --
		if(OSEGwFlagsMax) {
			if(OSEGwOldFlags && !e) UpdateFlagPriorities();
			free(pOSEGFlagPrior);
		}
	}

	if(!e) {
	  char *pathSEG=strcpy(SegBuf,WorkPath(pNode,TYP_NTA));
	  char *pathTMP=pixOSEG?WorkPath(pNode,TYP_TMP):pathSEG;

	  e=ixSEG.Close(TRUE); //Frees default cache
	  if(!e) {
	   if(pixOSEG) {
	      _unlink(pathSEG);
	      rename(pathTMP,pathSEG);
	   }
	  }
	  else {
	    _unlink(pathTMP);
	    CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_NTVBUILD,ixSEG.Errstr(e));
	    e=-1;
	  }
	}
	else ixSEG.CloseDel(TRUE); //Frees cache
	
	if(!e && !numvectors) {
	  CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_NTVEMPTY);
	  e=-1; 
	}
    
    ASSERT_PLNODE(pNode);
    
	if(!e) {
		pCV->SetTotals(numvectors,fTotalLength);
		
		//Store values in NTV file header --
		dbNTV.SetMaxID(numnames);
		dbNTV.SetChecksum(pNode->m_dwFileChk);
		dbNTV.SetVersion(NTV_VERSION);
		dbNTV.MarkHdr();
			
		//This doesn't free the cache --
		if(e=dbNTV.Close()) CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_NTVBUILD,dbNTV.Errstr(e));
		else {
					  
			//Call WALLNET4.DLL to perform adjustment.
			LoadCMsg(errBuf,NET_SIZ_ERRBUF,IDS_COMP_GEOMETRY);
			pCV->StatMsg(errBuf);
			Pause();
			
			if(pCV) e=net_Adjust(WorkPath(pNode,TYP_NTV),&ND,net_CB);
			else e=NET_ErrCallBack;
			
			ASSERT_PLNODE(pNode);
					  
			if(e) {
				if(e!=NET_ErrCallBack) {
				  net_ErrMsg(errBuf,e);
				  CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_NTVADJUST,errBuf);
				}
			}
			else {
				if(!m_fplog) ::MessageBeep(MB_ICONASTERISK);
				//Write a review file --
				e=WriteNTN(WorkPath(pNode,TYP_NTN));
			}
		}
	}
	
    if(e) {
		//Assume user was informed of the error --
		if(dbNTV.Opened()) {
			dbNTV.CloseDel();
			if(pCV) pCV->Close();
		}
		else {
			//Close pCV and delete NTV, and NTS, NTP, and NTN created by WALLNET4.
			//Do NOT delete NTA file.
			ASSERT_PLNODE(pNode);
			PurgeWorkFiles(pNode,FALSE);
		}	    
		//If m_pErrorNode!=0, open view for text file where error occurred --
		return m_pErrorNode?-3:-1;
	}
	
	ASSERT(!dbNTV.Opened());
    
	if(!OpenWorkFiles(pNode)) return -1; //Also closes pCV if error
	
    //We now have a valid NTV database --
    pNode->m_dwWorkChk=pNode->m_dwFileChk;

	if(pCV) pCV->PostMessage(WM_COMMAND,ID_COMPLETED);
	
	if(m_fplog) {
		if(IDYES==CMsgBox(MB_YESNO,IDS_PRJ_ERRORLOG)) {
			LaunchFile(m_pReviewNode,CPrjDoc::TYP_LOG);
		}
	}
	return 0;
}

static void get_uvestr5(char *buf,double uve)
{
  if(uve>99.99) strcpy(buf,"--.--");
  else sprintf(buf,"%5.2f",uve);
}

static void get_uveline(char *buf,UINT n,double *pss,WORD *pni,UINT nc)
{
	if(n) buf+=sprintf(buf,"%-5u",n);
	for(n=0;n<2;n++,pss++,pni++) {
		*buf++=' ';
		UINT df=nc-*pni;
		if(df) get_uvestr5(buf,*pss/((2-n)*df));
		else strcpy(buf,"<FLT>");
		buf+=5;
	}
}

void CPrjDoc::CloseWorkFiles()
{
    if(nLen_piSYS) {
      free(piPLTSEG);
      piPLTSEG=NULL;
	  free(piLNK);
      free(piSYS);
	  piSYS=piLNK=NULL;
	  nLen_piLNK=nLen_piSYS=nLen_piPLTSEG=0;
    }
    if(ixSEG.Opened()) {
      ixSEG.DeleteCache();
      ixSEG.Close();
	  free(CSegView::pPrefixRecs);
	  CSegView::pPrefixRecs=NULL;
    }
	if(dbNTP.Opened()) dbNTP.Close();
	if(dbNTV.Opened()) dbNTV.Close();
    if(dbNTS.Opened()) dbNTS.Close();
	if(dbNTN.Opened()) dbNTN.Close();
	ND.ntv_no=ND.nts_no=ND.ntp_no=ND.ntn_no=0;
	CDBFile::FreeDefaultCache();
}

BOOL CPrjDoc::OpenWorkFiles(CPrjListNode *pNode)
{
    if(
       !OpenWork((PPVOID)&pNTN,&dbNTN,pNode,TYP_NTN) ||
       !OpenWork((PPVOID)&pSTR,&dbNTS,pNode,TYP_NTS) ||
       !OpenWork((PPVOID)&pVEC,&dbNTV,pNode,TYP_NTV) ||
       !OpenWork((PPVOID)&pPLT,&dbNTP,pNode,TYP_NTP) ||
       OpenSegment(&ixSEG,pNode,CTRXFile::ReadWrite))
    {
      pCV->Close(); //Should CloseWorkFiles(), etc.
      return FALSE;
    }

	ND.ntv_no=dbNTV.GetDBF_NO();
	ND.nts_no=dbNTS.GetDBF_NO();
	ND.ntp_no=dbNTP.GetDBF_NO();
	ND.ntn_no=dbNTN.GetDBF_NO();

	CSegView::GetPrefixRecs();
    return TRUE;
}

void CPrjDoc::Review(CPrjListNode *pNode)
{
    int e;
    netptr pNet;
	NET_WORK nw;
    
    if(pCV && pNode==m_pReviewNode && m_pReviewDoc==this) {
	  pCV->BringToTop();
	  return;
    }
	
    m_bReviewing=TRUE;
	BeginWaitCursor();
    if(InitCompView(pNode) || !OpenWorkFiles(pNode)) {
      m_bReviewing=FALSE;
	  EndWaitCursor();
	  return;
	}
	
	pNet=(netptr)dbNTN.RecPtr();
	numvectors=0;
	fTotalLength=0.0;
	pCV->m_iBaseNameCount=0;
	e=dbNTN.First();
	while(!e) {
	   ASSERT(pCV);
	   e=(pNet->flag==NET_SURVEY_FLAG);
	   nw.vectors=pNet->vectors;
	   nw.length=pNet->length;
	   memcpy(nw.name,pNet->name,NET_SIZNAME);
	   pCV->NetMsg(&nw,e);
	   if(!e) {
		   numvectors+=nw.vectors;
		   fTotalLength+=nw.length;
	   }
	   e=dbNTN.Next();
	}
	pCV->SetTotals(numvectors,fTotalLength);
	pCV->SendMessage(WM_COMMAND,ID_COMPLETED);
	m_bReviewing=FALSE;
	EndWaitCursor();

	pPV->Enable(IDC_UPDATEFRAME, FrameNodeExists(pNode)!=NULL);
}

//#define XSMALL 0.0000000000001 (fails for house.prj)
//DBL_EPSILON == 2.2204460492503131e-016 (smallest value such that 1.0+DBL_EPSILON != 1.0)

#define XSMALL DBL_EPSILON
#define TRAV_PRECISION 6
#define COMP_PRECISION 5

static apfcn_i get_fuvestr(char *buf,int i,int prec,int linkID)
{
	double inc_ss,sys_ss;
	char *p=NULL;
	char idbuf[20];
  
	if(bStrFloating[i]) p="<FLOAT>  ";
	else if(bStrBridge[i]) p="<BRIDGE> ";

	if(p) {
		if(linkID /*&& bStrBridge[i]!=2*/) {
		  p=strcpy(idbuf,p);
		  _snprintf(p+3,7,"%c-%03d>",p[5],linkID);
		  if(bStrLowerc[i]) strlwr(p);
		}
		goto _fixbuf;
    }

	if(bStrFixed[i]) {
		p="<FIXED>  ";
		goto _fixbuf;
	}
	
	//Compute inc_ss = (Se/n)/((SS-Se)/(n*(nc-1))) = F statistic
	//Compute sys_ss = (SS-Se)/(n*(nc-1)) = UVE after deletion
    sys_ss=stSTR.sys_ss[i];
    inc_ss=stSTR.inc_ss[i];
	if(stSTR.sys_nc[i]>1 && sys_ss>inc_ss) {
		sys_ss=(sys_ss-inc_ss)/(stSTR.sys_nc[i]-1);
		inc_ss/=sys_ss;
		if(!i) sys_ss/=2.0;
	}
	else {
		//Our network consists of a single isolated loop, in which case inc_ss==sys_ss.
		//Let's print the loop's UVE in place of the non-applicable F-statistic --
		//ASSERT(inc_ss==sys_ss);
		sys_ss=0.0;
		if(!i) inc_ss/=2.0;
	}
		
	if(prec==COMP_PRECISION) {
	  if(inc_ss>99.9) strcpy(buf,"--.-/");
	  else sprintf(buf,"%4.1f/",inc_ss);
	}
	else {
	  if(inc_ss>999.9) strcpy(buf,"---.-/");
	  else sprintf(buf,"%5.1f/",inc_ss);
	}
			  
	get_uvestr5(buf+prec,sys_ss);
	return prec+5;
	
_fixbuf:
	if(prec==COMP_PRECISION) {
	  *buf++=' ';
	  i=10;
	}
	else i=8;
	strcpy(buf,p);
	return i;
}
	
static apfcn_i GetStats(int i)
{
	double vdif;
	  
	if(!GoWorkFile(&dbNTS,i)) return 0;
	
	bStrFloating[0]=(pSTR_flag&NET_STR_FLTMSKH)!=0;
	bStrFloating[1]=(pSTR_flag&NET_STR_FLTMSKV)!=0;
	ASSERT(!bStrFloating[0] || !(pSTR_flag&NET_STR_BRIDGEH));
	ASSERT(!bStrFloating[1] || !(pSTR_flag&NET_STR_BRIDGEV));

	bStrBridge[0]=!bStrFloating[0] && (pSTR_flag&NET_STR_BRIDGEH)!=0;
	bStrBridge[1]=!bStrFloating[1] && (pSTR_flag&NET_STR_BRIDGEV)!=0;

	bStrLowerc[0]=(pSTR_flag&NET_STR_BRGFLTH)==NET_STR_BRGFLTH ||
				  (bStrFloating[0] && (pSTR_flag&NET_STR_CHNFLTXH)!=0);

	bStrLowerc[1]=(pSTR_flag&NET_STR_BRGFLTV)==NET_STR_BRGFLTV ||
				  (bStrFloating[1] && (pSTR_flag&NET_STR_CHNFLTXV)!=0);
	bStrFixed[0]=bStrFixed[1]=false;

	//Compute adjustment vector --
	if(pSTR->sys_nc==1) for(i=0;i<3;i++) stSTR.cxyz[i]=-pSTR->xyz[i];
	else for(i=0;i<3;i++) stSTR.cxyz[i]=pSTR->e_xyz[i]-pSTR->xyz[i];
	
	for(i=0;i<2;i++) {
	
		stSTR.sys_nc[i]=pSTR->sys_nc-pSTR->sys_ni[i];
		                 
		if(pSTR->sys_nc==1) {
		    //For isolated loops, our F-statistic will be based on a
		    //comparison of the loop against all other isolated loops.
		    //For a 1-loop system, SS=Se=(X1-X2)(X1-X2)/(V1+V2) --
		    
			stSTR.sys_nc[i]=nISOL-nISOL_ni[i];    
			stSTR.sys_ss[i]=ssISOL[i];                 
		    if(!bStrFloating[i]) stSTR.inc_ss[i]=pSTR->sys_ss[i];
			else stSTR.inc_ss[i]=0.0;
		}
		else {
			stSTR.sys_ss[i]=pSTR->sys_ss[i];
		    if(!bStrFloating[i]) {
		        if(pSTR->hv[i]<=0.0) {
				  bStrFixed[i]=true;
				  vdif=0.0;
				}
				else if(bStrBridge[i]) vdif=0.0;
				else vdif=pSTR->hv[i]-pSTR->e_hv[i];
				
			    if(vdif>XSMALL) {
			      if(i) stSTR.inc_ss[1]=(stSTR.cxyz[2]*stSTR.cxyz[2])/vdif;
	  		      else stSTR.inc_ss[0]=
	  		        (stSTR.cxyz[0]*stSTR.cxyz[0]+stSTR.cxyz[1]*stSTR.cxyz[1])/vdif;
				  vdif=(vdif+pSTR->e_hv[i])/vdif;
				}
				else {
				  if(!bStrBridge[i]) vdif=1.0;
				  stSTR.inc_ss[i]=0.0;
				}
				//Best correction --
				//Note: vdif = V/(V-v)
				if(i) stSTR.cxyz[2]*=vdif;
				else { 
				  stSTR.cxyz[0]*=vdif; 
				  stSTR.cxyz[1]*=vdif;
				} 
			}
			else stSTR.inc_ss[i]=0.0;
		}
	}
    return 1;
}

static __inline int getLinkID(int linkID)
{
	while(linkID>0) linkID=piFRG[linkID-1];
	return -linkID;
}

static apfcn_v SetSegment(int linkID)
{
	char buf[80];

    sprintf(buf,"%-5u",pSTR->vec_cnt);
	    
	int rec=4+get_fuvestr(buf+4,0,COMP_PRECISION,linkID);
	buf[rec++]=' ';
	get_fuvestr(buf+rec,1,COMP_PRECISION,linkID);
    pCV->AddString(buf,IDC_SEGMENTS);
}
 
void CPrjDoc::SetSystem(int iSys)
{
  ASSERT(piSYS);
  
  if(iSys<0) {
    ASSERT(nISOL);
    iSys=nSYS;
  }

  nSYS_Offset=iSys?piSYS[iSys-1]:0;
  nSYS_LinkOffset=-1;

  iSys=piSYS[iSys];

  for(int s=nSYS_Offset;s<iSys;s++) {
	if(!GetStats(piSTR[s])) break;
    int linkID=0;
	if(nLNKS && piFRG[s]) {
		VERIFY(linkID=getLinkID(piFRG[s]));
		if(nSYS_LinkOffset<0) nSYS_LinkOffset=linkID-1;
		linkID-=nSYS_LinkOffset;
	}
    SetSegment(linkID);
  }
  
  //Display loop counts for system --
  pCV->SetLoopCount(stSTR.sys_nc[0],stSTR.sys_nc[1]);
}

static double NEAR PASCAL atn2(double a,double b)
{
  if(a==0.0 && b==0.0) return 0.0;
  return atan2(a,b)*(180.0/3.141592653589793);
}

typedef struct {
	double di;
	double az;
	double va;
} sphtyp;

static apfcn_v get_sphtyp(sphtyp *sp,double *pd)
{
  double d2;

  sp->az=atn2(pd[0],pd[1]);
  if(sp->az<0.0) sp->az+=360.0;
  d2=pd[0]*pd[0]+pd[1]*pd[1];
  sp->va=atn2(pd[2],sqrt(d2));
  sp->di=sqrt(d2+pd[2]*pd[2]);
}

BOOL CPrjDoc::GetVecData(vecdata *pvecd,int rec)
{
  int sign=rec<0?-1:1;
  
  if(!GoWorkFile(&dbNTV,sign*rec)) {
    pvecd->pvec=NULL;
    return FALSE;
  }
  pvecd->pvec=pVEC;
  pvecd->sp_index=-1;

  if(pVEC->flag&NET_VEC_FIXED) {
	  memset(pvecd->sp,0,3*sizeof(double));
      for(rec=0;rec<3;rec++) {
		  if(fabs(pvecd->e_sp[rec]=sign*stSTR.cxyz[rec])>m_LnLimit) {
			  if(pvecd->sp_index==-1) pvecd->sp_index=rec;
			  else pvecd->sp_index=-2;
		  }
	  }
  }
  else {
	  double xyz[3];

	  //Obtain the original vector --
	  for(rec=0;rec<3;rec++) xyz[rec]=pVEC->xyz[rec];
  
	  //Store it for display in spherical coordinates (Dst,Az,Inc) -- 
	  get_sphtyp((sphtyp *)&pvecd->sp,xyz);
  
	  //Now add (or subtract) the best ENU correction --
	  for(rec=0;rec<3;rec++) xyz[rec]+=sign*stSTR.cxyz[rec];
  
	  //Convert the proposed corrected vector to spherical coordinates,
	  //then store the difference (correction) with azimuth normalized -- 
	  get_sphtyp((sphtyp *)&pvecd->e_sp,xyz);
	  for(rec=0;rec<3;rec++) pvecd->e_sp[rec]-=pvecd->sp[rec];
	  if(pvecd->e_sp[1]>180.0) pvecd->e_sp[1]-=360.0;
	  else if(pvecd->e_sp[1]<-180.0) pvecd->e_sp[1]+=360.0;
  
	  //memcpy(xyz,pvecd->e_sp,sizeof(xyz)); ???
  
	  //If exactly one of the spherical coordinate measurements is outside of its
	  //accepted range, set sp_index to its index so it can be highlighted when displayed --
	  if(fabs(pvecd->e_sp[1])>m_AzLimit) pvecd->sp_index=1;
	  if(fabs(pvecd->e_sp[2])>m_VtLimit) {
		  if(pvecd->sp_index==-1) pvecd->sp_index=2;
		  else pvecd->sp_index=-2;
	  }
	  if(pvecd->sp_index>-2 && fabs(pvecd->e_sp[0])>m_LnLimit) {
		  if(pvecd->sp_index==-1) pvecd->sp_index=0;
		  else pvecd->sp_index=-2;
	  }
  }
  return TRUE;
}

void CPrjDoc::LoadVecFile(int nRec)
{
  char nambuf[NTV_SIZNAME+1];
  
  if(nRec<0) nRec=-nRec;
  if(GoWorkFile(&dbNTV,nRec)) {
    GetStrFromFld(nambuf,pVEC->filename,NTV_SIZNAME);
    ASSERT(m_pReviewNode && m_pReviewDoc);
	CPrjListNode *pNode=CPrjDoc::m_pReviewNode->FindName(nambuf);
    if(pNode) {
	    CLineView::m_nLineStart=(LINENO)pVEC->lineno;
	    CLineView::m_nCharStart=0;
        CPrjDoc::m_pReviewDoc->OpenSurveyFile(pNode);
    }
  }
}

int CPrjDoc::NTS_id(int iSys,int iTrav)
{
  ASSERT(piSYS);
  if(iSys<0) {
    ASSERT(nISOL);
    iSys=nSYS;
  }
  if(iSys) iTrav+=piSYS[iSys-1]; //iTrav=index in piSTR of traverse
  return iTrav;
}

int CPrjDoc::GetStrFlag(int iSys,int iTrav)
{
	  
  if(dbNTS.Go(piSTR[NTS_id(iSys,iTrav)])) {
    ASSERT(FALSE);
    return -1;
  }
  return pSTR_flag;
}

int CPrjDoc::TravToggle(int iSys,int iTrav)
{
  int i=GetStrFlag(iSys,iTrav);
  if(i<0) return -1;	  
  ASSERT(i==pCV->m_iStrFlag);
  dbNTS.Mark();
  pSTR_flag^=NET_STR_REVERSE;
  return i^=NET_STR_REVERSE;
}

int CPrjDoc::FloatStr(int iSys,int iTrav,int i)
{
	//Return e=0  - Success
	//		 e=1  - Do nothing
	//       e=-1 - Fatal error (Abort review)
	int e;
	
	iTrav=piSTR[NTS_id(iSys,iTrav)];

	if(dbNTS.Go(iTrav)) {
		ASSERT(FALSE);
		return 1;
	}
	
	LoadCMsg(errBuf,NET_SIZ_ERRBUF,iSys<0?IDS_COMP_STATISTICS:IDS_COMP_SYSTEM,pSTR->sys_id);
	pCV->StatMsg(errBuf);
	
	BeginWaitCursor();
	ND.sys_id=iTrav;
	ND.net_id=i;

	ASSERT(ND.ntn_no && ND.ntp_no && ND.nts_no && ND.ntv_no);

	e=net_Update(&ND);
	EndWaitCursor();
	
	if(e) {
		if(e==NET_ErrDisconnect) {
		  //This is a normal occurence. Too many "floating" vectors --
		  CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_BADFLOAT);
		  e=1;
		}
		else {
		  net_ErrMsg(errBuf,e);
		  CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_NTVADJUST,errBuf);
		  e=-1;
		}
	}

    return e;
}

BOOL CPrjDoc::SetTraverse(int iSys,int iTrav)
{
  char buf[20];
  
  int id,ntsRec,ntvRec;
  double hzLen,vtLen;
  
  CListBox *plb=pTV->GetTravLB();
  
  ntsRec=piSTR[id=NTS_id(iSys,iTrav)];

  if(ntsRec==pTV->m_recNTS) {
	  if(ntsRec==iFindStr) {
		  plb->SetCurSel(iFindSel);
		  iFindStr=0;
	  }
	  return TRUE;
  }
	  
  if(!GetStats(ntsRec)) return FALSE;
  
  plb->SetRedraw(FALSE);
  
  if(pTV->m_recNTS) plb->ResetContent(); //Clears traverse linstbox
  pTV->m_recNTS=ntsRec;

  if(nLNKS && (id=piFRG[id])) {
		VERIFY(id=getLinkID(id));
		ASSERT(nSYS_LinkOffset>=0);
		id-=nSYS_LinkOffset;
  }
  else id=0;
    
  get_fuvestr(buf,0,TRAV_PRECISION,id);
  pTV->SetF_UVE(buf,IDC_F_UVEH);
  
  get_fuvestr(buf,1,TRAV_PRECISION,id);
  pTV->SetF_UVE(buf,IDC_F_UVEV);
  
  //Fill the owner-draw listbox with NTV record numbers as data
  //while computing horizontal and vertical total lengths --
  
  hzLen=vtLen=0.0;
  ntvRec=pSTR->vec_id;
  int sign;

  id=pSTR->id[0];
  
  for(int i=pSTR->vec_cnt;i;i--) {
    ASSERT(ntvRec && ntvRec<=INT_MAX);
    if(!GoWorkFile(&dbNTV,ntvRec)) break;
    ASSERT(pVEC->str_id==pTV->m_recNTS);
    
    if(!(pVEC->flag&NET_VEC_FIXED)) {
		hzLen+=sqrt(pVEC->xyz[0]*pVEC->xyz[0]+pVEC->xyz[1]*pVEC->xyz[1]);
		vtLen+=fabs(pVEC->xyz[2]);
    }

    //Let sign of item data indicate direction of vector within string.
    //If positive, id[0] (FROM) is on the string's id[0] side, which
    //means the vector was added rather than subtracted to obtain the
    //string displacement --
    
    sign=id==pVEC->id[0];
    ASSERT(sign || id==pVEC->id[1]);
    id=pVEC->id[sign];
    if(!sign) sign--;
    
    //Only INT_MAX records are supported -- 
    plb->AddString((LPCSTR)(DWORD)(sign*ntvRec));
    
    ntvRec=pVEC->str_nxt_vc;
  }
  
  ASSERT(!ntvRec && id==pSTR->id[1]);
  
  if(pTV->m_recNTS==iFindStr) {
	  plb->SetCurSel(iFindSel);
	  iFindStr=0;
  }

  plb->SetRedraw(TRUE);
  plb->Invalidate();
  
  //Single vector strings will have their best correction oriented with 
  //vector's from/to instead of the string sequence --
  if(pSTR->vec_cnt>1) sign=1;
  
  pTV->SetTotals(hzLen,vtLen,stSTR.cxyz,sign);
  pTV->EnableButtons(iTrav);
  
  return TRUE;
}

int CPrjDoc::GetNetFileno()
{
	//Static function to return fileid of of current network component base station.
	//Called by CCompView::OnNetChg() --
	return dbNTV.Go(pNTN->vec_id)?-1:pVEC->fileno;
}

LINENO CPrjDoc::GetNetLineno()
{
	//Static function to return fileid of of current network component base station.
	//Called by CCompView::OnNetChg() --
	return dbNTV.Go(pNTN->vec_id)?0:(LINENO)pVEC->lineno;
}

//Added 11/4/04 to support link fragments --
static void AllocLinkFragment(int str_id,int lnk_nxt)
{
	if(nLen_piLNK<nFRGS+1) piLNK=(int *)realloc(piLNK,(nLen_piLNK+=LNK_INCREMENT)*sizeof(*piLNK));
	if(piLNK) {
		piLNK[nFRGS++]=str_id;
		piFRG[str_id]=lnk_nxt;
	}
	else {
		nFRGS=nLen_piLNK=0;
	}
}

static void InitLinkFragments()
{
	//Replace piFRG[] negative record numbers with piFRG[] indices plus 1
	//to form a circularly linked set of link fragments for each link.
	//Each sequence will contain a single negative piFRG[] = -n, where
	//n is the link ID (1..nLNKS).  piLNK[n-1] is the piFRG[] index of
	//the first link fragment, piFRG[piLNK[n-1]]-1 is the index of the
	//next fragment, etc., until piFRG[]<0.
	int ln,iFrg,iRec,lNext=1;
	ASSERT(nLNKS==0 && nFRGS>1);
	while(lNext<nFRGS) {
		iFrg=piLNK[nLNKS];
		iRec=piSTR[iFrg];
		while(piFRG[iFrg]<0) {
			for(ln=lNext;ln<nFRGS;ln++) {
				if(piSTR[piLNK[ln]]==-piFRG[iFrg]) break;
            }
			ASSERT(ln<nFRGS);
			if(ln>=nFRGS) goto _errfmt;
			piFRG[iFrg]=piLNK[ln]+1;
			iFrg=piLNK[ln];
			if(piFRG[iFrg]==-iRec) {
				//Cycle closed -- last fragment
				piFRG[iFrg]=piLNK[nLNKS]+1;
			}
		}
		++nLNKS; //Link processed
		//Advance lNext to next unprocessed fragment in piLNK,
		ASSERT(lNext>=nLNKS);
		while(lNext<nFRGS && piFRG[piLNK[lNext]]>=0) lNext++;
		if(lNext<nFRGS) {
			if(lNext>nLNKS) {
				//then set piLNK[nLNKS] to point to first fragment of next link --
				piLNK[nLNKS] = piLNK[lNext];
				ASSERT(lNext+1<nFRGS);
			}
			lNext++;
		}
	}

	//Next, sort piLNK[] by increasing value, which is the same as the order
	//of appearance in the IDC_SEGMENTS (traverses) list box.
	//NOTE: This list is too short (<100) to justify a fancier algorithm.
	for(ln=0;ln<nLNKS-1;ln++) {
		iRec=ln;
		for(lNext=ln+1;lNext<nLNKS;lNext++) if(piLNK[lNext]<piLNK[iRec]) iRec=lNext;
		if(iRec!=ln) {
			iFrg=piLNK[ln];
			piLNK[ln]=piLNK[iRec];
			piLNK[iRec]=iFrg;
		}
	}

	//Finally, prepare circularly-linked fragments to include the negative link ID --
	for(ln=0;ln<nLNKS;ln++) {
		iRec=iFrg=piLNK[ln];
		do {
			lNext=iRec;
			iRec=piFRG[iRec]-1;
			ASSERT(iRec>=0);
		}
		while(iRec!=iFrg);
		piFRG[lNext]=-ln-1;
	}

#ifdef _DEBUG
	//As a sanity check, see if all this matches the NTS database --
	lNext=0;
	for(ln=0;ln<nLNKS;ln++) {
		iRec=piLNK[ln];
		while(iRec>=0) {
			lNext++;
			if(!GoWorkFile(&dbNTS,piSTR[iRec])) {
				goto _errfmt;
			}
			iRec=piFRG[iRec]-1;
			if(iRec>=0) {
				if(piSTR[iRec]!=pSTR->lnk_nxt) {
					goto _errfmt;
				}
			}
			else if(iRec!=-ln-2) {
				goto _errfmt;
			}
		}
	}
	ASSERT(lNext==nFRGS);
#endif

	return;

_errfmt:
	//corrupted links in NTS file
	ASSERT(FALSE);
	for(lNext=0;lNext<nFRGS;lNext++) piFRG[piLNK[lNext]]=0;
	nLNKS=nFRGS=0;
}

int CPrjDoc::SetNetwork(int iNet)
{
	char buf[80];
			    
	ASSERT(dbNTN.Opened() && dbNTS.Opened());
	
	CSegView::SetNewVisibility(TRUE);
	
	//We are selecting a new network. If iNet==0, we want to select
	//the component with an active saved view (if one exists), otherwise
	//the first component component is selected --

	if(!iNet) {
		ASSERT(ixSEG.Opened());
		VIEWFORMAT *pvf=((SEGHDR *)ixSEG.ExtraPtr())->vf;
		if(!pvf[iNet].bActive && !pvf[++iNet].bActive) iNet=1;
		else iNet=pvf[iNet].iComponent;
	}
	
	free(CCompView::m_pNamTyp);
	CCompView::m_pNamTyp=NULL;
	nSTR=nSYS=nLNKS=nFRGS=0;
	
	if(iNet<=0) iNet=1;
	if(!GoWorkFile(&dbNTN,iNet)) return -1;
	
	iNet--; //return value
	
	if(!pNTN->str_id1) {
	  pCV->SysTTL("");
	  pCV->LoopTTL(0);
	  ASSERT(iNet>=0);
	  return iNet;
	}

	nSYS=pNTN->num_sys;
	nSTR_1=pNTN->str_id1;
	nSTR_2=pNTN->str_id2;
	nSTR=nSTR_2-nSTR_1+1;
		  
	int sRec=nSYS+1+3*nSTR;
	  
	//If necessary, resize array of indices into NTS --
	if(nLen_piSYS<sRec) {
	  int *p=(int *)realloc(piSYS,sRec*sizeof(int));
	  if(!p) {
	     CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_MEMORY);
	     nSTR=nSYS=0;
	     return -1;
	  }
	  piSYS=p;
	  nLen_piSYS=sRec;
	}
	
	piSTR=piSYS+nSYS+1;
	piPLT=piSTR+nSTR;

	//Added 11/4/04 to support link fragments
	piFRG=piPLT+nSTR;
	memset(piFRG,0,nSTR*sizeof(*piFRG));
	
	nISOL=nISOL_ni[0]=nISOL_ni[1]=0;
	ssISOL[0]=ssISOL[1]=0.0;
		
	int sys_id=-2;  //Start with invalid system ID
	int sys_id_recs=-2;
		  
	for(sRec=nSTR_1;sRec<=nSTR_2;sRec++) {
	    if(!GoWorkFile(&dbNTS,sRec)) {
	       return -1;
	    }
	    if(!pSTR->vec_cnt) continue;
	    
	    if(sys_id!=pSTR->sys_id) {
	      //We have encountered the first string of a new system --
	      //If previous system was a multiloop system, set its string count --
	      if(sys_id>=0) piSYS[sys_id]=sys_id_recs;
	      sys_id=pSTR->sys_id;
	      sys_id_recs=0;
	    }
	    
	    if(sys_id==-1) {
		    //Accumulate statistics for isolated loops --
			ssISOL[0]+=pSTR->sys_ss[0];
			ssISOL[1]+=pSTR->sys_ss[1];
			if(pSTR->flag&NET_STR_FLOATH) nISOL_ni[0]++;
			if(pSTR->flag&NET_STR_FLOATV) nISOL_ni[1]++;
			nISOL++;
		}
		else sys_id_recs++;
	    //Now let's handle the current string --
	    piSTR[pSTR->str_id]=sRec;

		if(pSTR->lnk_nxt) {
			//This sets piLNK[nFRGS++]=str_id, piFRG[str_id]=-lnk_nxt
			AllocLinkFragment(pSTR->str_id,-pSTR->lnk_nxt);
		}
	}

	if(nFRGS) {
		InitLinkFragments();
	}
	
	if(sys_id_recs) {
	   ASSERT(sys_id>=0);
	   piSYS[sys_id]=sys_id_recs;
	}
	
	double ss_str[2];
	
	//Initialize total loop counts and total net_ss --
	net_nc=nISOL;
	net_ni[0]=nISOL_ni[0];
	net_ni[1]=nISOL_ni[1];
	net_ss[0]=ssISOL[0];
	net_ss[1]=ssISOL[1];
	
	for(sRec=sys_id=0;sys_id<nSYS;sys_id++) {
	  if(!GoWorkFile(&dbNTS,piSTR[sRec])) {
	    return -1;
	  }
	  ASSERT(sRec==pSTR->str_id);
	  net_nc+=pSTR->sys_nc;
	  net_ni[0]+=pSTR->sys_ni[0];
	  net_ni[1]+=pSTR->sys_ni[1];
	  net_ss[0]+=(ss_str[0]=pSTR->sys_ss[0]);
	  net_ss[1]+=(ss_str[1]=pSTR->sys_ss[1]);
	  get_uveline(buf,piSYS[sys_id],ss_str,pSTR->sys_ni,pSTR->sys_nc);
	  pCV->AddString(buf,IDC_SYSTEMS);
	  sRec=(piSYS[sys_id]+=sRec);
	}
	
	piSYS[nSYS]=nISOL+sRec;
	
	ASSERT(piSYS);
	
	if(nISOL) {
	  get_uveline(buf,nISOL,ssISOL,nISOL_ni,nISOL);
	  pCV->AddString(buf,IDC_ISOLATED);
	}
	
	ASSERT(net_nc);
	
	get_uveline(buf,0,net_ss,net_ni,net_nc);
	pCV->SysTTL(buf);
	
	pCV->LoopTTL(net_nc-net_ni[0],net_nc-net_ni[1]);
	ASSERT(iNet>=0);
	return iNet;	  
}

void CPrjDoc::SetVector()
{
  //Sets iFindStr and iFindSel (vector index in string)
  //Assumes pPLT is positioned at vector.

  ASSERT(pPLT->str_id);
  if(dbNTS.Go(pPLT->str_id)) return;

  int rec_tgt=(1+pPLT->vec_id)/2;
  int rec=pSTR->vec_id;
  
  for(int i=pSTR->vec_cnt;i;i--) {
	if(rec_tgt==rec) {
	  iFindStr=pPLT->str_id;
	  iFindSel=pSTR->vec_cnt-i;
	  return;
	}
	if(!rec || dbNTV.Go(rec)) break;
	rec=pVEC->str_nxt_vc;
  }
  
  ASSERT(FALSE);
}

void CPrjDoc::AppendStationName(CString &str,int pfx,const void *f)
{
	char buf[SRV_LEN_PREFIX_DISP+SRV_SIZNAME+4];  //28+8+4 (2 extra)
	char *p=buf;

	if(pfx>0) {
		CSegView::GetPrefix(p,pfx);
		p+=strlen(p);
		*p++=':';
	}
	GetStrFromFld(p,f,SRV_SIZNAME);
	str+=buf;
}

DWORD CPrjDoc::GetVecPltPosition()
{
	//Return dbNTP record number corresponding to dbNTV.Position();
	//Position pPLT at pVEC's FROM station if idx==0.
	//Position it at pVEC's TO station if idx==1.
	DWORD ntvrec=dbNTV.Position();
	int i,imax=dbNTP.NumRecs();
	for(i=1;i<=imax;i++) {
		if(dbNTP.Go(i)) break;
		if((pPLT->vec_id+1)/2==ntvrec) {
			return i;
		}
	}
	return 0;
}

void CPrjDoc::AppendPltStationName(CString &str,int iForward)
{
	if(iForward>=0) {
		//Assumes dbNTP is positioned at a vector --
		ASSERT(pPLT->vec_id);
		if(iForward) VERIFY(!dbNTP.Prev());
		AppendStationName(str,pPLT->end_pfx,pPLT->name);
		str+=" to ";
		if(iForward) VERIFY(!dbNTP.Next());
		else VERIFY(!dbNTP.Prev());
	}
	AppendStationName(str,pPLT->end_pfx,pPLT->name);
	if(!iForward) VERIFY(!dbNTP.Next()); 
}

void CPrjDoc::AppendVecStationNames(CString &str)
{
	AppendStationName(str,pVEC->pfx[0],pVEC->nam[0]);
	str+=" to ";
	AppendStationName(str,pVEC->pfx[1],pVEC->nam[1]);
}


BOOL CPrjDoc::GetPrefixedName(CString &buf,BOOL bVector)
{
	if(dbNTP.Go(bVector>0?m_iFindVector:m_iFindStation)) return FALSE;

	BOOL bBracket=-1;

	if(bVector>=0) {
		int incr=bVector?m_iFindVectorMax:m_iFindStationMax;
		if(incr>1) {
			char pfx[12];
			incr=bVector?m_iFindVectorIncr:m_iFindStationIncr;
			incr=(incr>0)?'>':'<';
			sprintf(pfx,"%d%c ",bVector?m_iFindVectorCnt:m_iFindStationCnt,incr);
			buf+=pfx;
		}
		bBracket=(!bVector || abs(m_iFindVectorIncr)>1);
	}

	if(bBracket>=0) buf+=(bBracket?'[':'(');	
	AppendPltStationName(buf,(bVector>0)?m_iFindVectorDir:-1);
	if(bBracket>=0) buf+=(bBracket?']':')');	
	return TRUE;
}


BOOL CPrjDoc::FindVector(BOOL bMatchCase,int iNext)
{
 	// Find vector in NTP database corresponding to name pair, cfg_argv[0] and cfg_argv[1].
	// Upon success, leave pPLT positioned at found vector (pPLT->vec_id>0)
 	//
	// iNext==2  - Locate data file vector in Geometry
	// iNext==3  - Locate data file vector on map
	// iNext==0  - Locate possibly ambiguous vector in Geometry going forward (initial call)
	// iNext==1  - Locate next Geometry vector going forward in found set
	// iNext==-1 - Locate next Geometry vector going backward in found set

	if((iNext==-1 || iNext==1) && (iNext*m_iFindVectorIncr)<1) {
		//We are reversing direction so return same vector as last
		ASSERT(m_iFindVectorCnt>0);
		ASSERT(m_iFindVectorMax>1);
		m_iFindVectorIncr=-m_iFindVectorIncr;
		//m_iFindVector+=iNext; //Return TO station in new direction
		//VERIFY(!dbNTP.Go((iNext>0)?m_iFindVector:(m_iFindVector+1)));
		VERIFY(!dbNTP.Go(m_iFindVector));
		m_iFindStation=0;
		return TRUE;
	}

    int i,ln[2];
 	LPBYTE pName[2];
    DWORD pfx[2];
    int (*compare)(const void *,const void *,UINT);

	ASSERT(cfg_argc==2);

    for(i=0;i<2;i++) {
	  pName[i]=(LPBYTE)cfg_argv[i];
      //Note: TrimPrefix() returns -1 if prefix is absent --
      if((pfx[i]=TrimPrefix(pName+i,bMatchCase))==-2) {
         //A prefix was present but not found in database --
         return FALSE;
      }
      //Append a blank to speed searches in blank-extended NTP database fields --
	  ln[i]=strlen((char *)pName[i]);
      if(ln[i]<NET_SIZNAME) pName[i][ln[i]++]=' ';
    }
        
    compare=bMatchCase?memcmp:memicmp;
	BOOL iNotInLoopCnt=0,bAllVecs=FALSE;
	int iNotInLoopVec,iNotInLoopVecDir;
	char *pBase;

	if(iNext>1) {
		//Search from data file (wedview.cpp) -- we'll make just one forward pass --
		pBase=CPrjDoc::m_pErrorNode->BaseName();
		i=strlen(pBase);
		if(i<LEN_BASENAME) memset(pBase+i,' ',LEN_BASENAME-i);
		bAllVecs=iNext>2; //If map search
		iNext=0;
	}
	else pBase=NULL;

	int irec,incr;
	int recfirst=0;
	int reclast=dbNTP.NumRecs()+1;

	ASSERT(!iNext || (m_iFindVector && m_iFindVector<reclast));

	if(iNext>=0) incr=1;
	else {
		incr=recfirst;
		recfirst=reclast;
		reclast=incr;
		incr=-1;
	}

	if(iNext) {
		if(abs(m_iFindVectorIncr)>1) {
			//We're stepping through non-loop vector matches --
			ASSERT(pBase==NULL);
			bAllVecs=TRUE; //Have switched to map search from a failed geometry search
		}
		ASSERT(m_iFindVectorCnt>0);
		ASSERT(m_iFindVectorMax>1);
		//Continuing in same direction, so we'll begin search at last matched station.
		irec=m_iFindVector-incr;
		//If going backward, the last matched station was actually
		//m_iFindVector-1 (a FROM station) --
		if(incr<0) irec--;
		if((m_iFindVectorCnt+=incr)>m_iFindVectorMax) {
			irec=recfirst;
			m_iFindVectorCnt=1;
		}
		else if(!m_iFindVectorCnt) {
			irec=recfirst;
			m_iFindVectorCnt=m_iFindVectorMax;
		}
		ASSERT((irec+incr)>=1 && (irec+incr)<=(int)dbNTP.NumRecs());
	}
	else {
		irec=recfirst;
		m_iFindVector=m_iFindVectorCnt=m_iFindVectorMax=0;
		m_iFindVectorIncr=bAllVecs+1;  //Indicate an bAllVecs find
		/*
		if(bAllVecs) {
			m_iFindStation=m_iFindStationCnt=m_iFindStationMax=0;
			m_iFindStationIncr=2;  //Indicate an bAllVecs find
		}
		else {
			m_iFindVector=m_iFindVectorCnt=m_iFindVectorMax=0;
			m_iFindVectorIncr=1;
		}
		*/
	}

    i=2; //Start with no match pending

	while((irec+=incr)!=reclast) {
		if(dbNTP.Go(irec)) {
			ASSERT(FALSE);
			goto _nofind;
		}
		if(i==2 || (incr>0 && !pPLT->vec_id)) {
			//Completed match not possible here:
			//This is either an explicit FROM station going forward
			//or the preceding FROM/TO station didn't match.

			//First match going backward requires a vector --
			if(incr<0 && !pPLT->vec_id) continue;
			
			for(i=0;i<2;i++) {
				if(!compare(pName[i],pPLT->name,ln[i]) &&
				(pfx[i]==(DWORD)-1 || pfx[i]==pPLT->end_pfx)) break;
			}
			if(i<2) i=1-i;
		}
		else {
			//Possible second match (possibly a FROM station going backward).
			//This is a station whose FROM/TO station matches pName[1-i] --
			if(!compare(pName[i],pPLT->name,ln[i]) && (pfx[i]==(DWORD)-1 || pfx[i]==pPLT->end_pfx)) {
				//The name pair matches target --

				//Position pPLT at TO station to obtain vector information --
				if(incr<0 && dbNTP.Go(irec+1)) {
					ASSERT(FALSE);
					goto _nofind;
				}
				
				ASSERT(pPLT->vec_id);

				if(pBase) {
					//Data file search --
					if(dbNTV.Go((pPLT->vec_id+1)/2) ||
						(DWORD)CPrjDoc::m_nLineStart!=pVEC->lineno ||
						memcmp(pBase,pVEC->filename,LEN_BASENAME))	goto _nocompare;

					if(bAllVecs) {
						m_iFindVectorMax=m_iFindVectorCnt=1;
						m_iFindVector=irec;
						m_iFindVectorDir=i;
						m_iFindStation=0;
						return TRUE;
					}
					//else we'll now test if this is a loop vector --
				}

				if(bAllVecs || pPLT->str_id>0) {

					//The vector qualifies as a match -- 
					if(pBase || iNext) {
						//Position m_iFindVector at the TO station where pPLT
						//is already positioned. pName[i] is the last name matched
						if(incr<0) {
							m_iFindVector=irec+1;
							m_iFindVectorDir=1-i;
						}
						else {
							m_iFindVector=irec;
							m_iFindVectorDir=i; //If 0, a reversal is required for display
						}
						m_iFindVectorIncr=incr;
						if(pBase) {
							m_iFindVectorMax=m_iFindVectorCnt=1;
						}
						else if(bAllVecs) {
							//Scanning non-loop vectors --
							//m_iFindVectorCnt has already been incremented and pPLT is positioned
							//at the matching vector --
							ASSERT(pPLT->vec_id && !pPLT->str_id);
							m_iFindVectorIncr*=2;
						}
						m_iFindStation=0;
						return TRUE;
					}

					//Initial Geometry vector scan. Continue forward scan to count matches --
					if(++m_iFindVectorCnt==1) {
						m_iFindVector=irec;
						m_iFindVectorDir=i; //pName[i] is the last name matched
					}
					i=1-i;
					continue;
				}

				//At least one matching vector is not in a loop.
				if(++iNotInLoopCnt==1) {
					iNotInLoopVec=irec;
					iNotInLoopVecDir=i;
				}

				if(pBase || pfx[0]!=(DWORD)-1 && pfx[1]!=(DWORD)-1) goto _nofind;
				//Not in loop but one or both prefixes missing, so keep searching...
				i=1-i;
				continue;
			}
_nocompare:
			i=2; //no match pending
		}
    }
 
	ASSERT(!iNext);
	if(!iNext) {
		m_iFindVectorMax=m_iFindVectorCnt;
		if(m_iFindVector) {
			m_iFindVectorCnt=1;
			VERIFY(!dbNTP.Go(m_iFindVector));
			m_iFindStation=0;
			return TRUE;
		}
	}

_nofind:
    //Repair name strings --
    for(i=0;i<2;i++) if(pName[i][ln[i]-1]==' ') pName[i][ln[i]-1]=0;
	m_iFindVector=0;
	if(iNotInLoopCnt) {
		CString msg;
		if(pBase) {
			ASSERT(dbNTP.Position()==(DWORD)iNotInLoopVec);
			CString sTemp;
			AppendPltStationName(sTemp,iNotInLoopVecDir); //Include from station
			msg.Format(IDS_ERR_NOTLOOPVECTOR1,(LPCSTR)sTemp);
		}
		else {
			msg.Format(IDS_ERR_NOTLOOPVECTOR,cfg_argv[0],cfg_argv[1],iNotInLoopCnt);
		}
		CNotLoopDlg dlg(msg,"Locate On Map");
		if(dlg.DoModal()==IDOK) return FALSE;

		//Request to locate non-loop vector(s) on map.
		//If pBase!=NULL, there is only one to locate.
		ASSERT(!pBase || (!bAllVecs && iNotInLoopCnt==1));
		//In case of a manual serach, there may be duplicate not-in-loop vectors.
		//Set m_iFindVectorMax=iNotInLoopCnt for Find Prev/Next usage.
		//Also, disable any current manual station search.
		m_iFindStation=0;
		m_iFindVectorMax=iNotInLoopCnt;
		m_iFindVectorIncr=2; //Display on map only
		m_iFindVectorCnt=1;
		m_iFindVectorDir=iNotInLoopVecDir;
		VERIFY(!dbNTP.Go(m_iFindVector=iNotInLoopVec));
		return TRUE;
	}
    CMsgBox(MB_ICONINFORMATION,(pBase?IDS_ERR_VECNOFINDEDIT:IDS_ERR_VECNOFIND),cfg_argv[0],cfg_argv[1]);
    return (FALSE);
}

void CPrjDoc::LocateOnMap()
{
	//Assumes pPLT is positioned at vector that was found --
	DWORD rec=dbNTP.Position();
	//BOOL bNewComponent=(pCV->GoToComponent()>1);
	pCV->GoToComponent();
	UINT iTab=pCV->GetReView()->GetTabIndex();
	if(iTab!=TAB_PLOT) pCV->GetReView()->switchTab(TAB_PLOT);
	//else if(bNewComponent) pCV->PlotTraverse(0);
	//rec is the vector's TO station in dbNTP --
	//What if this is an explicit search, such as "T1 <REF>" --
	if(!m_iFindVectorDir) rec--;
	VERIFY(!dbNTP.Go(rec));
	if(!pPLT->end_id) {
		if(!m_iFindVectorDir) VERIFY(!dbNTP.Next());
		else VERIFY(!dbNTP.Prev());
	}
	LocateStation();
}

CGradient * CPrjDoc::GetGradientPtr(int iTyp)
{
	return pSV->m_Gradient[iTyp];
}

void CPrjDoc::GetValueRange(double *fMinimum,double *fMaximum,int typ)
{
	DWORD rec=0,reclast=dbNTP.NumRecs();
	double fVal,fMin=DBL_MAX,fMax=-DBL_MAX,fLastZ=DBL_MIN;
	int iVal,iMin=INT_MAX,iMax=INT_MIN;

	if(typ==2) {
		m_iValueRangeNet=(int)dbNTN.Position();
		rec=pNTN->plt_id1-1;
		reclast=pNTN->plt_id2;
	}
	else {
		rec=0;
		reclast=dbNTP.NumRecs();
	}

	while(rec<reclast) {
		if(dbNTP.Go(++rec)) {
			ASSERT(FALSE);
			break;
		}
		if(!pPLT->end_id || (typ && !pPLT->vec_id)) {
			if(!typ) fLastZ=DBL_MIN;
			continue;
		}

		switch(typ) {
			case 0: //Depth --
				fVal=fLastZ;
				fLastZ=pPLT->xyz[2];
				if(!pPLT->vec_id || fVal==DBL_MIN) continue;
				fVal=(fVal+fLastZ)/2.0;
				if(fVal<fMin) fMin=fVal;
				if(fVal>fMax) fMax=fVal;
				break;

			case 1: //Date -- Only need range for now --
				iVal=trx_DateToJul(pPLT->date);
				if(iVal>TRX_EMPTY_DATE) {
					if(iVal<iMin) iMin=iVal;
					if(iVal>iMax) iMax=iVal;
				}
				break;

			case 2: //F-Ratio --
				fVal=pPLT->str_id?GetFRatio():GRAD_VAL_NONSTRING;
				if(fVal<fMin) fMin=fVal;
				if(fVal>fMax) fMax=fVal;
				break;
		}
	}
	if(typ==1) {
		if(iMin==INT_MAX) {
			iMin=iMax=TRX_EMPTY_DATE;
		}
		*fMinimum=(double)iMin;
		*fMaximum=(double)iMax;
	}
	else {
		*fMinimum=fMin;
		*fMaximum=fMax;
	}
}

void CPrjDoc::FillHistArray(float *pHistValue,int len,double fStart,double fEnd,int typ)
{
	if(!pHistValue) return;
	WORD *wArray=(WORD *)(pHistValue+len);
	DWORD rec,reclast;
	double fVal,fLastZ=DBL_MIN;
	int iCell,iVal,iNonStr=0;

	if(typ==2) {
		ASSERT(m_iValueRangeNet);
		rec=pNTN->plt_id1-1;
		reclast=pNTN->plt_id2;
	}
	else {
		rec=0;
		reclast=dbNTP.NumRecs();
	}

	for(iCell=0;iCell<len;iCell++) pHistValue[iCell]=FLT_MAX;
	memset(wArray,0,(len+1)*sizeof(WORD));

	while(rec<reclast) {
		if(dbNTP.Go(++rec)) {
			ASSERT(FALSE);
			break;
		}
		if(!pPLT->end_id || (typ && !pPLT->vec_id)) {
			if(!typ) fLastZ=DBL_MIN;
			continue;
		}

		switch(typ) {
			case 0: //Depth --
				fVal=fLastZ;
				fLastZ=pPLT->xyz[2];
				if(!pPLT->vec_id || fVal==DBL_MIN) continue;
				fVal=(fVal+fLastZ)/2.0;
				break;

			case 1: //Date --
				iVal=trx_DateToJul(pPLT->date);
				if(iVal<=(double)TRX_EMPTY_DATE) continue;
				fVal=(double)iVal;
				if(fVal==fStart) pHistValue[0]=FLT_LVALUE(iVal);
				else if(fVal==fEnd) pHistValue[len-1]=FLT_LVALUE(iVal);
				break;

			case 2: //F-Ratio --
				fVal=pPLT->str_id?GetFRatio():GRAD_VAL_NONSTRING;
				break;
		}

		//If in range, increment counter for this value --
		if(fVal<fStart || fVal>fEnd) continue;

		if(typ==2 && fVal==GRAD_VAL_NONSTRING) {
			iNonStr++;
			continue;
		}

		iCell=(int)(len*(fVal-fStart)/(fEnd-fStart))+1;
		ASSERT(iCell<=len+1 && iCell>0);
		if(iCell>len) iCell=len;
		if(++wArray[iCell]>wArray[0]) wArray[0]++;
		if(pHistValue[--iCell]==FLT_MAX) pHistValue[iCell]=(typ!=1)?(float)fVal:FLT_LVALUE(iVal);
	}

	if(iNonStr) {
		//Make the non-string spike no taller than the current tallest spike if there is one --
		if(iNonStr>(int)wArray[0]) {
			if(wArray[0]) iNonStr=(int)wArray[0];
			else wArray[0]=(WORD)iNonStr;
		}
		iCell=(int)(len*(GRAD_VAL_NONSTRING-fStart)/(fEnd-fStart))+1;
		if(iCell>len) iCell=len;
		wArray[iCell]=(WORD)iNonStr;
		pHistValue[--iCell]=(float)GRAD_VAL_NONSTRING;
	}
}

void CPrjDoc::InitGradient(CGradient *pGrad,int n)
{
	ASSERT(!pGrad->GetPegCount());

	switch(n) {
		case 0: //color by depth
			pGrad->SetStartPegColor(RGB_BLUE);
			pGrad->SetEndPegColor(RGB_YELLOW);
			pGrad->SetInterpolationMethod(CGradient::HSLLongest);
			break;
		case 1: //color by date
			pGrad->SetStartPegColor(RGB_BLUE);
			pGrad->SetEndPegColor(RGB_RED);
			pGrad->SetInterpolationMethod(CGradient::HSLLongest);
			break;
		case 2: //color by stats
			pGrad->SetStartPegColor(RGB_CYAN);
			pGrad->SetEndPegColor(RGB_RED);
			pGrad->AddPeg(RGB_YELLOW,0.4);
			pGrad->AddPeg(RGB_WHITE,0.6);
			pGrad->SetInterpolationMethod(CGradient::Linear);
			break;
	}
}

void CPrjDoc::InitGradValues(CGradient &grad,double fMin,double fMax,int iTyp)
{
	if(iTyp==2) {
			//F-Ratio
			grad.m_fStartValue=-3.0;
			grad.m_fEndValue=__max(3.0,fMax);
			for(int i=grad.GetPegCount();i;) {
				grad.RemovePeg(--i);
			}
			grad.AddPeg(RGB_YELLOW,grad.PosFromValue(-1.0));
			grad.AddPeg(RGB_WHITE,grad.PosFromValue(0.0));
	}
	else if(fMin>=fMax) {
		if(iTyp==0) {
			//Depth --
			grad.m_fStartValue=fMin-10.0;
			grad.m_fEndValue=fMax+10.0;
		}
		else {
			//Date
			grad.m_fStartValue=fMin;
			grad.m_fEndValue=fMax+15.0;
		}
	}
	else {
		grad.m_fStartValue=fMin;
		grad.m_fEndValue=fMax;
	}
}

double CPrjDoc::GetFRatio(int i)
{
	//Assumes stSTR (struct strstats) is initialized.
	//Also initialized are bStrFloating[],bStrFixed[],and bStrBridge[].
  
	if(bStrFloating[i] || bStrBridge[i] ) return GRAD_VAL_FLOATING;
	if(bStrFixed[i]) return GRAD_VAL_FIXED;
	
	//Compute inc_ss = (Se/n)/((SS-Se)/(n*(nc-1))) = F statistic
	//Compute sys_ss = (SS-Se)/(n*(nc-1)) = UVE after deletion
    //double sys_ss=stSTR.sys_ss[i];
    double inc_ss=stSTR.inc_ss[i];

	if(net_nc-net_ni[i]>1 && net_ss[i]>inc_ss) {
		inc_ss/=(net_ss[i]-inc_ss)/(net_nc-net_ni[i]-1);
	}
	/*
	//System-specic, not global, F-ratio --
	if(stSTR.sys_nc[i]>1 && sys_ss>inc_ss) {
		sys_ss=(sys_ss-inc_ss)/(stSTR.sys_nc[i]-1);
		inc_ss/=sys_ss;
		//if(!i) sys_ss/=2.0;
	}
	*/
	else {
		//Our network consists of a single isolated loop.
		//Let's return the loop's UVE in place of the non-applicable F-ratio --
		//ASSERT(inc_ss==sys_ss);
		if(!i) inc_ss/=2.0;
	}
	return inc_ss;
}

double CPrjDoc::GetFRatio()
{
	ASSERT(m_iValueRangeNet);

	if(!GetStats(pPLT->str_id)) return GRAD_VAL_NONSTRING;

	double fH=GetFRatio(0),fV=GetFRatio(1);
	if(fH>=0.0) {
		if(fV>=0.0) fH=(2.0*fH+fV)/3.0;
	}
	else if(fV>=0.0) fH=fV;

	return __min(GRAD_VAL_MAXFRATIO,fH);
}




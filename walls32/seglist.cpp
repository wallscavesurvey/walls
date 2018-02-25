// seglist.cpp : CSegView::WriteVectorList() implementation
//

#include "stdafx.h"
#include "walls.h"
#include "prjdoc.h"
#include "plotview.h"
#include "segview.h"
#include "statdlg.h"
#include "compile.h"
#include "filebuf.h"
#include "listdlg.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#undef _SEGNC

DWORD FAR * CSegView::pPrefixRecs=NULL;
#define pRNode CPrjDoc::m_pReviewNode


DWORD *CSegView::pFlagNamePos=NULL; //ordered by seq
DWORD *CSegView::pFlagNameSeq; //Ordered by id

typedef struct {
	double a;
	double b;
	double c;
	double d;
	double e;
	double f;
	double xyzmin[3];
	double xyzmax[3];
} WALLS_CTM;

/////////////////////////////////////////////////////////////////////////////
// CSegView

BOOL CSegView::FlagVisible(CSegListNode *pNode)
{
	BOOL bLeaf=(pNode->IsLeaf() || pNode->IsOpen());
    if(bLeaf) pNode->m_bVisible|=LST_INCLUDE;
    else pNode->FlagVisible(LST_INCLUDE);
    return bLeaf;
}

void CSegView::GetLRUD(UINT i,float *lrud)
{
	char key[6];
	float rec[5];

	*key=3;
	*(UINT *)(key+1)=i;
	memset(lrud,0,5*sizeof(float));
	ixSEG.UseTree(NTA_LRUDTREE);
	if(!ixSEG.Find(key)) {
		do {
			ixSEG.GetRec(rec,sizeof(rec));
			for(i=0;i<5;i++) if(rec[i]==LRUD_BLANK || rec[i]==LRUD_BLANK_NEG) break;
			if(i==5) {
				if(LEN_ISFEET()) {
					for(i=0;i<4;i++) rec[i]=(float)LEN_SCALE(rec[i]);
				}
				memcpy(lrud,rec,sizeof(rec));
				break;
			}
		}
		while(!ixSEG.FindNext(key));
	}
	ixSEG.UseTree(NTA_SEGTREE);
}

UINT CSegView::GetPrefix(char *sprefix,UINT pfx,BOOL bLblFlags /*=8*/)
{
	char keybuf[256];
	*sprefix=0;
	if(!pfx || !pPrefixRecs) return 0;

	bLblFlags=(bLblFlags&8)?3:((bLblFlags&4)?2:1); //No. prefixes to extract

	ixSEG.UseTree(NTA_PREFIXTREE);
	if(!ixSEG.Go(pPrefixRecs[pfx-1])) {
		#ifdef _DEBUG
		short	spfx=0;
		ixSEG.GetRec(&spfx,sizeof(spfx));
		ASSERT(UINT(spfx)==pfx);
		#endif
		ixSEG.GetKey(keybuf);
		trx_Stxpc(keybuf);

		LPSTR p=keybuf;
		LPSTR p0=p+strlen(keybuf);
		LPSTR pEnd=p0;
		while(bLblFlags-- && p0>keybuf) {
			//*p0 is initially 0, after 1st pass *p0 is ':'
			for(p=p0;p>keybuf && *(p-1)!=SRV_CHAR_PREFIX;p--);
			if(p0-p>SRV_SIZNAME) {
				memcpy(p+SRV_SIZNAME,p0,pEnd-p0);
				pEnd-=(p0-p)-SRV_SIZNAME;
				*pEnd=0;
			}
			p0=p-1;
		}
		while(*p==SRV_CHAR_PREFIX) p++;
		if(pEnd-p>SRV_LEN_PREFIX_DISP) {
			ASSERT(0);
			p[SRV_LEN_PREFIX_DISP]=0;
		}
		strcpy(sprefix,p);
	}
	else pfx=0;
	ixSEG.UseTree(NTA_SEGTREE);
	return pfx;
}

static apfcn_s GetNoteStr(char *pNote)
{
	pNote[0]=5;
	pNote[1]=(char)NTA_CHAR_NOTEPREFIX;
	*((long *)&pNote[2])=pPLT->end_id;
	if(!ixSEG.Find(pNote)) {
		ixSEG.GetKey(pNote);
		pNote[(BYTE)pNote[0]+1]=0;
		pNote+=5;
		*pNote='\t';
		ElimNL(pNote);
	}
	else *pNote=0;
	return pNote;
}

static apfcn_u GetFlagNamePosition(BOOL bInit)
{
	DWORD dw;
	BYTE key[8];
	static bool bPending;

	if(bInit) {
		*key=5;
		key[1]=(char)NTA_CHAR_FLAGPREFIX;
		*((long *)&key[2])=pPLT->end_id;
		ixSEG.UseTree(NTA_NOTETREE);
		bPending=ixSEG.Find(key)==0;
		return bPending;
	}

	while(bPending) {
		VERIFY(ixSEG.GetKey(key,8)==0);
		dw=(DWORD)*(WORD *)&key[6];
		ASSERT(dw<pixSEGHDR->numFlagNames);
		*key=5;
		bPending=ixSEG.FindNext(key)==0;
		if(!(CSegView::pFlagNameSeq[dw]&0x80000000)) return trx_RevDW(CSegView::pFlagNameSeq[dw]);
	}

	return (UINT)-1;

}

static apfcn_i CopyNumericLabel(LPSTR dst,LPCSTR src)
{
	//To insure proper sorting of prefixed names, convert a trailing digit string
	//to an 8-byte digit string --.

	int len=strlen(src);
	LPCSTR p=src+len;
	for(;p>src;p--) {
	   if(!isdigit(p[-1])) break;
	}
	//*p=0 or p points to first digit
	len=p-src;
	if(len) memcpy(dst,src,len);
	if(*p) len+=sprintf(dst+len,"%08u",atoi(p));
	dst[len]=0;
	_strupr(dst);
	return len;
}

static apfcn_u GetPrefixNameKey(char *pfxnamekey,char *sprefix,UINT pfx)
{
	int e=1; //offset of next char in pfxnamekey
	char buf[SRV_SIZNAME+2];

	if(pPLT->end_pfx!=pfx) pfx=CSegView::GetPrefix(sprefix,pPLT->end_pfx);
	if(!pfx) *sprefix=0;
    if(*sprefix) {
	    strcpy(pfxnamekey+1,sprefix);
		_strupr(pfxnamekey+1);
		e+=strlen(sprefix);
	}
	pfxnamekey[e++]=0;
	GetStrFromFld(buf,pPLT->name,SRV_SIZNAME);
	_strupr(buf);
	e+=CopyNumericLabel(pfxnamekey+e,buf);
	*pfxnamekey=e-1;
	return pfx;
}

static apfcn_s GetPrefixName(char *pfxname)
{
	int e=0; //offset of next char in pfxname
	*pfxname=0;
	if(pPLT->end_pfx) {
		VERIFY(CSegView::GetPrefix(pfxname,pPLT->end_pfx));
		e+=strlen(pfxname);
	}
	pfxname[e++]='\t';
	GetStrFromFld(pfxname+e,pPLT->name,SRV_SIZNAME);
	return pfxname;
}

static UINT PASCAL IndexName(char *sprefix,UINT pfx,int uFlags)
{
	DWORD rec;
	BYTE key[SRV_SIZNAME+SRV_LEN_PREFIX_DISP+8+40]; //40 extra bytes due to possible numeric expansion
	
	if(!pPLT->end_id) return 0; //If this is <REF>

	rec=dbNTP.Position();

    if(uFlags&LFLG_GROUPBYFLAG) {
		DWORD pos;
		if(GetFlagNamePosition(TRUE)) {
			pfx=GetPrefixNameKey((char *)key+4,sprefix,pfx);
			*key=4+key[4];
		    ixSEG.UseTree(NTA_NOTETREE);
			while((pos=GetFlagNamePosition(FALSE))!=(DWORD)-1) {
				*(DWORD *)&key[1]=pos;
				pixNAM->InsertKey(&rec,key);
				ASSERT(!trx_errno || trx_errno==CTRXFile::ErrDup);
			}
		}
		ixSEG.UseTree(NTA_SEGTREE);
	}
	else {
		pfx=GetPrefixNameKey((char *)key,sprefix,pfx);
		pixNAM->InsertKey(&rec,key);
	}
	
	return pfx;
}

static void empty_msg(CFileBuffer *pFile)
{
	pFile->WriteArgv("No stations satisfy the criteria for listing.\r\n");
}

static void UpdateRange(WALLS_CTM &ctm)
{
	for(int i=0;i<3;i++) {
		if(ctm.xyzmax[i]<pPLT->xyz[i]) ctm.xyzmax[i]=pPLT->xyz[i];
		if(ctm.xyzmin[i]>pPLT->xyz[i]) ctm.xyzmin[i]=pPLT->xyz[i];
	}
}

static UINT ListNames(CFileBuffer *pFile,int uFlags,UINT uDecimals,WALLS_CTM &ctm)
{
  //Write an alphabetical list of prefixed names with coordinates --

  UINT count,count_ttl;
  char keybuf[8];
  char namebuf[SRV_SIZNAME+SRV_LEN_PREFIX_DISP+8];
  char flagbuf[TRX_MAX_KEYLEN+3];
  char notebuf[TRX_MAX_KEYLEN+3];
  DWORD rec,lastflagpos,flagpos;
  char *pNote;

  int e=pixNAM->First();
  count=count_ttl=0;
  lastflagpos=(DWORD)-1;

  while(!e) {
	  if(pixNAM->GetRec(&rec)) break;
	  ixSEG.UseTree(NTA_NOTETREE);

	  if(!dbNTP.Go(rec)) {
		pNote=GetNoteStr(notebuf);
		if(*pNote || (uFlags&LFLG_NOTEONLY)==0) {
			if(uFlags&LFLG_GROUPBYFLAG) {
				if(pixNAM->GetKey(keybuf,5)!=CTRXFile::ErrTruncate) {
					ASSERT(FALSE);
					break;
				}
				if(lastflagpos!=(flagpos=*(DWORD *)&keybuf[1])) {
					//New flag section --
					if(lastflagpos!=(DWORD)-1)
						pFile->WriteArgv("Station total: %u\r\n\r\n",count);
					count=0;
					lastflagpos=flagpos;
					flagpos=trx_RevDW(flagpos);
					DWORD dwPos=CSegView::pFlagNamePos[flagpos];
					if(ixSEG.SetPosition(dwPos) || ixSEG.GetKey(flagbuf)) {
						ASSERT(FALSE);
						strcpy(flagbuf,"?");
					}
					else {
						if(!*flagbuf) strcpy(flagbuf,"<Unnamed Flags>");
						else trx_Stxpc(flagbuf);
					}
					pFile->WriteArgv("%s\r\n",flagbuf);
					memset(flagbuf,'=',strlen(flagbuf));
					pFile->WriteArgv("%s\r\n",flagbuf);
				}
			}

			{
				double xy[2];
				int dec=uDecimals;
				if(uFlags&LFLG_LATLONG) {
					dec+=5;
					xy[0]=pPLT->xyz[0];
					xy[1]=pPLT->xyz[1];
					CMainFrame::ConvertUTM2LL(xy);
					double tmp=xy[0];
					xy[0]=xy[1];
					xy[1]=tmp;
				}
				else {
					xy[0]=LEN_SCALE(pPLT->xyz[0]);
					xy[1]=LEN_SCALE(pPLT->xyz[1]);
				}

				pFile->WriteArgv("%s\t%.*f\t%.*f\t%.*f%s\r\n",GetPrefixName(namebuf),
				  dec,xy[0],
				  dec,xy[1],
				  uDecimals,LEN_SCALE(pPLT->xyz[2]),pNote);
			}

			if(uFlags&LFLG_GROUPBYLOC) UpdateRange(ctm);
		    count++;
			count_ttl++;
		}
	  }
	  e=pixNAM->Next();
  }
  ixSEG.UseTree(NTA_SEGTREE);
  if(count) pFile->WriteArgv("\r\nStation total: %u\r\n",count);
  else if(!(uFlags&LFLG_GROUPBYFLAG)) empty_msg(pFile);
  return count_ttl;
}

static int PASCAL ListFileLink(CFileBuffer *pFile,UINT bIsVector)
{
	char sname[10],fname[20];

	//Let's get the File name and line number from the vector table --
	if(!dbNTV.Go((pPLT->vec_id+1)/2)) {
		GetStrFromFld(sname,pVEC->filename,8);
		sprintf(fname,"%s:%u",sname,pVEC->lineno);
	}
	else *fname=0;
	pFile->WriteArgv("\t%s",fname);
	if(bIsVector&2) pFile->WriteArgv("-");
	if(!(bIsVector&1)) {
		pFile->WriteArgv(">");
		return 1;
	}
	return 0;
}

static double VecLength(double *xyz1,double *xyz2)
{
	double s,sum=0.0;
	for(int i=0;i<3;i++) {
		s=xyz2[i]-xyz1[i];
		sum+=s*s;
	}
	return sqrt(sum);
}

static UINT PASCAL ListVector(CFileBuffer *pFile,char *sprefix,UINT pfx,UINT uDecimals)
{
	char sname[10];
	double xyz[3];
	
	GetStrFromFld(sname,pPLT->name,8);
	if(pPLT->end_id) {
	    //This is not <REF> --
	    
	    if((WORD)pPLT->end_pfx!=pfx) pfx=CSegView::GetPrefix(sprefix,pPLT->end_pfx);
		memcpy(xyz,pPLT->xyz,sizeof(xyz));
    }
    else {
      pfx=0;
      memset(xyz,0,sizeof(xyz));
    }
    
 	if(!pfx) *sprefix=0;
 	 
	pFile->WriteArgv("%s\t%s\t%.*f\t%.*f\t%.*f",sprefix,sname,
		uDecimals,LEN_SCALE(xyz[0]),
		uDecimals,LEN_SCALE(xyz[1]),
		uDecimals,LEN_SCALE(xyz[2]));
	return pfx;
}

static void PASCAL ListSegTitle(CFileBuffer *pFile,CSegListNode *pNode)
{
    char buf[SRV_SIZ_SEGNAMBUF];
    char *pTitle;
    if(pNode->TitlePtr()) pTitle=pNode->TitlePtr();
    else {
       strcpy(buf,pNode->Name());
       pTitle=buf;
       *buf&=0x7F;
#ifdef _SEGNC
       *buf=toupper(*buf);
#endif
    }
    pFile->WriteArgv("/%s",pTitle);
}

static int PASCAL ListSegPath(CFileBuffer *pFile,CSegListNode *pNode)
{
	if(pNode->m_pSeg==NULL) return 0; 
	int e=ListSegPath(pFile,pNode->Parent());
	if(e) {
	  ListSegTitle(pFile,pNode);
	  return 2;
	}
	ASSERT(pNode->TitlePtr());
	pFile->WriteArgv("%s\r\nSegment: ",pNode->TitlePtr());
	return 1;
}

UINT CSegView::GetFlagNameRecs()
{
	//DWORD *pFlagNamePos; //ordered by seq
	//DWORD *pFlagNameSeq; //Ordered by id

	UINT max_cnt=pixSEGHDR->numFlagNames,iseq=0;

	if(max_cnt) {

		ASSERT(!pFlagNamePos);

		pFlagNamePos=(DWORD *)calloc(2*max_cnt,sizeof(DWORD));
		if(!pFlagNamePos) return 0;
		pFlagNameSeq=&pFlagNamePos[max_cnt];

		UINT cnt=max_cnt;
		NTA_NOTETREE_REC nrec;

		ixSEG.UseTree(NTA_NOTETREE);
		if(!ixSEG.First()) {
			do {
				VERIFY(!ixSEG.GetRec(&nrec,sizeof(nrec)));
				if(M_ISFLAGNAME(nrec.seg_id)) {
					cnt--;
					ASSERT((WORD)nrec.id<(WORD)max_cnt);
					//if(M_ISHIDDEN(nrec.seg_id)) {
						//pFlagNameSeq[(WORD)nrec.id]=(DWORD)-1;
						//pFlagNamePos[iseq]=(DWORD)-1;
					//}
					//else {
					pFlagNameSeq[(WORD)nrec.id]=(M_ISHIDDEN(nrec.seg_id)?(iseq|0x80000000):iseq);
					pFlagNamePos[iseq]=ixSEG.Position();
					//}
					iseq++;
				}
			}
			while(cnt && !ixSEG.Next());
		}
		ASSERT(!cnt);
		ixSEG.UseTree(NTA_SEGTREE);
	}
	if(!iseq) FreeFlagNameRecs();
	return iseq;
}

void CSegView::GetPrefixRecs()
{
	int e;
	UINT numrecs;
	short pfx;
	
    ASSERT(pPrefixRecs==NULL);
	ixSEG.UseTree(NTA_PREFIXTREE);
	numrecs=(UINT)ixSEG.NumRecs();
	
	if(numrecs>0) {
	    if(!(e=ixSEG.First()) && (pPrefixRecs=(DWORD FAR *)calloc(numrecs,sizeof(DWORD)))!=NULL) {
			do {
			   e=ixSEG.GetRec(&pfx,sizeof(pfx));
			   pfx--;
			   if((UINT)pfx<numrecs) pPrefixRecs[pfx]=ixSEG.GetPosition();       
			}
			while(!ixSEG.Next());
		}
	}
	ixSEG.UseTree(NTA_SEGTREE);
}

static void init_ctm(WALLS_CTM &ctm,VIEWFORMAT &vf)
{
	//Matrix to convert world coordinates to inch page coordinates --
	double scale=vf.fFrameWidth/vf.fPanelWidth;
	double sA=sin(vf.fPanelView*U_DEGREES)*scale;
	double cA=cos(vf.fPanelView*U_DEGREES)*scale;
	ctm.a =  cA;
	ctm.b = -sA;
	ctm.c = -sA;
	ctm.d = -cA;
	ctm.e = vf.fFrameWidth/2 + sA*vf.fCenterNorth - cA*vf.fCenterEast;
	if(vf.bProfile) ctm.f=vf.fFrameHeight/(2*scale);
	else ctm.f = vf.fFrameHeight/2 + sA*vf.fCenterEast + cA*vf.fCenterNorth;
	for(int i=0;i<3;i++) {
		ctm.xyzmin[i]=DBL_MAX;
		ctm.xyzmax[i]=-DBL_MAX;
	}
}

/*
static void mult(WALLS_CTM &ctm,double *xy)
{
	double x=xy[0];
	xy[0]=ctm.a*x + ctm.c*xy[1] + ctm.e;
	xy[1]=ctm.b*x + ctm.d*xy[1] + ctm.f;
}
*/

static BOOL PltPointInFrame(WALLS_CTM &ctm,VIEWFORMAT &vf)
{
	double x=ctm.a*pPLT->xyz[0] + ctm.c*pPLT->xyz[1] + ctm.e;
	if(x<0.0 || x>vf.fFrameWidth) return FALSE;
	if(vf.bProfile) {
		if(fabs(pPLT->xyz[2]-vf.fCenterUp)>ctm.f) return FALSE;
	}
	else {
		x=ctm.b*pPLT->xyz[0] + ctm.d*pPLT->xyz[1] + ctm.f;
		if(x<0.0 || x>vf.fFrameHeight) return FALSE;
	}
	return TRUE;
}

void CSegView::WriteVectorList(SEGSTATS *st,LPCSTR pathname,CSegListNode *pNode,int uFlags,UINT uDecimals)
{
	char sprefix[SRV_LEN_PREFIX_DISP+2];
	BOOL bLeaf;
    CTRXFile xNAM;		//Name index file (temporary)
	
	BeginWaitCursor();

    //Close any existing file with that name --
	CMainFrame::CloseLineDoc(pathname,TRUE); 
	
	CFileBuffer file(CPrjDoc::SAVEBUFSIZ);
	
	if(file.OpenFail(pathname,
	  CFile::modeCreate|CFile::modeReadWrite|CFile::shareExclusive)) {
	    EndWaitCursor();
		return;
    }
		
	//GetPrefixRecs();

	pixNAM=NULL;
	ASSERT(pFlagNamePos==NULL);

	if(!(uFlags&LFLG_VECTOR)) {
		//Create array of flag name positions --
		if(!(uFlags&LFLG_GROUPBYFLAG) || GetFlagNameRecs()) {
			//Create temporary index for prefixed names --
			if(!xNAM.Create(GetDocument()->WorkPath(pRNode,CPrjDoc::TYP_NAM),sizeof(long))) { 
			  xNAM.SetUnique(); //We will only insert unique names
			  pixNAM=&xNAM;
			  //Assign cache and make it the default for indexes --
			  //if(e=xNAM.AllocCache(SIZ_INDEX_CACHE,TRUE)) xNAM.CloseDel();
			  ASSERT(xNAM.Cache()!=NULL);
			}
		}
	}

	if(uFlags&LFLG_LATLONG) {
		PRJREF *pr;
		if(!CPrjDoc::m_pReviewNode->InheritedState(FLG_GRIDMASK) ||
			!(pr=CPrjDoc::m_pReviewNode->GetAssignedRef())) uFlags&=~LFLG_LATLONG;
		else CMainFrame::PutRef(pr);
	}
	
	//First, set flag in each segment node to indicate which vectors
	//are to be included in list (we assume that they are initially cleared).
	//Note: LstInclude(segid) == ((m_pSegments+segid)->m_bVisible & LST_INCLUDE)!=0
	
	//Initialize test fcn: BOOL LstInclude(seg_id)
    bLeaf=FlagVisible(pNode); 
	
	UINT pfx=0;
	UINT lastrec=(UINT)pNTN->plt_id2;
    char date[10],time[10];
	VIEWFORMAT vf;
	WALLS_CTM ctm;

    _strtime(time);
    time[5]=0;
	
	TRY
	{
		if(ListSegPath(&file,pNode)<2 && bLeaf) file.WriteArgv("/");
		if(!bLeaf) file.WriteArgv("/...");
		file.WriteLn();
	 	file.WriteArgv(IDS_VECLSTFMT,
			  st->numvecs,_strdate(date),time,
			  st->component,st->refstation+5,
			  uDecimals,LEN_SCALE(st->ttlength),LEN_UNITSTR(),
			  uDecimals,LEN_SCALE(st->highpoint),st->highstation,
			  uDecimals,LEN_SCALE(st->lowpoint),st->lowstation);

		if(pRNode->InheritedState(FLG_GRIDMASK)) {
			char fmt[180];
			file.WriteArgv("%s\r\n",CPrjDoc::GetDatumStr(fmt,pRNode));
		}

		if(uFlags&(LFLG_NOTEONLY|LFLG_GROUPBYFLAG|LFLG_GROUPBYLOC)) {
			file.WriteArgv("Criteria for Listing: All ");
			if(uFlags&LFLG_NOTEONLY) file.WriteArgv("noted ");
			file.WriteArgv((uFlags&LFLG_VECTOR)?"vectors":"stations");
			if(uFlags&LFLG_GROUPBYFLAG) file.WriteArgv(" with non-hidden flags");
			if(uFlags&LFLG_GROUPBYLOC) {
				file.WriteArgv(" in current %s view.\r\n",pSV->InitPlotView()->m_bProfile?"profile":"plan");
				pPV->ClearTracker();
				CPrjDoc::InitViewFormat(&vf);
				file.WriteArgv("View Frame: %.2f x %.2f %s, View Direction: %.2f Deg\r\nFrame Center: %.2f East, %.2f North, %.2f Up\r\n",
					LEN_SCALE(vf.fPanelWidth),LEN_SCALE(vf.fPanelWidth*vf.fFrameHeight/vf.fFrameWidth),
					LEN_UNITSTR(),vf.fPanelView,LEN_SCALE(vf.fCenterEast),LEN_SCALE(vf.fCenterNorth),LEN_SCALE(vf.fCenterUp));
				init_ctm(ctm,vf);
			}
			else file.WriteArgv(".\r\n");
		}

	 	file.WriteArgv((uFlags&LFLG_VECTOR)?IDS_VECLSTFMTH:((uFlags&LFLG_LATLONG)?IDS_NAMLSTFMTH_LL:IDS_NAMLSTFMTH));

		if((uFlags&LFLG_VECTOR) || pixNAM) {

			int end_id,from_id=-2,last_end_id=-1;
			BOOL bInFrame,bLastRef=TRUE,bLastInFrame=FALSE;
			UINT outCount=0,count=0;
			double length=0.0,xyz[3];

			//For name-ordered reports, index records only on first pass --
			for(UINT rec=(UINT)pNTN->plt_id1;rec<=lastrec;rec++,from_id=end_id) {
			  if(dbNTP.Go(rec)) break;
			  end_id=pPLT->end_id;

			  bInFrame=(!(uFlags&LFLG_GROUPBYLOC) || PltPointInFrame(ctm,vf));

			  if(!pPLT->vec_id || !(bInFrame+bLastInFrame) || !LstInclude(pPLT->seg_id)) {
				  if(uFlags&LFLG_VECTOR) {
					bLastRef=end_id==0;
					if(!bLastRef) memcpy(xyz,pPLT->xyz,sizeof(xyz));
				  }
				  bLastInFrame=bInFrame;
				  continue;
			  }

			  //This is an included vector --
			  if(from_id!=last_end_id && (bLastInFrame || (uFlags&LFLG_VECTOR))) {
				  //Check from station --
				  if(!dbNTP.Go(rec-1)) {
					  if(uFlags&LFLG_VECTOR) {
						  pfx=ListVector(&file,sprefix,pfx,uDecimals);
						  if(bLastInFrame && (uFlags&LFLG_GROUPBYLOC)) UpdateRange(ctm);
					  }
					  else pfx=IndexName(sprefix,pfx,uFlags);
				  }
				  dbNTP.Go(rec);
				  if(uFlags&LFLG_VECTOR) {
					 if(uFlags&LFLG_GROUPBYLOC) outCount+=ListFileLink(&file,bLastInFrame);
				     file.WriteLn();
				  }
			  }
			  
			  if(uFlags&LFLG_VECTOR) {
					count++;
					pfx=ListVector(&file,sprefix,pfx,uDecimals);
					if(bInFrame && (uFlags&LFLG_GROUPBYLOC)) UpdateRange(ctm);
					outCount+=ListFileLink(&file,bInFrame+2);
					file.WriteLn();
					if(end_id && !bLastRef) {
						length+=VecLength(xyz,pPLT->xyz);
					}
					memcpy(xyz,pPLT->xyz,sizeof(xyz));
			  }
			  else if(bInFrame) {
				  pfx=IndexName(sprefix, pfx, uFlags);
			  }

			  last_end_id=end_id;
			  bLastRef=(end_id==0);
			  bLastInFrame=bInFrame;
			}

			if(uFlags&LFLG_VECTOR) {
				file.WriteArgv("\r\nVectors Listed: ");
				if(count) file.WriteArgv("%u  Length: %.2f %s\r\n",count,LEN_SCALE(length),LEN_UNITSTR());
				else file.WriteArgv("None\r\n");
			}
			else {
			    //Now actually produce the name-ordered report --
				count=ListNames(&file,uFlags,uDecimals,ctm);
			}
			if(count && (uFlags&LFLG_GROUPBYLOC)) {
				if(outCount) file.WriteArgv("Vectors Crossing Frame Boundary: %u",outCount);
				file.WriteArgv("\nCoordinate Ranges of Stations Inside Frame --\r\n"
					"East: \t%.*f\t%.*f\r\n"
					"North:\t%.*f\t%.*f\r\n"
					"Elev: \t%.*f\t%.*f\r\n",
					uDecimals,LEN_SCALE(ctm.xyzmin[0]),uDecimals,LEN_SCALE(ctm.xyzmax[0]),
					uDecimals,LEN_SCALE(ctm.xyzmin[1]),uDecimals,LEN_SCALE(ctm.xyzmax[1]),
					uDecimals,LEN_SCALE(ctm.xyzmin[2]),uDecimals,LEN_SCALE(ctm.xyzmax[2]));
			}
		}
		else empty_msg(&file);
		file.Close();
	}
	CATCH(CFileException,e)
	{
		file.Abort(); // will not throw an exception
		EndWaitCursor();

		TRY
			file.ReportException(pathname,e,TRUE, AFX_IDP_FAILED_TO_SAVE_DOC);
		END_TRY
		goto _ret;
	}
	END_CATCH
	
 	EndWaitCursor();
 	
 	//Now let's put the file in the editor --
 	AfxGetApp()->OpenDocumentFile(pathname);
 	
_ret:
	UnFlagVisible(pNode);
	if(pixNAM) pixNAM->CloseDel(FALSE); //Do not close cache
	FreeFlagNameRecs();
}

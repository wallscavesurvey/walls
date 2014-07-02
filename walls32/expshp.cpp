// expshp.cpp :
// CSegView::ExportSHP(), CSegView::ExportSVG(), and CPrjDoc::ExporNTW() implementations,
// including callbacks for wallshp.dll, wallshpx.dll, and wallsvg.dll.

#include "stdafx.h"
#include "walls.h"
#include "prjdoc.h"
#include "segview.h"
#include "plotview.h"
#include "compview.h"
#include "compile.h"
#include "wall_shp.h"
#include "expavdlg.h"
#include "svgadv.h"
#include "ExpSvgDlg.h"
#include "SvgProc.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

CExportShp *pExpShp;
bool bUseMag;

static BOOL bCRF,bVersionNotChecked;
static enum {TYP_SHP=0,TYP_CRF=1,TYP_SVG=2,TYP_SVGP=3};
static int typ_shp;
static HINSTANCE SVGhinst;

static STRING_BUF id_Orphans[2]; //vector id's ignored because not found in Walls project

int WriteLrudPolygons(LRUDPOLY_CB pOutPoly);

static SVG_VIEWFORMAT *pSvg;

CExportShp::CExportShp()
{
 	firstrec=(UINT)pNTN->plt_id1;
	lastrec=(UINT)pNTN->plt_id2;
	bInitStations=bInitVectors=bInitNotes=bInitPolygons=bSVGFloors=FALSE;
    pid=NULL;
	pFlagNameRec=NULL;
	pFlagBits=NULL;
}

CExportShp::~CExportShp()
{
	free(pid);
	free(pFlagNameRec);
	free(pFlagBits);
}

BOOL CExportShp::Init(UINT maxids)
{
	if(!(pid=(int FAR *)calloc(maxids,sizeof(int)))) return FALSE;
	len_pid=maxids;
	return TRUE;
}

void CExportShp::GetPLTCoords(double *xyz)
{
	memcpy(xyz,pPLT->xyz,3*sizeof(double));
	if(typ_shp<TYP_SVG) {
		if(bUseMag && pPLT->end_id) {
			MD.east=xyz[0];
			MD.north=xyz[1];
			if(MF(MAG_CVTUTM2LL3)) {
				//returned MAG_ERR_CVTUTM2LL
				ASSERT(0);
				xyz[0]=xyz[1]=0.0;
			}
			else {
				xyz[0]=MD.lon.fdeg*MD.lon.isgn;
				xyz[1]=MD.lat.fdeg*MD.lat.isgn;
			}
		}
	}
}
/*
BOOL CExportShp::VecIsVertical(void)
{
	return (fabs(fr_S.xyz[0]-pPLT->xyz[0])<0.1 && fabs(fr_S.xyz[1]-pPLT->xyz[1])<0.1);
}
*/

int CExportShp::GetLrud(SHP_TYP_LRUD *pL)
{
	//Return tne endpoint coordinates of an LRUD bar or passage shot --
	SHP_TYP_STATION pS;
	NTA_LRUDTREE_REC lrud;
	static UINT id;
	static double xy[2];
	int iNewKey;
	char key[6];
	double az;

	if(!pL) {
		id=(UINT)-1;
		return GetStation(NULL);
	}

	while(TRUE) {
		iNewKey=0;
		if(id==(UINT)-1) {
			if(!GetStation(&pS)) break;
			id=pS.id;
			memcpy(xy,pS.xyz,sizeof(xy));
			iNewKey=1;
		}
		*key=3;
		*(UINT *)(key+1)=id;

		ixSEG.UseTree(NTA_LRUDTREE);

		if(iNewKey=iNewKey?(!ixSEG.Find(key)):(!ixSEG.FindNext(key))) {
			//An Lrud was found for this station
			lrud.flags=0;
			ASSERT(ixSEG.SizRec()<=sizeof(NTA_LRUDTREE_REC));
			VERIFY(!ixSEG.GetRec(&lrud,ixSEG.SizRec()));
			if(lrud.az==LRUD_BLANK ||
			   (lrud.dim[1]==LRUD_BLANK && (lrud.dim[0]==LRUD_BLANK || lrud.dim[0]==LRUD_BLANK_NEG))) continue;
			//We have at least one ray with a horizontal component
			az=lrud.az*(PI/180.0);
			if(lrud.dim[1]==LRUD_BLANK || lrud.dim[0]==LRUD_BLANK || lrud.dim[0]==LRUD_BLANK_NEG) {
				pL->xyFr[0]=xy[0];
				pL->xyFr[1]=xy[1];
				if(lrud.dim[1]==LRUD_BLANK) {
					az-=PI2;
					pL->xyTo[0]=xy[0]+lrud.dim[0]*sin(az);
					pL->xyTo[1]=xy[1]+lrud.dim[0]*cos(az);
				}
				else {
					if(lrud.dim[0]==LRUD_BLANK) az+=PI2;
					pL->xyTo[0]=xy[0]+lrud.dim[1]*sin(az);
					pL->xyTo[1]=xy[1]+lrud.dim[1]*cos(az);
				}
			}
			else {
				az-=PI2;
				pL->xyFr[0]=xy[0]+lrud.dim[0]*sin(az);
				pL->xyFr[1]=xy[1]+lrud.dim[0]*cos(az);
				az+=PI;
				pL->xyTo[0]=xy[0]+lrud.dim[1]*sin(az);
				pL->xyTo[1]=xy[1]+lrud.dim[1]*cos(az);
			}
			if((lrud.flags&LRUD_FLG_CS)!=0 && lrud.dim[2]!=LRUD_BLANK && lrud.dim[3]!=LRUD_BLANK) {
				pL->flags=lrud.flags;
				pL->up=lrud.dim[2];
				pL->dn=lrud.dim[3];
			}
			else pL->flags=0;
			break;
		}
		else id=(UINT)-1;
	}

	ixSEG.UseTree(NTA_SEGTREE);
	return iNewKey;
}

void CExportShp::Store_fr_S(SHP_TYP_STATION *pS)
{
	if(pS->pfx_id==fr_S.pfx_id) strcpy(fr_S.prefix,pS->prefix);
	else fr_S.pfx_id=CSegView::GetPrefix(fr_S.prefix,fr_S.pfx_id);
	if(typ_shp<TYP_SVG) CSegView::GetLRUD(fr_S.id,fr_S.lrud);
}

int CExportShp::GetStation(SHP_TYP_STATION *pS,BOOL bNeedNames /*=FALSE*/)
{
    if(!pS) {
	   rec=firstrec;
	   end_id=fr_S.id=0;
	   next=-1;
	   memset(pid,0,len_pid*sizeof(int));
	   bInitStations=TRUE;
	   return len_pid;
	}

	if(!bInitStations) return FALSE;

	if(next) {
		if(next>0) goto _next;
		pS->pfx_id=0;
		*pS->prefix=0;
	}

_loop:
	if(rec>lastrec || dbNTP.Go(rec)) return bInitStations=FALSE;

	end_id=pPLT->end_id;

	if(!pPLT->vec_id || (typ_shp<=TYP_SVG && !pSV->LstInclude(pPLT->seg_id))
		/*|| (typ_shp>=TYP_SVG && VecIsVertical())*/) {
		if(!pid[end_id] || typ_shp>=TYP_SVG) {
			//Save the data for this station in case next pt is a TO station.
			//For SVGs we'll need to check for verticalness of next shot.
			GetPLTCoords(fr_S.xyz);
			if(bNeedNames && !pid[end_id]) {
				GetStrFromFld(fr_S.label,pPLT->name,SHP_LEN_LABEL);
				fr_S.pfx_id=pPLT->end_pfx; //Don't retrieve prefix unless needed
			}
		}
		goto _cont;
	}
	
	//A TO station for an included vector is found --
	
	if(fr_S.id && !pid[fr_S.id]) {
		//We need to return the saved from station --
		pid[fr_S.id]=1;
		if(bNeedNames) Store_fr_S(pS);
		*pS=fr_S;
	    next=1; //skip record advance
	    return TRUE;
	}
	
_next:

	//Ignore <REF>
	if(end_id && !pid[end_id]) {
		fr_S.id=end_id;
		GetPLTCoords(fr_S.xyz);
		pid[fr_S.id]=1;
		if(bNeedNames) {
			GetStrFromFld(fr_S.label,pPLT->name,SHP_LEN_LABEL);
			fr_S.pfx_id=pPLT->end_pfx;
			Store_fr_S(pS);
		}
		*pS=fr_S;
		next=0;
		rec++;
		return TRUE;
	}
	
_cont:
	fr_S.id=end_id;
	rec++;
	goto _loop;		  
}
//===========================================================================

void CSegView::GetParentTitles(int segid,char *buf,UINT bufsiz)
{
	UINT l,buflen=0;
	CSegListNode *pNode=(m_pSegments+segid)->Parent();
	char title[SHP_SIZ_BUF_TITLES];

	buf[0]=0;

	while(pNode && pNode->m_pSeg) {
		if(pNode->TitlePtr()) {
		   l=strlen(strcpy(title,pNode->TitlePtr()));
		   if(buflen && buflen+l+3<bufsiz) {
			   strcat(title," | ");
			   strcat(title,buf);
		   }
		   title[bufsiz-1]=0;
		   buflen=strlen(strcpy(buf,title));
		}
		pNode=pNode->Parent();
	}
}

void CSegView::GetAttributeNames(int segid,char *buf,UINT bufsiz)
{
	UINT l,buflen=0;
	CSegListNode *pNode=m_pSegments+segid;
	char names[SHP_SIZ_BUF_TITLES];

	buf[0]=0;

	while(pNode && pNode->m_pSeg) {
		if(pNode->TitlePtr()) break;
		l=strlen(strcpy(names,pNode->Name()));
		(*names)&=0x7F;
		if(buflen && buflen+l+3<bufsiz) {
			strcat(names," / ");
			strcat(names,buf);
		}
		names[bufsiz-1]=0;
		buflen=strlen(strcpy(buf,names));
		pNode=pNode->Parent();
	}
}

void CExportShp::GetShpLineType(SHP_TYP_VECTOR *pV)
{
	int segid=pV->seg_id;
	styletyp *st=(pSV->m_pSegments+segid)->Style();
	COLORREF rgb;
	int idx=st->GradIdx();
	if(idx==1) {
		rgb=GetGradient(0)->ColorFromValue((pV->station[0].xyz[2]+pV->station[1].xyz[2])/2.0);
	}
	else if(idx==2) {
			int j=-1;
			if(*pV->date) {
				CString s(pV->date);
				s.Insert(4,'-');
				s.Insert(7,'-');
				j=trx_DateStrToJul(s);
			}
			if(j<0) {
				j++;
				j=trx_DateToJul(&j);
			}
			rgb=GetGradient(1)->ColorFromValue(j);
	}
	else if(idx==3) {
		double dVal=GRAD_VAL_NONSTRING;
		if(pPLT->str_id) dVal=CPrjDoc::GetFRatio();
		rgb=GetGradient(2)->ColorFromValue(dVal);
	}
	else rgb=RGB(st->r,st->g,st->b);
	sprintf(pV->linetype,"%02X%02X%02X%02X",GetRValue(rgb),GetGValue(rgb),GetBValue(rgb),st->lineidx);
}

BOOL CExportShp::GetVector(SHP_TYP_VECTOR *pV)
{
    if(!pV) {
	   rec=firstrec-1;
	   fr_S.id=0;
	   fr_S.pfx_id=0;
	   *fr_S.prefix=0;
	   if(typ_shp<TYP_SVG) ShpStatusMsg(counter=0);
	   bInitVectors=TRUE;
	   return TRUE;
	}

	if(!bInitVectors) return FALSE;

_loop:
	if(++rec>lastrec || dbNTP.Go(rec)) {
		if(typ_shp<TYP_SVG) ShpStatusMsg(counter);
		return bInitVectors=FALSE;
	}

	end_id=pPLT->end_id;
	
	if(!end_id || !fr_S.id || !pPLT->vec_id || (typ_shp!=TYP_SVGP && !pSV->LstInclude(pPLT->seg_id))
		/*|| (typ_shp>=TYP_SVG && VecIsVertical())*/) {
		//Save the data for this station in case next pt is a TO station --
		if(end_id) {
			GetPLTCoords(fr_S.xyz);
			GetStrFromFld(fr_S.label,pPLT->name,SHP_LEN_LABEL);
			if(fr_S.pfx_id!=pPLT->end_pfx) fr_S.pfx_id=CSegView::GetPrefix(fr_S.prefix,pPLT->end_pfx);
		}
		fr_S.id=end_id;
		goto _loop;		  
	}
	
	//A TO station for an included vector is found --

	pV->station[pPLT->vec_id&1]=fr_S;

	fr_S.id=end_id;
	GetPLTCoords(fr_S.xyz);
	GetStrFromFld(fr_S.label,pPLT->name,SHP_LEN_LABEL);
	if(fr_S.pfx_id!=pPLT->end_pfx) fr_S.pfx_id=CSegView::GetPrefix(fr_S.prefix,pPLT->end_pfx);
	pV->station[1-(pPLT->vec_id&1)]=fr_S;

	if(typ_shp==TYP_SVGP) return TRUE;


	*pV->date=*pV->filename=*pV->title=0;
	pV->seg_id=pPLT->seg_id;

	if((typ_shp<TYP_SVG || typ_shp==TYP_SVG && ((pSV->m_pSegments+pV->seg_id)->Style())->GradIdx()==2) &&
			!dbNTV.Go((pPLT->vec_id+1)/2)) {
		GetStrFromDateFld(pV->date,pVEC->date,"");
		if(typ_shp<TYP_SVG) {
			GetStrFromFld(pV->filename,pVEC->filename,SHP_LEN_FILENAME);
			CPrjListNode *pNode=CPrjDoc::m_pReviewNode->FindName(pV->filename);
			if(pNode) {
				pV->title[SHP_LEN_TITLE]=0;
				strncpy(pV->title,pNode->Title(),SHP_LEN_TITLE);
			}
		}
	}

	GetShpLineType(pV);

    if(pV->attributes) pSV->GetAttributeNames(pV->seg_id,pV->attributes,SHP_SIZ_BUF_ATTRIBUTES);
    if(pV->titles) pSV->GetParentTitles(pV->seg_id,pV->titles,SHP_SIZ_BUF_TITLES);

	if(typ_shp<TYP_SVG && !(++counter%100)) ShpStatusMsg(counter);
	return TRUE;
	
}

static COLORREF getFillColor(NTW_POLYGON *pPly)
{
	int flg=pPly->flgcnt;

	if(flg&(NTW_FLG_NOFILL|NTW_FLG_BACKGROUND)) return (COLORREF)-1;
	return (flg&NTW_FLG_FLOOR)?pSV->FloorColor():pPly->clr;
}

int CExportShp::GetPolygon(SHP_TYP_POLYGON *pP)
{
	//Return 0 or error code!
    if(!pP) {
	   //Determine polygon type --
	   bSVGFloors=pSV->HasSVGFloors();
	   bInitPolygons=TRUE;
	   if(bSVGFloors) {
			if(!CCompView::m_pNTWpnt && !LoadNTW()) return SHP_ERR_LOADNTW;
			rec=counter=0;
			lastprec=CCompView::m_NTWhdr.nPolyRecs-1;
			pPolyRec=(NTW_RECORD *)(CCompView::m_pNTWply+CCompView::m_NTWhdr.nPolygons);
			pPolyPnt=CCompView::m_pNTWpnt;
			return 0;
	   }
	   return -1;
	}

	if(!bInitPolygons) return SHP_ERR_VERSION;

	if(!bSVGFloors) {
		counter=0;
		bInitPolygons=FALSE;
		return WriteLrudPolygons((LRUDPOLY_CB)pP);
	}

	if(!rec) {
		pP->scale=CCompView::m_NTWhdr.scale;
		pP->datumE=CCompView::m_NTWhdr.datumE;
		pP->datumN=CCompView::m_NTWhdr.datumN;
		pP->pName=(SHP_MASKNAME *)(pPolyRec+CCompView::m_NTWhdr.nPolyRecs);
	}

	ShpStatusMsg(counter);

	if(rec++>lastprec) {
		bInitPolygons=FALSE;
		return SHP_ERR_FINISHED;
	}

	NTW_POLYGON *pPly=CCompView::m_pNTWply+pPolyRec->iPly;
	NTW_POLYGON *pEndPly=CCompView::m_pNTWply+CCompView::m_NTWhdr.nPolygons;

	pP->pPolygons=pPly;
	pP->clrFill=getFillColor(pPly); //make dependent on floor/background flags!
	pP->iGroupName=pPolyRec->iNam;
	pP->pPoints=pPolyPnt;
	pP->nPolygons=0;
	do {
		counter++;
		pP->nPolygons++;
		pPolyPnt+=(pPly->flgcnt&NTW_FLG_MASK);
		pPly++;
	}
	while(pPly!=pEndPly && !(pPly->flgcnt&NTW_FLG_OUTER));
	pPolyRec++;

	return 0;
	
}

void CExportShp::GetNTPRecs(void)
{
   long id;
   memset(pid,0,len_pid*sizeof(int));

   for(UINT rec=firstrec;rec<=lastrec;rec++) {
	   if(dbNTP.Go(rec)) break;
	   if(id=pPLT->end_id) if(!pid[id]) pid[id]=rec;
   }
}

BOOL CExportShp::GetFlagNameRecs()
{
	UINT max_cnt=pixSEGHDR->numFlagNames;

	free(pFlagNameRec);
	pFlagNameRec=NULL;

	if(max_cnt) {

		pFlagNameRec=(DWORD *)calloc(max_cnt,sizeof(DWORD));
		if(!pFlagNameRec || ixSEG.First()) return FALSE;

		UINT cnt=max_cnt;
		NTA_NOTETREE_REC nrec;

		do {
			VERIFY(!ixSEG.GetRec(&nrec,sizeof(nrec)));
			if(M_ISFLAGNAME(nrec.seg_id)) {
				cnt--;
				ASSERT((WORD)nrec.id<(WORD)max_cnt);
				pFlagNameRec[(WORD)nrec.id]=M_ISHIDDEN(nrec.seg_id)?0:ixSEG.Position();
			}
		}
		while(cnt && !ixSEG.Next());
		ASSERT(!cnt);
	}

	return !ixSEG.First();
}

BOOL CExportShp::SetFlagBits()
{
	NTA_NOTETREE_REC nrec;
	char buf[SHP_LEN_NOTE+10]; 

	if(!GetFlagNameRecs() || !AllocFlagBits()) return FALSE;
	GetNTPRecs();

	while(TRUE) {
		VERIFY(!ixSEG.GetRec(&nrec,sizeof(nrec)));
	    if(!M_ISFLAGNAME(nrec.seg_id) && pid[nrec.id]) {
	      if(pSV->LstInclude(nrec.seg_id&M_SEGMASK)) { 
		      UINT flags=(nrec.seg_id&~M_SEGMASK)>>M_NOTESHIFT;
			  if((flags&M_FLAGS) && !dbNTP.Go(pid[nrec.id]) && !ixSEG.GetKey(buf,SHP_LEN_NOTE+6)) {
				ASSERT((long)nrec.id==pPLT->end_id);
				if(*buf>6) {
					WORD flg_idx;
					if(pFlagNameRec && (flg_idx=*((WORD *)(buf+6)))!=(WORD)-1 && pFlagNameRec[flg_idx]!=0) {
						pFlagBits[nrec.id]|=FB_FLAGGED;
					}
				}
			  } //flags&M_FLAGS
		  } //LstInclude
		} //pid[nrec.id]

		if(ixSEG.Next()) break;
	}
	return TRUE;
}


BOOL CExportShp::GetNoteOrFlag(SHP_TYP_NOTE *pN,int typ)
{
	NTA_NOTETREE_REC nrec;
	BOOL bret=FALSE;
	char buf[SHP_LEN_NOTE+10]; 

    if(!pN) {
		next=0;
		ixSEG.UseTree(NTA_NOTETREE);
		if(!ixSEG.NumRecs()) {
			ixSEG.UseTree(NTA_SEGTREE);
			return FALSE;
		}
		if(typ==SHP_NOTES) {
			if(!pFlagBits && pSV->IsHideNotes()) {
				ASSERT(!pFlagNameRec);
				bInitNotes=SetFlagBits() && !ixSEG.First();
			}
			else {
				bInitNotes=!ixSEG.First();
				if(bInitNotes) GetNTPRecs();
			}
		}
		else {
			bInitNotes=GetFlagNameRecs();
			if(bInitNotes && pSV->IsHideNotes()) AllocFlagBits();
			if(bInitNotes) GetNTPRecs();
		}
		ixSEG.UseTree(NTA_SEGTREE);
	    return bInitNotes;
	}

	if(!bInitNotes) return FALSE;

	if(!next) {
		pN->station.pfx_id=0;
		*pN->station.prefix=0;
		next=1;
	}

	ixSEG.UseTree(NTA_NOTETREE);

	typ=(typ==SHP_NOTES)?M_NOTES:M_FLAGS;

	while(!bret) {
		VERIFY(!ixSEG.GetRec(&nrec,sizeof(nrec)));
	    if(!M_ISFLAGNAME(nrec.seg_id) && pid[nrec.id]) {
	      if(pSV->LstInclude(nrec.seg_id&M_SEGMASK)) { 
		      UINT flags=(nrec.seg_id&~M_SEGMASK)>>M_NOTESHIFT;
			  if((flags&typ) && !dbNTP.Go(pid[nrec.id]) && !ixSEG.GetKey(buf,SHP_LEN_NOTE+6)) {
				ASSERT((long)nrec.id==pPLT->end_id);
				pN->station.id=pPLT->end_id;
				GetPLTCoords(pN->station.xyz);
				GetStrFromFld(pN->station.label,pPLT->name,SHP_LEN_LABEL);
				if(pN->station.pfx_id!=pPLT->end_pfx) {
					//This will change default tree from NTA_NOTETREE to NTA_SEGTREE!
					pN->station.pfx_id=CSegView::GetPrefix(pN->station.prefix,pPLT->end_pfx);
					ixSEG.UseTree(NTA_NOTETREE);
				}
				
			    if(typ==M_NOTES) {
					if(!pFlagBits || (pFlagBits[nrec.id]&FB_FLAGGED)) {
						buf[(BYTE)*buf+1]=0;
						strcpy(pN->note,buf+6);
						ElimNL(pN->note);
						bret=TRUE;
					}
				}
				else if(*buf>6) {
					WORD flg_idx;
					
					if(pFlagNameRec && (flg_idx=*((WORD *)(buf+6)))!=(WORD)-1 &&
							pFlagNameRec[flg_idx]!=0) {
						DWORD cur_pos=ixSEG.Position(); //Save index file position
						if(!ixSEG.Go(pFlagNameRec[flg_idx])) {
							if(!ixSEG.GetKey(buf,SHP_LEN_NOTE+6)) {
								buf[(BYTE)*buf+1]=0;
								strcpy(pN->note,buf+1);
								if(pFlagBits) pFlagBits[nrec.id]|=FB_FLAGGED;
								bret=TRUE;
							}
						}
						#ifdef _DEBUG
							else ASSERT(FALSE);
						#endif
						VERIFY(!ixSEG.Go(cur_pos));
					}
				}

			  } //flags&typ
		  } //LstInclude
		} //pid[nrec.id]

		if(ixSEG.Next()) {
			bInitNotes=FALSE;
			break;
		}
	}
	ixSEG.UseTree(NTA_SEGTREE);
	return bret;
}

BOOL CExportShp::AllocFlagBits()
{
	UINT len=pixSEGHDR->numFlagNames;
	ASSERT(!pFlagBits);
	if(len) {
		if(len<len_pid) len=len_pid;
		pFlagBits=(BYTE *)calloc(len+1,sizeof(BYTE));
	}
	return pFlagBits!=NULL;
}

int CExportShp::GetFlagStyle(SHP_TYP_FLAGSTYLE *pFS)
{
	int iret=0;
	NTA_FLAGTREE_REC frec;
	char buf[SHP_LEN_FLAGNAME+10];
	
	if(!pFS) {
		ixSEG.UseTree(NTA_NOTETREE);
		if((next=pixSEGHDR->numFlagNames)>0 && ixSEG.First()) next=0;
		else AllocFlagBits();
		ixSEG.UseTree(NTA_SEGTREE);
		return next;
	}

	if(!next) return FALSE;

	ixSEG.UseTree(NTA_NOTETREE);
	while(!iret) {
	   if(ixSEG.GetRec(&frec,sizeof(NTA_FLAGTREE_REC))) {
		   ASSERT(FALSE);
		   next=0;
	   }
	   else if(M_ISFLAGNAME(frec.seg_id)) {
		 if(pFlagBits && M_ISHIDDEN(frec.seg_id)) pFlagBits[frec.id]|=FB_HIDDEN;
		 else if(!ixSEG.GetKey(buf,SHP_LEN_FLAGNAME+6)) {
			pFS->id=frec.id;
			pFS->wPriority=frec.iseq;
			if((pFS->wSize=M_SYMBOLSIZE(frec.seg_id))==M_SIZEMASK) 
				pFS->wSize=(WORD)CPlotView::m_mfPrint.iFlagSize;
			if((pFS->bShape=(BYTE)M_SYMBOLSHAPE(frec.seg_id))>=M_NUMSHAPES)
				pFS->bShape=(BYTE)CPlotView::m_mfPrint.iFlagStyle;
			pFS->bShade=(BYTE)M_SYMBOLSHADE(frec.seg_id);
			//pFS->wHidden=M_ISHIDDEN(frec.seg_id);
			pFS->color=frec.color;
			buf[(BYTE)*buf+1]=0;
			strcpy(pFS->flagname,buf+1);
			iret=1;
		 }
		 next--;
	   }
	   if(next && ixSEG.Next()) {
		  next=0;
		  break;
	   }
	}
	ixSEG.UseTree(NTA_SEGTREE);
	return iret;
}

int CExportShp::GetNoteLocation(SHP_TYP_NOTELOCATION *pN)
{
	NTA_NOTETREE_REC nrec;
	BOOL bret=FALSE;
	char buf[SHP_LEN_NOTE+10]; 

    if(!pN) {
		ixSEG.UseTree(NTA_NOTETREE);
		if(!ixSEG.NumRecs() || ixSEG.First()) next=0;
		else {
			GetNTPRecs();
			next=-1;
		}
		ixSEG.UseTree(NTA_SEGTREE);
		return -next;
	}

	if(!next) return FALSE;

	if(next<0) {
		pN->pfx_id=0;
		*pN->prefix=0;
		next=1;
	}

	ixSEG.UseTree(NTA_NOTETREE);

	while(!bret) {
		VERIFY(!ixSEG.GetRec(&nrec,sizeof(nrec)));
		if(M_ISNOTE(nrec.seg_id)) {
			if(pid[nrec.id] &&
				(!pSV->IsHideNotes() || !pFlagBits || (pFlagBits[nrec.id]&FB_FLAGGED))) {
			  if(pSV->LstInclude(nrec.seg_id&M_SEGMASK)) { 
				  if(!dbNTP.Go(pid[nrec.id]) && !ixSEG.GetKey(buf,SHP_LEN_NOTE+6)) {
					ASSERT((long)nrec.id==pPLT->end_id);
					GetPLTCoords(pN->xyz);
					buf[(BYTE)*buf+1]=0;
					strcpy(pN->note,buf+6);
					ElimNL(pN->note);
					GetStrFromFld(pN->label,pPLT->name,SHP_LEN_LABEL);
					if(pN->pfx_id!=pPLT->end_pfx) {
						//This will change default tree from NTA_NOTETREE to NTA_SEGTREE!
						pN->pfx_id=CSegView::GetPrefix(pN->prefix,pPLT->end_pfx);
						ixSEG.UseTree(NTA_NOTETREE);
					}
					bret=TRUE;
				  }
			  } //LstInclude
			}
		} //pid[nrec.id]

		if(ixSEG.Next()) {
			next=0;
			break;
		}
	}
	ixSEG.UseTree(NTA_SEGTREE);
	return bret;
}

int CExportShp::GetFlagLocation(SHP_TYP_FLAGLOCATION *pF)
{
	NTA_NOTETREE_REC nrec;
	BOOL bret=FALSE;
	char buf[16];

    if(!pF) {
		ixSEG.UseTree(NTA_NOTETREE);
		if((next=pixSEGHDR->numFlagNames)<=0 || ixSEG.First()) next=0;
		else {
			GetNTPRecs();
			next-=(int)ixSEG.NumRecs();
		}
		ixSEG.UseTree(NTA_SEGTREE);
		return -next;
	}

	if(!next) return FALSE;

	if(next<0) {
		pF->pfx_id=0;
		*pF->prefix=0;
		next=1;
	}

	ixSEG.UseTree(NTA_NOTETREE);

	while(!bret) {
		VERIFY(!ixSEG.GetRec(&nrec,sizeof(nrec)));
		if(M_ISFLAG(nrec.seg_id)) {
			if(pid[nrec.id]) {
			  if(pSV->LstInclude(nrec.seg_id&M_SEGMASK)) { 
				  if(!dbNTP.Go(pid[nrec.id]) && !ixSEG.GetKey(buf,16) && *buf>6) {
					ASSERT((long)nrec.id==pPLT->end_id);
					GetPLTCoords(pF->xyz);
					pF->flag_id=(UINT)*(WORD *)(buf+6);
					if(!pFlagBits || !(pFlagBits[pF->flag_id]&FB_HIDDEN)) {
						GetStrFromFld(pF->label,pPLT->name,SHP_LEN_LABEL);
						if(pF->pfx_id!=pPLT->end_pfx) {
							//This will change default tree from NTA_NOTETREE to NTA_SEGTREE!
							pF->pfx_id=CSegView::GetPrefix(pF->prefix,pPLT->end_pfx);
							ixSEG.UseTree(NTA_NOTETREE);
						}
						if(pFlagBits) pFlagBits[nrec.id]|=FB_FLAGGED;
						bret=TRUE;
					}
				  }
			  } //LstInclude
			}
		} //pid[nrec.id]

		if(ixSEG.Next()) {
			next=0;
			break;
		}
	}
	ixSEG.UseTree(NTA_SEGTREE);
	return bret;
}

void CExportShp::ShpStatusMsg(int counter)
{
	((CExpavDlg *)(pDlg))->StatusMsg(counter);
}

void CExportShp::IncPolyCnt()
{
	if(!(++counter%100)) ShpStatusMsg(counter);
}

void CExportShp::SvgStatusMsg(char *msg)
{
	if(typ_shp==TYP_SVG) ((CExpSvgDlg *)(pDlg))->StatusMsg(msg);
	else if(typ_shp==TYP_SVGP) ((CSvgProcDlg *)(pDlg))->StatusMsg(msg);
}

static int PASCAL GetVecData_CB(int typ,LPVOID pData)
{
	if(typ==SHP_VERSION) {
		bVersionNotChecked=FALSE;
		if(typ_shp<TYP_SVG) {
			static CString csPrj;
			ASSERT(pData);
			if(pData!=NULL && CPrjDoc::m_pReviewNode->InheritedState(FLG_GRIDMASK)) {
				PRJREF *pr=CPrjDoc::m_pReviewNode->GetAssignedRef();
				if(pr) {
					UINT id=0;
					if(!bUseMag && abs(pr->zone)>60) {
						id=(pr->zone>0)?IDS_PRJ_WGS84UPSN:IDS_PRJ_WGS84UPSS;
					}
					else {
						switch(pr->datum) {
							//only supported datums for now
							case 15 : id=bUseMag?IDS_PRJ_NAD27GEO:IDS_PRJ_NAD27UTM; break;
							case 19 : id=bUseMag?IDS_PRJ_NAD83GEO:IDS_PRJ_NAD83UTM; break;
							case 27 : id=bUseMag?IDS_PRJ_WGS84GEO:IDS_PRJ_WGS84UTM; break;
						}
					}
					if(id) {
						try {
							if(bUseMag || abs(pr->zone)>60) {
								csPrj.Format(id);
							}
							else {
								int iaZone=abs(pr->zone);
								csPrj.Format(id,iaZone,(pr->zone<0)?'S':'N',(pr->zone<0)?10000000:0,-183+iaZone*6);
							}
						}
						catch(...) {
							ASSERT(0);
						}
						*(LPCSTR *)pData=(LPCSTR)csPrj;
					}
				}
			}
			return EXP_SHP_VERSION;
		}
		*(SVG_VIEWFORMAT **)pData=pSvg;
		return EXP_SVG_VERSION;
	}

	if(bVersionNotChecked) {
		if(bVersionNotChecked==TRUE) {
			bVersionNotChecked++;
			switch(typ_shp) {
				case TYP_SHP: pData=EXP_SHPDLL_NAME; break;
#ifdef _USE_SHPX
				case TYP_CRF: pData=EXP_SHPXDLL_NAME; break;
#endif
				default: pData=EXP_SVGDLL_NAME; break;
			}
		    CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_IMPVERSION1,(char *)pData);
		}
		return FALSE;
	}

	if(!pData) pExpShp->shp_type=typ;

	switch(typ) {
	  case SHP_WALLS:
				//return 0 or error code --
	  			return pExpShp->GetPolygon((SHP_TYP_POLYGON *)pData);

	  case SHP_VECTORS:
	  			return pExpShp->GetVector((SHP_TYP_VECTOR *)pData);
	  
	  case SHP_STATIONS:
	  			return pExpShp->GetStation((SHP_TYP_STATION *)pData,TRUE);

	  case SHP_LRUDS:
				return pExpShp->GetLrud((SHP_TYP_LRUD *)pData);

	  case SHP_FLAGSTYLE:
				return pExpShp->GetFlagStyle((SHP_TYP_FLAGSTYLE *)pData);
	 			
	  case SHP_FLAGLOCATION:
				return pExpShp->GetFlagLocation((SHP_TYP_FLAGLOCATION *)pData);

	  case SHP_NOTELOCATION:
				return pExpShp->GetNoteLocation((SHP_TYP_NOTELOCATION *)pData);

	  case SHP_FLAGS:
	  case SHP_NOTES:
	  			return pExpShp->GetNoteOrFlag((SHP_TYP_NOTE *)pData,typ);

	  case SHP_STATUSMSG:
				pExpShp->SvgStatusMsg((char *)pData);
				return 0;

	  case SHP_UTM2LL:
		        CMainFrame::ConvertUTM2LL((double *)pData);
				return 0;

	  case SHP_ORPHAN1:
	  case SHP_ORPHAN2:
				return id_Orphans[typ-SHP_ORPHAN1].Add((LPCSTR)pData);
	}
	
	return FALSE;
} 

BOOL CSegView::ExportSHP(CExpavDlg *pDlg,LPCSTR pathname,LPCSTR basename,
						 CSegListNode *pNode,UINT flags)
{
	CExportShp ex;
	int e=1;
	HINSTANCE hinst=NULL;
	LPFN_SHP_EXPORT pfnExport=NULL;
	LPFN_ERRMSG pfnErrMsg=NULL;
	ex.pDlg=pDlg;

	ASSERT(!bUseMag);
	bUseMag=false;

#ifdef _USE_SHPX
	typ_shp=((flags&SHP_WALLSHPX)!=0)?TYP_CRF:TYP_SHP;
#else
	typ_shp=0;
#endif
		
	pExpShp=&ex;
	
	if(!ex.Init(dbNTV.MaxID()+1)) {
		CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_MEMORY);
		return FALSE;
	}

	//pSV->GetPrefixRecs();
	pSV->FlagVisible(pNode);

	//Load wallshp.DLL and wallmag.DLL if needed
	{
		if(!(flags&SHP_UTM)) {
			ASSERT(CPrjDoc::m_pReviewNode->GetAssignedRef());
			CMainFrame::PutRef(CPrjDoc::m_pReviewNode->GetAssignedRef());
			bUseMag=true;
		}
		UINT prevMode=SetErrorMode(SEM_NOOPENFILEERRORBOX);
#ifdef _USE_SHPX
		hinst=LoadLibrary(typ_shp?EXP_SHPXDLL_NAME:EXP_SHPDLL_NAME);
#else
		hinst=LoadLibrary(EXP_SHPDLL_NAME);
#endif
		SetErrorMode(prevMode);
	}
		    	    
	if(hinst) {
	  pfnErrMsg=(LPFN_ERRMSG)GetProcAddress(hinst,EXP_DLL_ERRMSG);
	  pfnExport=(LPFN_SHP_EXPORT)GetProcAddress(hinst,EXP_SHPDLL_EXPORT);
	}
		    	    
	if(pfnExport) {

	  bVersionNotChecked=TRUE;
	
	  BeginWaitCursor();
	  e=pfnExport(pathname,basename,flags,GetVecData_CB);
 	  EndWaitCursor();

	  if(e==0) {
		  int numsets=((flags&SHP_FLAGS)!=0)+
				((flags&SHP_NOTES)!=0)+
				((flags&SHP_VECTORS)!=0)+
				((flags&SHP_STATIONS)!=0)+
				((flags&SHP_WALLS)!=0);

		  CString msg;
#ifdef _USE_SHPX
		  msg.Format((!typ_shp)?IDS_SHP_NOTIFY3:IDS_SHPX_NOTIFY3,numsets,pathname,basename);
#else
		  msg.Format(IDS_SHP_NOTIFY3,numsets,(flags&SHP_UTM)?" UTM":"",pathname,basename);
#endif
		  if(flags&SHP_WALLS) {
			  msg+='\n';
			  msg+=pfnErrMsg(bUseMag?-1:SHP_GET_NTWSTATS);
		  }
		  AfxMessageBox(msg);
	  }
 	  else if(e>1) {
	    if(pfnErrMsg) {
	      CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_EXPTEXT1,pfnErrMsg(e));
	    }
	  }
	}
	else {
#ifdef _USE_SHPX
		CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_IMPLOADDLL1,typ_shp?EXP_SHPXDLL_NAME:EXP_SHPDLL_NAME);
#else
		CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_IMPLOADDLL1,EXP_SHPDLL_NAME);
#endif
	}

	if(hinst) FreeLibrary(hinst);
	bUseMag=false;

	pSV->UnFlagVisible(pNode);
	//free(pSV->pPrefixRecs);
	//pSV->pPrefixRecs=NULL;

#ifdef _USE_SHPX
	return (e==0)?(TRUE+typ_shp):0;
#else
	return (e==0);
#endif
}

static void CloseWallsSVG()
{
	if(SVGhinst) FreeLibrary(SVGhinst);
	//free(pSV->pPrefixRecs);
	//pSV->pPrefixRecs=NULL;
}

static int openWallsSVG(LPFN_SVG_EXPORT *pfnExp,LPFN_ERRMSG *pfnErr)
{
	SVGhinst=NULL;
	bUseMag=false;
	*pfnExp=NULL;

	if(!pExpShp->Init(dbNTV.MaxID()+1)) return IDS_ERR_MEMORY;
	//CSegView::GetPrefixRecs();

	//Load DLL --
	{
		UINT prevMode=::SetErrorMode(SEM_NOOPENFILEERRORBOX);
		SVGhinst=LoadLibrary(EXP_SVGDLL_NAME);
		::SetErrorMode(prevMode);
	}
		    	    
	if(SVGhinst) {
	  *pfnErr=(LPFN_ERRMSG)GetProcAddress(SVGhinst,EXP_DLL_ERRMSG);
	  if(*pfnErr) {
		  *pfnExp=(LPFN_SVG_EXPORT)GetProcAddress(SVGhinst,EXP_SVGDLL_EXPORT);
	  }
	}

	if(!*pfnExp) {
		CloseWallsSVG();
		return IDS_ERR_IMPLOADDLL1;
	}

	bVersionNotChecked=TRUE;
	return 0;
}

static void WriteOrphanVecs(const STRING_BUF &sb,LPCSTR svgPath,FILE *flog)
{
	fputs("\n",flog);
	fputs(svgPath,flog);
	LPCSTR pNext=sb.pbuf;
	for(UINT n=0;n<sb.cnt;n++) {
		fputs("\n",flog);
		fputs(pNext,flog);
		pNext+=strlen(pNext)+1;
	}
	if(sb.bOverflow) 
		fputs("\n... Export aborted",flog);
	fputs("\n====================\n",flog);
}

static LPCSTR GetOrphanErrorMsg(CString &s,const STRING_BUF &sb,LPCSTR path)
{
	LPCSTR pNext=sb.pbuf;
	s.Format("\n%s\nVector(s): %s",trx_Stpnam(path),pNext);
	UINT cnt=min(sb.cnt,3);
	for(UINT i=1;i<cnt;i++) {
		pNext+=strlen(pNext)+1;
		s+=", "; s+=pNext;
	}
	CString s0;
	s0.Format("%s (%u%s total)\n",(sb.cnt>3)?", . . .":"",sb.cnt,sb.bOverflow?"+":"");
	s+=s0;
	return s;
}

int CSegView::ExportSVG(CExpSvgDlg *pDlg,SVG_VIEWFORMAT *psvg)
{
	CExportShp ex;
	int e=0;
	LPFN_SVG_EXPORT pfnExport=NULL;
	LPFN_ERRMSG pfnErrMsg=NULL;

	//Init globals --
	typ_shp=TYP_SVG;
	pSvg=psvg;
	ex.pDlg=pDlg;
	pExpShp=&ex;

	if(e=openWallsSVG(&pfnExport,&pfnErrMsg)) {
		CMsgBox(MB_ICONEXCLAMATION,e,EXP_SVGDLL_NAME);
		return e;
	}
	
	pSV->m_pRoot->FlagVisible(LST_INCLUDE);

	if(psvg->flags&SVG_EXISTS) {
		//We've already determined that the file exists and is rewritable.
		//We need to partially parse this file to see if it can be overwritten
		//without prompting the user. If there are no errors, the DLL
		//will set the SVG_UPDATED and SVG_ADDITIONS flags accordingly
		//while clearing the SVG_EXISTS flag.
		if(!(e=pfnExport(psvg->flags,GetVecData_CB))) {
			//For now, this call will return 0 even if there is a parsing error.
			if(psvg->version!=EXP_SVG_VERSION || (psvg->flags&SVG_UPDATED)!=0) {
				//Simple overwrite warning for now, adjustment dialog later --
				if(IDOK!=CMsgBox(MB_OKCANCEL,IDS_SVG_EXISTDLG)) e=1;
			}
		}
	}

	if(!e) {
		BeginWaitCursor();

		id_Orphans[0].Clear(); id_Orphans[1].Clear();
		e=pfnExport(psvg->flags,GetVecData_CB);
		EndWaitCursor();

		UINT nOrphans=id_Orphans[0].cnt+id_Orphans[1].cnt;
		if(nOrphans && (!e || e==SVG_ERR_VECNOTFOUND)) {
			CString s,s0;
			if(!e) {
				s.Format("CAUTION: The merged export was successful but the source SVG(s) contained %u "
					"vector ids not represented in the compiled survey branch. You'll want to "
					"review these areas of the map:\n",nOrphans);
			}
			else {
				s.Format("ERROR: The export was aborted because a source SVG contained more than %u"
					" vector ids not represented in the compiled survey branch:\n",STRING_BUF::MAX_CNT);
			}
			if(id_Orphans[0].cnt) s+=GetOrphanErrorMsg(s0,id_Orphans[0],pSvg->mrgpath);
			if(id_Orphans[1].cnt) s+=GetOrphanErrorMsg(s0,id_Orphans[1],pSvg->mrgpath2);

			if(IDYES==CMsgBox((MB_YESNO|(e?MB_ICONEXCLAMATION:MB_ICONINFORMATION)),"%s\nWould you like to generate and open an error log?",(LPCSTR)s)) {
				LPCSTR logpath=GetDocument()->LogPath();
				_unlink(logpath);
				FILE *flog=fopen(logpath,"w");
				if(flog) {
					fputs("SVG vectors not found in compilation of ",flog);
					fputs(CPrjDoc::m_pReviewNode->Title(),flog);
					fputs("\n",flog);
					if(id_Orphans[0].cnt) WriteOrphanVecs(id_Orphans[0],pSvg->mrgpath,flog);
					if(id_Orphans[1].cnt) WriteOrphanVecs(id_Orphans[1],pSvg->mrgpath2,flog);
					fclose(flog);
					CMainFrame::CloseLineDoc(logpath,TRUE); 
					GetDocument()->LaunchFile(CPrjDoc::m_pReviewNode,CPrjDoc::TYP_LOG);
				}
			}
		}
		if(nOrphans) {
			id_Orphans[0].Clear();
			id_Orphans[1].Clear();
		}
	}

 	if(e && e!=1 && e!=SVG_ERR_VECNOTFOUND) {
		CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_EXPTEXT1,pfnErrMsg(e));
	}

	CloseWallsSVG();

	pSV->m_pRoot->FlagVisible(~LST_INCLUDE);

	return e;
}


void CPrjDoc::ProcessSVG()
{
	//See if an SVG is attached as a descendant item. If so, call the SVG DLL to
	//generate an NTW workfile. The NTW file consists of concatenated arrays of binary
	//data representing the w2d_Mask layer. If successful, flag the NTA to indicate a valid NTW is present.

	CPrjListNode *pSN;

	if(!m_pReviewNode->IsProcessingSVG() || !(pSN=m_pReviewNode->FindFirstSVG())) return;

	if(!theApp.m_bIsVersionNT) {
		//AfxMessageBox(IDS_ERR_NTWNEEDSNT);
		return;
	}

	LPFN_SVG_EXPORT pfnExport;
	LPFN_ERRMSG pfnErrMsg;
	CExportShp ex;
	SVG_VIEWFORMAT svg;

	//Init globals --
	typ_shp=TYP_SVGP;
	pSvg=&svg;
	pExpShp=&ex;

	int e=openWallsSVG(&pfnExport,&pfnErrMsg);

	if(e) {
		//either memory problem or DLL issue
		CMsgBox(MB_ICONEXCLAMATION,e,EXP_SVGDLL_NAME);
		return;
	}

	CString svgpath=SurveyPath(pSN);

	memset(&svg,0,sizeof(svg));
	svg.flags=SVG_WRITENTW;
	svg.svgpath=WorkPath(m_pReviewNode,TYP_NTW); //We'll write NTW, not SVG
	svg.mrgpath=svgpath;
	svg.mrgpath2=NULL;
	CSvgAdv::LoadAdvParams(&svg.advParams,TRUE);

	CSvgProcDlg dlg(pfnExport,pfnErrMsg,GetVecData_CB);
	ex.pDlg=&dlg;

	e=dlg.DoModal(); //Shows progress message and pauses only upon failure.

	if(e==IDCANCEL || !dlg.m_pfnExp) {
		//EndDialog(IDCANCEL) was called after a successful NTW write --
		pixSEGHDR->style[HDR_FLOOR].SetSVGAvail(TRUE);
		ixSEG.MarkExtra();
	}
	//else error message was shown, OK button was enabled, and user pressed it.

	CloseWallsSVG();
}
    

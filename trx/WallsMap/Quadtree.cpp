#include "stdafx.h"
#include "WallsMapDoc.h"
#include "Quadtree.h"

UINT QTNODE::insrec=0; //current ext[] index to insert
UINT QTNODE::maxLvl=QT_DFLT_DEPTH;
CFltPoint QTNODE::findpt;
CQuadTree *QTNODE::pTree=NULL;
CFltRect *QTNODE::pExt=NULL;
#ifdef _USE_QTFLG
const CFltRect *QTNODE::pFrm=NULL;
BYTE *QTNODE::pFlags=NULL;
#endif
QTNODE *QTNODE::pQTN=NULL;
DWORD *QTNODE::piQT=NULL;

void QTNODE::AllocRec(CFltRect extN) //construct tree and init nrecs
{
	CFltPoint fpt(extN.Center());
	if(lvl==maxLvl) {
		nrecs++;
		pTree->m_numParts++;
    }
	else {
		BYTE qa,qb;
		if(fpt.x<pExt->l) {
			//tr and/or br
			qa=2; qb=4; 
			if(fpt.y>pExt->t) qa=0;
			else if(fpt.y<=pExt->b) qb=0;
		}
		else if(fpt.x>=pExt->r) {
		    //tl and/or bl
			qa=1; qb=3;
			if(fpt.y>pExt->t) qa=0;
			else if(fpt.y<=pExt->b) qb=0;
		}
		else if(fpt.y>pExt->t) {
			//bl and/or br
			qa=3; qb=4;
			if(fpt.x<pExt->l) qa=0;
			else if(fpt.x>=pExt->r) qb=0;
	    }
		else if(fpt.y<=pExt->b) {
			//tl and/or tr
			qa=1; qb=2;
			if(fpt.x<pExt->l) qa=0;
			else if(fpt.x>=pExt->r) qb=0;
		}
		else {
			ASSERT(pExt->IsPtInside(fpt));
			nrecs++;
			pTree->m_numParts++;
			return;
		}
		if(qa) AllocNodeRec(qa,extN,fpt);
		if(qb) AllocNodeRec(qb,extN,fpt);
	}
}

void QTNODE::AllocNodeRec(BYTE q, CFltRect extN, const CFltPoint &fpt)
{
	switch(q) {
		case 1 : extN.r=fpt.x; extN.b=fpt.y; break;
		case 2 : extN.l=fpt.x; extN.b=fpt.y; break;
		case 3 : extN.r=fpt.x; extN.t=fpt.y; break;
		case 4 : extN.l=fpt.x; extN.t=fpt.y;
	}
	if(!quad[q-1]) {
		ASSERT(pTree->m_numNodes<pTree->m_maxNodes);
		pQTN[quad[q-1]=pTree->m_numNodes++].lvl=lvl+1;
	}
	pQTN[quad[q-1]].AllocRec(extN);
}

void QTNODE::InsertRec(CFltRect extN) //insert rec nos in allocated array.
{
	CFltPoint fpt(extN.Center());
	if(lvl==maxLvl) {
		piQT[fstrec+nrecs++]=insrec;
    }
	else {
		BYTE qa,qb;
		if(fpt.x<pExt->l) {
			//tr and/or br
			qa=2; qb=4; 
			if(fpt.y>pExt->t) qa=0;
			else if(fpt.y<=pExt->b) qb=0;
		}
		else if(fpt.x>=pExt->r) {
		    //tl and/or bl
			qa=1; qb=3;
			if(fpt.y>pExt->t) qa=0;
			else if(fpt.y<=pExt->b) qb=0;
		}
		else if(fpt.y>pExt->t) {
			//bl and/or br
			qa=3; qb=4;
			if(fpt.x<pExt->l) qa=0;
			else if(fpt.x>=pExt->r) qb=0;
	    }
		else if(fpt.y<=pExt->b) {
			//tl and/or tr
			qa=1; qb=2;
			if(fpt.x<pExt->l) qa=0;
			else if(fpt.x>=pExt->r) qb=0;
		}
		else {
			ASSERT(pExt->IsPtInside(fpt));
			piQT[fstrec+nrecs++]=insrec;
			return;
		}
		if(qa) InsertNodeRec(qa,extN,fpt);
		if(qb) InsertNodeRec(qb,extN,fpt);
	}
}

void QTNODE::InsertNodeRec(BYTE q, CFltRect extN, const CFltPoint &fpt)
{
	switch(q) {
		case 1 : extN.r=fpt.x; extN.b=fpt.y; break;
		case 2 : extN.l=fpt.x; extN.b=fpt.y; break;
		case 3 : extN.r=fpt.x; extN.t=fpt.y; break;
		case 4 : extN.l=fpt.x; extN.t=fpt.y;
	}
	ASSERT(quad[q-1]);
	pQTN[quad[q-1]].InsertRec(extN);
}

#ifdef _USE_QTFLG
void QTNODE::InitFlags(CFltRect extN)
{
	//if lvl!=maxLvl, this node contains only the indexes of poly extents that contain the node's center pt,
	//else it contains the records of all poly extents overlapping the node's quadrant.
	//So, when lvl!=maxLvl we only need to test if the node's center pt is in the frame to conlude
	//that all extents overlap the frame. Otherwise we need to test all extents for frame overlap.

	DWORD *p0=piQT+fstrec;
	DWORD *p1=p0+nrecs;

	CFltPoint fpt(extN.Center());

	if(lvl==maxLvl || !pFrm->IsPtInside(fpt)) {
		//By far the most common case for shapefiles resembling a grid --
		for(;p0<p1;p0++) {
			if(!pFlags[*p0] && !pFrm->IsRectOutside(pExt[*p0])) {
				pFlags[*p0]=1;
			}
		}
		if(lvl==maxLvl) return;
	}
	else {
		//All extents in this node contain the quad's ctr, so must be in the frame since the frame contains the center --
		for(;p0<p1;p0++) {
			ASSERT(pExt[*p0].IsPtInside(fpt));
			pFlags[*p0]=1;
		}
	}

	//There may be extents in subnodes --
	for(int i=0; i<4; i++)
		if(quad[i]) InitNodeFlags(i, extN, fpt);
}

void QTNODE::InitNodeFlags(int i, CFltRect extN, const CFltPoint &fpt)
{
	switch(i) {
		case 0 : extN.r=fpt.x; extN.b=fpt.y; break;
		case 1 : extN.l=fpt.x; extN.b=fpt.y; break;
		case 2 : extN.r=fpt.x; extN.t=fpt.y; break;
		case 3 : extN.l=fpt.x; extN.t=fpt.y;
	}
	if(pFrm->IsRectOutside(extN)) {
		ASSERT(extN.l>=pFrm->r || extN.r<pFrm->l || extN.t<=pFrm->b || extN.b>pFrm->t);
		return; //frame does not overlap this subnode's area
	}
	pQTN[quad[i]].InitFlags(extN);
}
#endif

DWORD QTNODE::GetPolyRecContainingPt(CFltRect ext)
{
	ASSERT(ext.IsPtInside(findpt));
	if(lvl<maxLvl) {
		CFltPoint fpt(ext.Center());

		BYTE qi;
		if(findpt.x<fpt.x) {
			ext.r=fpt.x;
			if(findpt.y<fpt.y) {
				ext.t=fpt.y;
				qi=2; //bl
			}
			else {
				ext.b=fpt.y;
				qi=0; //tl;
			}
		}
		else {
			ext.l=fpt.x;
			if(findpt.y<fpt.y) {
				ext.t=fpt.y;
				qi=3; //br
			}
			else {
				ext.b=fpt.y;
				qi=1; //tr;
			}
		}
		if(quad[qi]) {
			DWORD rec=pQTN[quad[qi]].GetPolyRecContainingPt(ext);
			if(rec)	return rec;
		}
		//pt could be in an extent covering this quad's center
	}

	for(DWORD i=fstrec; i<fstrec+nrecs; i++) {
		pTree->m_tests++;
		DWORD rec=piQT[i];
		if(pExt[rec].IsPtInside(findpt)) {
			pTree->m_polyTests++;
			if(pTree->m_pShp->ShpPolyTest(findpt,++rec))
				return rec;
		}
	}
	return 0;
}

//============================================================

CQuadTree::~CQuadTree()
{
#ifdef _USE_QTFLG
	if(m_pFlags) free(m_pFlags);
#endif
	if(m_piQT) free(m_piQT);
	if(m_pQTN) free(m_pQTN);
}

#ifdef _USE_QTFLG
BYTE *CQuadTree::InitFlags(const CFltRect &geoExt)
{
	if(!m_pFlags) m_pFlags=(BYTE *)calloc(m_pShp->m_nNumRecs, 1);
	else memset(m_pFlags, 0, m_pShp->m_nNumRecs);
	if(geoExt.IsRectOutside(m_extent)) {
		ASSERT(0);
		return m_pFlags;
	}
	if(m_pFlags) {
		QTNODE::maxLvl=m_maxLvl;
		QTNODE::pTree=this;
		QTNODE::pFlags=m_pFlags;
		QTNODE::pQTN=m_pQTN;
		QTNODE::piQT=m_piQT;
		QTNODE::pExt=&m_pShp->m_ext[0];
		QTNODE::pFrm=&geoExt;
		m_pQTN[0].InitFlags(m_extent);
	}
	return m_pFlags;
}
#endif

DWORD CQuadTree::GetPolyRecContainingPt(const CFltPoint &fpt)
{
	if(!m_extent.IsPtInside(fpt)) return 0;
	m_tests=m_polyTests=0;
	QTNODE::piQT=m_piQT;
	QTNODE::pQTN=m_pQTN;
	QTNODE::maxLvl=m_maxLvl;
	QTNODE::pTree=this;
	QTNODE::findpt=fpt;
	QTNODE::pExt=&m_pShp->m_ext[0];
	return m_pQTN[0].GetPolyRecContainingPt(m_extent);
}

int CQuadTree::Create(CShpLayer *pShp)
{
	ASSERT(!m_pQTN);
	QTNODE::pTree=this;
	m_pShp=pShp;
	m_extent=pShp->Extent();
	DWORD nPolyRecs=pShp->m_nNumRecs;
	#ifdef _DEBUG
	UINT maxNodeRecs=0;
	#endif
	ASSERT(nPolyRecs==pShp->m_ext.size());
	QTNODE::maxLvl=m_maxLvl=pShp->m_bQTdepth;
	DWORD minNodeCap=(DWORD)pow(2,m_maxLvl);
	if(!(m_maxNodes=pShp->m_uQTnodes)) m_maxNodes=500;
	else m_maxNodes+=minNodeCap;

	try {
_retry:
		m_pQTN=(QTNODE *)calloc(m_maxNodes,sizeof(QTNODE));
		if(!m_pQTN) throw 0;
		QTNODE::pQTN=m_pQTN;
		m_numNodes=1;
		m_numParts=0;
		SHP_POLY_DATA *pp,*ppoly=pShp->m_pdbfile->ppoly;
		CFltRect *pe,*pext=&m_pShp->m_ext[0];
		pp=ppoly;
		pe=pext;
		for(DWORD r=0;r<nPolyRecs;r++,pp++,pe++) {
			if(pp->IsDeleted()) continue;
			if(m_maxNodes-m_numNodes<minNodeCap) {
				free(m_pQTN);
				m_maxNodes+=(m_maxNodes/2);
				goto _retry;
			}
			QTNODE::pExt=pe;
			m_pQTN[0].AllocRec(m_extent);
		}
		//alloc space for record nos grouped by node --
		m_piQT=(DWORD *)malloc(m_numParts*sizeof(DWORD));
		if(!m_piQT) throw 0;
		QTNODE::piQT=m_piQT;
		QTNODE *pQT=m_pQTN;
		DWORD nRecs=0;
		for(UINT n=0;n<m_numNodes;n++,pQT++) {
			pQT->fstrec=nRecs;
			nRecs+=pQT->nrecs;
			#ifdef _DEBUG
			if(maxNodeRecs<pQT->nrecs) maxNodeRecs=pQT->nrecs;
			#endif
			pQT->nrecs=0;
		}
		ASSERT(nRecs==m_numParts);
		pp=ppoly;
		pe=pext;
		for(DWORD r=0;r<nPolyRecs;r++,pp++,pe++) {
			if(pp->IsDeleted()) continue;
			QTNODE::pExt=pe;
			QTNODE::insrec=r;
			m_pQTN[0].InsertRec(m_extent);
		}
	}
	catch(...) {
		CMsgBox("Failed to create spatial index for %s.",pShp->FileName());
		return 0;
	}
	pShp->m_uQTnodes=m_numNodes;
	return 1;
}

#ifdef QT_TEST
void CQuadTree::QT_TestPoints(LPCSTR ptShpName)
{
	UINT nRecs,nFound=0,nMissed=0;
	int nBadHits=-1;
	CShpLayer *pShp=m_pShp->m_pDoc->FindShpLayer(ptShpName,0,(UINT *)1);
	if(!pShp || !(nRecs=pShp->m_nNumRecs) || !m_pShp->m_nNumRecs ) return;
	UINT nTests=0,nPolyTests=0;
	AfxGetMainWnd()->BeginWaitCursor();

	START_TIMER();
	for(DWORD i=0;i<nRecs; i++) {
		if(pShp->IsPtRecDeleted(i+1)) continue;

		DWORD rec=GetPolyRecContainingPt(pShp->m_fpt[i]);
		nTests+=m_tests;
		nPolyTests+=m_polyTests;
		nFound+=(rec!=0);
	
		#ifdef _DEBUG
		if(rec) {
            #ifdef _CNT_WRONG
			CString spoly,sname,scnty;
			pShp->m_pdb->Go(i+1);
			pShp->m_pdb->GetTrimmedFldStr(scnty,3); //assumes county fld is 3 in pt shapefile
			m_pdb->Go(rec); m_pdb->GetTrimmedFldStr(spoly,1);
			if(spoly.Compare(scnty)) {
				pShp->m_pdb->GetTrimmedFldStr(sname,53); //TSSID
				if(nBadHits<0) nBadHits++;
				nBadHits++;
			}
		    #endif
		}
		else {
			CFltPoint latlon(pShp->m_fpt[i]);
			if(pShp->m_iZone) {
				geo_UTM2LatLon(latlon,pShp->m_iZone,geo_WGS84_datum);
			}
			if(!pShp->IsNotLocated(latlon)) {
				CString sname;
				pShp->m_pdb->Go(i+1);
				pShp->m_pdb->GetTrimmedFldStr(sname, 53); //TSSID
				nMissed++;
			}
		}
		#else
		nMissed+=(rec==0);
		#endif
	}
	STOP_TIMER();
	AfxGetMainWnd()->EndWaitCursor();
	
	CMsgBox("Layer %s (%u recs) tested against %s (%u recs) --\n\nTime=%.2f, Tree_depth=%u, Range_tests/pt=%.2f, Poly_tests/pt=%.2f, Hits=%u, Misses=%u, "
		"Bad_hits=%d, Nodes=%u of %u, Parts/poly=%.2f, NodeSpc=%uK of %uK, PtSpc=%uK",
		pShp->FileName(), nRecs, m_pShp->FileName(),m_pShp->m_nNumRecs, TIMER_SECS(), m_maxLvl,
		(double)nTests/nRecs, (double)nPolyTests/nRecs, nFound, nMissed, nBadHits,
		m_numNodes,m_maxNodes,(double)m_numParts/m_pShp->m_nNumRecs,(m_numNodes*28+512)/1024,
		(m_maxNodes*28+512)/1024,(m_numParts*4+512)/1024);
}
#endif


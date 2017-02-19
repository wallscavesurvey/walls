//CMapLayer fcns --

#include "stdafx.h"
#include "WallsMap.h"
#include "Mainfrm.h"
#include "WallsMapDoc.h"
#include "WallsMapView.h"
#include "MapLayer.h"
#include "ShowShapeDlg.h"
#include <ogr_spatialref.h>

int CTreeLayer::LayerType() const {return TYP_FOLDER;}
LPCSTR CTreeLayer::GetInfo() {return "";}
CTreeLayer::~CTreeLayer() {};

LPCSTR CTreeLayer::LayerPath(CString &path)
{
	path=Title();
	PTL p=parent;
	while(p) {
		path.Insert(0,'/');
		path.Insert(0,p->Title());
		p=p->parent;
	}
	return (LPCSTR)path;
}

bool CTreeLayer::SetVisible(bool bVisible)
{
	if(!bVisible)
		m_uFlags&=~NTL_FLG_VISIBLE;
	else m_uFlags|=NTL_FLG_VISIBLE;
	return false; //by itself will not require map refresh
}
//========================================================

CMapLayer::~CMapLayer()
{
	ASSERT(!m_datumName && !m_unitsName && !m_projName && !m_projectionRef && !m_pathName);
}

void CMapLayer::FreeDupedStrings()
{
	//Called by ~CShpLayer() and ~CGdalLayer() but not ~CNtiLayer() --
	free((LPSTR)m_pathName); m_pathName=NULL;
	free((LPSTR)m_projectionRef); m_projectionRef=NULL;
	free((LPSTR)m_projName); m_projName=NULL;
	free((LPSTR)m_datumName); m_datumName=NULL;
	free((LPSTR)m_unitsName); m_unitsName=NULL;
}

LPCSTR CMapLayer::GetInfo()
{
	if(m_csInfo.IsEmpty()) AppendInfo(m_csInfo);
	return (LPCSTR)m_csInfo;
}

BOOL CMapLayer::IsLatLongExtent()
{
	return fabs(m_extent.tl.x)<181 && fabs(m_extent.br.x)<181 &&
					fabs(m_extent.tl.y)<91 && fabs(m_extent.br.y)<91;
}

LPCSTR CMapLayer::GetShortCoordSystemStr(int iNad,int iZone)
{
	LPCSTR p;
	if(iZone) {
		if(abs(iZone)<=60)
			p=String128("UTM %d%c",abs(iZone),(iZone<0)?'S':'N');
		else
			p="Unk Proj";
	}
	else {
		p="Degrees";
	}

	return String128("%s, %s",p,(iNad==2)?"NAD27":((iNad==1)?"NAD83":"Unk Datum"));
}

LPSTR CMapLayer::GetCoordSystemStr() const
{
	LPCSTR p;
	if(m_iZone) {
		if(abs(m_iZone)<=60)
			p=String128("UTM %d%c",abs(m_iZone),(m_iZone<0)?'S':'N');
		else if(m_projName) 
			p=m_projName;
		else p="Unknown projection";
	}
	else {
		ASSERT(!m_bProjected);
		p="Geographic (Lat/Long)";
	}
	return String128("%s, Datum: %s",p,(m_iNad==2)?"NAD27":((m_iNad==1)?(m_bWGS84?"WGS84":"NAD83"):
		(m_iNad?(m_datumName?m_datumName:"Unsupported"):"Unknown")));
}

/*
LPSTR CMapLayer::GetKnownCoordSystemStr() const
{
	char buf[12];
	LPCSTR p=buf;
	if(m_iZone) {
		ASSERT(abs(m_iZone)<=60);
		_snprintf(buf,12,"UTM %d%c",abs(m_iZone),(m_iZone<0)?'S':'N');
		buf[11]=0;
	}
	else {
		ASSERT(!m_bProjected);
		p="Lat/Long";
	}
	ASSERT(m_iNad==2 || m_iNad==1);
	return String128("%s, %s",p,(m_iNad==2)?"NAD27":(m_bWGS84?"WGS84":"NAD83"));
}
*/

static LPSTR AddCR(LPSTR pSrc)
{
	LPSTR p;
	int nEOL=0;

	for(p=pSrc;*p;p++) if(*p=='\n') nEOL++;
	if(!nEOL) return pSrc;
	LPSTR pDst=(LPSTR)malloc(strlen(pSrc)+nEOL+1);
	if(!pDst) return pSrc;
	LPSTR pD=pDst;
	for(p=pSrc;*p;p++) {
		if(*p=='\n') *pD++='\r';
		*pD++=*p;
	}
	*pD=0;
	free(pSrc);
	return pDst;
}

bool CMapLayer::AppendProjectionRefInfo(CString &s) const
{
	if(m_projectionRef) {
		LPSTR p=(LPSTR)m_projectionRef;
		OGRSpatialReference oSRS;
		if(oSRS.importFromWkt(&p)==OGRERR_NONE && oSRS.exportToPrettyWkt(&p,FALSE)==OGRERR_NONE) {
			p=AddCR(p);
			s.AppendFormat("\r\nProjection details --\r\n%s\r\n",p);
			free(p);
			return true;
		}
	}
	return false;
}

void CMapLayer::ComputeInverseTransform()
{
	double d=1.0/(m_fTransImg[1]*m_fTransImg[5]-m_fTransImg[2]*m_fTransImg[4]);
	m_fTransGeo[1]=d*m_fTransImg[5];
	m_fTransGeo[2]=-d*m_fTransImg[2];
	m_fTransGeo[4]=-d*m_fTransImg[4];
	m_fTransGeo[5]=d*m_fTransImg[1];
	m_fTransGeo[0]=-m_fTransImg[0]*m_fTransGeo[1]-m_fTransImg[3]*m_fTransGeo[2];
	m_fTransGeo[3]=-m_fTransImg[0]*m_fTransGeo[4]-m_fTransImg[3]*m_fTransGeo[5];
}

bool CMapLayer::IsSyncCompatible(const CLayerSet *pLSet)
{

	if(!m_bTrans || !pLSet->m_bTrans)
		return m_bTrans==pLSet->m_bTrans;

	if(m_bProjected==pLSet->m_bProjected && m_iNad==pLSet->m_iNad && m_iZone==pLSet->m_iZone) return true;

	if(m_iNad>2 || pLSet->m_iNad>2 ||
			(m_bProjected && m_iNad && !m_iZone) || (pLSet->m_bProjected && pLSet->m_iNad && !pLSet->m_iZone) ||
			m_bProjected!=pLSet->m_bProjected && (m_bProjected && !m_iZone || pLSet->m_bProjected && !pLSet->m_iZone)
	) return false;

	return true;
}

void CMapLayer::DrawOutline(CDC *pDC,CFltRect &rectGeo,double fScale,CBrush *pBrush)
{
	CPoint tl(_rnd(fScale*(m_extent.l-rectGeo.l)),_rnd(fScale*(rectGeo.t-m_extent.t)));
	CSize sz(_rnd(fScale*m_extent.Width()),_rnd(fScale*m_extent.Height()));
	CRect rect(tl,sz);
	pDC->FrameRect(&rect,pBrush);
}

LPCSTR CMapLayer::GetPrefixedTitle(CString &cs) const
{
	if(LayerType()!=TYP_SHP || IsEditable())
		return m_csTitle;
	cs.Format("| %s",(LPCSTR)m_csTitle);
	return (LPCSTR)cs;
}

int CMapLayer::Visibility(DWORD rec /*=0*/)
{
	//Used to determine necessity of refresh after symbology or scale range change --
	//Return 0 if no part of layer is currently visible 
	//Return 1,2,or 3 if features/markers(1), labels(2), or both(3) are visible --

	POSITION pos=m_pDoc->GetFirstViewPosition();
	CWallsMapView *pView;
	int iret=0;
	CFltPoint *pFpt=NULL;
	if(rec) {
		ASSERT(LayerType()==TYP_SHP);
		//We won't be revising individual poly labels.
		if(((CShpLayer *)this)->ShpType()==CShpLayer::SHP_POINT)
			pFpt=&((CShpLayer *)this)->m_fpt[rec-1];
	}
	bool bFeaturesEnabled=(LayerType()!=TYP_SHP) || ((CShpLayer *)this)->ShpType()!=CShpLayer::SHP_POINT || 
		(m_uFlags&((m_pDoc->m_uEnableFlags&NTL_FLG_MARKERS)|NTL_FLG_SHOWMARKERS))!=0;

	bool bLabelsEnabled=(m_uFlags&((m_pDoc->m_uEnableFlags&NTL_FLG_LABELS)|NTL_FLG_SHOWLABELS))!=0;
	while(pView=(CWallsMapView *)m_pDoc->GetNextView(pos)) {
		CFltRect geoExt;
		pView->GetViewExtent(geoExt);
		if(pFpt) {
			if(!geoExt.IsPtInside(*pFpt)) continue;
		}
		else if(!ExtentOverlap(geoExt)) continue;
		UINT scaleDenom=CLayerSet::GetScaleDenom(geoExt,pView->GetScale(),m_pDoc->IsProjected());
		if(!LayerScaleRangeOK(scaleDenom)) continue;
		if(bFeaturesEnabled) iret|=1;
		if(bLabelsEnabled && !(iret&2) && LabelScaleRangeOK(scaleDenom)) iret|=2;
		if(iret==3) break;
	}
	return iret;
}

bool CMapLayer::SetVisible(bool bVisible)
{
	if(bVisible!=IsVisible()) {
		if(!bVisible) {
			m_uFlags&=~NTL_FLG_VISIBLE;
			#ifndef _USE_TM
			if(LayerType()==TYP_SHP) {
				CShpLayer *pShp=(CShpLayer *)this;
				if(pShp->ShpType()!=CShpLayer::SHP_POINT && !pShp->m_pdbfile->pDBGridDlg)
					//free a potentially large amnt of ram and address space
					pShp->UnInit_ppoly();
			}
			else ((CImageLayer *)this)->DeleteDIB();
			//speeds toggling visibility on/off for benchmarking especially
			#else
			if (LayerType() != TYP_SHP)
				((CImageLayer *)this)->DeleteDIB();
			#endif
		}
		else m_uFlags|=NTL_FLG_VISIBLE;
		return true;
	}
	return false;
}

bool CMapLayer::IsAppendable() const
{
	return LayerType()==TYP_SHP && IsEditable() && ((CShpLayer *)this)->ShpTypeOrg()==CShpLayer::SHP_POINT &&
		!((CShpLayer *)this)->IsAdditionPending();
}

void CMapLayer::InitDlgTitle(CDialog *pDlg) const
{
	char docname[MAX_PATH];
	strcpy(docname,m_pDoc->GetTitle());
	*trx_Stpext(docname)=0;
	CString title;
	title.Format("%s: %s",docname,Title());
	pDlg->SetWindowText(title);
}

static int getNad(LPCSTR pbuf,bool *pb84)
{
	char buf[128];
	_strupr(trx_Stncc(buf,pbuf,128));
	static LPCSTR ps[11]={"NAD_1927","NAD27","NORTH_AMERICAN_1927","NORTH_AMERICAN_DATUM_1927",
						"NAD_1983","NAD83","NORTH_AMERICAN_1983","NORTH_AMERICAN_DATUM_1983",
						"WGS_1984","WGS84","ITRF_1992"};
    for(int i=0;i<11;i++) {
		if(strstr(buf,ps[i])) {
			*pb84=(i>7);
			return (i>3)?1:2;
		}
	}
	return 3;
}

bool CMapLayer::GetProjection(LPCSTR pRef)
{
	//Set m_iNad (1, 2, or 3), m_iZone, m_bWGS84, m_bProjected, and strings m_projectionRef, m_datumName, m_unitsName --
	//some of these could have already been set!
	LPSTR pbuf=(LPSTR)pRef;

	OGRSpatialReference oSRS;
	if(oSRS.importFromWkt(&pbuf)==OGRERR_NONE) { //increments pbuf
		free((void *)m_projectionRef);
		m_projectionRef=_strdup(pRef);
		LPCSTR p=oSRS.GetAttrValue("DATUM");
		if(p) {
			m_iNad=getNad(p,&m_bWGS84);
			if(m_iNad==3) {
				//look inside projection name
				LPCSTR p0=NULL;
				if(oSRS.IsProjected()) {
					p0=p=oSRS.GetAttrValue("PROJCS");
				}
				else if(oSRS.IsGeographic()) {
					p0=p=oSRS.GetAttrValue("GEOGCS");
				}
				if(p0) {
					m_iNad=getNad(p0,&m_bWGS84);
					if(m_iNad!=3) p=p0;
				}
				if(m_iNad==3) {
					if(strlen(p)>7) p="Unknown";
					free((void *)m_datumName);
					m_datumName=_strdup(p);
				}
			}
			if(m_iNad!=3) {
				free((void *)m_datumName);
				m_datumName=_strdup((m_iNad==2)?"NAD27":(m_bWGS84?"WGS84":"NAD83"));
			}
		}
		{
			LPSTR pu;
			double du=oSRS.GetLinearUnits(&pu);
			if(pu) {
				if(!_stricmp(pu,"metre")) pu="meter";
				free((void *)m_unitsName);
				m_unitsName=_strdup(pu);
			}
		}
		if(oSRS.IsProjected()) {
			p=oSRS.GetAttrValue("PROJCS");
			if(p) m_projName=_strdup(p);
			int pbNorth=0;
			m_iZone=oSRS.GetUTMZone(&pbNorth);
			if(!pbNorth) m_iZone=-m_iZone;
			if(!m_iZone) m_iZone=99; //unknown projection
			return m_bProjected=true;
		}
		if(oSRS.IsGeographic()) {
			m_iZone=0;
			m_bProjected=false;
			return true;
		}
		{
			LPCSTR pn="LOCAL_CS";
			p=oSRS.GetAttrValue(pn);
			m_projName=_strdup((p && *p)?p:pn);
			m_bProjected=true;
			m_iZone=-99; //unsupported projection;
			return true;
		}
	}
	return false;
}

bool CMapLayer::GetPrjProjection(LPCSTR pPath)
{
	//Compute m_iNad (1, 2, or 3), m_iZone, and m_bProjected --
	char buf[1024];
	bool bProj=false;
	CFile cfPrj;
	if(cfPrj.Open(pPath,CFile::modeRead|CFile::shareDenyWrite)) {
		buf[cfPrj.Read(buf,1023)]=0;
		bProj=GetProjection(buf);
		cfPrj.Close();
	}
	return bProj;
}


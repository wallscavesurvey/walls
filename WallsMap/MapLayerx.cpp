//CMapLayer fcns --

#include "stdafx.h"
#include "WallsMap.h"
#include "utility.h"
#include "Mainfrm.h"
#include "WallsMapDoc.h"
#include "WallsMapView.h"
#include "MapLayer.h"
#include "ShowShapeDlg.h"

CMapLayer::~CMapLayer()
{
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
		else if(m_projName.GetLength()) 
			p=(LPCSTR)m_projName;
		else p="Unknown projection";
	}
	else {
		ASSERT(!m_bProjected);
		p="Geographic (Lat/Long)";
	}
	return String128("%s, Datum: %s",p,(m_iNad==2)?"NAD27 CONUS":((m_iNad==1)?"NAD83":
		(m_iNad?(m_datumName?m_datumName:"Unsupported"):"Unknown")));
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
	if(LayerType()!=CLayerSet::TYP_SHP || IsEditable())
		return m_csTitle;
	cs.Format("| %s",(LPCSTR)m_csTitle);
	return (LPCSTR)cs;
}

int CMapLayer::Visibility(DWORD rec /*=0*/)
{
	//Used to determine necessity of refresh after symbology change --
	//Return 0 if no part of layer is currently visible 
	//Return 1,2,or 3 if features/markers(1), labels(2), or both(3) are visible --
	if(!IsVisible()) return 0;
	POSITION pos=m_pDoc->GetFirstViewPosition();
	CWallsMapView *pView;
	int iret=0;
	CFltPoint *pFpt=NULL;
	if(rec) {
		ASSERT(LayerType()==CLayerSet::TYP_SHP);
		//We won't be revising individual poly labels.
		if(((CShpLayer *)this)->ShpType()==CShpLayer::SHP_POINT)
			pFpt=&((CShpLayer *)this)->m_fpt[rec-1];
	}
	bool bFeaturesEnabled=(LayerType()!=CLayerSet::TYP_SHP) || ((CShpLayer *)this)->ShpType()!=CShpLayer::SHP_POINT || 
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

void CMapLayer::SetVisible(bool bVisible)
{
	if(!bVisible) {
		m_uFlags&=~NTL_FLG_VISIBLE;
		if(LayerType()==CLayerSet::TYP_SHP) {
			if(((CShpLayer *)this)->ShpType()!=CShpLayer::SHP_POINT)
				//free a potentially large amnt of ram and address space
				((CShpLayer *)this)->UnInit_ppoly();
		}
		else ((CImageLayer *)this)->DeleteDIB();
	}
	else m_uFlags|=NTL_FLG_VISIBLE;
}

bool CMapLayer::IsAppendable() const
{
	return LayerType()==CLayerSet::TYP_SHP && IsEditable() && ((CShpLayer *)this)->ShpTypeOrg()==CShpLayer::SHP_POINT &&
		!((CShpLayer *)this)->IsAdditionPending();
}

void CMapLayer::InitDlgTitle(CDialog *pDlg,LPCSTR prefix /*=NULL*/) const
{
	char docname[MAX_PATH];
	strcpy(docname,m_pDoc->GetTitle());
	*trx_Stpext(docname)=0;
	CString title;
	title.Format("%s%s: %s",prefix?prefix:"",docname,Title());
	pDlg->SetWindowText(title);
}


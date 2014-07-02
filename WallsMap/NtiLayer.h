#ifndef __NTIILE_H
#include <ntifile.h>
#endif

#ifndef GDAL_PRIV_H_INCLUDED
#include <gdal_priv.h>
#endif

#ifndef _MAPLAYER_H
#include "MapLayer.h"
#endif

class CDIBWrapper;
class JP2KAKDataset {
public:
	static void KakaduInitialize();
};

class GDALDataset;
class GDALRasterBand;

struct CNTERecord
{
	enum {SIZ_CACHE=10,NODATA=0x7FFF};

	CNTERecord(GDALDataset *pnte,double *fTrans) :
		  nte(pnte)
		, pBand(NULL)
		, lenCache(0)
		, fCellSize(fTrans[1])
	{
		GDALRasterBand *po=nte->GetRasterBand(1);
		bLevelLimit=po->GetOverviewCount();
		geoExt.l=fTrans[0];
		geoExt.t=fTrans[3];
		geoExt.r=geoExt.l+fCellSize*po->GetXSize();
		geoExt.b=geoExt.t-fCellSize*po->GetYSize();
	}

	~CNTERecord()
	{
		GDALClose(nte);
	}

	GDALDataset *nte;
	GDALRasterBand *pBand;
	CFltRect geoExt;
	double pxPerGeo,fCellSize;
	UINT lenCache,XSize,YSize,rowBlks;
	UINT blkNo[SIZ_CACHE];
	INT16 blkElev[SIZ_CACHE][128*128];
	std::vector<GDALRasterBand *> vOvrBands;
	BYTE bLevelLimit;

	void SetBand(double fScale)
	{
		//fScale = Screen pixels / geo-units.

		pxPerGeo=1/fCellSize;
		BYTE lvl=0;
		while(pxPerGeo*0.5>=fScale && lvl<bLevelLimit) {
			pxPerGeo*=0.5;
			lvl++;
		}
		pBand=vOvrBands[lvl];
		XSize=pBand->GetXSize();
		YSize=pBand->GetYSize();
		rowBlks=((XSize+127)>>7);
		lenCache=0;
	}

	INT16 GetApproxElev(const CFltPoint &geoPt)
	{
		if(!geoExt.IsPtInside(geoPt))
			return NODATA;

		UINT blkOffX=(UINT)((geoPt.x-geoExt.l)*pxPerGeo+0.5);
		if(blkOffX>=XSize) return NODATA;
		UINT col=(blkOffX>>7);
		blkOffX-=(col<<7);

		UINT blkOffY=(UINT)((geoExt.t-geoPt.y)*pxPerGeo+0.5);
		if(blkOffY>=YSize) return NODATA;
		UINT row=(blkOffY>>7);
		blkOffY-=(row<<7);

		UINT b=row*rowBlks+col; //col = block number;
		UINT i;

		for(i=0;i<lenCache;i++)
			if(b==blkNo[i]) break;

		if(i==lenCache) {
			if(lenCache==SIZ_CACHE) i=0;
			else i=lenCache++;
			if(pBand->ReadBlock(col,row,&blkElev[i][0])) {
				if(i==0 && lenCache>1) {
					lenCache--;
				}
				else blkNo[i]=(UINT)-1;
				return NODATA;
			}
			blkNo[i]=b;
		}
		return blkElev[i][(blkOffY<<7)+blkOffX];
	}

	INT16 GetBestElev(const CFltPoint &geoPt)
	{
		if(!geoExt.IsPtInside(geoPt))
			return NODATA;

		GDALRasterBand *pB=vOvrBands[0];

		ASSERT(pB);

		if(pB==pBand) return GetApproxElev(geoPt); 

		double pxPerGeo0=1/fCellSize;

		UINT blkOffX=(UINT)((geoPt.x-geoExt.l)*pxPerGeo0+0.5);
		if(blkOffX>=(UINT)pB->GetXSize()) return NODATA;
		UINT col=(blkOffX>>7);
		blkOffX-=(col<<7);

		UINT blkOffY=(UINT)((geoExt.t-geoPt.y)*pxPerGeo0+0.5);
		if(blkOffY>=(UINT)pB->GetYSize()) return NODATA;
		UINT row=(blkOffY>>7);
		blkOffY-=(row<<7);

		//Read 128x128 block into next available cache block but don't add it to cache --

		if(lenCache==SIZ_CACHE) lenCache=SIZ_CACHE-1;
		pB->ReadBlock(col,row,&blkElev[lenCache][0]);

		return blkElev[lenCache][(blkOffY<<7)+blkOffX];
	}
};


class CNtiLayer : public CImageLayer
{
public:
	CNtiLayer(const CNTIFile &nti)
	{
		m_nti=nti;
		m_nte=NULL;
	}
	CNtiLayer() {m_nte=NULL;}
	virtual ~CNtiLayer();
	virtual int LayerType() const;

	const CNTIFile & GetNtiFile() const {return m_nti;}
	void NullifyHandle() {m_nti.NullifyHandle();}

	INT16 GetBestElev(const CFltPoint &ptGeo) {return m_nte->GetBestElev(ptGeo);}
	INT16 GetApproxElev(const CFltPoint &ptGeo) {return m_nte->GetApproxElev(ptGeo);}

	bool ParseTransColor(COLORREF &clr) const;
	
	virtual BOOL OpenHandle(LPCTSTR lpszPathName,UINT uFlags=0);
	virtual void CloseHandle();
	virtual BOOL Open(LPCTSTR lpszPathName,UINT uFlags);
	virtual int CopyToDIB(CDIBWrapper *pDestDIB,const CFltRect &geoExt,double fScale);
	virtual int CopyAllToDC(CDC *pCDC,const CRect &crDst,HBRUSH hbr=NULL);
	virtual void AppendInfo(CString &s) const;
	virtual void CopyBlock(LPBYTE pDstOrg,LPBYTE pSrcOrg,int srcLenX,int srcLenY,int nSrcRowWidth);
	int ReadRasterBand(UINT band,UINT xStart,UINT yStart,UINT xSize,UINT ySize,LPBYTE pDest);

	static void Init();
	static void UnInit();
	void SetNTIPointers();
	BOOL SetNTIProperties(); //used by Open() and CExportNTI::DoDataExchange()
	bool HasNTERecord() {return m_nte!=NULL;}

	static BOOL m_bExpanding;

	CNTERecord *m_nte;

private:

	void CheckForNTE();

	void ImgPtToGeoPt(CFltPoint &imgPos,int lvl) const
	{
		if(lvl) {
			imgPos.x*=(double)m_uWidth/m_nti.LvlWidth(lvl);
			imgPos.y*=(double)m_uHeight/m_nti.LvlHeight(lvl);
		}
		CImageLayer::ImgPtToGeoPt(imgPos);
	}

	void ImgRectToGeoRect(CFltRect &geoRect,CRect &imgRect,int lvl) const
	{
		geoRect=CFltRect(imgRect.left,imgRect.top,imgRect.right,imgRect.bottom);
		ImgPtToGeoPt(geoRect.tl,lvl);
		ImgPtToGeoPt(geoRect.br,lvl);
	}

	void GeoPtToImgPt(CFltPoint &geoPos,int lvl) const
	{
		CImageLayer::GeoPtToImgPt(geoPos);
		if(lvl) {
			geoPos.x*=(double)m_nti.LvlWidth(lvl)/m_uWidth;
			geoPos.y*=(double)m_nti.LvlHeight(lvl)/m_uHeight;
		}
	}

	CNTIFile m_nti;
};


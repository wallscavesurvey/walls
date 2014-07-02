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

	CNTERecord(GDALDataset *pnte,const CFltRect &extent) :
		nte(pnte)
		, geoExt(extent)
		, lenCache(0) {}

	~CNTERecord()
	{
		GDALClose(nte);
	}

	GDALDataset *nte;
	GDALRasterBand *pBand;
	CFltRect geoExt;
	double pxPerGeo;
	UINT lenCache,XSize,YSize,rowBlks;
	UINT blkNo[SIZ_CACHE];
	INT16 blkElev[SIZ_CACHE][128*128];
	std::vector<GDALRasterBand *> vOvrBands;

	void SetBand(UINT lvl,double fZoom)
	{
		lenCache=0;
		pxPerGeo=fZoom;	
		pBand=vOvrBands[lvl];
		XSize=pBand->GetXSize();
		YSize=pBand->GetYSize();
		rowBlks=((XSize+127)>>7);
	}

	INT16 GetElev(const CFltPoint &geoPt)
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

	INT16 GetElev(const CFltPoint &ptGeo) {return m_nte->GetElev(ptGeo);}

	BYTE GetZoomLevel(double &fZoom,double fScale)
	{
		fZoom = 1/m_fCellSize;
		//fZoom = (image pixels / geo-units) at level 0

		UINT lvl=0;
		while(fZoom*0.5>=fScale && lvl<m_bLevelLimit) {
		   fZoom*=0.5;
		   lvl++;
		}
		return lvl;
	}

	bool ParseTransColor(COLORREF &clr) const;
	
	void SetNteBand(double fScale)
	{
		double fZoom;
		BYTE lvl=GetZoomLevel(fZoom,fScale);
		m_nte->SetBand(lvl,fZoom);
	}

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

	void GeoPtToImgPt(CFltPoint &geoPos,int lvl) const
	{
		CImageLayer::GeoPtToImgPt(geoPos);
		if(lvl) {
			geoPos.x*=(double)m_nti.LvlWidth(lvl)/m_uWidth;
			geoPos.y*=(double)m_nti.LvlHeight(lvl)/m_uHeight;
		}
	}

	CNTIFile m_nti;
	CNTERecord *m_nte;
};


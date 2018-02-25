#include "stdafx.h"
#include "MapLayer.h"
#include "GdalLayer.h"
#include "WallsMapDoc.h"
#include <ogr_spatialref.h>
#include <gdal_priv.h>
#include <cpl_error.h>
#include <tiff.h>
#include <io.h>
//=====================================================================
//GDAL File operations --

#ifdef _USE_GDTIMER
double	secs_rasterio,secs_stretch;
#endif

LPBYTE CGdalLayer::m_pBufGDAL=NULL;
UINT CGdalLayer::m_lenBufGDAL=0;
UINT CGdalLayer::m_uRasterIOActive=0;

bool CGdalLayer::bInitialized=false;
bool CGdalLayer::bSetMaxCache=false;

bool CGdalLayer::IsScaledType() const
{
	if(!m_bPalette) {
		switch(m_uDataType) {
			case GDT_Byte:
				return m_uBitsPerSample<8;
			case GDT_UInt16:
			case GDT_Int16:
			case GDT_UInt32:
			case GDT_Int32:
			case GDT_Float32:
				return true;
			//case GDT_Float64:
		}
	}
	return false;
}

void CGdalLayer::AppendInfo(CString &cs) const
{
	AppendFileTime(cs,PathName(),0);

	cs.AppendFormat("Type: %s/%s",m_pGDAL->GetDriver()->GetDescription(), 
            m_pGDAL->GetDriver()->GetMetadataItem( GDAL_DMD_LONGNAME ));

	int nBands=m_pGDAL->GetRasterCount();

	cs.AppendFormat("\r\nImage size: %dx%dx%d (%d band%s)", 
            m_uWidth, m_uHeight,
			m_uBitsPerSample*nBands,nBands,(nBands>1)?"s":"");

	if(m_bPalette) {
		cs.AppendFormat(" %u-color Palette\r\n",m_uColors);
	}
	else cs+="\r\n";

	AppendGeoRefInfo(cs);

	if(m_pGDAL->GetCompressionType()>0) {
		const char *sTyp;
		switch(m_pGDAL->GetCompressionType()) {
			case COMPRESSION_NONE : sTyp="None"; break;
			case COMPRESSION_JPEG : sTyp="jpeg"; break;
			case COMPRESSION_LZW : sTyp="lzw"; break;
			case COMPRESSION_PACKBITS : sTyp="packbits"; break;
			case COMPRESSION_ADOBE_DEFLATE : sTyp="zip"; break;
			default:
				{
					sTyp=m_pGDAL->GetMetadataItem("COMPRESSION","IMAGE_STRUCTURE");
					if(!sTyp) sTyp="Unknown";
				}
		}
		cs.AppendFormat("Compression type: %s\r\n",sTyp);
	}

	GDALRasterBand *poBand=m_pGDAL->GetRasterBand(1);
	int bBlkSizeX,bBlkSizeY;
	poBand->GetBlockSize(&bBlkSizeX, &bBlkSizeY);

	cs.AppendFormat("Block Size: %dx%d  Total blocks: %.2fx%.2f  Type: %s  ColorInterp: %s",
                bBlkSizeX, bBlkSizeY,
				((double)m_uWidth) / bBlkSizeX,
				((double)m_uHeight) / bBlkSizeY,
				(LPCSTR)GDALGetDataTypeName((GDALDataType)m_uDataType),
				(LPCSTR)GDALGetColorInterpretationName(poBand->GetColorInterpretation()));

	for(int i=2;i<=nBands;i++) {
		cs.AppendChar(',');
		cs+=GDALGetColorInterpretationName(m_pGDAL->GetRasterBand(i)->GetColorInterpretation());
	}
	cs+="\r\n";

	int nOvr=poBand->GetOverviewCount();
	if(nOvr) {
		CString sRes;
		for(int i=0;i<nOvr;i++) {
			GDALRasterBand *poBando=poBand->GetOverview(i);
			sRes.AppendFormat(" (%dx%d)",poBando->GetXSize(),poBando->GetYSize());
		}
		cs.AppendFormat("File has %d overviews:%s\r\n",nOvr,sRes);
	}
	AppendProjectionRefInfo(cs);
}

void CPL_STDCALL GDALErrorHandler(CPLErr eErrClass, int err_no, const char *msg)
{
	if(eErrClass==CE_Warning /*|| err_no==CPLE_NotSupported*/) return;
	if(CGdalLayer::m_uRasterIOActive) {
		if(CGdalLayer::m_uRasterIOActive>1) return;
		CGdalLayer::m_uRasterIOActive++;
	}
	CString emsg;
	if(CGdalLayer::m_uRasterIOActive)
		emsg.Format("NOTE: File reading was interrupted. The data may be corrupt or incomplete.\nGDAL driver message: %s",msg);
	else
		emsg.Format("Open failed - GDAL driver message: %s",msg);
	AfxMessageBox(emsg);
}

static int __stdcall WallsMap_LoadDLL(const char *path,const char *pHdr)
{
	if(pHdr && !memcmp(pHdr,"msid",4)) {
		HMODULE hLib = LoadLibrary("WallsMap_sid.dll");
		if(hLib)
		{
			FreeLibrary(hLib);
		}
		else {
			CMsgBox("File %s can't be opened.\nMrSID images require that WallsMap_sid.dll be present.\n",trx_Stpnam(path));
			return 0;
		}
	}
	return 1;
}

void CGdalLayer::Init()
{
	if(bInitialized) return;
	CPLSetErrorHandler(GDALErrorHandler);
	GDALAllRegister();
	GDALSetWM_pLoadDLL(WallsMap_LoadDLL); //delay load certain DLL drivers (see above)
	bInitialized=true;
}

void CGdalLayer::UnInit()
{
	free(m_pBufGDAL);
	m_pBufGDAL=NULL;
	if(bInitialized)
		GDALDestroyDriverManager();
}

CGdalLayer::~CGdalLayer()
{
	FreeDupedStrings();
	if(m_pGDAL) {
		GDALClose(m_pGDAL);
	}
}

int CGdalLayer::LayerType() const
{
	return TYP_GDAL;
}

#ifdef _USE_SCALING
static int CPL_STDCALL scale_progress(double,const char *, void *)
{
	return 1;
}
static BOOL AdjustScaleLimits(int *cell,double *dMin,double *dMax)
{
	//Result is similar to OpenEV on ilatlon.tif
	UINT iTotal=0;
	for(int i=0;i<256;i++) {
		iTotal+=cell[i];
	}
	ASSERT(iTotal);
	UINT iLimit=(UINT)(iTotal*0.02); //tail percentage to truncate
	double sInc=(*dMax-*dMin)/256;

	iTotal=0;
	for(int i=0;i<256;i++) {
		if((iTotal+=cell[i])>iLimit) {
			*dMin+=(i*sInc);
			break;
		}
	}

	iTotal=0;
	for(int i=255;i>=0;i--) {
		if((iTotal+=cell[i])>iLimit) {
			*dMax-=((255-i)*sInc);
			break;
		}
	}
	return *dMax>*dMin;
}
#endif

void CGdalLayer::CloseHandle()
{
	m_dib.DeleteObject();
	if(m_pGDAL) {
		GDALClose(m_pGDAL);
		m_pGDAL=NULL;
	}
}

BOOL CGdalLayer::OpenHandle(LPCTSTR lpszPathName,UINT uFlags)
{
	if(m_pGDAL) return TRUE;

	if(_access(lpszPathName,4)==-1) {
		//Avoids funky GDAL behavior --
		if(!(uFlags&NTL_FLG_SILENT)) {
			if(uFlags&NTL_FLG_LAYERSET)
				CLayerSet::m_bOpenConflict=-1; //not found or could not be opened
			else
				CMsgBox("File %s is no longer present or accessible.",trx_Stpnam(lpszPathName));
		}
		return FALSE;
	}

	if(!bInitialized) Init();

	//Will call WallsMap_LoadDLL() to safely load a demand loaded DLL for sid files --
	if(!(m_pGDAL=(GDALDataset *)GDALOpen(lpszPathName,GA_ReadOnly))) {
		if(uFlags&NTL_FLG_LAYERSET) CLayerSet::m_bOpenConflict=1;
		return FALSE;
	}

	if(!bSetMaxCache) {
		bSetMaxCache=true;
		//int cache=GDALGetCacheMax(); //40 MB default!
		GDALSetCacheMax(256 * (1024 * 1024));
	}
	return TRUE;
}

BOOL CGdalLayer::Open(LPCTSTR lpszPathName,UINT uFlags)
{
	ASSERT(!PathName() && !m_lpBitmapInfo);

	if(!OpenHandle(lpszPathName,uFlags)) return FALSE;


	if(COMPRESSION_OJPEG==m_pGDAL->GetCompressionType()) {
		if(!(uFlags&NTL_FLG_SILENT))
			CMsgBox("File %s uses an unsupported compression method (\"old jpeg\") and can't be opened.",trx_Stpnam(lpszPathName));
		return FALSE;
	}

	m_uRasterIOActive=0;

	m_uWidth=m_pGDAL->GetRasterXSize();
	m_uHeight=m_pGDAL->GetRasterYSize();

	GDALRasterBand *poBand=m_pGDAL->GetRasterBand(1);
	GDALColorTable *pCT=poBand->GetColorTable();
	m_uColors=pCT?pCT->GetColorEntryCount():0;
	m_uDataType=poBand->GetRasterDataType();


	//GDAL only supports 8, 16, 32, and 64-bit data sizes!!
	m_uBytesPerSample=(UINT)GDALGetDataTypeSize((GDALDataType)m_uDataType);
	//This is actually the provisional bits per sample --

	//GetBitsPerSample() will return zero for drivers other than our modified gtiff and Kakadu --
	m_uBitsPerSample=m_pGDAL->GetBitsPerSample();
	if(!m_uBitsPerSample || m_uBitsPerSample>m_uBytesPerSample) m_uBitsPerSample=m_uBytesPerSample;
	m_uBytesPerSample>>=3;
	//Now both m_uBytesPerSample and m_uBitsPerSample are correct

	UINT32 nBands=m_pGDAL->GetRasterCount();

	if(!nBands ||
		(pCT && (nBands>2 || m_uColors<2 || m_uColors>256 || m_uDataType!=GDT_Byte)) || !ComputeLevelLimit(m_bLevelLimit,m_uWidth,m_uHeight)) {
			if(!(uFlags&NTL_FLG_SILENT)) {
				CString msg,msg0;
				msg0.Format("Size: %u x %u, Bands: %u, Sample bits: %u, Sample type: %s",
					m_uWidth,m_uHeight,nBands,m_uBitsPerSample,(LPCSTR)GDALGetDataTypeName((GDALDataType)m_uDataType));
				if(pCT) {
					msg.Format(", Palette colors: %u",m_uColors);
					msg0+=msg;
				}
				msg.Format("File %s has an unsupported combination of format traits and can't be opened.\n%s",trx_Stpnam(lpszPathName),(LPCSTR)msg0);
				AfxMessageBox(msg);
			}
			return FALSE;
	}

#ifdef _USE_SCALING
    m_bScaleSamples=false;
	if(m_uDataType==GDT_Int16 && nBands==1 && !(uFlags&NTL_FLG_SILENT)) {
		double dMin,dMax,dMean,dStd;
		int iStat=poBand->GetStatistics(
			TRUE, //Approx OK? (scan only part if possible)
			TRUE, //Force even if scan of data necessary?
			&dMin,&dMax,&dMean,&dStd
		);
		if(dMin<=-32768.0) dMin=-32767.0; //necessary for ilatlon.tif
		if(!iStat && dMin<dMax) {
			int cell[256];
			iStat=poBand->GetHistogram(dMin,dMax,256,cell,FALSE,TRUE,scale_progress,NULL);
			if(!iStat && AdjustScaleLimits(cell,&dMin,&dMax)) {
				m_sScaleMin=(short)floor(dMin);
				m_sScaleMax=(short)ceil(dMax);
				m_bScaleSamples=true;
			}
		}
	}
#endif

	m_pathName=_strdup(lpszPathName);
	m_bPalette=(pCT!=NULL);
	m_bScaledIO=IsScaledType();
	if(nBands==2) nBands--; //ignore alpha band for now
	m_uBands=m_uOvrBands=min(nBands,3);
	if(m_uBands!=3) {
		m_iLastTolerance=0;
		m_crTransColor&=0xFFFFFF; //in case file format changed!
	}

	InitSizeStr();

	m_lpBitmapInfo=(LPBITMAPINFO)calloc(1,sizeof(BITMAPINFOHEADER)+((!m_bPalette && m_uBands==1)?256:m_uColors)*sizeof(RGBQUAD));

	if(!m_lpBitmapInfo) {
		ASSERT(FALSE);
		return FALSE;
	}

	m_lpBitmapInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	m_lpBitmapInfo->bmiHeader.biPlanes = 1;
	m_lpBitmapInfo->bmiHeader.biCompression = BI_RGB;
	m_lpBitmapInfo->bmiHeader.biClrImportant = 0;
	m_lpBitmapInfo->bmiHeader.biClrUsed=(!m_bPalette && m_uBands==1)?256:m_uColors;
	m_lpBitmapInfo->bmiHeader.biBitCount = m_uBands*8;

	if(m_bPalette) {
		 GDALColorEntry	sEntry;
         for(UINT32 i = 0; i < m_uColors; i++ )  {
                pCT->GetColorEntryAsRGB(i,&sEntry );
				m_lpBitmapInfo->bmiColors[i].rgbRed=(BYTE)sEntry.c1;
				m_lpBitmapInfo->bmiColors[i].rgbGreen=(BYTE)sEntry.c2;
				m_lpBitmapInfo->bmiColors[i].rgbBlue=(BYTE)sEntry.c3;
				m_lpBitmapInfo->bmiColors[i].rgbReserved=(BYTE)sEntry.c4;
         }
	}
	else if(m_uBands==1) {
		//Grayscale --
		for(UINT i=0;i<256;i++)
				m_lpBitmapInfo->bmiColors[i].rgbRed=m_lpBitmapInfo->bmiColors[i].rgbGreen=
				m_lpBitmapInfo->bmiColors[i].rgbBlue=(BYTE)i;
	}

	m_fMetersPerUnit=1.0;

	m_bTrans=(m_pGDAL->GetGeoTransform(m_fTransImg) == CE_None );
	if(m_bTrans && m_fTransImg[1]>0.0 && m_fTransImg[5]<0.0 && (m_fTransImg[1]+m_fTransImg[5])<0.0001*m_fTransImg[1]
			&& !m_fTransImg[2] && !m_fTransImg[4])
	{
        LPCSTR pAreaOrPoint=m_pGDAL->GetMetadataItem( GDALMD_AREA_OR_POINT ); 
        if(pAreaOrPoint && !strcmp(pAreaOrPoint,GDALMD_AOP_POINT))
        {
			//Correct for GDAL geotiff driver bug. Tie point corresponds to center of pixel, not UL corner,
			//whenever tag GTRasterTypeGeoKey is defined and set to RasterPixelIsPoint (2)
			//instead of RasterPixelIsArea (1), the default value:
			m_fTransImg[0]-=0.5*m_fTransImg[1];
			m_fTransImg[3]-=0.5*m_fTransImg[5];
        }
		ComputeInverseTransform();
	}
	else {
		if(m_bTrans) {
			CMsgBox("File: %s\n\nNOTE: For display as a georeferenced image, this file requires "
				"a transformation that's currently not supported. It must be viewed as an ordinary image file.",
				PathName());
			m_bTrans=false;
		}
		//Some GDAL datasets (PNM) don't initialize this!
		ClearTransforms();
	}

	ComputeCellRatios();
	ComputeExtent();

	if(m_bTrans) {
		bool bProj=false;
		if(!CLayerSet::m_pLsetImgSource) {
			LPCSTR p=m_pGDAL->GetProjectionRef(); //At least ""
			ASSERT(p);
			if(!(bProj=GetProjection(p))) {
				CString prjpath(PathName());
				prjpath.Truncate(trx_Stpext(prjpath)-(LPCSTR)prjpath);
				prjpath+=SHP_EXT_PRJ;
				bProj=GetPrjProjection(prjpath);
			}
		}

		if(!bProj) {
			if(CLayerSet::m_pLsetImgSource) {
				//An exported PNG file --
				m_bProjected=(m_iZone=CLayerSet::m_pLsetImgSource->m_iZone)!=0;
				m_iNad=CLayerSet::m_pLsetImgSource->m_iNad;
				if(CLayerSet::m_pLsetImgSource->m_unitsName) {
					free((void *)m_unitsName);
					m_unitsName=_strdup(CLayerSet::m_pLsetImgSource->m_unitsName);
				}
				if(CLayerSet::m_pLsetImgSource->m_datumName) {
					free((void *)m_datumName);
					m_datumName=_strdup(CLayerSet::m_pLsetImgSource->m_datumName);
				}
			}
			else {
				//Assume either unknown UTM or WGS86 LatLon degrees
				m_iNad=0;
				if(IsLatLongExtent()) {
					m_iZone=0;
					if(!m_unitsName)
						m_unitsName=_strdup("degrees");
				}
				else {
					m_iZone=-99;
					m_bProjected=true;
					if(!m_unitsName)
						m_unitsName=_strdup("unknown");
				}
			}
		}
	}

	if(!m_unitsName) m_unitsName=_strdup(m_bTrans?"unknown":"pixels");

 	m_bOpen=true;
	m_uFlags=(uFlags&~NTL_FLG_OPENONLY);
    return TRUE;
}

BOOL CGdalLayer::ReadGrayscale(BYTE *lpBits,const CRect &crRegion,int szWinX,int szWinY)
{
	int nSizScan=SizeScanline(m_uBytesPerSample*szWinX);

	if(m_lenBufGDAL<(UINT)(nSizScan*szWinY))
		m_pBufGDAL=(LPBYTE)realloc(m_pBufGDAL,m_lenBufGDAL=nSizScan*szWinY);
	if(!m_pBufGDAL) {
		ASSERT(FALSE);
		m_lenBufGDAL=0;
		return FALSE;
	}

	GDALRasterBand *poBand=m_pGDAL->GetRasterBand(1);

	ASSERT(poBand->GetYSize()==m_uHeight && poBand->GetXSize()==m_uWidth);

	if(poBand->RasterIO(GF_Read,crRegion.left,crRegion.top,crRegion.Width(),crRegion.Height(),
				m_pBufGDAL,szWinX,szWinY,(GDALDataType)m_uDataType,0,nSizScan)) {
			free(m_pBufGDAL);
			return FALSE;
	}

	ASSERT(m_uBitsPerSample!=8 && m_uBytesPerSample<=32);
	double fDiv=255.0/(0xFFFFFFFF>>(32-m_uBitsPerSample));

	#ifdef _USE_SCALING
	if(m_bScaleSamples) {
		fDiv=255.0/(m_sScaleMax-m_sScaleMin);
	}
	#endif

	int nSizScan8=SizeScanline(szWinX);

	for(int row=0;row<szWinY;row++) {
		BYTE *pSrc=(BYTE *)(m_pBufGDAL+row*nSizScan);
		BYTE *pDst=lpBits+row*nSizScan8;

		switch(m_uBytesPerSample) {

			case 1 :
				for(int col=0;col<szWinX;col++,pSrc++)
					*pDst++=(BYTE)(*pSrc*fDiv+0.5);
				break;

			case 2 :
	#ifdef _USE_SCALING
				if(m_bScaleSamples) {
					for(int col=0;col<szWinX;col++,pSrc+=2) {
						short s=*(short *)pSrc;
						if(s<m_sScaleMin) s=m_sScaleMin;
						else if(s>m_sScaleMax) s=m_sScaleMax;
						*pDst++=(BYTE)((s-m_sScaleMin)*fDiv+0.5);
					}
				}
				else {
					for(int col=0;col<szWinX;col++,pSrc+=2)
						*pDst++=(BYTE)(*(WORD *)pSrc*fDiv+0.5);
				}
	#else
				/*
				//This doesn't work well with ilatlon.tif, etc. (Range test needed apparently.)
				if(m_uDataType==GDT_Int16) {
					for(int col=0;col<szWinX;col++,pSrc+=2) {
						int i=(int)*(short *)pSrc+32768; //>=0
						*pDst++=(BYTE)(i*fDiv+0.5);
					}
				}
				else
				*/
				{
					for(int col=0;col<szWinX;col++,pSrc+=2)
						*pDst++=(BYTE)(*(WORD *)pSrc*fDiv+0.5);
				}
	#endif
				break;

			case 4 :
				if(m_uDataType==GDT_Float32) {
					for(int col=0; col<szWinX; col++, pSrc+=4)
						*pDst++=(BYTE)(*(float*)pSrc*fDiv+0.5);
				}
				else {
					for(int col=0; col<szWinX; col++, pSrc+=4)
						*pDst++=(BYTE)(*(DWORD *)pSrc*fDiv+0.5);
				}
				break;
		}
	}
	return TRUE;
}

BOOL CGdalLayer::ReadScaledRGB(BYTE *lpBits,const CRect &crRegion,int szWinX,int szWinY)
{
	int nSizScan=m_uBytesPerSample*szWinX;

	if(m_lenBufGDAL<(UINT)(nSizScan*szWinY))
		m_pBufGDAL=(LPBYTE)realloc(m_pBufGDAL,m_lenBufGDAL=nSizScan*szWinY);
	if(!m_pBufGDAL) {
		ASSERT(FALSE);
		m_lenBufGDAL=0;
		return FALSE;
	}

	ASSERT(m_uBitsPerSample!=8 && m_uBytesPerSample<=32);
	double fDiv=255.0/(0xFFFFFFFF>>(32-m_uBitsPerSample));

	int nSizScan8=SizeScanline(szWinX);
	int nBytesOff=0;
	for(int b=m_uBands;b;b--,nBytesOff++) {

		GDALRasterBand *poBand=m_pGDAL->GetRasterBand(b);

		ASSERT(poBand->GetYSize()==m_uHeight && poBand->GetXSize()==m_uWidth);

		if(poBand->RasterIO(GF_Read,crRegion.left,crRegion.top,crRegion.Width(),crRegion.Height(),
				m_pBufGDAL,szWinX,szWinY,(GDALDataType)m_uDataType,m_uBytesPerSample,nSizScan)) {
			return FALSE;
		}

		for(int row=0;row<szWinY;row++) {
			BYTE *pSrc=(BYTE *)(m_pBufGDAL+row*nSizScan);
			BYTE *pDst=lpBits+row*nSizScan8+nBytesOff;

			switch(m_uBytesPerSample) {
				case 1 :
					for(int col=0;col<szWinX;col++,pSrc+=m_uBytesPerSample,pDst+=m_uBands)
						*pDst=(BYTE)(*pSrc*fDiv+0.5);
					break;

				case 2 :
					for(int col=0;col<szWinX;col++,pSrc+=2,pDst+=m_uBands) {
						*pDst=(BYTE)(*(WORD *)pSrc*fDiv+0.5);
					}
					break;

				case 4 :
					for(int col=0;col<szWinX;col++,pSrc+=4,pDst+=m_uBands)
						*pDst=(BYTE)(*(DWORD *)pSrc*fDiv+0.5);
					break;
			}
		}
	}
	return TRUE;
}

BOOL CGdalLayer::ReadBitmapData(BYTE *lpBits,const CRect &crRegion,int szWinX,int szWinY)
{

	BOOL bRet=TRUE;

	if(!m_uRasterIOActive) m_uRasterIOActive=1;
	if(m_bScaledIO) { 
		if(m_uBands==1) bRet=ReadGrayscale(lpBits,crRegion,szWinX,szWinY);
		else bRet=ReadScaledRGB(lpBits,crRegion,szWinX,szWinY);
	}
	else {
#if 0 //_DEBUG
		float *pflt=(float *)calloc(m_uWidth*m_uHeight,sizeof(float));
#endif
		int nBytesOff=0;
		int nSizScan=SizeScanline(szWinX);
		//try {
			for(int b=m_uBands;b;b--,nBytesOff++) {

				GDALRasterBand *poBand=m_pGDAL->GetRasterBand(b);

				ASSERT(poBand->GetYSize()==m_uHeight && poBand->GetXSize()==m_uWidth);

#if 0  //_DEBUG
				float *pflt=(float *)calloc(m_uWidth*m_uHeight,sizeof(float));
				if(poBand->RasterIO(GF_Read,crRegion.left,crRegion.top,crRegion.Width(),crRegion.Height(),
						pflt,szWinX,szWinY, GDT_Float32,0,0)!=CE_None) {
					bRet=FALSE;
					break;
				}
				//float fmin=100.0f,fmax=-100.0f;
				float fmax=18.94000005f, fmin=1.34000003f;
				float fsumd=0.0f,fsum=0.0f,fdif=0.0,flast=-100.0f;
				UINT ndata=0;
				for(UINT r=0;r<m_uHeight;r++) {
					flast=-100.0f;
					for(UINT i=0;i<m_uWidth;i++) {
						float f=pflt[r*m_uWidth+i];
						if(f==-32767.0f) {
							flast=-100.0f;
							*lpBits++=255;
							continue;
						}
						if(flast!=-100.0f) {
							fdif=fabs(f-flast);
							fsumd+=fdif;
							fsum+=f;
							ndata++;
						}
						flast=f;
						*lpBits++=(BYTE)((f-fmin)/(fmax-fmin)*255);
						if(flast<fmin) fmin=flast;
						if(flast>fmax) fmax=flast;
					}
				}
				flast=fsumd/ndata; //ave diff
				fsum=fsum/ndata;   //ave height
				free(pflt);
#else
				if(poBand->RasterIO(GF_Read,crRegion.left,crRegion.top,crRegion.Width(),crRegion.Height(),
						lpBits+nBytesOff,szWinX,szWinY,GDT_Byte,m_uBands,nSizScan)!=CE_None) {
					bRet=FALSE;
					break;
				}
#endif
			}
		//}
		//catch(std::exception &e) {
			//AfxMessageBox(e.what());
			//bRet=FALSE;
		//}
	}
	return bRet;
}

int CGdalLayer::CopyAllToDC(CDC *pDC,const CRect &crDst,HBRUSH hbr)
{
	//Copy complete image to destination DC, filling and centering it as needed.
	//iWidth and iHeight are dimensions of bitmap selected in *pDC (or of window)

	CSize szDst(crDst.Width(),crDst.Height());
	CSize szSrc(m_uWidth,m_uHeight);
	UINT uWidth=szDst.cx;
	UINT uHeight=szDst.cy;

	//if(uWidth<=0 || uHeight<=0 || m_uWidth<=0 || m_uHeight<=0) {
		//return 0;
	//}

	#ifdef _USE_GDTIMER
	bool bTiming=false;
	#endif

	//Determine size(uHeight,uWidth) >= crDst to scale image to initially --
	bool bFitWidth=m_uWidth*uHeight>=uWidth*m_uHeight;
	UINT wDst=bFitWidth?uHeight:uWidth;
	UINT wSrc=bFitWidth?m_uHeight:m_uWidth;

	if(uWidth<=100 || m_uBands==1) {
		UINT N=(uWidth<=100)?6:2;
		for(;N>1 && N*wDst>wSrc;N--);
		if(N>1) {
			uWidth*=N;
			uHeight*=N;
		}
		else {
			uWidth=m_uWidth;
			uHeight=m_uHeight;
		}
	}
	else if(wDst>wSrc) {
		uWidth=m_uWidth;
		uHeight=m_uHeight;
	}

	//Generate a new local DIB if required --
	CSize  szDIB=m_dib.GetHandle()?m_dib.GetSize():CSize(0,0);

	if((bFitWidth && (UINT)szDIB.cx<uWidth) || (!bFitWidth && (UINT)szDIB.cy<uHeight)) {

		CRect crImage(0,0,m_uWidth,m_uHeight);

		m_lpBitmapInfo->bmiHeader.biWidth = uWidth;
		m_lpBitmapInfo->bmiHeader.biHeight = -(int)uHeight;
		m_lpBitmapInfo->bmiHeader.biSizeImage = uHeight*((uWidth*m_uBands+3)&~3);

		if(!m_dib.InitBitmap(m_lpBitmapInfo)) return 0;

#ifdef _USE_GDTIMER
		bTiming=true;
		CLR_NTI_TM(1);
		START_NTI_TM(1);
#endif
		if(!ReadBitmapData((BYTE *)m_dib.GetDIBits(),crImage,uWidth,uHeight)) {
			m_dib.DeleteObject();
			return 0;
		}

#ifdef _USE_GDTIMER
		STOP_NTI_TM(1);
		secs_rasterio+=NTI_SECS(1);
#endif

		m_uRasterIOActive=0;
		m_dib_rect=crImage;
	}

	//We now have a local bitmap, m_dib, containing the *entire* image rectangle
	//m_dib_rect at zoom level m_dib_zoom

	//Compute position of m_dib's upper left corner in *pDestDIB.
	//Also ddjust szSrc to be the size of the image in *pDestDID --
	int dibOffX,dibOffY;

	if(bFitWidth) {
		if(szSrc.cx<=szDst.cx) {
			dibOffX=((szDst.cx-szSrc.cx)>>1);
		}
		else {
			bFitWidth=false;
			szSrc.cx=szDst.cx;
			szSrc.cy=(int)(szSrc.cx*(m_uHeight/(double)m_uWidth));
			dibOffX=0;
		}
		dibOffY=((szDst.cy-szSrc.cy)>>1);
	}
	else {
		if(szSrc.cy<=szDst.cy) {
			bFitWidth=true;
			dibOffY=((szDst.cy-szSrc.cy)>>1);
		}
		else {
			szSrc.cy=szDst.cy;
			szSrc.cx=(int)(szSrc.cy*(m_uWidth/(double)m_uHeight));
			dibOffY=0;
		}
		dibOffX=((szDst.cx-szSrc.cx)>>1);
	}

	dibOffX+=crDst.left;
	dibOffY+=crDst.top;

	if(hbr) ::FillRect(pDC->m_hDC,&crDst,hbr);

#ifdef _USE_GDTIMER
	if(bTiming) {
		CLR_NTI_TM(1);
		START_NTI_TM(1);
	}
#endif

	if(!bFitWidth) {
		//Compute final screen pixels / image pixels --
		m_dib.Stretch(pDC->m_hDC,dibOffX,dibOffY,szSrc.cx,szSrc.cy,HALFTONE);
	}
	else m_dib.Draw(pDC->m_hDC,dibOffX,dibOffY);

#ifdef _USE_GDTIMER
	if(bTiming) {
		STOP_NTI_TM(1);
		secs_stretch+=NTI_SECS(1);
	}
#endif

	return 1;
}

int CGdalLayer::CopyToDIB(CDIBWrapper *pDestDIB,const CFltRect &geoExt,double fScale)
{
	//pDestDIB				- Points to screen bitmap to which image data will be copied.
	//geoExt				- Geographical extent corresponding to bitmap.
	//fScale				- Screen pixels / geo-units.
	//
	//Returns:	0			- No data copied to bitmap.
	//			1			- Only a portion of the data copied.
	//			2			- All data copied.

	//Convert geoExt to trimmed image pixel extent -- 
	CRect crImage;
	GeoRectToImgRect(geoExt,crImage);

	if(crImage.Width()<=0 || crImage.Height()<=0) return 0;

	//Return value is 2 if all image data is copied --
	int iRet=(crImage.Width()==(int)m_uWidth && crImage.Height()==(int)m_uHeight)?2:1;

	if(!m_bAlpha) return iRet;

	//Compute zoom level (m_dib_px/image_px) we'll use for RasterIO to our local bitmap --

	double fZoom=min(fScale*m_fCellSize,1.0); //initally far less than 1 for normal images
	//fZoom = screen pixels that will be occupied by one image pixel after stretchblt

	//After possible adjustment, fZoom * crImage.Width() will be m_dib width --
	if(fZoom<1.0) {
		//To avoid pixelation, use a bigger DIB and let GDI rather than GDAL average colors --
		int w=(int)(crImage.Width()*fZoom);
		int h=(int)(crImage.Height()*fZoom);
		if(w<2000 && h<2000 && max(w,h)>30) {
			fZoom=2000.0/((w>h)?crImage.Width():crImage.Height());
			if(fZoom>1.0) fZoom=1.0;
		}
	}

	if(!m_dib.GetHandle()) {
		m_dib_zoom=0.0;
	}

	//Did last operation modify original DIB so that a new one is needed?
	bool bDibChanged=(m_hdcMask && m_crTransColorMask && (!m_bUseTransColor || m_crTransColor!=m_crTransColorMask));

	if((fZoom>m_dib_zoom && m_dib_zoom<1.0) || ContainsRect(m_dib_rect,crImage)!=2 || bDibChanged)
	{
		if(m_hdcMask) DeleteMask();

		//For our new local DIB let's expand this minimal image region to block boundaries,
		//but not beyond the image itself --
		crImage.left-=(crImage.left%BlkWidth);
		crImage.top-=(crImage.top%BlkWidth);
		if(crImage.right%BlkWidth) {
			crImage.right+=BlkWidth-(crImage.right%BlkWidth);
			if(crImage.right>(int)m_uWidth) crImage.right=m_uWidth;
		}
		if(crImage.bottom%BlkWidth) {
			crImage.bottom+=BlkWidth-(crImage.bottom%BlkWidth);
			if(crImage.bottom>(int)m_uHeight) crImage.bottom=m_uHeight;
		}
		//Convert crImage dimensions (image pixels) to m_dib dimensions --
		int imgWidth=(int)(crImage.Width()*fZoom+0.5);
		int imgHeight=(int)(crImage.Height()*fZoom+0.5);
		if(!imgWidth) imgWidth++;
		if(!imgHeight) imgHeight++;

		m_lpBitmapInfo->bmiHeader.biWidth = imgWidth;
		m_lpBitmapInfo->bmiHeader.biHeight = -imgHeight;
		m_lpBitmapInfo->bmiHeader.biSizeImage = imgHeight*((imgWidth*m_uBands+3)&~3); //expand stride (row width) to DWORD multiple

		if(!m_dib.InitBitmap(m_lpBitmapInfo)) return 0;

		if(!ReadBitmapData((BYTE *)m_dib.GetDIBits(),crImage,imgWidth,imgHeight)) {
			m_dib.DeleteObject();
			return 0;
		}

		if(m_bUseTransColor && IsUsingBGRE())
			ApplyBGRE();

		m_uRasterIOActive=0;
		m_dib_rect=crImage;
		m_dib_zoom=fZoom;
	}
	//else we're zooming out from the zoom level of the previous bitmap, which still contains it,
	//or else zooming greater than 1:1 and the bitmap still contains it.
	else if((m_hdcMask!=0)!=m_bUseTransColor || m_bUseTransColor && m_crTransColorMask!=m_crTransColor) {
		if(m_hdcMask) {
			DeleteMask(); //turning off transcolor
		}
		if(m_bUseTransColor && m_uBands==3 && IsUsingBGRE()) {
			//turning on transcolor, apply BGRE test to m_dib if required
			ApplyBGRE();
		}
	}

	//We now have a local bitmap, m_dib, containing the required image rectangle
	//m_dib_rect at zoom level m_dib_zoom

	//Compute position of m_dib's upper left corner in *pDestDIB --
	CFltPoint dibOffset((double)m_dib_rect.left,(double)m_dib_rect.top);
	ImgPtToGeoPt(dibOffset);
	int destOffX=_rnd(fScale*(dibOffset.x-geoExt.l));
	int destOffY=_rnd(fScale*(dibOffset.y-geoExt.t));
	if(geoExt.t>geoExt.b) destOffY=-destOffY;

	fZoom=fScale*m_fCellSize;

	int destWidth=(int)ceil(fZoom*m_fCellRatioX*m_dib_rect.Width());
	int destHeight=(int)ceil(fZoom*m_fCellRatioY*m_dib_rect.Height());
	
	int srcWidth=m_dib.GetWidth();
	int srcHeight=m_dib.GetHeight();


	//fScale*m_fCellSize = screen pixels to cover in the dimension with the smallest cell length (m_fCellRatio==1) --
 	if(!CopyData(pDestDIB,destOffX,destOffY,destWidth,destHeight,0,0,srcWidth,srcHeight))
		iRet=0;

	if(!iRet || CLayerSet::m_bExporting) {
		m_dib.DeleteObject();
	}

	return iRet;
}

void CGdalLayer::CopyBlock(LPBYTE pDstOrg,LPBYTE pSrcOrg,int srcLenX,int srcLenY,int nSrcRowWidth)
{
	//Copies srcLenX x srcLenY portion of a 256 x 256 block of bytes --
	//nSrcRowWidth is the number of samples (one color component) in a full row of source data

	nSrcRowWidth=(nSrcRowWidth*m_uBytesPerSample+3)&~3; //convert to a row offset in bytes
	if(IsScaledType()) {
		double fDiv=1.0; //Avoid "use of undefined" error

		switch(m_uBytesPerSample) {

			case 1 :
			case 2 :
				fDiv=255.0/(0xFFFF>>(16-m_uBitsPerSample));
				break;

			case 4 :
				//if(m_uBitsPerSample==32) fDiv=256/4294967296.0; //= 256 / 2**32
				//else fDiv=256/(double)(1<<m_uBitsPerSample);
				fDiv=255.0/(0xFFFFFFFF>>(32-m_uBitsPerSample));
				break;
		}
	
		for(int row=0;row<srcLenY;row++) {
			LPBYTE pDst=pDstOrg+row*256;
			LPBYTE pSrc=pSrcOrg+(row*nSrcRowWidth);

			int dstCols=srcLenX/m_uBytesPerSample;
			switch(m_uBytesPerSample) {
				case 1 :
					for(int col=0;col<dstCols;col++,pSrc++)
						*pDst++=(BYTE)(*pSrc*fDiv+0.5);
					break;

				case 2 :
					for(int col=0;col<dstCols;col++,pSrc+=2) {
						*pDst++=(BYTE)(*(WORD *)pSrc*fDiv+0.5);
					}
					break;

				case 4 :
					for(int col=0;col<dstCols;col++,pSrc+=4)
						*pDst++=(BYTE)(*(DWORD *)pSrc*fDiv+0.5);
					break;
			}
		}
	}
	else {
		for(int row=0;row<srcLenY;row++) {
			memcpy(pDstOrg+row*256,pSrcOrg+(row*nSrcRowWidth),srcLenX);
		}
	}
}

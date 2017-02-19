#define GDAL_MAXLEVELS 16

#undef _USE_SCALING //Only affects 16-bit grayscale

#undef _USE_GDTIMER
#ifdef _USE_GDTIMER
extern double secs_rasterio,secs_stretch;
#endif

class GDALDataset;
class CDIBWrapper;

class CGdalLayer : public CImageLayer
{
	friend class CExportNTI;
	friend class CExportJob;
public:
	CGdalLayer() : m_pGDAL(NULL) {}
	virtual ~CGdalLayer();
	virtual int LayerType() const;
	virtual BOOL OpenHandle(LPCTSTR lpszPathName,UINT uFlags=0);
	virtual void CloseHandle();
	virtual BOOL Open(LPCTSTR lpszPathName,UINT uFlags);
	virtual int CopyToDIB(CDIBWrapper *pDestDIB,const CFltRect &geoExt,double fScale);
	virtual int CopyAllToDC(CDC *pCDC,const CRect &crDst,HBRUSH hbr=NULL);

//	virtual void DrawOutline(CDIBWrapper *pDestDIB,const CFltRect &geoExt,double fScale);
	virtual void AppendInfo(CString &s) const;
	virtual void CopyBlock(LPBYTE pDstOrg,LPBYTE pSrcOrg,int srcLenX,int srcLenY,int nSrcRowWidth);

	static void Init();
	static void UnInit();

	UINT32 DataType() const {return m_uDataType;}
	UINT32 BytesPerSample() const {return m_uBytesPerSample;}
	bool IsScaledType() const;

	static UINT m_uRasterIOActive;

	static bool IsInitialized() {return bInitialized && bSetMaxCache;}

private:
	static bool bInitialized;
	static bool bSetMaxCache;
	int SizeScanline(int width) {return (width*m_uBands+3)&~3;}
	BOOL ReadGrayscale(BYTE *lpBits,const CRect &crRegion,int szWinX,int szWinY);
	BOOL ReadScaledRGB(BYTE *lpBits,const CRect &crRegion,int szWinX,int szWinY);
	BOOL ReadBitmapData(BYTE *lpBits,const CRect &crRegion,int szWinX,int szWinY);

	UINT32 m_uDataType,m_uBytesPerSample;

	static LPBYTE m_pBufGDAL;
	static UINT m_lenBufGDAL;

	double m_dib_zoom;

	GDALDataset *m_pGDAL;
#ifdef _USE_SCALING
	short m_sScaleMin,m_sScaleMax;
	bool m_bScaleSamples;
#endif
	bool m_bScaledIO;
};


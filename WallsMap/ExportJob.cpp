#include "stdafx.h"
#include "WallsMap.h"
#include "mainfrm.h"
#include "WallsMapDoc.h"
#include "WallsMapView.h"
#include "GdalLayer.h"
#include "NtiLayer.h"
#include <gdal_priv.h>
#include <cpl_multiproc.h>
#include <ntifile.h>
#include <iqa/iqa.h>
#include "ExportNTI_Adv.h"
#include "ExportJob.h"

void CPL_STDCALL GDALErrorHandler(CPLErr eErrClass, int err_no, const char *msg);

static LPBYTE pDataBuf[3];
static UINT nWidth,nHeight,nBands,nColors;
static BYTE bt,gt,rt,et;
static GDALDataset *pGDAL;
static bool bScaled,bUseTransColor;

#define SSIM_BUFFERS 7
#define SSIM_BUFSIZE (NTI_BLKWIDTH*NTI_BLKWIDTH*sizeof(float))

#ifdef SSIM_RGB
double ssim_total[3];
#else
double ssim_total;
#endif
UINT ssim_count;

CNTIFile CExportJob::nti;

//===================================================
//ExportNTI functions -- members of CWallsMapDoc

UINT CExportJob::exitNTI(LPCSTR msg)
{
	int err=nti.Errno();

	if(m_bExpanding) {
		nti.PurgeCache();
		VERIFY(!nti.TruncateOverviews());
		nti.EncodeFcn()(NULL,1); //Encode uninitialize!
		VERIFY(!nti.Close());
	}
	else {
		if(pGDAL) {
			GDALClose(pGDAL);
			pGDAL=NULL;
			CPLPopErrorHandler();
			CPLCleanupTLS();
		}

		for(int i=0;i<3;i++) {
			free(pDataBuf[i]);
			pDataBuf[i]=NULL;
		}
		if(nti.Opened()) {
			nti.EncodeFcn()(NULL,1); //Encode uninitialize!
			nti.CloseDel();
		}
	}

	if(msg && CGdalLayer::m_uRasterIOActive<2) CMsgBox("%s%s%s.",msg,err?" - ":"",err?nti.Errstr(err):"");

	OnProgress(-1);

	return 0;
}

static void FilterBGR(CSH_HDRP hp)
{
	int id=CExportJob::nti.FilterFlag();
	UINT srcLenX,srcLenY;

	CExportJob::nti.GetRecDims(hp->Recno,&srcLenX,&srcLenY);

	//Transform the colors for better compression performance --
	for(UINT row=0;row<srcLenY;row++) {
		LPBYTE pB=hp->Reserved[0]+row*NTI_BLKWIDTH;
		LPBYTE pG=hp->Reserved[1]+row*NTI_BLKWIDTH;
		LPBYTE pR=hp->Reserved[2]+row*NTI_BLKWIDTH;
		if(id==NTI_FILTERGREEN) {
			for(UINT col=0;col<srcLenX;col++,pB++,pG++,pR++) {
				*pB=(*pB-*pG);
				*pR=(*pR-*pG);
			}
		}
		else if(id==NTI_FILTERRED) {
			for(UINT col=0;col<srcLenX;col++,pB++,pG++,pR++) {
				*pB=(*pB-*pR);
				*pG=(*pG-*pR);
			}
		}
		else {
			for(UINT col=0;col<srcLenX;col++,pB++,pG++,pR++) {
				*pG=(*pG-*pB);
				*pR=(*pR-*pB);
			}
		}
	}
}

static void FixTransColor(CSH_HDRP hp)
{
	int id=CExportJob::nti.FilterFlag();
	UINT srcLenX,srcLenY;

	CExportJob::nti.GetRecDims(hp->Recno,&srcLenX,&srcLenY);

	//Make colors nearby the transparent color match exactly --
	for(UINT row=0;row<srcLenY;row++) {
		LPBYTE pB=hp->Reserved[0]+row*NTI_BLKWIDTH;
		LPBYTE pG=hp->Reserved[1]+row*NTI_BLKWIDTH;
		LPBYTE pR=hp->Reserved[2]+row*NTI_BLKWIDTH;
		for(UINT col=0;col<srcLenX;col++,pB++,pG++,pR++) {
			if(abs(bt-*pB)<et && abs(gt-*pG)<et && abs(rt-*pR)<et) {
				*pB=bt;
				*pG=gt;
				*pR=rt;
			}
		}
	}
}

static int GetQuadrantRecs(LPBYTE *pBuf,UINT lvl,UINT offX,UINT offY,UINT *pWidth,UINT *pHeight)
{
	// lvl        - Current level index
	// offX,offY  - Position in this level's bitmap (dest), a quad corresponding to a rec in lvl-1
	// Initializes: pBuf[]   - array of ptrs to previous level's record buffers for up to three bands
	//              *pWidth  - width of pixel block to copy resized version of (max is NTI_BLKWIDTH)
	//			    *pHeight - height of pixel block to copy resized version of (max is NTI_BLKWIDTH)
	// Returns:   - Number of bands in level lvl-1, (length of pBuf[]), or 0 if quad has no data

	UINT srcWidth=CExportJob::nti.LvlWidth(--lvl);
	UINT srcHeight=CExportJob::nti.LvlHeight(lvl);
	UINT srcBands=CExportJob::nti.LvlBands(lvl);
	offX<<=1; offY<<=1;
	//offX,offY are now the corresponding offsets in the parent (source) image --
	if(offX>=srcWidth || offY>=srcHeight) return 0;

	UINT width=(offX+NTI_BLKWIDTH>srcWidth)?(srcWidth-offX):NTI_BLKWIDTH;
	UINT height=(offY+NTI_BLKWIDTH>srcHeight)?(srcHeight-offY):NTI_BLKWIDTH;

	UINT rec0=CExportJob::nti.LvlRec(lvl)+
		(offY>>NTI_BLKSHIFT)*((srcWidth+NTI_BLKWIDTH-1)>>NTI_BLKSHIFT)+
		(offX>>NTI_BLKSHIFT);

	CSH_HDRP hp=CExportJob::nti.GetRecord(rec0);
	if(!hp) return -1;

	for(UINT b=0;b<srcBands;b++) {
		pBuf[b]=hp->Reserved[b];
	}

	if(srcBands==3 && CExportJob::nti.FilterFlag()) {
		int id=CExportJob::nti.FilterFlag();
		//Restore the transformed colors --
		for(UINT row=0;row<height;row++) {
			LPBYTE pB=pBuf[0]+row*NTI_BLKWIDTH;
			LPBYTE pG=pBuf[1]+row*NTI_BLKWIDTH;
			LPBYTE pR=pBuf[2]+row*NTI_BLKWIDTH;
			if(id==NTI_FILTERGREEN) {
				for(UINT col=0;col<width;col++,pB++,pG++,pR++) {
					*pB=(*pB+*pG);
					*pR=(*pR+*pG);
				}
			}
			else if(id==NTI_FILTERRED) {
				for(UINT col=0;col<width;col++,pB++,pG++,pR++) {
					*pB=(*pB+*pR);
					*pG=(*pG+*pR);
				}
			}
			else {
				for(UINT col=0;col<width;col++,pB++,pG++,pR++) {
					*pG=(*pG+*pB);
					*pR=(*pR+*pB);
				}
			}
		}
	}
	*pWidth=width;
	*pHeight=height;
	return srcBands;
}

static void CopyTransBgrRecs(LPBYTE *ppDst,UINT dstOff,LPBYTE *ppSrc,int srcLenX,int srcLenY)
{
	// ppDst[]    - Destination: Ptr to TL corner of a quadrant inside a NTI_BLKWIDTH x NTI_BLKWIDTH record
	// ppSrc[]    - Source: Ptr to srcLenX x srcLenY block, 1/4 of which will be copied

	BYTE b[4],g[4],r[4];

	LPBYTE pSrcB=ppSrc[0],pSrcG=ppSrc[1],pSrcR=ppSrc[2];
	LPBYTE pDstB=ppDst[0]+dstOff,pDstG=ppDst[1]+dstOff,pDstR=ppDst[2]+dstOff;

	for(int y=0;y<srcLenY;y+=2) {
		int y1=y*NTI_BLKWIDTH;
		int y2=y1;
		if(y<srcLenY-1) y2+=NTI_BLKWIDTH;

		LPBYTE pB=pDstB,pG=pDstG,pR=pDstR;

		for(int x=0;x<srcLenX;x+=2) {
			int x2=(x<srcLenX-1)?(x+1):x;

			b[0]=pSrcB[y1+x];
			b[1]=pSrcB[y1+x2];
			b[2]=pSrcB[y2+x];
			b[3]=pSrcB[y2+x2];
			g[0]=pSrcG[y1+x];
			g[1]=pSrcG[y1+x2];
			g[2]=pSrcG[y2+x];
			g[3]=pSrcG[y2+x2];
			r[0]=pSrcR[y1+x];
			r[1]=pSrcR[y1+x2];
			r[2]=pSrcR[y2+x];
			r[3]=pSrcR[y2+x2];

			WORD bs=0,gs=0,rs=0,nDif=0; //colors different
			for(int i=0;i<4;i++) {
				if(abs(bt-b[i])<et && abs(gt-g[i])<et && abs(rt-r[i])<et) continue;
				nDif++;
				bs+=b[i];
				gs+=g[i];
				rs+=r[i];
			}
			if(nDif) {
				bs=(bs+nDif/2)/nDif;
				gs=(gs+nDif/2)/nDif;
				rs=(rs+nDif/2)/nDif;
			}
			else {
				bs=bt;
				gs=gt;
				rs=rt;
			}
			*pB++=(BYTE)bs;
			*pG++=(BYTE)gs;
			*pR++=(BYTE)rs;
		}
		pDstB+=NTI_BLKWIDTH;
		pDstG+=NTI_BLKWIDTH;
		pDstR+=NTI_BLKWIDTH;
	}
}

static void CopyTransPalRecs(LPBYTE *ppDst,UINT dstOff,LPBYTE *ppSrc,int srcLenX,int srcLenY,LPBYTE pal)
{
	// ppDst[]    - Destination: Ptr to TL corner of a quadrant inside a NTI_BLKWIDTH x NTI_BLKWIDTH record
	// ppSrc[]    - Source: Ptr to srcLenX x srcLenY block, 1/4 of which will be copied
	// pal		  - points to first palette byte
	BYTE b[4],g[4],r[4];

	LPBYTE pSrc=ppSrc[0];
	LPBYTE pDstB=ppDst[0]+dstOff,pDstG=ppDst[1]+dstOff,pDstR=ppDst[2]+dstOff;

	for(int y=0;y<srcLenY;y+=2) {
		int y1=y*NTI_BLKWIDTH;
		int y2=y1;
		if(y<srcLenY-1) y2+=NTI_BLKWIDTH;

		LPBYTE pB=pDstB,pG=pDstG,pR=pDstR;

		for(int x=0;x<srcLenX;x+=2) {
			int x2=(x<srcLenX-1)?(x+1):x;

			int i0=(pSrc[y1+x]<<2);
			int i1=(pSrc[y1+x2]<<2);
			int i2=(pSrc[y2+x]<<2);
			int i3=(pSrc[y2+x2]<<2);

			LPBYTE pPal=pal;
			b[0]=pPal[i0];
			b[1]=pPal[i1];
			b[2]=pPal[i2];
			b[3]=pPal[i3];
			pPal++;
			g[0]=pPal[i0];
			g[1]=pPal[i1];
			g[2]=pPal[i2];
			g[3]=pPal[i3];
			pPal++;
			r[0]=pPal[i0];
			r[1]=pPal[i1];
			r[2]=pPal[i2];
			r[3]=pPal[i3];

			WORD bs=0,gs=0,rs=0,nDif=0; //colors different
			for(int i=0;i<4;i++) {
				if(abs(bt-b[i])<et && abs(gt-g[i])<et && abs(rt-r[i])<et) continue;
				nDif++;
				bs+=b[i];
				gs+=g[i];
				rs+=r[i];
			}
			if(nDif) {
				bs=(bs+nDif/2)/nDif;
				gs=(gs+nDif/2)/nDif;
				rs=(rs+nDif/2)/nDif;
			}
			else {
				bs=bt;
				gs=gt;
				rs=rt;
			}
			*pB++=(BYTE)bs;
			*pG++=(BYTE)gs;
			*pR++=(BYTE)rs;
		}
		pDstB+=NTI_BLKWIDTH;
		pDstG+=NTI_BLKWIDTH;
		pDstR+=NTI_BLKWIDTH;
	}
}

static void CopyQuadrantRecs(LPBYTE *ppDst,UINT dstOff,LPBYTE *ppSrc,int srcLenX,int srcLenY,UINT srcBands)
{
	// ppDst[]    - Destination: Ptr to TL corner of a quadrant inside a NTI_BLKWIDTH x NTI_BLKWIDTH record
	// ppSrc[]    - Source: Ptr to srcLenX x srcLenY block, 1/4 of which will be copied
	// srcBands	  - If 1 (and not grayscale) source bytes are palette indices, else BGR
	//				bytes are directly averaged

	LPBYTE bgr=(srcBands==1)?((LPBYTE)CExportJob::nti.Palette()):NULL;

	if(!bgr) {
		if(bUseTransColor && srcBands==3) {
			CopyTransBgrRecs(ppDst,dstOff,ppSrc,srcLenX,srcLenY);
		}
		else {
			//don't bother with grayscale transparent color in overviews --
			for(UINT b=0;b<srcBands;b++) {
				LPBYTE pDst=ppDst[b]+dstOff;
				LPBYTE pSrc=ppSrc[b];

				for(int y=0;y<srcLenY;y+=2) {
					int y1=y*NTI_BLKWIDTH;
					int y2=y1;
					if(y<srcLenY-1) y2+=NTI_BLKWIDTH;

					LPBYTE p=pDst;	
					for(int x=0;x<srcLenX;x+=2) {
						int x2=(x<srcLenX-1)?(x+1):x;
						*p++=( pSrc[y1+x] + pSrc[y1+x2] +
								pSrc[y2+x] + pSrc[y2+x2] + 2) >> 2;
					}
					pDst+=NTI_BLKWIDTH;
				}
			}
		}
	}
	else {
		if(bUseTransColor) {
			CopyTransPalRecs(ppDst,dstOff,ppSrc,srcLenX,srcLenY,bgr);
		}
		else {
			LPBYTE pSrc=ppSrc[0];
			for(UINT b=0;b<3;b++) {
				LPBYTE pDst=ppDst[b]+dstOff;
				for(int y=0;y<srcLenY;y+=2) {
					int y1=y*NTI_BLKWIDTH;
					int y2=y1;
					if(y<srcLenY-1) y2+=NTI_BLKWIDTH;

					LPBYTE p=pDst;
					for(int x=0;x<srcLenX;x+=2) {
						int x2=(x<srcLenX-1)?(x+1):x;
						*p++=( bgr[pSrc[y1+x]<<2] + bgr[pSrc[y1+x2]<<2] +
								bgr[pSrc[y2+x]<<2] + bgr[pSrc[y2+x2]<<2] + 2) >> 2; 
					}
					pDst+=NTI_BLKWIDTH;
				}
				bgr++;
			}
		}
	}
}

#ifndef SSIM_RGB
static LPBYTE ConvertToGray(CSH_HDRP hp,LPBYTE pDest=NULL)
{
	LPBYTE pD=pDest,pR=hp->Reserved[2], pG=hp->Reserved[1], pB=hp->Reserved[0];
	if(!pD) pD=pDest=pR;
	for(int i=0;i<NTI_BLKWIDTH*NTI_BLKWIDTH;i++) {
		*pD++=(BYTE)(0.2126**pR++ + 0.7152**pG++ + 0.0722**pB++);
	}
	return pDest;
}
#endif

static BOOL ChkAbort(BOOL bExpanding)
{
	return IDYES==AfxMessageBox(bExpanding?IDS_ABORTEXPAND:IDS_ABORTEXPORT,MB_YESNO|MB_ICONQUESTION);
}

UINT CExportJob::DoWork()
{
	pGDAL=NULL;

	nBands=m_pImgLayer->NumBands();
	nColors=m_pImgLayer->NumColorEntries();

	if(bUseTransColor=m_pImgLayer->m_bUseTransColor) {
		DWORD dwTransColor=m_pImgLayer->m_crTransColor;
		rt=(dwTransColor&0xff);
		gt=((dwTransColor>>8)&0xff);
		bt=((dwTransColor>>16)&0xff);
		et=m_bExpanding?(dwTransColor>>24):CImageLayer::GetToleranceError(CExportNTI_Adv::m_Adv.iTolerance);
		if(!et) et++; //require exact matches
	}

	UINT nti_fcn,rec,recMax;
	int nTyp=m_pImgLayer->LayerType();

	bool ssim_computing = false;
	float *ssim_pbuf[SSIM_BUFFERS] = { 0, 0, 0, 0, 0, 0, 0 };
	byte *ssim_srcbuf=NULL;
	ssim_count = 0;

	char parambuf[32];

	if(m_bExpanding) {
		nti=((CNtiLayer *)m_pImgLayer)->GetNtiFile();
		((CNtiLayer *)m_pImgLayer)->NullifyHandle();
		ASSERT(nti.Opened());
		VERIFY(!nti.AttachCache());
		nWidth=m_pImgLayer->Width();
		nHeight=m_pImgLayer->Height();
		nti.SetRecTotal(rec=nti.NumRecs(0));
		recMax=nti.RecordCapacity();
		ASSERT(recMax>rec);
		nti_fcn=nti.XfrFcnID();
		ASSERT(nti_fcn>0);
		if(nti_fcn==NTI_FCNJP2K) {
			LPCSTR p=nti.GetParams();
			if(*p=='L') nti_jp2k_rate=-1.0;
			else if(*p=='B') nti_jp2k_rate=0.0;
			else nti_jp2k_rate=(atof(p)/100.0)*(8*nti.NumOvrBands());
			JP2KAKDataset::KakaduInitialize(); 
		}
		else if(nti_fcn==NTI_FCNWEBP) {
			LPCSTR p,p0=nti.GetParams();
		    for(p=p0;*p && *p!=' '; p++); //skip past first param 
			for(int i=0;i<NTI_WEBP_NUM_PRESETS;i++) {
				int len=strlen(webp_preset[i]);
				if(p-p0==len && !memcmp(p0,webp_preset[i],len)) {
					nti_webp_preset=i;
					break;
				}
			}
			if(*p++==' ') nti_webp_quality=(float)atof(p);
			ASSERT(nti_webp_quality>0.0f);
		}
		START_TIMER(); //Initializes nti_timer0
	}
	else {
		LPCSTR p = trx_Stpext(m_pathName);
		//if(!_stricmp(p,nti_dot_extension)) m_pathName.Truncate(p-(LPCSTR)m_pathName);
		nWidth = crWindow.Width();
		nHeight = crWindow.Height();

		//first portion of szExtra (a null will be added in Create() --
		UINT lenRef = m_pImgLayer->ProjectionRef() ? strlen(m_pImgLayer->ProjectionRef()) : 0;

		ASSERT(!CExportNTI_Adv::m_Adv.bGet_ssim || CExportNTI_Adv::IsLossyMethod() &&
			!m_pImgLayer->NumColorEntries() && nWidth >= NTI_BLKWIDTH && nHeight >= NTI_BLKWIDTH); //dialog should have tested for this

		ssim_computing = CExportNTI_Adv::m_Adv.bGet_ssim!=0;

		UINT lenParam = 0;
		nti_fcn=CExportNTI_Adv::m_Adv.bMethod;
		if(nti_fcn == NTI_FCNJP2K) {
			CString sRate;
			if(!CExportNTI_Adv::GetRateStr(sRate)) {
				ssim_computing=false;
				nti_jp2k_rate = -1.0;
			}
			else {
				nti_jp2k_rate = 0.08 * CExportNTI_Adv::m_Adv.fRate;
			}
			strcpy(parambuf,sRate);
			lenParam = sRate.GetLength();
			ASSERT(lenParam <= 9);
			JP2KAKDataset::KakaduInitialize();
		}
		else if(nti_fcn == NTI_FCNWEBP) {
			ASSERT(CExportNTI_Adv::m_Adv.bPreset < NTI_WEBP_NUM_PRESETS);
			nti_webp_preset = CExportNTI_Adv::m_Adv.bPreset;
			nti_webp_quality = CExportNTI_Adv::m_Adv.fQuality;
			lenParam = _snprintf(parambuf, 10, "%s %s", webp_preset[CExportNTI_Adv::m_Adv.bPreset],
				GetFloatStr(CExportNTI_Adv::m_Adv.fQuality,1));
			ASSERT(lenParam <= 9);
		}

		//space for ssim --
		if (ssim_computing) {
			ASSERT((nti_fcn == NTI_FCNWEBP || nti_fcn == NTI_FCNJP2K));
#ifdef SSIM_RGB
			ssim_total[0] = ssim_total[1] = ssim_total[2] = 0.0;
#else
			ssim_total = 0.0;
#endif
			//space for 14 chars: "  SSIM: 100.00"
			memset(parambuf + lenParam,0,14);
			lenParam += 14;
		}

		char transbuf[16]; //"255,255,200,001"
		*transbuf=0;
		UINT lenTrans=0;
		if(bUseTransColor) {
			lenTrans=sprintf(transbuf,"%u,%u,%u,%u",rt,gt,bt,(et>1)?et:0);
			ASSERT(lenTrans>6 && lenTrans<16);
		}

		if(nti_fcn!=NTI_FCNJP2K && nti_fcn!=NTI_FCNWEBP) {
			switch(CExportNTI_Adv::m_Adv.bFilter) {
				case 0  : nti_fcn|=NTI_FILTERGREEN; break;
				case 1  : nti_fcn|=NTI_FILTERRED; break;
				case 2  : nti_fcn|=NTI_FILTERBLUE;
			}
		}

		//UINT szExtra=lenRef+lenParam+lenTrans+4; //nti.Create() will actually allow ((nSzExtra+3)&~3)

		if(nti.Create(m_pathName,nWidth,nHeight,nBands,nColors,nti_fcn,lenRef+lenParam+lenTrans+4) ||
			nti.AttachCache())
		{
				return exitNTI(trx_Stpnam(m_pathName));
		}

		//Finish initializing header --
		{
			LPSTR pDest=(LPSTR)nti.GetProjectionRef();
			if(lenRef) {
				memcpy(pDest, m_pImgLayer->ProjectionRef(), lenRef);
				pDest+=lenRef;
			}
			*pDest++=0;
			if(lenParam) {
				memcpy(pDest,parambuf,lenParam);
				pDest+=lenParam;
			}
			*pDest++=0;
			if(lenTrans) {
				memcpy(pDest,transbuf,lenTrans);
				pDest+=lenTrans;
			}
			*pDest++=0;
			*pDest=0; //terminate sequence of extra data
		}
	
		//Initialize transform --
		nti.SetTransform(m_pImgLayer->GetTransImg());
		if(m_bExtentWindow && m_pImgLayer->IsTransformed()) {
			nti.UnitsOffX()+=m_pImgLayer->GetTransImg()[1]*crWindow.left;
			nti.UnitsOffY()+=m_pImgLayer->GetTransImg()[5]*crWindow.top;
		}
		nti.SetMetersPerUnit(m_pImgLayer->MetersPerUnit());

		if(m_pImgLayer->IsTransformed()) {
			CWallsMapDoc *pDoc=m_pImgLayer->m_pDoc;
			ASSERT(pDoc);
			if(pDoc->DatumName()) nti.SetDatumName(pDoc->DatumName());
			{
				LPCSTR p=pDoc->UnitsName();
				if(strlen(p)>=NTI_SIZUNITSNAME && !_stricmp(p,"US survey foot")) {
					p="US foot";
				}
				if(pDoc->UnitsName()) nti.SetUnitsName(p);
			}
			nti.SetUTMZone(pDoc->LayerSet().m_iZone);
		}

		if(nColors) {
			memcpy(nti.Palette(),m_pImgLayer->BitmapInfo()->bmiColors,nColors*sizeof(RGBQUAD));
		}

		UINT nSrcRowBytes=nWidth;
		UINT32 uBytesPerSample=1;
		UINT32 nDataType=GDT_Byte;

		if(nTyp==TYP_GDAL && ((CGdalLayer *)m_pImgLayer)->IsScaledType()) {
			uBytesPerSample=((CGdalLayer *)m_pImgLayer)->BytesPerSample();
			nSrcRowBytes*=uBytesPerSample;
			nDataType=((CGdalLayer *)m_pImgLayer)->DataType();
		}

		nSrcRowBytes+=3;  //mult of 4 bytes
		nSrcRowBytes&=~3;

		//Space for reading a complete row of bands of height NTI_BLKWIDTH --
		for(UINT b=0;b<nBands;b++) {
			pDataBuf[b]=(LPBYTE)malloc(NTI_BLKWIDTH*nSrcRowBytes);
			if(!pDataBuf[b]) {
				return exitNTI("Unsufficient memory");
			}
		}

		if(nTyp==TYP_GDAL) {
			CPLPushErrorHandler(GDALErrorHandler);
			ASSERT(CGdalLayer::IsInitialized());
			if(!(pGDAL=(GDALDataset *)GDALOpen(m_pImgLayer->PathName(),GA_ReadOnly))) {
				return exitNTI("Cannot reopen dataset for export");
			}
			CGdalLayer::m_uRasterIOActive=1;
		}

		//Note: nti.NumOvrBands() will be the same as nti.LvlBands(0) unless the image is paletted,
		//in which case zlib will be used for lvl=0 and nti_jp2K_rate will be incremented for lvl>1.
		if(nti_fcn==NTI_FCNJP2K && nti_jp2k_rate>0.0) nti_jp2k_rate*=nti.NumOvrBands();

		//NOTE: Set m_pImgLayer->m_pGDAL=pGDAL to use MapLayer fcns!!!

		rec=0;
		recMax=CExportNTI_Adv::m_bNoOverviews?nti.LvlRec(1):nti.RecordCapacity();

		START_TIMER(); //Initializes nti_timer0

		//Write main level --

#ifdef _SAVE_PAL
		UINT palcount = 0;
#endif

		LPCSTR pError = NULL;

		UINT srcLenY=NTI_BLKWIDTH;

		for(UINT sizY=0;sizY<nHeight;sizY+=srcLenY) {
			if(sizY+srcLenY>nHeight) srcLenY=nHeight-sizY;

			UINT b=0;

			if(nTyp==TYP_GDAL) {
				for(;b<nBands;b++) {
					if(pGDAL->GetRasterBand(nBands-b)->RasterIO(GF_Read,crWindow.left,sizY+crWindow.top,nWidth,srcLenY,
						pDataBuf[b],nWidth,srcLenY,(GDALDataType)nDataType,0,nSrcRowBytes))
							break;
				}
			}
			else {
				//NTI files for now --
				for(;b<nBands;b++) {
					if(((CNtiLayer *)m_pImgLayer)->ReadRasterBand(b,crWindow.left,sizY+crWindow.top,nWidth,srcLenY,pDataBuf[b]))
						break;
				}
			}

			if (b < nBands) {
				pError = "RasterIO failure";
				goto _CleanUp;
			}

			UINT srcLenX=NTI_BLKWIDTH*uBytesPerSample;
			UINT maxOffX=nWidth*uBytesPerSample;
			for (UINT offX = 0; offX<maxOffX; offX += srcLenX) {
				if (offX + srcLenX>maxOffX) srcLenX = maxOffX - offX;
				CSH_HDRP hp = nti.AppendRecord();
				if (!hp) {
					pError = "Append failure";
					goto _CleanUp;
				}
				UINT b;
				for (b = 0; b<nBands; b++) {
					m_pImgLayer->CopyBlock(hp->Reserved[b], pDataBuf[b] + offX, srcLenX, srcLenY, nWidth);
				}
				if (nBands == 3) {
					if (nti.FilterFlag()) FilterBGR(hp);
					if (bUseTransColor && et>1) FixTransColor(hp);
				}
				if (nti.FlushRecord(hp)) {
					pError = "Flush failure";
					goto _CleanUp;
				}
				ASSERT(rec + 1 == nti.NumRecs());

#ifdef _SAVE_PAL
				if(nBands==1 && srcLenY == NTI_BLKWIDTH && srcLenX == NTI_BLKWIDTH*uBytesPerSample) {
					CFile cfpal;
					CString fn;
					fn.Format("%05u.pal",++palcount);
					if (cfpal.Open(fn, CFile::modeWrite | CFile::modeCreate)) {
						cfpal.Write(hp->Buffer, NTI_BLKWIDTH*NTI_BLKWIDTH);
						cfpal.Close();
					}
				}
#endif

				if (ssim_computing  && srcLenY == NTI_BLKWIDTH && srcLenX == NTI_BLKWIDTH*uBytesPerSample) {
					if (!ssim_count) {
						int i;
						for (i = 0; i < SSIM_BUFFERS; i++) {
							if (!(ssim_pbuf[i] = (float *)malloc(SSIM_BUFSIZE))) {
								break;
							}
						}
						if(i<SSIM_BUFFERS || !(ssim_srcbuf=(byte *)malloc(NTI_BLKSIZE))) {
							pError = "Insufficient ram";
							goto _CleanUp;
						}
					}
					ASSERT(rec == hp->Recno);

					//Original version is still in cache -- create grayscale version of original --
					if(nBands>1) ConvertToGray(hp,ssim_srcbuf);
					else memcpy(ssim_srcbuf,hp->Buffer,NTI_BLKSIZE);

					_csh_LockHdr(hp); //keep original version in cache
					hp->Recno = rec+1; //will not be found by GetRecord()
					CSH_HDRP hpnew = nti.GetRecord(rec); //decode compressed version
	
					if (!hpnew) {
						pError = "Decoding error"; //can't happen?
						goto _CleanUp;
					}

					if (nBands>1)
						ssim_total += iqa_ssim(ssim_srcbuf, ConvertToGray(hpnew), NTI_BLKWIDTH, NTI_BLKWIDTH, NTI_BLKWIDTH, 1, 0, ssim_pbuf);
					else
						ssim_total += iqa_ssim(ssim_srcbuf, hp->Buffer, NTI_BLKWIDTH, NTI_BLKWIDTH, NTI_BLKWIDTH, 1, 0, ssim_pbuf);

					_csh_PurgeHdr(hpnew); //discard decoded version
					 hp->Recno--;
					_csh_UnLockHdr(hp); //keep original version in cache for possible use for 1st overview
					ssim_count++;
				}
					
				if(!(++rec%5)) {
					OnProgress(rec,recMax);
					if(m_bAbort) {
						if (ChkAbort(FALSE)) {
							pError = "";
							goto _CleanUp;
						}
						m_bAbort=FALSE;
					}
				}
			}
		}

		ASSERT(rec == nti.NumRecs(0));
		ASSERT(nti.LvlWidth(0) == nWidth);

	_CleanUp:

		if(ssim_count) {
			free(ssim_srcbuf);
			for(int i = 0; i < SSIM_BUFFERS; i++) free(ssim_pbuf[i]);
		}

		for(UINT b=nBands;b;) {
			free(pDataBuf[--b]);
			pDataBuf[b]=NULL;
		}

		if(nTyp==TYP_GDAL) {
			CGdalLayer::m_uRasterIOActive=0;
			GDALClose(pGDAL);
			pGDAL=NULL;
			CPLPopErrorHandler();
			CPLCleanupTLS();
		}

		if (pError)
			return exitNTI((*pError != 0) ? pError : NULL);

		if(CExportNTI_Adv::m_bNoOverviews || nti.NumLevels()<=1) {
			goto _finish;
		}
	}

	//Write Overviews --

	UINT ovrBands=nti.NumOvrBands();
	for(UINT lvl=1;lvl<nti.NumLevels();lvl++) {
		/*
		if(nti_fcn==NTI_FCNJP2K && (lvl>1 || !nti.NumColors()) && nti_jp2k_rate>0.0 && nti_jp2k_rate<NTI_MAX_JP2KRATE*ovrBands) {
			nti_jp2k_rate*=NTI_SCALE_JP2KRATE;
			if(nti_jp2k_rate>NTI_MAX_JP2KRATE*ovrBands) nti_jp2k_rate=NTI_MAX_JP2KRATE*ovrBands;
		}
		*/
		ASSERT(rec==nti.LvlRec(lvl));
		UINT lvl_height=nti.LvlHeight(lvl);
		UINT lvl_width=nti.LvlWidth(lvl);

		for(UINT offY=0;offY<lvl_height;offY+=NTI_BLKWIDTH) {
			UINT lvl_sizY=lvl_height-offY;
			if(lvl_sizY>NTI_BLKWIDTH) lvl_sizY=NTI_BLKWIDTH;

			for(UINT offX=0;offX<lvl_width;offX+=NTI_BLKWIDTH) {
				LPBYTE ppDst[3],ppSrc[3];
				CSH_HDRP hp=nti.AppendRecord();
				if(!hp) return exitNTI("Append failure");
				for(UINT b=0;b<ovrBands;b++) {
					ppDst[b]=hp->Reserved[b];
				}
				UINT srcLenX,srcLenY,srcBands;

				if(srcBands=GetQuadrantRecs(ppSrc,lvl,offX,offY,&srcLenX,&srcLenY)) {
					//copy TL
					if(srcBands==(UINT)-1) return exitNTI("Block access");
					CopyQuadrantRecs(ppDst,0,ppSrc,srcLenX,srcLenY,srcBands);
				}

				ASSERT(srcBands);

				if(srcBands=GetQuadrantRecs(ppSrc,lvl,offX+(NTI_BLKWIDTH>>1),offY,&srcLenX,&srcLenY)) {
					//copy TR
					if(srcBands==(UINT)-1) return exitNTI("Block access");
					CopyQuadrantRecs(ppDst,(NTI_BLKWIDTH>>1),ppSrc,srcLenX,srcLenY,srcBands);
				}

				if(srcBands=GetQuadrantRecs(ppSrc,lvl,offX,offY+(NTI_BLKWIDTH>>1),&srcLenX,&srcLenY)) {
					//copy TR
					if(srcBands==(UINT)-1) return exitNTI("Block access");
					CopyQuadrantRecs(ppDst,NTI_BLKWIDTH*(NTI_BLKWIDTH>>1),ppSrc,srcLenX,srcLenY,srcBands);
				}

				if(srcBands=GetQuadrantRecs(ppSrc,lvl,offX+(NTI_BLKWIDTH>>1),offY+(NTI_BLKWIDTH>>1),&srcLenX,&srcLenY)) {
					//copy TR
					if(srcBands==(UINT)-1) return exitNTI("Block access");
					CopyQuadrantRecs(ppDst,(NTI_BLKWIDTH+1)*(NTI_BLKWIDTH>>1),ppSrc,srcLenX,srcLenY,srcBands);
				}

				//NOTE: There should now be at least 15 blocks in the image cache --

				if(ovrBands==3 && nti.FilterFlag()) FilterBGR(hp);

				if(nti.FlushRecord(hp)) {
					return exitNTI("Flush failure");
				}
				ASSERT(hp->Recno==rec);
				if(!(++rec%5)) {
					OnProgress(rec,recMax);
					if(m_bAbort) {
						if(ChkAbort(m_bExpanding)) return exitNTI(NULL);
						m_bAbort=FALSE;
					}
				}
			}
		}
	}

	ASSERT(rec==recMax && rec==nti.NumRecs());

	//We've transfomed the contents of lower-level blocks, each of which
	//were accessed (and transformed) only once. We must purge the cache
	//so that subsequent GetRecord() calls produce untransformed data.
	
_finish:
	nti.PurgeCache();

	nti.EncodeFcn()(NULL,1); //Encode uninitialize!

	if(ssim_count) {
#ifdef SSIM_RGB
		for (int i = 0; i < 3; i++) {
			ssim_total[i]*=(100.0/ssim_count);
		}
		double s = 0.2126*ssim_total[2] + 0.7152*ssim_total[1] + 0.0722*ssim_total[0];
#else
		double s = ssim_total *= (100.0 / ssim_count);
#endif
		if (s < 0.0) s = 0.0; else if (s>100.0) s = 100.0;
		UINT len=sprintf(parambuf,"  SSIM: %.2f  ",s);
		ASSERT(len>=14 && len<=16);
		LPSTR p=(LPSTR)nti.GetParams();
		p+=strlen(p);
		memcpy(p,parambuf,14); //already null terminated, trans color section may follow
		ASSERT(*(p+14)==0);
	}

	if(nti.FlushFileHdr()) {
		return exitNTI("Failure writing NTI header");  //Version still set to 0 -- file not complete
	}

	if(m_bExpanding) {
		VERIFY(!nti.Close());
	}
	else {
		VERIFY(!dos_FlushFileBuffers(nti.DosHandle())); //So size will be computed correctly!
	}

	STOP_TIMER();

	OnProgress(rec,rec);
	OnProgress(0);

	if(nTyp==TYP_GDAL) {
		CPLCleanupTLS();
	}

	return 0;
}

#include "stdafx.h"
#include "resource.h"
#include "WallsMapDoc.h"
#include "DlgSymbolsImg.h"
#include "utility.h"
#include <sys/stat.h>
#include <io.h>
#include <math.h>
#include <ogr_spatialref.h>
#include "MapLayer.h"

//=====================================================================
//CImageLayer operations --

CDlgSymbolsImg * CImageLayer::m_pDlgSymbolsImg=NULL;


CImageLayer::~CImageLayer()
{
	if(m_pDlgSymbolsImg && m_pDlgSymbolsImg->m_pLayer==this)
		m_pDlgSymbolsImg->DestroyWindow();
	DeleteMask();
	free(m_lpBitmapInfo);
}

//static bool __cdecl CImageLayer::ComputeLevelLimit(unsigned char &,unsigned int,unsigned int)
bool CImagelLayer::ComputeLevelLimit(BYTE &bLevelLimit,UINT32 uWidth,UINT32 uHeight)
{
	bLevelLimit=0;
	UINT32 maxDim=max(uWidth,uHeight)*2;
	UINT32 minDim=min(uWidth,uHeight)*2; 
	for(UINT shf=0;shf<MaxLevels;shf++) {
		if((maxDim=((maxDim+1)>>1))<=BlkWidth) break;
		if(!(minDim=((minDim+1)>>1))) return false;
		bLevelLimit++;
	}
	return true;
}

void CImageLayer::AppendGeoRefInfo(CString &cs) const
{
	if(m_bTrans) {
		cs+="\r\nCoordinate system: ";
		if(m_bProjected) {
			ASSERT(m_iZone);
			if(abs(m_iZone)<=60)	{
				cs.AppendFormat("UTM, Zone %u%c",abs(m_iZone),(m_iZone<0)?'S':'N');
			}
			else {
				OGRSpatialReference oSRS;
				LPCSTR p=(LPSTR)m_projectionRef;
				if(p && oSRS.importFromWkt((LPSTR *)&p)==OGRERR_NONE && (p=oSRS.GetAttrValue("PROJECTION",0)))
					cs+=p;
				else
					cs+="Unknown projection";
			}
		}
		else cs+="Geographical (Lat/Long)";
		cs.AppendFormat("  Units: %s  Datum: %s\r\nOrigin: (%.10g,%.10g)  Pixel Size: (%.10g,%.10g)",
			m_unitsName,
			m_datumName?m_datumName:"Unspecified",
			m_fTransImg[0], m_fTransImg[3],
			m_fTransImg[1], m_fTransImg[5]);
	}
	else cs+="Image is not georeferenced.";
	cs+="\r\n\r\n";
}

void CImageLayer::InitSizeStr()
{
	m_csSizeStr.Format("%ux%u",m_uWidth,m_uHeight);
}

// CDlgProperties message handlers

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

bool CImageLayer::AppendProjectionRefInfo(CString &s) const
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

void CImageLayer::GetNameAndSize(CString &s,LPCSTR path,bool bDate)
{
   struct __stat64 status;

   if(!_stat64(path,&status)) {
		char buf[64];
		LPCSTR p0,p1;
		if(bDate) p0=p1="";
		else {
			p0=trx_Stpnam(path);
			p1="\n";
		}

		s.Format("%s%sSize: %s KB (%ux%ux%u)",p0,p1,
			CommaSep(buf,status.st_size/1024),
			Width(),Height(),Bands()*BitsPerSample());

	   if(bDate) {
		   s+="  Modified: ";
		   s+=GetTimeStr(_localtime64(&status.st_mtime));
	   }
   }
}

void CImageLayer::GeoRectToImgRect(const CFltRect &geoExt,CRect &imgRect) const
{
	CFltRect viewExt=geoExt;
	GeoPtToImgPt(viewExt.tl);
	GeoPtToImgPt(viewExt.br);
	if(viewExt.l<0.0) viewExt.l=0.0;
	if(viewExt.r>(double)m_uWidth) viewExt.r=m_uWidth;
	if(viewExt.t<0.0) viewExt.t=0.0;
	if(viewExt.b>(double)m_uHeight) viewExt.b=m_uHeight;
	imgRect.SetRect((int)floor(viewExt.l),(int)floor(viewExt.t),
		(int)ceil(viewExt.r),(int)ceil(viewExt.b));
}

void CImageLayer::ApplyBGRE()
{
	BYTE r=m_crTransColor&0xff,g=(m_crTransColor>>8)&0xff,b=(m_crTransColor>>16)&0xff,e=(m_crTransColor>>24);
	ASSERT(e>1);
	UINT lenX=m_dib.GetWidth();
	UINT rowbytes=(3*lenX+3)&~3;
	LPBYTE pBase=(BYTE *)m_dib.GetDIBits();
	LPBYTE pBaseLimit=pBase+rowbytes*m_dib.GetHeight();
	for(;pBase<pBaseLimit;pBase+=rowbytes) {
		LPBYTE p=pBase;
		for(UINT x=0;x<lenX;x++) {
			//replace close bgr matches --
			if(abs(b-p[0])<e && abs(g-p[1])<e && abs(r-p[2])<e) {
				*p++=b;
				*p++=g;
				*p++=r;
			}
			else p+=3;
		}
	}
}

void CImageLayer::DeleteMask()
{
	if(!m_hdcMask) return;
	::SelectObject(m_hdcMask,m_hbmMaskOrg);
	VERIFY(::DeleteDC(m_hdcMask));
	m_mask.DeleteObject();
	m_hdcMask=NULL;
	m_crTransColorMask=0;
}

void CImageLayer::CreateMask(HDC hdc)
{
	int width=m_dib.GetWidth();
	int height=m_dib.GetHeight();

	//The mask will have border white (1) and interior black (0)
	ASSERT(!m_hdcMask);
	VERIFY(m_hdcMask=::CreateCompatibleDC(hdc));
	VERIFY(m_mask.CreateBitmap(width,height,1,1,NULL));
	VERIFY(m_hbmMaskOrg=(HBITMAP)::SelectObject(m_hdcMask,(HBITMAP)m_mask));

	m_crTransColorMask=m_crTransColor;
	COLORREF bkclr=SetBkColor(hdc,m_crTransColor&0xffffff); //Background color in rgb bitmap mapped to 1 in mask
	VERIFY(::BitBlt(m_hdcMask,0,0,width,height,hdc,0,0,SRCCOPY));

	//By XOR'ing mask against m_dib, turn transparent color to black (0) in m_dib (hdc) without affecting other colors --
	if((m_crTransColor&0xffffff)!=RGB(0,0,0)) {
		VERIFY(::BitBlt(hdc,0,0,width,height,m_hdcMask,0,0,SRCINVERT)); //XOR
	}
	::SetBkColor(hdc,bkclr);
}

void CImageLayer::ApplyMask(HDC destDC,int destOffX,int destOffY,int destWidth,int destHeight)
{
	//Condition the destDC bitmap for OR'ing the source (m_dib) bitmap to it via SRC_PAINT.
	//The mask has already been used to turn the transparent color of source to black (or 0) --
	COLORREF clrbk=::SetBkColor(destDC,RGB(255, 255, 255));    // 0xFFFFFF (border portion of mask)
	COLORREF clrtx=::SetTextColor(destDC,RGB(0, 0, 0));        // 0x000000 (interior portion of mask)
	//Black out (set to zero) the non-transparent (interior) pixels of pDest, leaving the border pixels unchanged --
	::StretchBlt(destDC,destOffX,destOffY,destWidth,destHeight,m_hdcMask,0,0,m_dib.GetWidth(),m_dib.GetHeight(),SRCAND);
	::SetTextColor(destDC,clrtx);
	::SetBkColor(destDC,clrbk); 
}

bool CImageLayer::CopyData(CDIBWrapper *pDestDIB,int destOffX,int destOffY,double fZoom)
{
	int destWidth=(int)ceil(fZoom*m_fCellRatioX*m_dib_rect.Width());
	int destHeight=(int)ceil(fZoom*m_fCellRatioY*m_dib_rect.Height());
	
	int srcWidth=m_dib.GetWidth();
	int srcHeight=m_dib.GetHeight();

	HDC destDC=pDestDIB->GetMemoryHDC();
	HDC srcDC=m_dib.GetMemoryHDC(destDC);

	bool bRet=false;

	if(m_bUseTransColor || m_bAlpha!=0xFF) {
		BLENDFUNCTION bf;

		HDC hdcTemp=NULL;
		HBITMAP hbmTemp=NULL,hbmTempOld=NULL;
		//construct a bitmap with source already alphablended with the destination --
		hdcTemp=::CreateCompatibleDC(destDC); //srcDC may have an 8-bit bitmap!
		hbmTemp=::CreateCompatibleBitmap(destDC,srcWidth,srcHeight); //initially a black bitmap
		hbmTempOld=(HBITMAP)::SelectObject(hdcTemp,hbmTemp);

		if(m_bAlpha!=0xFF) {
			VERIFY(::StretchBlt(hdcTemp,0,0,srcWidth,srcHeight,destDC,destOffX,destOffY,destWidth,destHeight,SRCCOPY));
		}

		if(m_bUseTransColor) {
			//zero interior of destination, which is no longer needed --
			if(!m_hdcMask) CreateMask(srcDC);
			ApplyMask(destDC,destOffX,destOffY,destWidth,destHeight);
		}

		if(m_bAlpha!=0xFF) {
			bf.BlendOp = AC_SRC_OVER;
			bf.BlendFlags = 0;
			bf.SourceConstantAlpha = m_bAlpha;  // 0x7f (127) = half of 0xff = 50% transparency
			bf.AlphaFormat = 0; //(ignore source alpha channel, otherwise AC_SRC_ALPHA, which supposedly has failed under Win7)
			VERIFY(::AlphaBlend(hdcTemp,0,0,srcWidth,srcHeight,srcDC,0,0,srcWidth,srcHeight,bf)); //alphablended borders!
		}
		else {
			VERIFY(::BitBlt(hdcTemp,0,0,srcWidth,srcHeight,srcDC,0,0,SRCCOPY));
		}

		if(m_bUseTransColor) {
			//Black out the possibly blended border of hdcTemp in preparation for a SRCPAINT --
			VERIFY(::PatBlt(m_hdcMask,0,0,srcWidth,srcHeight,DSTINVERT));
			VERIFY(::BitBlt(hdcTemp,0,0,srcWidth,srcHeight,m_hdcMask,0,0,SRCAND));
			VERIFY(::PatBlt(m_hdcMask,0,0,srcWidth,srcHeight,DSTINVERT)); //restore mask
			//then OR the remaining portion (border) of destDC to hdcTemp --
			VERIFY(::StretchBlt(hdcTemp,0,0,srcWidth,srcHeight,destDC,destOffX,destOffY,destWidth,destHeight,SRCPAINT));
		}

		//Finally stretch the possibly alphablended hdcTemp via SRCCOPY and HALFTONE, the latter not effective with SRCPAINT!!
		int iPrevMode=::SetStretchBltMode(destDC,(fZoom>1.0 && !(m_pDoc->m_uEnableFlags&NTL_FLG_AVERAGE))?COLORONCOLOR:HALFTONE);
		if(::StretchBlt(destDC,destOffX,destOffY,destWidth,destHeight,hdcTemp,0,0,srcWidth,srcHeight,SRCCOPY))
			bRet=true; //OK!!!
		::SetStretchBltMode(destDC,iPrevMode);

		VERIFY(::DeleteObject(::SelectObject(hdcTemp,hbmTempOld)));
		VERIFY(::DeleteDC(hdcTemp));
		
#if 0
		else {
			//Alpha only
#if 1
			HDC hdcTemp=NULL;
			HBITMAP hbmTemp=NULL,hbmTempOld=NULL;
			//construct a bitmap with source already alphablended with the destination --
			hdcTemp=::CreateCompatibleDC(destDC); //srcDC may have an 8-bit bitmap!
			hbmTemp=::CreateCompatibleBitmap(destDC,srcWidth,srcHeight); //initially a black bitmap
			hbmTempOld=(HBITMAP)::SelectObject(hdcTemp,hbmTemp);

			VERIFY(::StretchBlt(hdcTemp,0,0,srcWidth,srcHeight,destDC,destOffX,destOffY,destWidth,destHeight,SRCCOPY));
			VERIFY(::AlphaBlend(hdcTemp,0,0,srcWidth,srcHeight,srcDC,0,0,srcWidth,srcHeight,bf)); //alphablended borders!
			//OR the alpablended source with zero borders to destination --
			int iPrevMode=::SetStretchBltMode(destDC,(fZoom>1.0 && !(m_pDoc->m_uEnableFlags&NTL_FLG_AVERAGE))?COLORONCOLOR:HALFTONE);
			if(::StretchBlt(destDC,destOffX,destOffY,destWidth,destHeight,hdcTemp,0,0,srcWidth,srcHeight,SRCCOPY))
				bRet=true; //OK!!!
			::SetStretchBltMode(destDC,iPrevMode);
			VERIFY(::DeleteObject(::SelectObject(hdcTemp,hbmTempOld)));
			VERIFY(::DeleteDC(hdcTemp));
			bRet=true;
#else
			if(::AlphaBlend(destDC,destOffX,destOffY,destWidth,destHeight,srcDC,0,0,srcWidth,srcHeight,bf))	bRet=true;
#endif
		}
#endif
	}
	else {
		int iPrevMode=::SetStretchBltMode(destDC,(fZoom>1.0 && !(m_pDoc->m_uEnableFlags&NTL_FLG_AVERAGE))?COLORONCOLOR:HALFTONE);
		if(::StretchBlt(destDC,destOffX,destOffY,destWidth,destHeight,srcDC,0,0,srcWidth,srcHeight,m_hdcMask?SRCPAINT:SRCCOPY))
		    bRet=true;
		::SetStretchBltMode(destDC,iPrevMode);
	}
	return bRet;
}

#include "stdafx.h"
#include "resource.h"
#include "WallsMapDoc.h"
#include "DlgSymbolsImg.h"
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
bool CImageLayer::ComputeLevelLimit(BYTE &bLevelLimit,UINT32 uWidth,UINT32 uHeight)
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
				cs+=m_projName?m_projName:"Unknown projection";
				if(m_projName) cs+=" (Unsupported)";
			}
		}
		else cs+="Geographical (Lat/Long)";
		if(m_datumName) cs.AppendFormat("  Datum: %s",m_datumName);
		cs.AppendFormat("\r\nUnits: %s  Origin: (%.10g,%.10g)\r\nPixel Size: (%.10g,%.10g)",
			m_unitsName,
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
	//hdc is m_dib's HDC
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

void CImageLayer::ApplyMask(HDC destDC,int destOffX,int destOffY,int destWidth,int destHeight,
	int srcOffX,int srcOffY,int srcWidth,int srcHeight)
{
	//Condition the destDC bitmap for OR'ing the source (m_dib) bitmap to it via SRC_PAINT.
	//The mask has already been used to turn the transparent color of source to black (or 0) --
	COLORREF clrbk=::SetBkColor(destDC,RGB(255, 255, 255));    // 0xFFFFFF (border portion of mask)
	COLORREF clrtx=::SetTextColor(destDC,RGB(0, 0, 0));        // 0x000000 (interior portion of mask)
	//Black out (set to zero) the non-transparent (interior) pixels of pDest, leaving the border pixels unchanged --
	::StretchBlt(destDC,destOffX,destOffY,destWidth,destHeight,m_hdcMask,srcOffX,srcOffY,srcWidth,srcHeight,SRCAND);
	::SetTextColor(destDC,clrtx);
	::SetBkColor(destDC,clrbk); 
}

bool CImageLayer::CopyData(CDIBWrapper *pDestDIB,int destOffX,int destOffY,int destWidth,int destHeight,
		int srcOffX,int srcOffY,int srcWidth,int srcHeight)
{
	#undef _USE_ALPHA_TEMP

	HDC destDC=pDestDIB->GetMemoryHDC();	
	HDC srcDC=m_dib.GetMemoryHDC(destDC);

	bool bRet=false;
	bool bZoomedIn=destWidth>srcWidth;
	bool bCOC=(bZoomedIn && !(m_pDoc->m_uEnableFlags&NTL_FLG_AVERAGE));

	BLENDFUNCTION bf;
	if(m_bAlpha!=0xFF) {
		bf.BlendOp = AC_SRC_OVER;
		bf.BlendFlags = 0;
		bf.SourceConstantAlpha = m_bAlpha;  // 0x7f (127) = half of 0xff = 50% transparency
		bf.AlphaFormat = 0; //(ignore source alpha channel, otherwise AC_SRC_ALPHA, which supposedly has failed under Win7)
	}

	if(m_bUseTransColor
#ifdef _USE_ALPHA_TEMP		
		|| m_bAlpha!=0xFF
#endif
	  )
	{
		HDC hdcTemp=NULL;
		HBITMAP hbmTemp=NULL,hbmTempOld=NULL;
		//construct a bitmap with source already alphablended with the destination --
		hdcTemp=::CreateCompatibleDC(destDC); //srcDC may have an 8-bit bitmap!
		hbmTemp=::CreateCompatibleBitmap(destDC,destWidth,destHeight); //initially a black bitmap
		hbmTempOld=(HBITMAP)::SelectObject(hdcTemp,hbmTemp);

		if(m_bUseTransColor && !m_hdcMask) {
			//The 1-bit mask of size m_dib.Width(),m_dib.Height() will have border white (1) and interior black (0)
			//This also XOR's mask against srcDC, turning transparent color to black (0) without affecting other colors --
			CreateMask(srcDC);
		}

		if(m_bAlpha!=0xFF) {
			VERIFY(::BitBlt(hdcTemp,0,0,destWidth,destHeight,destDC,destOffX,destOffY,SRCCOPY));
			VERIFY(::AlphaBlend(hdcTemp,0,0,destWidth,destHeight,srcDC,srcOffX,srcOffY,srcWidth,srcHeight,bf)); //alphablended borders!
			//hdcTemp now has an blended but unsmooth copy of the replacement bitmap, but with a blended border
		}
		else {
			//m_bUseTransColor==true
			int iPrevMode=::SetStretchBltMode(hdcTemp,bCOC?COLORONCOLOR:HALFTONE);
			VERIFY(::StretchBlt(hdcTemp,0,0,destWidth,destHeight,srcDC,srcOffX,srcOffY,srcWidth,srcHeight,SRCCOPY));
			::SetStretchBltMode(hdcTemp,iPrevMode);
			//hdcTemp now has smooth copy of the replacement bitmap, but with an unblended border
		}

		if(m_bUseTransColor) {
			//Using mask with StretchBlt with SRCAND, black out (set to zero) the non-transparent (interior) pixels
			//of hdcDest, leaving the border pixels unchanged --
			ApplyMask(destDC,destOffX,destOffY,destWidth,destHeight,srcOffX,srcOffY,srcWidth,srcHeight);

			//Black out the possibly blended border of hdcTemp in preparation for a SRCPAINT --
			VERIFY(::PatBlt(m_hdcMask,0,0,m_dib.GetWidth(),m_dib.GetHeight(),DSTINVERT));
			VERIFY(::StretchBlt(hdcTemp,0,0,destWidth,destHeight,m_hdcMask,srcOffX,srcOffY,srcWidth,srcHeight,SRCAND));
			VERIFY(::PatBlt(m_hdcMask,0,0,m_dib.GetWidth(),m_dib.GetHeight(),DSTINVERT)); //restore mask
		}

		//Finally stretch the possibly alphablended hdcTemp via SRCCOPY and HALFTONE, the latter not effective with SRCPAINT!!
		//int iPrevMode=::SetStretchBltMode(destDC,(fZoom>1.0 && !(m_pDoc->m_uEnableFlags&NTL_FLG_AVERAGE))?COLORONCOLOR:HALFTONE);
		int iPrevMode=::SetStretchBltMode(destDC,
				(m_bUseTransColor || bCOC)?COLORONCOLOR:HALFTONE);
		if(::StretchBlt(destDC,destOffX,destOffY,destWidth,destHeight,hdcTemp,0,0,
				destWidth,destHeight,m_bUseTransColor?SRCPAINT:SRCCOPY))
			bRet=true; //OK!!!
		::SetStretchBltMode(destDC,iPrevMode);

		VERIFY(::DeleteObject(::SelectObject(hdcTemp,hbmTempOld)));
		VERIFY(::DeleteDC(hdcTemp));
		
	}
	else {
		#ifndef _USE_ALPHA_TEMP		
		if(m_bAlpha!=0xFF) {
			//Alpha only
			if(::AlphaBlend(destDC,destOffX,destOffY,destWidth,destHeight,srcDC,srcOffX,srcOffY,srcWidth,srcHeight,bf))
				bRet=true;
		}
		else
		#endif
		{
			int iPrevMode=::SetStretchBltMode(destDC,bCOC?COLORONCOLOR:HALFTONE);
			if(::StretchBlt(destDC,destOffX,destOffY,destWidth,destHeight,srcDC,srcOffX,srcOffY,srcWidth,srcHeight,m_hdcMask?SRCPAINT:SRCCOPY))
				bRet=true;
			::SetStretchBltMode(destDC,iPrevMode);
		}
	}
	return bRet;
}

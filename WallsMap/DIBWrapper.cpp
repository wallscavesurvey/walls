// CDIBWrapper.cpp : implementation file

#include "stdafx.h"
#include "DIBWrapper.h"

/////////////////////////////////////////////////////////////////////////////
// CE DIBSection global functions

/////////////////////////////////////////////////////////////////////////////
// CDIBWrapper

CDIBWrapper::CDIBWrapper()
{
	m_pRT = NULL;
	m_pBR = NULL;
	m_hBitmap = NULL;
	m_hOldBitmap = NULL;
}

CDIBWrapper::~CDIBWrapper()
{
	DeleteObject();
	SafeRelease(&m_pRT);
	SafeRelease(&m_pBR);
}

ID2D1DCRenderTarget * CDIBWrapper::InitRT()
{
	if ((!m_pRT && 0 > d2d_pFactory->CreateDCRenderTarget(&d2d_props, &m_pRT)) ||
		0 > m_pRT->BindDC(m_MemDC, CRect(0, 0, m_iWidth, m_iHeight)))
	{
		SafeRelease(&m_pRT);
		SafeRelease(&m_pBR);
		ASSERT(0);
		return NULL;
	}
	//reset transform --
	m_pRT->SetTransform(D2D1::Matrix3x2F::Identity());
	return m_pRT;
}

ID2D1SolidColorBrush * CDIBWrapper::InitBR(COLORREF rgb, float opacity)
{
	ASSERT(m_pRT);
	if (!m_pBR) {
		if (0 > m_pRT->CreateSolidColorBrush(RGBAtoD2D(rgb, opacity), &m_pBR)) {
			return NULL;
		}
	}
	else m_pBR->SetColor(RGBAtoD2D(rgb, opacity));
	return m_pBR;
}


// --- In  :
// --- Out :
// --- Returns :
// --- Effect : Resets the object to an empty state, and frees all memory used.
void CDIBWrapper::DeleteObject()
{
	// Unselect the bitmap out of the memory DC before deleting bitmap
	ReleaseMemoryDC();
	if (m_hBitmap) ::DeleteObject(m_hBitmap);
	m_hBitmap = NULL;
	m_ppvBits = NULL;
	SafeRelease(&m_pRT);
	SafeRelease(&m_pBR);
	m_iHeight = m_iWidth = 0;
	m_iColorTableSize = 0;
}

// --- In  : pDC   - Points to destination device context
//           (x,y) - point at which the topleft corner of the image is drawn
// --- Out :
// --- Returns : TRUE on success
// --- Effect : Draws the image 1:1 on the device context
BOOL CDIBWrapper::Draw(HDC destDC, int x, int y)
{
	if (!m_hBitmap) return FALSE;
	BOOL bResult = ::BitBlt(destDC, x, y, GetWidth(), GetHeight(), GetMemoryHDC(destDC), 0, 0, SRCCOPY);
	return bResult;
}

BOOL CDIBWrapper::Fill(HBRUSH hbr)
{
	if (!m_hBitmap) return FALSE;

	BOOL bResult = FALSE;

	// Create a memory DC compatible with the destination DC
	CDC* pMemDC = GetMemoryDC();
	if (!pMemDC) return FALSE;

	HBRUSH hbrOld = (HBRUSH)pMemDC->SelectObject(hbr);
	bResult = pMemDC->BitBlt(0, 0, GetWidth(), GetHeight(), NULL, 0, 0, PATCOPY);
	pMemDC->SelectObject(hbrOld);

	return bResult;
}

// --- In  : pDestDC - Points to destination device context
//           (x,y)   - point at which the topleft corner of the image is drawn
//           (cx,cy) - size to stretch the image
// --- Out : Returns : TRUE on success
BOOL CDIBWrapper::Stretch(HDC destDC, int x, int y, int cx, int cy, int mode)
{
	if (!m_hBitmap) return FALSE;

	int iPrevMode = ::SetStretchBltMode(destDC, mode);
	//::SetBrushOrgEx(pDestDC->m_hDC,0,0,NULL);
	BOOL bResult = ::StretchBlt(destDC, x, y, cx, cy, GetMemoryHDC(destDC), 0, 0, GetWidth(), GetHeight(), SRCCOPY);
#ifdef _DEBUG
	if (!bResult) {
		LastErrorBox();
	}
#endif
	::SetStretchBltMode(destDC, iPrevMode);
	return bResult;
}

// --- In  : pDC - device context to use when calling CreateCompatibleDC
// --- Out :
// --- Returns : A pointer to a memory DC
// --- Effect : Creates a memory DC and selects in the current bitmap so it can be
//              modified using the GDI functions. Only one memDC can be created for
//              a given CDIBWrapper object. If you have a memDC but wish to recreate it
//              as compatible with a different DC, then call ReleaseMemoryDC first.
//              If the memory DC has already been created then it will be recycled.
//              Note that if using this in an environment where the color depth of
//              the screen may change, then you will need to set "m_bReuseMemDC" to FALSE
CDC* CDIBWrapper::GetMemoryDC(CDC* pDC /*=NULL*/)
{
	if (m_MemDC.GetSafeHdc())   // Already created?
	{
		// Flush the GDI batch queue 
		GdiFlush();
		return &m_MemDC;
	}

	// Create a memory DC compatible with the given DC
	if (!m_MemDC.CreateCompatibleDC(pDC)) return NULL;

	// Select in the bitmap
	m_hOldBitmap = (HBITMAP) ::SelectObject(m_MemDC.GetSafeHdc(), m_hBitmap);

	// Flush the GDI batch queue 
	GdiFlush();

	return &m_MemDC;
}

HDC CDIBWrapper::GetMemoryHDC(HDC dc/*dc=NULL*/)
{
	HDC hdc = m_MemDC.m_hDC;
	if (hdc)   // Already created?
	{
		// Flush the GDI batch queue 
		GdiFlush();
		return hdc;
	}

	// Create a memory DC compatible with the given DC
	if (!(hdc = ::CreateCompatibleDC(dc))) return NULL;
	VERIFY(m_MemDC.Attach(hdc));

	// Select in the bitmap
	m_hOldBitmap = (HBITMAP) ::SelectObject(hdc, m_hBitmap);

	// Flush the GDI batch queue 
	GdiFlush();

	return hdc;
}

// --- Returns : TRUE on success
// --- Effect : Selects out the current bitmap and deletes the mem dc. If bForceRelease 
//              is FALSE, then the DC release will not actually occur. This is provided 
//              so you can have
//
//                 GetMemoryDC(...)
//                 ... do something
//                 ReleaseMemoryDC()
//
//               bracketed calls. If m_bReuseMemDC is subsequently set to FALSE, then 
//               the same code fragment will still work.
BOOL CDIBWrapper::ReleaseMemoryDC()
{
	if (!m_MemDC.GetSafeHdc())
		return TRUE; // Nothing to do

	// Flush the GDI batch queue 
	GdiFlush();

	// Select out the current bitmap
	if (m_hOldBitmap) ::SelectObject(m_MemDC.GetSafeHdc(), m_hOldBitmap);
	m_hOldBitmap = NULL;

	// Delete the memory DC
	VERIFY(m_MemDC.DeleteDC());
	m_MemDC.m_hDC = NULL;
	return TRUE;
}

// --- In  : lpBitmapInfo - pointer to a BITMAPINFO structure
// --- Out :
// --- Returns : Returns TRUE on success, FALSE otherwise
// --- Effect : Initialises the bitmap using the information in lpBitmapInfo to determine
//              the dimensions and colors, and the then sets the bits from the bits in
//              lpBits. If failure, then object is initialised back to an empty bitmap.
BOOL CDIBWrapper::InitBitmap(LPBITMAPINFO lpBitmapInfo)
{
	DeleteObject();

	if (!lpBitmapInfo) return FALSE;

	HDC hDC = NULL;
	TRY{
		BITMAPINFOHEADER& bmih = lpBitmapInfo->bmiHeader;

	// Compute the number of colors in the color table
	//m_iColorTableSize = NumColorEntries(bmih.biBitCount, bmih.biCompression, bmih.biClrUsed);
	m_iColorTableSize = bmih.biClrUsed;

	//DWORD dwBitmapInfoSize = sizeof(BITMAPINFOHEADER) + m_iColorTableSize*sizeof(RGBQUAD);

	// Copy over BITMAPINFO contents
	//memcpy(&m_DIBinfo, lpBitmapInfo, dwBitmapInfoSize);

	// Should now have all the info we need to create the sucker.
	//TRACE(_T("Width %d, Height %d, Bits/pixel %d, Image Size %d\n"),
	//      bmih.biWidth, bmih.biHeight, bmih.biBitCount, bmih.biSizeImage);

	// Create a DC which will be used to get DIB, then create DIBsection
	hDC = ::GetDC(NULL);
	if (!hDC)
	{
		TRACE0("Unable to get DC\n");
		AfxThrowResourceException();
	}

	m_iBitCount = bmih.biBitCount;
	m_iHeight = abs(bmih.biHeight);
	m_iWidth = bmih.biWidth;

	if (!bmih.biSizeImage) {
		 bmih.biSizeImage = GetImageSize();
	}

	m_hBitmap = CreateDIBSection(hDC, lpBitmapInfo, DIB_RGB_COLORS, &m_ppvBits, NULL, 0);
	if (!m_hBitmap)
	{
		TRACE0("CreateDIBSection failed\n");
		AfxThrowResourceException();
	}

	// Flush the GDI batch queue 
	GdiFlush();
#ifdef _DEBUG
		if (!GetMemoryHDC(hDC)) {
			TRACE0("GetMemoryDC failed\n");
			AfxThrowResourceException();
		}
#endif
		::DeleteObject(hDC);
		hDC = NULL;
	}
		CATCH(CException, e)
	{
		e->Delete();
#ifdef _DEBUG
		_TraceLastError();
#endif
		if (hDC) ::DeleteObject(hDC);
		DeleteObject();
		return FALSE;
	}
	END_CATCH

		return TRUE;
}

#ifdef _USE_DIBSAVE
// WriteDIB		- Writes a DIB to file
// Returns		- TRUE on success
// szFile		- Name of file to write to
// hDIB			- Handle of the DIB
BOOL CDIBWrapper::Save(LPCTSTR szFile)
{
	BITMAPFILEHEADER	hdr;
	LPBITMAPINFOHEADER	lpbi;

#define DS_BITMAP_FILEMARKER  ((WORD) ('M' << 8) | 'B')    // is always "BM" = 0x4D42

	if (!m_hBitmap || m_iColorTableSize)
		return FALSE;

	CFile file;
	if (!file.Open(szFile, CFile::modeWrite | CFile::modeCreate))
		return FALSE;

	DIBSECTION dib;
	if (!::GetObject(m_hBitmap, sizeof(dib), &dib))
		return FALSE;

	lpbi = &dib.dsBmih;

	//lpbi->biHeight=-lpbi->biHeight;

	DWORD dwBitmapInfoSize = sizeof(BITMAPINFO) + (m_iColorTableSize - 1) * sizeof(RGBQUAD);
	DWORD dwFileHeaderSize = dwBitmapInfoSize + sizeof(hdr);

	// Fill in the fields of the file header 
	hdr.bfType = DS_BITMAP_FILEMARKER;
	hdr.bfSize = dwFileHeaderSize + lpbi->biSizeImage;
	hdr.bfReserved1 = 0;
	hdr.bfReserved2 = 0;
	hdr.bfOffBits = dwFileHeaderSize;

	// Write the file header 
	file.Write(&hdr, sizeof(hdr));

	// Write the DIB header
	file.Write(lpbi, dwBitmapInfoSize);

	// Write DIB bits
	file.Write(GetDIBits(), lpbi->biSizeImage);

	return TRUE;

	/*
	int nColors = 1 << lpbi->biBitCount;

	// Fill in the fields of the file header
	hdr.bfType		= ((WORD) ('M' << 8) | 'B');	// is always "BM"
	hdr.bfSize		= GlobalSize (m_hBitmap) + sizeof( hdr );
	hdr.bfReserved1 	= 0;
	hdr.bfReserved2 	= 0;
	hdr.bfOffBits		= (DWORD) (sizeof( hdr ) + lpbi->biSize +
						nColors * sizeof(RGBQUAD));

	// Write the file header
	file.Write( &hdr, sizeof(hdr) );

	// Write the DIB header and the bits
	file.Write( lpbi, GlobalSize(m_hBitmap) );

	return TRUE;
	*/
}
#endif

/////////////////////////////////////////////////////////////////////////////
// CDIBSectionLite diagnostics

#ifdef _DEBUG
void CDIBWrapper::AssertValid() const
{
	ASSERT(m_hBitmap);

	DIBSECTION ds;
	DWORD dwSize = GetObject(m_hBitmap, sizeof(DIBSECTION), &ds);
	ASSERT(dwSize == sizeof(DIBSECTION));

	ASSERT(ds.dsBmih.biClrUsed == m_iColorTableSize);

	CObject::AssertValid();
}

void CDIBWrapper::Dump(CDumpContext& dc) const
{
	CObject::Dump(dc);
}

#endif //_DEBUG


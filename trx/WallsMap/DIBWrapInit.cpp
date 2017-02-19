// CDIBWrapper::InitBitmap() : implementation file

#include "stdafx.h"
#include "utility.h"
#include "DIBWrapper.h"
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
    TRY {
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

		m_iBitCount=bmih.biBitCount;
		m_iHeight=abs(bmih.biHeight);
		m_iWidth=bmih.biWidth;

        if(!bmih.biSizeImage) {
             bmih.biSizeImage = GetImageSize();
        }

        m_hBitmap = CreateDIBSection(hDC, lpBitmapInfo, DIB_RGB_COLORS, &m_ppvBits, NULL, 0);
        ::ReleaseDC(NULL, hDC);
		hDC=NULL;

        if (!m_hBitmap)
        {
            TRACE0("CreateDIBSection failed\n");
            AfxThrowResourceException();
        }

        // Flush the GDI batch queue 
        GdiFlush();

    }
    CATCH (CException,e)
    {
        e->Delete();
#ifdef _DEBUG
        _TraceLastError();
#endif
        DeleteObject();
        return FALSE;
    }
	END_CATCH

    return TRUE;
}

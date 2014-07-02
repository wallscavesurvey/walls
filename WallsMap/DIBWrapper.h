#if !defined(_DIBWRAPPER_H)
#define _DIBWRAPPER_H

#pragma once

/////////////////////////////////////////////////////////////////////////////
// defines

//#define DIBSECTION_NO_DITHER          // Disallow dithering via DrawDib functions
//#define DIBSECTION_NO_PALETTE         // Remove palette support

/////////////////////////////////////////////////////////////////////////////
// CDIBWrapper object

class CDIBWrapper : public CObject
{
// Construction
public:
	CDIBWrapper();
	virtual ~CDIBWrapper();
    void DeleteObject();
    HBITMAP  m_hBitmap;          // Handle to DIBSECTION

// static helpers
public:
    static int BytesPerLine(int nWidth, int nBitsPerPixel)
		{ return ( (nWidth * nBitsPerPixel + 31) & (~31) ) / 8; }

    static int NumColorEntries(int nBitsPerPixel, int nCompression, DWORD biClrUsed = 0);

// Attributes
public:
    HBITMAP      GetHandle() const			 { return (this)? m_hBitmap : NULL;			}
    operator     HBITMAP() const             { return GetHandle();						}
    CSize        GetSize() const             { return CSize(GetWidth(), GetHeight());	}
    INT          GetHeight() const           { return m_iHeight;						} 
    INT          GetWidth() const            { return m_iWidth;							}
    LPVOID       GetDIBits() const           { return m_ppvBits;						}
    DWORD        GetImageSize() const        { return m_iHeight*BytesPerLine(m_iWidth,m_iBitCount); }
	INT			 GetImageWidth() const		 { return BytesPerLine(m_iWidth,m_iBitCount); }
  

// Operations (Setting the bitmap)
public:
    BOOL InitBitmap(LPBITMAPINFO lpBitmapInfo);
	BOOL Init(int w,int h)
	{
		BITMAPINFOHEADER bmi;
		memset(&bmi,0,sizeof(bmi));
		bmi.biSize = sizeof(BITMAPINFOHEADER);
		bmi.biWidth = w;
		bmi.biHeight = h;
		bmi.biPlanes = 1;
		bmi.biSizeImage = abs(h)*((3*w+3)&~3);
		bmi.biBitCount = 24;
		bmi.biCompression = BI_RGB;
		return InitBitmap((LPBITMAPINFO)&bmi);
	}

	//Optionally linked functions --
    BOOL SetBitmap(UINT nIDResource);
    BOOL SetBitmap(LPCTSTR lpszResourceName);
    BOOL SetBitmap(HBITMAP hBitmap, CPalette* pPalette = NULL);
    BOOL LoadBMP(LPCTSTR lpszFileName);
    BOOL Save(LPCTSTR lpszFileName);
    BOOL Copy(CDIBWrapper& Bitmap);

// Display Operations (DIBWrapper.cpp) --
public:
	BOOL Fill(HBRUSH hbr);
	BOOL Draw(HDC destDC,int x,int y);
	BOOL Draw(HDC destDC, const CPoint &ptDest) {return Draw(destDC,ptDest.x,ptDest.y);}
	BOOL Stretch(HDC destDC,int x,int y,int cx,int cy,int mode=COLORONCOLOR); 
	BOOL Stretch(HDC destDC, const CPoint &ptDest, const CSize &szDest, int mode=COLORONCOLOR) {
		return Stretch(destDC,ptDest.x,ptDest.y,szDest.cx,szDest.cy,mode);
	}
	BOOL Stretch(HDC destDC,const CRect &crDest, int mode=COLORONCOLOR) {
		return Stretch(destDC,crDest.left,crDest.top,crDest.Width(),crDest.Height(),mode);
	}

    CDC* GetMemoryDC(CDC* pDC = NULL);
	ID2D1DCRenderTarget *pRT() { return m_pRT; }
	ID2D1SolidColorBrush *pBR() { return m_pBR; }
	HDC GetMemoryHDC(HDC dc=NULL);
    BOOL ReleaseMemoryDC();

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

// Implementation
protected:
	//Optionally linked --
	UINT GetColorTableEntries(HDC hdc, HBITMAP hBitmap);
public:
	ID2D1DCRenderTarget *InitRT();

	ID2D1SolidColorBrush *InitBR(COLORREF rgb,float opacity);

	void ReleaseRT() { SafeRelease(&m_pRT); }
	void ReleaseBR() { SafeRelease(&m_pBR); }

protected:
	INT		m_iHeight;			 // (positive)
	INT		m_iWidth;
	INT		m_iBitCount;
    VOID    *m_ppvBits;          // Pointer to bitmap bits
    UINT     m_iColorTableSize;  // Size of color table

    CDC      m_MemDC;            // Memory DC for drawing on bitmap with GDI
	ID2D1DCRenderTarget* m_pRT;
	ID2D1SolidColorBrush *m_pBR;

private:
    HBITMAP  m_hOldBitmap;      // Storage for previous bitmap in Memory DC
};


#endif // !defined(_DIBWRAPPER_H)

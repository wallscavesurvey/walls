//for use with sditemp.exe and qtpixels.dll 
#pragma once

#define GetR_F(rgb) ((float)(GetRValue(rgb)/255.0))
#define GetG_F(rgb) ((float)(GetGValue(rgb)/255.0))
#define GetB_F(rgb) ((float)(GetBValue(rgb)/255.0))

#define GetR_f(rgb) (GetRValue(rgb)/255.0)
#define GetG_f(rgb) (GetGValue(rgb)/255.0)
#define GetB_f(rgb) (GetBValue(rgb)/255.0)

#define GetR(rgb) GetRValue(rgb)
#define GetG(rgb) GetGValue(rgb)
#define GetB(rgb) GetBValue(rgb)

#define RGBAtoD2D(rgb,alpha) D2D1::ColorF(GetR_F(rgb),GetG_F(rgb),GetB_F(rgb),alpha)
#define RGBtoD2D(rgb) D2D1::ColorF(GetR_F(rgb),GetG_F(rgb),GetB_F(rgb))

enum { SYM_SQUARE, SYM_CIRCLE, SYM_TRIANGLE };

class CPixelData {
public:
	double fLineSize;
	double fVecSize;
	COLORREF clrSym;
	COLORREF clrLine;
	COLORREF clrVec;
	COLORREF clrBack;
	int iSymSize;
	int iSymTyp;
	int iOpacitySym;
	int iOpacityVec;
	BOOL bAntialias;
	BOOL bDotCenter;
	BOOL bBackSym;
};

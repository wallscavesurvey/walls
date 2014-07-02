#ifndef _PLACEMARK_H
#define _PLACEMARK_H

enum e_flagsymbols {
	FST_SQUARES, FST_CIRCLES, FST_TRIANGLES,
	FST_PLUSES, FST_TRUETYPE, M_NUMSHAPES
};
enum e_flagshades { FST_SOLID, FST_CLEAR, FST_FILLED, M_NUMSHADES };
enum e_flagfills { FSF_HOLLOW=0, FSF_FILLED=1, FSF_ANTIALIAS=2, FSF_USED2D=4, FSF_DOTCENTER=8 };

struct SHP_MRK_STYLE {
	SHP_MRK_STYLE() :
	crMrk(RGB(0, 0, 0)),
	crBkg(RGB(255, 0, 0)),
	wFilled(FSF_FILLED),
	wShape(FST_SQUARES),
	wSize(9),
	fLineWidth(1),
	iOpacitySym(100),
	iOpacityVec(100)
	{}

	bool MarkerVisible() { return wSize>0 && iOpacitySym>0 && ((wFilled & FSF_FILLED) || fLineWidth>0.0); }

	SHP_MRK_STYLE(COLORREF crMk, COLORREF crBk, WORD wFill, WORD wShp, WORD wSiz, float fLineW, int fOpacity = 100) :
		crMrk(crMk), crBkg(crBk), wFilled(wFill), wShape(wShp), wSize(wSiz), fLineWidth(fLineW),
		iOpacitySym(fOpacity), iOpacityVec(100) {}

	LPCSTR GetProfileStr(CString &s) const
	{
		s.Format("%u %u %u %u %u %.1f %u %u", crMrk, crBkg, wFilled, wShape, wSize, fLineWidth,
			iOpacitySym, iOpacityVec);
		return (LPCSTR)s;
	}
	void ReadProfileStr(CString &s);
	void Scale(double fScale) {
		if (fScale != 1.0) {
			fLineWidth = (float)(fLineWidth*fScale);
			wSize = (WORD)(fScale*wSize);
		}
	}

	COLORREF crMrk;
	COLORREF crBkg;
	float fLineWidth;
	int iOpacitySym;
	int iOpacityVec;
	WORD wFilled;
	WORD wShape;
	WORD wSize;
};

class CPlaceMark
{
public:
	CPlaceMark() : m_pbm(NULL) {}
	~CPlaceMark()
	{
		Destroy();
	}

	void Destroy()
	{
		SafeRelease(&m_pbm);
	}
	CPlaceMark *Create(ID2D1RenderTarget *prt, SHP_MRK_STYLE &st);

	void Plot(ID2D1RenderTarget *rt, const CPoint &pt)
	{
		ASSERT(m_pbm);
		rt->SetTransform(D2D1::Matrix3x2F::Translation(pt.x - m_sz*0.5f + 0.5f, pt.y - m_sz*m_ctrRatioY + 0.5f));
		rt->DrawBitmap(m_pbm, D2D1::RectF(0.f, 0.f, m_sz, m_sz), m_opacity);
	}

	static void PlotStyleSymbol(ID2D1RenderTarget *prt, SHP_MRK_STYLE &st, CPoint &pt)
	{
		if (st.wSize && st.iOpacitySym > 0) {
			CPlaceMark pm;
			if (pm.Create(prt, st)) {
				pm.Plot(prt, pt);
			}
		}
	}

private:
	float m_sz;
	float m_ctrRatioY;
	float m_opacity;
	ID2D1Bitmap *m_pbm;
};

HBRUSH CreateStyleBrush(const SHP_MRK_STYLE &style);
HPEN CreateStylePen(const SHP_MRK_STYLE &style, BOOL bNonWhite = FALSE);
#endif

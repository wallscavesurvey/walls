#include "stdafx.h"
#include "PlaceMark.h"

CPlaceMark * CPlaceMark::Create(ID2D1RenderTarget *prt, SHP_MRK_STYLE &st)
{
	if (m_sz) Destroy();
	float s = (float)st.wSize;

	D2D1_SIZE_F szF = D2D1::SizeF(s, s);
	D2D1_COLOR_F sClr(RGBtoD2D(st.crBkg)); //alpha=255
	D2D1_COLOR_F lClr(RGBtoD2D(st.crMrk));

	ID2D1BitmapRenderTarget *pRT;
	ID2D1SolidColorBrush *pBR;

	if (0 > prt->CreateCompatibleRenderTarget(&szF, NULL, NULL, D2D1_COMPATIBLE_RENDER_TARGET_OPTIONS_NONE, &pRT)) {
		return NULL;
	}
	if (0 > pRT->CreateSolidColorBrush(sClr, &pBR)) {
		pRT->Release();
		return NULL;
	}

	m_opacity = st.iOpacitySym / 100.0f;
	m_ctrRatioY = (st.wShape == FST_TRIANGLES) ? 0.7113248654f : 0.5f;
	m_sz = (float)st.wSize;

	pRT->SetTransform(D2D1::Matrix3x2F::Scale(s, s));
	float w = st.fLineWidth / s;  //in effect, don't scale line width

	pRT->BeginDraw();

	pRT->Clear(D2D1::ColorF(0.f, 0.f, 0.f, 0.f)); //transparent since alpha==0

	switch (st.wShape) {
	case FST_SQUARES:
		pRT->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
		if (st.wFilled & FSF_FILLED) pRT->FillRectangle(D2D1::RectF(0.f, 0.f, 1.f, 1.f), pBR);
		pBR->SetColor(lClr);
		pRT->DrawRectangle(D2D1::RectF(w / 2, w / 2, 1 - w / 2, 1 - w / 2), pBR, w);
		break;

	case FST_PLUSES:
		pRT->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
		pBR->SetColor(lClr);
		pRT->DrawLine(D2D1::Point2F(0, 0.5f), D2D1::Point2F(1.f, 0.5f), pBR, w); //default square line caps
		pRT->DrawLine(D2D1::Point2F(0.5f, 0), D2D1::Point2F(0.5f, 1.f), pBR, w);
		break;

	case FST_CIRCLES:
		pRT->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
		{
			D2D1_ELLIPSE circle = D2D1::Ellipse(D2D1::Point2F(0.5f, 0.5f), 0.5f - w / 2, 0.5f - w / 2);
			if (st.wFilled & FSF_FILLED) pRT->FillEllipse(circle, pBR);
			pBR->SetColor(lClr);
			pRT->DrawEllipse(circle, pBR, w);
		}
		break;

	case FST_TRIANGLES:
		pRT->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
		{
			ID2D1PathGeometry *pPath = NULL;
			if (0 > d2d_pFactory->CreatePathGeometry(&pPath)) {
				ASSERT(0);
				break;
			}
			ID2D1GeometrySink *pSink = NULL;
			if (0 > pPath->Open(&pSink)) {
				pPath->Release();
				break;
			}
			//pSink->SetFillMode(D2D1_FILL_MODE_ALTERNATE); //default, not relevant anyway

			pSink->BeginFigure(D2D1::Point2F(w*0.8660254038f, 1 - w / 2),
				(st.wFilled & FSF_FILLED) ? D2D1_FIGURE_BEGIN_FILLED : D2D1_FIGURE_BEGIN_HOLLOW);
			pSink->AddLine(D2D1::Point2F(0.5, 0.1330745962f + w));
			pSink->AddLine(D2D1::Point2F(1 - w * 0.8660254038f, 1 - w / 2));
			pSink->EndFigure(D2D1_FIGURE_END_CLOSED);

			pSink->Close();
			pSink->Release();

			if (st.wFilled & FSF_FILLED) pRT->FillGeometry(pPath, pBR);
			pBR->SetColor(lClr);
			pRT->DrawGeometry(pPath, pBR, w);
			pPath->Release();
		}
	}

	if ((st.wFilled&FSF_DOTCENTER) && st.wShape != FST_PLUSES) {
		CD2DPointF ctr(0.5f, m_ctrRatioY);
		pRT->DrawLine(ctr, ctr, pBR, 2 / s, d2d_pStrokeStyleRnd); // pBR color should be lClr
	}

	pBR->Release();
	if (0 > pRT->EndDraw() || 0 > pRT->GetBitmap(&m_pbm)) {
		ASSERT(0);
		pRT->Release();
		return NULL;
	}
	pRT->Release();
	return this;
}

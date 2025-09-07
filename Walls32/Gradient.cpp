// Gradient.cpp: implementation of the CGradient class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Gradient.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

//IMPLEMENT_SERIAL(CGradientObArray,CObArray, 0)

//Change last parameter (schema number) for each new NTAC version --
IMPLEMENT_SERIAL(CGradient, CObject, 1)
IMPLEMENT_SERIAL(CPeg, CObject, 1)

UINT CPeg::uniqueID = 0;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CGradient::CGradient()
{
	m_StartPeg.color = 0x00000000;
	m_StartPeg.position = 0.0;
	m_EndPeg.color = 0x00FFFFFF;
	m_EndPeg.position = 1.0;
	m_Background.color = 0x00000000; //A default pal
	m_Background.position = 0.0;
	m_InterpolationMethod = Cosine;
	m_Flags = 0;
	m_Quantization = -1;
	m_fStartValue = m_fEndValue = 0.0;
}

CGradient::CGradient(CGradient &src)
{
	m_StartPeg.color = src.m_StartPeg.color;
	m_EndPeg.color = src.m_EndPeg.color;
	m_Background.color = src.m_Background.color;
	m_InterpolationMethod = src.m_InterpolationMethod;
	m_Flags = src.m_Flags;
	m_Quantization = src.m_Quantization;
	m_fStartValue = src.m_fStartValue;
	m_fEndValue = src.m_fEndValue;

	pegs.RemoveAll();
	for (int i = 0; i < src.pegs.GetSize(); i++)
		pegs.Add(src.pegs[i]);
}

CGradient::~CGradient()
{
	pegs.RemoveAll();
}

CGradient& CGradient::operator =(CGradient &src)
{
	pegs.RemoveAll();
	for (int i = 0; i < src.pegs.GetSize(); i++)
		pegs.Add(src.pegs[i]);

	m_StartPeg.color = src.m_StartPeg.color;
	m_EndPeg.color = src.m_EndPeg.color;
	m_Background.color = src.m_Background.color;
	m_InterpolationMethod = src.m_InterpolationMethod;
	m_Flags = src.m_Flags;
	m_Quantization = src.m_Quantization;
	m_fStartValue = src.m_fStartValue;
	m_fEndValue = src.m_fEndValue;

	return *this;
}

int CGradient::AddPeg(COLORREF crColor, double fPosition)
{
	CPeg peg;

	if (fPosition < 0) fPosition = 0;
	else if (fPosition > 1) fPosition = 1;

	peg.color = crColor;
	peg.position = fPosition;
	pegs.Add(peg);
	SortPegs();

	return IndexFromId(peg.GetID());
}

int CGradient::AddPeg(CPeg peg)
{
	return AddPeg(peg.color, peg.position);
}

//----- Assorted short functions -----//
void CGradient::RemovePeg(int iIndex) { pegs.RemoveAt(iIndex); }
void CGradient::SortPegs() { QuickSort(0, pegs.GetUpperBound()); }
int CGradient::GetPegCount() const { return pegs.GetSize(); }

const CPeg CGradient::GetPeg(int iIndex) const
{
	if (iIndex < PEG_START || iIndex == PEG_NONE || iIndex >= GetPegCount()) {
		ASSERT(FALSE);
		iIndex = PEG_START;
	}

	if (iIndex >= 0)
	{
		const CPeg peg(pegs[iIndex]);
		return peg;
	}
	else if (iIndex == PEG_START)
		return m_StartPeg;
	else if (iIndex == PEG_END)
		return m_EndPeg;
	else return m_Background;
}

int CGradient::SetPeg(int iIndex, COLORREF crColor, double fPosition)
{
	UINT tempid;

	ASSERT(iIndex > -4 && iIndex != -1 && iIndex < GetPegCount());
	//You must pass a valid peg index or PEG_START, PEG_END, or PEG_BACKGROUND!

	if (fPosition < 0) fPosition = 0;
	else if (fPosition > 1) fPosition = 1;

	if (iIndex == PEG_START)
		m_StartPeg.color = crColor;
	else if (iIndex == PEG_END)
		m_EndPeg.color = crColor;
	else if (iIndex == PEG_BACKGROUND)
		m_Background.color = crColor;
	else
	{
		pegs[iIndex].color = crColor;
		pegs[iIndex].position = fPosition;
		tempid = pegs[iIndex].GetID();
		SortPegs();
		return IndexFromId(tempid);
	}
	return -1;
}

int CGradient::SetPeg(int iIndex, CPeg peg)
{
	UINT tempid;

	ASSERT(iIndex > -4 && iIndex != -1 && iIndex < GetPegCount());
	//You must pass a valid peg index or PEG_START, PEG_END, or PEG_BACKGROUND!

	if (peg.position < 0.0) peg.position = 0.0;
	else if (peg.position > 1.0) peg.position = 1.0;

	if (iIndex == PEG_START)
		m_StartPeg.color = peg.color;
	else if (iIndex == PEG_END)
		m_EndPeg.color = peg.color;
	else if (iIndex == PEG_BACKGROUND)
		m_Background.color = peg.color;
	else
	{
		pegs[iIndex].color = peg.color;
		pegs[iIndex].position = peg.position;
		tempid = pegs[iIndex].GetID();
		SortPegs();
		return IndexFromId(tempid);
	}
	return -1;
}
void CGradient::ShiftPegToPosition(int iIndex, double pos)
{
	ASSERT(iIndex >= 0 && iIndex < GetPegCount());
	pegs[iIndex].position = pos;
}


void CGradient::Make8BitPalette(RGBTRIPLE *lpPal)
{
	MakeEntries(lpPal, 256);
}

void CGradient::MakePalette(CPalette *lpPal)
{
	RGBTRIPLE *entries = new RGBTRIPLE[256];
	LOGPALETTE *logpal;
	logpal = (LOGPALETTE*)malloc(2 * sizeof(WORD) + 256 * sizeof(PALETTEENTRY));

	lpPal->DeleteObject();

	Make8BitPalette(entries);

	logpal->palVersion = 0x300;
	logpal->palNumEntries = 256;

	for (int i = 0; i < 256; i++)
	{
		logpal->palPalEntry[i].peRed = entries[i].rgbtRed;
		logpal->palPalEntry[i].peGreen = entries[i].rgbtGreen;
		logpal->palPalEntry[i].peBlue = entries[i].rgbtBlue;
		logpal->palPalEntry[i].peFlags = PC_RESERVED;
	}

	delete[] entries;

	lpPal->CreatePalette(logpal);
	free(logpal);
}

void CGradient::MakeEntries(RGBTRIPLE *lpPal, int iEntryCount)
{
	double pos;
	COLORREF color;

	ASSERT(iEntryCount > 1);
	ASSERT(iEntryCount < 65535);

	InterpolateFn Interpolate = GetInterpolationProc();
	ASSERT(Interpolate != NULL);

	if (pegs.GetSize() > 0)
	{
		//Some things are already constant and so can be found early
		double firstpegpos = pegs[0].position;
		double lastpegpos = pegs[pegs.GetUpperBound()].position;
		COLORREF lastpegcolour = pegs[pegs.GetUpperBound()].color;
		int curpeg;

		for (int i = 0; i < iEntryCount; i++)
		{
			if (m_Quantization == -1)
				pos = (double)i / iEntryCount;
			else
				pos = ((double)(int)(((double)i / iEntryCount)*m_Quantization)) / m_Quantization + 0.5 / m_Quantization;

			if (pos <= firstpegpos)
			{
				color = Interpolate(m_StartPeg.color, pegs[0].color, pos, 0, firstpegpos);
				lpPal[i].rgbtRed = GetRValue(color);
				lpPal[i].rgbtGreen = GetGValue(color);
				lpPal[i].rgbtBlue = GetBValue(color);
			}
			else if (pos > lastpegpos)
			{
				color = Interpolate(lastpegcolour, m_EndPeg.color, pos, lastpegpos, 1);
				lpPal[i].rgbtRed = GetRValue(color);
				lpPal[i].rgbtGreen = GetGValue(color);
				lpPal[i].rgbtBlue = GetBValue(color);
			}
			else
			{
				curpeg = IndexFromPos(pos);
				color = Interpolate(pegs[curpeg].color, pegs[curpeg + 1].color, pos, pegs[curpeg].position, pegs[curpeg + 1].position);
				lpPal[i].rgbtRed = GetRValue(color);
				lpPal[i].rgbtGreen = GetGValue(color);
				lpPal[i].rgbtBlue = GetBValue(color);
			}
		}
	}
	else
	{
		//When there are no extra peg we can just interpolate the start and end
		for (int i = 0; i < iEntryCount; i++)
		{
			if (m_Quantization == -1)
				pos = (double)i / iEntryCount;
			else
				pos = ((double)(int)(((double)i / iEntryCount)*m_Quantization)) / m_Quantization + 0.5 / m_Quantization;

			color = Interpolate(m_StartPeg.color, m_EndPeg.color, pos, 0, 1);
			lpPal[i].rgbtRed = GetRValue(color);
			lpPal[i].rgbtGreen = GetGValue(color);
			lpPal[i].rgbtBlue = GetBValue(color);
		}
	}

	if (m_Flags&GF_USEBACKGROUND)
	{
		lpPal[0].rgbtRed = GetRValue(m_Background.color);
		lpPal[0].rgbtGreen = GetGValue(m_Background.color);
		lpPal[0].rgbtBlue = GetBValue(m_Background.color);
	}
}

COLORREF CGradient::InterpolateLinear(COLORREF first, COLORREF second, double position, double start, double end)
{
	if (start == end) return first;
	if (end - start == 0) return second;
	if (position == start) return first;
	if (position == end) return second;
	return RGB((BYTE)((GetRValue(second)*(position - start) + GetRValue(first)*(end - position)) / (end - start)),
		(BYTE)((GetGValue(second)*(position - start) + GetGValue(first)*(end - position)) / (end - start)),
		(BYTE)((GetBValue(second)*(position - start) + GetBValue(first)*(end - position)) / (end - start)));
}

COLORREF CGradient::InterpolateReverse(COLORREF first, COLORREF second, double position, double start, double end)
{
	if (start == end) return first;
	if (end - start == 0) return second;
	if (position == start) return second;
	if (position == end) return first;
	return RGB((BYTE)((GetRValue(first)*(position - start) + GetRValue(second)*(end - position)) / (end - start)),
		(BYTE)((GetGValue(first)*(position - start) + GetGValue(second)*(end - position)) / (end - start)),
		(BYTE)((GetBValue(first)*(position - start) + GetBValue(second)*(end - position)) / (end - start)));
}

COLORREF CGradient::InterpolateFlatStart(COLORREF first, COLORREF, double, double, double)
{
	return first;
}

COLORREF CGradient::InterpolateFlatMid(COLORREF first, COLORREF second, double, double, double)
{
	unsigned short sr, sg, sb, er, eg, eb;
	sr = GetRValue(first);
	sg = GetGValue(first);
	sb = GetBValue(first);
	er = GetRValue(second);
	eg = GetGValue(second);
	eb = GetBValue(second);

	return RGB((sr + er) / 2, (sg + eg) / 2, (sb + eb) / 2);
}

COLORREF CGradient::InterpolateFlatEnd(COLORREF, COLORREF second, double, double, double)
{
	return second;
}

COLORREF CGradient::InterpolateCosine(COLORREF first, COLORREF second, double position, double start, double end)
{
	double theta = (position - start) / (end - start) * 3.141592653589793;
	double f = (1.0 - cos(theta)) * 0.5;

	return RGB((BYTE)(((double)GetRValue(first))*(1 - f) + ((double)GetRValue(second))*f),
		(BYTE)(((double)GetGValue(first))*(1 - f) + ((double)GetGValue(second))*f),
		(BYTE)(((double)GetBValue(first))*(1 - f) + ((double)GetBValue(second))*f));
}

void RGB_to_HSL(double r, double g, double b, double *h, double *s, double *l)
{
	double v;
	double m;
	double vm;
	double r2, g2, b2;

	v = max(r, g);
	v = max(v, b);
	m = min(r, g);
	m = min(m, b);

	if ((*l = (m + v) / 2.0) <= 0.0) return;
	if ((*s = vm = v - m) > 0.0) {
		*s /= (*l <= 0.5) ? (v + m) :
			(2.0 - v - m);
	}
	else
		return;


	r2 = (v - r) / vm;
	g2 = (v - g) / vm;
	b2 = (v - b) / vm;

	if (r == v)
		*h = (g == m ? 5.0 + b2 : 1.0 - g2);
	else if (g == v)
		*h = (b == m ? 1.0 + r2 : 3.0 - b2);
	else
		*h = (r == m ? 3.0 + g2 : 5.0 - r2);

	*h /= 6.0;
}

void HSL_to_RGB(double h, double sl, double l, double *r, double *g, double *b)
{
	double v;

	v = (l <= 0.5) ? (l * (1.0 + sl)) : (l + sl - l * sl);
	if (v <= 0) {
		*r = *g = *b = 0.0;
	}
	else {
		double m;
		double sv;
		int sextant;
		double fract, vsf, mid1, mid2;

		m = l + l - v;
		sv = (v - m) / v;
		h *= 6.0;
		sextant = (int)h;
		fract = h - sextant;
		vsf = v * sv * fract;
		mid1 = m + vsf;
		mid2 = v - vsf;
		switch (sextant) {
		case 0: *r = v; *g = mid1; *b = m; break;
		case 1: *r = mid2; *g = v; *b = m; break;
		case 2: *r = m; *g = v; *b = mid1; break;
		case 3: *r = m; *g = mid2; *b = v; break;
		case 4: *r = mid1; *g = m; *b = v; break;
		case 5: *r = v; *g = m; *b = mid2; break;
		}
	}
}

static COLORREF InterpolateHSLCW(COLORREF first, COLORREF second, double position, double start, double end, BOOL bCW)
{
	double sh = 0, ss = 0, sl = 0, eh = 0, es = 0, el = 0, h = 0, s = 0, l = 0, r = 0, g = 0, b = 0;
	RGB_to_HSL((double)GetRValue(first) / 255.0, (double)GetGValue(first) / 255.0,
		(double)GetBValue(first) / 255.0, &sh, &ss, &sl);
	RGB_to_HSL((double)GetRValue(second) / 255.0, (double)GetGValue(second) / 255.0,
		(double)GetBValue(second) / 255.0, &eh, &es, &el);

	sh = sh - floor(sh);
	eh = eh - floor(eh);

	if (bCW ? (eh >= sh) : (eh <= sh)) h = (eh*(position - start) + sh * (end - position)) / (end - start);
	else h = ((eh + (bCW ? 1.0 : -1.0))*(position - start) + sh * (end - position)) / (end - start);

	if (fabs(h) <= 10.0*DBL_EPSILON) h = 0.0;
	else h = h - floor(h);

	s = ((es*(position - start) + ss * (end - position)) / (end - start));
	l = ((el*(position - start) + sl * (end - position)) / (end - start));

	HSL_to_RGB(h, s, l, &r, &g, &b);
	return RGB((BYTE)(r*255.0), (BYTE)(g*255.0), (BYTE)(b*255.0));
}


COLORREF CGradient::InterpolateHSLClockwise(COLORREF first, COLORREF second, double position, double start, double end)
{
	return InterpolateHSLCW(first, second, position, start, end, TRUE);
	/*
		double sh = 0, ss = 0, sl = 0, eh = 0, es = 0, el = 0, h = 0, s = 0, l = 0, r = 0, g = 0, b = 0;
		RGB_to_HSL((double)GetRValue(first)/255.0, (double)GetGValue(first)/255.0,
			(double)GetBValue(first)/255.0, &sh, &ss, &sl);
		RGB_to_HSL((double)GetRValue(second)/255.0, (double)GetGValue(second)/255.0,
			(double)GetBValue(second)/255.0, &eh, &es, &el);

		sh = sh - floor(sh);
		eh = eh - floor(eh);

		//Interpolate H clockwise
		if(eh >= sh) h = (eh*(position - start) + sh*(end-position))/(end-start);
		else h = ((eh + 1.0)*(position - start) + sh*(end-position))/(end-start);

		if(fabs(h)<=10.0*DBL_EPSILON) h=0.0;
		else h = h - floor(h);

		s = ((es*(position - start) + ss*(end-position))/(end-start));
		l = ((el*(position - start) + sl*(end-position))/(end-start));

		HSL_to_RGB(h, s, l, &r, &g, &b);
		return RGB((BYTE)(r*255.0), (BYTE)(g*255.0), (BYTE)(b*255.0));
	*/
}

COLORREF CGradient::InterpolateHSLAntiClockwise(COLORREF first, COLORREF second, double position, double start, double end)
{
	//NOTE: This routine was buggy!!! (DMcK)
	return InterpolateHSLCW(first, second, position, start, end, FALSE);
	/*
		double sh = 0, ss = 0, sl = 0, eh = 0, es = 0, el = 0, h = 0, s = 0, l = 0, r = 0, g = 0, b = 0;
		RGB_to_HSL((double)GetRValue(first)/255.0, (double)GetGValue(first)/255.0,
			(double)GetBValue(first)/255.0, &sh, &ss, &sl);
		RGB_to_HSL((double)GetRValue(second)/255.0, (double)GetGValue(second)/255.0,
			(double)GetBValue(second)/255.0, &eh, &es, &el);

		sh = sh - floor(sh);
		eh = eh - floor(eh);

		//Interpolate H anticlockwise
		if(eh <= sh) {
			h = (eh*(position-start) + sh*(end-position))/(end-start);
		}
		else {
			h = ((eh - 1.0)*(position - start) + sh*(end-position))/(end-start);
		}

		if(fabs(h)<=10.0*DBL_EPSILON) h=0.0;
		else h = h - floor(h);

		s = ((es*(position - start) + ss*(end-position))/(end-start));
		l = ((el*(position - start) + sl*(end-position))/(end-start));

		HSL_to_RGB(h, s, l, &r, &g, &b);
		return RGB((BYTE)(r*255.0), (BYTE)(g*255.0), (BYTE)(b*255.0));
	*/
}

static COLORREF InterpolateHSL(COLORREF first, COLORREF second, double position, double start, double end, BOOL bShort)
{
	double sh = 0, ss = 0, sl = 0, eh = 0, es = 0, el = 0, h = 0, s = 0, l = 0, r = 0, g = 0, b = 0;
	RGB_to_HSL((double)GetRValue(first) / 255.0, (double)GetGValue(first) / 255.0,
		(double)GetBValue(first) / 255.0, &sh, &ss, &sl);
	RGB_to_HSL((double)GetRValue(second) / 255.0, (double)GetGValue(second) / 255.0,
		(double)GetBValue(second) / 255.0, &eh, &es, &el);

	sh = sh - floor(sh);
	eh = eh - floor(eh);
	h = (eh - sh) - floor(eh - sh);

	bShort = bShort ? (h >= 0.5) : (h < 0.5);

	//Interpolate H short route
	if (bShort ? (eh < sh) : (eh >= sh)) {
		h = (eh*(position - start) + sh * (end - position)) / (end - start);
	}
	else {
		h = eh + ((sh > eh) ? 1.0 : -1.0);
		h = (h*(position - start) + sh * (end - position)) / (end - start);
	}

	if (fabs(h) <= 10.0*DBL_EPSILON) h = 0.0;
	else h = h - floor(h);

	s = ((es*(position - start) + ss * (end - position)) / (end - start));
	l = ((el*(position - start) + sl * (end - position)) / (end - start));

	HSL_to_RGB(h, s, l, &r, &g, &b);

	return RGB((BYTE)(r*255.0), (BYTE)(g*255.0), (BYTE)(b*255.0));
}


COLORREF CGradient::InterpolateHSLLongest(COLORREF first, COLORREF second, double position, double start, double end)
{
	return InterpolateHSL(first, second, position, start, end, FALSE);

	/*
		double sh = 0, ss = 0, sl = 0, eh = 0, es = 0, el = 0, h = 0, s = 0, l = 0, r = 0, g = 0, b = 0;
		RGB_to_HSL((double)GetRValue(first)/255.0, (double)GetGValue(first)/255.0,
			(double)GetBValue(first)/255.0, &sh, &ss, &sl);
		RGB_to_HSL((double)GetRValue(second)/255.0, (double)GetGValue(second)/255.0,
			(double)GetBValue(second)/255.0, &eh, &es, &el);

		sh = sh - floor(sh);
		eh = eh - floor(eh);
		h=(eh-sh)-floor(eh-sh);

		//Interpolate H short route
		if( (h<0.5) ? (eh<sh) : (eh>=sh) ) {
			h = (eh*(position - start) + sh*(end-position))/(end-start);
		}
		else {
			h = eh + ((sh>eh)?1.0:-1.0);
			h = (h*(position - start) + sh*(end-position))/(end-start);
		}

		if(fabs(h)<=10.0*DBL_EPSILON) h=0.0;
		else h = h - floor(h);

		s = ((es*(position - start) + ss*(end-position))/(end-start));
		l = ((el*(position - start) + sl*(end-position))/(end-start));

		HSL_to_RGB(h, s, l, &r, &g, &b);

		return RGB((BYTE)(r*255.0), (BYTE)(g*255.0), (BYTE)(b*255.0));
	*/
}

COLORREF CGradient::InterpolateHSLShortest(COLORREF first, COLORREF second, double position, double start, double end)
{
	return InterpolateHSL(first, second, position, start, end, TRUE);

	/*
		double sh = 0, ss = 0, sl = 0, eh = 0, es = 0, el = 0, h = 0, s = 0, l = 0, r = 0, g = 0, b = 0;
		RGB_to_HSL((double)GetRValue(first)/255.0, (double)GetGValue(first)/255.0,
			(double)GetBValue(first)/255.0, &sh, &ss, &sl);
		RGB_to_HSL((double)GetRValue(second)/255.0, (double)GetGValue(second)/255.0,
			(double)GetBValue(second)/255.0, &eh, &es, &el);

		sh = sh - (double)floor(sh);
		eh = eh - (double)floor(eh);
		h=(eh-sh)-floor(eh-sh);

		//Interpolate H short route
		if((h >= 0.5)?(eh < sh):(eh >= sh)) {
			h = (eh*(position - start) + sh*(end-position))/(end-start);
		}
		else {
			h = eh + (sh>eh?1.0:-1.0);
			h = (h*(position - start) + sh*(end-position))/(end-start);
		}

		if(fabs(h)<=10.0*DBL_EPSILON) h=0.0;
		else h = h - floor(h);

		s = ((es*(position - start) + ss*(end-position))/(end-start));
		l = ((el*(position - start) + sl*(end-position))/(end-start));

		HSL_to_RGB(h, s, l, &r, &g, &b);
		return RGB((BYTE)(r*255.0), (BYTE)(g*255.0), (BYTE)(b*255.0));
	*/
}

int CGradient::IndexFromPos(double pos)
{
	ASSERT(pos >= 0.0 && pos <= 1.0);
	// positions betwen 0 and 1!

	if (pos < pegs[0].position)
		return PEG_START;

	for (int i = 0; i < pegs.GetSize() - 1; i++)
		if (pos >= pegs[i].position && pos <= pegs[i + 1].position)
			return i;

	return -1; // Eh? somethings wrong here
}

int CGradient::IndexFromId(UINT id)
{
	for (int i = 0; i < pegs.GetSize(); i++)
		if (id == pegs[i].GetID())
			return i;
	return -1;
}


//----- Fast sorting alogarithm functions -----//
int CGradient::Partition(int lb, int ub)
{
	CPeg temppeg, pivot;
	int i, j, p;

	/* select pivot and exchange with 1st element */
	p = lb + ((ub - lb) >> 1);
	pivot = pegs[p];
	pegs[p] = pegs[lb];

	/* sort lb+1..ub based on pivot */
	i = lb + 1;
	j = ub;
	while (1)
	{
		while (i < j && pivot.position >= pegs[i].position) i++;
		while (j >= i && pegs[j].position >= pivot.position) j--;
		if (i >= j) break;
		temppeg.color = pegs[i].color;
		temppeg.position = pegs[i].position;
		pegs[i].color = pegs[j].color;
		pegs[i].position = pegs[j].position;
		pegs[j].color = temppeg.color;
		pegs[j].position = temppeg.position;
		j--; i++;
	}

	/* pivot belongs in a[j] */
	pegs[lb] = pegs[j];
	pegs[j] = pivot;

	return j;
}


void CGradient::QuickSort(int lb, int ub)
{
	int m;

	/**************************
	 *  sort array pegs[lb..ub]  *
	 **************************/

	while (lb < ub)
	{
		/* quickly sort short lists */
		if (ub - lb <= 12)
		{
			InsertSort(lb, ub);
			return;
		}

		m = Partition(lb, ub);

		if (m - lb <= ub - m)
		{
			QuickSort(lb, m - 1);
			lb = m + 1;
		}
		else
		{
			QuickSort(m + 1, ub);
			ub = m - 1;
		}
	}
}

void CGradient::InsertSort(int lb, int ub)
{
	CPeg temppeg;

	for (int i = lb + 1; i <= ub; i++)
	{
		temppeg = pegs[i];
		int j = i - 1;
		for (; j >= lb && pegs[j].position > temppeg.position; j--)
			pegs[j + 1] = pegs[j];
		pegs[j + 1] = temppeg;
	}
}

COLORREF CGradient::ColorFromPosition(double pos)
{
	if (pos < 0 || pos > 1)
	{
		ASSERT(0); // Position out of bounds!!
		return 0;
	}

	if (pos == 0) {
		return (m_Flags&GF_USEBACKGROUND) ? m_Background.color : GetStartPegColor();
	}
	if (pos == 1) return GetEndPegColor();

	InterpolateFn Interpolate = GetInterpolationProc();
	ASSERT(Interpolate != NULL);

	if (m_Quantization != -1)
		pos = ((double)(int)(pos*m_Quantization)) / m_Quantization + 0.5 / m_Quantization;

	if (pegs.GetSize() > 0)
	{
		//Some thing are aready constant and so can be found early
		double firstpegpos = pegs[0].position;
		double lastpegpos = pegs[pegs.GetUpperBound()].position;

		if (pos <= firstpegpos)
			return Interpolate(m_StartPeg.color, pegs[0].color, pos, 0,
				firstpegpos);
		else if (pos > lastpegpos)
			return Interpolate(pegs[pegs.GetUpperBound()].color, m_EndPeg.color,
				pos, lastpegpos, 1);
		else
		{
			int curpeg;
			curpeg = IndexFromPos(pos);
			return Interpolate(pegs[curpeg].color, pegs[curpeg + 1].color, pos,
				pegs[curpeg].position, pegs[curpeg + 1].position);
		}
	}
	else
	{
		//When there are no extra pegs we can just interpolate the start and end
		return Interpolate(m_StartPeg.color, m_EndPeg.color, pos, 0, 1);
	}
}

void CGradient::Serialize(CArchive &ar)
{
	if (ar.IsLoading())
	{
		int temp;

		ar >> m_Background.color;
		ar >> m_StartPeg.color;
		ar >> m_EndPeg.color;
		ar >> m_Flags;
		ar >> m_Quantization;
		ar >> temp;
		m_InterpolationMethod = (CGradient::InterpolationMethod)temp;
		ar >> m_fStartValue;
		ar >> m_fEndValue;

		ar >> temp;
		pegs.SetSize(temp);
		for (int i = 0; i < temp; i++)
			pegs[i].Serialize(ar);
	}
	else
	{
		ar << m_Background.color;
		ar << m_StartPeg.color;
		ar << m_EndPeg.color;
		ar << m_Flags;
		ar << m_Quantization;
		ar << (unsigned int)m_InterpolationMethod;
		ar << m_fStartValue;
		ar << m_fEndValue;
		ar << pegs.GetSize();

		for (int i = 0; i < pegs.GetSize(); i++)
			pegs[i].Serialize(ar);
	}
}

CGradient::InterpolationMethod CGradient::GetInterpolationMethod() const
{
	return m_InterpolationMethod;
}

void CGradient::SetInterpolationMethod(const InterpolationMethod method)
{
	m_InterpolationMethod = method;
}

int CGradient::GetQuantization() const
{
	return m_Quantization;
}

void CGradient::SetQuantization(const int entries)
{
	m_Quantization = entries;
}

void CPeg::Serialize(CArchive &ar)
{
	if (ar.IsLoading())
	{
		ar >> color;
		ar >> position;
		//ar >> value;
		id = uniqueID;
		uniqueID++;
	}
	else
	{
		ar << color;
		ar << position;
		//ar << value;
	}
}

InterpolateFn CGradient::GetInterpolationProc()
{
	switch (m_InterpolationMethod)
	{
	case Linear: return InterpolateLinear;
	case FlatStart: return InterpolateFlatStart;
	case FlatMid: return InterpolateFlatMid;
	case FlatEnd: return InterpolateFlatEnd;
	case Cosine: return InterpolateCosine;
	case HSLRedBlue: return InterpolateHSLClockwise;
	case HSLBlueRed: return InterpolateHSLAntiClockwise;
	case HSLShortest: return InterpolateHSLShortest;
	case HSLLongest: return InterpolateHSLLongest;
	case Reverse: return InterpolateReverse;
	default: return 0;
	}
}

void CGradient::FillRect(CDC *dc, CRect *pRect, bool vertical)
{
	CBitmap membmp;
	CDC memdc;
	COLORREF color;
	int len = (vertical ? pRect->Height() : pRect->Width());

	RGBTRIPLE *entry, *pal = new RGBTRIPLE[len];
	ASSERT(pal);
	if (!pal) return;

	if (vertical) membmp.CreateCompatibleBitmap(dc, 1, len);
	else membmp.CreateCompatibleBitmap(dc, len, 1);

	ASSERT(membmp.GetSafeHandle());

	memdc.CreateCompatibleDC(dc);
	ASSERT(memdc.GetSafeHdc());

	memdc.SelectObject(membmp);

	MakeEntries(pal, len);

	POINT pv, ph;

	pv.x = ph.y = 0;

	for (int i = 0; i < len; i++)
	{
		pv.y = ph.x = i;
		entry = &pal[i];
		color = RGB(entry->rgbtRed, entry->rgbtGreen, entry->rgbtBlue);
		memdc.SetPixelV(vertical ? pv : ph, color);
	}

	if (vertical)
		dc->StretchBlt(pRect->left, pRect->top, pRect->Width(), len, &memdc, 0, 0, 1, len, SRCCOPY);
	else
		dc->StretchBlt(pRect->left, pRect->top, len, pRect->Height(), &memdc, 0, 0, len, 1, SRCCOPY);

	//memdc.SelectObject(oldbmp);

	delete[] pal;
}

double CGradient::ValueFromPos(double pos)
{
	return (1.0 - pos)*m_fStartValue + pos * m_fEndValue;
}

LPCSTR CGradient::ValueStr(CString &str, float fVal)
{
	if (m_Flags&GF_USEINTVALUE) {
		char buf[16];
		str = trx_JulToDateStr(INT_LVALUE(fVal), buf);
	}
	else {
		if (!(m_Flags&GF_USELENGTHUNITS) && (fVal == -1.0f || fVal == -2.0f || fVal == -3.0f)) {
			if (fVal == -1.0f) str = "Fixed";
			else if (fVal == -2.0f) str = "Floated";
			else str = "Not in loop";
		}
		else {
			if (m_Flags&GF_USEFEET) fVal *= 3.28084f;
			str.Format("%.2f", (double)fVal);
		}
		if (m_Flags&GF_USELENGTHUNITS) str += (m_Flags&GF_USEFEET) ? " ft" : " m";
	}
	return (LPCSTR)str;
}

double CGradient::PosFromValue(double value)
{
	ASSERT(!(m_Flags&GF_USEINTVALUE));
	double fPos = (value - m_fStartValue) / (m_fEndValue - m_fStartValue);
	fPos = __min(fPos, 1.0);
	return __max(fPos, 0.0);
}

double CGradient::PosFromValue(int value)
{
	ASSERT(m_Flags&GF_USEINTVALUE);
	double fPos = (value - IntStartValue()) / (double)(IntEndValue() - IntStartValue());
	fPos = __min(fPos, 1.0);
	return __max(fPos, 0.0);
}

// Gradient.h: interface for the CGradient class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_GRADIENT_H__B8518180_B289_11D4_A890_C3D6794EED5F__INCLUDED_)
#define AFX_GRADIENT_H__B8518180_B289_11D4_A890_C3D6794EED5F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <afxtempl.h>

#define FLT_LVALUE(iVal) (*(float *)&iVal)
#define INT_LVALUE(fVal) (*(int *)&fVal)

class CPeg : CObject
{
public:
	CPeg() {id = uniqueID, uniqueID++, color = 0x00000000, position = 0.0;};
	CPeg(const CPeg &src) {color = src.color, position = src.position, id = src.id;};
	CPeg& operator = (const CPeg &src) {color = src.color, position = src.position, id = src.id; return *this;};
	void Serialize(CArchive &ar);
	const UINT GetID() const {return id;};

	DECLARE_SERIAL(CPeg)

	COLORREF color;
	double position;
protected:
	UINT id;
	static UINT uniqueID;
};

#define PEG_BACKGROUND -4
#define PEG_START -3
#define PEG_END -2
#define PEG_NONE -1

#define	GF_USEBACKGROUND 1
#define GF_USEINTVALUE 2
#define GF_USEFEET 4
#define GF_USELENGTHUNITS 8

typedef COLORREF (__cdecl* InterpolateFn)(COLORREF first, COLORREF second, double position, double start, double end);

class CGradient : public CObject  
{
public:
	CGradient();
	CGradient(CGradient &gradient);
	virtual ~CGradient();

	CGradient& operator =(CGradient &src);
	
	DECLARE_SERIAL(CGradient)

//----- Attributes -----//
	int GetPegCount() const;
	const CPeg GetPeg(int iIndex) const;
	int SetPeg(int iIndex, COLORREF crColor, double fPosition);
	int SetPeg(int iIndex, CPeg peg);
	void ShiftPegToPosition(int iIndex,double pos);
	int AddPeg(COLORREF crColor, double fPosition);
	int AddPeg(CPeg peg);
	void RemovePeg(int iIndex);
	int IndexFromPos(double pos);

	//Value-related attributes --
	double ValueFromPos(double pos);
	double PosFromValue(double value);
	double PosFromValue(int value);
	LPCSTR ValueStr(CString &str,float fVal);
	LPCSTR ValueStrFromPos(CString &str,double fPos) {return ValueStr(str,(float)ValueFromPos(fPos));}
	
	double StartValue() const {return m_fStartValue;}
	double EndValue() const {return m_fEndValue;}
	int IntStartValue() const {return (int)m_fStartValue;}
	int IntEndValue() const {return (int)m_fEndValue;}

	void SetStartValue(double fStartValue) {m_fStartValue=(double)fStartValue;}
	void SetEndValue(double fEndValue) {m_fEndValue=(double)fEndValue;}
	void SetStartValue(int iStartValue) {m_fStartValue=(double)iStartValue;}
	void SetEndValue(int iEndValue) {m_fEndValue=(double)iEndValue;}
	//--

	void FillRect(CDC *dc,CRect *pRect,bool vertical=false);
	void SetStartPegColor(const COLORREF crColor){m_StartPeg.color = crColor;};
	COLORREF GetStartPegColor() const {return m_StartPeg.color;};
	void SetEndPegColor(const COLORREF crColor) {m_EndPeg.color = crColor;};
	COLORREF GetEndPegColor() const {return m_EndPeg.color;};
	void SetBackgroundColor(const COLORREF crColor) {m_Background.color = crColor;};
	COLORREF GetBackgroundColor() const {return m_Background.color;};

	BOOL GetUseBackground() const {return 0!=(m_Flags&GF_USEBACKGROUND);}
	void SetUseBackground(const BOOL bUseBackground)
	{
		m_Flags=bUseBackground?(m_Flags|GF_USEBACKGROUND):(m_Flags&~GF_USEBACKGROUND);
	}

	void SetUseIntValue(BOOL bUse)
	{
		m_Flags = (m_Flags&~GF_USEINTVALUE)|(bUse*GF_USEINTVALUE);
	}

	void SetUseFeet(BOOL bUse)
	{
		m_Flags = (m_Flags&~GF_USEFEET)|(bUse*GF_USEFEET);
	}

	void SetUseLengthUnits(BOOL bUse)
	{
		m_Flags = (m_Flags&~GF_USELENGTHUNITS)|(bUse*GF_USELENGTHUNITS);
	}

	enum InterpolationMethod
	{
		Linear,
		FlatStart,
		FlatMid,
		FlatEnd,
		Cosine,
		HSLRedBlue,
		HSLBlueRed,
		HSLShortest,
		HSLLongest,
		Reverse
	};

	InterpolationMethod GetInterpolationMethod() const;
	void SetInterpolationMethod(const InterpolationMethod method);
	int GetQuantization() const;
	void SetQuantization(const int entries);

//----- Operations -----//
	void MakePalette(CPalette *lpPalette);
	void Make8BitPalette(RGBTRIPLE *lpPal);
	void MakeEntries(RGBTRIPLE *lpPal, int iEntryCount);
	
	COLORREF ColorFromPosition(double pos);
	COLORREF ColorFromValue(double value) {return ColorFromPosition(PosFromValue(value));}
	COLORREF ColorFromValue(int value) {return ColorFromPosition(PosFromValue(value));}
	
	void Serialize(CArchive &ar);

//----- Internals -----//
protected:
	void SortPegs();
	
	//----- Interpolation routines -----//
	static COLORREF InterpolateLinear(COLORREF first, COLORREF second,
		double position, double start, double end);
	static COLORREF InterpolateFlatStart(COLORREF first, COLORREF second,
		double position, double start, double end);
	static COLORREF InterpolateFlatMid(COLORREF first, COLORREF second,
		double position, double start, double end);
	static COLORREF InterpolateFlatEnd(COLORREF first, COLORREF second,
		double position, double start, double end);
	static COLORREF InterpolateCosine(COLORREF first, COLORREF second,
		double position, double start, double end);
	static COLORREF InterpolateHSLClockwise(COLORREF first, COLORREF second,
		double position, double start, double end);
	static COLORREF InterpolateHSLAntiClockwise(COLORREF first, COLORREF second,
		double position, double start, double end);
	static COLORREF InterpolateHSLLongest(COLORREF first, COLORREF second,
		double position, double start, double end);
	static COLORREF InterpolateHSLShortest(COLORREF first, COLORREF second,
		double position, double start, double end);
	static COLORREF InterpolateReverse(COLORREF first, COLORREF second,
		double position, double start, double end);

public:
	double m_fStartValue;
	double m_fEndValue;

private:	
	int IndexFromId(UINT id);
	void InsertSort(int lb, int ub);
	int Partition(int lb, int ub);
	void QuickSort(int lb, int ub);

protected:
	InterpolateFn GetInterpolationProc();
	POSITION GetNextPeg(POSITION current);
	CArray <CPeg, CPeg&> pegs;
	CPeg m_StartPeg, m_EndPeg, m_Background;
	int m_Quantization;
	InterpolationMethod m_InterpolationMethod;
	UINT m_Flags;
};

#endif // !defined(AFX_GRADIENT_H__B8518180_B289_11D4_A890_C3D6794EED5F__INCLUDED_)

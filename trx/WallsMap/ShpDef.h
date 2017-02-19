#pragma once

#ifndef __SAFEMIRRORFILE_H
#include "SafeMirrorFile.h"
#endif

class CShpLayer;

#define FLDIDX_NOSRC	 -10000
#define FLDIDX_TIMESTAMP -20000
#define FLDIDX_XCOMBO    -30000

#undef _USE_TRX

#ifdef _USE_TRX
#define FLDIDX_INDEX	 -40000
extern bool bUsingIndex;
#endif

struct FILTER
{
	FILTER() : fld(0), bNeg(0), pVals(NULL) {};
	FILTER(UINT f,WORD neg,LPSTR p) : fld(f), bNeg(neg), pVals(p) {};
	WORD fld;
	WORD bNeg;
	LPSTR pVals;
};

typedef std::vector<FILTER> V_FILTER;
typedef V_FILTER::iterator it_filter;

struct REPL_FCN
{
	REPL_FCN() : fld(0) {};
	REPL_FCN(UINT f,LPCSTR pOld,LPCSTR pNew) : fld(f), sOld(pOld), sNew(pNew) {};
	UINT fld;
	CString sOld;
	CString sNew;
};

typedef std::vector<REPL_FCN> V_REPL_FCN;
typedef V_REPL_FCN::iterator it_repl_fcn;

typedef std::vector<DBF_FLDDEF> V_FLDDEF;
typedef std::vector<int> V_FLDIDX;
typedef std::vector<double> V_FLDSCALE;

class CShpDBF;
class CShpLayer;
class CFileCfg;

enum {SHPD_USEVIEWCOORD=1,SHPD_RETAINDELETED=2,SHPD_EXCLUDEMEMOS=4,SHPD_INITEMPTY=8,
	SHPD_GENTEMPLATE=16,SHPD_INITFLDS=32,SHPD_DESCFLDS=64,SHPD_ISCONVERTING=128,SHPD_MAKE2D=256,SHPD_COMBOFLDS=512,SHPD_FILTER=1024,SHPD_REPL=2048};

class CShpDef
{
public:
	CShpDef(void): numFlds(0),numSrcFlds(0),numLocFlds(0),numMemoFlds(0), iPrefixFld(0), uFlags(0), m_pFdef(0) {}
	~CShpDef(void);

	BOOL Process(CShpLayer *pShp,LPCSTR pathName);
	DBF_pFLDDEF FindDstFld(UINT srcFld) const;
	BOOL Write(CShpLayer *pShp,CSafeMirrorFile &cf) const;

	CFileCfg *m_pFdef;
	V_FLDSCALE v_fldScale;
	VEC_XCOMBO v_xc;
	VEC_LPSTR v_fldCombo;
	V_FILTER v_filter;
	V_REPL_FCN v_repl_fcn;
	V_FLDDEF v_fldDef;
	V_FLDIDX v_fldIdx; //source field number if positive
	CString csTimestamps[2];
	CString csPrefixFmt;
	int iPrefixFld;
	int numFlds;
	int numSrcFlds;
	int numLocFlds;
	int numMemoFlds;
	UINT uFlags;
private:
	bool GetReplArgs(UINT f,LPSTR p);
};

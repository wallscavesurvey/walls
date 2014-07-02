#ifndef _FLTPOINT_H
#include "fltpoint.h"
#endif

#define QT_DFLT_DEPTH 5

class CShpLayer;
class CQuadTree;

struct QTNODE {
	UINT lvl;
	UINT nrecs;
	UINT fstrec;
	UINT quad[4]; //0-based indices into pTree->m_pQTN[], root is pTree->m_pQTN[0]
	static UINT maxLvl;
	static UINT insrec;
	static DWORD *piQT;
	static CFltRect *pExt;
	static CQuadTree *pTree;
	static CFltPoint findpt;
	static QTNODE *pQTN;
	#ifdef _USE_QTFLG
	static const CFltRect *pFrm;
	static BYTE *pFlags;
	void InitFlags(CFltRect extN);
	void InitNodeFlags(int i, CFltRect extN, const CFltPoint &fpt);
	#endif
	void AllocRec(CFltRect extN); //construct tree and init nrecs
	void AllocNodeRec(BYTE q, CFltRect ext, const CFltPoint &fpt);
	void InsertRec(CFltRect extN); //insrec value (ext[] index) in allocated array.
	void InsertNodeRec(BYTE q, CFltRect ext, const CFltPoint &fpt);
	DWORD GetPolyRecContainingPt(CFltRect extN);
};

class CQuadTree {
public:
	CQuadTree() :
	   #ifdef _USE_QTFLG
	   m_pFlags(NULL),
	   #endif
	   m_piQT(NULL), m_pQTN(NULL),m_numNodes(0) {}
	~CQuadTree();
	int Create(CShpLayer *pShp);
	DWORD GetPolyRecContainingPt(const CFltPoint &fpt);
	#ifdef QT_TEST
	void QT_TestPoints(LPCSTR ptshpName);
	#endif
	#ifdef _USE_QTFLG
	BYTE *InitFlags(const CFltRect &geoExt);
	BYTE *m_pFlags;
	#endif
	QTNODE *m_pQTN;
	DWORD *m_piQT;
	UINT m_numParts;
	UINT m_tests,m_polyTests;
	UINT m_numNodes;
	UINT m_maxNodes;
	UINT m_maxLvl;
	CFltRect m_extent;
	CShpLayer *m_pShp;
};
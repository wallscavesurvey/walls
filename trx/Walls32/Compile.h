//COMPILE.H -- Include file for compile.cpp and graphics.cpp --

#ifndef __COMPILE__
#define EXTERN extern
#else
#define EXTERN
#endif

#ifndef __DBFILE_H
#include <dbfile.h>
#endif
#ifndef __TRXFILE_H
#include "trxfile.h"
#endif
#ifndef __NTFILE_H
#include "ntfile.h"
#endif
#ifndef __SEGHIER_H
#include "seghier.h"
#endif

//I considered a cache of sequentially encountered names retained in memory
//to minimize index file access. In testing 11364-vector SISTEMA, a 1-name
//default length worked best and still only shaved about one second off the
//compilation time (64k index cache) when duplicates were not searched for!
EXTERN LPBYTE pCache;
EXTERN DWORD wCache;
EXTERN DWORD pfxCache;
EXTERN CSegView *pSV;
EXTERN CPlotView *pPV;
EXTERN CPageView *pGV;
EXTERN CCompView *pCV;
EXTERN CTraView *pTV;

EXTERN int iFindStr;
EXTERN int iFindSel;
EXTERN int mapStr_id;
EXTERN double zLastPt;
EXTERN float fLrudExag;

#define SRV_DLG_INCREMENT 100
#define NULL_ID ((DWORD)0xFFFFFFFF)

#define _NOTESTR(key) ((char *)key+6)
#define _NOTELEN(key) (*(BYTE *)key-5)

#define ND CPrjDoc::net_data

EXTERN NET_WORK *pNET;

class CNETData {
public:
	CNETData(int nFiles);
	~CNETData();
};
	
typedef struct {
  double sys_ss[2];
  double inc_ss[2];
  double cxyz[3];
  DWORD	 sys_nc[2];
} strstats;

EXTERN strstats stSTR;

//F-Ratio value limits for pSV->m_Gradient[2]
#define GRAD_VAL_MAXFRATIO 100.0
#define GRAD_VAL_NONSTRING -3.0
#define GRAD_VAL_FLOATING -2.0
#define GRAD_VAL_FIXED -1.0

typedef struct {
  UINT rec;
  int next;
} pltseg;

//Originally static in graphics.cpp --
EXTERN BOOL bProfile;
EXTERN double xscale,yscale,xoff,yoff;
EXTERN UINT bClipped;
EXTERN BOOL bLastRef;
EXTERN MAPFORMAT *pMF;
EXTERN DWORD *pVnodeMap;
EXTERN int iPolysDrawn;
EXTERN BOOL bNoVectors;

#ifdef VISTA_INC
EXTERN TOOLINFO g_toolItem;
EXTERN HWND g_hwndTrackingTT;
extern HWND CreateTrackingToolTip(HWND hWnd);
extern void DestroyTrackingTT();
#endif

EXTERN CPoint ptMeasureStart,ptMeasureEnd;
EXTERN DPOINT dpCoordinate,dpCoordinateMeasure;
EXTERN BOOL bMeasureDist;
extern void GetDistanceFormat(CString &s,BOOL bProfile,bool bFeetUnits,bool bConvert);

#define pNamTyp CCompView::m_pNamTyp

typedef struct {
	double az,x,y;
} RAYPOINT;

//#define MAX_RAYPOINTS 50

typedef std::vector<RAYPOINT> VEC_RAYPOINT;
typedef std::vector<POINT> VEC_POINT;

EXTERN UINT rayCnt;
EXTERN VEC_RAYPOINT vRayPnt; //stores rays from a point
EXTERN VEC_POINT vPnt; //workspace

EXTERN DWORD numfiles,maxfiles;
EXTERN DWORD numnames;
EXTERN DWORD numvectors,numvectors0;
EXTERN COLORREF clrGradient;

EXTERN double ssISOL[2];
EXTERN double fTotalLength,fTotalLength0;
EXTERN CPrjListNode *pBranch;
EXTERN CDBFile dbNTN;
EXTERN CDBFile dbNTS;
EXTERN CDBFile dbNTP;
EXTERN CNTVFile dbNTV;
EXTERN CTRXFile ixSEG;
EXTERN CTRXFile *pixNAM,*pixOSEG;
EXTERN SEGHDR *pixSEGHDR;
EXTERN NTVRec *pNTV;
EXTERN strtyp *pSTR;
#define pSTR_flag (*(WORD *)&pSTR->flag)
EXTERN vectyp *pVEC;
EXTERN plttyp *pPLT;
EXTERN nettyp *pNTN;

//Initialized to NULL --
EXTERN int *piPLT;
EXTERN int *piSTR;
EXTERN int *piSYS;
EXTERN int *piFRG;

EXTERN int *piLNK;	//Added 11/4/04 to support link fragments
#define LNK_INCREMENT 100

EXTERN pltseg *piPLTSEG;
EXTERN CDC *pvDC;

#define PLTSEG_INCREMENT 200
#define SIZ_EDITNAMEBUF 48
 
//Initialized to 0 --
EXTERN int nLen_piSYS;
EXTERN int nLen_piPLTSEG;  //Current allocated lengths of piSYS[] and piPLTSEG[]

EXTERN int nSTR,nSTR_1,nSTR_2,nSYS,nISOL,nNTV,nPLTSEGS;
EXTERN int nLNKS,nFRGS,nLen_piLNK,nSYS_Offset,nSYS_LinkOffset;  //Added 11/4/04 to support link fragments
EXTERN WORD nISOL_ni[2];
EXTERN char errBuf[NET_SIZ_ERRBUF];

EXTERN char SegBuf[SRV_SIZ_SEGBUF];
EXTERN styletyp SegRec;
EXTERN BOOL bSegChg;

//Accessed by CPlotView::ViewMsg(), etc., and graphics routines --
EXTERN double fView;

//Control point management during compilation, and also FindStation() processing --
EXTERN double FixXYZ[3];
EXTERN int bFixed;

int TrimPrefix(LPBYTE *argv,BOOL bMatchCase);

BOOL LoadNTW();
HPEN GetStylePen(styletyp *pStyle);
double TransAx(double *pENU);
double TransAy(double *pENU);
double TransAz(double *pENU);
PXFORM getXform(PXFORM pf);
UINT OutCode(double x,double y);
void PlotLrudTicks(BOOL bTicksOnly);
void PlotLrudPolys();
BOOL PlotSVGMaskLayer();
apfcn_v PlotVector(int typ);
COLORREF GetPltGradientColor(int idx);
class CGradient;
CGradient *GetGradient(int idx);
	
//EOF

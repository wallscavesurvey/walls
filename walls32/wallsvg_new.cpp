// wallsvg.cpp : Called from CSegView::ExportSVG() in Walls v.2B7
//

#include <windows.h>
#include <stdio.h>
//expat.h includes stdlib.h
#include <expat.h>
#include <stdarg.h>
#include <string.h>
#include <malloc.h>
#include <time.h>
#include <math.h>
#include <float.h>
#include <sys\stat.h>
#include <time.h>
#include <ctype.h>
#include <trx_str.h>
#include <trxfile.h>
#include <zlib.h>
#include "wall_shp.h"

static char buf_titles[SHP_SIZ_BUF_TITLES],buf_attributes[SHP_SIZ_BUF_ATTRIBUTES];

#ifndef DLLExportC
#define DLLExportC extern "C" __declspec(dllexport)
#endif

//=======================================================

//Structures returned from Walls --
static SHP_TYP_VECTOR sV;
static SHP_TYP_FLAGSTYLE sFS;
//Assume  sizeof(sN) >= sizeof(sF) --
static SHP_TYP_FLAG sF;
static SVG_VIEWFORMAT *pSVG;

static double viewA,cosA,sinA,xMid,yMid,grid_xMin,grid_yMin,grid_xMax,grid_yMax,width,height,scale;
static char width_str[20],height_str[20],transformMatrix[48];

static FILE *fpout;
static gzFile fpin,fpmrg,gzout;
static UINT flags;
static BOOL bElementsSeen,bNoOutput,bSymbolScan,bInsideGradient,bLegendSeen;
static BOOL bFirstCommentSeen,bWriting;
static BOOL bInTextElement,bInUse,bNTW;

static double midE,midN;

static CTRXFile trx;
#define SVG_TRX_VECTORS 0
#define SVG_TRX_LABELS 1
#define MAX_SHAPEPTS 15

//===== Merged content variables =============================

#define MAX_VEC_SKIPPED 20
#define SVG_MAX_SIZNAME 63

#pragma pack(1)
typedef struct {
	double fr_xy[2];
	double to_xy[2];
	WORD wVecColor;
} SVG_TYP_TRXVECTOR;

typedef struct {
	double fr_xy[2];
	double to_xy[2];
} SVG_TYP_TRXVECTOR_NTW;

typedef struct {
	float sx;
	float sy;
	float nx;
	float ny;
	BYTE bPresent;
} SVG_TYP_TRXLABEL;

typedef struct {
	double x;
	double y;
	char prefix[SVG_MAX_SIZNAME+1];
	char name[SVG_MAX_SIZNAME+1];
} SVG_ENDPOINT;

typedef struct {
	double x;
	double y;
} SVG_PATHPOINT;

typedef struct {
	float width;
	float height;
} SVG_VIEWBOX;

typedef struct {
	float xmin;
	float ymin;
	float xmax;
	float ymax;
	int pathstyle_idx;
} SVG_PATHRANGE;

typedef struct { //12*8=96 bytes (coords can be floats!) --
	double B_xy[2]; //SVG version: fr_ = B_, to_ = B_ + M_
	double M_xy[2];
	double Bn_xy[2]; //Shift parameters
	double Mn_xy[2];
	double maxdist;
	int rt,lf;
} SVG_VECNODE;
#pragma pack()

#define NUM_HDR_NODES 7
#define NUM_HDR_LEVELS 3

#define LEN_VECLST 10
#define MAX_STYLES 100

/*	Notes:
	Walls and Detail layers --
	In the shp layers, all path points will be warped (independently
	positioned), rotated, and scaled, while text and symbols will be
	rotated and scaled, not warped. In the sym layers, paths will be
	rotated and scaled,	while text and symbols will be scaled only.
	The final world coordinates of all elements in Walls and Details
	will depend on vector shifts. All grouped objects in the sym
	layers will be repositioned as groups. Objects in the shp
	layers will be repositioned individually as elements, except
	that path data points will be indepenently repositioned.
	
	Legend layer --
	All elements will be scaled only and will retain their world
	coodrinates.
*/

const char *st_stroke="stroke:";
const char *st_stroke_width="stroke-width:";
const char *st_stroke_dasharray="stroke-dasharray:";
const char *st_fill="fill:";
const char *st_font_size="font-size:";

enum ew2d_flg {FLG_WALLS_SHP=1,FLG_WALLS_SYM=2,FLG_DETAIL_SHP=4,FLG_DETAIL_SYM=8,
			FLG_WALLS=32,FLG_DETAIL=64,FLG_LEGEND=128,FLG_IMAGE=256, //same as SVG_ versions
			FLG_VECTORS=(1<<11),FLG_LABELS=(1<<12),FLG_BACKGROUND=(1<<13),FLG_FRAME=(1<<14),
			FLG_GRID=(1<<15),FLG_NOTES=(1<<16),FLG_FLAGS=(1<<17),FLG_MARKERS=(1<<18),
			FLG_MASK=(1<<19),FLG_LRUDS=(1<<20),FLG_SURVEY=(1<<21),FLG_SYMBOLS=(1<<22)};

#define NUM_W2DNAMES 20
//w2d Layers examined in the merge file --
static char *w2d_name[NUM_W2DNAMES]={
	"Ref",
	"Detail",
	"Detail_shp",
	"Detail_sym",
	"Walls",
	"Walls_shp",
	"Walls_sym",
	"Legend",
	"Vectors",
	"Labels",
	"Background",
	"Frame",
	"Grid",
	"Notes",
	"Flags",
	"Markers",
	"Mask",
	"Lruds",
	"Survey",
	"Symbols"
};

enum ew2d_idx {ID_REF,ID_DETAIL,ID_DETAIL_SHP,ID_DETAIL_SYM,
			ID_WALLS,ID_WALLS_SHP,ID_WALLS_SYM,ID_LEGEND,ID_VECTORS,
			ID_LABELS,ID_BACKGROUND,ID_FRAME,ID_GRID,ID_NOTES,ID_FLAGS,
			ID_MARKERS,ID_MASK,ID_LRUDS,ID_SURVEY,ID_SYMBOLS};

static int w2d_flg[NUM_W2DNAMES]={0,FLG_DETAIL,FLG_DETAIL_SHP,FLG_DETAIL_SYM,
			FLG_WALLS,FLG_WALLS_SHP,FLG_WALLS_SYM,FLG_LEGEND,FLG_VECTORS,
			FLG_LABELS,FLG_BACKGROUND,FLG_FRAME,FLG_GRID,FLG_NOTES,FLG_FLAGS,
			FLG_MARKERS,FLG_MASK,FLG_LRUDS,FLG_SURVEY,FLG_SYMBOLS};


#define FLG_MERGEDLAYERS (FLG_WALLS+FLG_DETAIL+FLG_LEGEND+FLG_MASK)
#define FLG_SHP_MSK (FLG_WALLS_SHP+FLG_DETAIL_SHP+FLG_MASK)
#define FLG_SYM_MSK (FLG_WALLS_SYM+FLG_DETAIL_SYM)
#define FLG_DETAIL_MSK (FLG_DETAIL_SHP+FLG_DETAIL_SYM)
#define FLG_WALLS_MSK (FLG_WALLS_SHP+FLG_WALLS_SYM)
#define FLG_ADJUSTED (FLG_SHP_MSK+FLG_SYM_MSK)
#define FLG_WALLS_DETAIL_MSK (FLG_DETAIL_MSK+FLG_WALLS_MSK)
#define FLG_STYLE_MSK (FLG_LABELS+FLG_BACKGROUND+FLG_FRAME+FLG_GRID+FLG_NOTES+FLG_FLAGS+FLG_MARKERS+FLG_LRUDS)

#define MAX_VECCOLORS 20
#define MAX_SYMNAMES 50
#define SIZ_REFBUF 384
static char refbuf[SIZ_REFBUF];
#define SIZ_GZBUF 512
static char gzbuf[SIZ_GZBUF];

static NTW_HEADER ntw_hdr;
static int ntw_count,ntw_size,nMaskOpenPaths,nNTW_OpenPathLine;
static POINT *pNTWp;
static byte *pNTWf;
static NTW_POLYGON *pnPly;
static BOOL bAllocErr,bReverseY;

#define MAX_MSKLVL 10
#define MASKNAME_INC 20
static int msklvl,nMaskName,maxMaskName;
static byte bOuterSeen[MAX_MSKLVL];
static int iMaskName[MAX_MSKLVL];
static SHP_MASKNAME *pMaskName;

static NTW_RECORD *pNtwRec;
static int maxNtwRec,nNtwRec;
#define MAXNTWREC_INC 40

#define MAX_PATHCNT_INC 1000
#define MAX_SYMCNT_INC 200
static SVG_PATHRANGE *pathrange,*symrange;
static double symTransX,symTransY;
static UINT reflen,siz_vecrec;
static BOOL bAdjust,bSkipSym;
static SVG_VECNODE *vnod;
static int *vstk;
static int vnod_siz,vnod_len,max_slevel;
static int veccnt,skipcnt,iLevel,iLevelSym,iLevelID,numGridThick;
static double fLabelSize,fNoteSize,fTitleSize,fMarkerThick,fVectorThick,fGridThick[2];
static COLORREF cBackClr,cPassageClr,cMarkerClr,cGridClr,cLrudClr;
static double fFrameThick;
static byte pathstyle_used[MAX_STYLES+1];
static char *pathstyle[MAX_STYLES+1],*textstyle[MAX_STYLES+1],*legendstyle[MAX_STYLES+1];
static char *symname[MAX_STYLES+1];
static float flagscale[MAX_STYLES+1];
static SVG_VIEWBOX flagViewBox[MAX_STYLES+1];
static SVG_PATHPOINT legendPt,*pLegendPt;
static int skiplin[MAX_VEC_SKIPPED];
static double max_dsquared,max_dsq_x,max_dsq_y;
static double mrgViewA,mrgCosA,mrgSinA,xMrgMid,yMrgMid,xMrgMin,yMrgMin,xMrgMax,yMrgMax;
static double mrgMidE,mrgMidN,mrgScale,scaleRatio;
static UINT node_visits;
static UINT tpoints,tpoints_w,tpoints_d,pathcnt,max_pathcnt,symcnt,max_symcnt,tptotal,tppcnt;
static double flength_w,flength_d;
static int max_iv[LEN_VECLST];  //index of nearest vector to last point
static int veclst_i[LEN_VECLST];
static double veclst_d[LEN_VECLST];
static double dist_exp,dist_off,len_exp;
static double xpath_min,xpath_max,ypath_min,ypath_max;
static double min_veclen_sq,max_diffx,max_diffy;
static int vecs_used,decimals;
static SVG_TYP_TRXLABEL labelRec;
static char *pNextPolyPt;

#pragma pack(1)
typedef struct {
	double a;
	double b;
	double c;
	double d;
	double e;
	double f;
} SVG_CTM;
#pragma pack()
	
static SVG_CTM utm_to_xy,utm_to_mrgxy,mrgxy_to_xy,xy_to_mrgxy,pathCTM,symTransCTM;
static BOOL bPathCTM;

//static clock_t tm_scan,tm_walls;

//=================================

typedef struct {
	WORD wFlag;		/*flag count for this style*/
	WORD wPriority; /*0=highest*/
	WORD wSize;		/*size in points*/
	BYTE bShape;	/*0-square,1-circle,2-triangle,3-plus*/
	BYTE bShade;	/*0-clear,1-solid,2-opaque*/
	DWORD color;
	char *pFlagname;
} SVG_TYP_FLAGSTYLE;

typedef struct {
	double xy[2];
	UINT flag_id;
} SVG_TYP_FLAGLOCATION;

static SVG_TYP_FLAGSTYLE *pFS;
static SVG_TYP_FLAGLOCATION *pFL;

static UINT numFlagStyles;
static UINT numFlagLocations,numNoteLocations;
static char *pLabelStyle,*pNoteStyle;
static BOOL bUseStyles,bUseLabelPos,bMarkerWritten,bMaskPathSeen;
static char vecColor[MAX_VECCOLORS][6];
static int numVecColors;
static int iGroupFlg,iGroupID;
static LPFN_EXPORT_CB pGetData;

static char argv_buf[2*_MAX_PATH];

int outBackground();
int outFrame();
int outGrid();
int outRef();
int outMergedContent();
int outVectors();
int outLruds();
int outMarkers();
int outFlags();
int outLabels();
int outNotes();

#define NUM_GROUPS 12

typedef int (*GRP_FCN_TYP)();

static GRP_FCN_TYP grp_fcn[NUM_GROUPS+1]={
	NULL,
	&outBackground,
	&outRef,
	NULL,
	&outVectors,
	&outLruds,
	&outMarkers,
	&outFlags,
	&outLabels,
	&outNotes,
	&outGrid,
	&outFrame,
	NULL
};

static char *grp_id[NUM_GROUPS]={
	"svg",
	"w2d_Background",
	"w2d_Ref",
	"w2d_Survey",
	"w2d_Vectors",
	"w2d_Lruds",
	"w2d_Markers",
	"w2d_Flags",
	"w2d_Labels",
	"w2d_Notes",
	"w2d_Grid",
	"w2d_Frame"
};

#define MAX_DEPTH 10
static int grp_typ[MAX_DEPTH];

enum e_gtyp {W2D_SVG,W2D_BACKGROUND,W2D_REF,W2D_SURVEY,W2D_VECTORS,W2D_LRUDS,
	W2D_MARKERS,W2D_FLAGS,W2D_LABELS,W2D_NOTES,W2D_GRID,W2D_FRAME};

#define W2D_IGNORE -3

enum {
  SVG_ERR_SUCCESS,
  SVG_ERR_ABORTED,
  SVG_ERR_SVGCREATE,
  SVG_ERR_WRITE,
  SVG_ERR_XMLREAD,
  SVG_ERR_XMLPARSE,
  SVG_ERR_VERSION,
  SVG_ERR_NOMEM,
  SVG_ERR_INDEXINIT,
  SVG_ERR_INDEXWRITE,
  SVG_ERR_NOVECTORS,
  SVG_ERR_MRGOPEN,
  SVG_ERR_BADIDSTR,
  SVG_ERR_VECNOTFOUND,
  SVG_ERR_VECPATH,
  SVG_ERR_VECDIRECTION,
  SVG_ERR_WALLPATH,
  SVG_ERR_NOPATHID,
  SVG_ERR_REFFORMAT,
  SVG_ERR_SIZREFBUF,
  SVG_ERR_NOREF,
  SVG_ERR_NOTEXT,
  SVG_ERR_NOPATH,
  SVG_ERR_LAYERS,
  SVG_ERR_BADELEMENT,
  SVG_ERR_TSPAN,
  SVG_ERR_NOMASK,
  SVG_ERR_FLATTEN,
  SVG_ERR_MASKPATHSOPEN,
  SVG_ERR_MASKNONPATH,
  SVG_ERR_GROUPLVL,
  SVG_ERR_W2DNAME,
  SVG_ERR_LINEDATA,
  SVG_ERR_RECTDATA,
  SVG_ERR_ELLIPSE,
  SVG_ERR_POLYDATA,
  SVG_ERR_BADXFORM,
  SVG_ERR_ROTXFORM,
  SVG_ERR_UNKNOWN,
  SVG_ERR_ENDSCAN
};

static NPSTR errstr[]={
  "operation successful",
  "User abort",
  "Cannot create",
  "Error writing",
  "Error reading file",
  "Error parsing",
  "Incompatible version of wallsvg.dll",
  "Not enough memory or resources",
  "Error creating temporary index",
  "Error writing temporary index",
  "No survey vectors in view",
  "Can't open SVG with content to merge",
  "Wrong format for vector path ID",
  "Vector %s in merged SVG not part of exported data",
  "Wrong format for vector definition",
  "Vector direction difference > 90 deg",
  "Wrong format for path attribute",
  "Missing path ID or style in vector group",
  "Unrecognized w2d_Ref format",
  "String in w2d_Ref group too long",
  "No w2d_Ref group prior to w2d_Vectors",
  "Text element expected",
  "Path or line element expected",
  "Bad arrangement of predefined layers",
  "Bad element type in predefined layer",
  "Unsupported <text>..<tspan> arrangement",
  "No w2d_Mask layer in source SVG",
  "Could not flatten curve",
  "Mask layer has %u open path(s) with fill assigned (line %d)",
  "Only path elements expected in mask layer",
  "Group depth limit exceeded",
  "Unrecognized w2d_* layer name (check case)",
  "Line endpoint coordinates expected",
  "Rect attributes expected",
  "Circle or ellipse attributes expected",
  "Bad format for polygon data",
  "Unsupported transform attribute",
  "Skew transforms not supported",
  "Unknown error"
};

enum e_fmt_mkr {FMT_MKR_SQUARE,FMT_MKR_CIRCLE,FMT_MKR_TRIANGLE,FMT_MKR_PLUS,FMT_MKR_MARKER};
#define NUM_PATHTYPES 7
enum e_pathtype {P_PATH=1,P_RECT,P_LINE,P_POLYGON,P_POLYLINE,P_ELLIPSE,P_CIRCLE};
static LPCSTR pathType[NUM_PATHTYPES+1]={NULL,"path","rect","line","polygon","polyline","ellipse","circle"};
static int iPathType;

//static char *pEvenOddRule="fill-rule:evenodd;clip-rule:evenodd;";

static char *pSymbolStr[5]={
	"\t<symbol id=\"%s\">\r\n\t\t"
		"<rect fill=\"%s\" stroke=\"%s\" stroke-width=\"0.12\" fill-opacity=\"%s\""
		" x=\"0.06\" y=\"0.06\" width=\"1.88\" height=\"1.88\"/>\r\n"
	"\t</symbol>\r\n",
	"\t<symbol id=\"%s\">\r\n\t\t"
		"<circle fill=\"%s\" stroke=\"%s\""
		" stroke-width=\"0.12\" fill-opacity=\"%s\" r=\"0.94\" cx=\"1\" cy=\"1\"/>\r\n"
	"\t</symbol>\r\n",
	"\t<symbol id=\"%s\">\r\n\t\t"
		"<rect fill=\"none\" x=\"0\" y=\"0\" width=\"2\" height=\"2\"/>\r\n\t\t"
		"<path fill=\"%s\" stroke=\"%s\" stroke-width=\"0.10\" fill-opacity=\"%s\""
		" d=\"M0.238,0.48H1.762L1,1.8z\"/>\r\n"
	"\t</symbol>\r\n",
	"\t<symbol id=\"%s\">\r\n\t\t"
		"<path stroke=\"%s%s\" stroke-width=\"0.12\" d=\"M0,1H2M1,0V2\"/>\r\n"
	"\t</symbol>\r\n",
	"\t<symbol  id=\"_m\">\r\n\t\t"
		"<!--marker-->\r\n\t\t"
		"<path style=\"fill:%s;\" d=\"M0,0h2v2H0z\"/>\r\n"
	"\t</symbol>\r\n"
};

//=========================================================================
//Expat parser related data and functions --
//=========================================================================
#define MRGBUFFSIZE 16384
#define SIZ_LINEBREAK 128
#define SIZ_PATH_LINEBREAK 130
#define SIZ_SVG_LINEBREAK 50

static char MrgBuff[MRGBUFFSIZE];

typedef struct {
	int bEntity;
	int errcode;
	int errline;
} TYP_UDATA;

static TYP_UDATA udata;

static int Depth,bNoClose,nCol;
static XML_Parser parser,mrgparser;
static int parser_ErrorCode;
static int parser_ErrorLineNumber;
static char *pTemplate;
static UINT uTemplateLen;

#define OUTC(c) gz_putc(c)
#define OUTS(s) gz_puts(s)
#define UD(t) ((TYP_UDATA *)userData)->t

#define PI 3.141592653589793
#define PI2 (PI/2.0)
#define U_DEGREES (PI/180.0)

static void gz_write(LPCSTR s,int len)
{
	if(gzout) gzwrite(gzout,(const voidp)s,len);
	else fwrite(s,len,1,fpout);
}

static void gz_putc(int c)
{
	if(gzout) gzputc(gzout,c);
	else putc(c,fpout);
}

static void gz_puts(LPCSTR s)
{
	if(gzout) gzputs(gzout,s);
	else fputs(s,fpout);
}

static int gz_printf(char *format,...)
{
	int cnt;
    va_list marker;
    va_start(marker,format);
    cnt=_vsnprintf(gzbuf,SIZ_GZBUF,format,marker);
    va_end(marker);
	if(cnt<0) return 0;
	if(gzout) gzputs(gzout,gzbuf);
	else fputs(gzbuf,fpout);
	return cnt;
}

//Geometric transformations --
#ifdef _DEBUG
static int compare_vec(double *x1,double *x2)
{
	int i=0;
	if(fabs(*x1-*x2)>0.0000001 || fabs(x1[1]-x2[1])>0.0000001) {
		i=1;
	}
	return i;
}
#endif

static void ctm_mult(SVG_CTM *ctm,double *xy)
{
	double x=xy[0];
	xy[0]=ctm->a*x + ctm->c*xy[1] + ctm->e;
	xy[1]=ctm->b*x + ctm->d*xy[1] + ctm->f;
}

static void ctm_mult(SVG_CTM *ctm,double *x1,double *x2)
{
	double x=x1[0];
	x2[0]=ctm->a*x + ctm->c*x1[1] + ctm->e;
	x2[1]=ctm->b*x + ctm->d*x1[1] + ctm->f;
}

static void ctm_mult(SVG_CTM *ctmlf,SVG_CTM *ctmrt)
{
	double x=ctmrt->a;
	ctmrt->a=ctmlf->a*x + ctmlf->c*ctmrt->b;
	ctmrt->b=ctmlf->b*x + ctmlf->d*ctmrt->b;
	x=ctmrt->c;
	ctmrt->b=ctmlf->a*x + ctmlf->c*ctmrt->d;
	ctmrt->c=ctmlf->b*x + ctmlf->d*ctmrt->d;
	x=ctmrt->e;
	ctmrt->e+=(ctmlf->a*x + ctmlf->c*ctmrt->f);
	ctmrt->f+=(ctmlf->b*x + ctmlf->d*ctmrt->f);
}

static void ctm_invert(SVG_CTM *ctm,SVG_CTM *ctmi)
{
	double det=1.0/(ctm->a*ctm->d-ctm->b*ctm->c);
	ctmi->a=ctm->d*det;
	ctmi->b=-ctm->b*det;
	ctmi->c=-ctm->c*det;
	ctmi->d=ctm->a*det;
	ctmi->e=(ctm->c*ctm->f-ctm->d*ctm->e)*det;
	ctmi->f=(ctm->b*ctm->e-ctm->a*ctm->f)*det;
}

static void getRotateCTM(SVG_CTM *ctm,SVG_PATHPOINT *pp,double angle)
{
	ctm->a =  ctm->d = cos(angle);
	ctm->c = -(ctm->b = sin(angle));
	if(pp) {
		ctm->e = pp->x - ctm->a*pp->x - ctm->c*pp->y;
		ctm->f = pp->y - ctm->b*pp->x - ctm->d*pp->y;
	}
	else ctm->e=ctm->f=0.0;
}

static void init_utm_to_xy()
{
	//Matrix to convert world coordinates to exported page coordinates --
	double sA=scale*sinA;
	double cA=scale*cosA;

	utm_to_xy.a =  cA;
	utm_to_xy.b = -sA;
	utm_to_xy.c = -sA;
	utm_to_xy.d = -cA;
	utm_to_xy.e = xMid + sA*midN - cA*midE;
	utm_to_xy.f = yMid + sA*midE + cA*midN;
}

static void init_utm_to_mrgxy()
{
	//Matrix to convert world coordinates to merge SVG's page coordinates --
	double sA=mrgScale*mrgSinA;
	double cA=mrgScale*mrgCosA;

	utm_to_mrgxy.a =  cA;
	utm_to_mrgxy.b = -sA;
	utm_to_mrgxy.c = -sA;
	utm_to_mrgxy.d = -cA;
	utm_to_mrgxy.e = xMrgMid + sA*mrgMidN - cA*mrgMidE;
	utm_to_mrgxy.f = yMrgMid + sA*mrgMidE + cA*mrgMidN;
}

static void init_xy_to_mrgxy()
{
	//Matrix to convert from merge SVG's page coordinates to exported page coordinates --
	//mrgxy_to_xy = utm_to_xy * inverse(utm_to_mrgxy)

	ctm_invert(&utm_to_mrgxy,&mrgxy_to_xy);
	ctm_mult(&utm_to_xy,&mrgxy_to_xy);
	ctm_invert(&mrgxy_to_xy,&xy_to_mrgxy);
}

static double *get_xy(double *xy,double *xyz)
{
	//Convert world coordinates to exported page coordinates --
	ctm_mult(&utm_to_xy,xyz,xy);
	return xy;
}

static int write_polygons()
{
	//Convert PolyBezier array to polygons with BYTE flag array replaced
	//with array of integer polygon lengths.
	int i=0;

	if(!pNTWp || !pNTWf) return SVG_ERR_NOMEM;

	HDC hdc=::GetDC(NULL);
	if(!hdc || !::BeginPath(hdc) || !::PolyDraw(hdc,pNTWp,pNTWf,ntw_count) ||
		!::EndPath(hdc) || !::FlattenPath(hdc)) {
		if(hdc) ::ReleaseDC(NULL,hdc);
		return SVG_ERR_FLATTEN;
	}

	i=::GetPath(hdc,NULL,NULL,0);
	if(i>ntw_count) {
		if(!(pNTWp=(POINT *)realloc(pNTWp,i*sizeof(POINT))) ||
		   !(pNTWf=(byte *)realloc(pNTWf,i))) i=0;
	}
	if((i && i!=::GetPath(hdc,pNTWp,pNTWf,i)) || !::ReleaseDC(NULL,hdc)) i=0;
	if(!i) {
		return SVG_ERR_FLATTEN;
	}

	ntw_hdr.nPoints=ntw_count=i;
	ntw_hdr.nPolygons=max_pathcnt; //shouldn't have changed
	ntw_hdr.minE=ntw_hdr.minN=ntw_hdr.maxE=ntw_hdr.maxN=0;
	
	//Now let's update the array pnPly[] with point counts and calculate range.
	//The pnPly array already has the flags (interior/exterior, closed/nonclosed)
	//set in the MSB of this integer --
	NTW_POLYGON *pPly=pnPly;
	BYTE *pFlg=pNTWf;
	POINT *pPnt=pNTWp;
	UINT nPly=1,np=0;

	for(i=0;i<ntw_count;i++) {
		if(pPnt->x<ntw_hdr.minE) ntw_hdr.minE=pPnt->x;
		if(pPnt->y<ntw_hdr.minN) ntw_hdr.minN=pPnt->y;
		if(pPnt->x>ntw_hdr.maxE) ntw_hdr.maxE=pPnt->x;
		if(pPnt->y>ntw_hdr.maxN) ntw_hdr.maxN=pPnt->y;
		if(((*pFlg)&PT_MOVETO)==PT_MOVETO) {
			if(i) {
#ifdef _DEBUG
				if(((pFlg[-1]&PT_CLOSEFIGURE)!=0)!=((pPly->flgcnt&NTW_FLG_OPEN)==0)) 
					return SVG_ERR_FLATTEN;
#endif
				if((*pFlg)&PT_CLOSEFIGURE) {
					#ifdef _DEBUG
						return SVG_ERR_FLATTEN;
					#endif
					np=0; //never happens with PolyDraw98
				}
				if(nPly<max_pathcnt) (pPly++)->flgcnt|=(np&NTW_FLG_MASK);
				nPly++;
			}
			np=0;
		}
		np++;
		pFlg++;
		pPnt++;
	}
	pPly->flgcnt|=(np&NTW_FLG_MASK);

	if(nPly!=max_pathcnt) {
		return SVG_ERR_UNKNOWN;
	}

	ntw_hdr.nPolyRecs=nNtwRec;
	ntw_hdr.nMaskName=nMaskName;

	//Write NTW file --
	if(!(fpout=fopen(pSVG->svgpath,"wb"))) {
	  return SVG_ERR_SVGCREATE;
	}

	if(fwrite(&ntw_hdr,sizeof(ntw_hdr),1,fpout)!=1 ||
		fwrite(pNTWp,sizeof(POINT)*ntw_count,1,fpout)!=1 ||
		fwrite(pnPly,sizeof(NTW_POLYGON)*nPly,1,fpout)!=1 ||
		fwrite(pNtwRec,sizeof(NTW_RECORD)*nNtwRec,1,fpout)!=1) {
	  return SVG_ERR_SVGCREATE;
	}
	
	if(fwrite(pMaskName,sizeof(SHP_MASKNAME)*nMaskName,1,fpout)!=1) {
	  return SVG_ERR_SVGCREATE;
	}

	return 0;
}

static void get_ntw_point(POINT *p,SVG_PATHPOINT *mp)
{
	//Convert from merge SVG's page coordinates to point in NTW file --
	double x=(mp->x-xMrgMid);
	double y=(mp->y-yMrgMid);
	//double E=x*mrgCosA-y*mrgSinA;  //(E/mrgScale)+mrgMidE is UTM coordinate
	//double N=-x*mrgSinA-y*mrgCosA;
	p->x=(long)((x*mrgCosA-y*mrgSinA)*1000);
	p->y=(long)((-x*mrgSinA-y*mrgCosA)*1000);
}

static void out_ntw_point(POINT *pt,byte flg)
{
	#define NTW_COUNT_INC 5000
	if(bAllocErr) return;
	if(ntw_count==ntw_size) {
		ntw_size+=NTW_COUNT_INC;
		if(!(pNTWp=(POINT *)realloc(pNTWp,ntw_size*sizeof(POINT))) ||
			!(pNTWf=(byte *)realloc(pNTWf,ntw_size))) {
			bAllocErr=TRUE;
			return;
		}
	}
	pNTWp[ntw_count]=*pt;
	pNTWf[ntw_count++]=flg;
}

static void out_ntw_points(SVG_PATHPOINT *pp,char typ)
{
	static POINT lastPt,lastCtl;
	static byte lastFlg;
    
	if(typ!='m') {
		if(!pp) {
			if(typ=='z') lastFlg|=PT_CLOSEFIGURE;
			else {
				pnPly[pathcnt].flgcnt|=NTW_FLG_OPEN;
				if(!(pnPly[pathcnt].flgcnt&NTW_FLG_NOFILL)) {
					if(!nMaskOpenPaths) nNTW_OpenPathLine=XML_GetCurrentLineNumber(parser);
					nMaskOpenPaths++;
				}
			}
		}
		out_ntw_point(&lastPt,lastFlg);
	}

	if(!pp) return;

	switch(typ) {
		case 'm' :
		case 'l' :	get_ntw_point(&lastPt,pp);
					lastFlg=(typ=='m')?PT_MOVETO:PT_LINETO;
					break;

		case 'c' :	get_ntw_point(&lastCtl,pp++);
					out_ntw_point(&lastCtl,PT_BEZIERTO);
					get_ntw_point(&lastCtl,pp++);
					out_ntw_point(&lastCtl,PT_BEZIERTO);
					get_ntw_point(&lastPt,pp);
					lastFlg=PT_BEZIERTO;
					break;

		case 's' :  if(lastFlg==PT_BEZIERTO) {
						lastPt.x+=(lastPt.x-lastCtl.x);
						lastPt.y+=(lastPt.y-lastCtl.y);
					}
					tpoints++;
					out_ntw_point(&lastPt,PT_BEZIERTO);
					get_ntw_point(&lastCtl,pp++);
					out_ntw_point(&lastCtl,PT_BEZIERTO);
					get_ntw_point(&lastPt,pp);
					lastFlg=PT_BEZIERTO;
					break;
	}

}

static int GetPathType(LPCSTR name)
{
	for(int i=1;i<=NUM_PATHTYPES;i++) {
		if(!strcmp(name,pathType[i])) break;
	}
	return iPathType=(i<=NUM_PATHTYPES)?i:0;
}

static BOOL isInside(double *xy)
{
	return xy[0]>0 && xy[1]>0 && xy[0]<width && xy[1]<height;
}

static void out_xmlstr(LPCSTR p)
{
	char buf[256];
	char *p0=buf;
	while(*p) {
		switch(*p) {
			case '\"'	: strcpy(p0,"&quot;"); p0+=6; break;
			case '\''	: strcpy(p0,"&apos;"); p0+=6; break;
			case '>'	: strcpy(p0,"&gt;"); p0+=4; break;
			case '<'	: strcpy(p0,"&lt;"); p0+=4; break;
			case '&'	: strcpy(p0,"&amp;"); p0+=5; break;
			default		: *p0++=*p;
		}
		p++;
	}
	*p0=0;
	OUTS(buf);
}

static char *flagname_str(char *p)
{
	static char _namebuf[256];
	char *p0=_namebuf;
	*p0++='_';
	while(*p) {
	  if(*p>='A' && *p<='Z' || *p>='a' && *p<='z' || *p>='0' && *p<='9' ||
		*p=='.' || *p=='-') *p0++=*p++;
	  else p0+=sprintf(p0,"_%02x",(UINT)*(unsigned char *)p++);
	}
	*p0=0;
	return _namebuf;
}

static char * fltstr(char *fbuf,double f,int dec)
{
	//Obtains the minimal width numeric string of f rounded to dec decimal places.
	//Any trailing zeroes (including trailing decimal point) are truncated.
	BOOL bRound=TRUE;

	if(dec<0) {
		bRound=FALSE;
		dec=-dec;
	}
	if(_snprintf(fbuf,20,"%-0.*f",dec,f)==-1) {
	  *fbuf=0;
	  return fbuf;
	}
	if(bRound) {
		char *p=strchr(fbuf,'.');
		for(p+=dec;dec && *p=='0';dec--) *p--=0;
		if(*p=='.') {
		   *p=0;
		   if(!strcmp(fbuf,"-0")) strcpy(fbuf,"0");
		}
	}
	return fbuf;
}

static char * fltstr(double f,int dec)
{
	static char fbuf[40];
	return fltstr(fbuf,f,dec);
}

static char * fltstr(double f)
{
	return fltstr(f,decimals);
}

static char * fltstr(char *fbuf,double f)
{
	return fltstr(fbuf,f,decimals);
}

//===========================================================
//Output functions --

void process_points(SVG_PATHPOINT *pp,int npts);
char *getTransform(char *pTrans,LPCSTR *attr,UINT uScale);

static int skip_depth(void)
{
	int i;
	for(i=0;i<Depth;i++) OUTC('\t');
	return Depth;
}

static void gz_outPath(const char *pStart)
{
	//Output string pStart, inserting a line break and Depth tabs when
	//(and only when) a string of 2 or more space characters is
	//encountered (in which the string of spaces is not output).
	const char *p0;
	int col;
	
	while(p0=strchr(pStart,' ')) {
		for(col=1;*++p0==' ';col++);
		if(col<=1) col=0;
		gz_write(pStart,p0-pStart-col);
		if(col && *p0) {
		  OUTS("\r\n");
		  skip_depth();
		}
		pStart=p0;
	}
	if(*pStart) OUTS(pStart);
}

static void out_stream(char *p)
{
	//Update nCol while outputting all data --
	OUTS(p);
	while(*p) {
		if(*p++=='\n') nCol=0;
		else nCol++;
	}
}

static void out_wraptext(LPCSTR p)
{
	for(;*p;p++) {
		if(*p==' ' && nCol>SIZ_LINEBREAK) {
			  OUTS("\r\n");
			  nCol=skip_depth();
		}
		else {
			OUTC(*p);
			nCol++;
		}
	}
}

static int trimctrl(XML_Char *s)
{
	// Replace whitespace spans with single spaces,
	// but eliminate all leading whitespace.
	// Note: isgraph(c)==1 if c printable and not space.

	XML_Char *p=s;
	int len=0;
	int bCtl=1;
	while(*s) {
		if(isgraph(*s)) bCtl=0;
		else {
		  if(bCtl) {
		    s++;
		    continue;
		  }
		  bCtl=1;
		  *s=' '; //shouldn't be required, since expat has done this
		}
	    *p++=*s++;
	    len++;
	}
	*p=0;
	return len;
}

static void outStartGroup(int id,BOOL bHide)
{
	skip_depth();
	gz_printf("<g id=\"w2d_%s\"",w2d_name[id]);
	if(bHide) OUTS(" style=\"display:none\"");
	if(id==ID_LEGEND) gz_printf(" transform=\"matrix(%s)\"",argv_buf);
	OUTS(">\r\n");
	Depth++;
}

static void outGradientStart(LPCSTR *attr)
{
	SVG_PATHPOINT pt,ptTrans;
	char attrbuf[4];

	BOOL bTrans=(getTransform((char *)&ptTrans,attr,3)!=NULL);

	for(;attr[0];attr+=2) {
		if(!strcmp(attr[0],"x1") || !strcmp(attr[0],"x2") || !strcmp(attr[0],"cx")) {
			pt.x=atof(attr[1]);
			continue;
		}
		if(!strcmp(attr[0],"y1") || !strcmp(attr[0],"y2") || !strcmp(attr[0],"cy")) {
			strcpy(attrbuf,attr[0]);
			pt.y=atof(attr[1]);
			//transform and output point --
			if(bTrans) {
				//For now, assume matrix prefix is "1 0 0 -1". Consistent with Illustrator 10?
				//AI 10+ must have fixed this -- it's now "1 0 0 1" !!!
				pt.x+=ptTrans.x;
				if(bReverseY) pt.y=-pt.y;
				pt.y+=ptTrans.y;
			}
			process_points(&pt,0);
			strcpy(attrbuf,attr[0]);
			if(*attrbuf=='y') *attrbuf='x';
			else attrbuf[1]='x';
			gz_printf(" %s=\"%s\"",attrbuf,fltstr(pt.x));
			gz_printf(" %s=\"%s\"",attr[0],fltstr(pt.y));
			continue;
		}
		if(!strcmp(attr[0],"r")) {
			gz_printf(" r=\"%s\"",fltstr(atof(attr[1])*scaleRatio));
			continue;
		}
		if(!strcmp(attr[0],"fx") || !strcmp(attr[0],"fy") ||
			!strcmp(attr[0],"gradientTransform")) continue;

		gz_printf(" %s=\"%s\"",attr[0],attr[1]);
	}
}

static int outPathCTM()
{
	double *pd=&pathCTM.a;
	char fbuf[6][20];

	for(int len=0;len<6;len++) fltstr(fbuf[len],*pd++);

	return gz_printf("transform=\"matrix(%s %s %s %s %s %s)\"",
		fbuf[0],fbuf[1],fbuf[2],fbuf[3],fbuf[4],fbuf[5]);
}

static void outStartElement(LPCSTR name,LPCSTR *attr)
{
	bInUse=!strcmp(name,"use");

	if(bNoClose) {
		OUTS(">\r\n");
	    bNoClose=FALSE;
	}
	skip_depth();
	OUTC('<');
	OUTS(name);

	if(bInsideGradient==1) outGradientStart(attr);
	else {
		for(;attr[0];attr+=2) {
			if(bInUse && !strcmp(attr[0],"transform")) {
				OUTC(' ');
				outPathCTM();
			}
			else {
				if(!strcmp(attr[0],"d")) {
					OUTS(" d=\"");
					gz_outPath(attr[1]);
					OUTC('\"');
				}
				else gz_printf(" %s=\"%s\"",attr[0],attr[1]);
			}
		}
	}
	if(bInUse || bInsideGradient==2 || GetPathType(name)) bNoClose=TRUE;
	else OUTS(">\r\n");
	if(bInsideGradient==1) bInsideGradient++;
	Depth++;
}

static void outEndElement(LPCSTR name)
{
	Depth--;
	if(bNoClose) {
		OUTS("/>\r\n");
		bNoClose=FALSE;
	}
	else {
		skip_depth();
		gz_printf("</%s>\r\n",name);
	}
}

static LPCSTR  get_symname(LPCSTR *attr)
{
	LPCSTR pnam=NULL;
	while(attr[0]) {
		if(!strcmp(attr[0],"xlink:href")) {
			pnam=attr[1];
			if(pnam[0]=='#') pnam++;
			else pnam=NULL;
			break;
		}
		attr+=2;
	}
	return pnam;
}

static int save_style(char **style,LPCSTR st)
{
	int i;
	for(i=0;i<MAX_STYLES;i++) {
		if(!style[i]) {
			if(!(style[i]=(char *)malloc(strlen(st)+1))) return MAX_STYLES;
			strcpy(style[i],st);
			style[i+1]=NULL;
			break;
		}
		if(!strcmp(st,style[i])) break;
	}
	return i;
}

static int save_symname(LPCSTR *attr)
{
	LPCSTR pnam=get_symname(attr);
	return pnam?(save_style(symname,pnam)+1):(MAX_STYLES+1);
}

static int find_style(char **style,LPCSTR st)
{
	if(!st) return 0;
	for(int i=0;i<MAX_STYLES;i++) {
		if(!style[i]) break;
		if(!strcmp(st,style[i])) return i+1;
	}
	return 0;
}

static void free_styles(char **style)
{
	for(int i=0;i<MAX_STYLES;i++) {
		if(!style[i]) break;
		free(style[i]);
	}
}

static int outLegendPath(LPCSTR el,LPCSTR *attr)
{
	int i;

	nCol=skip_depth();
	nCol+=gz_printf("<%s",el);
	for(;attr[0];attr+=2) {
		nCol+=gz_printf(" %s=\"",attr[0]);
		if(!strcmp(attr[0],"d") || !strcmp(attr[0],"points")) {
			out_wraptext(attr[1]);
			OUTC('"');
		}
		else if(!strcmp(attr[0],"style") && (i=find_style(legendstyle,attr[1]))) {
			nCol+=gz_printf("&_l%d;\"",i);
		}
		else nCol+=gz_printf("%s\"",attr[1]);
	}
	OUTS("/>\r\n");
	return 0;
}

static void outTspanElement(LPCSTR *attr)
{
	int i;
	//Later, place style="enable-backgound:new;" on text element via entity --
	gz_printf("<tspan");
	for(;attr[0];attr+=2) {
		gz_printf(" %s=\"",attr[0]);
		if(!strcmp(attr[0],"style")) {
			if(iGroupFlg&FLG_LEGEND) {
				if((i=find_style(legendstyle,attr[1]))) {
					gz_printf("&_l%d;\"",i);
					continue;
				}
			}
			else {
				if((i=find_style(textstyle,attr[1]))) {
					gz_printf("&_t%d;\"",i);
					continue;
				}
			}
		}
		gz_printf("%s\"",attr[1]);
	}
	OUTC('>'); //Now ready to output characters
}

static void outGroup(int id)
{
	skip_depth();
	gz_printf("<g id=\"w2d_%s\"/>\r\n",w2d_name[id]);
}

static void out_entity_line(LPCSTR name,char *value)
{
	if(nCol) OUTS("\r\n");
	skip_depth();
	gz_printf("<!ENTITY %s \"%s\">\r\n",name,value);
	nCol=0;
}

static int outWallsSubgroup(int svg_typ)
{
	int e;
	skip_depth();
	gz_printf("<g id=\"%s\"%s>\r\n",grp_id[svg_typ],
		(svg_typ==W2D_LRUDS)?" style=\"&D_s;\"":"");
	Depth++;
	e=grp_fcn[svg_typ]();
	Depth--;
	skip_depth();
	OUTS("</g>\r\n");
	return e;
}

static int outWallsGroup(int svg_typ,int svg_flg)
{
  int e=0;
  BOOL bNone,bHide;

  if(svg_typ==W2D_FRAME) {
	  bNone=fFrameThick<=0.0; 
	  bHide=FALSE;
  }
  else {
	  bNone=(svg_flg&flags)==0 ||
		(svg_typ==W2D_FLAGS && !numFlagLocations) ||
		(svg_typ==W2D_NOTES && !numNoteLocations);

	  bHide=bNone || ((flags>>16)&svg_flg)!=0;
  }
  
  if(bNone && !(flags&SVG_ADJUSTABLE)) return 0;

  Depth=1;
  gz_printf("\t<g id=\"%s\"%s",grp_id[svg_typ],bHide?" style=\"display:none\"":"");
  if(bNone) OUTS("/>\r\n");
  else {
	  OUTS(">\r\n");
	  Depth++;
	  if(svg_typ==W2D_SURVEY) {
		  e=outWallsSubgroup(W2D_VECTORS);
		  if(!e && (pSVG->advParams.uFlags&SVG_ADV_LRUDBARS)) e=outWallsSubgroup(W2D_LRUDS);
		  if(!e) e=outWallsSubgroup(W2D_MARKERS);
	  }
	  else e=grp_fcn[svg_typ]();
	  OUTS("\t</g>\r\n");
	  Depth--;
  }
  return e;
}

static char *colorstr(char *fbuf,COLORREF clr)
{
	if(clr==(COLORREF)-1) strcpy(fbuf,"none   ");
	else sprintf(fbuf,"#%06X",trx_RevDW(clr)>>8);
	return fbuf;
}

static COLORREF colorval(LPCSTR cbuf)
{
	if(cbuf) {
		DWORD dw;
		sscanf(cbuf,"%lX",&dw);
		return (COLORREF)trx_RevDW(dw<<8);
	}
	return (COLORREF)-1;
}

static void store_style_attr(char *pos,char *str)
{
	char *p=strchr(pos,';');
	if(!p) {
		p=pos+strlen(pos);
		*p=';'; p[1]=0;
	}
	int len=strlen(str);
	if(p-pos!=len) memmove(pos+len,p,strlen(p)+1);
	memcpy(pos,str,len);
}

static LPCSTR getStyleValPtr(LPCSTR p,LPCSTR typ)
{
	if((p=strstr(p,typ))) p+=strlen(typ);
	return p;
}

static LPCSTR getColorPtr(LPCSTR p,LPCSTR typ)
{
	if((p=getStyleValPtr(p,typ)) && *p=='#' && p[7]==';') return ++p;
	return NULL;
}

static COLORREF getColorAttr(LPCSTR p,LPCSTR typ)
{
	return (p=getColorPtr(p,typ))?colorval(p):(COLORREF)-1;
}

static void store_color(LPCSTR typ,COLORREF clr)
{
	char clrbuf[10];
	char *p=(char *)getColorPtr(MrgBuff,typ);
	if(p) memcpy(p-1,colorstr(clrbuf,clr),7);
}

static void out_color_entity(LPCSTR name,LPCSTR typ, COLORREF clr)
{
	store_color(typ,clr);
	out_entity_line(name,MrgBuff);
}

static void out_grid_entity(LPCSTR entityName)
{
	strcat(MrgBuff,"opacity:0.5;");
	out_color_entity(entityName,st_stroke,cGridClr);
}

static void out_veccolor_entity(char *pColor,int i)
{
	char name[8];
	strcpy(name,"V_s");
	if(i) sprintf(name+2,"%d",i);
	if(pColor) memcpy(pColor,vecColor[i],6);
	out_entity_line(name,MrgBuff);
}

static void out_frame_entity()
{
	if(fFrameThick<=0.0) return;
	char *p=(char *)getStyleValPtr(MrgBuff,st_stroke_width);
	if(p) store_style_attr(p,fltstr(fFrameThick));
	out_entity_line("F_s",MrgBuff);
}

static void out_font_entity(LPCSTR name,COLORREF clr,double size)
{
	char *p=strstr(MrgBuff,st_font_size);
	if(p) store_style_attr((char *)p+10,fltstr(size));
	if(clr!=-1) store_color(st_fill,clr);
	out_entity_line(name,MrgBuff);
}

static void outFlagScaleEntities()
{
	int iSym;
	double d;
	for(UINT s=0;s<numFlagStyles;s++)
		if(pFS[s].wFlag>0) {
			if(!bUseStyles || !(iSym=find_style(symname,pFS[s].pFlagname)) || !(d=flagscale[iSym])) {
				d=pFS[s].wSize/2.0;
			}
			out_entity_line(pFS[s].pFlagname,fltstr(d*scaleRatio));
		}
}

static void ExtractStyleArg(char *dst,char *src,const char *typ,BOOL bForced)
{
	char *p=strstr(src,typ);
	if(p) {
		char *srcbrk=p;
		int len=strlen(typ);
		char *srcnxt=strchr(p+=len,';');
		if(!srcnxt) srcnxt=p+strlen(p);
		else srcnxt++;
		strcpy(dst,typ);
		dst+=len;
		while(TRUE) {
			strcpy(dst,fltstr(atof(p)*scaleRatio));
			dst+=strlen(dst);
			if(!(p=strchr(p,' ')) || p>=srcnxt) break;
			p++;
			if(!isdigit((BYTE)*p)) break;
			*dst++=' ';
		}
		*dst++=';'; *dst=0;
		strcpy(srcbrk,srcnxt);
	}
	else if(bForced && strstr(src,"stroke:#")) {
		//If there is a stroke, then replace assumed stroke-width:1 with scaled value --
		sprintf(dst,"%s%s;",typ,fltstr(scaleRatio));
	}
	else *dst=0;
}

static void outStyles(char **style,int pfx,BOOL bScale)
{
	char *st;
	char buf[16];
	char stroke_width[32];
	char font_size[32];
	char stroke_dasharray[128];
	char stylebuf[256];

	for(int s=0;s<MAX_STYLES;s++) {
		if(!(st=style[s])) break;
		if(pfx=='p' && !pathstyle_used[s]) continue;
		if(bScale) {
			*stroke_dasharray=*font_size=*stroke_width=0;
			st=strcpy(stylebuf,st);
			if(pfx=='t') ExtractStyleArg(font_size,st,st_font_size,FALSE);
			else {
				ExtractStyleArg(stroke_width,st,st_stroke_width,TRUE);
				ExtractStyleArg(stroke_dasharray,st,st_stroke_dasharray,FALSE);
			}
			if(*font_size) strcat(st,font_size);
			if(*stroke_width) strcat(st,stroke_width);
			if(*stroke_dasharray) strcat(st,stroke_dasharray);
		}
		sprintf(buf,"_%c%d",pfx,s+1);
		out_entity_line(buf,st);
	}
}

static int outFlagSymbols()
{
	char fill[10];
	char stroke[10];
	char opacity[4];

	for(UINT s=0;s<numFlagStyles;s++) 
		if(pFS[s].wFlag>0 && pFS[s].bShape<4 && !find_style(symname,pFS[s].pFlagname)) {
			SVG_TYP_FLAGSTYLE *pfs=&pFS[s];
			strcpy(opacity,"1");
			colorstr(stroke,pfs->color);
			if(pfs->bShape==3) {
				//plus sign
				*fill=0;
			}
			else if(pfs->bShade==0) {
				//clear
				strcpy(fill,"none");
			}
			else if(pfs->bShade==1) {
				//solid
				colorstr(fill,pfs->color);
				strcpy(stroke,"black");
			}
			else if(pfs->bShade==2) {
				//opaque
				colorstr(fill,cPassageClr);
			}
			else {
				//transparent
				colorstr(fill,pfs->color);
				strcpy(stroke,"black");
				strcpy(opacity,"0.5");
			}
			gz_printf(pSymbolStr[pfs->bShape],pfs->pFlagname,fill,stroke,opacity);
			nCol=0;
		}
	return 0;
}

static int get_idstr(char *idstr,char *prefix,char *label,BOOL bForcePrefix)
{
	char *p0=idstr;
	char *p=prefix;
	int iLabel=0;
	*p0++='_';
	while(TRUE) {
	  if(!*p) {
		  if(iLabel) break;
		  iLabel++;
		  p=label;
		  if(p0-idstr>1) *p0++='.';
		  continue;
	  }
	  if(*p>='A' && *p<='Z' || *p>='a' && *p<='z' || *p>='0' && *p<='9') *p0++=*p;
	  else p0+=sprintf(p0,"-%02x",(UINT)*(unsigned char *)p);
	  p++;
	}
	*p0=0;

	iLabel=p0-idstr;
	if(!bForcePrefix && !isdigit((BYTE)idstr[1]) && idstr[1]!='-') {
		memmove(idstr,idstr+1,iLabel--);
	}
	return iLabel;
}

static char *get_vecID(char *idstr,SHP_TYP_STATION *pFr,SHP_TYP_STATION *pTo)
{
	int i=get_idstr(idstr,pFr->prefix,pFr->label,FALSE);
	get_idstr(idstr+i,pTo->prefix,pTo->label,TRUE);
	return idstr;
}

static void outSymInstance(SVG_VIEWBOX *pvb,LPCSTR pName,double x,double y,BOOL bScaled)
{
	skip_depth();
	gz_printf("<use xlink:href=\"#%s\"",pName);

	gz_printf(" width=\"%s\"",fltstr(pvb->width));
	gz_printf(" height=\"%s\"",fltstr(pvb->height));
	gz_printf(" x=\"%s\"",fltstr(-pvb->width/2.0));
	gz_printf(" y=\"%s\" transform=\"matrix(",fltstr(-pvb->height/2.0));

	if(bScaled) gz_printf("&%s; 0 0 -&%s; ",pName,pName);
	else OUTS("1 0 0 -1 ");
	OUTS(fltstr(x));
	OUTC(' ');
	OUTS(fltstr(y));
	OUTS(")\"/>");
	OUTS("\r\n");
}

static int outFlags()
{
	if(!(flags&SVG_FLAGS)) return 0;

	UINT iSym,n=numFlagStyles;
	SVG_TYP_FLAGLOCATION *pfl;
	SVG_TYP_FLAGSTYLE *pfs;
	SVG_VIEWBOX *pvb,vb={2.0f,2.0f};

	while(n-->0) {
		for(UINT l=0;l<numFlagLocations;l++) {
			pfl=&pFL[l];
			pfs=&pFS[pfl->flag_id];
			if(pfs->wPriority!=n) continue;

			if(!fpmrg || !bUseStyles || !(iSym=find_style(symname,pfs->pFlagname))) pvb=&vb;
			else pvb=&flagViewBox[iSym];

			outSymInstance(pvb,pfs->pFlagname,pfl->xy[0],pfl->xy[1],TRUE); //bScaled=TRUE
		}
	}
	return 0;
}

static BOOL getLabelRec(LPCSTR pID)
{
	char key[32];

	trx_Stncp(key,pID,32);
	if(key[*key]=='_') key[0]--;
	trx.UseTree(SVG_TRX_LABELS);
	BOOL bRet=!trx.Find(key) && !trx.GetRec(&labelRec,sizeof(SVG_TYP_TRXLABEL));
	trx.UseTree(SVG_TRX_VECTORS);
	return bRet;
}

static int outNotes()
{
	if(!pGetData(SHP_NOTELOCATION,NULL)) return 0;

	SHP_TYP_NOTELOCATION sN;
	double xy[2];
	double n_x=fNoteSize*0.15;
	double n_y=fNoteSize*-0.15;

	while(pGetData(SHP_NOTELOCATION,&sN)) {
		if(!isInside(get_xy(xy,sN.xyz))) continue;

		BOOL bOverride=FALSE;

		if(bUseLabelPos || (flags&SVG_ADJUSTABLE)) {
			get_idstr(argv_buf,sN.prefix,sN.label,FALSE);
			if(bUseLabelPos && getLabelRec(argv_buf)) {
				//labelRec has the label's offset in final page units.
				//We'll override the offset if this station has note in merge file --
				if(labelRec.bPresent&2)	bOverride=TRUE;
			}
		}

		if(bOverride) {
			xy[0]+=labelRec.nx; xy[1]+=labelRec.ny;
		}
		else {
			xy[0]+=n_x; xy[1]+=n_y;
		}

		skip_depth();
		if(flags&SVG_ADJUSTABLE) gz_printf("<g id=\"%s_\">",argv_buf);
		OUTS("<text style=\"&N_s;\" x=\"");
		OUTS(fltstr(xy[0]));
		OUTS("\" y=\"");
		OUTS(fltstr(xy[1]));
		OUTS("\">");
		out_xmlstr(sN.note);
		OUTS("</text>");
		if(flags&SVG_ADJUSTABLE) OUTS("</g>");
		OUTS("\r\n");
	}
	return 0;
}

static int outBackground()
{
	skip_depth();
	OUTS("<path style=\"&B_s;\" d=\"M0,0H");
	OUTS(fltstr(width));
	OUTC('V');
	OUTS(fltstr(height));
	OUTS("H0z\"/>\r\n");
	return 0;
}

//Grid fcns ----------------------------------
enum e_outcode {O_LEFT=1,O_RIGHT=2,O_BOTTOM=4,O_TOP=8};

UINT OutCode(double x,double y)
{
	UINT code=0;
	if(x<grid_xMin) code=O_LEFT;
	else if(x>grid_xMax) code=O_RIGHT;
	if(y<grid_yMin) code |= O_TOP;
	else if(y>grid_yMax) code |= O_BOTTOM;
	return code;
}

static apfcn_b ClipEndpoint(double x0,double y0,double *px1,double *py1,UINT code)
{
  //If necessary, adjusts *px1,*py1 to point on frame without changing slope.
  //Returns TRUE if successful (line intersects frame), otherwise FALSE.
  //Assumes code==OutCode(*px1,*py1) and that OutCode(x0,y0)&code==0;

  double slope;

  if(code&(O_RIGHT|O_LEFT)) {
	slope=(*py1-y0)/(*px1-x0);
    *px1=(code&O_RIGHT)?grid_xMax:grid_xMin;
    *py1=y0+slope*(*px1-x0);
	if(*py1<grid_yMin) code=O_TOP;
	else if(*py1>grid_yMax) code=O_BOTTOM;
	else code=0;
  }

  if(code&(O_TOP|O_BOTTOM)) {
	slope=(*px1-x0)/(*py1-y0);
    *py1=(code&O_BOTTOM)?grid_yMax:grid_yMin;
    *px1=x0+slope*(*py1-y0);
	if(*px1<grid_xMin) code=O_LEFT;
	else if(*px1>grid_xMax) code=O_RIGHT;
	else code=0;
  }

  return code==0;
}

static apfcn_b ClipEndpoints(double &x0, double &y0, double &x1, double &y1)
{
	UINT out0=OutCode(x0,y0);
	UINT out1=OutCode(x1,y1);
	if((out0&out1) ||
	   (out1 && !ClipEndpoint(x0,y0,&x1,&y1,out1)) ||
	   (out0 && !ClipEndpoint(x1,y1,&x0,&y0,out0))) return FALSE;
	return TRUE;
}

static apfcn_v GetRange(double &minE,double &maxE,double &minN,double &maxN,double x,double y)
{
	double Tx=cosA*(x-xMid)-sinA*(y-yMid);
	double Ty=-cosA*(y-yMid)-sinA*(x-xMid);
	double d;
	
	d=Tx/scale+midE;
	if(d<minE) minE=d;
	if(d>maxE) maxE=d;
	
	d=Ty/scale+midN;
	if(d<minN) minN=d;
	if(d>maxN) maxN=d;
}

static apfcn_d GridX(double E,double N)
{
	return scale*((E-midE)*cosA-(N-midN)*sinA)+xMid;
}

static apfcn_d GridY(double E,double N)
{
	return -scale*((E-midE)*sinA+(N-midN)*cosA)+yMid;
}

static void outPt(double x,double y)
{
	OUTS(fltstr(x));
	OUTC(' ');
	OUTS(fltstr(y));
}

static void outGridLn(double *xy0,double *xy1,char c)
{
	char x0s[40],y0s[40],x1s[40],y1s[40];

	if(!ClipEndpoints(xy0[0],xy0[1],xy1[0],xy1[1])) return;

	skip_depth();
	gz_printf("<path style=\"&G_%c;\" d=\"M",c);
	fltstr(x0s,xy0[0]);
	fltstr(y0s,xy0[1]);
	fltstr(x1s,xy1[0]-xy0[0]);
	fltstr(y1s,xy1[1]-xy0[1]);
	OUTS(x0s);
	OUTC(' ');
	OUTS(y0s);
	if(!strcmp(x1s,"0")) {
		OUTC('v');
		OUTS(y1s);
	}
	else if(!strcmp(y1s,"0")) {
		OUTC('h');
		OUTS(x1s);
	}
	else {
		OUTC('l');
		OUTS(x1s);
		OUTC(' ');
		OUTS(y1s);
	}
	OUTS("\"/>\r\n");
}

static int outGrid()
{
	if(!(flags&SVG_GRID)) return 0;

	UINT numSubs=pSVG->gridsub;
	double fIntval=pSVG->gridint/numSubs;

	if(fIntval*scale<width/300) return 0;

	double minE=DBL_MAX,maxE=-DBL_MAX,minN=DBL_MAX,maxN=-DBL_MAX;
	double f,xy0[2],xy1[2],xyz0[2],xyz1[2];

	grid_xMin=grid_yMin=fFrameThick;
	grid_xMax=width-fFrameThick;
	grid_yMax=height-fFrameThick;

	GetRange(minE,maxE,minN,maxN,grid_xMin,grid_yMin);
	GetRange(minE,maxE,minN,maxN,grid_xMin,grid_yMax);
	GetRange(minE,maxE,minN,maxN,grid_xMax,grid_yMin);
	GetRange(minE,maxE,minN,maxN,grid_xMax,grid_yMax);
	
	UINT iSub=0;
	
	//Draw North-South gridlines --
	f=minE-fmod(minE,pSVG->gridint);
	if(f>minE) f-=pSVG->gridint;
	while(f<maxE) {
		xyz0[0]=f; xyz0[1]=maxN;
		xyz1[0]=f; xyz1[1]=minN;
		get_xy(xy0,xyz0);
		get_xy(xy1,xyz1);
		outGridLn(xy0,xy1,'1');
		if(numSubs>1 && !(iSub%numSubs)) outGridLn(xy0,xy1,'0'); //Major grid line
		f+=fIntval;
		iSub++;
	}

	//Draw West-East gridlines --
	iSub=0;
	f=minN-fmod(minN,pSVG->gridint);
	if(f>minN) f-=pSVG->gridint;
	while(f<maxN) {
		xyz0[0]=minE; xyz0[1]=f;
		xyz1[0]=maxE; xyz1[1]=f;
		get_xy(xy0,xyz0);
		get_xy(xy1,xyz1);
		outGridLn(xy0,xy1,'1');
		if(numSubs>1 && !(iSub%numSubs)) outGridLn(xy0,xy1,'0');
		f+=fIntval;
		iSub++;
	}
	return 0;
}
//--------------------------------------------
static void outSymInstances()
{
	int symCnt;
	double h,lasth,margin=fFrameThick+0.5*fTitleSize;

	for(symCnt=0;symCnt<MAX_STYLES;symCnt++) {
		if(!symname[symCnt]) break;
	}
	if(!symCnt) return;

	skip_depth();
	gz_printf("<g id=\"w2d_Symbols\" style=\"display:none\">\r\n");
	Depth++;
	lasth=margin;
	for(int i=1;i<=symCnt;i++) {
		h=flagViewBox[i].height;
		outSymInstance(&flagViewBox[i],symname[i-1],
			margin+0.5*flagViewBox[i].width,lasth+0.5*h,FALSE);
		lasth+=h;
	}
	Depth--;
	skip_depth();
	OUTS("</g>\r\n");
}

static int outRef()
{
	skip_depth();
	OUTS("<text style=\"&T_s;\" x=\"");
	OUTS(fltstr(fFrameThick+0.25*fTitleSize));
	OUTS("\" y=\"");
	OUTS(fltstr(height-fFrameThick-0.25*fTitleSize));
	OUTS("\">");
	out_xmlstr(pSVG->title);
	OUTS(" Plan: ");
	OUTS(fltstr(pSVG->view,6));
	OUTS(" Scale: 1:");
	OUTS(fltstr((72*12)/(scale*0.3048),6));

	double f=(flags&SVG_FEET)?(1.0/0.3048):1.0;

	OUTS(" Frame: ");
	OUTS(fltstr(f*width/scale,3));
	OUTC('x');
	OUTS(fltstr(f*height/scale,3));
	OUTS((flags&SVG_FEET)?"f":"m");
	OUTS(" Center: ");
	OUTS(fltstr(f*midE,3));
	OUTC(' ');
	OUTS(fltstr(f*midN,3));
	OUTS("</text>\r\n");
	return 0;
}

static char *get_idkey(char *buf,SHP_TYP_STATION *pFr,SHP_TYP_STATION *pTo)
{
	char *p=buf+1;

	p+=strlen(strcpy(p,pFr->prefix)); *p++=0;
	p+=strlen(strcpy(p,pFr->label)); *p++=0;
	p+=strlen(strcpy(p,pTo->prefix)); *p++=0;
	p+=strlen(strcpy(p,pTo->label));
	*buf=(char)(p-buf-1);
	return buf;
}

static int add_vecColor(LPCSTR p)
{
	int i=numVecColors;
	while(i--) if(!memcmp(vecColor[i],p,6)) return i;
	if(numVecColors<MAX_VECCOLORS) {
		memcpy(vecColor[numVecColors],p,6);
		return numVecColors++;
	}
	return 0;
}

static int find_vecColor(LPCSTR p)
{
	int i=numVecColors;
	while(i--) if(!memcmp(vecColor[i],p,6)) return i;
	return 0;
}

static void get_station_ptrs(SHP_TYP_VECTOR *psV,SHP_TYP_STATION **pF,SHP_TYP_STATION **pT)
{
	SHP_TYP_STATION *pFr,*pTo;
	int i;

	pFr=&psV->station[0];
	pTo=&psV->station[1];
	if((i=strcmp(pFr->prefix,pTo->prefix))==0) i=strcmp(pFr->label,pTo->label);
	if(i>0) {
		*pF=pTo;
		*pT=pFr;
	}
	else {
		*pF=pFr;
		*pT=pTo;
	}
}

static int createIndex(void)
{
	int i;
	SHP_TYP_VECTOR sV;
	SHP_TYP_STATION *pFr,*pTo;
	SVG_TYP_TRXVECTOR rec;
	char idstr[64];

	if(!pGetData(SHP_VECTORS,NULL)) return SVG_ERR_NOVECTORS;
	siz_vecrec=bNTW?sizeof(SVG_TYP_TRXVECTOR_NTW):sizeof(SVG_TYP_TRXVECTOR);

	strcpy(argv_buf,pSVG->svgpath);
	*trx_Stpext(argv_buf)=0;
	if(trx.Create(argv_buf,siz_vecrec,
		0,bUseLabelPos+1,0,1024) || trx.AllocCache(32))
		return SVG_ERR_INDEXINIT;
	trx.SetUnique(TRUE); trx.SetExact(TRUE);

	if(bUseLabelPos) {
		trx.UseTree(SVG_TRX_LABELS);
		if(trx.InitTree(sizeof(SVG_TYP_TRXLABEL),TRX_MAX_KEYLEN,
			CTRXFile::KeyUnique|CTRXFile::KeyExact)) return SVG_ERR_INDEXINIT;
		trx.UseTree(SVG_TRX_VECTORS);
	}

	//Index current version of survey vectors provided by Walls,
	//eliminating any duplicates.
	//Index will not depend on frame dimensions --

	sV.titles=sV.attributes=NULL; //Used only in WALLSHPX.DLL

	numVecColors=0;

	while(pGetData(SHP_VECTORS,&sV)) {
		get_station_ptrs(&sV,&pFr,&pTo);
		if(flags&SVG_MERGEDCONTENT) {
			memcpy(rec.fr_xy,pFr->xyz,sizeof(rec.fr_xy));
			memcpy(rec.to_xy,pTo->xyz,sizeof(rec.to_xy));
		}
		if(!bNTW) rec.wVecColor=add_vecColor(sV.linetype); //e>=0
		if(i=trx.InsertKey(&rec,get_idkey(idstr,pFr,pTo))) {
			if(i!=CTRXFile::ErrDup)	return SVG_ERR_INDEXWRITE;
		}
	}

	return trx.NumRecs()?0:SVG_ERR_NOVECTORS;
}

static int outVectors()
{
	SHP_TYP_VECTOR sV;
	SHP_TYP_STATION *pFr,*pTo;
	SVG_TYP_TRXVECTOR rec;
	double xyFr[2],xyTo[2];
	char idstr[64];
	int iVecColor;

    if(!(flags&SVG_VECTORS)) return 0;

	sV.titles=sV.attributes=NULL; //Used only in WALLSHPX.DLL
	if(!pGetData(SHP_VECTORS,NULL)) return 0;

	while(pGetData(SHP_VECTORS,&sV)) {
		get_station_ptrs(&sV,&pFr,&pTo);

		if(iVecColor=trx.Find(get_idkey(idstr,pFr,pTo))) {
			return SVG_ERR_UNKNOWN; //index failure shouldn't happen!
		}
		if(trx.GetRec(&rec,sizeof(SVG_TYP_TRXVECTOR))) {
			return SVG_ERR_UNKNOWN; //index failure shouldn't happen!
		}
		if(rec.wVecColor==WORD(-1)) continue;

		iVecColor=rec.wVecColor; //Color of merged version if present
		rec.wVecColor=(WORD)-1;
		trx.PutRec(&rec);

		get_xy(xyFr,pFr->xyz);
		get_xy(xyTo,pTo->xyz);
		if(!isInside(xyFr) && !isInside(xyTo)) continue;

		skip_depth();
		OUTS("<path ");
		if(flags&SVG_ADJUSTABLE) {
			OUTS("id=\"");
			OUTS(get_vecID(idstr,pFr,pTo));
			OUTS("\" ");
		}

		OUTS("style=\"");

		//Check for color override --
		if(!bUseStyles) iVecColor=find_vecColor(sV.linetype);
		if(iVecColor && numVecColors>1) gz_printf("&V_%d;",iVecColor);
		else OUTS("&V_s;");

		OUTS("\" d=\"M");
		OUTS(fltstr(xyFr[0]));
		fltstr(idstr,xyFr[1]);
		if(*idstr!='-') OUTC(',');
		OUTS(idstr);
		OUTC('l');
		OUTS(fltstr(xyTo[0]-xyFr[0]));
		fltstr(idstr,xyTo[1]-xyFr[1]);
		if(*idstr!='-') OUTC(',');
		OUTS(idstr);
		OUTS("\"/>\r\n");
	}
	return 0;
}

static int outMarkers()
{
	SHP_TYP_STATION sS;
	double xy[2];

	if(!(flags&SVG_VECTORS) || !pGetData(SHP_STATIONS,NULL)) return 0;
	while(pGetData(SHP_STATIONS,&sS)) {
		if(!isInside(get_xy(xy,sS.xyz))) continue;
		skip_depth();
		OUTS("<use xlink:href=\"#_m\" width=\"2\" height=\"2\" x=\"-1\" y=\"-1\" transform=\"matrix(&_M; 0 0 -&_M; ");
		OUTS(fltstr(xy[0]));
		OUTC(' ');
		OUTS(fltstr(xy[1]));
		OUTS(")\"/>\r\n");
	}
	return 0;
}

static int outLruds()
{
	SHP_TYP_LRUD sL;
	double xyFr[2],xyTo[2];
	char fbuf[40];

	if(!pGetData(SHP_LRUDS,NULL)) return 0;

	while(pGetData(SHP_LRUDS,&sL)) {

		get_xy(xyFr,sL.xyFr);
		get_xy(xyTo,sL.xyTo);

		if(!isInside(xyFr) && !isInside(xyTo)) continue;

		skip_depth();
		OUTS("<path d=\"M"); //All lruds have same style

		OUTS(fltstr(xyFr[0]));
		fltstr(fbuf,xyFr[1]);
		if(*fbuf!='-') OUTC(',');
		OUTS(fbuf);
		OUTC('l');
		OUTS(fltstr(xyTo[0]-xyFr[0]));
		fltstr(fbuf,xyTo[1]-xyFr[1]);
		if(*fbuf!='-') OUTC(',');
		OUTS(fbuf);
		OUTS("\"/>\r\n");
	}
	return 0;
}

static int outLabels()
{
	if(!pGetData(SHP_STATIONS,NULL)) return 0;

	SHP_TYP_STATION sS;
	double xy[2];
	double l_x=scaleRatio*(pSVG->offLabelX-1);
	double l_y=scaleRatio*(pSVG->offLabelY-1)+fLabelSize;

	while(pGetData(SHP_STATIONS,&sS)) {
		if(!isInside(get_xy(xy,sS.xyz))) continue;

		BOOL bOverride=FALSE;

		if(bUseLabelPos || (flags&SVG_ADJUSTABLE)) {
			get_idstr(argv_buf,sS.prefix,sS.label,FALSE);
			if(bUseLabelPos && getLabelRec(argv_buf)) {
				//labelRec has the label's offset in final page units.
				//We'll override the offsets or omit label if not in merge file --
				if((labelRec.bPresent&1)==0) continue;
				bOverride=TRUE;
			}
		}

		if(bOverride) {
			xy[0]+=labelRec.sx; xy[1]+=labelRec.sy;
		}
		else {
			xy[0]+=l_x; xy[1]+=l_y;
		}

		skip_depth();
		if(flags&SVG_ADJUSTABLE) gz_printf("<g id=\"%s\">",argv_buf);
		OUTS("<text style=\"&L_s;\" x=\"");
		OUTS(fltstr(xy[0]));
		OUTS("\" y=\"");
		OUTS(fltstr(xy[1]));
		OUTS("\">");
		if((flags&SVG_SHOWPREFIX) && sS.prefix[0]) {
			out_xmlstr(sS.prefix);
			OUTC(':');
		}
		out_xmlstr(sS.label);
		OUTS("</text>");
		if(flags&SVG_ADJUSTABLE) OUTS("</g>");
		OUTS("\r\n");
	}
	return 0;
}

static void GetLineStyle(double t)
{
	if(t<0.01) t=0.01;
	sprintf(MrgBuff,"stroke:#000000;fill:none;stroke-width:%.2lf;",t);
}

//==================================================================
//Handlers for expat --
static void out_entity (void *userData,
                          const XML_Char *entityName,
                          int is_parameter_entity,
                          const XML_Char *value,
                          int value_length,
                          const XML_Char *base,
                          const XML_Char *systemId,
                          const XML_Char *publicId,
                          const XML_Char *notationName)
{
	if(!value) return;
	memcpy(MrgBuff,value,value_length);
	MrgBuff[value_length]=0;

	if(!strcmp(entityName,"_M")) {
		if(flags&SVG_VECTORS) {
			out_entity_line(entityName,fltstr(fMarkerThick));
		}
		outFlagScaleEntities();
		if(fpmrg) {
			outStyles(pathstyle,'p',TRUE);
			outStyles(textstyle,'t',TRUE);
			outStyles(legendstyle,'l',FALSE);
		}
		return;
	}

	if(!strcmp(entityName,"G_0")) {
		//Major grid lines --
		if((flags&SVG_GRID) && pSVG->gridsub>1) {
			//Obtain grid line thicknesses based on vector thicknesses --
			GetLineStyle((numGridThick==2)?(fGridThick[1]*scaleRatio):(3*fVectorThick));
			out_grid_entity(entityName);
		}
		return;
	}

	if(!strcmp(entityName,"G_1")) {
		//Minor grid lines --
		if(flags&SVG_GRID) {
			//Obtain grid line thicknesses based on vector thicknesses --
			GetLineStyle((numGridThick>0)?(fGridThick[0]*scaleRatio):fVectorThick);
			out_grid_entity(entityName);
		}
		return;
	}

	if(!strcmp(entityName,"F_s")) {
		out_frame_entity();
		return;
	}
	if(!strcmp(entityName,"B_s")) {
		if(((cBackClr&0xFFFFFF)==0xFFFFFF) &&
			(!bMaskPathSeen || ((cPassageClr&0xFFFFFF)==0xFFFFFF))) {
			strcat(MrgBuff,"opacity:0;");
			out_entity_line(entityName,MrgBuff);
		}
		else out_color_entity(entityName,st_fill,cBackClr);
		return;
	}

	if(bMaskPathSeen && !strcmp(entityName,"P_s")) {
		//Only to be applied to first path of each mask layer group. If both
		//the background and passage color is white, make the opacity zero --
		if(((cBackClr&0xFFFFFF)==0xFFFFFF) && ((cPassageClr&0xFFFFFF)==0xFFFFFF)) {
			strcat(MrgBuff,"fill-opacity:0;");
			out_entity_line(entityName,MrgBuff);
		}
		else out_color_entity(entityName,st_fill,cPassageClr);
		return;
	}

	if(!strcmp(entityName,"V_s")) {
		if(flags&SVG_VECTORS) {
			//Obtain line thickness based on label font size --
			GetLineStyle(fVectorThick);
			//Output vector line styles --
			for(int i=0;i<numVecColors;i++) out_veccolor_entity((char *)MrgBuff+8,i);
		}
		return;
	}

	if(!strcmp(entityName,"D_s")) {
		if((flags&SVG_VECTORS) && (pSVG->advParams.uFlags&SVG_ADV_LRUDBARS)) {
			//Obtain line thickness based on label font size --
			GetLineStyle(fVectorThick);
			out_color_entity(entityName,st_stroke,cLrudClr);
		}
		return;
	}

	if(!strcmp(entityName,"T_s")) {
		//Black frame title --
		out_font_entity(entityName,0,fTitleSize);
		return;
	}
	if(!strcmp(entityName,"N_s")) {
		if(numNoteLocations) {
			if(pNoteStyle) strcpy(MrgBuff,pNoteStyle);
			out_font_entity(entityName,pNoteStyle?-1:pSVG->noteColor,fNoteSize);
		}
		return;
	}
	if(!strcmp(entityName,"L_s")) {
		if(pLabelStyle) strcpy(MrgBuff,pLabelStyle);
		out_font_entity(entityName,pLabelStyle?-1:pSVG->labelColor,fLabelSize);
		return;
	}
	out_entity_line(entityName,MrgBuff);
}

static void out_startDoctype(void *userData,
                               const XML_Char *doctypeName,
                               const XML_Char *sysid,
                               const XML_Char *pubid,
                               int has_internal_subset)
{
	gz_printf("<!DOCTYPE %s ",doctypeName);
	if(pubid && *pubid) gz_printf("PUBLIC \"%s\" ",pubid);
	else if(sysid) OUTS("SYSTEM ");
	if(sysid && *sysid) gz_printf("\"%s\"",sysid);
	if(has_internal_subset) {
		UD(bEntity)=1;
		OUTS(" [\r\n");
	}
	nCol=0;
	Depth=1;
}

static void out_endDoctype(void *userData)
{
	if(UD(bEntity)) {
		OUTS("]>\r\n\r\n");
		UD(bEntity)=0;
	}
	else OUTS(">\r\n");
	nCol=Depth=0;
}

static void out_instruction(void *userData,const XML_Char *target,const XML_Char *data)
{
	gz_printf("<?walls updated=\"no\" merged-content=\"%s\""
		" adjustable=\"%s\" dll-version=" EXP_SVG_VERSION_STR "?>\r\n",
		(flags&SVG_MERGEDCONTENT)?"yes":"no",
		(flags&SVG_ADJUSTABLE)?"yes":"no");
}

static void out_xmldecl(void *userData,
                       const XML_Char  *version,
                       const XML_Char  *encoding,
                       int             standalone)
{
	OUTS("<?xml");
	if(version) gz_printf(" version=\"%s\"",version);
	if(encoding) gz_printf(" encoding=\"%s\"",encoding);
	gz_printf(" standalone=\"%s\"?>\r\n",standalone?"yes":"no");
}

static void out_chardata(void *userData,const XML_Char *s,int len)
{
	//Note: chardata is anything that occurs between tags. EOL appears as an
	//isolated 0xA, thereby breaking up a succession of data into blocks.
	//An end of buffer condition will also presumably cause a break in the
	//stream. Here we will output all data as given while keeping track of
	//column number.

	if(UD(errcode) || bNoOutput) return;
	if(bNoClose) {
			OUTC('>');
			nCol++;
			bNoClose=0;
	}
	memcpy(MrgBuff,s,len);
	MrgBuff[len]=0;
	out_stream(MrgBuff);
}

static void out_comment(void *userData,const XML_Char *s)
{
	if(bNoOutput) return;
	if(!bFirstCommentSeen) {
		//Ignore first comment, a caution message --
		char date[10],time[10];
		_strdate(date);
		_strtime(time);
		sprintf(MrgBuff,"Generator: Walls 2.0 B7, wallsvg.dll "
			EXP_SVG_VERSION_STR ", %s %s",date,time);
		s=MrgBuff;
		bFirstCommentSeen=TRUE;
	}
	if(!bElementsSeen) {
		if(nCol) OUTS("\r\n");
		nCol=skip_depth();
	}
	OUTS("<!--");
	nCol+=4;
	out_stream((char *)s);
	OUTS("-->");
	if(!bElementsSeen) {
		OUTS("\r\n");
		nCol=0;
	}
	else nCol+=3;
}

static int get_grp_typ(LPCSTR pAttr)
{
  int typ;

  for(typ=1;typ<NUM_GROUPS;typ++) {
	  if(!strcmp(grp_id[typ],pAttr)) break;
  }
  return typ;
}

static void out_startElement(void *userData, LPCSTR el, LPCSTR *attr)
{
  if(UD(errcode)) return;
  bElementsSeen=TRUE;

  int typ;
  BOOL bNone=FALSE,bHide=FALSE;
  LPCSTR pAttr;

  //Now determine typ --
  if(!strcmp(el,"g")) {
	  bNoOutput=TRUE; //No more comments
	  typ=get_grp_typ(pAttr=attr[1]);
  }
  else if(!strcmp(el,"svg")) {
	  typ=W2D_SVG;
	  Depth=0;
  }
  else typ=NUM_GROUPS;

  if(typ==W2D_BACKGROUND) {
	OUTS("\r\n");
	nCol=0;
	if(fpmrg) {
	  bSymbolScan=TRUE;
	  if(outMergedContent()) return; //error condition
	  //else continue parsing XML template --
	  bSymbolScan=FALSE;
	}
	if(!bMarkerWritten) {
		gz_printf(pSymbolStr[FMT_MKR_MARKER],colorstr(argv_buf,cMarkerClr));
	}
	outFlagSymbols();
	OUTS("\r\n");
	nCol=0;
	bNoOutput=TRUE;
  }

  if(!nCol && Depth) nCol=skip_depth();

  grp_typ[Depth++]=typ;

  nCol+=gz_printf("<%s", el);

  //Note: Within attribute values, each CTRL character (LF, TAB, etc.)
  //will appear as a space.

  char viewbox_str[80];
  char *pValue;

  for(int i=0;pAttr=attr[i];i+=2) {
	if(nCol>(typ?SIZ_LINEBREAK:SIZ_SVG_LINEBREAK)) {
		OUTS("\r\n");
		nCol=skip_depth();
	}
	else {
		OUTC(' ');
		nCol++;
	}
	pValue=(char *)attr[i+1];
	if(typ==W2D_SVG) {
		if(!strcmp(pAttr,"width")) sprintf(pValue=argv_buf,"%spt",width_str);
		else if(!strcmp(pAttr,"height")) sprintf(pValue=argv_buf,"%spt",height_str);
		else if(!strcmp(pAttr,"viewBox")) {
			sprintf(viewbox_str,"0 0 %s %s",width_str,height_str);
			pValue=viewbox_str;
		}
		else if(!strcmp(pAttr,"style")) {
			sprintf(viewbox_str,"overflow:visible;enable-background:new 0 0 %s %s",width_str,height_str);
			pValue=viewbox_str;
		}
		else trimctrl(pValue); 
	}
	else trimctrl(pValue);

	sprintf(MrgBuff,"%s=\"%s\"",pAttr,pValue);
	out_wraptext(MrgBuff);
  }

  bNoClose=(typ==NUM_GROUPS || typ<0);

  if(!bNoClose) {
	  OUTC('>');
	  if(typ!=W2D_SVG) {
		OUTS("\r\n");
		nCol=0;
	  }
	  else if(!(flags&SVG_ADJUSTABLE)) {
		OUTS("\r\n\r\n\t<!--NOTE: This inner svg clips output to the frame. Remove it for AI 10 compatibility!-->\r\n");
		OUTS("\t<svg width=\"");
		OUTS(fltstr(width));
		OUTS("\" height=\"");
		OUTS(fltstr(height));
		OUTS("\" overflow=\"hidden\">");
	  }

	  if(!bNone && grp_fcn[typ]) UD(errcode)=grp_fcn[typ]();
  }
}

static void out_endElement(void *userData, LPCSTR el)
{
  if(UD(errcode)) return;
  int typ=grp_typ[--Depth];

  if(bNoClose) {
	  if(XML_GetCurrentByteCount(parser)==0) OUTS("/>");
	  else OUTC('>');
	  bNoClose=FALSE;
  }

  if(typ>=NUM_GROUPS || typ==W2D_IGNORE || typ==W2D_SVG) return;

  if(nCol==0) skip_depth();
  gz_printf("</%s>\r\n",el);
  nCol=0;

  if(typ==W2D_REF) {
	//Ignore rest of template --
	udata.errcode=SVG_ERR_ENDSCAN;
  }
}

//=========================================================================
static void walls_instruction(void *userData,const XML_Char *target,const XML_Char *data)
{
	char *p;

	if(!strcmp(target,"walls")) {
		if(!(strstr(data,"updated=\"no\""))) {
			pSVG->flags|=SVG_UPDATED;
		}
		if(!(strstr(data,"merged-content=\"no\""))) {
			pSVG->flags|=SVG_ADDITIONS;
		}
		if(strstr(data,"adjustable=\"yes\"")) {
			pSVG->flags|=SVG_EXISTADJUSTABLE;
		}
		if(p=strstr(data,"dll-version=\"")) {
			pSVG->version=(UINT)floor(0.5+100*atof(p+13));
		}
	}
	UD(errcode)=SVG_ERR_ENDSCAN;
}

static void first_comment(void *userData,const XML_Char *s)
{
	UD(errcode)=SVG_ERR_ENDSCAN;
}

//=========================================================================

static int finish(int e)
{
	if(trx.Opened()) trx.CloseDel();
	if(!bNTW) {
		free(pLabelStyle);
		free(pNoteStyle);
		free_styles(symname);
		free_styles(pathstyle);
		free_styles(textstyle);
		free_styles(legendstyle);
		if(pathrange) {free(pathrange); pathrange=NULL;}
		if(symrange) {free(symrange); symrange=NULL;}
		if(pFS) {free(pFS); pFS=NULL;}
		if(pFL) {free(pFL); pFL=NULL;}
	}
	else {
		if(pnPly) {free(pnPly); pnPly=NULL;}
		if(pNTWp) {free(pNTWp); pNTWp=NULL;}
		if(pNTWf) {free(pNTWf); pNTWf=NULL;}
		if(pNtwRec) {free(pNtwRec); pNtwRec=NULL;}
		if(pMaskName) {free(pMaskName); pMaskName=NULL;}
	}

	if(vstk) {free(vstk); vstk=NULL;}
	if(vnod) {free(vnod); vnod=NULL;}
	if(bWriting) {
		if(gzout) {
			if(gzclose(gzout) && e<=0) e=SVG_ERR_WRITE;
			gzout=NULL;
		}
		if(fpout) {
			if(fclose(fpout) && e<=0) e=SVG_ERR_WRITE;
			fpout=NULL;
		}
		if(e>0) unlink(pSVG->svgpath);
	}
	if(fpin) {gzclose(fpin); fpin=NULL;};
	if(fpmrg) {gzclose(fpmrg); fpmrg=NULL;};
	return e;
}

DLLExportC char * PASCAL ErrMsg(int code)
{
  if(code<0) code=-code;
  if(code>SVG_ERR_UNKNOWN) {
	  code=SVG_ERR_UNKNOWN;
  }
  if(code>SVG_ERR_ABORTED && code<=SVG_ERR_WRITE) {
	sprintf(argv_buf,"%s file %s",(LPSTR)errstr[code],trx_Stpnam(pSVG->svgpath));
  }
  else if(code==SVG_ERR_MASKPATHSOPEN) {
	  sprintf(argv_buf,errstr[code],nMaskOpenPaths,nNTW_OpenPathLine);
  }
  else {
	  int len=0;
	  if(fpmrg) len=sprintf(argv_buf,"File %s - ",trx_Stpnam(pSVG->mrgpath));
	  if(parser_ErrorLineNumber>0) len+=sprintf(argv_buf+len,"Line %d: ",parser_ErrorLineNumber);
	  if(code==SVG_ERR_XMLPARSE) strcpy(argv_buf+len,XML_ErrorString(parser_ErrorCode));
	  else if(code==SVG_ERR_VECNOTFOUND) {
		  sprintf(argv_buf+len,errstr[code],refbuf);
	  }
	  else strcpy(argv_buf+len,errstr[code]);
  }
  strcat(argv_buf,".");
  return argv_buf;
}

//================================================================================

static int parse_file(XML_Parser p,gzFile fp)
{
	int e=0;
	char *buffer;
	UINT bufsize;

	udata.bEntity=0;
	udata.errcode=0;
	udata.errline=0;
	XML_SetUserData(p,&udata);
	XML_SetParamEntityParsing(p,XML_PARAM_ENTITY_PARSING_NEVER);
	XML_SetPredefinedEntityParsing(p,XML_PREDEFINED_ENTITY_PARSING_NEVER);

	if(fp) {
		buffer=MrgBuff;
		bufsize=MRGBUFFSIZE;
	}
	else buffer=pTemplate;
	parser=p;

	for (;;) {
		int done,len;
		if(fp) {
			len=gzread(fp,buffer,bufsize);
			if(len<0) {
				e=SVG_ERR_XMLREAD;
				break;
			}
			done=len<(int)bufsize;
		}
		else {
			len=uTemplateLen;
			done=TRUE;
		}
		if(!XML_Parse(p,buffer,len,done) && !udata.errcode) {
			udata.errline=XML_GetCurrentLineNumber(p);
			udata.errcode=SVG_ERR_XMLPARSE;
			parser_ErrorCode=XML_GetErrorCode(p);
			break;
		}
		if(done || udata.errcode) break;
	}

	if(udata.errcode) {
		if(udata.errcode==SVG_ERR_ENDSCAN) {
			udata.errcode=0;
		}
		else {
			parser_ErrorLineNumber=udata.errline;
			e=udata.errcode;
		}
	}
	XML_ParserFree(p);
	return e;
}

static double init_veclst()
{
	for(int i=0;i<vecs_used;i++) {
		veclst_d[i]=DBL_MAX;
		veclst_i[i]=0;
	}
	return DBL_MAX;
}


static int outFrame()
{
	double t=fFrameThick/2.0;
	skip_depth();
	OUTS("<path style=\"&F_s;\" d=\"");
	OUTC('M');
	OUTS(fltstr(t));
	OUTC(',');
	OUTS(fltstr(t));
	OUTC('H');
	OUTS(fltstr(width-t));
	OUTC('V');
	OUTS(fltstr(height-t));
	OUTC('H');
	OUTS(fltstr(t));
	OUTC('z');
	OUTS("\"/>\r\n");
	return 0;
}

static int outWallsData()
{
	Depth=1;
	int e=outWallsGroup(W2D_SURVEY,SVG_VECTORS);
	if(!e) e=outWallsGroup(W2D_FLAGS,SVG_FLAGS);
	if(!e) e=outWallsGroup(W2D_LABELS,SVG_LABELS);
	if(!e) e=outWallsGroup(W2D_NOTES,SVG_NOTES);
	if(!e) e=outWallsGroup(W2D_GRID,SVG_GRID);
	return e;
}

static int write_svg(void)
{
    XML_Parser xmlparser;

	int zOff=strlen(pSVG->svgpath)-1;
	if(zOff<0) return SVG_ERR_SVGCREATE;
	if(toupper(pSVG->svgpath[zOff])!='Z') zOff=0;

	pTemplate=NULL;
	HMODULE h=GetModuleHandle("WALLSVG");
	HRSRC hrsrc=FindResource(h,"WALLSVG.XML",RT_HTML);
	if(hrsrc) {
		uTemplateLen=SizeofResource(h,hrsrc);
		pTemplate=(char *)LockResource(LoadResource(h,hrsrc));
	}
	if(!pTemplate) {
		UINT e=GetLastError();
		return SVG_ERR_VERSION;
	}
	if(zOff) {
		if(!(gzout=gzopen(pSVG->svgpath,"wb"))) return SVG_ERR_SVGCREATE;
	}
	else {
		if(!(fpout=fopen(pSVG->svgpath,"wb"))) return SVG_ERR_SVGCREATE;
	}
	bWriting=TRUE;

	if (!(xmlparser=XML_ParserCreate(NULL))) return SVG_ERR_NOMEM;

	XML_SetXmlDeclHandler(xmlparser,out_xmldecl);
	XML_SetElementHandler(xmlparser,out_startElement,out_endElement);
	XML_SetProcessingInstructionHandler(xmlparser,out_instruction);
	XML_SetCharacterDataHandler(xmlparser,out_chardata);
	XML_SetCommentHandler(xmlparser,out_comment);
	XML_SetDoctypeDeclHandler(xmlparser,out_startDoctype,out_endDoctype);
	XML_SetEntityDeclHandler(xmlparser,out_entity);
	XML_SetGeneralEntityParsing(xmlparser,XML_GENERAL_ENTITY_PARSING_NEVER);

	//Format comments in SVG portion differently --
	bElementsSeen=bNoOutput=bSymbolScan=bMarkerWritten=FALSE;
	//Skip first comment, a CAUTION statement, in WALLSVG.XML--
	bFirstCommentSeen=FALSE;
	//When "<!ENTITY _M" is seen, call out_FlagEntities() -- 
	bNoClose=nCol=Depth=0;
	if(fVectorThick<0.0) fVectorThick=fLabelSize*0.03;
	if(fMarkerThick<0.0) fMarkerThick=fLabelSize*0.133;
	if(fpmrg) {
		fMarkerThick*=scaleRatio;
		fVectorThick*=scaleRatio;
		fLabelSize*=scaleRatio;
		fNoteSize*=scaleRatio;
		fTitleSize*=xMid/xMrgMid;
	}
	else {
		mrgScale=scale;
		scaleRatio=1.0;
	}
	if(fpmrg && bAdjust) {
		tppcnt=0;
		node_visits=0;
		init_veclst();
	}
	if(zOff=parse_file(xmlparser,NULL)) return zOff;

	if(flags&SVG_MERGEDCONTENT) {
		if(zOff=outMergedContent()) return zOff;
	}
	else if(flags&SVG_ADJUSTABLE) {
		outGroup(ID_MASK);
		outStartGroup(ID_DETAIL,FALSE);
		outGroup(ID_DETAIL_SHP);
		outGroup(ID_DETAIL_SYM);
		outEndElement("g");
		outStartGroup(ID_WALLS,FALSE);
		outGroup(ID_WALLS_SHP);
		outGroup(ID_WALLS_SYM);
		outEndElement("g");
		if(zOff=outWallsData()) return zOff;
		outGroup(ID_LEGEND);
	}
	outWallsGroup(W2D_FRAME,0);
	if(!(flags&SVG_ADJUSTABLE)) OUTS("\t</svg> <!--NOTE: Remove for AI 10 -->\r\n");
	OUTS("</svg>\r\n");
	return 0;
}

static int scan_svgheader(void)
{
	if(!(fpin=gzopen(pSVG->svgpath,"rb"))) return SVG_ERR_SVGCREATE;
	if (!(mrgparser=XML_ParserCreate(NULL))) return SVG_ERR_NOMEM;

	XML_SetCommentHandler(mrgparser,first_comment);
	XML_SetProcessingInstructionHandler(mrgparser,walls_instruction);

	XML_SetGeneralEntityParsing(mrgparser,XML_GENERAL_ENTITY_PARSING_NEVER);

	parse_file(mrgparser,fpin); //ignore parsing errors
	return 0;
}

static int GetFlagData()
{
	if(numFlagStyles=pGetData(SHP_FLAGSTYLE,NULL)) {
		pFS=(SVG_TYP_FLAGSTYLE *)malloc(numFlagStyles*(sizeof(SVG_TYP_FLAGSTYLE)+SHP_LEN_FLAGNAME+2));
		if(!pFS) return SVG_ERR_NOMEM;
		SHP_TYP_FLAGSTYLE sFS;
		char *pNext=(char *)&pFS[numFlagStyles];
		UINT len,id;
		for(len=0;len<numFlagStyles;len++) pFS[len].wFlag=0;
		for(len=0;len<numFlagStyles;len++) {
			if(!pGetData(SHP_FLAGSTYLE,&sFS)) break;
			id=sFS.id;
			//sanity check --
			if(id>=numFlagStyles || pFS[id].wFlag!=0) {
				return SVG_ERR_UNKNOWN;
			}
			pFS[id].wPriority=sFS.wPriority;
			pFS[id].wSize=sFS.wSize;
			pFS[id].bShape=sFS.bShape;
			pFS[id].bShade=sFS.bShade;
			pFS[id].color=sFS.color;
			pFS[id].pFlagname=strcpy(pNext,flagname_str(sFS.flagname));
			pNext+=strlen(pNext)+1;
		}
		if(!(pFS=(SVG_TYP_FLAGSTYLE *)_expand(pFS,pNext-(char *)pFS))) return SVG_ERR_NOMEM;
	}

	if(numFlagLocations=pGetData(SHP_FLAGLOCATION,NULL)) {
		pFL=(SVG_TYP_FLAGLOCATION *)malloc(numFlagLocations*sizeof(SVG_TYP_FLAGLOCATION));
		if(!pFL) return SVG_ERR_NOMEM;
		SHP_TYP_FLAGLOCATION sFL;
		UINT l=0;
		while(pGetData(SHP_FLAGLOCATION,&sFL)) {
			if(sFL.flag_id>=numFlagStyles || l>=numFlagLocations) {
				return SVG_ERR_UNKNOWN;
			}
			//if(pFS[sFL.flag_id].wHidden) continue;
			//check and convert coordinates --
			if(isInside(get_xy(pFL[l].xy,sFL.xyz))) {
				pFL[l].flag_id=sFL.flag_id;
				pFS[sFL.flag_id].wFlag++;
				l++;
			}
		}
		if(l) {
			if(!(pFL=(SVG_TYP_FLAGLOCATION *)_expand(pFL,l*sizeof(SVG_TYP_FLAGLOCATION))))
				return SVG_ERR_NOMEM;
			numFlagLocations=l;
		}
		else {
			//No flag instances to worry about!
			//We'll include the symbols, however.
			free(pFL); pFL=NULL;
			numFlagLocations=0;
		}
	}

	return 0;
}

static void getNumNoteLocations()
{
	SHP_TYP_NOTELOCATION sN;
	UINT count;
	double xy[2];

	count=0;
	if(pGetData(SHP_NOTELOCATION,NULL)) {
		while(pGetData(SHP_NOTELOCATION,&sN)) {
			if(isInside(get_xy(xy,sN.xyz))) count++;
		}
	}
	numNoteLocations=count;
}


static void init_constants()
{
	decimals=pSVG->advParams.decimals;
	char *p=fltstr(pSVG->width);
	p[sizeof(width_str)-1]=0;
	strcpy(width_str,p);
	width=atof(width_str);
	p=fltstr(pSVG->height);
	p[sizeof(height_str)-1]=0;
	strcpy(height_str,p);
	height=atof(height_str);

	xMid=0.5*width;
	yMid=0.5*height;
	grid_xMin=grid_yMin=0.0;
	grid_xMax=width;
	grid_yMax=height;
	viewA=pSVG->view*U_DEGREES;
	sinA=sin(viewA);
	cosA=cos(viewA);
	midE=pSVG->centerEast;
	midN=pSVG->centerNorth;
	scale=pSVG->scale; //pts per meter
	init_utm_to_xy();

	numNoteLocations=numFlagLocations=numFlagStyles=0;
	pFS=NULL;
	pFL=NULL;
	fLabelSize=pSVG->labelSize/10.0;
	fNoteSize=pSVG->noteSize/10.0;
	fTitleSize=pSVG->titleSize/10.0;
	fVectorThick=fMarkerThick=-1.0; //still to be determined
	numGridThick=0;
	fFrameThick=pSVG->frameThick;
	cPassageClr=pSVG->passageColor;
	cBackClr=pSVG->backColor;
	cGridClr=pSVG->gridColor;
	cLrudClr=pSVG->lrudColor;
	cMarkerClr=pSVG->markerColor;
	if(flags&SVG_MERGEDCONTENT) {
		bUseStyles=(flags&SVG_USESTYLES)!=0;
		bUseLabelPos=(flags&(SVG_LABELS+SVG_NOTES))!=0 && (pSVG->advParams.uFlags&SVG_ADV_USELABELPOS)!=0;
		dist_exp=pSVG->advParams.dist_exp;
		dist_off=pSVG->advParams.dist_off;
		len_exp=pSVG->advParams.len_exp;
		vecs_used=pSVG->advParams.vecs_used;
	}
	else bUseStyles=bUseLabelPos=FALSE;
}

//======================================================================
//Merge file functions --
static BOOL is_hex(int c)
{
	return isdigit((BYTE)c) || (c>='a' && c<='f');
}

static int get_name_from_id_component(char *rnam, LPCSTR pnam)
{
	UINT uhex;
	while(*pnam) {
		if(*pnam=='-') {
			if(!is_hex(pnam[1]) || !is_hex(pnam[2]) || 1!=sscanf(pnam+1,"%02x",&uhex)) {
				return SVG_ERR_BADIDSTR;
			}
			pnam+=3;
			*rnam++=(byte)uhex;
		}
		else *rnam++=*pnam++;
	}
	*rnam=0;
	return 0;
}

static int get_ep_name(LPCSTR pnam,SVG_ENDPOINT *ep)
{
	int e=0;
	char *p=strchr(pnam,'.');
	if(!p) ep->prefix[0]=0;
	else {
	   *p++=0;
	   e=get_name_from_id_component(ep->prefix,pnam);
	   pnam=p;
	}
	if(!e) e=get_name_from_id_component(ep->name,pnam);
	return e;
}

static int get_ep_names(LPCSTR ids,SVG_ENDPOINT *ep0,SVG_ENDPOINT *ep1)
{
	char idbuf[SVG_MAX_SIZNAME+1];
	char *p1,*p0=idbuf;
	int i;

	if(strlen(ids)>SVG_MAX_SIZNAME) {
		return SVG_ERR_BADIDSTR;
	}
	strcpy(p0,ids);
	if(*p0=='_') p0++;
	if(!(p1=strchr(p0,'_'))) {
		return SVG_ERR_BADIDSTR;
	}
	*p1++=0;
	if(get_ep_name(p0,ep0) || get_ep_name(p1,ep1) || (i=strcmp(ep0->prefix,ep1->prefix))>0 ||
			(i==0 && strcmp(ep0->name,ep1->name)>=0)) {
		return SVG_ERR_BADIDSTR;
	}
	return 0;
}

static int copy_prefixname(char *p,SVG_ENDPOINT *ep)
{
	if(ep->prefix[0]) return sprintf(p,"%s:%s",ep->prefix,ep->name);
	return strlen(strcpy(p,ep->name));
}

static int chk_idkey(LPCSTR ids,SVG_ENDPOINT *ep0,SVG_ENDPOINT *ep1)
{
	int e;
	char key[SVG_MAX_SIZNAME+1];
	char *p=key+1;

	if(e=get_ep_names(ids,ep0,ep1)) return e;

	p+=strlen(strcpy(p,ep0->prefix)); *p++=0;
	p+=strlen(strcpy(p,ep0->name)); *p++=0;
	p+=strlen(strcpy(p,ep1->prefix)); *p++=0;
	p+=strlen(strcpy(p,ep1->name));
	*key=(char)(p-key-1);
	if(trx.Find(key)) {
		//This will be a frequent problem!
		//Record names for reporting in refbuf --
		e=copy_prefixname(refbuf,ep0);
		strcpy(refbuf+e," - ");
		copy_prefixname(refbuf+e+3,ep1);
		return SVG_ERR_VECNOTFOUND;
	}

	if(bUseLabelPos) {
		//Insert endpoint station name IDs. Later, mark them "present" when
		//they are found in the w2d_Labels layer, in which case their station-
		//relative position in the merge file will also be recorded. Finally,
		//when the new w2d_Labels layer is written, omit a label if it exists
		//in this index but is not marked. If it exists and is marked, use the
		//recorded position.
		SVG_TYP_TRXLABEL lrec;
		trx.UseTree(SVG_TRX_LABELS);
		lrec.bPresent=0;
		lrec.sx=lrec.nx=(float)ep0->x;
		lrec.sy=lrec.ny=(float)ep0->y; //merge file page coordinates of endpoint
		get_idstr(key,ep0->prefix,ep0->name,FALSE);
		trx.InsertKey(&lrec,trx_Stxcp(key));
		lrec.sx=lrec.nx=(float)ep1->x;
		lrec.sy=lrec.ny=(float)ep1->y; //merge file page coordinates of endpoint
		get_idstr(key,ep1->prefix,ep1->name,FALSE);
		trx.InsertKey(&lrec,trx_Stxcp(key));
		trx.UseTree(SVG_TRX_VECTORS);
	}

	return 0;
}

static double dist_to_vec(double px,double py, double *B, double *M)
{
	//Vector is defined as B+t*M, 0<=t<=1
	//Minimum squared length is assumed >= min_veclen_sq
	double l,t;

	px-=B[0];
	py-=B[1];
	t=M[0]*px+M[1]*py;
	if(t>0) {
		l=M[0]*M[0]+M[1]*M[1];
		if(t>=l) {
			px-=M[0];
			py-=M[1];
		}
		else {
			t/=l;
			px-=t*M[0];
			py-=t*M[1];
		}
	}
	return px*px+py*py;
}

static double max_dist_to_vec(double px,double py, double *B, double *M)
{
	//Return maximum quared distance from (px,py) to a vector.
	//Vector is defined as B+t*M, 0<=t<=1
	//Minimum squared length is assumed >= min_veclen_sq
	double dx,dy,dmax;

	dx=B[0]-px; dy=B[1]-py;
	dmax=dx*dx+dy*dy;
	dx+=M[0];
	dy+=M[1];
	dy=dx*dx+dy*dy;
	return (dy>dmax)?dy:dmax;
}

static double dist_to_midpoint(double x,double y,SVG_VECNODE *pn)
{
	double dx=x-pn->B_xy[0]-0.5*pn->M_xy[0];
	double dy=y-pn->B_xy[1]-0.5*pn->M_xy[1];
	return dx*dx+dy*dy;
}

static double insert_veclst(int id,double dist)
{
	//Largest distance is at bottom of list --
	int i,j;
	for(i=0;i<vecs_used;i++) {
		if(!(j=veclst_i[i])) break;
		if(j==id) goto _dret;
	}
	for(i=0;i<vecs_used;i++) if(dist<=veclst_d[i]) break;
	if(i<vecs_used) {
		for(j=vecs_used-1;j>i;j--) {
			veclst_i[j]=veclst_i[j-1];
			veclst_d[j]=veclst_d[j-1];
		}
		veclst_i[i]=id;
		veclst_d[j]=dist;
	}
_dret:
	return veclst_d[vecs_used-1];
}

static double adjust_veclst(double x,double y)
{
	int i,j,isav,imax=vecs_used;
	double dsav;

	for(i=0;i<vecs_used;i++) {
		if(!(j=veclst_i[i])) {
			imax=i;
			break;
		}
		veclst_d[i]=dist_to_vec(x,y,vnod[j].B_xy,vnod[j].M_xy);
	}
	for(i=0;i<imax;i++) {
		for(j=i+1;j<imax;j++) {
			if(veclst_d[j]<veclst_d[i]) {
				dsav=veclst_d[i];
				veclst_d[i]=veclst_d[j];
				veclst_d[j]=dsav;
				isav=veclst_i[i];
				veclst_i[i]=veclst_i[j];
				veclst_i[j]=isav;
			}
		}
	}
	return imax?veclst_d[imax-1]:DBL_MAX;
}

static double min_vecdist(double x,double y)
{
	//return squared distance to nearest vector in vnod[] tree --
	double dmin,dmid;
	int ilast;
	int slevel;
	int nvisits;
	SVG_VECNODE *pn;

	//Get maximum distance to set of vectors nearest last point --
	dmin=adjust_veclst(x,y);

	vstk[0]=slevel=ilast=nvisits=0;

	while(slevel>=0) {
		pn=&vnod[vstk[slevel]];
		//Was the last node visited a child of this one?
		if(pn->lf==ilast || pn->rt==ilast) {
			if(pn->rt==ilast || pn->rt==-1) {
				//finished with this node --
				ilast=vstk[slevel--];
				continue;
			}
			//Finally, examine the right child --
			pn=&vnod[vstk[++slevel]=pn->rt];
		}
		//otherwise it was this node's parent, so examine this node --
		ilast=vstk[slevel];

		//Now examine pn's vector where pn=&vnod[ilast=vstk[slevel]] --
		dmid=dist_to_midpoint(x,y,pn);
		if(dmid<=pn->maxdist || //helps significantly!
			sqrt(dmid)-sqrt(pn->maxdist)<sqrt(dmin)) {
			//Search this node and its children --
			if(ilast>=NUM_HDR_NODES) {
				nvisits++;
				dmid=dist_to_vec(x,y,pn->B_xy,pn->M_xy);
				if(dmid<dmin) dmin=insert_veclst(ilast,dmid);
			}
			if((vstk[++slevel]=pn->lf)>0 || (vstk[slevel]=pn->rt)>0) continue;
			slevel--;
		}
		//We are finished with this node --
		slevel--;
	}
	node_visits+=nvisits;
	return veclst_d[0]; //distance to nearest vector --
	//return dmin; //distance to 3rd nearest vector --
}

static SVG_VECNODE *alloc_vnode(double *b_xy,double *m_xy)
{
	SVG_VECNODE *pn,*pnret;
	int slevel;
	int ichild,idx;
	double divider,midp[2];

	midp[0]=b_xy[0]+0.5*m_xy[0];
	midp[1]=b_xy[1]+0.5*m_xy[1];

	vstk[0]=slevel=0;
	pn=&vnod[0];

	while(TRUE) {
		idx=(slevel&1);
		divider=pn->B_xy[idx]+0.5*pn->M_xy[idx];
		if(midp[idx]<divider) ichild=pn->lf;
		else ichild=pn->rt;
		if(ichild==-1) {
			if(vnod_len>=vnod_siz) return NULL;
			if(midp[idx]<divider) pn->lf=vnod_len;
			else pn->rt=vnod_len;
			pn=&vnod[vnod_len++];
			break;
		}
		pn=&vnod[ichild];
		vstk[++slevel]=ichild;
	}
	
	//Set maxdist to the squared distance from the vector's midpoint to an endpoint --
	pn->maxdist=0.25*(m_xy[0]*m_xy[0]+m_xy[1]*m_xy[1]);
	pn->B_xy[0]=b_xy[0]; pn->B_xy[1]=b_xy[1]; 
	pn->M_xy[0]=m_xy[0]; pn->M_xy[1]=m_xy[1];
	pn->lf=pn->rt=-1;
	pnret=pn;

	if(slevel>max_slevel) max_slevel=slevel;

	//Now update maxdist in ancestor nodes, the computed value
	//being the maximum distance between this vector (any point within)
	//and the midpoint of an ancestor's vector.
	for(;slevel>=0;slevel--) {
		pn=&vnod[vstk[slevel]];
		divider=max_dist_to_vec(pn->B_xy[0]+0.5*pn->M_xy[0],
			pn->B_xy[1]+0.5*pn->M_xy[1],b_xy,m_xy);
		if(divider>pn->maxdist) pn->maxdist=divider;
	}
	return pnret;
}

static BOOL length_small(double *xy)
{
	if(xy[0]*xy[0]+xy[1]*xy[1]<min_veclen_sq) {
		if(skipcnt<MAX_VEC_SKIPPED) skiplin[skipcnt]=XML_GetCurrentLineNumber(parser);
		skipcnt++;
		return TRUE;
	}
	return FALSE;
}

static BOOL get_xy_coords(LPCSTR *attr,double *x)
{
	for(int i=0;i<2;i++) {
		if(!*attr || !isalpha((BYTE)**attr) || !*++attr || !isdigit((BYTE)**attr)) return FALSE;
		*x++=atof(*attr++);
	}
	return TRUE;
}

static char *get_coord(char *p0,double *d)
{
	char *p=p0;
	char c;

	if(*p=='-') p++;
	else{
		p=++p0;
		if(*p=='-') p++;
	}

	while(isdigit((BYTE)*p)) p++;
	if(*p=='.') {
		p++;
		while(isdigit((BYTE)*p)) p++;
	}
	c=*p;
	if(p==p0) return 0;
	*p=0;
	*d=atof(p0);
	*p=c;
	return p;
}

static BOOL get_line_coords(LPCSTR *attr,double *x1,double *x2)
{
	return get_xy_coords(attr,x1) && get_xy_coords(attr+4,x2);
}

static char *get_path_coords(char *data,SVG_PATHPOINT *pt)
{
	if(data=get_coord(data,&pt->x)) data=get_coord(data,&pt->y);
	return data;
}

static double abs_max(double a,double b)
{
	a=fabs(a); b=fabs(b);
	return (a>b)?a:b;
}

static int addto_vnode(LPCSTR *attr)
{
	if(!attr[0] || strcmp(attr[0],"id") || !attr[2] ||	strcmp(attr[2],"style") || !attr[4])
		return SVG_ERR_NOPATHID;

	LPCSTR idstr=attr[1],style=attr[3];

	SVG_ENDPOINT ep0,ep1;

	if(!strcmp(attr[4],"d")) {
		char *data=(char *)attr[5];

		if(*data!='M' || !(data=get_coord(data,&ep0.x)) || !(data=get_coord(data,&ep0.y))) {
			return SVG_ERR_VECPATH;
		}
		while(isspace((BYTE)*data)) data++;

		//data is postioned at type char for TO station's coordinates --
		if(*data==0) {
			if(skipcnt<MAX_VEC_SKIPPED) skiplin[skipcnt]=XML_GetCurrentLineNumber(parser);
			skipcnt++;
			return 0;
		}

		if(!strchr("lLvVhH",*data)) {
			return SVG_ERR_VECPATH;
		}
		BOOL bRelative=islower(*data);

		switch(toupper(*data)) {
			case 'L' :
				if(!(data=get_coord(data,&ep1.x)) || !(data=get_coord(data,&ep1.y))) {
					return SVG_ERR_VECPATH;
				}
				if(bRelative) {
					ep1.x+=ep0.x; ep1.y+=ep0.y;
				}
				break;
			case 'V' :
				ep1.x=ep0.x;
				if(!(data=get_coord(data,&ep1.y))) {
					return SVG_ERR_VECPATH;
				}
				if(bRelative) ep1.y+=ep0.y;
				break;
			case 'H' :
				ep1.y=ep0.y;
				if(!(data=get_coord(data,&ep1.x))) {
					return SVG_ERR_VECPATH;
				}
				if(bRelative) ep1.x+=ep0.x;
				break;
		}
	}
	else if(!strcmp(attr[4],"x1")) {
		if(!get_line_coords(attr+4,&ep0.x,&ep1.x)) return SVG_ERR_LINEDATA;
	}
	else {
		return SVG_ERR_NOPATH;
	}

	SVG_TYP_TRXVECTOR rec;
	double abs_diffx,abs_diffy,bxy[2],mxy[2];
	int e;

    if(e=chk_idkey(idstr,&ep0,&ep1)) return e;

	if(e=trx.GetRec(&rec,siz_vecrec)) {
		return SVG_ERR_UNKNOWN;
	}

	if(bUseStyles) {
		//Override vector color index if merged version is different --
		LPCSTR p=getColorPtr(style,st_stroke);
		if(p) {
			//For now, marker color is set to color of first merged vector!
			if(bUseStyles && cMarkerClr==(COLORREF)-1) cMarkerClr=colorval(p);
			if((e=add_vecColor(p))!=(int)rec.wVecColor) {
				//e>=0
				rec.wVecColor=(WORD)e;
				if(trx.PutRec(&rec)) return SVG_ERR_INDEXWRITE;
			}
		}
		if(fVectorThick<0.0) {
			if(p=getStyleValPtr(style,st_stroke_width)) fVectorThick=atof(p);
			else fVectorThick=1.0; //Illustrator will not specify stroke widths of 1.0
		}
	}

	ctm_mult(&utm_to_mrgxy,rec.fr_xy,bxy);
	ctm_mult(&utm_to_mrgxy,rec.to_xy,mxy);

	//Save maximum absolute change in coordinates --
	abs_diffx=abs_max(bxy[0]-ep0.x,mxy[0]-ep1.x);
	abs_diffy=abs_max(bxy[1]-ep0.y,mxy[1]-ep1.y);

	//Parametric form --
	ep1.x-=ep0.x; ep1.y-=ep0.y;
	mxy[0]-=bxy[0];
	mxy[1]-=bxy[1];

 	//Don't use a vector with squared hz length less than min_veclen_sq --
	if(length_small(&ep1.x) || length_small(mxy)) return 0;

	//Check vector orientation
	double d=fabs(atan2(ep1.y,ep1.x)-atan2(mxy[1],mxy[0]));
	if(d>PI) d=2*PI-d;
	if(d>PI/2) {
		return SVG_ERR_VECDIRECTION;
	}

	if(abs_diffx>max_diffx) max_diffx=abs_diffx;
	if(abs_diffy>max_diffy) max_diffy=abs_diffy;

	SVG_VECNODE *pn=alloc_vnode(&ep0.x,&ep1.x);

	if(!pn) {
		return SVG_ERR_UNKNOWN;
	}

	for(e=0;e<2;e++) {
		pn->Bn_xy[e]=bxy[e];
		pn->Mn_xy[e]=mxy[e];
	}

	veccnt++;
	return 0;
}

static void adjust_point(double *P)
{
	int i;
	double lsq[LEN_VECLST],lsqr[LEN_VECLST],wt[LEN_VECLST],wtsum;
	double S[2],*B,*M,*Bn,*Mn;
	double t,v;

	wtsum=0.0;
	for(i=0;i<vecs_used;i++) {
		M=vnod[veclst_i[i]].M_xy;
		lsqr[i]=sqrt(lsq[i]=M[0]*M[0]+M[1]*M[1]);
		wtsum+=(wt[i]=pow(lsqr[i],len_exp)/pow(dist_off+sqrt(veclst_d[i]),dist_exp));
	}

	S[0]=S[1]=0.0;
	for(i=0;i<vecs_used;i++) {
		SVG_VECNODE *pn=&vnod[veclst_i[i]];
		M=pn->M_xy;
		B=pn->B_xy;
		Bn=pn->Bn_xy;
		Mn=pn->Mn_xy;

		//relative distance along vector --
		t=(M[0]*(P[0]-B[0])+M[1]*(P[1]-B[1]))/lsq[i];
		//distance from vector (left side positive) --
		v=(M[1]*(P[0]-B[0])-M[0]*(P[1]-B[1]))/lsqr[i];

		v/=sqrt(Mn[0]*Mn[0]+Mn[1]*Mn[1]);
		S[0]+=wt[i]*(Bn[0] + t*Mn[0] + Mn[1]*v - P[0]);
		S[1]+=wt[i]*(Bn[1] + t*Mn[1] - Mn[0]*v - P[1]);
	}
	P[0]+=S[0]/wtsum;
	P[1]+=S[1]/wtsum;
}

static void update_pathrange(SVG_PATHPOINT *pp,int npts)
{
	SVG_PATHPOINT p;

	if(!npts) {
	  ctm_mult(&mrgxy_to_xy,&pp[0].x,&p.x);
	  xpath_min=xpath_max=p.x;
	  ypath_min=ypath_max=p.y;
	}
	else
	for(int i=1;i<=npts;i++) {
	  ctm_mult(&mrgxy_to_xy,&pp[i].x,&p.x);
	  if(xpath_min>p.x) xpath_min=p.x;
	  else if(xpath_max<p.x) xpath_max=p.x;
	  if(ypath_min>p.y) ypath_min=p.y;
	  else if(ypath_max<p.y) ypath_max=p.y;
	}
}

static void process_points(SVG_PATHPOINT *pp,int npts)
{
	//If npts>0, pp[1..npts] are npts data points to be transformed.
	//The coordinates are in respect to the merge file's frame.
	//If npts=0, pp[0] is a single data point to be transformed.

	if(!bWriting) {
		udata.errcode=SVG_ERR_UNKNOWN;
	}
	else {
		if(bNTW && !bAdjust) return;

		BOOL bTrans=(iLevelSym || (iGroupFlg&FLG_SYM_MSK)!=0);

		for(int i=(npts?1:0);i<=npts;i++) {
			if(bAdjust && !bTrans) {
				double d;
				d=min_vecdist(pp[i].x,pp[i].y);
				if(d>max_dsquared) {
					max_dsquared=d;
					max_dsq_x=pp[i].x;
					max_dsq_y=pp[i].y;
					for(int j=0;j<vecs_used;j++) max_iv[j]=veclst_i[j];
				}
				//The nearest vectors to pp[i] are now contained in veclst_i[] --
				adjust_point(&pp[i].x);
			}
			if(!bNTW) ctm_mult((bTrans?&symTransCTM:&mrgxy_to_xy),&pp[i].x);
		}
	}
}

static void getSymTransCTM(SVG_PATHRANGE *pr)
{
	//Obtain symTransCTM matrix for converting a symbol object to
	//output page coordinates. First, the symbol is translated and
	//scaled to occupy the new position of the bounding box's center.
	//Then, id the symbol is not in a SYM layer, it's rotated by viewA-mrgViewA.

	UINT flgsav=(iGroupFlg&FLG_SYM_MSK);

	SVG_PATHPOINT p,p0;
	//Bounding box is in new page coordinates --
	p.x=(pr->xmin+pr->xmax)/2.0;
	p.y=(pr->ymin+pr->ymax)/2.0;

	//Convert back to merge file coordinates --
	ctm_mult(&xy_to_mrgxy,&p.x,&p0.x);

	if(bAdjust) {
		//Assumes iLevelSym==0
		iGroupFlg&=~FLG_SYM_MSK;
		p=p0;
		process_points(&p,0);
		iGroupFlg|=flgsav;
	}

	//p is now the bounding box's new center while p0 is the old center's merge file coordinates.
	symTransCTM.a=symTransCTM.d=-scaleRatio;
	symTransCTM.b=symTransCTM.c=0.0;
	symTransCTM.e=p.x-scaleRatio*p0.x;
	symTransCTM.f=p.y-scaleRatio*p0.y;
	if(!flgsav && viewA!=mrgViewA) {
		SVG_CTM ctm;
		getRotateCTM(&ctm,&p,viewA-mrgViewA);
		ctm_mult(&ctm,&symTransCTM);
	}
}

static double path_length(SVG_PATHPOINT *pp,int i)
{
	double dx=pp[i].x-pp[0].x;
	double dy=pp[i].y-pp[0].y;
	return sqrt(dx*dx+dy*dy);
}

static void out_point(double x,double y,BOOL bComma)
{
	char fbuf[40];

	fltstr(fbuf,x);
	if(bComma && *fbuf!='-') {
		OUTC(','); nCol++;
	}
	OUTS(fbuf);
	nCol+=strlen(fbuf);

	fltstr(fbuf,y);
	if(*fbuf!='-') {
		OUTC(','); nCol++;
	}
	OUTS(fbuf);
	nCol+=strlen(fbuf);
}

static BOOL check_prange(SVG_PATHRANGE *pr)
{
	if(pr->xmax+max_diffx<=0 || pr->xmin-max_diffx>=width ||
	   pr->ymax+max_diffy<=0 || pr->ymin-max_diffy>=height) return FALSE;
	return TRUE;
}

char * find_attrVal(LPCSTR *attr,LPCSTR name)
{
	for(;*attr;attr+=2) {
		if(!strcmp(*attr,name)) {
			return (char *)attr[1];
		}
	}
	return NULL;
}

static LPCSTR getTransformMatrixPtr(LPCSTR *attr)
{
	LPCSTR p=find_attrVal(attr,"transform");
	if(p && strlen(p)>7 && !memcmp(p,"matrix(",7)) return p+7;
	return NULL;
}

static LPCSTR getGradientTransformMatrixPtr(LPCSTR *attr)
{
	LPCSTR p=find_attrVal(attr,"gradientTransform");
	if(p && strlen(p)>7 && !memcmp(p,"matrix(",7)) return p+7;
	return NULL;
}

static int getPathCTM(LPCSTR *attr)
{
	LPCSTR p;

	bPathCTM=FALSE;
	if(p=find_attrVal(attr,"transform")) {
		if(strlen(p)<7 || memcmp(p,"matrix(",7) || (6!=sscanf(p+7,"%lf %lf %lf %lf %lf %lf",
			&pathCTM.a,&pathCTM.b,&pathCTM.c,&pathCTM.d,&pathCTM.e,&pathCTM.f))) return -1;
		bPathCTM=TRUE;
	}
	return bPathCTM;
}

static SVG_PATHPOINT *getPathCTM_Pt(LPCSTR *attr,SVG_PATHPOINT *pt)
{
	if(!getPathCTM(attr)) return NULL;
	if(!pt) pt=(SVG_PATHPOINT *)&pathCTM.e;
	else *pt=*(SVG_PATHPOINT *)&pathCTM.e;
	return pt;
}

static void applyPathCTM(SVG_PATHPOINT *pp,int len)
{
	double t;
	for(;len;pp++,len--) {
		pp->x=pathCTM.a*(t=pp->x) + pathCTM.c*pp->y + pathCTM.e;
		pp->y=pathCTM.b*t + pathCTM.d*pp->y + pathCTM.f;
	}
}

static BOOL fixRotateCTM(SVG_PATHPOINT *pp)
{
	//Called only for P_RECT and P_ELLIPSE shapes, just prior to transform
	//attribute output --
	SVG_PATHPOINT pt;
	double rotA;

	//Compute angle rotated due to any change of coordinates --
	if(iGroupFlg&FLG_SYM_MSK) rotA=0.0;
	else {
		//In Walls, positive views correspond to ccw (negative) rotation angles in SVG --
		rotA=-(viewA-mrgViewA);
	}

	if(bPathCTM) {
		//Only rotation transforms are supported for path objects --
		if(pathCTM.a!=pathCTM.d || pathCTM.b!=-pathCTM.c) return FALSE;
		rotA+=atan2(pathCTM.b,pathCTM.a);
	}

	if(iPathType==P_RECT) {
		//The four points in pp[] have already been transformed to output page coordinates,
		//with both merge file and output file rotations applied depending on sym layer
		//membership. This rotation was computed above as rotA. We now need to insure the
		//rectangle's coordinates are again "level" and compute a new rotation transform
		//that will orient it correctly.

		if(rotA!=0.0) {
			//Any rotation will be about the rectangle's center point --
			pt.x=0.5*(pp[2].x+pp[0].x);
			pt.y=0.5*(pp[2].y+pp[0].y);
			getRotateCTM(&pathCTM,&pt,-rotA);
			applyPathCTM(pp,4);
		}
	}
	else pt=*pp; //P_ELLIPSE

	if(rotA!=0.0) {
		getRotateCTM(&pathCTM,&pt,rotA);
		bPathCTM=TRUE;
	}
	else bPathCTM=FALSE;

	return TRUE;
}

static char *getTransform(char *pTrans,LPCSTR *attr,UINT uScale)
{
	//uScale:	0 - Text element in _sym or legend layer (not scaled)
	//			1 - Transform on w2d_Legend element
	//			2 - Symbol instance
	//			3 - Getting pt inside legend, or setting label/note position
	SVG_PATHPOINT pt,pt2;
	double a,b,c,d;
	char abuf[20],dbuf[20],ebuf[20],fbuf[20];
	LPCSTR pAttr;

	if(bInsideGradient==1) pAttr=getGradientTransformMatrixPtr(attr);
	else pAttr=getTransformMatrixPtr(attr);

	if(pAttr) {
		if(!uScale && (iGroupFlg&FLG_LEGEND)) {
			char *p=strchr(pAttr,')');
			if(p) {
				memcpy(pTrans,pAttr,uScale=p-pAttr);
				pTrans[uScale]=0;
				return pTrans;
			}
			return NULL;
		}
		if(6!=sscanf(pAttr,"%lf %lf %lf %lf %lf %lf",&a,&b,&c,&d,&pt.x,&pt.y)) return NULL;
		bReverseY=(d<0.0);
		if(uScale==3) {
			*(SVG_PATHPOINT *)pTrans=pt;
			return pTrans;
		}
	}
	else {
		if(uScale!=1) return NULL;
		a=d=1.0;
		pt.x=pt.y=0.0;
	}

	//uScale:	0 - Text element in _sym layer (not scaled)
	//			1 - Transform on w2d_Legend element
	//			2 - Symbol instance

	if(uScale==1) {
		pt2=legendPt;
		pt2.x*=a;
		pt2.y*=d;
		pt2.x+=pt.x;
		pt2.y+=pt.y;			//pt2 are merge file's actual page coordinates of legend pt
		ctm_mult(&mrgxy_to_xy,&pt2.x,&pt.x); //pt are final page coordinates of legend pt
	}
	else if(!iLevelSym) {
		//transform point --
		UINT flgsav=(iGroupFlg&FLG_SYM_MSK);
		iGroupFlg&=~FLG_SYM_MSK;
		process_points(&pt,0);
		iGroupFlg|=flgsav;
	}
	else process_points(&pt,0); //symbol group

	if(isInside(&pt.x)) {
		if(uScale==1 || uScale==2) {
			a*=scaleRatio;
			d*=scaleRatio;
			if(uScale==1) {
				pt.x-=legendPt.x*a;
				pt.y-=legendPt.y*d;
			}
		}
		sprintf(pTrans,"%s 0 0 %s %s %s",
			fltstr(abuf,a,3),
			fltstr(dbuf,d,3),
			fltstr(ebuf,pt.x,4),
			fltstr(fbuf,pt.y,4));
		return pTrans;
	}
	return NULL;
}

static BOOL GetSymTransform(LPCSTR *attr)
{
	SVG_PATHPOINT pt;

	if(!getPathCTM_Pt(attr,&pt)) return FALSE;
	if(iGroupFlg&FLG_LEGEND) return TRUE;

	if(!iLevelSym) {
		//transform point --
		UINT flgsav=(iGroupFlg&FLG_SYM_MSK);
		iGroupFlg&=~FLG_SYM_MSK;
		process_points(&pt,0);
		iGroupFlg|=flgsav;
	}
	else process_points(&pt,0); //symbol group

	if(!isInside(&pt.x)) return FALSE;

	if(!(iGroupFlg&FLG_SYM_MSK) && viewA!=mrgViewA) {
		SVG_CTM ctm;
		getRotateCTM(&ctm,NULL,viewA-mrgViewA);
		ctm_mult(&ctm,&pathCTM);
	}

	pathCTM.a*=scaleRatio;
	pathCTM.b*=scaleRatio;
	pathCTM.c*=scaleRatio;
	pathCTM.d*=scaleRatio;
	pathCTM.e=pt.x;
	pathCTM.f=pt.y;
	return TRUE;
}

static void updateRange(SVG_PATHRANGE *r,SVG_PATHRANGE *s)
{
	if(r->xmax<s->xmax) r->xmax=s->xmax;
	if(r->ymax<s->ymax) r->ymax=s->ymax;
	if(r->xmin>s->xmin) r->xmin=s->xmin;
	if(r->ymin>s->ymin) r->ymin=s->ymin;
}

static void updateRange(SVG_PATHRANGE *r,SVG_PATHPOINT *p)
{
	float x=(float)p->x,y=(float)p->y;

	if(r->xmax<x) r->xmax=x;
	if(r->ymax<y) r->ymax=y;
	if(r->xmin>x) r->xmin=x;
	if(r->ymin>y) r->ymin=y;
}

static void updateSymRange(LPCSTR *attr)
{
	SVG_PATHPOINT pt;
	if(getPathCTM_Pt(attr,&pt)) {
		ctm_mult(&mrgxy_to_xy,&pt.x);
		updateRange(&symrange[symcnt],&pt);
	}
}

static int GetStyleAttr(LPCSTR *attr)
{
	for(int i=0;attr[i];i+=2) {
		if(!strcmp(attr[i],"style"))  return i+1;
	}
	return 0;
}

static COLORREF GetFillColor(LPCSTR *attr)
{
	  int i=GetStyleAttr(attr);
	  return i?getColorAttr(attr[i],st_fill):((COLORREF)-1);
}

static int traverse_path(LPCSTR *attr)
{
	int tp,npts;
	double tl;
	SVG_PATHPOINT bp,mp,pp[6];
	BOOL bRelative;
	char *data;
	char typ;

	data=find_attrVal(attr,"d");
	data=get_path_coords(data,&bp);

	pp[0]=bp; //unadjusted base point is saved in bp

	//Init range or adjust point --
	if(bWriting) {
		process_points(pp,0);
		if(bNTW) {
			out_ntw_points(pp,'m');
		}
		else {
			nCol+=gz_printf("d=\"M");
			out_point(pp[0].x,pp[0].y,0);
		}
	}
	else {
		if(!bNTW) update_pathrange(pp,0);
		mp=pp[0];
	}

	tp=1;
	tl=0.0;

	while(isspace((BYTE)*data)) data++;

	while(*data) {
		//data is postioned at type char for next set of control points --
		if(!strchr("cChHlLMsSvVz",*data)) {
			return -SVG_ERR_WALLPATH;
		}
		bRelative=islower(*data);

		switch(typ=tolower(*data)) {
			case 'z' :
				while(isspace((BYTE)*++data));
				if(bWriting) {
					if(!bNTW) {
						OUTC('z');
						nCol++;
					}
					else *data=0;
				}
				else {
					pp[1]=mp;
					if(!bNTW && (iGroupFlg&(FLG_DETAIL_SHP+FLG_WALLS_SHP))) tl+=path_length(pp,1);
				}
				continue;

			case 'm' :
				//Compound path --
				if(iGroupFlg&FLG_MASK) {
					*data=0;
					continue;
				}

				//Can't get here when bNTW --

				if(!(data=get_path_coords(data,&bp))) {
					return -SVG_ERR_WALLPATH;
				}
				while(isspace((BYTE)*data)) data++;
				pp[0]=bp; //unadjusted base point is saved in bp

				if(bWriting) {
					//Adjust point --
					process_points(pp,0);
					OUTC('M'); nCol++;
					out_point(pp[0].x,pp[0].y,0);
				}
				else mp=bp;
				continue;

			case 'l' :
				if(!(data=get_path_coords(data,&pp[1]))) {
					return -SVG_ERR_WALLPATH;
				}
				npts=1;
				break;

			case 'v' :
				pp[1].x=bRelative?0:bp.x;
				if(!(data=get_coord(data,&pp[1].y))) {
					return -SVG_ERR_WALLPATH;
				}
				typ='l';
				npts=1;
				break;

			case 'h' :
				pp[1].y=bRelative?0:bp.y;
				if(!(data=get_coord(data,&pp[1].x))) {
					return -SVG_ERR_WALLPATH;
				}
				typ='l';
				npts=1;
				break;

			case 's' :
			case 'c' :
				npts=(typ=='c')?3:2;
				for(int i=1;i<=npts;i++) {
					if(!(data=get_path_coords(data,&pp[i])))
						return -SVG_ERR_WALLPATH;
				}
				break;
				
		}

		if(bRelative) {
			for(int i=1;i<=npts;i++) {
				pp[i].x+=bp.x;
				pp[i].y+=bp.y;
			}
		}
		bp=pp[npts]; //save new unadjusted base point

		tp+=npts;

		if(bWriting) process_points(pp,npts);
		else if(!bNTW) update_pathrange(pp,npts);

		if(bWriting) {
			if(bNTW) out_ntw_points(&pp[1],typ);
			else {
				if(nCol>SIZ_PATH_LINEBREAK) {
					OUTS("\r\n\t");
					nCol=skip_depth()+1;
				}
				OUTC(typ); nCol++;
				for(int i=1;i<=npts;i++) {
					out_point(pp[i].x-pp[0].x,pp[i].y-pp[0].y,i>1);
				}
			}
		}
		else if(!bNTW && (iGroupFlg&(FLG_DETAIL_SHP+FLG_WALLS_SHP)))
			tl+=path_length(pp,npts);

		while(isspace((BYTE)*data)) data++;
		if(!*data) break;

		pp[0]=pp[npts];
	}
	if(bWriting) {
		if(bNTW) {
			out_ntw_points(NULL,typ); //write last record with close flag if typ=='z'
			if(bAllocErr) return -SVG_ERR_NOMEM;
		}
		else {
			OUTS("\"/>\r\n");
			nCol=0;
		}
	}
	else if(!bNTW) {
		if(iGroupFlg&FLG_WALLS_SHP) flength_w+=tl;
		else if(iGroupFlg&FLG_DETAIL_SHP) flength_d+=tl;
	}
	return tp;
}

static void out_point_coords(SVG_PATHPOINT *pt)
{
	OUTS(fltstr(pt->x));
	if(pt->y>=0.0) OUTC(',');
	OUTS(fltstr(pt->y));
}

static BOOL get_named_coords(LPCSTR *attr,SVG_PATHPOINT *pt,char *xnam,char *ynam)
{
	char *p;
	
	if(!(p=find_attrVal(attr,xnam))) return FALSE;
	pt->x=atof(p);
	if(!(p=find_attrVal(attr,ynam))) return FALSE;
	pt->y=atof(p);
	return TRUE;
}

static int GetNextPolyPt(SVG_PATHPOINT *pp)
{
	char *p=pNextPolyPt;

	while(isspace((BYTE)*p)) p++;
	if(*p) {
		pp->x=atof(p);
		while(*++p) {
			if((*p==',' && ++p) || *p=='-') break;
		}
		if(!*p) return -SVG_ERR_POLYDATA;
		pp->y=atof(p);
		while(!isspace((BYTE)*p) && *p) p++;
		pNextPolyPt=p;
		return 1;
	}
	pNextPolyPt=p;
	return 0;
}

static int expand_ellipse(LPCSTR *attr,SVG_PATHPOINT *pp)
{
	//Expand points around center at pp for range calculation --
	char *p=find_attrVal(attr,(iPathType==P_CIRCLE)?"r":"rx");
	if(!p) return 1;
	double r=atof(p);
	pp[1].x=pp[0].x-r;
	pp[2].x=pp[0].x+r;
	pp[1].y=pp[2].y=pp[0].y;
	if(iPathType==P_ELLIPSE && (p=find_attrVal(attr,"ry"))) r=atof(p);
	pp[3].y=pp[0].y-r;
	pp[4].y=pp[0].y+r;
	pp[3].x=pp[4].x=pp[0].x;
	return 5;
}

static int get_shape_pts(LPCSTR *attr,SVG_PATHPOINT *pp)
{
	int e;

	switch(iPathType) {
		case P_LINE:
			return (get_named_coords(attr,pp,"x1","y1") &&
				get_named_coords(attr,pp+1,"x2","y2"))?2:-SVG_ERR_LINEDATA;

		case P_CIRCLE:
		case P_ELLIPSE:
			if(get_named_coords(attr,pp,"cx","cy")) {
				if(!bWriting) {
					//Expand points around center for range calculation --
					e=expand_ellipse(attr,pp);
				}
				else e=1;
				if(bPathCTM) applyPathCTM(pp,e); //should be an ellipse rotation only!
				return e;
			}
			return -SVG_ERR_ELLIPSE;

		case P_POLYLINE:
		case P_POLYGON:
			if(pNextPolyPt || (pNextPolyPt=find_attrVal(attr,"points"))) {
				int i;
				for(i=0;i<MAX_SHAPEPTS;i++) {
					if((e=GetNextPolyPt(pp+i))<=0) break;
				}
				return (e<0)?e:i;
			}

		case P_RECT:
			if(get_named_coords(attr,pp,"x","y") &&
				get_named_coords(attr,pp+1,"width","height")) {
					pp[2].x=pp[0].x+pp[1].x;
					pp[2].y=pp[0].y+pp[1].y;
					pp[3].x=pp[0].x;
					pp[3].y=pp[2].y;
					pp[1].x+=pp[0].x;
					pp[1].y=pp[0].y;
					if(bPathCTM) applyPathCTM(pp,4);
					return 4;
			}
			return -SVG_ERR_RECTDATA;
	}
	return SVG_ERR_UNKNOWN;
}

static int traverse_shape(LPCSTR *attr)
{
	//Process P_LINE, P_RECT, P_POLYLINE, P_POLYGON, P_CIRCLE, or P_ELLIPSE
	int nLen,tp=0;
	double tl=0.0;
	char *px,*py;
	SVG_PATHPOINT bp,pp[MAX_SHAPEPTS+1];

	pNextPolyPt=NULL;

	while((nLen=get_shape_pts(attr,pp))>0) {
		if(bWriting) {
			process_points(pp-1,nLen);
			if(!tp) {
				tp+=nLen;
				if(bNTW) {
					//mask layer processing only --
					out_ntw_points(pp,'m');
					for(int i=1;i<=nLen-1;i++) out_ntw_points(pp+i,'l');
					if(nLen<MAX_SHAPEPTS) break;
				}
				else {
					if(iPathType==P_POLYGON || iPathType==P_POLYLINE) {
						OUTS("points=\"");
						out_point_coords(pp);
						for(int i=1;i<=nLen-1;i++) {
							OUTC(' ');
							out_point_coords(pp+i);
						}
						if(nLen<MAX_SHAPEPTS) break;
					}
					else {
						if(iPathType==P_LINE) {
							OUTS("x1=\""); OUTS(fltstr(pp[0].x));
							OUTS("\" y1=\""); OUTS(fltstr(pp[0].y));
							OUTS("\" x2=\""); OUTS(fltstr(pp[1].x));
							OUTS("\" y2=\""); OUTS(fltstr(pp[1].y));
						}
						else if(iPathType==P_RECT) {
							//Unapply old rotation and apply new one --
							if(bPathCTM || ((iGroupFlg&FLG_SYM_MSK)==0 && viewA!=mrgViewA)) {
								if(!fixRotateCTM(pp)) return -SVG_ERR_ROTXFORM;
								if(bPathCTM) {
									outPathCTM();
									OUTC(' ');
								}
							}
							OUTS("x=\""); OUTS(fltstr(pp[0].x));
							OUTS("\" y=\""); OUTS(fltstr(pp[0].y));
							OUTS("\" width=\""); OUTS(fltstr(pp[2].x-pp[0].x));
							OUTS("\" height=\""); OUTS(fltstr(pp[2].y-pp[0].y));
						}
						else if(iPathType==P_CIRCLE) {
							OUTS("cx=\""); OUTS(fltstr(pp[0].x));
							OUTS("\" cy=\""); OUTS(fltstr(pp[0].y));
							if(!(px=find_attrVal(attr,"r"))) return -SVG_ERR_ELLIPSE;
							OUTS("\" r=\""); OUTS(fltstr(atof(px)*scaleRatio));
						}
						else { //P_ELLIPSE
							if(bPathCTM || ((iGroupFlg&(FLG_DETAIL_SHP+FLG_WALLS_SHP)) &&  sinA!=0.0)) {
								if(!fixRotateCTM(pp)) return -SVG_ERR_ROTXFORM;
								if(bPathCTM) {
									outPathCTM();
									OUTC(' ');
								}
							}
							OUTS("cx=\""); OUTS(fltstr(pp[0].x));
							OUTS("\" cy=\""); OUTS(fltstr(pp[0].y));
							if(!(px=find_attrVal(attr,"rx")) || !(py=find_attrVal(attr,"ry"))) {
								return -SVG_ERR_ELLIPSE;
							}
							OUTS("\" rx=\""); OUTS(fltstr(atof(px)*scaleRatio));
							OUTS("\" ry=\""); OUTS(fltstr(atof(py)*scaleRatio));
						}
						break;
					}
				}
			}
			else {
				//Writing more polygon points --
				if(!bNTW) {
					OUTS("\r\n");
					skip_depth();
				}
				for(int i=0;i<nLen;i++) {
					if(bNTW) out_ntw_points(pp+i,'l');
					else {
						OUTC(' ');
						out_point_coords(pp+i);
					}
				}
			}
		}
		else {
			//Not writing --
			if(!bNTW) {
				if(!tp) {
					update_pathrange(pp,0);
					if(nLen>1) update_pathrange(pp,nLen-1);
				}
				else update_pathrange(pp-1,nLen);

				if(iPathType==P_CIRCLE || iPathType==P_ELLIPSE) {
					return 1;
				}

				if(iGroupFlg&(FLG_DETAIL_SHP+FLG_WALLS_SHP)) {
					//must be P_POLYGON, P_POLYLINE, or P_LINE
					for(int i=0;i<nLen-1;i++) tl+=path_length(pp+i,i+1);
					if(tp) {
						tl+=path_length(pp,MAX_SHAPEPTS);
					}
					if(iPathType==P_POLYGON) {
						if(!tp) bp=pp[0];
						pp[MAX_SHAPEPTS]=pp[nLen-1];
					}
				}
			}
		}
		tp+=nLen;
		if(nLen<MAX_SHAPEPTS) break;
	}

	if(nLen<0) return nLen;

	//No more shape pts --
	if(bWriting) {
		if(!bNTW) OUTS("\"/>\r\n");
		else {
			out_ntw_points(0,(iPathType==P_LINE || iPathType==P_POLYLINE)?0:'z');
			if(bAllocErr) return -SVG_ERR_NOMEM;
		}
	}
	else if(!bNTW && (iGroupFlg&(FLG_DETAIL_SHP+FLG_WALLS_SHP))) {
		if(iPathType==P_POLYGON) {
			pp[0]=bp;
			tl+=path_length(pp,MAX_SHAPEPTS);
		}
		if(iGroupFlg&FLG_WALLS_SHP) flength_w+=tl;
		else flength_d+=tl;
	}
	return tp;
}

int CheckValidPath(LPCSTR *attr)
{
	char *data;
	SVG_PATHPOINT pt;

	if(getPathCTM(attr)<0 || (bPathCTM && (iPathType!=P_RECT && iPathType!=P_ELLIPSE))) {
		//Illustrator transforms only rect and ellipse elements --
		return SVG_ERR_BADXFORM;
	}

	//Set data to start of data string --
	if(iPathType==P_PATH) {
		if(!(data=find_attrVal(attr,"d")) || *data!='M' || !(data=get_path_coords(data,&pt))) {
			return SVG_ERR_WALLPATH;
		}
		while(isspace((BYTE)*data)) data++;
		if(!*data) return -1; //ignore single point path
	}
	else if(iPathType==P_CIRCLE || iPathType==P_ELLIPSE) {
		//These elements ignored in mask layer for now --
		if(bNTW) return -1;
	}

	return 0;
}


static int process_path(LPCSTR *attr)
{
	int tp;
	char *p;

	if(tp=CheckValidPath(attr)) return (tp<0)?0:tp;

	if(bNTW) goto _start;

	if(bWriting) {
		if(pathrange[pathcnt].pathstyle_idx==-1) {
			pathcnt++;
			return 0;
		}
		if(iLevelSym || iGroupFlg&FLG_SYM_MSK) {
			if(!iLevelSym) {
				//Compute translation vector as shift in bounding box's center --
				getSymTransCTM(&pathrange[pathcnt]);
			}
		}
		else {
			if(iPathType==P_RECT && bAdjust) iPathType=P_POLYGON;
		}

		nCol=skip_depth();
		nCol+=gz_printf("<%s ",pathType[iPathType]);

		//Output id and style attributes (if writing) --
		if(p=find_attrVal(attr,"id")) nCol+=gz_printf("id=\"%s\" ",p);
		if(p=find_attrVal(attr,"style")) {
			nCol+=gz_printf("style=\"");
			if((tp=pathrange[pathcnt].pathstyle_idx)>MAX_STYLES) {
				//We must be in the mask layer at this point. Also, bUseStyles must be FALSE.
				if(tp==MAX_STYLES+1) nCol+=gz_printf("&P_s;\" ");
				else nCol+=gz_printf("&B_s;\" ");
			}
			else if(tp<MAX_STYLES) {
				nCol+=gz_printf("&_p%d;\" ",tp+1);
			}
			else nCol+=gz_printf("%s\" ",p);
		}
	}
	else {
		//save path bounding boxes and total path count --
		if(pathcnt>=max_pathcnt) {
			pathrange=(SVG_PATHRANGE *)realloc(pathrange,(max_pathcnt+=MAX_PATHCNT_INC)*sizeof(SVG_PATHRANGE));
		}
		//save path styles for entity definitions --
		if(tp=GetStyleAttr(attr)) {
			if(!bUseStyles && (iGroupFlg&FLG_MASK)) {
				//The styles for paths in the mask layer will be determined
				//by the B_s and P_s entity assignments. If this is the first (outline)
				//path that has a fill, we'll save the color as cPassageClr to test
				//against additional fill colors in this layer. Thereafter, a path filled with
				//cPassageClr will be assigned style P_s and a path filled with cBackClr will be
				//assigned style B_s. Any other fill color will remain unaltered --
				COLORREF clr=getColorAttr(attr[tp],st_fill);
				if(clr!=(COLORREF)-1) {
					if(cPassageClr==(COLORREF)-1) {
						bMaskPathSeen=TRUE;
						cPassageClr=clr;
					}
					if(cPassageClr==clr) tp=(MAX_STYLES+1);
					else if(cBackClr==clr) tp=(MAX_STYLES+2);
					else tp=save_style(pathstyle,attr[tp]);
				}
				else tp=MAX_STYLES;
			}
			else tp=save_style(pathstyle,attr[tp]);
		}
		else tp=MAX_STYLES;
		pathrange[pathcnt].pathstyle_idx=tp;
	}

_start:

	if(iPathType==P_PATH) tp=traverse_path(attr);
	else tp=traverse_shape(attr);

	if(tp<0) return -tp;

	tpoints+=tp;

	if(!bNTW && !bWriting) {
		if(iGroupFlg&FLG_WALLS_SHP) tpoints_w+=tp;
		else if(iGroupFlg&FLG_DETAIL_SHP) tpoints_d+=tp;
		pathrange[pathcnt].xmax=(float)xpath_max;
		pathrange[pathcnt].xmin=(float)xpath_min;
		pathrange[pathcnt].ymax=(float)ypath_max;
		pathrange[pathcnt].ymin=(float)ypath_min;
		if(iLevelSym) {
			updateRange(&symrange[symcnt],&pathrange[pathcnt]);
		}
	}
	pathcnt++;

	if(bAdjust) {
		if((int)((100.0*tpoints)/tptotal)>=(int)tppcnt+5) {
			sprintf(argv_buf,"Adjusting features: %3d%%",tppcnt+=5);
			pGetData(SHP_STATUSMSG,argv_buf);
		}
	}
	return 0;
}

static int GetGroupFlg(LPCSTR name)
{
	int i=1;
	if(name && strlen(name)>4 && !memicmp(name,"w2d_",4)) {
		name+=4;
		for(i=0;i<NUM_W2DNAMES;i++) {
			if(!strcmp(name,w2d_name[i])) {
				iGroupFlg=w2d_flg[i];
				iGroupID=i;
				return 1;
			}
		}
		i=0;
	}
	iGroupFlg=0;
	iGroupID=-1;
	return i;
}

static SVG_PATHPOINT *getOffset(LPCSTR *attr,SVG_PATHPOINT *pt)
{

	LPCSTR id[4]={"x","y","dx","dy"};
	double val[4]={DBL_MIN,DBL_MIN,DBL_MIN,DBL_MIN};
	for(;attr[0];attr+=2) {
		for(int i=0;i<4;i++) if(!strcmp(attr[0],id[i])) {
			val[i]=atof(attr[1]);
			break;
		}
	}
	if(val[0]>DBL_MIN && val[1]>DBL_MIN) {
		pt->x=val[0];
		if(val[2]>DBL_MIN) pt->x+=val[2];
		pt->y=val[1];
		if(val[3]>DBL_MIN) pt->y+=val[3];
		return pt;
	}

	return NULL;
}

static BOOL id_in_pathstyle(LPCSTR idstr)
{
	for(int i=0;i<MAX_STYLES;i++) {
		if(!pathstyle[i]) break;
		if(pathstyle_used[i] && strstr(pathstyle[i],idstr)) return TRUE;
	}
	return FALSE;
}

static BOOL isGradient(LPCSTR el)
{
	return (!strcmp(el,"linearGradient") || !strcmp(el,"radialGradient"));
}

static void GetViewBox(SVG_VIEWBOX *vb,LPCSTR *attr)
{
	float x;
	char *p=find_attrVal(attr,"viewBox");
	if(!p || 4!=sscanf(p,"%f %f %f %f",&x,&x,&vb->width,&vb->height)) {
		vb->height=vb->width=0.0f;
	}
}

static void scanw_startElement(void *userData, LPCSTR el, LPCSTR *attr)
{
  static int iParentFlg;

  if(UD(errcode)) return;

  if(iLevel<2) {
	if(bSymbolScan) {
		if(!strcmp(el,"symbol")) {
			int i=strcmp("_m",attr[1]);
			if((!i && bUseStyles) || (i && (i=find_style(symname,attr[1])))) {
				if(!i) bMarkerWritten=TRUE;
				GetViewBox(&flagViewBox[i],attr);
				outStartElement(el,attr);
				iLevel=2;
			}
		}
		else {
			if(!strcmp(el,"g") && attr[0] && !strcmp(attr[0],"id")) {
				UD(errcode)=SVG_ERR_ENDSCAN;
			}
		}
	}
	else if(!strcmp(el,"g") && attr[0] && !strcmp(attr[0],"id")) {
		GetGroupFlg(attr[1]);
		if(iGroupFlg&FLG_MERGEDLAYERS) {
			if(iLevel!=0 || Depth!=1) {
				UD(errcode)=SVG_ERR_LAYERS;
				goto _errexit;
			}
			if(iGroupID==ID_LEGEND) {
				bLegendSeen=TRUE;
				if(pLegendPt) {
					//Place legend's transform matrix in argv_buf --
					if(getTransform(argv_buf,attr,1)) {
						outStartGroup(iGroupID,FALSE);
						iParentFlg=iGroupFlg;
						iLevel=2;
					}
				}
				else if(flags&SVG_ADJUSTABLE) {
					outGroup(ID_LEGEND);
				}
			}
			else if(iGroupID==ID_MASK) {
				outStartGroup(iGroupID,FALSE);
				iParentFlg=iGroupFlg;
				iLevel=2;
			}
			else if(((iGroupFlg&FLG_WALLS)&&(flags&SVG_WALLS)) ||
				    ((iGroupFlg&FLG_DETAIL)&&(flags&SVG_DETAIL))) {
				outStartGroup(iGroupID,((iGroupFlg<<16)&flags)!=0);
				iParentFlg=iGroupFlg;
				iLevel=1;
			}
			else if(flags&SVG_ADJUSTABLE) {
				outStartGroup(iGroupID,FALSE);
				if(iGroupFlg&FLG_WALLS) {
					outGroup(ID_WALLS_SHP);
					outGroup(ID_WALLS_SYM);
				}
				else if(iGroupFlg&FLG_DETAIL) {
					outGroup(ID_DETAIL_SHP);
					outGroup(ID_DETAIL_SYM);
				}
				outEndElement(el);
			}
		}
		else if(iLevel==1 && (iGroupFlg&FLG_ADJUSTED)) {
			if(Depth!=2 ||
				((iGroupFlg&FLG_DETAIL_MSK)&&iParentFlg!=FLG_DETAIL) ||
				((iGroupFlg&FLG_WALLS_MSK)&&iParentFlg!=FLG_WALLS)) {
				UD(errcode)=SVG_ERR_LAYERS;
				goto _errexit;
			}
			outStartGroup(iGroupID,FALSE);
			iGroupFlg|=iParentFlg;
			iLevel++;
		}
		else if(iGroupFlg&FLG_VECTORS) {
			if(!(UD(errcode)=outWallsData()) && (bLegendSeen || !(flags&SVG_LEGEND))) UD(errcode)=SVG_ERR_ENDSCAN;
		}
	}
	iLevelSym=0;
	return;
  }

  if(bSymbolScan) {
	  if(iLevel==2 && !strcmp(el,"g") && !attr[0]) return;
	  iLevel++;
	  outStartElement(el,attr);
	  return;
  }

  if(bInsideGradient) {
	  //Ignore all but <stop> elements --
	  if(bInsideGradient==2 && !strcmp(el,"stop")) outStartElement(el,attr);
	  return;
  }
  
  //processing shapes--
  if(!strcmp(el,"g")) {
	  iLevel++;
	  //Preserve subgrouping for now--
	  if(!iLevelSym && (iGroupFlg&FLG_WALLS_DETAIL_MSK) && (!attr[0] || strcmp(attr[0],"id"))) {
		  //unnamed group -- let's start calculating bounding box in symrange[] --
		  //symrange[symcnt] contains group range --
		  bSkipSym=!check_prange(&symrange[symcnt]);
		  if(!bSkipSym) getSymTransCTM(&symrange[symcnt]);
		  iLevelSym=iLevel;
		  //Increment symcnt when we leave the group --
	  }
	  if(!iLevelSym || !bSkipSym) outStartElement(el,attr);
	  return;
  }

  if(iLevelSym && bSkipSym) return;

  if(GetPathType(el)) {
	  //Path definition, either a sym layer element  or a morphable shape.
	  //Compare the path's bounding box with the view frame --
	  if(iGroupFlg&FLG_LEGEND) {
		  UD(errcode)=outLegendPath(el,attr);
	  }
	  else {
		  UD(errcode)=process_path(attr);
	  }
  }
  else if(isGradient(el)) {
	  bInsideGradient=1;
	  //Check if gradient is actually used --
	  if(attr[0] && !strcmp(attr[0],"id") && id_in_pathstyle(attr[1])) {
		  outStartElement(el,attr); //increments bInsideGradient
	  }
  }
  else {
	  if(!strcmp(el,"text")) {
		  //For now, assume text was originated in Illustrator 10, in which
		  //case we can expect a transform.
		  if(getTransform(transformMatrix,attr,0)) {
			  bInTextElement=1;
			  skip_depth();
			  gz_printf("<text transform=\"matrix(%s)\">",transformMatrix);
		  }
		  //else ignore text element (assume out of frame)
	  }
	  else if(!strcmp(el,"tspan")) {
		  if(bInTextElement==1) {
			  outTspanElement(attr);
			  bInTextElement++;
			  reflen=0;
		  }
		  else if(bInTextElement==2) UD(errcode)=SVG_ERR_TSPAN;
	  }
	  else if(!strcmp(el,"use") && find_style(symname,get_symname(attr))) {
		  if(!GetSymTransform(attr)) return; //Symbol instance out of range
		  //Sets bInUse=TRUE --
		  outStartElement(el,attr);
		  iLevel++;
	  }
	  else {
		  UD(errcode)=SVG_ERR_BADELEMENT;
	  }
  }

_errexit:
  if(UD(errcode)) UD(errline)=XML_GetCurrentLineNumber(parser);
}

static void scanw_endElement(void *userData, LPCSTR el)
{
	if(UD(errcode)) return;

	if(bSymbolScan || !strcmp(el,"g")) {
		if(iLevel>0) {
			if(bSymbolScan && iLevel==2 && !strcmp(el,"g")) return;
			if(!iLevelSym || !bSkipSym) {
				outEndElement(el);
			}
			if(iLevel==iLevelSym) {
			  symcnt++;
			  iLevelSym=0;
			}
			iLevel--;
			if(bSymbolScan) {
				if(iLevel==1) iLevel--;
			}
			else {
				if(iLevel==1 && (iGroupFlg&(FLG_LEGEND+FLG_MASK))) {
					iLevel--;
					//if(iGroupFlg&FLG_LEGEND) UD(errcode)=SVG_ERR_ENDSCAN;
				}
			}
		}
	}
	else if(bInTextElement) {
		if(!strcmp(el,"text")) {
			OUTS("</text>\r\n");
			bInTextElement=0;
		}
		else {
			if(bInTextElement!=2 || strcmp(el,"tspan")) UD(errcode)=SVG_ERR_TSPAN;
			else {
				//At this point, output the text itself and terminate tspan element --
				OUTS(refbuf);
				OUTS("</tspan>");
			}
			bInTextElement=1;
		}
	}
	else if(bInUse) {
		outEndElement(el);
		iLevel--;
		bInUse=FALSE;
	}
	else if(bInsideGradient) {
		if(bNoClose) outEndElement(el);
		else if(isGradient(el)) {
			if(bInsideGradient==2) outEndElement(el);
			bInsideGradient=0;
		}
	}
}

static void scan_chardata(void *userData,const XML_Char *s,int len)
{
	if(bInTextElement==2) {
		if(reflen+len>=SIZ_REFBUF) {
		  UD(errcode)=SVG_ERR_SIZREFBUF;
		  UD(errline)=XML_GetCurrentLineNumber(parser);
		  return;
		}
		memcpy(refbuf+reflen,s,len);
		refbuf[reflen+=len]=0;
	}
}

static int outMergedContent()
{
	/*
	Here we perform the merge, doing a nested parse of the
	external SVG containing graphics elements. We'll output
	the content of the following layers in order:

    w2d_Symbols (predefined symbols in merged file -- AI11 bug workaround)
	w2d_Mask  (optional mask layer)
	w2d_Detail ("Detail" layer in Walls2D)
		w2d_Details_shp (paths to be morphed)
		w2d_Details_sym (paths, text, and use elements to be translated)
	w2d_Walls ("Walls" layer in Walls2D)
		w2d_Walls_shp (paths to be morphed)
		w2d_Walls_sym (paths, text, and use elements to be translated)
    outWallsData() -- ouput Walls survey layers:
		w2d_Vectors, w2d_Flags, w2d_Labels, w2d_Notes, w2d_Grid
	w2d_Legend ("Legend" layer: objects whose world coordinates are fixed)

	In all but the w2d_Legend layer, the world coordinates of objects
	will change if the positions of nearby survey vectors have changed.
	Of course, the page (not world) coordinates of all content might
	change to accommodate a new map scale or view.
	*/

	if(!bSymbolScan && (flags&SVG_ADJUSTABLE)) {
		outSymInstances();
	}

	if(gzseek(fpmrg,0,SEEK_SET)) return SVG_ERR_MRGOPEN;

	iLevel=0;
	pathcnt=symcnt=0;
	bInTextElement=bInUse=bInsideGradient=0;
	tpoints=0;
	max_dsquared=0.0;
	bMaskPathSeen=bLegendSeen=FALSE;

	mrgparser=XML_ParserCreate(NULL);
	if(!mrgparser) return SVG_ERR_NOMEM;
	XML_SetElementHandler(mrgparser,scanw_startElement,scanw_endElement);
	XML_SetCharacterDataHandler(mrgparser,scan_chardata);
	return parse_file(mrgparser,fpmrg);
}

static BOOL get_mskname(LPCSTR name)
{
	if(!name) {
		iMaskName[msklvl]=iMaskName[msklvl-1];
		return TRUE;
	}

	if(nMaskName==maxMaskName) {
		pMaskName=(SHP_MASKNAME *)realloc(pMaskName,sizeof(SHP_MASKNAME)*(maxMaskName+=MASKNAME_INC));
		if(!pMaskName) return FALSE;
	}

	char *p=pMaskName[nMaskName].name;
	LPCSTR psrc=name;
	size_t len=0;

	for(;*psrc && len<SHP_LEN_MASKNAME;len++) {
		if(msklvl && *psrc=='_' && strlen(psrc)>6 && psrc[6]=='_' && !memcmp(psrc+1,"x00",3)) {
			short b;
			if(1==sscanf(psrc+4,"%02hx",&b)) {
				*p++=(char)b;
				psrc+=7;
				continue;
			}
		}
		*p=*psrc++;
		if(!msklvl) {
			if(*p=='.') break; //truncate file extension
		}
		else if(*p=='_') *p=' ';
		p++;
	}
	if(len<SHP_LEN_MASKNAME) memset(p,' ',SHP_LEN_MASKNAME-len);
	pMaskName[nMaskName].iParent=msklvl?iMaskName[msklvl-1]:-1;
	iMaskName[msklvl]=nMaskName++;
	return TRUE;
}

static int alloc_ntw_rec(void)
{
	if(nNtwRec==maxNtwRec) {
		pNtwRec=(NTW_RECORD *)realloc(pNtwRec,(maxNtwRec+=MAXNTWREC_INC)*sizeof(NTW_RECORD));
		if(!pNtwRec) return SVG_ERR_NOMEM;
	}
	pNtwRec[nNtwRec].iNam=iMaskName[msklvl];
	pNtwRec[nNtwRec++].iPly=pathcnt;
	return 0;
}

static void scanmask_startElement(void *userData, LPCSTR el, LPCSTR *attr)
{
  if(UD(errcode)) return;

  if(iLevel<1) {
	if(!strcmp(el,"g") && attr[0] && !strcmp(attr[0],"id")) {
		GetGroupFlg(attr[1]);
		if(iGroupID==ID_MASK) {
			iLevel=1;
			msklvl=0;
			bOuterSeen[0]=0;
			if(!get_mskname(trx_Stpnam(pSVG->mrgpath))) UD(errcode)=SVG_ERR_NOMEM;
		}
		else if(iGroupFlg&(FLG_VECTORS|FLG_ADJUSTED)) {
			UD(errcode)=SVG_ERR_ENDSCAN;
		}
	}
	return;
  }

  //In case mask layer has grouping --
  if(!strcmp(el,"g")) {
	  iLevel++;
	  if(++msklvl==MAX_MSKLVL) {
		  UD(errcode)=SVG_ERR_GROUPLVL;
		  goto _error;
	  }
	  bOuterSeen[msklvl]=0;
	  if(!get_mskname((attr[0] && !strcmp(attr[0],"id"))?attr[1]:NULL)) {
		  UD(errcode)=SVG_ERR_NOMEM;
		  goto _error;
	  }
	  return;
  }

  if(GetPathType(el)) {
	  //Check fill color of this path to determine floor status and background status.
	  //The first filled path determines the floor color. Also, the first path in
	  //a group will be flagged as an outline (outer) path, in which case a new shapefile
	  //record is established consisting of an outer polygon followed by zero or more
	  //inner polygons.

	  COLORREF clr=GetFillColor(attr);
	  
	  if(clr!=(COLORREF)-1) {
		  if(cPassageClr==(COLORREF)-1) cPassageClr=clr;
		  if(cPassageClr==clr) pnPly[pathcnt].flgcnt |= NTW_FLG_FLOOR;
		  if(cBackClr==clr) pnPly[pathcnt].flgcnt |= NTW_FLG_BACKGROUND;
	  }
	  else {
		  pnPly[pathcnt].flgcnt |= NTW_FLG_NOFILL;
	  }
	  pnPly[pathcnt].clr=clr;
	  //The first path in a group is considered an outline path --
	  if(!bOuterSeen[msklvl]) {
		  pnPly[pathcnt].flgcnt |= NTW_FLG_OUTER;
		  bOuterSeen[msklvl]++;
		  if(UD(errcode)=alloc_ntw_rec()) goto _error;
	  }
	  UD(errcode)=process_path(attr);
  }
  else UD(errcode)=SVG_ERR_MASKNONPATH;

_error:
  if(UD(errcode)) UD(errline)=XML_GetCurrentLineNumber(parser);
}

static void scanmask_endElement(void *userData, LPCSTR el)
{
	if(UD(errcode)) return;

	if(!strcmp(el,"g")) {
		if(iLevel>0) {
			iLevel--;
			bOuterSeen[--msklvl]=0;
			if(!(iGroupFlg&FLG_MASK) ||(!iLevel && (pathcnt!=max_pathcnt || ntw_count!=(int)tpoints))) {
				UD(errcode)=SVG_ERR_UNKNOWN;
			}
		}
	}
}

static int write_ntw(void)
{
	bWriting=TRUE;

	if(gzseek(fpmrg,0,SEEK_SET)) return SVG_ERR_MRGOPEN;

	max_pathcnt=pathcnt;
	pathcnt=0;
	tpoints=0;
	iLevel=0;
	max_dsquared=0.0;
	ntw_count=ntw_size=nMaskOpenPaths=nNtwRec=maxNtwRec=nMaskName=maxMaskName=0;
	bAllocErr=FALSE;
	cPassageClr=(COLORREF)-1;

	if(!(pnPly=(NTW_POLYGON *)calloc(max_pathcnt*sizeof(NTW_POLYGON),1))) {
		return SVG_ERR_NOMEM;
	}

	if(bAdjust) {
		tppcnt=0;
		node_visits=0;
		init_veclst();
	}

	int i=SVG_ERR_NOMEM;
	mrgparser=XML_ParserCreate(NULL);

	if(mrgparser) {
		XML_SetElementHandler(mrgparser,scanmask_startElement,scanmask_endElement);
		//tm_walls=clock();
		i=parse_file(mrgparser,fpmrg);
		//tm_walls=clock()-tm_walls;

		if(!i) i=write_polygons();
		if(!i && nMaskOpenPaths>0) i=-SVG_ERR_MASKPATHSOPEN;
	}
	return i;
}

int init_vnod(double xM,double yM)
{
	vnod_siz=trx.NumRecs()+NUM_HDR_NODES;
	if(!(vnod=(SVG_VECNODE *)calloc(vnod_siz,sizeof(SVG_VECNODE))) ||
	   !(vstk=(int *)malloc(vnod_siz*sizeof(int)))) return SVG_ERR_NOMEM;

	//Initialize 7 tree header nodes --

	for(int n=0;n<NUM_HDR_NODES;n++) {
		vnod[n].M_xy[0]=vnod[n].M_xy[1]=0.0;
	}
	SVG_VECNODE *pn=vnod;
	pn->B_xy[0]=xM;
	pn->B_xy[1]=yM;
	pn->lf=1; pn->rt=2;
	pn++; //1
	pn->B_xy[0]=0.5*xM;
	pn->B_xy[1]=yM;
	pn->lf=3; pn->rt=4;
	pn++; //2
	pn->B_xy[0]=1.5*xM;
	pn->B_xy[1]=yM;
	pn->lf=5; pn->rt=6;
	pn++; //3
	pn->B_xy[0]=0.5*xM;
	pn->B_xy[1]=0.5*yM;
	pn->lf=pn->rt=-1;
	pn++; //4
	pn->B_xy[0]=0.5*xM;
	pn->B_xy[1]=1.5*yM;
	pn->lf=pn->rt=-1;
	pn++; //5
	pn->B_xy[0]=1.5*xM;
	pn->B_xy[1]=0.5*yM;
	pn->lf=pn->rt=-1;
	pn++; //6
	pn->B_xy[0]=1.5*xM;
	pn->B_xy[1]=1.5*yM;
	pn->lf=pn->rt=-1;
	vnod_len=7;
	return 0;
}

static int parse_georef()
{
	LPCSTR p;
	//Tamapatz Project  Plan: 90  Scale: 1:75  Frame: 17.15x13.33m  Center: 489311.3 2393778.12
	if(reflen && (p=strstr(refbuf," Plan: "))) {
		char cUnits;
		int numval=sscanf(p+7,"%lf Scale: 1:%lf Frame: %lfx%lf%c Center: %lf %lf",
			&mrgViewA,&mrgScale,&xMrgMid,&yMrgMid,&cUnits,&mrgMidE,&mrgMidN);
		if(numval==7 && mrgScale>0.0) {
			mrgViewA*=U_DEGREES;
			mrgSinA=sin(mrgViewA);
			mrgCosA=cos(mrgViewA);
			if(cUnits=='f') {
				xMrgMid*=0.3048;
				yMrgMid*=0.3048;
				mrgMidE*=0.3048;
				mrgMidN*=0.3048;
			}
			mrgScale=(72*12.0/0.3048)/mrgScale;  //page pts per world meter
			xMrgMid*=(0.5*mrgScale);
			yMrgMid*=(0.5*mrgScale);

			init_utm_to_mrgxy();

			if(bNTW) {
				//not necessary? 
				scale=mrgScale;

				ntw_hdr.scale=1.0/(1000*mrgScale); //world meters per file unit
				ntw_hdr.datumE=mrgMidE;	   //world East coordinate of (0,0)
				ntw_hdr.datumN=mrgMidN;	   //world North coordinate of (0,0)
			}
			else init_xy_to_mrgxy();

			scaleRatio=scale/mrgScale;

			min_veclen_sq=mrgScale*pSVG->advParams.len_min;
			min_veclen_sq*=min_veclen_sq;
			//Could conceivably have multiple vector groups, so initialize now --
			return init_vnod(xMrgMid,yMrgMid);
		}
	}
	return SVG_ERR_REFFORMAT;
}

static double getFontSize(LPCSTR p)
{
	if((p=strstr(p,st_font_size))) return atof(p+10);
	return -1.0;
}

static char *getLabelStyle(double *pSize,LPCSTR *attr)
{
	int i=GetStyleAttr(attr);
	if(i) {
	  *pSize=getFontSize(attr[i]);
	  //I think we can assume that either tspan or text has the complete style spec.
	  //If fLabelSize>0 then the remaining relevant text attributes should also be here --
	  if(bUseStyles && *pSize>0.0) return _strdup(attr[i]);
	}
	return NULL;
}

static void setLabelPosition(LPCSTR *attr)
{
	//Station label or note ID is in index at current position. Mark it "present"
	//and replace the vector endpoint page coordinates with the label's or note's
	//offset with respect to this endpoint.
	SVG_PATHPOINT pt;

	if(getPathCTM_Pt(attr,&pt) || getOffset(attr,&pt)) {
		float *px;
		if(iGroupID==ID_NOTES) {
			labelRec.bPresent|=2;
			px=&labelRec.nx;
		}
		else {
			labelRec.bPresent|=1;
			px=&labelRec.sx;
		}
		//Now store label's offsets wrt to endpoint in final map units --
		px[0]=(float)(scaleRatio*(pt.x-px[0]));
		px[1]=(float)(scaleRatio*(pt.y-px[1]));
		trx.UseTree(SVG_TRX_LABELS);
		trx.PutRec(&labelRec);
		trx.UseTree(SVG_TRX_VECTORS);
	}
}

static void scanvecs_startElement(void *userData, LPCSTR el, LPCSTR *attr)
{
  if(UD(errcode)) return;

  if(!iLevel) {
	//We are not already in a recognized group -- 
	if(!strcmp(el,"g") && attr[0] && !strcmp(attr[0],"id")) {
		GetGroupFlg(attr[1]);
		if(iGroupID==ID_REF) {
			iLevel=-1;
		}
		else {
			if(iGroupFlg&(FLG_VECTORS|FLG_MASK|FLG_BACKGROUND)) {
				if(!(iGroupFlg&FLG_BACKGROUND) && mrgScale==0.0) {
					UD(errcode)=SVG_ERR_NOREF;
					UD(errline)=XML_GetCurrentLineNumber(parser);
				}
				else iLevel=1;
			}
		}
	}
	return;
  }

  if(iLevel<0) {
	  //processing reference --
	  if(iLevel<-1) return;
	  if(!strcmp(el,"text")) {
		  //now look for character data --
		  iLevel=-2;
		  bInTextElement=2;
		  reflen=0;
	  }
	  else UD(errcode)=SVG_ERR_NOTEXT;
  }
  else {
	  //processing vectors, or mask  --
	  if(!strcmp(el,"g")) {
		  iLevel++;
		  return;
	  }
	  if(iGroupFlg&FLG_VECTORS) {
		//Path definition --
		UD(errcode)=addto_vnode(attr);
	  }
	  else {
		  if(!GetPathType(el)) return;
		  if(iGroupFlg&FLG_MASK) UD(errcode)=process_path(attr);
		  else {
			  //Must be path in background. Assume last one is rock color --
			  cBackClr=GetFillColor(attr);
		  }
		  return;
	  }
  }

  if(UD(errcode)) UD(errline)=XML_GetCurrentLineNumber(parser);
}

static void scanvecs_endElement(void *userData, LPCSTR el)
{
  if(UD(errcode)) return;

  //iLevel can be -1,-2 (Ref) or >0 (Walls or Vectors)
  if(!iLevel) return;

  if(iLevel>0) {
	  if(!strcmp(el,"g")) {
		  iLevel--;
		  if(!iLevel && (iGroupFlg&FLG_VECTORS)) UD(errcode)=SVG_ERR_ENDSCAN;
	  }
  }
  else if(!strcmp(el,"text")) {
	  int e=parse_georef();
	  if(e) {
		  UD(errcode)=e;
		  UD(errline)=XML_GetCurrentLineNumber(parser);
	  }
	  iLevel=0;
	  bInTextElement=0;
  }
}

static void scanv_startElement(void *userData, LPCSTR el, LPCSTR *attr)
{
  int i;

  if(UD(errcode)) return;

  if(!iLevel) {
	//We are not already in a recognized group -- 
	if(!strcmp(el,"g") && attr[0] && !strcmp(attr[0],"id")) {
		if(!GetGroupFlg(attr[1])) {
			UD(errcode)=SVG_ERR_W2DNAME;
			UD(errline)=XML_GetCurrentLineNumber(parser);
			return;
		}
		if(iGroupID==ID_REF) {
			iLevel=-1;
		}
		else {
			UINT target=FLG_VECTORS+FLG_MASK+FLG_BACKGROUND;
			if(flags&SVG_DETAIL) target|=(FLG_DETAIL_SHP+FLG_DETAIL_SYM);
			if(flags&SVG_WALLS) target|=(FLG_WALLS_SHP+FLG_WALLS_SYM);
			if(flags&SVG_LEGEND) target|=FLG_LEGEND;
			if(bUseStyles) {
				target|=(FLG_FRAME+FLG_LABELS);
				if(flags&SVG_NOTES) target|=FLG_NOTES;
				if(flags&SVG_FLAGS) target|=FLG_FLAGS;
				if(flags&SVG_GRID) target|=FLG_GRID;
				if(flags&SVG_VECTORS) {
					target|=FLG_MARKERS;
					if(pSVG->advParams.uFlags&SVG_ADV_LRUDBARS) target|=FLG_LRUDS;
				}
			}
			if(bUseLabelPos) {
				if(flags&SVG_LABELS) target|=FLG_LABELS;
				if(flags&SVG_NOTES) target|=FLG_NOTES;
			}
			if(iGroupFlg&target) {
				if(mrgScale==0.0 && (iGroupFlg&(FLG_BACKGROUND+FLG_GRID))==0) {
					UD(errcode)=SVG_ERR_NOREF;
					UD(errline)=XML_GetCurrentLineNumber(parser);
				}
				else {
					iLevelSym=0;
					iLevel=1;
					iLevelID=0;
				}
			}
		}
	}
	return;
  }

  if(iLevel<0) {
	  //processing reference --
	  if(iLevel<-1) {
		  if(fTitleSize<=0.0 && !strcmp(el,"tspan") && (i=GetStyleAttr(attr)))
			  fTitleSize=getFontSize(attr[i]);
		  return;
	  }
	  if(!strcmp(el,"text")) {
		  if(bUseStyles && fTitleSize<=0.0 && (i=GetStyleAttr(attr)))
			  fTitleSize=getFontSize(attr[i]);
		  //now look for character data --
		  iLevel=-2;
		  bInTextElement=2;
		  reflen=0;
	  }
	  else UD(errcode)=SVG_ERR_NOTEXT;
  }
  else {
	  //processing vectors, shapes (detail, walls, or mask), labels, or legend  --
	  if(!strcmp(el,"g")) {
		  iLevel++;
		  if(!iLevelSym && (iGroupFlg&FLG_WALLS_DETAIL_MSK) && (!attr[0] || strcmp(attr[0],"id"))) {
			  //unnamed group -- let's start calculating bounding box in symrange[] --
			  iLevelSym=iLevel;
			  if(symcnt>=max_symcnt) {
				  symrange=(SVG_PATHRANGE *)realloc(symrange,(max_symcnt+=MAX_SYMCNT_INC)*sizeof(SVG_PATHRANGE));
				  if(!symrange) {
					  UD(errcode)=SVG_ERR_NOMEM;
					  goto _eret;
				  }
			  }
			  symrange[symcnt].xmax=symrange[symcnt].ymax=-FLT_MAX;
			  symrange[symcnt].xmin=symrange[symcnt].ymin=FLT_MAX;
			  //Increment symcnt when we leave the group --
		  }
		  else {
			  if(bUseLabelPos && (iGroupFlg&(FLG_LABELS+FLG_NOTES))!=0 &&
				!iLevelID && attr[0] && !strcmp(attr[0],"id")) {
				//Station name ID should already be in index, store record in labelRec --
				if(getLabelRec(attr[1])) iLevelID=iLevel;
			  }
		  }
		  return;
	  }
	  if(iGroupFlg&FLG_VECTORS) {
			//Path or line definition --
			UD(errcode)=addto_vnode(attr);
	  }
	  else if(iGroupFlg&FLG_ADJUSTED) {
		  //This is either a path, text, or tspan element (not in the w2d_Legend layer).

		  //Illustrator 10 apparently saves all text objects as tspans with an associated style.
		  //For now we'll assume this and ignore the enclosing <text> element --

		  if(!strcmp(el,"text")) {
			  if(iLevelSym) updateSymRange(attr);
			  return;
		  }
		  if(!strcmp(el,"tspan")) {
			  //save text styles for entity definitions --
			  if(i=GetStyleAttr(attr)) save_style(textstyle,attr[i]);
			  return;
		  }
		  if(!strcmp(el,"use")) {
			  //Save symbol ID --
			  if(iGroupFlg&FLG_WALLS_DETAIL_MSK) {
				  save_symname(attr);
				  if(iLevelSym) updateSymRange(attr);
			  }
			  return;
		  }
		  if(GetPathType(el)) {
			  UD(errcode)=process_path(attr);
		  }
	  }
	  else if(iGroupFlg&FLG_LEGEND) {
		  if(!strcmp(el,"use")) {
			  //Save symbol ID (no style assigned here by Illustrator --
			  save_symname(attr);
			  return;
		  }
		  if(i=GetStyleAttr(attr)) save_style(legendstyle,attr[i]);
		  //Determine position of legend block from position of first text element --
		  if(!pLegendPt && !strcmp(el,"text")) {
			  pLegendPt=getPathCTM_Pt(attr,&legendPt);
		  }
	  }
	  else if(iGroupFlg&FLG_STYLE_MSK) {
		  if(iGroupFlg&(FLG_LABELS+FLG_NOTES)) {
			  //Relevant style can be either in the <text> or <tspan> element --
			  BOOL bTyp=strcmp(el,"text")==0;
			  if(!bTyp) bTyp=2*(strcmp(el,"tspan")==0);
			  if(!bTyp) return;
			  if(iGroupID==ID_NOTES) {
				  if(fNoteSize<=0.0 && (pNoteStyle=getLabelStyle(&fNoteSize,attr)) &&
					  !bUseLabelPos) iLevel=0;
			  }
			  else if(fLabelSize<=0.0 && (pLabelStyle=getLabelStyle(&fLabelSize,attr)) &&
				  !bUseLabelPos) iLevel=0;
			  if(iLevel && bTyp==1 && iLevelID) {
				  //Assume transform is on text element --
				  setLabelPosition(attr);
				  iLevelID=0;
			  }
		  }
		  else if(iGroupFlg&(FLG_FLAGS+FLG_MARKERS)) {
			  if(!strcmp(el,"use")) {
				  if(iGroupID==ID_FLAGS) {
					  i=save_symname(attr);
					  if(i<=MAX_STYLES && !flagscale[i]) {
						  //Get flag scale override from transform --
						  if(el=getTransformMatrixPtr(attr)) {
							  flagscale[i]=(float)atof(el);
						  }
					  }
				  }
				  else if(fMarkerThick<=0.0 && (el=getTransformMatrixPtr(attr)))
					  fMarkerThick=atof(el);
			  }
		  }
		  else if(i=GetStyleAttr(attr)) {
			  COLORREF *pc=NULL;
			  LPCSTR pTyp;
			  switch(iGroupID) {
				case ID_BACKGROUND: {
									pc=&cBackClr; pTyp=st_fill; break;
									}

				case ID_LRUDS:		{
									pc=&cLrudClr; pTyp=st_stroke; break;
									}

				case ID_GRID:		if(numGridThick<2) {
										el=getStyleValPtr(attr[i],st_stroke_width);
										fGridThick[numGridThick]=el?atof(el):1.0;
										if(numGridThick==1 && fGridThick[0]!=fGridThick[1]) {
											numGridThick++;
											if(fGridThick[0]>fGridThick[1]) {
												double d=fGridThick[0];
												fGridThick[0]=fGridThick[1];
												fGridThick[1]=d;
											}
											if(cGridClr>=0) iLevel=0;
										}
									}
									pc=&cGridClr;
									pTyp=st_stroke;
									break;

				case ID_FRAME: 		if(el=getStyleValPtr(attr[i],st_stroke_width)) {
					  					fFrameThick=(xMid/xMrgMid)*atof(el);
										if(fFrameThick<0.01) fFrameThick=0.01;
									}
									else fFrameThick=1.0;
									iLevel=0;
									break;
			  }
			  if(pc && *pc==(COLORREF)-1) {
				  if((*pc=getColorAttr(attr[i],pTyp))==(COLORREF)-1) {
					  if(iGroupID==ID_BACKGROUND) {
						  cBackClr=0xFFFFFF; //Same as "none"
						  iLevel=0;
					  }
				  }
				  else if(iGroupID!=ID_GRID) iLevel=0;
			  }
		  }
	  }
  }
_eret:
  if(UD(errcode)) UD(errline)=XML_GetCurrentLineNumber(parser);
}

static void scanv_endElement(void *userData, LPCSTR el)
{
  if(UD(errcode)) return;

  //iLevel can be -1,-2 (Ref) or >0 (Walls or Vectors)
  if(!iLevel) return;

  if(iLevel>0) {
	  if(!strcmp(el,"g")) {
		  if(iLevel==iLevelSym) {
			  symcnt++;
			  iLevelSym=0;
		  }
		  iLevel--;
		  iLevelID=0;
	  }
  }
  else if(!strcmp(el,"text")) {
	  int e=parse_georef();
	  if(e) {
		  UD(errcode)=e;
		  UD(errline)=XML_GetCurrentLineNumber(parser);
	  }
	  iLevel=0;
	  bInTextElement=0;
  }
}

static int scan_merge_file(void)
{
	//Process w2d_Ref and w2d_Vector groups in merged file.
	//Obtain array pathrange[] for paths in w2d_Walls and w2d_Detail.
	//Obtain arrays pathstyles[] and textstyles[] for paths and tspans in w2d_Walls and w2d_Detail.
	//Obtain style for first tspan element in w2d_Labels so that they can be rescaled.
	//Obtain sizes for symbols found in w2d_Flags if we're inheriting styles
	//Also look for <use> elements, saving any symbol IDs.

	int i;

	if(!(fpmrg=gzopen(pSVG->mrgpath,"rb"))) return SVG_ERR_MRGOPEN;

    veccnt=skipcnt=max_slevel=0;
	//Attributes possibly optained from merge file elements --
	fLabelSize=fNoteSize=fTitleSize=-1.0;
	cMarkerClr=cGridClr=cBackClr=cPassageClr=cLrudClr=(COLORREF)-1;
	memset(flagscale,0,sizeof(flagscale));
	iLevel=0;
	pathcnt=max_pathcnt=symcnt=max_symcnt=0;
	tpoints=tpoints_w=tpoints_d=0;
	flength_w=flength_d=0.0;
	mrgScale=0.0;
	bAdjust=bMaskPathSeen=FALSE;
	max_diffx=max_diffy=0.0;
	pLegendPt=NULL;

	mrgparser=XML_ParserCreate(NULL);
	if(!mrgparser) return SVG_ERR_NOMEM;

	XML_SetElementHandler(mrgparser,scanv_startElement,scanv_endElement);
	XML_SetCharacterDataHandler(mrgparser,scan_chardata);
	//NOTE: We will parse general entities --
	//tm_scan=clock();
	if((i=parse_file(mrgparser,fpmrg))) return i;
	//tm_scan=clock()-tm_scan;

	if(fLabelSize<=0.0) fLabelSize=pSVG->labelSize/10.0;
	if(fNoteSize<=0.0) fNoteSize=pSVG->noteSize/10.0;
	if(fTitleSize<=0.0) fTitleSize=pSVG->titleSize/10.0;
	if(cMarkerClr==(COLORREF)-1) cMarkerClr=pSVG->markerColor;
	if(cBackClr==(COLORREF)-1 || !bUseStyles) cBackClr=pSVG->backColor;
	if(cPassageClr==(COLORREF)-1 || !bUseStyles) cPassageClr=pSVG->passageColor;
	if(cGridClr==(COLORREF)-1) cGridClr=pSVG->gridColor;
	if(cLrudClr==(COLORREF)-1) cLrudClr=pSVG->lrudColor;
	if(!veccnt) return SVG_ERR_NOVECTORS;
	if(veccnt<vecs_used) vecs_used=veccnt;
	//Flag to indicate if feature adjustment is required --
	double fmax=pSVG->advParams.maxCoordChg*mrgScale;
	if(bAdjust=(max_diffx>fmax || max_diffy>fmax)) {
	  pSVG->fMaxDiff=(float)(max(max_diffx,max_diffy)/mrgScale);
	}
	pSVG->flength_w=(float)(flength_w/mrgScale);
	pSVG->flength_d=(float)(flength_d/mrgScale);
	pSVG->tpoints_w=tpoints_w;
	pSVG->tpoints_d=tpoints_d;

	//Now let's see which paths and path styles will need to be included --
	memset(pathstyle_used,0,sizeof(pathstyle_used));
	for(i=0;i<(int)pathcnt;i++) {
	  if(check_prange(&pathrange[i])) pathstyle_used[pathrange[i].pathstyle_idx]=1;
	  else pathrange[i].pathstyle_idx=-1;
	}

	return 0;
}

static int scan_ntw_vectors(void)
{
	//Process w2d_Ref and w2d_Vector groups in SVG source file.

	int i;

	if(!(fpmrg=gzopen(pSVG->mrgpath,"rb"))) return SVG_ERR_MRGOPEN;

    veccnt=skipcnt=max_slevel=0;

	cBackClr=(COLORREF)-1;
	iLevel=0;
	pathcnt=0;
	mrgScale=0.0;
	bAdjust=FALSE;
	max_diffx=max_diffy=0.0;
	tpoints=0;

	mrgparser=XML_ParserCreate(NULL);
	if(!mrgparser) return SVG_ERR_NOMEM;

	XML_SetElementHandler(mrgparser,scanvecs_startElement,scanvecs_endElement);
	XML_SetCharacterDataHandler(mrgparser,scan_chardata);
	//NOTE: We will parse general entities --
	//tm_scan=clock();
	if((i=parse_file(mrgparser,fpmrg))) return i;
	//tm_scan=clock()-tm_scan;

	if(!(max_pathcnt=pathcnt)) return SVG_ERR_NOMASK;

	if(!veccnt) return SVG_ERR_NOVECTORS;
	if(veccnt<vecs_used) vecs_used=veccnt;

	//Flag to indicate if feature adjustment is required --
	double fmax=pSVG->advParams.maxCoordChg*mrgScale;

	bAdjust=(max_diffx>fmax || max_diffy>fmax);

	return 0;
}

//=====================================================================================================
//Main entry point --
DLLExportC int PASCAL Export(UINT uFlags,LPFN_EXPORT_CB wall_GetData)
{
	int e;

	pGetData=wall_GetData;
	flags=uFlags;
	if(wall_GetData(SHP_VERSION,&pSVG)!=EXP_SVG_VERSION) return SVG_ERR_VERSION;

	fpout=NULL;
	gzout=NULL;
	bWriting=FALSE;
	fpin=fpmrg=NULL;
	vstk=NULL; vnod=NULL;
	pathrange=symrange=NULL;
	pathstyle[0]=textstyle[0]=legendstyle[0]=symname[0]=NULL;
	parser_ErrorLineNumber=0;
	pLabelStyle=pNoteStyle=NULL;
	pnPly=NULL;

	bNTW=(flags&SVG_WRITENTW)!=0;

	if(bNTW) {
		pNtwRec=NULL;
		pMaskName=NULL;
		pNTWp=NULL; pNTWf=NULL;
		dist_exp=pSVG->advParams.dist_exp;
		dist_off=pSVG->advParams.dist_off;
		len_exp=pSVG->advParams.len_exp;
		vecs_used=pSVG->advParams.vecs_used;
		scale=0.0; //just in case
		flags|=SVG_MERGEDCONTENT;
		bUseLabelPos=FALSE; //accessed in createIndex
		if(e=createIndex()) return finish(e);
		if(e=scan_ntw_vectors()) return finish(e);
		tptotal=tpoints; //for calculating percent progress.
		e=write_ntw();
		return finish(e);
	}

	if(flags&SVG_EXISTS) {
		pSVG->flags&=~(SVG_EXISTS+SVG_UPDATED+SVG_ADDITIONS+SVG_EXISTADJUSTABLE);
		pSVG->version=0;
		e=finish(scan_svgheader());
		if(!e) pSVG->version=EXP_SVG_VERSION;
		return e;
	}
	init_constants();

	if(e=createIndex()) return finish(e);

	if(flags&SVG_MERGEDCONTENT) {
		//Scan the SVG to be merged, processing w2d_Ref, w2d_Vector, w2d_Walls, w2d_Detail,
		//and w2d_Mask groups, building searchable binary tree of vectors, vnod[], and an
		//array, pathrange[], of shape or wall segment ranges. We also determine the location
		//of the first w2d_Legend text object, save names of referenced symbols, and check
		//if a marker symbol (_m) and marker size entity (_M) is defined.
		if(e=scan_merge_file()) return finish(e);
		tptotal=tpoints; //for calculating percent progress.
	}

 	if(flags&SVG_FLAGS) {
		//Need flag info to define symbols in output SVG --
		if(e=GetFlagData()) return finish(e);
	}

	if(flags&SVG_NOTES) {
		//Determine if note entities need to be defined --
		getNumNoteLocations();
	}

	//Write the SVG file while parsing the XML template and also
	//parsing again, if necessary, the SVG to be merged --

	e=write_svg();

	return finish(e);
}

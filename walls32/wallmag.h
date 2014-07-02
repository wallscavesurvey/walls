/*wallmag.h*/

#ifndef __WALLMAG_H
#define __WALLMAG_H

//====================================================================
#pragma pack(1)
typedef float WMM_ARRAY[13][13];

typedef struct {
	float epoch;
	WMM_ARRAY c,cd;
} WMM_COFDATA;
#pragma pack()

extern WMM_COFDATA *pWMM;

typedef struct {
	float lat;
	float lon;
	float altm;
	float year;
	float dec,decMinYr;
	float dip,dipMinYr;
	float ti,tiNtYr;
	float hi,hiNtYr;
	float x,xNtYr;
	float y,yNtYr;
	float z,zNtYr;
} WMM_MAG_DATA;

extern WMM_MAG_DATA wmm;
extern double mag_HorzIntensity;

void wmm_Init(float epoch,WMM_ARRAY *pc,WMM_ARRAY *pcd);
int wmm_Calc(WMM_MAG_DATA *wmm);

//====================================================================

#define MAG_ERR_INIT		1
#define MAG_ERR_VERSION		2
#define MAG_ERR_DATERANGE	3
#define MAG_ERR_ELEVRANGE	4
#define MAG_ERR_CVTUTM2LL	5
#define MAG_ERR_OUTOFZONE	6
#define MAG_ERR_TRANSLATE	7
#define MAG_ERR_LLRANGE		8
#define MAG_ERR_ZONERANGE   9

typedef struct {
	double fdeg;
	double fmin;
	double fsec;
	int isgn;
	int ideg;
	int imin;
	int isec;	/*not used*/
} DEGTYP;

typedef struct {
	double north;
	double east;
	double north2;
	double east2;
	DEGTYP lat,lon;
	float decl;
	float conv;
	char *modname;
	int modnum;
	int zone;
	int zone2;
	int	datum;
	int	m,d,y;
	int elev;
	int format;
	int fcn;
} MAGDATA;

#define MAG_VERSION			2

#define MAG_GETVERSION		0
#define MAG_GETDATUMCOUNT	1
#define MAG_GETDATUMNAME	2
#define MAG_GETNAD27		3
#define MAG_CVTUTM2LL	 	4
#define MAG_CVTUTM2LL2	 	5
#define MAG_CVTUTM2LL3	 	6
#define MAG_CVTLL2UTM		7
#define MAG_CVTDATE			8
#define MAG_FIXFORMAT		9
#define MAG_CVTLL2UTM2		10
#define MAG_CVTLL2UTM3		11
#define MAG_GETUTMZONE		12
#define MAG_GETDECL			13
#define MAG_CVTDATUM		100
#define MAG_CVTDATUM2		200

/*
#define MAG_DLL_NAME "WALLMAG.DLL"
#define MAG_DLL_GETDATA "_GetData@4"
typedef int (FAR PASCAL *LPFN_MAG_GETDATA)(MAGDATA FAR *pD);
*/

//In wallmag.cpp --
int mag_GetData(MAGDATA *pD);
int mag_InitModel();
double getDEG(DEGTYP *p,int format);
void getDMS(DEGTYP *p,double val,int format);
int ComputeDecl(MAGDATA *pD,double lat,double lon);

//For convenience --
extern MAGDATA app_MagData;
#define MAX_YEAR 2100
#define MIN_YEAR 1899
#define MD app_MagData
#define MF(f) (MD.fcn=f,mag_GetData(&MD))
#define MF_NAD27() MF(MAG_GETNAD27)
#define MF_WGS84() (MF(MAG_GETDATUMCOUNT)-1)

#endif
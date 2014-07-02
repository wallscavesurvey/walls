// wgeomag.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <wallmag.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define epochrange 5.0f
#define DEG_TO_RAD ((float)(3.14159265359/180.0))

#define MAXORD_ 12


#define  A_CONST 6378.137f
#define  B_CONST 6356.7523142f
#define  RE_CONST  6371.2f
#define  A2_CONST (A_CONST*A_CONST)
#define  B2_CONST (B_CONST*B_CONST)
#define  C2_CONST (A2_CONST-B2_CONST)
#define  A4_CONST (A2_CONST*A2_CONST)
#define  B4_CONST (B2_CONST*B2_CONST)
#define  C4_CONST (A4_CONST-B4_CONST)

static float wmm_epoch; //data read
static WMM_ARRAY wmm_c;
static WMM_ARRAY wmm_cd;

/*************************************************************************/

static int D1,D2,D3,D4;
static float fn[13],fm[13],pp[13],k[13][13];
static float sp[13],cp[13],dp[13][13];
static float otime,oalt,olat,olon;
static float snorm[169];
static float *p = snorm;

void wmm_Init(float epoch,WMM_ARRAY *pc,WMM_ARRAY *pcd)
{
  int j,m;
  float flnmj;

  wmm_epoch=epoch;
  memcpy(wmm_c,pc,sizeof(WMM_ARRAY));
  memcpy(wmm_cd,pcd,sizeof(WMM_ARRAY));

/* INITIALIZE CONSTANTS */
  sp[0] = 0.0;
  cp[0] = *p = pp[0] = 1.0;
  dp[0][0] = 0.0;
  
  /* CONVERT SCHMIDT NORMALIZED GAUSS COEFFICIENTS TO UNNORMALIZED */
  *snorm = 1.0;
  for (int n=1; n<=MAXORD_; n++) {
      *(snorm+n) = *(snorm+n-1)*(float)(2*n-1)/(float)n;
      j = 2;
      for (m=0,D1=1,D2=(n-m+D1)/D1; D2>0; D2--,m+=D1)
        {
          k[m][n] = (float)(((n-1)*(n-1))-(m*m))/(float)((2*n-1)*(2*n-3));
          if (m > 0)
            {
              flnmj = (float)((n-m+1)*j)/(float)(n+m);
              *(snorm+n+m*13) = *(snorm+n+(m-1)*13)*sqrt(flnmj);
              j = 1;
              wmm_c[n][m-1] = *(snorm+n+m*13)*wmm_c[n][m-1];
              wmm_cd[n][m-1] = *(snorm+n+m*13)*wmm_cd[n][m-1];
            }
          wmm_c[m][n] = *(snorm+n+m*13)*wmm_c[m][n];
          wmm_cd[m][n] = *(snorm+n+m*13)*wmm_cd[m][n];
        }
      fn[n] = (float)(n+1);
      fm[n] = (float)n;
  }

  k[1][1] = 0.0;
}

/*************************************************************************/

static int E0000(float alt, float glat, float glon, float time, float *dec, float *dip, float *ti, float *gv)
{
  int m;
  static float temp1,temp2;
  static float rlon,rlat,srlon,srlat,crlon,crlat,srlat2,crlat2;
  static float ca,sa,aor,ar,br,bt,bp,bpp;
  static float par,parp,bx,by,bz,bh,r,ct,st;
  static float tc[13][13];

  float dt,q,q1,q2,d,r2;

  dt = time - wmm_epoch;
  if (otime < 0.0 && (dt < 0.0 || dt > 5.0)) return 1;

  rlon = glon*DEG_TO_RAD;
  rlat = glat*DEG_TO_RAD;
  srlon = sin(rlon);
  srlat = sin(rlat);
  crlon = cos(rlon);
  crlat = cos(rlat);
  srlat2 = srlat*srlat;
  crlat2 = crlat*crlat;
  sp[1] = srlon;
  cp[1] = crlon;

/* CONVERT FROM GEODETIC COORDS. TO SPHERICAL COORDS. */
  if (alt != oalt || glat != olat)
    {
      q = sqrt(A2_CONST-C2_CONST*srlat2);
      q1 = alt*q;
      q2 = ((q1+A2_CONST)/(q1+B2_CONST))*((q1+A2_CONST)/(q1+B2_CONST));
      ct = srlat/sqrt(q2*crlat2+srlat2);
      st = (float)sqrt(1.0-(ct*ct));
      r2 = (float)((alt*alt)+2.0*q1+(A4_CONST-C4_CONST*srlat2)/(q*q));
      r = sqrt(r2);
      d = sqrt(A2_CONST*crlat2+B2_CONST*srlat2);
      ca = (alt+d)/r;
      sa = C2_CONST*crlat*srlat/(r*d);
    }
  if (glon != olon)
    {
      for (m=2; m<=MAXORD_; m++)
        {
          sp[m] = sp[1]*cp[m-1]+cp[1]*sp[m-1];
          cp[m] = cp[1]*cp[m-1]-sp[1]*sp[m-1];
        }
    }
  aor = RE_CONST/r;
  ar = aor*aor;
  br = bt = bp = bpp = 0.0;
  for (int n=1; n<=MAXORD_; n++)
    {
      ar = ar*aor;
      for (m=0,D3=1,D4=(n+m+D3)/D3; D4>0; D4--,m+=D3)
        {
/*
   COMPUTE UNNORMALIZED ASSOCIATED LEGENDRE POLYNOMIALS
   AND DERIVATIVES VIA RECURSION RELATIONS
*/
          if (alt != oalt || glat != olat)
            {
              if (n == m)
                {
                  *(p+n+m*13) = st**(p+n-1+(m-1)*13);
                  dp[m][n] = st*dp[m-1][n-1]+ct**(p+n-1+(m-1)*13);
                  goto S50;
                }
              if (n == 1 && m == 0)
                {
                  *(p+n+m*13) = ct**(p+n-1+m*13);
                  dp[m][n] = ct*dp[m][n-1]-st**(p+n-1+m*13);
                  goto S50;
                }
              if (n > 1 && n != m)
                {
                  if (m > n-2) *(p+n-2+m*13) = 0.0;
                  if (m > n-2) dp[m][n-2] = 0.0;
                  *(p+n+m*13) = ct**(p+n-1+m*13)-k[m][n]**(p+n-2+m*13);
                  dp[m][n] = ct*dp[m][n-1] - st**(p+n-1+m*13)-k[m][n]*dp[m][n-2];
                }
            }
        S50:
/*
    TIME ADJUST THE GAUSS COEFFICIENTS
*/
          if (time != otime)
            {
              tc[m][n] = wmm_c[m][n]+dt*wmm_cd[m][n];
              if (m != 0) tc[n][m-1] = wmm_c[n][m-1]+dt*wmm_cd[n][m-1];
            }
/*
    ACCUMULATE TERMS OF THE SPHERICAL HARMONIC EXPANSIONS
*/
          par = ar**(p+n+m*13);
          if (m == 0)
            {
              temp1 = tc[m][n]*cp[m];
              temp2 = tc[m][n]*sp[m];
            }
          else
            {
              temp1 = tc[m][n]*cp[m]+tc[n][m-1]*sp[m];
              temp2 = tc[m][n]*sp[m]-tc[n][m-1]*cp[m];
            }
          bt = bt-ar*temp1*dp[m][n];
          bp += (fm[m]*temp2*par);
          br += (fn[n]*temp1*par);
/*
    SPECIAL CASE:  NORTH/SOUTH GEOGRAPHIC POLES
*/
          if (st == 0.0 && m == 1)
            {
              if (n == 1) pp[n] = pp[n-1];
              else pp[n] = ct*pp[n-1]-k[m][n]*pp[n-2];
              parp = ar*pp[n];
              bpp += (fm[m]*temp2*parp);
            }
        }
    }
  if (st == 0.0) bp = bpp;
  else bp /= st;
/*
    ROTATE MAGNETIC VECTOR COMPONENTS FROM SPHERICAL TO
    GEODETIC COORDINATES
*/
  bx = -bt*ca-br*sa;
  by = bp;
  bz = bt*sa-br*ca;
/*
    COMPUTE DECLINATION (DEC), INCLINATION (DIP) AND
    TOTAL INTENSITY (TI)
*/
  bh = sqrt((bx*bx)+(by*by));
  *ti = sqrt((bh*bh)+(bz*bz));
  *dec = atan2(by,bx)/DEG_TO_RAD;
  *dip = atan2(bz,bh)/DEG_TO_RAD;
/*
    COMPUTE MAGNETIC GRID VARIATION IF THE CURRENT
    GEODETIC POSITION IS IN THE ARCTIC OR ANTARCTIC
    (I.E. GLAT > +55 DEGREES OR GLAT < -55 DEGREES)

    OTHERWISE, SET MAGNETIC GRID VARIATION TO -999.0
*/
  *gv = -999.0;
  if (fabs(glat) >= 55.)
    {
      if (glat > 0.0 && glon >= 0.0) *gv = *dec-glon;
      if (glat > 0.0 && glon < 0.0) *gv = *dec+fabs(glon);
      if (glat < 0.0 && glon >= 0.0) *gv = *dec+glon;
      if (glat < 0.0 && glon < 0.0) *gv = *dec-fabs(glon);
      if (*gv > +180.0) *gv -= 360.0;
      if (*gv < -180.0) *gv += 360.0;
    }
  otime = time;
  oalt = alt;
  olat = glat;
  olon = glon;
  return 0;
}

int wmm_Calc(WMM_MAG_DATA *wmm)
{
	float dLat,dLon,altm,time;

	static float ati, adec, adip;
	static float dec, dip, ti, gv;
	static float time1, dec1, dip1, ti1;
	static float time2, dec2, dip2, ti2;

	float x1,x2,y1,y2,z1,z2,h1,h2;
	float ax,ay,az,ah;
	float rTd=0.017453292f;
  
    altm=wmm->altm/1000;
	dLat=wmm->lat;
	dLon=wmm->lon;
	time=wmm->year;
	otime = oalt = olat = olon = -1000.0;

	if(E0000(altm,dLat,dLon,time,&dec,&dip,&ti,&gv)) return 1;

    time1 = time;
    dec1 = dec;
    dip1 = dip;
    ti1 = ti;
    time = time1 + 1.0f;
    
	if(E0000(altm,dLat,dLon,time,&dec,&dip,&ti,&gv)) return 1;

    time2 = time;
    dec2 = dec;
    dip2 = dip;
    ti2 = ti;
    
/*COMPUTE X, Y, Z, AND H COMPONENTS OF THE MAGNETIC FIELD*/
    
    x1=ti1*(cos((dec1*rTd))*cos((dip1*rTd)));
    x2=ti2*(cos((dec2*rTd))*cos((dip2*rTd)));
    y1=ti1*(cos((dip1*rTd))*sin((dec1*rTd)));
    y2=ti2*(cos((dip2*rTd))*sin((dec2*rTd)));
    z1=ti1*(sin((dip1*rTd)));
    z2=ti2*(sin((dip2*rTd)));
    h1=ti1*(cos((dip1*rTd)));
    h2=ti2*(cos((dip2*rTd)));

/*  COMPUTE ANNUAL CHANGE FOR TOTAL INTENSITY  */
    ati = ti2 - ti1;

/*  COMPUTE ANNUAL CHANGE FOR DIP & DEC  */
    adip = (float)((dip2 - dip1) * 60.);
    adec = (float)((dec2 - dec1) * 60.);


/*  COMPUTE ANNUAL CHANGE FOR X, Y, Z, AND H */
    ax = x2-x1;
    ay = y2-y1;
    az = z2-z1;
    ah = h2-h1;

 	wmm->dec=dec1 ; wmm->decMinYr=adec;
	wmm->dip=dip1 ; wmm->dipMinYr=adip;
	wmm->ti=ti1 ; wmm->tiNtYr=ati;
	wmm->hi=h1 ; wmm->hiNtYr=ah;
	wmm->x=x1 ; wmm->xNtYr=ax;
	wmm->y=y1 ; wmm->yNtYr=ay;
	wmm->z=z1 ; wmm->zNtYr=az;

	return 0;
}

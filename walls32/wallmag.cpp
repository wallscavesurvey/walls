//Wallmag.c
#include "stdafx.h"
#include <gpslib.h>
#include "wallmag.h"

#undef _USE_WMM
#define ID_MODELDATA 100

typedef struct {
	float g;
	float hh;
	float g2;
	float hh2;
} SHC;

typedef struct {
	char name[8];
	int	  shc_idx;
	int   max1;
	int   max2;
	int   max3;
	float epoch;
	float yrmin;
	float yrmax;
	float altmin;
	float altmax;
} MODEL;

static MODEL *pMDL;
static SHC	 *pSHC;

static int num_models = 0;
#define FLTYP double

#define IEXT 0
#define EXT_COEFF1 (FLTYP)0
#define EXT_COEFF2 (FLTYP)0
#define EXT_COEFF3 (FLTYP)0

/* Geomag global variables */
static FLTYP gh1[196];
static FLTYP gh2[196];
static FLTYP gha[196];
static FLTYP ghb[196];
static FLTYP dtemp, ftemp, htemp, itemp;
static FLTYP x, y, z, d, f, h, i;
static FLTYP xtemp, ytemp, ztemp;

double mag_HorzIntensity; //global set to h after decl calculation

#ifdef _USE_WMM
static LPSTR wmm_modname = "WMM2005";
WMM_COFDATA *pWMM = NULL;
WMM_MAG_DATA wmm;
#endif

/*==========================================================================*/
/*     getshc() -- Reads spherical harmonic coefficients from the specified */
/*     model into an array.                                                 */
/*                                                                          */
/*     Input:                                                               */
/*           iflag      - Flag for SV equal to ) or not equal to 0          */
/*                        for designated read statements                    */
/*           ii         - Index of SHC record in pSHC[]                     */
/*           nmax       - Maximum degree and order of model                 */
/*                                                                          */
/*     Output:                                                              */
/*           gh[]=gh1[] or gh2[] - Schmidt quasi-normal internal spherical  */
/*                        harmonic coefficients                             */
/*==========================================================================*/
static void getshc(int mm, int ii, int nmax, FLTYP *gh)
{
	float *pF;

	pF = &pSHC[ii].g;
	if (!mm) pF += 2;

	for (ii = 1; ii <= nmax; ii++) {
		for (mm = 0; mm <= ii; mm++) {
			*++gh = *pF;
			if (mm) *++gh = pF[1];
			pF += 4;
		}
	}
}

/****************************************************************************/
/*                                                                          */
/*                           Subroutine extrapsh                            */
/*                                                                          */
/****************************************************************************/
/*                                                                          */
/*     Extrapolates linearly a spherical harmonic model with a              */
/*     rate-of-change model.                                                */
/*                                                                          */
/*     Input:                                                               */
/*           date     - date of resulting model (in decimal year)           */
/*           dte1     - date of base model                                  */
/*           nmax1    - maximum degree and order of base model              */
/*           gh1      - Schmidt quasi-normal internal spherical             */
/*                      harmonic coefficients of base model                 */
/*           nmax2    - maximum degree and order of rate-of-change model    */
/*           gh2      - Schmidt quasi-normal internal spherical             */
/*                      harmonic coefficients of rate-of-change model       */
/*                                                                          */
/*     Output:                                                              */
/*           gha or b - Schmidt quasi-normal internal spherical             */
/*                    harmonic coefficients                                 */
/*           nmax   - maximum degree and order of resulting model           */
/*                                                                          */
/*     FORTRAN                                                              */
/*           A. Zunde                                                       */
/*           USGS, MS 964, box 25046 Federal Center, Denver, CO.  80225     */
/*                                                                          */
/*     C                                                                    */
/*           C. H. Shaffer                                                  */
/*           Lockheed Missiles and Space Company, Sunnyvale CA              */
/*           August 16, 1988                                                */
/*                                                                          */
/****************************************************************************/


static int extrapsh(FLTYP date, FLTYP dte1, int nmax1, int nmax2, int gh)
{
	int   nmax;
	int   k, l;
	int   ii;
	FLTYP factor;

	factor = date - dte1;
	if (nmax1 == nmax2)
	{
		k = nmax1 * (nmax1 + 2);
		nmax = nmax1;
	}
	else
	{
		if (nmax1 > nmax2)
		{
			k = nmax2 * (nmax2 + 2);
			l = nmax1 * (nmax1 + 2);
			switch (gh)
			{
			case 3:  for (ii = k + 1; ii <= l; ++ii)
			{
				gha[ii] = gh1[ii];
			}
					 break;
			case 4:  for (ii = k + 1; ii <= l; ++ii)
			{
				ghb[ii] = gh1[ii];
			}
					 break;
			}
			nmax = nmax1;
		}
		else
		{
			k = nmax1 * (nmax1 + 2);
			l = nmax2 * (nmax2 + 2);
			switch (gh)
			{
			case 3:  for (ii = k + 1; ii <= l; ++ii)
			{
				gha[ii] = factor * gh2[ii];
			}
					 break;
			case 4:  for (ii = k + 1; ii <= l; ++ii)
			{
				ghb[ii] = factor * gh2[ii];
			}
					 break;
			}
			nmax = nmax2;
		}
	}
	switch (gh)
	{
	case 3:  for (ii = 1; ii <= k; ++ii)
	{
		gha[ii] = gh1[ii] + factor * gh2[ii];
	}
			 break;
	case 4:  for (ii = 1; ii <= k; ++ii)
	{
		ghb[ii] = gh1[ii] + factor * gh2[ii];
	}
			 break;
	}
	return(nmax);
}

/****************************************************************************/
/*                                                                          */
/*                           Subroutine interpsh                            */
/*                                                                          */
/****************************************************************************/
/*                                                                          */
/*     Interpolates linearly, in time, between two spherical harmonic       */
/*     models.                                                              */
/*                                                                          */
/*     Input:                                                               */
/*           date     - date of resulting model (in decimal year)           */
/*           dte1     - date of earlier model                               */
/*           nmax1    - maximum degree and order of earlier model           */
/*           gh1      - Schmidt quasi-normal internal spherical             */
/*                      harmonic coefficients of earlier model              */
/*           dte2     - date of later model                                 */
/*           nmax2    - maximum degree and order of later model             */
/*           gh2      - Schmidt quasi-normal internal spherical             */
/*                      harmonic coefficients of internal model             */
/*                                                                          */
/*     Output:                                                              */
/*           gha or b - coefficients of resulting model                     */
/*           nmax     - maximum degree and order of resulting model         */
/*                                                                          */
/*     FORTRAN                                                              */
/*           A. Zunde                                                       */
/*           USGS, MS 964, box 25046 Federal Center, Denver, CO.  80225     */
/*                                                                          */
/*     C                                                                    */
/*           C. H. Shaffer                                                  */
/*           Lockheed Missiles and Space Company, Sunnyvale CA              */
/*           August 17, 1988                                                */
/*                                                                          */
/****************************************************************************/


static int interpsh(FLTYP date, FLTYP dte1, int nmax1, FLTYP dte2, int nmax2, int gh)
{
	int   nmax;
	int   k, l;
	int   ii;
	FLTYP factor;

	factor = (date - dte1) / (dte2 - dte1);
	if (nmax1 == nmax2)
	{
		k = nmax1 * (nmax1 + 2);
		nmax = nmax1;
	}
	else
	{
		if (nmax1 > nmax2)
		{
			k = nmax2 * (nmax2 + 2);
			l = nmax1 * (nmax1 + 2);
			switch (gh)
			{
			case 3:  for (ii = k + 1; ii <= l; ++ii)
			{
				gha[ii] = gh1[ii] + factor * (-gh1[ii]);
			}
					 break;
			case 4:  for (ii = k + 1; ii <= l; ++ii)
			{
				ghb[ii] = gh1[ii] + factor * (-gh1[ii]);
			}
					 break;
			}
			nmax = nmax1;
		}
		else
		{
			k = nmax1 * (nmax1 + 2);
			l = nmax2 * (nmax2 + 2);
			switch (gh)
			{
			case 3:  for (ii = k + 1; ii <= l; ++ii)
			{
				gha[ii] = factor * gh2[ii];
			}
					 break;
			case 4:  for (ii = k + 1; ii <= l; ++ii)
			{
				ghb[ii] = factor * gh2[ii];
			}
					 break;
			}
			nmax = nmax2;
		}
	}
	switch (gh)
	{
	case 3:  for (ii = 1; ii <= k; ++ii)
	{
		gha[ii] = gh1[ii] + factor * (gh2[ii] - gh1[ii]);
	}
			 break;
	case 4:  for (ii = 1; ii <= k; ++ii)
	{
		ghb[ii] = gh1[ii] + factor * (gh2[ii] - gh1[ii]);
	}
			 break;
	}
	return(nmax);
}


/****************************************************************************/
/*                                                                          */
/*                           Subroutine shval3                              */
/*                                                                          */
/****************************************************************************/
/*                                                                          */
/*     Calculates field components from spherical harmonic (sh)             */
/*     models.                                                              */
/*                                                                          */
/*     Input:                                                               */
/*           igdgc     - indicates coordinate system used; set equal        */
/*                       to 1 if geodetic, 2 if geocentric                  */
/*           latitude  - north latitude, in degrees                         */
/*           longitude - east longitude, in degrees                         */
/*           elev      - elevation above mean sea level (igdgc=1), or       */
/*                       radial distance from earth's center (igdgc=2)      */
/*           a2,b2     - squares of semi-major and semi-minor axes of       */
/*                       the reference spheroid used for transforming       */
/*                       between geodetic and geocentric coordinates        */
/*                       or components                                      */
/*           nmax      - maximum degree and order of coefficients           */
/*           iext      - external coefficients flag (=0 if none)            */
/*           ext1,2,3  - the three 1st-degree external coefficients         */
/*                       (not used if iext = 0)                             */
/*                                                                          */
/*     Output:                                                              */
/*           x         - northward component                                */
/*           y         - eastward component                                 */
/*           z         - vertically-downward component                      */
/*                                                                          */
/*     based on subroutine 'igrf' by D. R. Barraclough and S. R. C. Malin,  */
/*     report no. 71/1, institute of geological sciences, U.K.              */
/*                                                                          */
/*     FORTRAN                                                              */
/*           Norman W. Peddie                                               */
/*           USGS, MS 964, box 25046 Federal Center, Denver, CO.  80225     */
/*                                                                          */
/*     C                                                                    */
/*           C. H. Shaffer                                                  */
/*           Lockheed Missiles and Space Company, Sunnyvale CA              */
/*           August 17, 1988                                                */
/*                                                                          */
/****************************************************************************/


static int shval3(int igdgc, FLTYP flat, FLTYP flon, FLTYP elev, int nmax,
	int gh, int iext, FLTYP ext1, FLTYP ext2, FLTYP ext3)
{
	FLTYP earths_radius = 6371.2;
	FLTYP dtr = 0.01745329;
	FLTYP slat;
	FLTYP clat;
	FLTYP ratio;
	FLTYP aa, bb, cc, dd;
	FLTYP sd;
	FLTYP cd;
	FLTYP r;
	FLTYP a2;
	FLTYP b2;
	FLTYP rr;
	FLTYP fm, fn;
	FLTYP sl[14];
	FLTYP cl[14];
	FLTYP p[119];
	FLTYP q[119];
	int ii, j, k, l, m, n;
	int npq;
	int ios;
	double arguement;
	double power;
	a2 = 40680925.0;
	b2 = 40408588.0;
	ios = 0;
	r = elev;
	arguement = flat * dtr;
	slat = sin(arguement);
	if ((90.0 - flat) < 0.001)
	{
		aa = 89.999;            /*  300 ft. from North pole  */
	}
	else
	{
		if ((90.0 + flat) < 0.001)
		{
			aa = -89.999;        /*  300 ft. from South pole  */
		}
		else
		{
			aa = flat;
		}
	}
	arguement = aa * dtr;
	clat = cos(arguement);
	arguement = flon * dtr;
	sl[1] = sin(arguement);
	cl[1] = cos(arguement);
	switch (gh)
	{
	case 3:  x = 0;
		y = 0;
		z = 0;
		break;
	case 4:  xtemp = 0;
		ytemp = 0;
		ztemp = 0;
		break;
	}
	sd = 0.0;
	cd = 1.0;
	l = 1;
	n = 0;
	m = 1;
	npq = (nmax * (nmax + 3)) / 2;
	if (igdgc == 1)
	{
		aa = a2 * clat * clat;
		bb = b2 * slat * slat;
		cc = aa + bb;
		arguement = cc;
		dd = sqrt(arguement);
		arguement = elev * (elev + 2.0 * dd) + (a2 * aa + b2 * bb) / cc;
		r = sqrt(arguement);
		cd = (elev + dd) / r;
		sd = (a2 - b2) / dd * slat * clat / r;
		aa = slat;
		slat = slat * cd - clat * sd;
		clat = clat * cd + aa * sd;
	}
	ratio = earths_radius / r;
	arguement = 3.0;
	aa = sqrt(arguement);
	p[1] = 2.0 * slat;
	p[2] = 2.0 * clat;
	p[3] = 4.5 * slat * slat - 1.5;
	p[4] = 3.0 * aa * clat * slat;
	q[1] = -clat;
	q[2] = slat;
	q[3] = -3.0 * clat * slat;
	q[4] = aa * (slat * slat - clat * clat);
	for (k = 1; k <= npq; ++k)
	{
		if (n < m)
		{
			m = 0;
			n = n + 1;
			arguement = ratio;
			power = n + 2;
			rr = pow(arguement, power);
			fn = n;
		}
		fm = m;
		if (k >= 5)
		{
			if (m == n)
			{
				arguement = (1.0 - 0.5 / fm);
				aa = sqrt(arguement);
				j = k - n - 1;
				p[k] = (1.0 + 1.0 / fm) * aa * clat * p[j];
				q[k] = aa * (clat * q[j] + slat / fm * p[j]);
				sl[m] = sl[m - 1] * cl[1] + cl[m - 1] * sl[1];
				cl[m] = cl[m - 1] * cl[1] - sl[m - 1] * sl[1];
			}
			else
			{
				arguement = fn * fn - fm * fm;
				aa = sqrt(arguement);
				arguement = ((fn - 1.0)*(fn - 1.0)) - (fm * fm);
				bb = sqrt(arguement) / aa;
				cc = (2.0 * fn - 1.0) / aa;
				ii = k - n;
				j = k - 2 * n + 1;
				p[k] = (fn + 1.0) * (cc * slat / fn * p[ii] - bb / (fn - 1.0) * p[j]);
				q[k] = cc * (slat * q[ii] - clat / fn * p[ii]) - bb * q[j];
			}
		}
		switch (gh)
		{
		case 3:  aa = rr * gha[l];
			break;
		case 4:  aa = rr * ghb[l];
			break;
		}
		if (m == 0)
		{
			switch (gh)
			{
			case 3:  x = x + aa * q[k];
				z = z - aa * p[k];
				break;
			case 4:  xtemp = xtemp + aa * q[k];
				ztemp = ztemp - aa * p[k];
				break;
			}
			l = l + 1;
		}
		else
		{
			switch (gh)
			{
			case 3:  bb = rr * gha[l + 1];
				cc = aa * cl[m] + bb * sl[m];
				x = x + cc * q[k];
				z = z - cc * p[k];
				if (clat > 0)
				{
					y = y + (aa * sl[m] - bb * cl[m]) *
						fm * p[k] / ((fn + 1.0) * clat);
				}
				else
				{
					y = y + (aa * sl[m] - bb * cl[m]) * q[k] * slat;
				}
				l = l + 2;
				break;
			case 4:  bb = rr * ghb[l + 1];
				cc = aa * cl[m] + bb * sl[m];
				xtemp = xtemp + cc * q[k];
				ztemp = ztemp - cc * p[k];
				if (clat > 0)
				{
					ytemp = ytemp + (aa * sl[m] - bb * cl[m]) *
						fm * p[k] / ((fn + 1.0) * clat);
				}
				else
				{
					ytemp = ytemp + (aa * sl[m] - bb * cl[m]) *
						q[k] * slat;
				}
				l = l + 2;
				break;
			}
		}
		m = m + 1;
	}
	if (iext != 0)
	{
		aa = ext2 * cl[1] + ext3 * sl[1];
		switch (gh)
		{
		case 3:   x = x - ext1 * clat + aa * slat;
			y = y + ext2 * sl[1] - ext3 * cl[1];
			z = z + ext1 * slat + aa * clat;
			break;
		case 4:   xtemp = xtemp - ext1 * clat + aa * slat;
			ytemp = ytemp + ext2 * sl[1] - ext3 * cl[1];
			ztemp = ztemp + ext1 * slat + aa * clat;
			break;
		}
	}
	switch (gh)
	{
	case 3:   aa = x;
		x = x * cd + z * sd;
		z = z * cd - aa * sd;
		break;
	case 4:   aa = xtemp;
		xtemp = xtemp * cd + ztemp * sd;
		ztemp = ztemp * cd - aa * sd;
		break;
	}
	return(ios);
}


/****************************************************************************/
/*                                                                          */
/*                           Subroutine dihf                                */
/*                                                                          */
/****************************************************************************/
/*                                                                          */
/*     Computes the geomagnetic d, i, h, and f from x, y, and z.            */
/*                                                                          */
/*     Input:                                                               */
/*           x  - northward component                                       */
/*           y  - eastward component                                        */
/*           z  - vertically-downward component                             */
/*                                                                          */
/*     Output:                                                              */
/*           d  - declination                                               */
/*           i  - inclination                                               */
/*           h  - horizontal intensity                                      */
/*           f  - total intensity                                           */
/*                                                                          */
/*     FORTRAN                                                              */
/*           A. Zunde                                                       */
/*           USGS, MS 964, box 25046 Federal Center, Denver, CO.  80225     */
/*                                                                          */
/*     C                                                                    */
/*           C. H. Shaffer                                                  */
/*           Lockheed Missiles and Space Company, Sunnyvale CA              */
/*           August 22, 1988                                                */
/*                                                                          */
/****************************************************************************/

static int dihf(int gh)
{
	int ios;
	int j;
	FLTYP sn;
	FLTYP h2;
	FLTYP hpx;
	double arguement, arguement2;
	double rad, pi;

	ios = gh;
	sn = 0.0001;
	rad = 57.29577951;
	pi = 3.141592653589793;
	switch (gh)
	{
	case 3:   for (j = 1; j <= 1; ++j)
	{
		h2 = x * x + y * y;
		arguement = h2;
		h = sqrt(arguement);       /* calculate horizontal intensity */
		arguement = h2 + z * z;
		f = sqrt(arguement);      /* calculate total intensity */
		if (f < sn)
		{
			d = 999.0 * rad;        /* If d and i cannot be determined, */
			i = 999.0 * rad;        /*       set equal to 999.0         */
		}
		else
		{
			arguement = z;
			arguement2 = h;
			i = atan2(arguement, arguement2);
			if (h < sn)
			{
				d = 999.0 * rad;
			}
			else
			{
				hpx = h + x;
				if (hpx < sn)
				{
					d = pi;
				}
				else
				{
					arguement = y;
					arguement2 = hpx;
					d = 2.0 * atan2(arguement, arguement2);
				}
			}
		}
	}
			  break;
	case 4:   for (j = 1; j <= 1; ++j)
	{
		h2 = xtemp * xtemp + ytemp * ytemp;
		arguement = h2;
		htemp = sqrt(arguement);
		arguement = h2 + ztemp * ztemp;
		ftemp = sqrt(arguement);
		if (ftemp < sn)
		{
			dtemp = 999.0;    /* If d and i cannot be determined, */
			itemp = 999.0;    /*       set equal to 999.0         */
		}
		else
		{
			arguement = ztemp;
			arguement2 = htemp;
			itemp = atan2(arguement, arguement2);
			if (htemp < sn)
			{
				dtemp = 999.0;
			}
			else
			{
				hpx = htemp + xtemp;
				if (hpx < sn)
				{
					dtemp = 180.0;
				}
				else
				{
					arguement = ytemp;
					arguement2 = hpx;
					dtemp = 2.0 * atan2(arguement, arguement2);
				}
			}
		}
	}
			  break;
	}
	return(ios);
}

double getDEG(DEGTYP *p, int format)
{
	/*isgn,ideg,imin,fsec --> fdeg and fmin --*/
	double val;
	switch (format) {
	case 0: val = p->fdeg; break;
	case 1: val = (double)p->ideg + p->fmin / 60.0; break;
	case 2: val = (double)p->ideg + p->imin / 60.0 + p->fsec / 3600.0;
	}
	val *= (double)p->isgn;
	if (val == 0.0 && p->isgn < 0) {
		val = -DBL_MIN;
	}
	//if(val==180.0) val=-180.0; //DMcK 4/1998
	return val;
}

void getDMS(DEGTYP *p, double val, int format)
{
	/*fdeg --> isgn,ideg,imin,fsec and fmin --*/

	if (val) {
		//if(val==180.0) val=-180.0;  //DMcK 4/1998
		if (val < 0.0) {
			val = -val;
			p->isgn = -1;
		}
		else p->isgn = 1;
	}
	else p->isgn = 1;

	p->fdeg = val;
	if (!format) return;

	p->ideg = (int)val;
	if ((p->fmin = 60.0*(val - (double)p->ideg)) >= 59.99999995) {
		p->ideg++;
		p->fmin = 0.0;
		p->fsec = 0.0;
		p->imin = 0;
	}
	else if (format > 1) {
		p->imin = (int)p->fmin;
		if ((p->fsec = 60.0*(p->fmin - (double)p->imin)) >= 59.99999995) {
			p->fsec = 0.0;
			if (++p->imin >= 60) {
				p->imin = 0;
				p->fmin = 0.0;
				p->ideg++;
			}
		}
	}
}

/****************************************************************************/
/*                                                                          */
/*                           Subroutine julday                              */
/*                                                                          */
/****************************************************************************/
/*                                                                          */
/*     Computes the decimal day of year from month, day, year.              */
/*     Leap years accounted for 1900 and 2000 are not leap years.           */
/*                                                                          */
/*     Input:                                                               */
/*           year - Integer year of interest                                */
/*           month - Integer month of interest                              */
/*           day - Integer day of interest                                  */
/*                                                                          */
/*     Output:                                                              */
/*           date - Julian date to thousandth of year                       */
/*                                                                          */
/*     FORTRAN                                                              */
/*           S. McLean                                                      */
/*           NGDC, NOAA egc1, 325 Broadway, Boulder CO.  80301              */
/*                                                                          */
/*     C                                                                    */
/*           C. H. Shaffer                                                  */
/*           Lockheed Missiles and Space Company, Sunnyvale CA              */
/*           August 12, 1988                                                */
/*                                                                          */
/*     Julday Bug Fix                                                       */
/*           Thanks to Rob Raper                                            */
/****************************************************************************/

static FLTYP julday(int i_month, int i_day, int i_year)
{
	static int afdm[13] = { 0,1,32,60,91,121,152,182,213,244,274,305,335 };
	int   leap_year = 0;
	int   truncated_dividend;
	FLTYP year;
	FLTYP day;
	FLTYP decimal_date;
	FLTYP remainder = 0.0;
	FLTYP divisor = 4.0;
	FLTYP dividend;
	FLTYP left_over;

	/* Test for leap year.  If true add one to day. */

	year = i_year;                                 /*    Century Years not   */
	if ((i_year != 1900) && (i_year != 2100))      /*  divisible by 400 are  */
	{                                              /*      NOT leap years    */
		dividend = year / divisor;
		truncated_dividend = (int)dividend;
		left_over = dividend - truncated_dividend;
		remainder = left_over * divisor;
		if ((remainder > 0.0) && (i_month > 2))
		{
			leap_year = 1;
		}
		else
		{
			leap_year = 0;
		}
	}
	day = afdm[i_month] + i_day - 1 + leap_year;
	if (leap_year)
	{
		decimal_date = year + (day / 366.0);  /*In version 3.0 this was incorrect*/
	}
	else
	{
		decimal_date = year + (day / 365.0);  /*In version 3.0 this was incorrect*/
	}
	return(decimal_date);
}

int mag_InitModel()
{
	HGLOBAL hg;
	HRSRC hr;
	int *lp;

	//HMODULE h=GetModuleHandle("WALLMAG");
	//HRSRC hrsrc=FindResource(h,"WALLSVG.XML",RT_HTML);
	num_models = 0;
	if (hr = FindResource(NULL, MAKEINTRESOURCE(ID_MODELDATA), RT_RCDATA)) {
		if (hg = LoadResource(NULL, hr)) {
			lp = (int *)LockResource(hg);
#ifdef _USE_WMM
			if (*lp != lp[1] + sizeof(WMM_COFDATA)) {
				pWMM = NULL;
				if (lp[1]) return FALSE;
			}
			else {
				pWMM = (WMM_COFDATA *)((BYTE *)lp + lp[1]);
				wmm_Init(pWMM->epoch, &pWMM->c, &pWMM->cd);
			}
#endif
			num_models = lp[2];
			pMDL = (MODEL *)(lp + 3);
			pSHC = (SHC *)(pMDL + num_models);
		}
	}
	return num_models;
}

int ComputeDecl(MAGDATA *pD, double lat, double lon)
{
	MODEL *pM;
	float sdate = (float)julday(pD->m, pD->d, pD->y);
	float elev = (float)((double)pD->elev / 1000.0);

#ifdef _USE_WMM
	if (sdate >= pWMM->epoch && sdate < pWMM->epoch + 1.0) {
		if (sdate >= pWMM->epoch + 5.0) return MAG_ERR_DATERANGE;
		wmm.lat = (float)lat;
		wmm.lon = (float)lon;
		wmm.year = sdate;
		wmm.altm = (float)pD->elev;
		wmm_Calc(&wmm);
		pD->decl = wmm.dec;
		pD->modname = wmm_modname;
		pD->modnum = num_models + 1;
		return 0;
	}
#endif

	pD->modnum = 0;
	pM = pMDL;
	int ix = 0;
	for (; ix < num_models; ix++, pM++) {
		if (sdate <= pM->yrmax) break;
	}

	if (ix == num_models) {
		//return MAG_ERR_DATERANGE;
		pM--;
		//assert(pM==pMDL+(ix-1));
	}

	if (sdate < pM->yrmin) {
		if (!ix) return MAG_ERR_DATERANGE;
		ix--;
		pM--;
	}

	/* Get elevation */
	if ((elev < pM->altmin) || (elev > pM->altmax)) return MAG_ERR_ELEVRANGE;

	pD->modname = pM->name;
	pD->modnum = ix + 1;

	/* Play it safe for this DLL version -- */
	x = y = z = d = f = h = i = 0.0;
	xtemp = ytemp = ztemp = dtemp = ftemp = htemp = itemp = 0.0;

	if (pM->max2 == 0) {
		getshc(1, pM->shc_idx, pM->max1, gh1);
		getshc(1, pM[1].shc_idx, pM[1].max1, gh2);
		ix = interpsh(sdate, pM->yrmin, pM->max1, pM[1].yrmin, pM[1].max1, 3);
		ix = interpsh(sdate + 1.0, pM->yrmin, pM->max1, pM[1].yrmin, pM[1].max1, 4);
	}
	else {
		getshc(1, pM->shc_idx, pM->max1, gh1);
		getshc(0, pM->shc_idx, pM->max2, gh2);
		ix = extrapsh(sdate, pM->epoch, pM->max1, pM->max2, 3);
		ix = extrapsh(sdate + 1.0, pM->epoch, pM->max1, pM->max2, 4);
	}

	/* Do the first calculations */
	shval3(1, lat, lon, elev, ix, 3,
		IEXT, EXT_COEFF1, EXT_COEFF2, EXT_COEFF3);
	dihf(3);
	shval3(1, lat, lon, elev, ix, 4,
		IEXT, EXT_COEFF1, EXT_COEFF2, EXT_COEFF3);
	dihf(4);

	//pD->degyr = (float)((dtemp - d)*57.29578);
	pD->decl = (float)(d*57.29578);
	mag_HorzIntensity = (lat == 90) ? 0 : h;
	return 0;
}

int mag_GetData(MAGDATA *pD)
{
	//using namespace GeographicLib;
	int   ix = pD->fcn;
	double lat, lon, conv;

	if (ix < MAG_CVTDATUM) {
		switch (ix) {
		case MAG_GETVERSION: return MAG_VERSION;
		case MAG_GETDATUMCOUNT: return gps_nDatums;
		case MAG_GETDATUMNAME: {
			if (pD->datum >= gps_nDatums) {
				pD->modname = "";
				return FALSE;
			}
			pD->modname = gDatum[pD->datum].name;
			return TRUE;
		}
		case MAG_GETNAD27:
			return gps_NAD27_datum;

		case MAG_FIXFORMAT:
			lat = getDEG(&pD->lat, pD->format);
			lon = getDEG(&pD->lon, pD->format);
			getDMS(&pD->lat, lat, 2); //get all components
			getDMS(&pD->lon, lon, 2);
			return 0;

		case MAG_GETUTMZONE:
		case MAG_GETDECL:
			lat = getDEG(&pD->lat, pD->format);
			lon = getDEG(&pD->lon, pD->format);
			return (ix == MAG_GETUTMZONE) ? DegToUTMZone(lat, lon) : ComputeDecl(pD, lat, lon);

		case MAG_CVTDATE:
			lat = getDEG(&pD->lat, pD->format);
			lon = getDEG(&pD->lon, pD->format);
			break;

		case MAG_CVTUTM2LL3: //returns any LL in range, no conv or decl computed
		case MAG_CVTUTM2LL2: //returns any LL in range, decl, and conv (possibly out-of-zone with MAG_ERR_OUTOFZONE error)
		case MAG_CVTUTM2LL: //returns any LL in range plus conv and decl
			gps_datum = pD->datum;

			if (!(pD->zone2 = UTMUPStoDeg(pD->zone, pD->east, pD->north, &lat, &lon, &conv)))
				return MAG_ERR_ZONERANGE;

			//no fatal errors possible now --
			getDMS(&pD->lat, lat, pD->format);
			getDMS(&pD->lon, lon, pD->format);

			if (ix == MAG_CVTUTM2LL3)
				//no decl or zone check required --
				return 0;

			pD->conv = (float)conv;
			if (ix == MAG_CVTUTM2LL2) {
				ComputeDecl(pD, lat, lon); //ignore return
				return (pD->zone2 != pD->zone) ? MAG_ERR_OUTOFZONE : 0;
			}
			break; //compute decl

		case MAG_CVTLL2UTM: //compute utm and zone with conv and decl
		case MAG_CVTLL2UTM2: //compute utm and zone with conv but no decl
		case MAG_CVTLL2UTM3: //force utm to specific zone and compute conv and decl, can rtn MAG_ERR_OUTOFZONE
			lat = getDEG(&pD->lat, pD->format);
			lon = getDEG(&pD->lon, pD->format);
			gps_datum = pD->datum;
			pD->zone2 = (ix == MAG_CVTLL2UTM3) ? pD->zone : 0;
			//Force zone to pD->zone2 if not zero, return actual zone  --
			if (!(pD->zone2 = DegToUTMUPS(lat, lon, &pD->zone2, &pD->east, &pD->north, &conv)))
				return pD->zone ? MAG_ERR_ZONERANGE : MAG_ERR_LLRANGE;
			pD->conv = (float)conv;
			if (ix == MAG_CVTLL2UTM3) {
				ComputeDecl(pD, lat, lon); //ignore return
				return (pD->zone2 != pD->zone) ? MAG_ERR_OUTOFZONE : 0;
			}
			pD->zone = pD->zone2;
			if (ix == MAG_CVTLL2UTM2) return 0;
			break;
			//compute decl
		}
	}
	else {
		//Datum conversion --
		if (ix >= MAG_CVTDATUM2) {
			//UTM/UPS specified --
			if ((ix -= MAG_CVTDATUM2) == pD->datum) return 0; //no change
			gps_datum = pD->datum;
			if (!UTMUPStoDeg(pD->zone, pD->east, pD->north, &lat, &lon, NULL))
				return MAG_ERR_CVTUTM2LL;
		}
		else {
			//Lat/Long specified --
			if ((ix -= MAG_CVTDATUM) == pD->datum) return 0;
			lat = getDEG(&pD->lat, pD->format);
			lon = getDEG(&pD->lon, pD->format);
		}
		if (pD->datum != gps_WGS84_datum) {
			gps_datum = pD->datum;
			//Convert to WGS84 --
			if (!translate(FALSE, &lat, &lon))
				return MAG_ERR_TRANSLATE;
		}
		//Convert from WGS84 --
		gps_datum = ix; //desired datum
		if (ix != gps_WGS84_datum && !translate(TRUE, &lat, &lon))
			return MAG_ERR_TRANSLATE;
		//Finally, refresh UTM/UPS --
		getDMS(&pD->lat, lat, pD->format);
		getDMS(&pD->lon, lon, pD->format);
		pD->datum = ix;
		pD->zone = 0;
		if (!DegToUTMUPS(lat, lon, &pD->zone, &pD->east, &pD->north, &conv)) {
			ASSERT(0);
			return MAG_ERR_TRANSLATE;
		}
		pD->conv = (float)conv;
		ComputeDecl(pD, lat, lon);
		return 0;
	}

	return ComputeDecl(pD, lat, lon);
}

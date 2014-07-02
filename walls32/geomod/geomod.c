/****************************************************************************/
/*                                                                          */
/*       Disclaimer: This C version of the Geomagnetic Field                */
/*       Modeling software is being supplied to aid programmers in          */
/*       integrating spherical harmonic field modeling calculations         */
/*       into their code. It is being distributed unoffically. The          */
/*       National Geophysical Data Center does not support it as            */
/*       a data product or guarantee it's correctness. The limited          */
/*       testing done on this code seems to indicate that the results       */
/*       obtained from this program are compariable to those obtained       */
/*                                                                          */
/****************************************************************************/
/*                                                                          */
/*     In regards to the disclaimer, to the best of my knowlege and quick   */
/*     testing, this program, generates numaric output which is withing .1  */
/*     degress of the current fortran version.  However, it *is* a program  */
/*     and most likely contains bugs.                                       */ 
/*                                              dio  6-6-96                 */
/*                                                                          */
/****************************************************************************/
/*                                                                          */
/*     This is version 3.01 of the source code but still represents the     */
/*     geomag30 executable.                                                 */	
/*                                              dio 9-17-96                 */
/*                                                                          */
/****************************************************************************/
/*                                                                          */
/*      This program calculates the geomagnetic field values from           */
/*      a spherical harmonic model.  Inputs required by the user are:       */
/*      a spherical harmonic model data file, coordinate preference,        */
/*      elevation, date/range-step, latitude, and longitude.                */
/*                                                                          */
/*         Spherical Harmonic                                               */
/*         Model Data File       :  Name of the data file containing the    */
/*                                  spherical harmonic coefficients of      */
/*                                  the chosen model.  The model and path   */
/*                                  must be less than MAXREAD chars.        */
/*                                                                          */
/*         Coordinate Preference :  Geodetic (measured from                 */
/*                                  the surface of the earth),              */
/*                                  or geocentric (measured from the        */
/*                                  center of the earth).                   */
/*                                                                          */
/*         Elevation             :  Elevation above sea level in kilometers.*/
/*                                  if geocentric coordinate preference is  */
/*                                  used then the elevation must be in the  */
/*                                  range of 6370.20 km - 6971.20 km as     */
/*                                  measured from the center of the earth.  */
/*                                  Enter elevation in kilometers in the    */
/*                                  form xxxx.xx.                           */
/*                                                                          */
/*         Date                  :  Date, in decimal years, for which to    */
/*                                  calculate the values of the magnetic    */
/*                                  field.  The date must be within the     */
/*                                  limits of the model chosen.             */
/*                                                                          */
/*         Latitude              :  Entered in decimal degrees in the       */
/*                                  form xxx.xxx.  Positive for northern    */
/*                                  hemisphere, negative for the southern   */
/*                                  hemisphere.                             */
/*                                                                          */
/*         Longitude             :  Entered in decimal degrees in the       */
/*                                  form xxx.xxx.  Positive for eastern     */
/*                                  hemisphere, negative for the western    */
/*                                  hemisphere.                             */
/*                                                                          */
/****************************************************************************/
/*                                                                          */
/*      Subroutines called :  getshc,interpsh,    */
/*                            extrapsh,shval3,dihf,safegets                 */
/*                                                                          */
/****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>                         /* standard C include statements */
#include <math.h>
#include <string.h>
#include <ctype.h>

#define FLTYP double

#define IEXT 0
#define RECL 81

#define MAXINBUFF RECL+14

/** Max size of in buffer **/

#define MAXREAD MAXINBUFF-2
/** Max to read 2 less than total size (just to be safe) **/

#define MAXSHC 1500
#define MAXMOD 30
/** Max number of models in a file **/

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

static SHC shc[MAXSHC];
static MODEL model[MAXMOD];
static int num_model,num_shc;

#define EXT_COEFF1 (FLTYP)0
#define EXT_COEFF2 (FLTYP)0
#define EXT_COEFF3 (FLTYP)0

FLTYP gh1[171];
FLTYP gh2[171];
FLTYP gha[171];                                  /* Geomag global variables */
FLTYP ghb[171];
FLTYP d=0,f=0,h=0,i=0;
FLTYP dtemp,ftemp,htemp,itemp;
FLTYP x=0,y=0,z=0;
FLTYP xtemp,ytemp,ztemp;

FILE *stream = NULL;                /* Pointer to specified model data file */

/****************************************************************************/
/*                                                                          */
/*                             Program Geomag                               */
/*                                                                          */
/****************************************************************************/
/*                                                                          */
/*      This program, originally written in FORTRAN, was developed using    */
/*      subroutines written by                                              */
/*      A. Zunde                                                            */
/*      USGS, MS 964, Box 25046 Federal Center, Denver, Co.  80225          */
/*      and                                                                 */
/*      S.R.C. Malin & D.R. Barraclough                                     */
/*      Institute of Geological Sciences, United Kingdom.                   */
/*                                                                          */
/****************************************************************************/
/*                                                                          */
/*      Translated                                                          */
/*      into C by    : Craig H. Shaffer                                     */
/*                     Lockheed Missiles and Space Company                  */
/*                     Sunnyvale, California                                */
/*                     (408) 756 - 5210                                     */
/*                     29 July, 1988                                        */
/*                                                                          */
/*      Contact      : Greg Fountain                                        */
/*                     Lockheed Missiles and Space Company                  */
/*                     Dept. MO-30, Bldg. 581                               */
/*                     1111 Lockheed Way                                    */
/*                     P.O. Box 3504                                        */
/*                     Sunnyvale, Calif.  94088-3504                        */
/*                                                                          */
/*      Rewritten by : David Owens                                          */
/*                     dio@ngdc.noaa.gov                                    */
/*                     For Susan McClean                                    */
/*                                                                          */
/*      Contact      : Susan McLean                                         */
/*                     sjm@ngdc.noaa.gov                                    */
/*                     National Geophysical Data Center                     */
/*                     World Data Center-A for Solid Earth Geophysics       */
/*                     NOAA, E/GC1, 325 Broadway,                           */
/*                     Boulder, CO  80303                                   */
/*                                                                          */
/*      Original                                                            */
/*      FORTRAN                                                             */
/*      Programmer   : National Geophysical Data Center                     */
/*                     World Data Center-A for Solid Earth Geophysics       */
/*                     NOAA, E/GC1, 325 Broadway,                           */
/*                     Boulder, CO  80303                                   */
/*                                                                          */
/*      Contact      : Minor Davis                                          */
/*                     National Geophysical Data Center                     */
/*                     World Data Center-A for Solid Earth Geophysics       */
/*                     NOAA, E/GC1, 325 Broadway,                           */
/*                     Boulder, CO  80303                                   */
/*                                                                          */
/****************************************************************************/
/*                                                                          */
/*      dio modifications include overhauling the interactive interface to  */
/*  support platform independence and improve fatal error dectection and    */
/*  prevention.  A command line interface was added and the interactive     */
/*  interface was streamlined.  The function safegets() was added and the   */
/*  function getshc's i/0 was modified.  A option to specify a date range   */
/*  range and step was also added.                                          */
/*                                                                          */
/*                                             dio 6-6-96                   */
/*                                                                          */
/****************************************************************************/
/*                                                                          */
/*      Some variables used in this program                                 */
/*                                                                          */
/*    Name         Type                    Usage                            */
/* ------------------------------------------------------------------------ */
/*                                                                          */
/*   a2,b2      Scalar Float           Squares of semi-major and semi-minor */
/*                                     axes of the reference spheroid used  */
/*                                     for transforming between geodetic or */
/*                                     geocentric coordinates.              */
/*                                                                          */
/*   minalt     Float array of MAXMOD  Minimum height of model.             */
/*                                                                          */
/*   altmin     Float                  Minimum height of selected model.    */
/*                                                                          */
/*   altmax     Float array of MAXMOD  Maximum height of model.             */
/*                                                                          */
/*   maxalt     Float                  Maximum height of selected model.    */
/*                                                                          */
/*   d          Scalar Float           Declination of the field from the    */
/*                                     geographic north (deg).              */
/*                                                                          */
/*   sdate  Scalar Float           start date inputted                  */
/*                                                                          */
/*   ddot       Scalar Float           annual rate of change of decl.       */
/*                                     (deg/yr)                             */
/*                                                                          */
/*   elev       Scalar Float           elevation.                           */
/*                                                                          */
/*   epoch      Float array of MAXMOD  epoch of model.                      */
/*                                                                          */
/*   ext        Scalar Float           Three 1st-degree external coeff.     */
/*                                                                          */
/*   latitude   Scalar Float           Latitude.                            */
/*                                                                          */
/*   longitude  Scalar Float           Longitude.                           */
/*                                                                          */
/*   gh1        Float array of 120     Schmidt quasi-normal internal        */
/*                                     spherical harmonic coeff.            */
/*                                                                          */
/*   gh2        Float array of 120     Schmidt quasi-normal internal        */
/*                                     spherical harmonic coeff.            */
/*                                                                          */
/*   gha        Float array of 120     Coefficients of resulting model.     */
/*                                                                          */
/*   ghb        Float array of 120     Coefficients of rate of change model.*/
/*                                                                          */
/*   i          Scalar Float           Inclination (deg).                   */
/*                                                                          */
/*   idot       Scalar Float           Rate of change of i (deg/yr).        */
/*                                                                          */
/*   igdgc      Integer                Flag for geodetic or geocentric      */
/*                                     coordinate choice.                   */
/*                                                                          */
/*   inbuff     Char a of MAXINBUF     Input buffer.                        */
/*                                                                          */
/*   irec_pos   Integer array of MAXMOD Record counter for header           */
/*                                                                          */
/*   stream  Integer                   File handles for an opened file.     */
/*                                                                          */
/*   fileline   Integer                Current line in file (for errors)    */
/*                                                                          */
/*   max1       Integer array of MAXMOD Main field coefficient.             */
/*                                                                          */
/*   max2       Integer array of MAXMOD Secular variation coefficient.      */
/*                                                                          */
/*   max3       Integer array of MAXMOD Acceleration coefficient.           */
/*                                                                          */
/*   mdfile     Character array of MAXREAD  Model file name.                   */
/*                                                                          */
/*   minyr      Float                  Min year of all models               */
/*                                                                          */
/*   maxyr      Float                  Max year of all models               */
/*                                                                          */
/*   yrmax      Float array of MAXMOD  Max year of model.                   */
/*                                                                          */
/*   yrmin      Float array of MAXMOD  Min year of model.                   */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/*                                                                          */
/*                           Subroutine getshc                              */
/*                                                                          */
/****************************************************************************/
/*                                                                          */
/*     Reads spherical harmonic coefficients from the specified             */
/*     model into an array.                                                 */
/*                                                                          */
/*     Input:                                                               */
/*           stream     - Logical unit number                               */
/*           iflag      - Flag for SV equal to ) or not equal to 0          */
/*                        for designated read statements                    */
/*           strec      - Starting record number to read from model         */
/*           nmax_of_gh - Maximum degree and order of model                 */
/*                                                                          */
/*     Output:                                                              */
/*           gh1 or 2   - Schmidt quasi-normal internal spherical           */
/*                        harmonic coefficients                             */
/*                                                                          */
/*     FORTRAN                                                              */
/*           Bill Flanagan                                                  */
/*           NOAA CORPS, DESDIS, NGDC, 325 Broadway, Boulder CO.  80301     */
/*                                                                          */
/*     C                                                                    */
/*           C. H. Shaffer                                                  */
/*           Lockheed Missiles and Space Company, Sunnyvale CA              */
/*           August 15, 1988                                                */
/*                                                                          */
/****************************************************************************/

int readshc(int nmax)
{
	char  inbuff[MAXINBUFF];
	int m,n,mm,nn;
	SHC *pS=&shc[num_shc];

	for ( nn = 1; nn <= nmax; ++nn)
	{
		for (mm = 0; mm <= nn; ++mm)
		{
			fgets(inbuff, 3, stream);
			inbuff[3]='\0';
			sscanf(inbuff, "%d", &m);
			fgets(inbuff, 3, stream);
			inbuff[3]='\0';
			sscanf(inbuff, "%d", &n);
			if (++num_shc>MAXSHC || (nn != n) || (mm != m)) return -1;
			fgets(inbuff, MAXREAD-4, stream);
			sscanf(inbuff, "%f%f%f%f", &pS->g, &pS->hh, &pS->g2, &pS->hh2);
			pS++;
		}
	}

	return(0);
}

void getshc(int iflag, int ii, int nmax_of_gh, FLTYP *gh)
{
	int mm,nn;
	SHC *pS;
	FLTYP g,hh;

	pS=&shc[ii];
	ii = 0;

	for(nn = 1; nn <= nmax_of_gh; ++nn)
	{
		for (mm = 0; mm <= nn; ++mm)
		{
			if(iflag!=1) {
			  g=pS->g2;
			  hh=pS->hh2;
			}
			else {
			  g=pS->g;
			  hh=pS->hh;
			}
			pS++;
			gh[++ii]=g;
			if (mm != 0) gh[++ii]=hh;
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


int extrapsh(FLTYP date, FLTYP dte1, int nmax1, int nmax2, int gh)
{
   int   nmax;
   int   k, l;
   int   ii;
   FLTYP factor;

   factor = date - dte1;
   if (nmax1 == nmax2)
   {
      k =  nmax1 * (nmax1 + 2);
      nmax = nmax1;
   }
   else
   {
      if (nmax1 > nmax2)
      {
	 k = nmax2 * (nmax2 + 2);
	 l = nmax1 * (nmax1 + 2);
	 switch(gh)
	 {
	    case 3:  for ( ii = k + 1; ii <= l; ++ii)
		     {
			gha[ii] = gh1[ii];
		     }
		     break;
	    case 4:  for ( ii = k + 1; ii <= l; ++ii)
		     {
			ghb[ii] = gh1[ii];
		     }
		     break;
	    default: printf("\nError in subroutine extrapsh");
		     break;
	 }
	 nmax = nmax1;
      }
      else
      {
	 k = nmax1 * (nmax1 + 2);
	 l = nmax2 * (nmax2 + 2);
	 switch(gh)
	 {
	    case 3:  for ( ii = k + 1; ii <= l; ++ii)
		     {
			gha[ii] = factor * gh2[ii];
		     }
		     break;
	    case 4:  for ( ii = k + 1; ii <= l; ++ii)
		     {
			ghb[ii] = factor * gh2[ii];
		     }
		     break;
	    default: printf("\nError in subroutine extrapsh");
		     break;
	 }
	 nmax = nmax2;
      }
   }
   switch(gh)
   {
      case 3:  for ( ii = 1; ii <= k; ++ii)
	       {
		  gha[ii] = gh1[ii] + factor * gh2[ii];
	       }
	       break;
      case 4:  for ( ii = 1; ii <= k; ++ii)
	       {
		  ghb[ii] = gh1[ii] + factor * gh2[ii];
	       }
	       break;
      default: printf("\nError in subroutine extrapsh");
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


int interpsh(FLTYP date, FLTYP dte1, int nmax1, FLTYP dte2, int nmax2, int gh)
{
   int   nmax;
   int   k, l;
   int   ii;
   FLTYP factor;

   factor = (date - dte1) / (dte2 - dte1);
   if (nmax1 == nmax2)
   {
      k =  nmax1 * (nmax1 + 2);
      nmax = nmax1;
   }
   else
   {
      if (nmax1 > nmax2)
      {
	 k = nmax2 * (nmax2 + 2);
	 l = nmax1 * (nmax1 + 2);
	 switch(gh)
	 {
	    case 3:  for ( ii = k + 1; ii <= l; ++ii)
		     {
			gha[ii] = gh1[ii] + factor * (-gh1[ii]);
		     }
		     break;
	    case 4:  for ( ii = k + 1; ii <= l; ++ii)
		     {
			ghb[ii] = gh1[ii] + factor * (-gh1[ii]);
		     }
		     break;
	    default: printf("\nError in subroutine extrapsh");
		     break;
	 }
	 nmax = nmax1;
      }
      else
      {
	 k = nmax1 * (nmax1 + 2);
	 l = nmax2 * (nmax2 + 2);
	 switch(gh)
	 {
	    case 3:  for ( ii = k + 1; ii <= l; ++ii)
		     {
			gha[ii] = factor * gh2[ii];
		     }
		     break;
	    case 4:  for ( ii = k + 1; ii <= l; ++ii)
		     {
			ghb[ii] = factor * gh2[ii];
		     }
		     break;
	    default: printf("\nError in subroutine extrapsh");
		     break;
	 }
	 nmax = nmax2;
      }
   }
   switch(gh)
   {
      case 3:  for ( ii = 1; ii <= k; ++ii)
	       {
		  gha[ii] = gh1[ii] + factor * (gh2[ii] - gh1[ii]);
	       }
	       break;
      case 4:  for ( ii = 1; ii <= k; ++ii)
	       {
		  ghb[ii] = gh1[ii] + factor * (gh2[ii] - gh1[ii]);
	       }
	       break;
      default: printf("\nError in subroutine extrapsh");
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


int shval3(int igdgc, FLTYP flat, FLTYP flon, FLTYP elev, int nmax,
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
   FLTYP fm,fn;
   FLTYP sl[14];
   FLTYP cl[14];
   FLTYP p[119];
   FLTYP q[119];
   int ii,j,k,l,m,n;
   int npq;
   int ios;
   double arguement;
   double power;
   a2 = 40680925.0;
   b2 = 40408588.0;
   ios = 0;
   r = elev;
   arguement = flat * dtr;
   slat = sin( arguement );
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
   clat = cos( arguement );
   arguement = flon * dtr;
   sl[1] = sin( arguement );
   cl[1] = cos( arguement );
   switch(gh)
   {
      case 3:  x = 0;
	       y = 0;
	       z = 0;
	       break;
      case 4:  xtemp = 0;
	       ytemp = 0;
	       ztemp = 0;
	       break;
      default: printf("\nError in subroutine shval3");
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
      dd = sqrt( arguement );
      arguement = elev * (elev + 2.0 * dd) + (a2 * aa + b2 * bb) / cc;
      r = sqrt( arguement );
      cd = (elev + dd) / r;
      sd = (a2 - b2) / dd * slat * clat / r;
      aa = slat;
      slat = slat * cd - clat * sd;
      clat = clat * cd + aa * sd;
   }
   ratio = earths_radius / r;
   arguement = 3.0;
   aa = sqrt( arguement );
   p[1] = 2.0 * slat;
   p[2] = 2.0 * clat;
   p[3] = 4.5 * slat * slat - 1.5;
   p[4] = 3.0 * aa * clat * slat;
   q[1] = -clat;
   q[2] = slat;
   q[3] = -3.0 * clat * slat;
   q[4] = aa * (slat * slat - clat * clat);
   for ( k = 1; k <= npq; ++k)
   {
      if (n < m)
      {
	 m = 0;
	 n = n + 1;
	 arguement = ratio;
	 power =  n + 2;
	 rr = pow(arguement,power);
	 fn = n;
      }
      fm = m;
      if (k >= 5)
      {
	 if (m == n)
	 {
	    arguement = (1.0 - 0.5/fm);
	    aa = sqrt( arguement );
	    j = k - n - 1;
	    p[k] = (1.0 + 1.0/fm) * aa * clat * p[j];
	    q[k] = aa * (clat * q[j] + slat/fm * p[j]);
	    sl[m] = sl[m-1] * cl[1] + cl[m-1] * sl[1];
	    cl[m] = cl[m-1] * cl[1] - sl[m-1] * sl[1];
	 }
	 else
	 {
	    arguement = fn*fn - fm*fm;
	    aa = sqrt( arguement );
	    arguement = ((fn - 1.0)*(fn-1.0)) - (fm * fm);
	    bb = sqrt( arguement )/aa;
	    cc = (2.0 * fn - 1.0)/aa;
	    ii = k - n;
	    j = k - 2 * n + 1;
	    p[k] = (fn + 1.0) * (cc * slat/fn * p[ii] - bb/(fn - 1.0) * p[j]);
	    q[k] = cc * (slat * q[ii] - clat/fn * p[ii]) - bb * q[j];
	 }
      }
      switch(gh)
      {
	 case 3:  aa = rr * gha[l];
		  break;
	 case 4:  aa = rr * ghb[l];
		  break;
	 default: printf("\nError in subroutine shval3");
		  break;
      }
      if (m == 0)
      {
	 switch(gh)
	 {
	    case 3:  x = x + aa * q[k];
		     z = z - aa * p[k];
		     break;
	    case 4:  xtemp = xtemp + aa * q[k];
		     ztemp = ztemp - aa * p[k];
		     break;
	    default: printf("\nError in subroutine shval3");
		     break;
	 }
	 l = l + 1;
      }
      else
      {
	 switch(gh)
	 {
	    case 3:  bb = rr * gha[l+1];
		     cc = aa * cl[m] + bb * sl[m];
		     x = x + cc * q[k];
		     z = z - cc * p[k];
		     if (clat > 0)
		     {
			y = y + (aa * sl[m] - bb * cl[m]) *
			    fm * p[k]/((fn + 1.0) * clat);
		     }
		     else
		     {
			y = y + (aa * sl[m] - bb * cl[m]) * q[k] * slat;
		     }
		     l = l + 2;
		     break;
	    case 4:  bb = rr * ghb[l+1];
		     cc = aa * cl[m] + bb * sl[m];
		     xtemp = xtemp + cc * q[k];
		     ztemp = ztemp - cc * p[k];
		     if (clat > 0)
		     {
			ytemp = ytemp + (aa * sl[m] - bb * cl[m]) *
			    fm * p[k]/((fn + 1.0) * clat);
		     }
		     else
		     {
			ytemp = ytemp + (aa * sl[m] - bb * cl[m]) *
				q[k] * slat;
		     }
		     l = l + 2;
		     break;
	    default: printf("\nError in subroutine shval3");
		     break;
	 }
      }
      m = m + 1;
   }
   if (iext != 0)
   {
      aa = ext2 * cl[1] + ext3 * sl[1];
      switch(gh)
      {
	 case 3:   x = x - ext1 * clat + aa * slat;
		   y = y + ext2 * sl[1] - ext3 * cl[1];
		   z = z + ext1 * slat + aa * clat;
		   break;
	 case 4:   xtemp = xtemp - ext1 * clat + aa * slat;
		   ytemp = ytemp + ext2 * sl[1] - ext3 * cl[1];
		   ztemp = ztemp + ext1 * slat + aa * clat;
		   break;
	 default:  printf("\nError in subroutine shval3");
		   break;
      }
   }
   switch(gh)
   {
      case 3:   aa = x;
		x = x * cd + z * sd;
		z = z * cd - aa * sd;
		break;
      case 4:   aa = xtemp;
		xtemp = xtemp * cd + ztemp * sd;
		ztemp = ztemp * cd - aa * sd;
		break;
      default:  printf("\nError in subroutine shval3");
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

int dihf(int gh)
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
   switch(gh)
   {
      case 3:   for (j = 1; j <= 1; ++j)
		{
		   h2 = x*x + y*y;
		   arguement = h2;
		   h = sqrt(arguement);       /* calculate horizontal intensity */
		   arguement = h2 + z*z;
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
		      i = atan2(arguement,arguement2);
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
			    d = 2.0 * atan2(arguement,arguement2);
			 }
		      }
		   }
		}
		break;
      case 4:   for (j = 1; j <= 1; ++j)
		{
		   h2 = xtemp*xtemp + ytemp*ytemp;
		   arguement = h2;
		   htemp = sqrt(arguement);
		   arguement = h2 + ztemp*ztemp;
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
		      itemp = atan2(arguement,arguement2);
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
			    dtemp = 2.0 * atan2(arguement,arguement2);
			 }
		      }
		   }
		}
		break;
      default:  printf("\nError in subroutine dihf");
		break;
   }
   return(ios);
}

#ifdef _USE_JULDAY
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


FLTYP julday(int i_month,int i_day,int i_year)
{
   static int afdm[13]={0,1,32,60,91,121,152,182,213,244,274,305,335};
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
      dividend = year/divisor;
      truncated_dividend = (int)dividend;
      left_over = dividend - truncated_dividend;
      remainder = left_over*divisor;
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
      decimal_date = year + (day/366.0);  /*In version 3.0 this was incorrect*/
   }
   else
   {
      decimal_date = year + (day/365.0);  /*In version 3.0 this was incorrect*/
   }
   return(decimal_date);
}
#endif

int main(int argc, char**argv)
{
 
	MODEL *pM;

	int   nmax;
	int   fileline;
	int   ddeg, ideg;


	float minyr,maxyr;

	FLTYP minalt,maxalt;
	FLTYP elev=-999999.0;
	FLTYP sdate=-1.0;
	FLTYP latitude=200.0;
	FLTYP longitude=200.0;
	FLTYP ddot;
	FLTYP fdot;
	FLTYP hdot;
	FLTYP idot;
	FLTYP xdot;
	FLTYP ydot;
	FLTYP zdot;
	FLTYP dmin, imin;
	char  mdfile[MAXREAD];
	char  inbuff[MAXINBUFF];

	int bWrite;

	/* Initializations. */

	inbuff[MAXREAD+1]='\0';  /* Just to protect mem. */
	inbuff[MAXINBUFF]='\0';  /* Just to protect mem. */

    bWrite=argc==2;

	if(!bWrite) {
		if(argc<6) {
			printf("\nUSAGE: geomod <model_file> <date> <elev> <lat> <lon>\n"
				     "       geomod <model_file>\n");
			return 0;
		}

		//Degrees and tenths
		longitude=atof(argv[5]);
		latitude=atof(argv[4]);
		if((latitude<-90)||(latitude>90)||(longitude<-180)||(longitude>180)){
		   printf("Latitude or longitude out of range\n");
		   return 1;
		}

		//Elev in Kilometers
		elev=atof(argv[3]);
		//Geodetic, not geocentric

		//Decimal years
		sdate=atof(argv[2]);
	}
      
	strncpy(mdfile,argv[1],MAXREAD);
	if(strchr(mdfile,'.')) {
	  printf("Model file cannot have an extension.\n");
	  return 2;
	}
	_strupr(mdfile);
	if(!(stream=fopen(mdfile, "rt"))) {
	  printf("Cannot open %s.\n",mdfile);
	  return 2;
	}

    /*  Obtain the desired model file and read the data  */

	fileline = 0;                            /* First line will be 1 */
	num_model = num_shc = 0;                 /* First model will be 0 */

	while(fgets(inbuff,MAXREAD,stream)) {
		fileline++;

		if(strlen(inbuff) != RECL || strncmp(inbuff,"   ",3)) {
			printf("Corrupt record in file %s on line %d.\n", mdfile, fileline);
			fclose(stream);
			return 3;
		}

		if(num_model >= MAXMOD){                /* If too many headers */
		   printf("Too many models in file %s on line %d.", mdfile, fileline);
		   fclose(stream);
		   return 4;
		}

		pM=&model[num_model++];                           /* New model */

		/* Get fields from buffer into individual vars.  */
		sscanf(inbuff, "%s%f%d%d%d%f%f%f%f", pM->name, &pM->epoch, 
			   &pM->max1, &pM->max2, &pM->max3, &pM->yrmin, 
			   &pM->yrmax, &pM->altmin, &pM->altmax);

		pM->shc_idx=num_shc;
		if(readshc(pM->max1))
		{
		   printf("Invalid model file format.\n");
		   fclose(stream);
		   return 5;
		}

		/* Compute date range for all models */ 
		if(num_model == 1){                    /*If first model */
		   minyr=pM->yrmin;
		   maxyr=pM->yrmax;
		}
		else {
		   if(pM->yrmin<minyr) minyr=pM->yrmin;
		   if(pM->yrmax>maxyr) maxyr=pM->yrmax;
		}
	}
    fclose(stream);
        
	if(bWrite) {
		strcat(mdfile,".BIN");
		if(!(stream=fopen(mdfile, "wb")) ||
			fwrite(&num_model,sizeof(num_model),1,stream)!=1 ||
			(int)fwrite(model,sizeof(MODEL),num_model,stream)!=num_model ||
			(int)fwrite(shc,sizeof(SHC),num_shc,stream)!=num_shc ||
			fclose(stream))
		{
		  printf("Cannot create %s.\n",mdfile);
		  if(stream) fclose(stream);
		  return 2;
		}
		printf("File %s written - Models: %d, Size: %d bytes.\n",
			mdfile,
			num_model,
			sizeof(num_model)+sizeof(MODEL)*num_model+sizeof(SHC)*num_shc);
	}
	else {
		/*  Take in field data  */

		if((sdate<minyr)||(sdate>maxyr)) {
		 printf("Date out of supported range.\n");
		 return 6;
		}

		/* Pick model */
		for(pM=model;sdate>pM->yrmax;pM++);
		if(sdate<pM->yrmin) pM--;

		/* Get altitude min and max for selected model. */
		minalt=pM->altmin;
		maxalt=pM->altmax;    

		/* Get elevation */
		if((elev<minalt)||(elev>maxalt)){
			printf("Elevation must be in range (%.2f to %.2f)\n", minalt, maxalt);
			return 7;
		}

		/** This will compute everything needed for 1 point in time. **/

		if(pM->max2 == 0) {
			getshc(1, pM->shc_idx, pM->max1, gh1);
			getshc(1, pM[1].shc_idx, pM[1].max1, gh2);
			nmax = interpsh(sdate, pM->yrmin, pM->max1,	pM[1].yrmin, pM[1].max1, 3);
			nmax = interpsh(sdate+1, pM->yrmin , pM->max1,pM[1].yrmin, pM[1].max1, 4);
		}
		else {
			getshc(1, pM->shc_idx, pM->max1, gh1);
			getshc(0, pM->shc_idx, pM->max2, gh2);
			nmax = extrapsh(sdate, pM->epoch, pM->max1, pM->max2, 3);
			nmax = extrapsh(sdate+1, pM->epoch, pM->max1, pM->max2, 4);
		}

		/* Do the first calculations */ 
		shval3(1, latitude, longitude, elev, nmax, 3,
			IEXT, EXT_COEFF1, EXT_COEFF2, EXT_COEFF3);
		dihf(3);
		shval3(1, latitude, longitude, elev, nmax, 4,
			IEXT, EXT_COEFF1, EXT_COEFF2, EXT_COEFF3);
		dihf(4);

		ddot = ((dtemp - d)*57.29578)*60;
		idot = ((itemp - i)*57.29578)*60;
		d = d*(57.29578);   i = i*(57.29578);
		hdot = htemp - h;   xdot = xtemp - x;
		ydot = ytemp - y;   zdot = ztemp - z;
		fdot = ftemp - f;
   
		/* Change d and i to deg and min */ 

		ddeg=(int)d;
		dmin=(d-(FLTYP)ddeg)*60;
		if(ddeg!=0)	dmin=fabs(dmin);
		ideg=(int)i;
		imin=(i-(FLTYP)ideg)*60;
		if(ideg!=0) imin=fabs(imin);

		/** Above will compute everything for 1 point in time.  **/

		/*  Output the final results. */

		printf("\n\n\n  Model: %s \n", pM->name);
		printf("  Latitude: %3.2f deg\n", latitude);
		printf("  Longitude: %4.2f deg\n", longitude);
		printf("  Elevation: ");
		printf("%.2f km\n", elev);

		printf("  Date of Interest: %4.2f", sdate);
		printf("\n\n  ----------------------------------------------");
		printf("----------------------------");
		printf("\n       D            I           H        X        Y");
		printf("        Z        F");
		printf("\n     (deg)        (deg)        (nt)     (nt)    ");
		printf(" (nt)     (nt)     (nt)");
		printf("\n %4dd %4.1fm  %4dd %4.1fm   %7.0f", ddeg,dmin,ideg,imin,h);
		printf("  %7.0f  %7.0f  %7.0f  %7.0f\n", x,  y, z, f);
		printf("\n     dD         dI        dH          dX        dY");
		printf("         dZ         dF");
		printf("\n   (min/yr)  (min/yr)   (nT/yr)     (nT/yr)   (nT/yr)");
		printf("    (nT/yr)    (nT/yr)");
		printf("\n %7.1f    %7.1f   %7.1f     ", ddot, idot, hdot);
		printf("%7.1f   %7.1f    %7.1f    %7.1f", xdot ,ydot, zdot, fdot);
		printf("\n  -------------------------------------------------");
		printf("-------------------------\n");
	}
	return 0;
}

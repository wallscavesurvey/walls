/*latlong.c -- part of gpslib.lib*/
#include <math.h>
#include <gpslib.h>

/* define constants */
static const double CONVERT  = 11930464.7111111111111;        /* 2^31 / 180 */
static const double WGSa     = 6378137.0;           /* WGS84 semimajor axis */
static const double WGSinvf  = 298.257223563;                  /* WGS84 1/f */

/****************************************************************************/
/* Converts latitude and longitude in decimal degrees from WGS84 to another */
/* datum or from another datum to WGS84. The arguments to this function     */
/* include a direction flag 'fromWGS84', pointers to double precision       */
/* latitude and longitude, and an index to the gDatum[] array.              */
/****************************************************************************/
int translate(int fromWGS84, double *latitude, double *longitude)
{
  double dx = gDatum[gps_datum].dx;
  double dy = gDatum[gps_datum].dy;
  double dz = gDatum[gps_datum].dz;
  
  double phi    = *latitude  * Pi / 180.0;
  double lambda = *longitude * Pi / 180.0;
  double a0, b0, es0, f0;               /* Reference ellipsoid of input data */
  double a1, b1, es1, f1;              /* Reference ellipsoid of output data */
  double psi;                                         /* geocentric latitude */
  double x, y, z;           /* 3D coordinates with respect to original datum */
  double psi1;                            /* transformed geocentric latitude */
  
  if (gps_datum == gps_nDatums-1)            /* do nothing if current datum is WGS84 */
    return 0;
		
  if (fromWGS84) {                        /* convert from WGS84 to new datum */
    a0 = WGSa;                                       /* WGS84 semimajor axis */
    f0 = 1.0 / WGSinvf;                                  /* WGS84 flattening */
    a1 = gEllipsoid[gDatum[gps_datum].ellipsoid].a;
    f1 = 1.0 / gEllipsoid[gDatum[gps_datum].ellipsoid].invf;
  }
  else {                                      /* convert from datum to WGS84 */
    a0 = gEllipsoid[gDatum[gps_datum].ellipsoid].a;          /* semimajor axis */
    f0 = 1.0 / gEllipsoid[gDatum[gps_datum].ellipsoid].invf;     /* flattening */
    a1 = WGSa;                                       /* WGS84 semimajor axis */
    f1 = 1 / WGSinvf;                                    /* WGS84 flattening */
    dx = -dx;
    dy = -dy;
    dz = -dz;
  }
  
  b0 = a0 * (1 - f0);                      /* semiminor axis for input datum */
  es0 = 2 * f0 - f0*f0;                                    /* eccentricity^2 */
  
  b1 = a1 * (1 - f1);                     /* semiminor axis for output datum */
  es1 = 2 * f1 - f1*f1;                                    /* eccentricity^2 */
  
  /* Convert geodedic latitude to geocentric latitude, psi */
  if (*latitude == 0.0 || *latitude == 90.0 || *latitude == -90.0)
    psi = phi;
  else
    psi = atan((1 - es0) * tan(phi));
  
  /* Calc x and y axis coordinates with respect to original ellipsoid */  
  if (*longitude == 90.0 || *longitude == -90.0) {
    x = 0.0;
    y = fabs(a0 * b0 / sqrt(b0*b0 + a0*a0*pow(tan(psi), 2.0)));
  }
  else {
    x = fabs((a0 * b0) /
	     sqrt((1 + pow(tan(lambda), 2.0)) * 
		  (b0*b0 + a0*a0 * pow(tan(psi), 2.0))));
    y = fabs(x * tan(lambda));
  }
	
  if (*longitude < -90.0 || *longitude > 90.0)
    x = -x;
  if (*longitude < 0.0)
    y = -y;
  
  /* Calculate z axis coordinate with respect to the original ellipsoid */
  if (*latitude == 90.0)
    z = b0;
  else if (*latitude == -90.0)
    z = -b0;
  else
    z = tan(psi) * sqrt((a0*a0 * b0*b0) / (b0*b0 + a0*a0 * pow(tan(psi), 2.0)));
  
  /* Calculate the geocentric latitude with respect to the new ellipsoid */
  psi1 = atan((z - dz) / sqrt((x - dx)*(x - dx) + (y - dy)*(y - dy)));
  
  /* Convert to geocentric latitude and save return value */
  *latitude = atan(tan(psi1) / (1 - es1)) * 180.0 / Pi;
  
  /* Calculate the longitude with respect to the new ellipsoid */
  *longitude = atan((y - dy) / (x - dx)) * 180.0 / Pi;
  
  /* Correct the resultant for negative x values */
  if (x-dx < 0.0) {
    if (y-dy > 0.0)
      *longitude = 180.0 + *longitude;
    else
      *longitude = -180.0 + *longitude;
  }
  return fabs(*longitude)<=180.0 && fabs(*latitude)<=90.0;
}

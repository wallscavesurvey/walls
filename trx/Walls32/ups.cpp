/* ups.c   -   Convert to and from UPS Grid Format */
#include <math.h>
#include <gpslib.h>

/* prototype functions */
static void calcPhi(double *phi, double e, double t);

/****************************************************************************/
/* Convert degree to UPS Grid.                                              */
/****************************************************************************/
void toUPS(double lat, double lon, double *x, double *y)
{
  double               a, t, e, es, rho;
  const double         k0     = 0.994;
  double               lambda = lon * Degree;
  double               phi    = fabs(lat * Degree);
  
  datumParams(&a, &es);
  e   = sqrt(es);
  t   = tan(Pi/4.0 - phi/2.0) / pow( (1.0 - e * sin(phi)) / 
          (1.0 + e * sin(phi)), (e/2.0) );
  rho = 2.0 * a * k0 * t / sqrt(pow(1.0+e, 1.0+e) * pow(1.0-e, 1.0-e));
  *x  = rho * sin(lambda);
  *y  = rho * cos(lambda);
  
  if (lat > 0.0) *y = -(*y);           /* Northern hemisphere */
  *x += 2.0e6;	                       /* Add in false easting and northing */
  *y += 2.0e6;
}


/****************************************************************************/
/* Convert UPS Grid to degree.                                              */
/****************************************************************************/
void fromUPS(int southernHemisphere, double x, double y, double *lat, 
	     double *lon)
{
  double               a, es, e, t, rho;
  const double         k0 = 0.994;
  
  datumParams(&a, &es);
  e = sqrt(es);
 
  /* Remove false easting and northing */
  x  -= 2.0e6;
  y  -= 2.0e6;
  
  rho = sqrt(x*x + y*y);
  t   = rho * sqrt(pow(1.0+e, 1.0+e) * pow(1.0-e, 1.0-e)) / (2.0 * a * k0);
  
  calcPhi(lat, e, t);
  *lat /= Degree;
  
  if (y != 0.0)
    t = atan(fabs(x/y));
  else {
    t = Pi / 2.0;
    if (x < 0.0) t = -t;
  }
  
  if (!southernHemisphere)
    y = -y;
  
  if (y < 0.0) t = Pi - t;
  if (x < 0.0) t = -t;
  
  *lon = t / Degree;
}


/****************************************************************************/
/* Calculate Phi.                                                           */
/****************************************************************************/
static void calcPhi(double *phi, double e, double t)
{
  double   old           = Pi/2.0 - 2.0 * atan(t);
  short    maxIterations = 20;
  
  while ((fabs((*phi - old) / *phi) > 1.0e-8) && maxIterations-- ) { 
    old  = *phi;
    *phi = Pi/ 2.0 - 2.0 * atan( t * pow((1.0 - e * sin(*phi)) / 
                                ((1.0 + e * sin(*phi))), (e / 2.0)) );
  }
}

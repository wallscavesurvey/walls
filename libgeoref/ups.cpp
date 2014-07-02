/* ups.c   -   Convert to and from UPS Grid Format */
#include <georef.h>

/* prototype functions */
static void calcPhi(double *phi, double e, double t);

/****************************************************************************/
/* Convert degree to UPS Grid.                                              */
/****************************************************************************/
void toUPS(double lat, double lon, double *x, double *y)
{
  double               a, t, e, es, rho;
  const double         k0     = 0.994;
  double               lambda = lon * (GEO_PI/180);
  double               phi    = fabs(lat * (GEO_PI/180));
  
  datumParams(&a, &es);
  e   = sqrt(es);
  t   = tan(GEO_PI/4.0 - phi/2.0) / pow( (1.0 - e * sin(phi)) / 
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
  *lat /= (GEO_PI/180);
  
  if (y != 0.0)
    t = atan(fabs(x/y));
  else {
    t = GEO_PI / 2.0;
    if (x < 0.0) t = -t;
  }
  
  if (!southernHemisphere)
    y = -y;
  
  if (y < 0.0) t = GEO_PI - t;
  if (x < 0.0) t = -t;
  
  *lon = t / (GEO_PI/180);
}


/****************************************************************************/
/* Calculate Phi.                                                           */
/****************************************************************************/
static void calcPhi(double *phi, double e, double t)
{
  double   old           = GEO_PI/2.0 - 2.0 * atan(t);
  short    maxIterations = 20;
  
  while ((fabs((*phi - old) / *phi) > 1.0e-8) && maxIterations-- ) { 
    old  = *phi;
    *phi = GEO_PI/ 2.0 - 2.0 * atan( t * pow((1.0 - e * sin(*phi)) / 
                                ((1.0 + e * sin(*phi))), (e / 2.0)) );
  }
}

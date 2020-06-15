/*latlong.c -- part of gpslib.lib*/
#include <georef.h>
#include <assert.h>
#include <float.h>

/* define constants */
static const double CONVERT = 11930464.7111111111111;        /* 2^31 / 180 */
static const double WGSa = 6378137.0;           /* WGS84 semimajor axis */
static const double WGSinvf = 298.257223563;                  /* WGS84 1/f */

/****************************************************************************/
/* Converts latitude and longitude in decimal degrees from WGS84 to another */
/* datum or from another datum to WGS84. The arguments to this function     */
/* include a direction flag 'fromWGS84', pointers to double precision       */
/* latitude and longitude, and an index to the geo_gDatum[] array.              */
/****************************************************************************/
void geo_FromToWGS84(bool fromWGS84, double *latitude, double *longitude, int datum)
{
	if ((geo_datum = datum) == geo_WGS84_datum) return;

	double dx = geo_gDatum[geo_datum].dx;
	double dy = geo_gDatum[geo_datum].dy;
	double dz = geo_gDatum[geo_datum].dz;

	double phi = *latitude  * (GEO_PI / 180);
	double lambda = *longitude * (GEO_PI / 180);
	double a0, b0, es0, f0;               /* Reference ellipsoid of input data */
	double a1, b1, es1, f1;              /* Reference ellipsoid of output data */
	double psi;                                         /* geocentric latitude */
	double x, y, z;           /* 3D coordinates with respect to original datum */
	double psi1;                            /* transformed geocentric latitude */

	if (fromWGS84) {                        /* convert from WGS84 to new datum */
		a0 = WGSa;                                       /* WGS84 semimajor axis */
		f0 = 1.0 / WGSinvf;                                  /* WGS84 flattening */
		a1 = geo_gEllipsoid[geo_gDatum[geo_datum].ellipsoid].a;
		f1 = 1.0 / geo_gEllipsoid[geo_gDatum[geo_datum].ellipsoid].invf;
	}
	else {                                      /* convert from datum to WGS84 */
		a0 = geo_gEllipsoid[geo_gDatum[geo_datum].ellipsoid].a;          /* semimajor axis */
		f0 = 1.0 / geo_gEllipsoid[geo_gDatum[geo_datum].ellipsoid].invf;     /* flattening */
		a1 = WGSa;                                       /* WGS84 semimajor axis */
		f1 = 1 / WGSinvf;                                    /* WGS84 flattening */
		dx = -dx;
		dy = -dy;
		dz = -dz;
	}

	b0 = a0 * (1 - f0);                      /* semiminor axis for input datum */
	es0 = 2 * f0 - f0 * f0;                                    /* eccentricity^2 */

	b1 = a1 * (1 - f1);                     /* semiminor axis for output datum */
	es1 = 2 * f1 - f1 * f1;                                    /* eccentricity^2 */

	/* Convert geodedic latitude to geocentric latitude, psi */
	if (*latitude == 0.0 || *latitude == 90.0 || *latitude == -90.0)
		psi = phi;
	else
		psi = atan((1 - es0) * tan(phi));

	/* Calc x and y axis coordinates with respect to original ellipsoid */
	if (*longitude == 90.0 || *longitude == -90.0) {
		x = 0.0;
		y = fabs(a0 * b0 / sqrt(b0*b0 + a0 * a0*pow(tan(psi), 2.0)));
	}
	else {
		x = fabs((a0 * b0) /
			sqrt((1 + pow(tan(lambda), 2.0)) *
			(b0*b0 + a0 * a0 * pow(tan(psi), 2.0))));
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
		z = tan(psi) * sqrt((a0*a0 * b0*b0) / (b0*b0 + a0 * a0 * pow(tan(psi), 2.0)));

	/* Calculate the geocentric latitude with respect to the new ellipsoid */
	psi1 = atan((z - dz) / sqrt((x - dx)*(x - dx) + (y - dy)*(y - dy)));

	/* Convert to geocentric latitude and save return value */
	*latitude = atan(tan(psi1) / (1 - es1)) * (180 / GEO_PI);

	/* Calculate the longitude with respect to the new ellipsoid */
	*longitude = atan((y - dy) / (x - dx)) * (180 / GEO_PI);

	/* Correct the resultant for negative x values */
	if (x - dx < 0.0) {
		if (y - dy > 0.0)
			*longitude = 180.0 + *longitude;
		else
			*longitude = -180.0 + *longitude;
	}
}

//Caution -- unless *zone==0, UTM will be forced to specified zone!
//Zone will be negative for southern hemisphere, +/-61 in polar regions --
bool geo_LatLon2UTM(double lat, double lon, int *zone, double *x, double *y, int datum)
{
	assert(abs(*zone) <= 60);

	if (fabs(lat) > 90.0 || fabs(lon) > 180.0) return false;

	double lon0;

	if (!*zone) {
		lon0 = 6.0 * floor(lon / 6.0) + 3.0; //floor(x) == largest integer not greater than x
		if (lon0 == 183) lon0 = 177; //DMcK 4/1998
	}
	else lon0 = (abs(*zone) - 1) * 6 - 180 + 3; //3 + 6 * (N - 1)

	geo_datum = datum;

	toTM(lat, lon, 0.0, lon0, 0.9996, x, y);

	/* false easting */
	*x = *x + 500000.0;

	/* false northing */
	if ((lat < 0.0 && *zone <= 0.0) || (lat >= 0.0 && *zone < 0.0)) *y += 10000000.0;

	if (!*zone) {
		*zone = ((int)lon0 + 183) / 6;
		if (lat < 0.0) *zone = -*zone;
	}
	return true;
}

int geo_GetZone(const double &lat, const double &lon)
{
	int zone;
	//if ((lat < -80.0) || (lat > 84.0) || (lon<-181.0) || (lon>181.0))  return 0;
	if (fabs(lon) > 180.0) return 0;

	double lon0 = 6.0 * floor(lon / 6.0) + 3.0;
	if (lon0 == 183) lon0 = 177;
	zone = ((int)lon0 + 183) / 6;
	if (lat < 0.0) {
		zone = -zone;
	}
	return zone;
}

void geo_UTM2LatLon(int zone, double x, double y, double *lat, double *lon, int datum, double *conv /*=NULL*/)
{
	double lon0;
	int southernHemisphere = 0;

	geo_datum = datum;

	if (zone < 0) {
		zone = -zone;
		southernHemisphere++;
	}

	//  if (zone<=60) {

	lon0 = (double)(-183 + 6 * zone);

	/* remove false northing for southern hemisphere and false easting */
	//if(southernHemisphere) y = 1.0e7 - y; ???
	if (southernHemisphere) y -= 10000000.0;
	x -= 500000.0;

	fromTM(x, y, 0.0, lon0, 0.9996, lat, lon);

	//  }
	// else
	//    fromUPS(southernHemisphere, x, y, lat, lon);


	if (southernHemisphere && *lat == 0.0) {
		if (conv) *conv = 0.0;
		*lat = -DBL_MIN; //dmck
	}
	else if (conv) {
		*conv = sin(*lat*GEO_PI / 180)*(*lon - lon0);
	}

}




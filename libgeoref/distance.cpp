
#include <float.h>
#include <math.h>

//This is the Haversine formula for great-circle distance between two points.
//The implementation following this one that's commented out is from Julian Cayzac's blog.
//Unlike the Vincenty formula, which is accurate to within 0.5 mm, the Haversine formula
//ignores ellipsoidal effects and is supposedly accurate to around 0.3%.

double MetricDistance(double lat1, double lon1, double lat2, double lon2)
{
	/*Return distance in kilometers --*/
#define DEG_TO_RAD 0.01745329251994329577
#define EARTH_RADIUS_KM 6372.797560856
#ifdef _DEBUG
	double R;
#endif
	double sdlon = sin((lon2 - lon1)*(DEG_TO_RAD / 2));
	double sdlat = sin((lat2 - lat1)*(DEG_TO_RAD / 2));
	double sqrta = sqrt(sdlat*sdlat + cos(lat1*DEG_TO_RAD)*cos(lat2*DEG_TO_RAD)*sdlon*sdlon);
	if (sqrta > 1.0) sqrta = 1.0;

#ifdef _DEBUG
	R = EARTH_RADIUS_KM - 21 * fabs(sin((lat1 + lat2)*(DEG_TO_RAD / 2)));
	return 2 * R * asin(sqrta);
#else 
	return 2 * (EARTH_RADIUS_KM - 21 * fabs(sin((lat1 + lat2)*(DEG_TO_RAD / 2)))) * asin(sqrta);
#endif
}

double MetersPerDegreeLon(double lat)
{
	//Return distance in meters of a degree of longitude along a latitude line --
#define DEG_TO_RAD 0.01745329251994329577
#define EARTH_RADIUS_KM 6372.797560856

	lat *= DEG_TO_RAD;
	//sin((DEG_TO_RAD/2)) == 0.0087265354983739347
	return 2000.0 * (EARTH_RADIUS_KM - 21 * fabs(sin(lat))) * asin(0.0087265354983739347*cos(lat));
}

#if 0
//From http://blog.brainlex.com/2008/10/arc-and-distance-between-two-points-on.html

/// @brief The usual PI/180 constant
static const double DEG_TO_RAD = 0.017453292519943295769236907684886;
/// @brief Earth's quatratic mean radius for WGS-84
static const double EARTH_RADIUS_IN_METERS = 6372797.560856;

/** @brief Computes the arc, in radian, between two WGS-84 positions.
  *
  * The result is equal to <code>Distance(from,to)/EARTH_RADIUS_IN_METERS</code>
  *    <code>= 2*asin(sqrt(h(d/EARTH_RADIUS_IN_METERS )))</code>
  *
  * where:<ul>
  *    <li>d is the distance in meters between 'from' and 'to' positions.</li>
  *    <li>h is the haversine function: <code>h(x)=sin²(x/2)</code></li>
  * </ul>
  *
  * The haversine formula gives:
  *    <code>h(d/R) = h(from.lat-to.lat)+h(from.lon-to.lon)+cos(from.lat)*cos(to.lat)</code>
  *
  * @sa http://en.wikipedia.org/wiki/Law_of_haversines
  */
double ArcInRadians(const Position& from, const Position& to) {
	double latitudeArc = (from.lat - to.lat) * DEG_TO_RAD;
	double longitudeArc = (from.lon - to.lon) * DEG_TO_RAD;
	double latitudeH = sin(latitudeArc * 0.5);
	latitudeH *= latitudeH;
	double lontitudeH = sin(longitudeArc * 0.5);
	lontitudeH *= lontitudeH;
	double tmp = cos(from.lat*DEG_TO_RAD) * cos(to.lat*DEG_TO_RAD);
	return 2.0 * asin(sqrt(latitudeH + tmp * lontitudeH));
}

/** @brief Computes the distance, in meters, between two WGS-84 positions.
  *
  * The result is equal to <code>EARTH_RADIUS_IN_METERS*ArcInRadians(from,to)</code>
  *
  * @sa ArcInRadians
  */
double DistanceInMeters(const Position& from, const Position& to) {
	return EARTH_RADIUS_IN_METERS * ArcInRadians(from, to);
}
#endif

#if 0
/*
Notes about accuracy:
From http://www.movable-type.co.uk/scripts/latlong-vincenty.html
Vincenty’s formula is accurate to within 0.5mm, or 0.000015", on the ellipsoid being used.
Calculations based on a spherical model, such as the (much simpler) Haversine, are accurate
to around 0.3% (which is still good enough for most purposes, of course).

Note: the accuracy quoted by Vincenty applies to the theoretical ellipsoid being used, which
will differ (to varying degree) from the real earth geoid. If you happen to be located in
Colorado, 2km above msl, distances will be 0.03% greater. In the UK, if you measure the
distance from Land’s End to John O’ Groats using WGS-84, it will be 28m (0.003%) greater
than using the Airy ellipsoid, which provides a better fit for the UK.
*/

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  */
/* Vincenty Inverse Solution of Geodesics on the Ellipsoid (c) Chris Veness 2002-2012             */
/*                                                                                                */
/* from: Vincenty inverse formula - T Vincenty, "Direct and Inverse Solutions of Geodesics on the */
/*       Ellipsoid with application of nested equations", Survey Review, vol XXII no 176, 1975    */
/*       http://www.ngs.noaa.gov/PUBS_LIB/inverse.pdf                                             */
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  */

/**
 * Calculates geodetic distance between two points specified by latitude/longitude using
 * Vincenty inverse formula for ellipsoids
 *
 * @param   {Number} lat1, lon1: first point in decimal degrees
 * @param   {Number} lat2, lon2: second point in decimal degrees
 * @returns (Number} distance in metres between points
 */

 //This has been tested -- results match those at www.movable-type.co.uk
double distVincenty(double lat1, double lon1, double lat2, double lon2)
{
	static const double a = 6378137.0, b = 6356752.314245, f = 1 / 298.257223563;  // WGS-84 ellipsoid params
	double L = (lon2 - lon1)*DEG_TO_RAD;
	double U1 = atan((1 - f) * tan(lat1*DEG_TO_RAD));
	double U2 = atan((1 - f) * tan(lat2*DEG_TO_RAD));
	double sinU1 = sin(U1), cosU1 = cos(U1);
	double sinU2 = sin(U2), cosU2 = cos(U2);
	double cosSqAlpha, cosSigma, cos2SigmaM, sinSigma, sigma;

	double lambda = L, lambdaP;
	int iterLimit = 100;
	do {
		double sinLambda = sin(lambda), cosLambda = cos(lambda);
		sinSigma = sqrt((cosU2*sinLambda) * (cosU2*sinLambda) +
			(cosU1*sinU2 - sinU1 * cosU2*cosLambda) * (cosU1*sinU2 - sinU1 * cosU2*cosLambda));
		if (sinSigma == 0) return 0.0;  // co-incident points
		cosSigma = sinU1 * sinU2 + cosU1 * cosU2*cosLambda;
		sigma = atan2(sinSigma, cosSigma);
		double sinAlpha = cosU1 * cosU2 * sinLambda / sinSigma;
		cosSqAlpha = 1 - sinAlpha * sinAlpha;
		cos2SigmaM = cosSigma - 2 * sinU1*sinU2 / cosSqAlpha;
		if (_isnan(cos2SigmaM)) cos2SigmaM = 0;  // equatorial line: cosSqAlpha=0 (§6)
		double C = f / 16 * cosSqAlpha*(4 + f * (4 - 3 * cosSqAlpha));
		lambdaP = lambda;
		lambda = L + (1 - C) * f * sinAlpha *
			(sigma + C * sinSigma*(cos2SigmaM + C * cosSigma*(-1 + 2 * cos2SigmaM*cos2SigmaM)));
	} while (abs(lambda - lambdaP) > 1e-12 && --iterLimit > 0);

	if (iterLimit == 0) return -1.0;  // formula failed to converge

	double uSq = cosSqAlpha * (a*a - b * b) / (b*b);
	double A = 1 + uSq / 16384 * (4096 + uSq * (-768 + uSq * (320 - 175 * uSq)));
	double B = uSq / 1024 * (256 + uSq * (-128 + uSq * (74 - 47 * uSq)));
	double deltaSigma = B * sinSigma*(cos2SigmaM + B / 4 * (cosSigma*(-1 + 2 * cos2SigmaM*cos2SigmaM) -
		B / 6 * cos2SigmaM*(-3 + 4 * sinSigma*sinSigma)*(-3 + 4 * cos2SigmaM*cos2SigmaM)));

	return b * A*(sigma - deltaSigma);

	//double s = b*A*(sigma-deltaSigma);
	//s = s.toFixed(3); // round to 1mm precision
	// return s;

	// note: to return initial/final bearings in addition to distance, use something like:
	//var fwdAz = Math.atan2(cosU2*sinLambda,  cosU1*sinU2-sinU1*cosU2*cosLambda);
	//var revAz = Math.atan2(cosU1*sinLambda, -sinU1*cosU2+cosU1*sinU2*cosLambda);
	//return { distance: s, initialBearing: fwdAz.toDeg(), finalBearing: revAz.toDeg() };
}
#endif
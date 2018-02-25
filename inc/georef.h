/*georef.h*/

#ifndef __GEOREF_H
#define __GEOREF_H

#ifndef _INC_MATH
#include <math.h>
#endif

#define GEO_PI 3.14159265358979323846

// http://cvs.sourceforge.net/viewcvs.py/wikipedia/extensions/gis/transversemercator.php?rev=1.2&view=log

/**
 *  Tranverse Mercator transformations
 */
class CTransverseMercator
{
	/* public: */
	/* Reference Ellipsoid, default to WGS-84 */
	double Radius;           /* major semi axis = a */
	double Eccentricity; /* square of eccentricity */
	double Northing;
	double Easting;
	/* Transverse Mercator parameters */
	double Scale;
	double Easting_Offset;
	double Northing_Offset;
	double Northing_Offset_South; /* for Southern hemisphere */
	int Zone;

	CTransverseMercator()
		: Radius(6378137)
		, Eccentricity(0.006694379990)
		, Scale(0.9996)
		, Easting_Offset(500000.0)
		, Northing_Offset(0.0)
		, Northing_Offset_South(10000000.0) {}

	/* Flattening = f = (a-b) / a */
	/* Inverse flattening = 1/f = 298.2572236 */
	/* Minor semi axis b = a*(1-f) */
	/* Eccentricity e = sqrt(a^2 - b^2)/a = 0.081819190843 */


	/**
	 *  Convert latitude, longitude in decimal degrees to
	 *  UTM Zone, Easting, and Northing
	 */
	void LatLon2UTM(double latitude, double longitude )
	{
		Zone=LatLon2UTMZone(latitude,longitude);
		LatLonZone2UTM(latitude,longitude,Zone);
	}

	/**
	 *  Find UTM zone from latitude and longitude
	 */
	int LatLon2UTMZone(double latitude, double longitude )
	{
		double longitude2 = longitude - (int)((longitude + 180)/360) * 360;
		int zone;

		if (latitude >= 56.0 && latitude < 64.0	 && longitude2 >= 3.0 && longitude2 < 12.0) {
			zone = 32;
		}
		else if(latitude >= 72.0 && latitude < 84.0 && longitude2 >= 0.0 && longitude2 < 42.0) {
			zone = (longitude2 < 9.0) ? 31
			     : ((longitude2 < 21.0) ? 33 : ((longitude2 < 33.0) ? 35 :  37));
		}
		else {
			zone = (int) ((longitude2 + 180)/6) + 1;
		}
		//int c = (int)( (latitude + 96) / 8 );
				  /* 000000000011111111112222 */
				  /* 012345678901234567890134 */
		//return zone.substr("CCCDEFGHJKLMNPQRSTUVWXXX",c,1);
		return zone;
	}

	/**
	 *  Convert latitude, longitude in decimal degrees to
	 *  UTM Easting and Northing in a specific zone
	 *
	 *  \return false if problems
	 */
	bool LatLonZone2UTM( double latitude, double longitude, int zone )
	{
		return LatLonOrigin2TM(latitude,longitude,0.0,(zone - 1)*6 - 180 + 3);
	}

	double find_M (double lat_rad )
	{
		double e = Eccentricity;

		return Radius
			* ( ( 1 - e/4 - 3 * e * e/64
			      - 5 * e * e * e/256
			    ) * lat_rad
			  - ( 3 * e/8 + 3 * e * e/32
			      + 45 * e * e * e/1024
			    ) * sin(2 * lat_rad)
			  + ( 15 * e * e/256 +
			      45 * e * e * e/1024
			    ) * sin(4 * lat_rad)
			  - ( 35 * e * e * e/3072
			    ) * sin(6 * lat_rad) );
	}

	double deg2rad( double deg )
	{
		return (GEO_PI/180) * deg;
	}

	/**
	 *  Convert latitude, longitude in decimal degrees to
	 *  TM Easting and Northing based on a specified origin
	 *
	 *  \return false if problems
	 */
	bool LatLonOrigin2TM( double latitude, double longitude,
				  double latitude_origin, double longitude_origin )
	{
		if (longitude < -180 || longitude > 180 || latitude < -80 || latitude > 84) {
			// UTM not defined in this range
			return false;
		}
		double longitude2 = longitude - (int)((longitude + 180)/360) * 360;

		double lat_rad = deg2rad( latitude );

		double e = Eccentricity;
		double e_prime_sq = e / (1-e);

		double v = Radius / sqrt(1 - e * sin(lat_rad)*sin(lat_rad));
		double T = pow( tan(lat_rad), 2);
		double C = e_prime_sq * pow( cos(lat_rad), 2);
		double A = deg2rad( longitude2 -longitude_origin )
		     * cos(lat_rad);
		double M = find_M( lat_rad );
		double M0 =( latitude_origin != 0.0)?find_M(deg2rad( latitude_origin )):0.0;

		Northing = Northing_Offset + Scale *
			    ( (M - M0) + v*tan(lat_rad) * 
			      ( A*A/2 
				+ (5 - T + 9*C + 4*C*C) * pow(A,4)/24
				+ (61 - 58*T + T*T 
				+ 600*C - 330*e_prime_sq) * pow(A,6)/720 ));

		Easting = Easting_Offset + Scale * v *
			   ( A 
			     + (1-T+C)*pow(A,3)/6
			     + (5 - 18*T + pow(T,2) + 72*C 
				- 58 * e_prime_sq)*pow(A,5)/120 );

		// FIXME: Uze zone_letter
		// if (ord(zone_letter) < ord('N'))
		if (latitude < 0) Northing += Northing_Offset_South;

		return true;
	}
};

//====================================================================

/*GPSLIB.H -- UTM Conversion library header
  Modified from GPSTRANS/MacGPS sources (11/14/97 - DMcK)
*/

/*CAUTION! This must agree with definition in datum.c --*/
#define geo_nDatums 28
#define geo_NAD27_datum 15
#define geo_NAD83_datum 19
#define geo_WGS84_datum (geo_nDatums-1)

//latlong.cpp --
//Caution -- unless *zone==0, UTM will be forced to specified zone!
bool geo_LatLon2UTM(double lat,double lon,int *zone,double *x,double *y,int datum);
void geo_UTM2LatLon(int zone, double x, double y,double *lat, double *lon,int datum,double *conv=0);
void geo_FromToWGS84(bool fromWGS84,double *latitude,double *longitude,int datum);
int geo_GetZone(const double &lat,const double &lon);

//tm.cpp --
void toTM(double lat, double lon, double lat0, double lon0, double k0, double *x, double *y);
void fromTM(double x, double y, double lat0, double lon0, double k0, double *lat, double *lon);

//ups.cpp --
void toUPS(double lat, double lon, double *x, double *y);
void fromUPS(int southernHemisphere, double x, double y, double *lat, double *lon);

//datum.cpp --
void datumParams(double *a, double *es);

//dms.cpp --
char	*toDMS(double a);
char	*toDM(double a);
double	DMStoDegrees(char *s);
double	DMtoDegrees(char *s);

typedef struct {
	char *name;
	short ellipsoid;
	short dx;
	short dy;
	short dz;
} GEO_DATUM;

struct GEO_ELLIPSOID {
	/* name of ellipsoid --
	char *name;
	*/
	double a;		/* semi-major axis, meters */
	double invf;	/* 1/f */
};

extern int geo_datum;
extern GEO_DATUM const geo_gDatum[geo_nDatums];
extern struct GEO_ELLIPSOID const geo_gEllipsoid[];

#define GEO_ERR_INIT		1
#define GEO_ERR_VERSION		2
#define GEO_ERR_ELEVRANGE	3
#define GEO_ERR_CVTUTM2LL	4
#define GEO_ERR_OUTOFZONE	5
#define GEO_ERR_TRANSLATE	6
#define GEO_ERR_LLRANGE		7

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
	float conv;
	char *datumname;
	int zone;
	int zone2;
	int	datum;
	int format;
	int fcn;
} GEODATA;

#define GEO_VERSION			1

#define GEO_GETVERSION		0
#define GEO_GETDATUMCOUNT	1
#define GEO_GETDATUMNAME	2
#define GEO_GETNAD27		3
#define GEO_CVTUTM2LL	 	4
#define GEO_CVTUTM2LL2	 	5
#define GEO_CVTLL2UTM		6
#define GEO_FIXFORMAT		7
#define GEO_CVTLL2UTM2		8
#define GEO_CVTDATUM		100
#define GEO_CVTDATUM2		200

int geo_GetData(GEODATA *pD);
double MetricDistance(double lat1,double lon1,double lat2,double lon2); //in distance.cpp (libgeoref)
double _inline MetersPerDegreeLon(double lat);
double distVincenty(double lat1, double lon1, double lat2, double lon2);


double _inline MetersPerDegreeLat(double lat)
{
	//Return distance in meters of a degree of latitude along a meridian.
	return (6378-21*fabs(sin(lat*0.017453293))) * 17.453293;
	//At the equator 1 degree==6378*17.453293==111,317.1 meters.
	//At the poles 1 degree==((6378-21)*17.453293==110,950.6 meters
}

//For convenience --
#define GF(f) (GD.fcn=f,geo_GetData(&GD))
#define GF_NAD27() GF(GEO_GETNAD27)
#define GF_WGS84() (GF(GEO_GETDATUMCOUNT)-1)

#endif
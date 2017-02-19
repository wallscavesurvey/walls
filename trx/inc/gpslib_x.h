/*GPSLIB.H -- UTM Conversion library header
  Modified from GPSTRANS/MacGPS sources (11/14/97 - DMcK)
*/

#ifdef __cplusplus
extern "C" {
#endif

static const double Pi = 3.14159265358979323846;
/*static const double Degree = Pi/180.0; */
static const double Degree = 1.74532925199e-2;

#ifndef BYTE
typedef unsigned char BYTE;
#endif

void translate(int fromWGS84, double *latitude, double *longitude);
//Unless *zone==0, UTM will be forced to specified zone!
void LatLon2UTM(double lat,double lon,int *zone,double *x,double *y);
int OutOfZoneConv(float *conv,double lat,double lon,int zone); //assumes zone may not match lat/lon

/*Note: zone<0 in southern hemisphere, iabs(zone)=61 in polar regions*/
void DegToUTM(double lat, double lon, int *zone, double *x, double *y);
void UTMtoDeg(int zone, double x, double y,double *lat, double *lon);

void toTM(double lat, double lon, double lat0, double lon0, double k0, double *x, double *y);
void fromTM(double x, double y, double lat0, double lon0, double k0, double *lat, double *lon);

void toUPS(double lat, double lon, double *x, double *y);
void fromUPS(int southernHemisphere, double x, double y, double *lat, double *lon);
void datumParams(double *a, double *es);

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
} DATUM;

struct ELLIPSOID {
	/* name of ellipsoid --
	char *name;
	*/
	double a;		/* semi-major axis, meters */
	double invf;	/* 1/f */
};

extern int gps_datum;
extern DATUM const gDatum[];
extern struct ELLIPSOID const gEllipsoid[];
extern int gps_nDatums;

/*CAUTION! This must agree with definition in datum.c --*/
#define gps_NAD27_datum 15
#define gps_NAD83_datum 19
#define gps_WGS84_datum (gps_nDatums-1)

#ifdef __cplusplus
}
#endif

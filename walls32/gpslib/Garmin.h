static const double Pi = 3.14159265358979323846;
/*static const double Degree = Pi/180.0; */
static const double Degree = 1.74532925199e-2;


typedef unsigned char BYTE;

#define MAX_LENGTH 100	/* maximum length of Garmin binary message */
#define MAX_LINE 200	/* data file maximum line length */

/*#define OUTDRIVER "\p.AOut"
#define INDRIVER "\p.AIn" */

/* Garmin message type characters */
#define RTE_NAM 0x1d	/* Route name record */
#define RTE_WPT 0x1e	/* Route waypoint record */
#define ALM 	0x1f	/* Almanac record */
#define TRK 	0x22	/* Track record */
#define WPT 	0x23	/* Waypoint record */
#define GMNID   0x7e    /* Request Garmin ID */

/* Transfer types */
#define ALMANAC	0
#define ROUTE	1
#define TRACK	2
#define WAYPOINT	3

enum PROTOCOL {NONE, GARMIN, NMEA};

/* prototypes */
/*
int	serialOpen(enum PROTOCOL);
long	serialCharsAvail(void);
void	serialClose(void);
*/
short	getGPSMessage(void);
void	sendGPSMessage(BYTE *m, short length);

void	getGPSInfo(FILE *refNum, short type);
void	sendGPSInfo(FILE *refNum, short type);
void	saveFormat(char *);

double	int2deg(long n);
long	deg2int(double x);
char	*secs2dt(long secs, short offset);
long	dt2secs(char *dt, int offset);

void DegToUTM(double lat, double lon, char *zone, double *x, double *y);
void UTMtoDeg(short zone, short southernHemisphere, double x, double y,
				double *lat, double *lon);

void DegToKKJ(double lat, double lon, char *zone, double *x, double *y);
void KKJtoDeg(short zone, short southernHemisphere, double x, double y,
				double *lat, double *lon);

void toTM(double lat, double lon, double lat0, double lon0, double k0, double *x, double *y);
void fromTM(double x, double y, double lat0, double lon0, double k0, double *lat, double *lon);

void toUPS(double lat, double lon, double *x, double *y);
void fromUPS(short southernHemisphere, double x, double y, double *lat, double *lon);

void DegToBNG(double lat, double lon, char *zone, double *x, double *y);
void BNGtoDeg(char *zone, double x, double y, double *lat, double *lon);

void DegToITM(double lat, double lon, char *zone, double *x, double *y);
void ITMtoDeg(char *zone, double x, double y, double *lat, double *lon);

void datumParams(short datum, double *a, double *es);

char	*toDMS(double a);
char	*toDM(double a);
double	DMStoDegrees(char *s);
double	DMtoDegrees(char *s);

void	translate(short fromWGS84, double *latitude, double *longitude, short datumID);

struct DATUM {
	char *name;
	short ellipsoid;
	short dx;
	short dy;
	short dz;
};

struct ELLIPSOID {
	char *name;		/* name of ellipsoid */
	double a;		/* semi-major axis, meters */
	double invf;	        /* 1/f */
};


/* Messages declarations */

extern BYTE m1[], m2[];
extern BYTE p1[], p2[];
extern BYTE alm1[], alm2[];
extern BYTE trk1[], trk2[];
extern BYTE wpt1[], wpt2[];
extern BYTE rte1[], rte2[], rte3[];
extern BYTE almt[], rtet[], trkt[], wptt[];
extern BYTE gid2[], gid3[], gid4[], gid5[];
extern BYTE off1[], test[];
extern BYTE tim1[];

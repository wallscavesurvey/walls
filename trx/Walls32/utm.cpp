/* utm.c -- Convert to and from UTM Grid Format*/
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <gpslib.h>
#include <assert.h>
#include <float.h>

/* define constants */
static const double lat0 = 0.0;
/* reference transverse mercator latitude --*/
static const double k0   = 0.9996;

int DegToUTMZone(double lat,double lon)
{
/*	UTM Zone = int(1+([Longitude]+180.0)/6.0); //for -180 <= Longitude < 180
	A point on a west UTM boundary will be in that zone.
*/
	int zone;
	double lon0 = 6.0 * floor(lon / 6.0) + 3.0;
	if(lon0==183) lon0=177; //DMcK 4/1998
	zone=((int)lon0 +183) / 6;
	if (lat < 0.0) zone=-zone;
	return zone;
}

static int __inline DegToUTMUPSZone(double lat,double lon)
{
	int zone;
	if(lat>=84.0) zone=61;
	else if(lat<-80.0) zone=-61;
	else zone=DegToUTMZone(lat,lon);
	return zone;
}

/*Zone will be negative for southern hemisphere, +/-61 in polar regions*/
int DegToUTMUPS(double lat,double lon,int *zon,double *x,double *y,double *conv)
{
	//Force to specified zone if *zone!=0.
	//UPS if abs(zone)==61.
	//Return actual zone if change successful,
	//Return 0 if change not allowed due to proximity failure or bad argument,

	#define MAX_LATDIFF 1.0

	int m,zone,zone0;

	//check range --
	if(abs(*zon)>61 || fabs(lat)>90.0 || fabs(lon)>180.0)
		return 0;

	//first, get zone0 == true utm zone (regardless of lat) --
	/*
	double lon0 = 6.0 * floor(lon / 6.0) + 3.0;
	if(lon0==183) lon0=177; //DMcK 4/1998
	zone0=((int)lon0 +183) / 6;
	if (lat < 0.0) zone0=-zone0;
	*/

	zone0=DegToUTMZone(lat,lon); //see code above

	if(!*zon) {
		//return only utm zone
		zone=zone0;
	}
	else {
		//force to specic zone, can be ups if lat >= 83.5 or lat <= -84.5
		if(abs(*zon)<61) {
			zone=*zon;
		}
		else {
			int iups; //1,-1 or 0
			if(lat>=83.5) iups=1;
			else if(lat<=-79.5) iups=-1;
			else return 0;
			if(*zon*iups<0) return 0;
			toUPS(lat, lon, x, y);
			*conv=iups*lon;
			return *zon;
		}
	}

	if(zone!=zone0) {
		//forcing normal utm to a specific zone that's different --
		if(zone*zone0<0 && fabs(lat)>MAX_LATDIFF)
			return 0;
	}

	//don't allow a zone shift greater than 1
	m=abs(abs(zone)-abs(zone0));
	if(m>2 && m<58) return 0;

	//compute m = meridian of forced zone, -177 ..177
	m=(abs(zone) - 1)*6 - 180 + 3;

	//compute utm grid convergence
	*conv=sin(Degree*lat)*(lon-m);

	//convert lat/long to UTM --
	toTM(lat, lon, 0.0, m, 0.9996, x, y);

	/* false easting */
	*x = *x + 500000.0;

	/* false northing */
	if (zone<0) *y += 10000000.0;

	*zon=zone; //forced zone

	return zone0; //actual zone
}

/****************************************************************************/
/* Convert UTM Grid to degree.                                              */
/****************************************************************************/
int UTMUPStoDeg(int zone, double x, double y,double *lat, double *lon,double *conv)
{

	double lon0;
	int iret,azone=abs(zone);

	if(!azone || azone>61) return 0;
	if(azone==61) {
		//UPS to degrees --
		if(zone>0) {
			if(x<1200000 || x>2800000 || y<1200000 || y>2800000) return 0;
		}
		else {
			if(x<700000 || x>3100000 || y<700000 || y>3100000) return 0;
		}
		fromUPS(zone<0, x, y, lat, lon);
		if(zone<0 && *lat>-79.5 || zone>0 && *lat<83.5) return 0; //added
		if(conv) *conv=(zone<0)?-*lon:*lon;
		return zone;
		//goto _retzone;
	}

	//UTM to degrees --
/*
	if(x<-600000 || x>1600000) return 0;
	if(zone>0) {
		if(y<-9100000 || y>9600000) return 0;
	}
	else {
		if(y<900000 || y>19600000) return 0;
	}
*/
    lon0 = (double)(-183 + 6 * abs(zone));

    /* remove false northing for southern hemisphere and false easting */
	if(zone<0) y -= 10000000.0; 
    x -= 500000.0;

    fromTM(x, y, lat0, lon0, k0, lat, lon);
	if(zone<0 && *lat==0.0) {
		if(conv) *conv=0.0;
		*lat=-DBL_MIN; //dmck
	}
	else if(conv) {
		*conv=sin(*lat*Degree)*(*lon-lon0);
	}

//_retzone:
	//return actual zone, different if input coords not in specified zone --
	iret=DegToUTMZone(*lat,*lon);
	azone=abs(abs(iret)-azone);
	if(azone>2 && azone<58) return 0;
	return iret;
}


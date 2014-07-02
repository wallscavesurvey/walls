/* utm.c -- Convert to and from UTM Grid Format*/
#include <georef.h>

/* define constants */
static const double lat0 = 0.0;
/* reference transverse mercator latitude --*/
//static const double k0   = 0.9996;

/*Zone will be negative for southern hemisphere, +/-61 in polar regions*/
void geo_DegToUTM(double lat, double lon, int *zone, double *x, double *y)
{
  /*char   nz;*/
  double lon0;

#if 0
  if ((lat >= -80.0) && (lat <= 84.0)) {
	/*
    nz = 'C'+((short)(lat + 80.0)) / 8;
    // skip 'I' and 'O' --
    if (nz > 'H') ++nz;
    if (nz > 'N') ++nz;
	*/
#endif
    lon0 = 6.0 * floor(lon / 6.0) + 3.0;
	if(lon0==183) lon0=177; //DMcK 4/1998
	
    *zone=((int)lon0 +183) / 6;

    toTM(lat, lon, lat0, lon0, 0.9996, x, y);

    /* false easting */
    *x = *x + 500000.0;

    /* false northing for southern hemisphere */
    if (lat < 0.0) {
	  //*y = 10000000.0 - *y; ??? 
	  *y += 10000000.0;
	  *zone=-*zone;
	}

#if 0
  }
  else {
	  *zone=(lat>0.0)?61:-61;
	/*
    if (lat > 0.0) {
      if (lon < 0.0) nz = 'Y';
      else nz = 'Z';
	}
    else {
      if (lon < 0.0) nz = 'A';
      else nz = 'B';
	}
	*/
    toUPS(lat, lon, x, y);
  }
#endif
}

/****************************************************************************/
/* Convert UTM Grid to degree.                                              */
/****************************************************************************/
void geo_UTMtoDeg(int zone, double x, double y,double *lat, double *lon)
{
	double lon0;
	int southernHemisphere=0; 

	if(zone<0) {
	  zone=-zone;
	  southernHemisphere++;
	}

//  if (zone<=60) {

    lon0 = (double)(-183 + 6 * zone);

    /* remove false northing for southern hemisphere and false easting */
    //if(southernHemisphere) y = 1.0e7 - y; ???
	if(southernHemisphere) y -= 10000000.0; 
    x -= 500000.0;

    fromTM(x, y, lat0, lon0, 0.9996, lat, lon);
//  }
// else
//    fromUPS(southernHemisphere, x, y, lat, lon);
}


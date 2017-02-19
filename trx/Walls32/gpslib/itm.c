/****************************************************************************/
/*                                                                          */
/* ./gps/dms.c   -   Convert to various position formats                    */
/*                                                                          */
/* This file is part of gpstrans - a program to communicate with garmin gps */
/* Parts are taken from John F. Waers (jfwaers@csn.net) program MacGPS.     */
/*                                                                          */
/*                                                                          */
/*    Copyright (c) 1995 by Carsten Tschach (tschach@zedat.fu-berlin.de)    */
/*                                                                          */
/* Permission  to use, copy,  modify, and distribute  this software and its */
/* documentation for non-commercial purpose, is hereby granted without fee, */
/* providing that the  copyright notice  appear in all copies and that both */
/* the  copyright notice and  this permission  notice appear in  supporting */
/* documentation.  I make no representations about  the suitability of this */
/* software  for any  purpose.  It is  provides "as is" without  express or */
/* implid warranty.                                                         */
/*                                                                          */
/****************************************************************************/
#include "defs.h"
#include "Garmin.h"
#include <math.h>



/****************************************************************************/
/* The reference latitude and k0 factor were determined empirically to fit  */
/* the GPS data.                                                            */
/****************************************************************************/

/* define constants */
static const char *zoneID = "VWXYZQRSTULMNOPFGHJKABCDE";
static const double lat0 = 53.4999886;    /* reference transverse mercator */
static const double lon0 = -8.0;                 /* latitude and longitude */
static const double k0   = 1.000035;



/****************************************************************************/
/* Convert degree to Irish Grid Format.                                     */
/****************************************************************************/
void DegToITM(double lat, double lon, char *zone, double *x, double *y)
{
  short X, Y;
  
  strcpy(zone, "--");
  toTM(lat, lon, lat0, lon0, k0, x, y);
 
  /* add false easting and northing and round to nearest meter */
  *x = floor(*x + 200000.0 + 0.5);
  *y = floor(*y + 250000.0 + 0.5);
  
  /* check for invalid range */
  if (*x < 0.0 || *x >500000.0 || *y < 0.0 || *y > 500000.0) {	

    /* return with zone = "--" and set x and y to 0.0 */
    *x = 0.0;
    *y = 0.0;
    return;
  }
  
  X = (long)*x / 100000;
  Y = (long)*y / 100000;
  
  *x = (long)*x % 100000;
  *y = (long)*y % 100000;
  
  zone[0] = 'I';
  zone[1] = zoneID[5 * Y + X];
}


/****************************************************************************/
/* Convert Irish Grid Format to degree.                                     */
/****************************************************************************/
void ITMtoDeg(char *zone, double x, double y, double *lat, double *lon)
{
  int n;
  
  /* Check for invalid zone */
  if (strcmp(zone, "--") == 0 || x < 0.0 || x > 100000.0 || y < 0.0 ||
      y > 100000.0) {
    *lat = 0.0;
    *lon = 0.0;
    return;
  }
  
  n = strchr(zoneID, zone[1]) - zoneID;
  
  x += (n % 5) * 100000.0 - 200000.0;
  y += (n / 5) * 100000.0 - 250000.0;
  
  fromTM(x, y, lat0, lon0, k0, lat, lon);
}


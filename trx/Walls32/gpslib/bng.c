/****************************************************************************/
/*                                                                          */
/* ./grid/bng.c   -   Convert to British Grid                               */
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
/* The k0 constant was empirically determined to give the best fit to the   */
/* data from the GPS 45 and the PCX5 program. It should be 0.9996, but this */
/* number resulted in slight errors in the data.                            */
/****************************************************************************/

/* define constants */
static const char *zoneID0 = "STUQRNOPLMHJKFGCDEABXYZVW";
static const char *zoneID1 = "VWXYZQRSTULMNOPFGHJKABCDE";
static const double lat0   = 49.0; /* reference transverse mercator latitude */
static const double lon0   = -2.0;	                    /* and longitude */
static const double k0     = 0.99960133;



/****************************************************************************/
/* Convert degree to British Grid Format.                                   */
/****************************************************************************/
void DegToBNG(double lat, double lon, char *zone, double *x, double *y)
{
  short X0, Y0, X1, Y1;
  
  strcpy(zone, "--");
  toTM(lat, lon, lat0, lon0, k0, x, y);
 
  /* add false easting and northing and round to nearest meter */ 
  *x = floor(*x + 400000.0 + 0.5);
  *y = floor(*y - 100000.0 + 0.5);

  /* check for invalid range */
  if (*x < 0.0 || *x >1.0e6 || *y < 0.0 || *y > 2.5e6) {

    /* return with zone = "--" and set x and y to 0.0 */
    *x = 0.0;
    *y = 0.0;
    return;
  }
  
  X0 = (long)*x / 500000;
  Y0 = (long)*y / 500000;
  
  X1 = ((long)*x / 100000) % 5;
  Y1 = ((long)*y / 100000) % 5;
  
  *x = (long)*x % 100000;
  *y = (long)*y % 100000;
  
  zone[0] = zoneID0[5 * Y0 + X0];
  zone[1] = zoneID1[5 * Y1 + X1];
}


/****************************************************************************/
/* Convert British Grid Format to degree.                                   */
/****************************************************************************/
void BNGtoDeg(char *zone, double x, double y, double *lat, double *lon)
{
  int m, n;
  
  /* Check for invalid zone */
  if (strcmp(zone, "--") == 0 || x < 0.0 || x > 100000.0 || y < 0.0 ||
      y > 100000.0) {
    *lat = 0.0;
    *lon = 0.0;
    return;
  }

  m = strchr(zoneID0, zone[0]) - zoneID0;
  n = strchr(zoneID1, zone[1]) - zoneID1;

  x += (m % 5) * 500000.0 + (n % 5) * 100000.0 - 400000.0;
  y += (m / 5) * 500000.0 + (n / 5) * 100000.0 + 100000.0;
  
  fromTM(x, y, lat0, lon0, k0, lat, lon);
}

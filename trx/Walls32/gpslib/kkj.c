/****************************************************************************/
/*                                                                          */
/* ./grid/kkj.c   -   Convert to and from KKJ Grid Format                   */
/*                                                                          */
/* This file is part of gpstrans - a program to communicate with garmin gps */
/* Parts are taken from John F. Waers (jfwaers@csn.net) program MacGPS.     */
/*                                                                          */
/*                                                                          */
/*    Copyright (c) 1995 by Carsten Tschach (tschach@zedat.fu-berlin.de)    */
/*    Copyright (c) 1996 by Janne Sinkkonen (janne@iki.fi)                  */
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


/* define constants */
static const double lat0 = 0.0;	   /* reference transverse mercator latitude */
static const double lon0 = 27.0;
static const double k0   = 1.0;


/****************************************************************************/
/* Convert degree to KKJ Grid.                                              */
/****************************************************************************/
void DegToKKJ(double lat, double lon, char *zone, double *x, double *y)
{
  char   nz;
    sprintf(zone, "27E");
    toTM(lat, lon, lat0, lon0, k0, x, y);

    /* false easting */
    *x = *x + 500000.0;
}


/****************************************************************************/
/* Convert KKJ Grid to degree.                                              */
/****************************************************************************/
void KKJtoDeg(short zone, short southernHemisphere, double x, double y, 
	      double *lat, double *lon)
{
  double lon0;
    x -= 500000.0;
    fromTM(x, y, lat0, lon0, k0, lat, lon);
}


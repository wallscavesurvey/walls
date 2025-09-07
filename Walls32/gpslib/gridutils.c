/****************************************************************************/
/*                                                                          */
/* ./grid/gridutils.c   -   Calculate datum variables                       */
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
#include "gpslib.h"

/****************************************************************************/
/* Calculate datum parameters.                                              */
/****************************************************************************/
void datumParams(double *a, double *es)
{
  double f = 1.0 / gEllipsoid[gDatum[gps_datum].ellipsoid].invf;   /* flattening */
    
  *es = 2 * f - f*f;                                       /* eccentricity^2 */
  *a  = gEllipsoid[gDatum[gps_datum].ellipsoid].a;             /* semimajor axis */
}


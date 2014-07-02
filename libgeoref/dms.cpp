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
#include <stdio.h>
#include "georef.h"

/****************************************************************************/
/* Convert degrees to dd mm'ss.s" (DMS-Format)                              */
/****************************************************************************/
char *toDMS(double a)
{
  short        neg = 0;
  double       d, m, s;
  static char  dms[20];

  if (a < 0.0) {
    a   = -a;
    neg = 1;
  }
  d = (double) ((int) a); 
  a = (a - d) * 60.0;
  m = (double) ((int) a);
  s = (a - m) * 60.0;
  
  if (s > 59.5) {
    s  = 0.0;
    m += 1.0;
  }
  if (m > 59.5) {
    m  = 0.0;
    d += 1.0;
  }
  if (neg)
    d = -d;
  
  sprintf(dms, "%.0f°%02.0f'%04.1f\"", d, m, s);
  return dms;
}


/****************************************************************************/
/* Convert dd mm'ss.s" (DMS-Format) to degrees.                             */
/****************************************************************************/
double DMStoDegrees(char *dms)
{
  int     neg = 0;
  int     d, m;
  double  s;

  sscanf(dms, "%d%d%lf", &d, &m, &s); 
  s = (double) (abs(d)) + ((double)m + s / 60.0) / 60.0;
  
  if (d >= 0) 
    return s;
  else
    return -s;
}


/****************************************************************************/
/* Convert degrees to dd mm.mmm' (DMM-Format)                               */
/****************************************************************************/
char *toDM(double a)
{
  short        neg = 0;
  double       d, m;
  static char  dm[13];
  
  if (a < 0.0) {
    a = -a;
    neg = 1;
  }

  d = (double) ( (int) a);
  m = (a - d) * 60.0;
  
  if (m > 59.5) {
    m  = 0.0;
    d += 1.0;
  }
  if (neg) d = -d;
  
  sprintf(dm, "%.0f°%06.3f'", d, m);
  return dm;
}


/****************************************************************************/
/* Convert dd mm.mmm' (DMM-Format) to degree.                               */
/****************************************************************************/
double DMtoDegrees(char *dms)
{
  short   neg = 0;
  short   d;
  double  m;
  
  sscanf(dms, "%d%lf", &d, &m);
  m = (double) (abs(d)) + m / 60.0;
  
  if (d >= 0)
    return m;
  else
    return -m;
}

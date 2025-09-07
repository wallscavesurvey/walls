/****************************************************************************/
/*                                                                          */
/* ./gps/calendar.c   -   Procedures to convert time and datum              */
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
#include "Prefs.h"


/* define constants */
#define START 2447892L                    /* Julian date for 00:00 12/31/89 */

/* prototype functions */
static long date2days(short mm, short dd, short yy);
static void days2date(long julian, short *mm, short *dd, short *yy);


/****************************************************************************/
/*  dt is the date and time in the format "mm/dd/yyyy hh:mm:ss" and secs    */
/*  is the number of seconds since 12/31/1989 00:00:00, as defined by the   */
/*  #define START                                                           */
/****************************************************************************/

/****************************************************************************/
/* Convert seconds since 01/01/90 00:00:00 in date.                         */
/****************************************************************************/
char *secs2dt(long secs, short offset)
{
  static char   dt[30];
  short         mm, dd, yy, h, m, s;
  long          days, rest;

  days = (long) ((secs + (long)(offset * 3600.0)) / (24L * 3600L));
  rest = (secs + (long)(offset * 3600.0)) - (long) (days * 24L * 3600L);

  days2date(days, &mm, &dd, &yy);
  
  h = rest / 3600L;
  m = (rest - (long)h * 3600L) / 60L;
  s = (rest - (long)h * 3600L) - ((long)m * 60L);
  sprintf(dt, "%02d/%02d/%04d %02d:%02d:%02d", mm, dd, yy, h, m, s);
  
  return dt;
}


/****************************************************************************/
/* Convert dat in seconds since 01/01/90 00:00:00                           */  
/****************************************************************************/
long dt2secs(char *dt, int offset)
{
  int mm, dd, yy, h, m, s;
  
  sscanf(dt, "%d/%d/%d %d:%d:%d", &mm, &dd, &yy, &h, &m, &s);
  return 24L * 3600L * date2days(mm, dd, yy)
    + 3600L * h + 60L * m + s - (long)(offset*3600.0);
}


/****************************************************************************/
/* Convert date in days since 01/01/90 00:00:00                             */
/****************************************************************************/
static long date2days(short mm, short dd, short yy)
{
  long   jul;
  short  ja, jy, jm;
  
  if (yy < 0) ++yy;
  if (mm > 2) {
    jy = yy;
    jm = mm + 1;
  } else {
    jy = yy - 1;
    jm = mm + 13;
  }
  ja  = 0.01 * jy;
  jul = (long)((long) (365.25 * jy) + (long) (30.6001 * jm) +
	       dd + 1720997 - ja + (long)(0.25 * ja));
  
  return jul-START;
}


/****************************************************************************/
/* Convert days since 01/01/90 00:00:00 in date.                            */
/****************************************************************************/
static void days2date(long days, short *mm, short *dd, short *yy)
{
  long ja, jalpha, jb, jc, jd, je;

  jalpha = (long) (((float)(days + START - 1867216) - 0.25) / 36524.25);

  ja = days + START + 1 + jalpha - (long)(0.25 * jalpha);
  jb = ja + 1524L;
  jc = 6680.0 + (long) (((double)(jb - 2439870) - 122.1) / 365.25);
  jd = (long) (365.25 * jc);
  je = (long) ((jb - jd) / 30.6001);

  *dd = jb - jd - (short)(30.6001 * je);
  *mm = je - 1;
  if (*mm > 12) *mm -= 12;
  *yy = jc - 4715;
  if (*mm >  2) --(*yy);
  if (*yy <= 0) --(*yy);
}

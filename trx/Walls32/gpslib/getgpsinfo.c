/****************************************************************************/
/*                                                                          */
/* ./gps/getgpsinfo.c   -   Receive data from GPS                           */
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
#include <time.h>


/* define external variables */
extern BYTE gGarminMessage[MAX_LENGTH];

/* define global variables */
static short  records;
static FILE   *FileRefNum;
static char   fileData[MAX_LINE];
static int    AlmanacSat = 0;
char          GPSVersionStr[255];


/* prototype functions */
static void parseGPS(void);
static void doWaypoint(void);
static void doTrack(void);
static void doRouteName(void);
static void doAlmanac(void);
static long number(BYTE *p);
static char *string(short n, BYTE *p);
static void FileWrite(char *data);
static float tofloat(unsigned char *p);
static short toshort(unsigned char *p);


/****************************************************************************/
/*   The following format is used to save information about the data        */
/*   in the data files. If changed, the function getFileData() in           */
/*   SendGPSInfo.c must also be changed.                                    */
/*                                                                          */
/*   Format: DDD  UTC Offset:  -6.00 hrs  Datum[061]: NAD27 CONUS           */
/*           |||              ||||||            |||                         */
/*   0         1         2         3         4         5         6          */
/*   0123456789012345678901234567890123456789012345678901234567890123456789 */
/****************************************************************************/


/****************************************************************************/
/* Write format information to output file.                                 */
/****************************************************************************/
void saveFormat(char *fileData)
{
  extern struct  PREFS gPrefs;
  extern struct  DATUM const gDatum[];
  char           format[4];
  
  switch (gPrefs.format) {
  case DMS: strcpy(format, "DMS"); break;
  case DMM: strcpy(format, "DMM"); break;
  case DDD: strcpy(format, "DDD"); break;
  case UTM: strcpy(format, "UTM"); break;
  case KKJ: strcpy(format, "KKJ"); break;
  case BNG: strcpy(format, "BNG"); break;
  case ITM: strcpy(format, "ITM"); break;
  default:  strcpy(format, "UNK"); break;
  }
  
  sprintf(fileData, "Format: %s  UTC Offset: %6.2f hrs  Datum[%03d]: %s\n",
	  format, gPrefs.offset, gPrefs.datum, gDatum[gPrefs.datum].name);
}


/****************************************************************************/
/* Query GPS for its model and software version.                            */
/****************************************************************************/
char *getGPSVersion()
{
  extern struct  PREFS gPrefs;
  extern char    gMessageStr[];
  char           temp[255];
  short          err;
  int            last, i;

  SetFrameBusy(1);

  /* First check it GPS responds to the "are-you-ready" package */
  if (CheckGPS()) {
    if ((err = serialOpen(GARMIN)) != noErr) {
      sprintf(gMessageStr, "The port initialization of %s has failed.", 
	      gPrefs.Device);
      Error(gMessageStr);
      return;
    }

    /* send request and receive version package */
    sendGPSMessage(gid5, 2);
    getGPSMessage();
    
    serialClose();

    /* extract model name and software version */
#if 0
    sprintf(GPSVersionStr,"0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x \n",
	 (int)(*(gGarminMessage + 0)),
	 (int)(*(gGarminMessage + 1)),
	 (int)(*(gGarminMessage + 2)),
	 (int)(*(gGarminMessage + 3)),
	 (int)(*(gGarminMessage + 4)),
	 (int)(*(gGarminMessage + 5)),
	 (int)(*(gGarminMessage + 6)),
	 (int)(*(gGarminMessage + 7)),
	 (int)(*(gGarminMessage + 8)),
	 (int)(*(gGarminMessage + 9)),
	 (int)(*(gGarminMessage + 10)),
	 (int)(*(gGarminMessage + 11)));
#endif	   
    sprintf(GPSVersionStr,"<Not implemented>");

    SetFrameBusy(0);
    return (GPSVersionStr);
  } else {
    sprintf(GPSVersionStr, "<GPS not responding>");
   
    SetFrameBusy(0); 
    return (GPSVersionStr);
  }
}


/****************************************************************************/
/* Query GPS for current GPS time (in GMT/UTC).                             */
/****************************************************************************/
long getGPSTime()
{
  extern struct  PREFS gPrefs;
  extern char    gMessageStr[];
  short          err;
  struct tm      *txx;
  time_t         clock;

  /* Check it GPS responds to the "are-you-ready" package */
  if (!CheckGPS()) NotResponding();

  /* open serial device */
  if ((err = serialOpen(GARMIN)) != noErr) {
    sprintf(gMessageStr, "The port initialization of %s has failed.", 
	    gPrefs.Device);
    Error(gMessageStr);
    return;
  }

  /* send command and receive package */
  sendGPSMessage(tim1, 4);
  getGPSMessage();   /* this is the ACK-package */
  getGPSMessage();   /* This is of the unknown type 09: leap seconds ?*/
  getGPSMessage();   /* This is ACK again */
  getGPSMessage();   /* Stuff */

  /* close serial device */
  serialClose();
 
  /* exit if not a valid time package */ 

  if (*(gGarminMessage + 1) != 0x0e) return (-1);

  /* convert UTC time to localtime using the unix timezone */ 
  time(&clock);
  txx = gmtime(&clock);

  
  txx->tm_sec  =  *(gGarminMessage + 10);
  txx->tm_min  =  *(gGarminMessage +  9);
  txx->tm_hour =  *(gGarminMessage +  7);
  txx->tm_year = (toshort(gGarminMessage + 5) - 1900);
  txx->tm_mday =  *(gGarminMessage +  4);
  txx->tm_mon  = (*(gGarminMessage +  3)) - 1;
  
  clock = timegm(txx);
  return (clock);
}


/****************************************************************************/
/* Get multiple line packages from GPS (route, waypoint, track)             */
/****************************************************************************/
void getGPSInfo(FILE *refNum, short type)
{
  extern struct  PREFS gPrefs;
  extern char    gMessageStr[];
  
  short total;
  short done  = 0;
  BYTE  *t    = (BYTE *)&total;
  BYTE  *init, *req;
  BYTE  n, *p;
  short err;
  char  *rType;

  /* First check it GPS responds to the "are-you-ready" package */
  if (!CheckGPS()) NotResponding();

  /* open serial device */
  if ((err = serialOpen(GARMIN)) != noErr) {
    sprintf(gMessageStr, "The port initialization of %s has failed.", 
	    gPrefs.Device);
    Error(gMessageStr);
    return;
  }
  
  FileRefNum = refNum;
  records    = 0;
 
  /* Save output format if not Almanac-Data */ 
  saveFormat(fileData);
  if (type != ALMANAC) FileWrite(fileData);
 
  /* process different requests */ 
  switch (type) {
  case ALMANAC:
  default:
    init  = alm1;
    req   = alm2;
    rType = "almanac";
    break;
    
  case ROUTE:
    init  = rte1;
    req   = rte2;
    rType = "routes";
    break;
    
  case TRACK:
    init  = trk1;
    req   = trk2;
    rType = "track";
    break;
    
  case WAYPOINT:
    init  = wpt1;
    req   = wpt2;
    rType = "waypoint";
    break;
  }
  
  AlmanacSat = 0;

  /* open BarGraph window */
  InitBarGraph();
 
  /* send init package */ 
  sendGPSMessage(init, 4);

  /* exit if no package received from GPS */
  if (getGPSMessage()==0) {
    CloseBarGraph();
    serialClose();
    return;
  }

  /* get number of records to transfer */
  *t++ = gGarminMessage[4];         /* MSB of number of records to transfer */
  *t   = gGarminMessage[3];         /* LSB */
 
  /* repeat until all packages received */ 
  while (!done) {
    sendGPSMessage(req, 4);
    if (getGPSMessage() == 0) break;
    done = (gGarminMessage[1] == 0x0c);
    if (!done) {
      parseGPS();
      SetBarGraph((double)(++records)/(double)total);
    }
  }

  /* close BarGraph and serial device */
  CloseBarGraph();
  serialClose();
  sprintf(gMessageStr, "%d %s packets were transfered from the GPS receiver.",
	  (records - 1), rType);
  Message(gMessageStr);
}


/****************************************************************************/
/* Parse messages received from GPS.                                        */
/****************************************************************************/
static void parseGPS()
{
  switch (gGarminMessage[1]) {

  case RTE_NAM:	                                       /* Route name record */
    doRouteName();
    break;
  case RTE_WPT:	                                   /* Route waypoint record */
    doWaypoint();
    break;
  case ALM:                                               /* Almanac record */
    doAlmanac();
    break;
  case TRK:                                                 /* Track record */
    doTrack();
    break;
  case WPT:                                              /* Waypoint record */
    doWaypoint();
    break;
  default:
    break;
  }
}


/****************************************************************************/
/* Process RouteName package and write to output device.                    */
/****************************************************************************/
static void doRouteName()
{
  sprintf(fileData, "R\t%d\t%s\n", *(gGarminMessage+3), 
	  string(20, gGarminMessage+4));
  FileWrite(fileData);
}


/****************************************************************************/
/* Process Track package and write to output device.                        */
/****************************************************************************/
static void doTrack()
{
  extern struct PREFS  gPrefs;

  double  latitude, longitude, x, y;
  char    zone[6];
  
  if (*(gGarminMessage+15) == '\x01')	 /* New track, create blank record */
    if (records != 0) FileWrite("\n");
 
  /* convert bytes to latitude / longitude */ 
  latitude  = int2deg(number(gGarminMessage+3));
  longitude = int2deg(number(gGarminMessage+7));

  /* translate to selected datum */
  translate(1, &latitude, &longitude, gPrefs.datum);

  /* Write track time to file */ 
  sprintf(fileData, "T\t%s\t", secs2dt(number(gGarminMessage+11), 
				       gPrefs.offset));
  FileWrite(fileData);
 
  /* convert to selected position format */ 
  switch(gPrefs.format) {
  case DMS:            
    sprintf(fileData, "%s\t", toDMS(latitude));
    FileWrite(fileData);
    sprintf(fileData, "%s\n", toDMS(longitude));
    FileWrite(fileData);
    break;
  case DMM:
    sprintf(fileData, "%s\t", toDM(latitude));
    FileWrite(fileData);
    sprintf(fileData, "%s\n", toDM(longitude));
    FileWrite(fileData);
    break;
  case DDD:
    sprintf(fileData, "%03.7f\t%04.7f\n", latitude, longitude);
    FileWrite(fileData);
    break;
  case UTM:                                          /* Convert to UTM grid */
    DegToUTM(latitude, longitude, zone, &x, &y);
    sprintf(fileData, "%s\t%07.0f\t%07.0f\n", zone, x, y);
    FileWrite(fileData);
    break;
  case KKJ:                                          /* Convert to KKJ grid */
    DegToKKJ(latitude, longitude, zone, &x, &y);
    sprintf(fileData, "%s\t%07.0f\t%07.0f\n", zone, x, y);
    FileWrite(fileData);
    break;
  case BNG:                                      /* Convert to British grid */
    DegToBNG(latitude, longitude, zone, &x, &y);
    sprintf(fileData, "%s\t%05.0f\t%05.0f\n", zone, x, y);
    FileWrite(fileData);
    break;
  case ITM:                                         /* Convert to Irish grid */
    DegToITM(latitude, longitude, zone, &x, &y);
    sprintf(fileData, "%s\t%05.0f\t%05.0f\n", zone, x, y);
    FileWrite(fileData);
    break;
  default:
    break;
  }
}


/****************************************************************************/
/* Process Waypoint package and write to output device.                     */
/****************************************************************************/
static void doWaypoint()
{
  extern struct PREFS  gPrefs;

  double                latitude, longitude, x, y;
  char                  zone[6];

  /* convert bytes to latitude/longitude */
  latitude = int2deg(number(gGarminMessage+9));
  longitude = int2deg(number(gGarminMessage+13));

  /* translate to selected datum */
  translate(1, &latitude, &longitude, gPrefs.datum); 

  /* write waypoint name */  
  sprintf(fileData, "W\t%s\t", string(6, gGarminMessage+3));	
  FileWrite(fileData);

  /* write comment - up to 40 characters */
  sprintf(fileData, "%40s\t", string(40, gGarminMessage+21));
  FileWrite(fileData);

  /* write date/time */
  sprintf(fileData, "%s\t", secs2dt(number(gGarminMessage+17), gPrefs.offset));
  FileWrite(fileData);

  switch(gPrefs.format) {
  case DMS:
    sprintf(fileData, "%s\t", toDMS(latitude));
    FileWrite(fileData);
    sprintf(fileData, "%s\n", toDMS(longitude));
    FileWrite(fileData);
    break;
  case DMM:
    sprintf(fileData, "%s\t", toDM(latitude));
    FileWrite(fileData);
    sprintf(fileData, "%s\n", toDM(longitude));
    FileWrite(fileData);
    break;
  case DDD:
    sprintf(fileData, "%03.7f\t%04.7f\n", latitude, longitude);
    FileWrite(fileData);
    break;
  case UTM:                                          /* convert to UTM grid */
    DegToUTM(latitude, longitude, zone, &x, &y);
    sprintf(fileData, "%s\t%07.0f\t%07.0f\n", zone, x, y);
    FileWrite(fileData);
    break;

  case KKJ:                                          /* convert to KKJ grid */
    DegToKKJ(latitude, longitude, zone, &x, &y);
    sprintf(fileData, "%s\t%07.0f\t%07.0f\n", zone, x, y);
    FileWrite(fileData);
    break;
  case BNG:                                       /* convert to British grid */
    DegToBNG(latitude, longitude, zone, &x, &y);
    sprintf(fileData, "%s\t%05.0f\t%05.0f\n", zone, x, y);
    FileWrite(fileData);
    break;
  case ITM:                                         /* convert to irish grid */
    DegToITM(latitude, longitude, zone, &x, &y);
    sprintf(fileData, "%s\t%05.0f\t%05.0f\n", zone, x, y);
    FileWrite(fileData);
    break;
  default:
    break;
  }
}


/****************************************************************************/
/* Process Almanac package and write to output device.                     */
/****************************************************************************/
static void doAlmanac()
{
  BYTE  *p = gGarminMessage+3;
  BYTE  n;
  char  temp[255];

  /* next satellite */
  AlmanacSat++;

  /* don't handle if satellite not active */
  if (*(gGarminMessage + 3) == (unsigned char)0xff) return;

  /* print almanac data in yuma-format */
  sprintf(temp, "******** Week %d almanac for PRN-%02d ********\n", 
	  toshort(gGarminMessage + 3), AlmanacSat);
  FileWrite(temp);
  sprintf(temp, "ID:                         %02d\n", AlmanacSat);
  FileWrite(temp);
  sprintf(temp, "Health:                     000\n");
  FileWrite(temp);
  sprintf(temp, "Eccentricity:              % 1.10E\n", 
	  tofloat(gGarminMessage + 17));
  FileWrite(temp);
  sprintf(temp, "Time of Applicability(x):   32768.0000\n");
  FileWrite(temp);
  sprintf(temp, "Orbital Inclination(rad):  % 1.10f\n",
	  tofloat(gGarminMessage + 41));
  FileWrite(temp);
  sprintf(temp, "Rate of Right Ascen(r/s):  % 1.10E\n",
	  tofloat(gGarminMessage + 37));
  FileWrite(temp);
  sprintf(temp, "SQRT(A)  (m^1/2):          % 1.10f\n",
	  tofloat(gGarminMessage + 21));
  FileWrite(temp);
  sprintf(temp, "Right Ascen at TOA(rad):   % 1.10E\n",
	  tofloat(gGarminMessage + 33));
  FileWrite(temp);
  sprintf(temp, "Argument of Perigee(rad):  % 1.10f\n",
	  tofloat(gGarminMessage + 29));
  FileWrite(temp);
  sprintf(temp, "Mean Anom(rad):            % 1.10E\n",
	  tofloat(gGarminMessage + 25));
  FileWrite(temp);
  sprintf(temp, "Af0(s):                    % 1.10E\n",
	  tofloat(gGarminMessage + 9));
  FileWrite(temp);
  sprintf(temp, "Af1(s/s):                  % 1.10E\n",
	  tofloat(gGarminMessage + 13));
  FileWrite(temp);
  sprintf(temp, "week:                       %d\n",
	  toshort(gGarminMessage + 3));
  FileWrite(temp);
  sprintf(temp, "\n");
  FileWrite(temp);
}


/****************************************************************************/
/* Convert bytes to string.                                                 */
/****************************************************************************/
static char *string(short n, BYTE *p)
{
  static char  s[50];
  short        i;
  
  for (i = 0; i < n; i++) s[i] = *p++;
  s[n] = '\0';

  return s;
}


/****************************************************************************/
/* Convert bytes to long number.                                            */
/****************************************************************************/
static long number(BYTE *p)
{	
  long  n;
  BYTE  *q = (BYTE *)&n;
#ifndef __LINUX__
  *q++ = *(p+3);
  *q++ = *(p+2);
  *q++ = *(p+1);
  *q   = *p;
#else
  *q++ = *p;
  *q++ = *(p+1);
  *q++ = *(p+2);
  *q++ = *(p+3);
#endif
  return n;
}


/****************************************************************************/
/* Convert bytes to float number.                                           */
/****************************************************************************/
static float tofloat(BYTE *p)
{
  float  n;
  BYTE   *q = (unsigned char *)&n;
#ifndef __LINUX__
  *q++ = *(p+3);
  *q++ = *(p+2);
  *q++ = *(p+1);
  *q   = *p;
#else
  *q++ = *p;
  *q++ = *(p+1);
  *q++ = *(p+2);
  *q   = *(p+3);
#endif
  return n;
}


/****************************************************************************/
/* Convert bytes to short number.                                           */
/****************************************************************************/
static short toshort(BYTE *p)
{
  short  n;
  BYTE   *q = (unsigned char *)&n;

  *q++ = *p;
  *q++ = *(p+1);

  return n;
}


/****************************************************************************/
/* Write string to output device.                                           */
/****************************************************************************/
static void FileWrite(char *data)
{
  long N = (long)strlen(data);
  fputs(data, FileRefNum);
}


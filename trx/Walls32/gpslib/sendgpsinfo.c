/****************************************************************************/
/*                                                                          */
/* ./gps/sendgpsinfo.c   -   Send data to GPS                               */
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


/* define external variables */
extern BYTE          gGarminMessage[MAX_LENGTH];
extern struct PREFS  gPrefs;

/* define global variables */
static char  fileData[MAX_LINE];
static BYTE  newTrack;
static BYTE  message[100];
static FILE  *FileRefNum;
struct PREFS gFilePrefs;


/* prototype functions */
static short records(short type);
static short doWaypoint(short type);
static short doTrack(void);
static short doRoute(void);
static short doAlmanac(void);
static short sendRecords(short);
static void  copyNumber(BYTE *p, long n);
static char  *field(char *a, short n);
static void  cpystr(BYTE *p, char *q, short n);
static short getFileData(char *);
static void  cleanupDMS(char *s);



/****************************************************************************/
/* Send "commit-suicide-command" to GPS.                                    */
/****************************************************************************/
long sendGPSOff()
{
  extern struct  PREFS gPrefs;
  extern char    gMessageStr[];
  short          err;
 
  /* First check it GPS responds to the "are-you-ready" package */
  if (!CheckGPS()) NotResponding();
 
  /* open serial device */
  if ((err = serialOpen(GARMIN)) != noErr) {
    sprintf(gMessageStr, "The port initialization of %s has failed.", 
            gPrefs.Device);
    Error(gMessageStr);
    return;
  }
 
  /* send command - no respond is send from GPS */
  sendGPSMessage(off1, 4);
 
  /* close serial device */
  serialClose();
 
  printf("GPS turned off successfully\n");
}
 

/****************************************************************************/
/* Send multiple line packages to GPS (route, waypoint, track)              */
/****************************************************************************/
void sendGPSInfo(FILE *refNum, short type)
{
  extern char gMessageStr[];
  
  short  recs, processed = 0;
  short  err,  result    = 1;
  char   *rType;
  BYTE   *term;
  

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
  recs      = records(type);

  /* Continue only if GPS responds to the send-request package */
  if (sendRecords(recs) > 0) { 

    /* go to beginning of file and read first line */
    rewind(FileRefNum);
    GetLine(FileRefNum, &fileData, 1);

    if (getFileData(fileData)) {
      InitBarGraph();

      /* process different requests */
      switch (type) {
      default:
      case ALMANAC:
	rType = "almanac";
	term = almt;
	while (GetLine(FileRefNum, &fileData, 0) && 
	       processed < recs && result) {
	  if (fileData[0] == 'A') {
	    result = doAlmanac();
	    SetBarGraph((double)(++processed)/(double)recs);
	  }
	}
	break;
	
      case ROUTE:
	rType = "routes";
	term = rtet;
	while (GetLine(FileRefNum, &fileData, 0) && 
               processed < recs && result) {
	  if (fileData[0] == 'R') {
	    result = doRoute();
	    ++processed;
	  }
	  if (fileData[0] == 'W') {
	    result = doWaypoint(ROUTE);
	    ++processed;
	  }
	  SetBarGraph((double)(processed)/(double)recs);
	}
	break;
	
      case TRACK:
	rType = "track";
	term = trkt;
	newTrack = 1;
	while (GetLine(FileRefNum, &fileData, 0) && 
	       processed < recs && result) {
	  if (fileData[0] == 'T') {
	    result = doTrack();
	    SetBarGraph((double)(++processed)/(double)recs);
	  }
	  else
	    newTrack = 1;
	}
	break;
	
      case WAYPOINT:
	rType = "waypoint";
	term = wptt;
	while (GetLine(FileRefNum, &fileData, 0) && 
	       (processed < recs) && result) {
	  if (fileData[0] == 'W') {
	    result = doWaypoint(WAYPOINT);
	    SetBarGraph((double)(++processed)/(double)recs);
	  }
	}
	break;
      }

      /* close BarGraph window */
      CloseBarGraph();

      /* send terminator message to tell GPS - its over */
      if (result) {
	sendGPSMessage(term, 4);

        /* get final response but ignore it */	 
        getGPSMessage();

        /* print number of packages transfered */
	sprintf(gMessageStr, "%d %s packets were transfered to the GPS %s",
                recs, rType, "receiver.");
	Message(gMessageStr);
      }

      /* close serial device */
      serialClose();
    }
    else
      /* error if no correct format header in file */
      Error("The file header format is incorrect.");
  }
}


/****************************************************************************/
/* Waypoint file format:                                                    */
/*                                                                          */
/*                                                                          */
/* Format: DDD  UTC Offset:  -6.00 hrs  Datum[061]: NAD27 CONUS             */
/* W    HOME  123 SNOWMASS  07/28/1994 20:12:54  40.1777471  -105.0892654   */
/* 0    1     2             3                    4           5              */
/* TYPE NAME  COMMENT       DATE                 LATITUDE    LONGITUDE      */
/*                                                                          */
/*                                                                          */
/* Format: DMM  UTC Offset:  -6.00 hrs  Datum[061]: NAD27 CONUS             */
/* W    HOME  123 SNOWMASS  07/28/1994 20:12:54  40 10.665'  -105 05.356'   */
/* 0    1     2             3                    4            5             */
/* TYPE NAME  COMMENT       DATE                 LATITUDE     LONGITUDE     */
/*                                                                          */
/*                                                                          */
/* Format: DMS  UTC Offset:  -6.00 hrs  Datum[061]: NAD27 CONUS             */
/* W    HOME  123 SNOWMASS  07/28/1994 20:12:54  40 10'39.9" -105 05'21.4"  */
/* 0    1     2             3                    4           5              */
/* TYPE NAME  COMMENT       DATE                 LATITUDE    LONGITUDE      */
/*                                                                          */
/*                                                                          */
/* Format: UTM  UTC Offset:  -6.00 hrs  Datum[061]: NAD27 CONUS             */
/* W    HOME  123 SNOWMASS  07/28/1994 20:12:54  13 T  0492400  4447279     */
/* 0    1     2             3                    4  5  6        7           */
/* TYPE NAME  COMMENT       DATE                 ZE ZN EASTING  NORTHING    */
/*                                                                          */
/*                                                                          */
/* Format: BNG  UTC Offset:  -6.00 hrs  Datum[061]: NAD27 CONUS             */
/* W    EXAMPL BRITISH GRD  07/28/1994 20:12:54  SE   12345    67890        */
/* 0    1      2            3                    4    5        6            */
/* TYPE NAME   COMMENT      DATE                 ZONE EASTING  NORTHING     */
/*                                                                          */
/*                                                                          */
/* Format: ITM  UTC Offset:  -6.00 hrs  Datum[061]: NAD27 CONUS             */
/* W    EXAMPL IRISH GRID   07/28/1994 20:12:54  IN   12345    67890        */
/* 0    1      2            3                    4    5        6            */
/* TYPE NAME   COMMENT      DATE                 ZONE EASTING  NORTHING     */
/*                                                                          */
/*                                                                          */
/* Format: FIN  UTC Offset:  -6.00 hrs  Datum[061]: NAD27 CONUS             */
/* W    EXAMPL SWISS GRID   07/28/1994 20:12:54  27E 012345  067890         */
/* 0    1      2            3                    4   5       6              */
/* TYPE NAME   COMMENT      DATE                 ZE  EASTING NORTHING       */
/*                                                                          */
/*                                                                          */
/* Format: SUI  UTC Offset:  -6.00 hrs  Datum[061]: NAD27 CONUS             */
/* W    EXAMPL SWISS GRID   07/28/1994 20:12:54  012345  067890             */
/* 0    1      2            3                    5       6                  */
/* TYPE NAME   COMMENT      DATE                 EASTING NORTHING           */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/* Process Waypoint package - return 0 if GPS not respond.                  */
/****************************************************************************/
static short doWaypoint(short type)
{
  double  latitude, longitude, x, y;
  long    lat, lon, dat;
  short   zone;
  char    finzone[5];		/* Currently always 27E, i.e. not used */
  char    bgzone[5];
  char    date[20], nsZone;
  enum {TYPE  = 0, NAME,   COMMENT, DATE, LAT, LON};
  enum {ZE    = 4, ZN,     EASTING, NORTHING};                  /* UTM Grid */
  enum {BGZ   = 4, BGEAST, BGNORTH};                 /* British, Irish Grid */
  enum {SEAST = 4, SNORTH};                                   /* Swiss Grid */
	

  strncpy(date, field(fileData, DATE), 19);
  date[19] = 0;

  /* handle various grids */  
  switch(gFilePrefs.format) {
  case DMS:
    cleanupDMS(fileData);
    latitude = DMStoDegrees(field(fileData, LAT));
    longitude = DMStoDegrees(field(fileData, LON));
    break;
    
  case DMM:
    cleanupDMS(fileData);
    latitude = DMtoDegrees(field(fileData, LAT));
    longitude = DMtoDegrees(field(fileData, LON));
    break;
    
  case DDD:
    sscanf(field(fileData, LAT), "%lf %lf", &latitude, &longitude);
    break;
   
  case UTM:                                                     /* UTM Grid */
    sscanf(field(fileData, ZE), "%d", &zone);
    sscanf(field(fileData, ZN), "%c", &nsZone);
    sscanf(field(fileData, EASTING), "%lf %lf", &x, &y);
    UTMtoDeg(zone, nsZone <= 'M', x, y, &latitude, &longitude);
    break;

  case KKJ:                                                     /* KKJ Grid */
    sscanf(field(fileData, ZE), "%s", finzone);
    sscanf(field(fileData, EASTING), "%lf %lf", &x, &y);
    KKJtoDeg(2, 0, x, y, &latitude, &longitude); /* 2 is 27E, not used currently */
    break;
    
  case BNG:                                                 /* British Grid */
    sscanf(field(fileData, BGZ), "%s", bgzone);
    sscanf(field(fileData, BGEAST), "%lf %lf", &x, &y);
    BNGtoDeg(bgzone, x, y, &latitude, &longitude);
    break;
    
  case ITM:                                                   /* Irish Grid */
    sscanf(field(fileData, BGZ), "%s", bgzone);
    sscanf(field(fileData, BGEAST), "%lf %lf", &x, &y);
    ITMtoDeg(bgzone, x, y, &latitude, &longitude);
    break;
    
  default:
    break;
  }
 
  /* translate to selected datum */
  translate(0, &latitude, &longitude, gFilePrefs.datum);
 
  /* convert numbers to GPS bytes */ 
  lat = deg2int(latitude);
  lon = deg2int(longitude);
  dat = dt2secs(date, gFilePrefs.offset);
 
  /* create package header */ 
  if (type == WAYPOINT)
    message[0] = '\x23';
  else                                                     /* type == ROUTE */
    message[0] = '\x1e';

  /* number of message bytes to follow */
  message[1] = 58;

  /* copy position and time to GPS package */ 
  cpystr(message+2, field(fileData, NAME), 6);
  copyNumber(message+8, lat);
  copyNumber(message+12, lon);
  copyNumber(message+16, dat);
  fileData[strlen(fileData)-1] = 0;		/* replace newline with null */
  cpystr(message+20, field(fileData, COMMENT), 40);
 
  /* send package to GPS */ 
  sendGPSMessage(message, 60);

  /* make sure GPS respond */
  return getGPSMessage();   
}


/****************************************************************************/
/* Track file format:                                                       */
/*                                                                          */
/*                                                                          */
/* T    08/29/1994 13:32:01  40.146967 -105.125464 13   T  489391  4443614  */
/* 0    1                    2          3          4    5  6       7        */
/* TYPE DATE                 LAT        LON        ZONE ZN EASTING NORTHING */
/*                                                                          */
/*                                                                          */
/* Format: DDD  UTC Offset:  -6.00 hrs  Datum[061]: NAD27 CONUS             */
/* T    09/09/1994 20:03:49  40.1780207 -105.0901666                        */
/* 0    1                    2          3                                   */
/* TYPE DATE/TIME            LATITUDE   LONGITUDE                           */
/*                                                                          */
/*                                                                          */
/* Format: DMM  UTC Offset:  -6.00 hrs  Datum[061]: NAD27 CONUS             */
/* T    09/09/1994 20:03:49  40 10.681'  -105 05.410'                       */
/* 0    1                    2           3                                  */
/* TYPE DATE/TIME            LATITUDE    LONGITUDE                          */
/*                                                                          */
/*                                                                          */
/* Format: DMS  UTC Offset:  -6.00 hrs  Datum[061]: NAD27 CONUS             */
/* T    09/09/1994 20:03:49  40 10'40.9"  -105 05'24.6"                     */
/* 0    1                    2            3                                 */
/* TYPE DATE/TIME            LATITUDE     LONGITUDE                         */
/*                                                                          */
/*                                                                          */
/* Format: UTM  UTC Offset:  -6.00 hrs  Datum[061]: NAD27 CONUS             */
/* T    09/09/1994 20:03:49  13  T   0492323   4447310                      */
/* 0    1	             2   3   4         5                            */
/* TYPE DATE/TIME            ZE  ZN  EASTING   NORTHING                     */
/*                                                                          */
/*                                                                          */
/* Format: BNG  UTC Offset:  -6.00 hrs  Datum[061]: NAD27 CONUS             */
/* T    09/09/1994 20:03:49  SE    12345    67890                           */
/* 0    1	             2     3        4                               */
/* TYPE DATE/TIME            ZONE  EASTING  NORTHING                        */
/*                                                                          */
/*                                                                          */
/* Format: ITM  UTC Offset:  -6.00 hrs  Datum[061]: NAD27 CONUS             */
/* T    09/09/1994 20:03:49  IN    12345    67890                           */
/* 0    1	             2     3        4                               */
/* TYPE DATE/TIME            ZONE  EASTING  NORTHING                        */
/*                                                                          */
/*                                                                          */
/* Format: SUI  UTC Offset:  -6.00 hrs  Datum[061]: NAD27 CONUS             */
/* T    09/09/1994 20:03:49  012345   067890                                */
/* 0    1	             3        4                                     */
/* TYPE DATE/TIME            EASTING  NORTHING                              */
/*                                                                          */
/*                                                                          */
/* Note: Even though the date/time is included in the protocol, the GPS 45  */
/* appears to ignore this field. The only track data that is time stamped   */
/* is that actually generated by the GPS 45; actual track data uploaded     */
/* from the GPS 45 has correct time stamps.                                 */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/* Process Track package - return 0 if GPS not respond.                     */
/****************************************************************************/
static short doTrack()
{
  double  latitude, longitude, x, y;
  long    lat, lon, dat;
  short   zone;
  char    date[20], nsZone, bgzone[3], finzone[5];
  enum {TYPE,      DATE,   LAT,     LON};
  enum {ZE    = 2, ZN,     EASTING, NORTHING};                  /* UTM Grid */
  enum {BGZ   = 2, BGEAST, BGNORTH};                 /* British, Irish Grid */
  enum {SEAST = 2, SNORTH};                                   /* Swiss Grid */
	
  strncpy(date, field(fileData, DATE), 19);
  date[19] = 0;
  
  /* handle various grids */  
  switch(gFilePrefs.format) {
  case DMS:
    cleanupDMS(fileData);
    latitude = DMStoDegrees(field(fileData, LAT));
    longitude = DMStoDegrees(field(fileData, LON));
    break;
    
  case DMM:
    cleanupDMS(fileData);
    latitude = DMtoDegrees(field(fileData, LAT));
    longitude = DMtoDegrees(field(fileData, LON));
    break;
    
  case DDD:
    sscanf(field(fileData, LAT), "%lf %lf", &latitude, &longitude);
    break;

  case UTM:                                                     /* UTM Grid */
    sscanf(field(fileData, ZE), "%d", &zone);
    sscanf(field(fileData, ZN), "%c", &nsZone);
    sscanf(field(fileData, EASTING), "%lf %lf", &x, &y);
    UTMtoDeg(zone, nsZone <= 'M', x, y, &latitude, &longitude);
    break;

  case KKJ:                                                     /* KKJ Grid */
    sscanf(field(fileData, ZE), "%s", finzone);
    sscanf(field(fileData, EASTING), "%lf %lf", &x, &y);
    KKJtoDeg(2, 0, x, y, &latitude, &longitude); /* 2 is 27E, not used currently */
    break;

  case BNG:                                                 /* British Grid */
    sscanf(field(fileData, BGZ), "%s", bgzone);
    sscanf(field(fileData, BGEAST), "%lf %lf", &x, &y);
    BNGtoDeg(bgzone, x, y, &latitude, &longitude);
    break;
    
  case ITM:                                                   /* Irish Grid */
    sscanf(field(fileData, BGZ), "%s", bgzone);
    sscanf(field(fileData, BGEAST), "%lf %lf", &x, &y);
    ITMtoDeg(bgzone, x, y, &latitude, &longitude);
    break;
    
  default:
    break;
  }
  
  /* translate to selected datum */
  translate(0, &latitude, &longitude, gFilePrefs.datum);
  
  /* convert numbers to GPS bytes */ 
  lat = deg2int(latitude);
  lon = deg2int(longitude);
  dat = dt2secs(date, gFilePrefs.offset);
  
  /* create package header */ 
  message[0] = '\x22';

  /* number of message bytes to follow */
  message[1] = 13;

  /* copy position and time to GPS package */ 
  copyNumber(message+2, lat);
  copyNumber(message+6, lon);
  copyNumber(message+10, dat);

  /* set "new track flag" */
  message[14] = newTrack;
  newTrack    = 0;

  /* send package to GPS */ 
  sendGPSMessage(message, 15);

  /* make sure GPS respond */
  return getGPSMessage();
}


/****************************************************************************/
/* Route file format:                                                       */
/*                                                                          */
/*                                                                          */
/* R 	1	BOGUS 1                                                     */
/* 0    1       2                                                           */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/* Process Route package - return 0 if GPS not respond.                     */
/****************************************************************************/
static short doRoute()
{
  short route;

  /* create package header */ 
  message[0] = '\x1d';

  /* number of message bytes to follow */
  message[1] = 21;

  /* copy route data to GPS package */
  sscanf(field(fileData, 1), "%d", &route);
  message[2] = route;
  cpystr(message+3, field(fileData, 2), 20);

  /* send package to GPS */ 
  sendGPSMessage(message, 23);

  /* make sure GPS respond */
  return getGPSMessage();
}


/****************************************************************************/
/* Process Almanac package - uploading almanac data is no longer supported. */
/****************************************************************************/
#define ALM_BYTES 42

static short doAlmanac()
{
  short           i;
  unsigned short  x;
 
  /* Print error message and return */
  fprintf(stderr, "Error: Uploading of almanac data is no longer supported.\n");
  return (0);


  /* create package header */ 
  message[0] = '\x1f';

  /* number of message bytes to follow */
  message[1] = ALM_BYTES;

  /* copy almanac bytes to GPS package */
  for (i = 0; i < ALM_BYTES; i++) {
    sscanf(fileData+2+3*i, "%02X", &x);
    message[i+2] = (BYTE)x;
  }

  /* send package to GPS */ 
  sendGPSMessage(message, ALM_BYTES+2);

  /* make sure GPS respond */
  return getGPSMessage();
}


/****************************************************************************/
/* Return number of records in input-file.                                  */
/****************************************************************************/
static short records(short type)
{
  short  n = 0;
  
  /* Get line from input file */
  GetLine(FileRefNum, fileData, 1);

  /* Parse package type until end-of-file */
  while (GetLine(FileRefNum, fileData, 0)) {

    /* process different types */
    switch (type) {
    case ALMANAC:
    default:
      if (fileData[0] == 'A')
	++n;
      break;
      
    case ROUTE:
      if (fileData[0] == 'R' || fileData[0] == 'W')
	++n;
      break;
      
    case TRACK:
      if (fileData[0] == 'T')
	++n;
      break;
      
    case WAYPOINT:
      if (fileData[0] == 'W')
	++n;
      break;
    }
  }

  /* return number of records */
  return n;
}


/****************************************************************************/
/* Send a message containing the number of records to follow. Returns a     */
/* result code - zero if GPS did not respond.                               */
/****************************************************************************/
static short sendRecords(short recs)
{
  short  result;

  /* create package header */ 
  message[0] = '\x1b';

  /* number of message bytes to follow */
  message[1] = '\x02';

  /* convert number of records to GPS bytes */
  message[2] = recs % 256;                      /* LSB of number of records */
  message[3] = recs / 256;                      /* MSB */

  /* send package to GPS */ 
  sendGPSMessage(message, 4);

  /* make sure GPS respond */
  result = getGPSMessage();
  return result;
}


/****************************************************************************/
/* Convert long number to bytes.                                            */
/****************************************************************************/
static void copyNumber(BYTE *p, long n)
{	
  BYTE  *q = (BYTE *)&n;


#ifdef __LINUX__	
  *p++ = *q;
  *p++ = *(q+1);
  *p++ = *(q+2);
  *p++ = *(q+3);
#else
  *p++ = *(q+3);
  *p++ = *(q+2);
  *p++ = *(q+1);
  *p++ = *q;
#endif
}


/****************************************************************************/
/* returns a pointer to the nth field of a tab-delimited record which       */
/* starts at address a.                                                     */
/****************************************************************************/
static char *field(char *a, short n)
{
  short  i = 0;

  while ((i++) < n)
    while (*(++a) != '\t')
      ;
  return (a + 1);
}


/****************************************************************************/
/* Copy a string to GPS bytes.                                              */
/****************************************************************************/
static void cpystr(BYTE *p, char *q, short n)
{
  while ((*q != '\t') && (*q != '\n') && (*q != '\r') && (*q != 0)) {
    *p++ = *q++;
    --n;
  }
  while (n-- > 0)
    *p++ = ' ';
}


/****************************************************************************/
/*                                                                          */
/*  The following function retrieves the file format info from the first    */
/*  line of the data file. This is highly dependent upon the following      */
/*  format for the first line in the file -- note that there are not        */
/*  embedded tabs. See the file "getGPSInfo.c" for further information.     */
/*                                                                          */
/*  Format: DDD  UTC Offset:  -6.00 hrs  Datum[061]: NAD27 CONUS            */
/*          |||              ||||||            |||                          */
/*  0         1         2         3         4         5         6           */
/*  0123456789012345678901234567890123456789012345678901234567890123456789  */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/* Get data format from the first line of input file.                       */
/****************************************************************************/
static short getFileData(char *data)
{
  extern short  nDatums;
  char          fmt[10];
  
  sscanf(data, "%7s", fmt);
  if (strcmp(fmt, "Format:") != 0) return 0;
  
  /* Get position format */
  sscanf(data+8, "%3s", fmt);
  if (strcmp(fmt, "DMS") == 0)
    gFilePrefs.format = DMS;
  else if (strcmp(fmt, "DMM") == 0)
    gFilePrefs.format = DMM;
  else if (strcmp(fmt, "DDD") == 0)
    gFilePrefs.format = DDD;
  else if (strcmp(fmt, "UTM") == 0)
    gFilePrefs.format = UTM;
  else if (strcmp(fmt, "BNG") == 0)
    gFilePrefs.format = BNG;
  else if (strcmp(fmt, "ITM") == 0)
    gFilePrefs.format = ITM;
  else if (strcmp(fmt, "KKJ") == 0)
    gFilePrefs.format = KKJ;
  else return 0;
  
  /* Get time offset */
  sscanf(data+25, "%lf", &gFilePrefs.offset);
  if (gFilePrefs.offset < -24.0 || gFilePrefs.offset > 24.0) return 0;
 
  /* Get datum format */ 
  sscanf(data+43, "%3d", &gFilePrefs.datum);
  if (gFilePrefs.datum < 0 || gFilePrefs.datum > nDatums-1) return 0;
  
  /* Return if everything is okay */
  return 1;
}


/****************************************************************************/
/* Change degree, minutes and second symbols to spaces.                     */
/****************************************************************************/
static void cleanupDMS(char *s)
{
  while (*++s) 
    if (*s == '¡' || *s == '\260' || *s == '\'' || *s == '\"') *s = ' ';
}

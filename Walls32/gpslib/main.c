/****************************************************************************/
/*                                                                          */
/* ./ascii/main.c   -   Main program and small utility procedures           */
/*                                                                          */
/* This file is part of gpstrans - a program to communicate with garmin gps */
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
#include <signal.h>
//***7.1 #include <sys/time.h>
#include <time.h>


/* define external variables */
extern struct PREFS gPrefs;                     /* Preferences from prefs.c */

/* define global variables */
char  cmd = ' ', what = ' ';
char  *prog, *progname;
char  *dowhat;
char  FileName[255];
FILE  *refNum;
enum  PROTOCOL mode = GARMIN; 



/****************************************************************************/
/* Print message if GPS is not responding and exit.                         */
/****************************************************************************/
void NotResponding()
{
  fprintf(stderr, "GPS is not responding - make sure it is on and set");
  fprintf(stderr, " to GRMN/GRMN protocol.\n");
  exit(1);
}


/****************************************************************************/
/* Print headline with programname, version and copyright notice.           */
/****************************************************************************/
void PrintHeadLine()
{
  fprintf(stderr, "%s (ASCII) - Version %s\nCopyright (c) %s by Carsten ",
	  ProgramName, ProgramVersion, ProgramYear);
  fprintf(stderr, "Tschach (tschach@zedat.fu-berlin.de)\n");
  fprintf(stderr, "Linux/KKJ mods by     Janne Sinkkonen <janne@iki.fi> (1996)\n\n");
}


/****************************************************************************/
/* Print usage information.                                                 */
/****************************************************************************/
void usage()
{
  fprintf(stderr, "Usage: %s flag [filename]\n\n", progname);
  
  fprintf(stderr, "Flags are:  -v   version:   information about program\n");
  fprintf(stderr, "            -p   port:      set serial I/O-Device\n");
  fprintf(stderr, "            -s   setup:     set datum, format, offset ");
  fprintf(stderr, "and device\n\n");
  fprintf(stderr, "            -i   ident:     identify connected GPS\n");
  fprintf(stderr, "            -o   off:       Turn off GPS Device\n");
  fprintf(stderr, "            -t   time:      Get time from GPS (-ts will ");
  fprintf(stderr, "set time on host)\n\n");
  fprintf(stderr, "            -d?  download:  r = route, t = track, w = ");
  fprintf(stderr, "waypoint, a = almanac\n");
  fprintf(stderr, "            -u?  upload:    r = route, t = track, w = ");
  fprintf(stderr, "waypoint\n\n");
  fprintf(stderr, "If no filename is entered, data will be written to %s",
	  "stdout and read from stdin.\n");
  fprintf(stderr, "Serial I/O-Device can also be set within the %s",
	  "environment variable $GPSDEV.\n");
  exit(1);
}


/****************************************************************************/
/* Print error messages.                                                    */
/****************************************************************************/
void ParseError(err)
int err;
{
  switch (err) {
  case 1:
    fprintf(stderr, "You have to specify at least one parameter.\n\n");
    fprintf(stderr, "Start %s without any parameters to get usage ",
	    progname);
    fprintf(stderr, "information.\n");
    break;
  case 2:
    fprintf(stderr, "You have to specify one of '-da -dr -dt -dw'\n\n");
    fprintf(stderr, "Start %s without any parameters to get usage ",
	    progname);
    fprintf(stderr, "information.\n");
    break;
  case 3:
    fprintf(stderr, "You have to specify one of '-ur -ut -uw'\n\n");
    fprintf(stderr, "Start %s without any parameters to get usage ",
	    progname);
    fprintf(stderr, "information.\n");
    break;
  case 4:
    fprintf(stderr, "You have to specify a device name when using '-p'\n\n");
    fprintf(stderr, "Start %s without any parameters to get usage ",
	    progname);
    fprintf(stderr, "information.\n");
    break;
  case 5:
    fprintf(stderr, "Can't get time from GPS Receiver\n");
    break;
  }
  fflush(stderr);
  exit(1);
}


/****************************************************************************/
/* Print verbose version information and copyright notice.                  */
/****************************************************************************/
void HandleAbout()
{
  fprintf(stderr, "Parts of this program where taken from MacGPS by %s",
	  "John F. Waers\n\n");
  
  fprintf(stderr, "If you've any questions or bugs please send email to:\n\n");
  fprintf(stderr, "%53s", "tschach@zedat.fu-berlin.de");
  fprintf(stderr, "\n\n");
  fprintf(stderr, "The newest version can always be found at:\n\n");
  fprintf(stderr, "%57s", "ftp.fu-berlin.de:/pub/unix/misc/gps");
  fprintf(stderr, "\n\n");

  fprintf(stderr, "*******************************************************************************\n");
  fprintf(stderr, "*   Permission to use, copy, modify, and distribute this software and its     *\n");
  fprintf(stderr, "*   documentation for non-commercial purpose, is hereby granted without fee,  *\n");
  fprintf(stderr, "*   providing that the copyright notice appear in all copies and that both    *\n");
  fprintf(stderr, "*   the copyright notice and this permission notice appear in supporting      *\n");
  fprintf(stderr, "*   documentation. I make no representations about the suitability of this    *\n");
  fprintf(stderr, "*   software for any purpose. It is provides \"as is\" without express or       *\n");
  fprintf(stderr, "*   implid warranty.                                                          *\n");
  fprintf(stderr, "*******************************************************************************\n");
  exit(0);
}


/****************************************************************************/
/* Query GPS for device type and software version.                          */
/****************************************************************************/
void InfoAboutGPS()
{
  printf("Connected GPS is: %s\n", getGPSVersion());
  exit(0);
}


/****************************************************************************/
/* Turn off GPS device.                                                     */
/****************************************************************************/
void TurnOffGPS()
{
  sendGPSOff();
  exit(0);
}


/****************************************************************************/
/* Get time from GPS - compare with local time and adjust if requested.     */
/****************************************************************************/
void AdjustTime(char dowhat)
{
  char            c;
  long            diffis;
  time_t          clock, clockgps;
  struct timeval  tp;

  time(&clock);
  clockgps = getGPSTime();
  if (clockgps == -1) ParseError(5);

  printf("Local time determine from GPS-Receiver is:  %s", ctime(&clockgps));
  printf("Local time on machine is:                   %s\n", ctime(&clock));
  
  diffis = (clockgps - clock);
  if (diffis < 0) diffis = diffis * (-1);
  printf("Time difference is %d seconds.\n", diffis);

  /* if set-option and uid=root then adjust local time */
  if (dowhat == 's') {
    if (getuid() == 0) {
      if (diffis <= 7200) {
		clockgps   = getGPSTime();
		tp.tv_sec  = clockgps;
		tp.tv_usec = 5000;                /* to adjust lost time in process */
		settimeofday(&tp, (struct timezone *)NULL);
		fprintf(stderr, "\nLocal time set to:  %s", ctime(&clockgps));
      } else {
		fprintf(stderr, "\nTime difference is > 2 hours - time not set\n");
      }
    } else 
		fprintf(stderr, "\nSorry, only root can set the time.\n");
  }

  exit(0);
}


/*****************************************************************************/
/* Catch signals to get grateful death.                                      */
/*****************************************************************************/
void TerminateHandler(sig)
int   sig;
{
  if (sig == SIGINT) {
    fprintf(stderr, "\nDon't touch me...but you've pressed CTRL-C\n");
    fprintf(stderr, "It was you choice....exiting\n");
  } else {
    fprintf(stderr, "\nGotcha...somebody try to shot me...but he missed !\n");
    fprintf(stderr, "\nBut okay, commiting suicide....<argh!>\n");
  }
  exit(0);
}



/*****************************************************************************/
/*                        H a u p t p r o g r a m m                          */
/*****************************************************************************/
main(argc, argv)
int  argc;
char *argv[];
{
  /* Initialize Signal handler */
  signal(SIGTERM, TerminateHandler);
  signal(SIGINT,  TerminateHandler);

  /* Get program name and print headline */
  progname = argv[0];
  PrintHeadLine();

  /* Print usage information if no parameters where given */
  if (argc <= 1) usage();
 
  /* Init preferences - overwrite serial device if $GPSDEV is set */ 
  sprintf(FileName, "");
  InitPrefs();
  if (getenv("GPSDEV") != NULL) sprintf(gPrefs.Device, "%s", getenv("GPSDEV"));

  /* Parse arguements */
  for (prog = *argv++; --argc; argv++) {
    if (argv[0][0] == '-') {
      switch (argv[0][1]) {
      case 'd':
        dowhat = *argv+2;
		cmd = 'd';
        break;
      case 'u':
        dowhat = *argv+2;
		cmd = 'u';
        break;
      case 'p':
		sprintf(gPrefs.Device, "%s", *argv+2);
		break;
      case 'v':
		HandleAbout();
        break;
      case 'i':
		InfoAboutGPS();
		break;
      case 'o':
		TurnOffGPS();
		break;
      case 's':
		SetupProgram();
		break;
      case 't':
		dowhat = *argv+2;
		AdjustTime(*dowhat);
		break;
      }
    } else {
      if (strlen(FileName) == 0) sprintf(FileName, "%s", argv[0]);
    }
  }

  /* Exit if no serial device */
  if (strlen(gPrefs.Device) == 0) ParseError(4);

  /* Parse up- and download parameters */
  if (cmd != ' ') what = dowhat[0];

  switch (cmd) {
  case 'd': {
    if (strlen(FileName) == 0) 
      refNum = stdout;
    else
      refNum = fopen(FileName, "wt");

    switch (what) {
    case 'a':
      getGPSInfo(refNum, ALMANAC);
      break;
    case 'r':
      getGPSInfo(refNum, ROUTE);
      break;
    case 't':
      getGPSInfo(refNum, TRACK);
      break;
    case 'w':
      getGPSInfo(refNum, WAYPOINT);
      break;
    default:
      ParseError(2);
      break;
    }
    break;
  }
  case 'u': {
    if (strlen(FileName) == 0) 
      refNum = stdin;
    else
      refNum = fopen(FileName, "rt");

    switch (what) {
    case 'r':
      sendGPSInfo(refNum, ROUTE);
      break;
    case 't':
      sendGPSInfo(refNum, TRACK);
      break;
    case 'w':
      sendGPSInfo(refNum, WAYPOINT);
      break;
    default:
      ParseError(3);
      break;
    }
    break;
  }
  default:
    ParseError(1);
    break;
  }
  exit(0);
}


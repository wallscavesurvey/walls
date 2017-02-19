/****************************************************************************/
/*                                                                          */
/* ./gps/garminserial.c   -   Procedure to talk with serial device          */
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
#include "Prefs.h"
#include "Garmin.h"
#include <sys/types.h>
#include <fcntl.h>
#include <termio.h>
#include <sys/termios.h>

#ifdef SUNOS41
#include <sys/filio.h>
#endif


/* define external variables */
extern struct PREFS gPrefs;

/* define global variables */
int             ttyfp;
struct termio   ttyset;


/****************************************************************************/
/* Open serial device for requested protocoll - only GARMIN supported now.  */
/****************************************************************************/
int serialOpen(enum PROTOCOL p)
{
  char   temp[255];
  int    err;
  
  ttyfp = open(gPrefs.Device, O_RDWR);
 
  /* return when error opening device */ 
  if (ttyfp < 0)                         return (-1);
  if (ioctl(ttyfp, TCGETA, &ttyset) < 0) return (-1);

  /* set baud rate for device */
  switch (p) {
  case GARMIN:
    ttyset.c_cflag = CBAUD & B9600;
    break;
  case NMEA:
    ttyset.c_cflag = CBAUD & B4800;
    break;
    
  default:
    return -1;
  }

  /* set character size and allow to read data */ 
#ifdef SUNOS41 
  ttyset.c_cflag |= (CSIZE & CS8) | CRTSCTS | CREAD;
#else
  ttyset.c_cflag |= (CSIZE & CS8) | CREAD;
#endif

  ttyset.c_iflag  = ttyset.c_oflag = ttyset.c_lflag = (ushort)0;
  ttyset.c_oflag  = (ONLRET);

  /* return if unable to set communication parameters */
  if (ioctl(ttyfp, TCSETAF, &ttyset) < 0) return (-1);

  return 0;
}


/****************************************************************************/
/* Get number of serial characters available.                               */
/****************************************************************************/
long serialCharsAvail()
{
  int count;

  ioctl(ttyfp, FIONREAD, &count);
  return (long) count;
}


/****************************************************************************/
/* Close serial device.                                                     */
/****************************************************************************/
void serialClose()
{
  close(ttyfp);
}

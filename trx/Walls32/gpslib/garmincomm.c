/****************************************************************************/
/*                                                                          */
/* ./gps/garmincomm.c   -   Basic communication procedures between gps and  */
/*                          unix host.                                      */
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
extern ttyfp;

/* define global variables */
BYTE gGarminMessage[MAX_LENGTH];



/****************************************************************************/
/* Calculate checksum, prepare package and send to GPS.                     */
/****************************************************************************/
void sendGPSMessage(BYTE *message, short bytes)
{
  extern short  gOutRefNum;
  short         i;
  long          n;
  BYTE          csum = 0;
  BYTE          *p   = gGarminMessage;
  
  *p++ = 0x10;                                   /* Define start of message */

  for (i = 0; i < bytes; i++) {
    *p++  = message[i];
    csum -= message[i];
    if (message[i] == 0x10)
      *p++ = 0x10;
  }

  *p++ = csum;
  if (csum == 0x10) *p++ = 0x10;
  *p++ = 0x10;
  *p++ = 0x03;
  n = (long)(p - gGarminMessage);

#ifdef DEBUG                    /* print sent byte if compiled with -DDEBUG */
  printf("Sending:   ");
  for (i = 0; i<n; i++)
    printf("%02X ", *(gGarminMessage + i));
  printf("\n");
#endif

  write(ttyfp, gGarminMessage, n);
}


/****************************************************************************/
/* Sent a "are-you-ready" message to GPS - return TRUE if GPS responds or   */
/* FALSE if GPS did not respond (GPS off? / incorrect format?)              */
/****************************************************************************/
int CheckGPS()
{
  extern struct  PREFS gPrefs;
  extern char    gMessageStr[];

  long           count = 1L;
  BYTE           *p    = gGarminMessage;
  BYTE           last  = 0;
  BYTE           c;
  short          err;
  int            igot, i;
  short          length = 0;

  long           tttime = TickCount();


  /* Open device, FALSE if unable to open device */
  if ((err = serialOpen(GARMIN)) != noErr) {
    sprintf(gMessageStr, "The port initialization of %s has failed.",
	    gPrefs.Device);
    Error(gMessageStr);
    serialClose();
    return (1 == 0);
  }

  sendGPSMessage(test, 4);

  /* wait for respond */
  for (;;) {

    /* timeout exceded */
    if (TickCount() > (tttime + (long)TIMEOUT)) {
      serialClose();
      return (1 == 0);
    }
    
    while (serialCharsAvail()) {
      igot = read(ttyfp, &c, 1);

      if (c == 0x10 && last == 0x10)
	last = 0;
      else {
	last = *p++ = c;
	++length;
      }

      /* received valid respond package */
      if (*(p-1) == 0x03 && *(p-2) == 0x10) {
	serialClose();
	return (1 == 1);
      }
    }
  }
}


/****************************************************************************/
/* Get respond from GPS - returns number of bytes received, 0 if timeout    */
/* period elapsed without completing a package.                             */
/****************************************************************************/
short getGPSMessage()
{
  extern short  gInRefNum;
  extern enum   PROTOCOL mode;
  
  long          count  = 1L;
  BYTE          *p     = gGarminMessage;
  BYTE          last   = 0;
  BYTE          c;
  short         length = 0;
  int           igot, i;

  long          tttime = TickCount();
 
 
  for (;;) {

    /* exit if timeout exceeded */
    if (TickCount() > (tttime + (long)TIMEOUT)) {
      CloseBarGraph();
      Error("The GPS receiver is not responding.");
      mode = NONE;
      return 0;
    }
   
    while (serialCharsAvail()) {
      if (length >= MAX_LENGTH-1) {
	Error("GPS receiver communication protocol error.");
	mode = NONE;
	return 0;
      }

      igot = read(ttyfp, &c, 1);

      if (c == 0x10 && last == 0x10)
	last = 0;
      else {
	last = *p++ = c;
	++length;
      }

      /* return when package complete */
      if (*(p-1) == 0x03 && *(p-2) == 0x10) {

#ifdef DEBUG          /* print packages received when compiled with -DDEBUG */
	printf("Receiving: ");
	for (i = 0; i < length; i++)
	  printf("%02X ", *(gGarminMessage + i));
	printf("\n");
#endif

    	return length;
      }  
    }
  }
}


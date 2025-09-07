/****************************************************************************/
/*                                                                          */
/* ./gps/gpsmessage.c   -   Define GPS commands                             */
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



/****************************************************************************/
/* Define Garmin-Protocol commands.                                         */
/****************************************************************************/
BYTE m1[]	= "\xfe\x01\x20";       /* Get the model and version number */
BYTE m2[]	= "\x06\x02\xff\x00";	/* unknown */

BYTE p1[]	= "\x06\x02\x1b\x00";	/* Get 1st packet */
BYTE p2[]	= "\x06\x02\x0c\x00";	/* Get last packet */

BYTE alm1[]	= "\x0a\x02\x01\x00";	/* Send almanac command */
BYTE alm2[]	= "\x06\x02\x1f\x00";	/* Get next packet */

BYTE trk1[]	= "\x0a\x02\x06\x00";	/* Get track command */
BYTE trk2[]	= "\x06\x02\x22\x00";	/* Get next packet */

BYTE wpt1[]	= "\x0A\x02\x07\x00";	/* Get waypoint command */
BYTE wpt2[]	= "\x06\x02\x23\x00";	/* Get next packet */

BYTE rte1[]     = "\x0a\x02\x04\x00";	/* Get route command */
BYTE rte2[]	= "\x06\x02\x1d\x00";	/* Get next packet */

BYTE test[]     = "\x1c\x02\x00\x00";   /* Getting respond from Garmin */

BYTE gid4[]     = "\xfe\x00";           /* Get Garmin model & Software rev */
BYTE gid5[]     = "\x06\x02\xff\x00";  

BYTE off1[]     = "\x0a\x02\x08\x00";   /* Turn Garmin Off */
BYTE tim1[]     = "\x0a\x02\x05\x00";   /* Get UTC Time */

BYTE almt[]	= "\x0c\x02\x01\x00" ;	/* Almanac data base terminator */
BYTE rtet[]	= "\x0c\x02\x04\x00" ;	/* Route data base terminator */
BYTE trkt[]	= "\x0c\x02\x06\x00" ;	/* Track data base terminator */
BYTE wptt[]	= "\x0c\x02\x07\x00" ;	/* Waypoint data base terminator */


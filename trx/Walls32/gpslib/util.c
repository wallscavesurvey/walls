/****************************************************************************/
/*                                                                          */
/* ./ascii/util.c   -   Utility procedures                                  */
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
#ifdef __LINUX__
#include "time.h"
#endif

/* define global variables */
char         gMessageStr[255];
static char  copy[255];



/****************************************************************************/
/* Get line from input file - only implemented for compatibility reason.    */
/****************************************************************************/
short GetLine(FILE *refNum, char *line, short init)
{
  if (!feof(refNum)) {
    fgets(line, 255, refNum);
    return 1;
  } else {
    *line = '\0';
    return 0;
  }
}


/****************************************************************************/
/* Print message text.                                                      */
/****************************************************************************/
void Message(char *txt)
{
  fprintf(stderr, "INFO:  %s\n", txt);
  fflush(stderr);
}


/****************************************************************************/
/* Print error message and exit program.                                    */
/****************************************************************************/
void Error(char *txt)
{
  fprintf(stderr, "ERROR:  %s\n", txt);
  fflush(stderr);
  exit(1);
}


/****************************************************************************/
/* Get local time in seconds - only implemented for compatibility reason.   */
/****************************************************************************/
long TickCount()
{
  time_t count;

  time(&count);
  return (long) count;
}

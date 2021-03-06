/****************************************************************************/
/*                                                                          */
/* ./ascii/prefs.c   -   Set, Load and Save preferences                     */
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
#include "getline.h"
#include <pwd.h>
#include <sys/time.h>


/* define external variables */
extern struct DATUM     const gDatum[];       /* Variables from gps/datum.c */
extern struct ELLIPSOID const gEllipsoid[];
extern short  nDatums;

/* define global variables */
char    prop_scratch[255];
struct  PREFS gPrefs;

/* prototype functions */
void SetEllipsoid(int n);



/****************************************************************************/
/* Expend a filename to absolute path.                                      */
/****************************************************************************/
char *AbsolutFilename(ch)
char *ch;
{
  char           temp[255], temp1[255];
  char           *search;
  struct passwd  *pwdentry;

  if (ch[0] != '~') {
    sprintf(prop_scratch, "%s", ch);
    return(&prop_scratch[0]);
  }
  sprintf(temp, "%s", ch);
  if (ch[1] == '/') {
    sprintf(temp1, "%s%s", getenv("HOME"), &temp[1]);
    sprintf(temp, "%s", temp1);
  } else {
    sprintf(temp1, "%s", temp);
    search   = strchr(temp1, '/');
    *search  = (char) 0;
    pwdentry = getpwnam(&temp1[1]);
    search   = strchr(temp, '/');
    if (pwdentry != NULL) {
      sprintf(temp1, "%s%s", pwdentry->pw_dir, search);
      sprintf(temp, "%s", temp1);
    } else {
      temp[0] = (char) 0;
    }
  }
  sprintf(prop_scratch, "%s", temp);
  return (&prop_scratch[0]);
}

 
/****************************************************************************/
/* Analyzes a dataline and separate keyword and data.                       */
/****************************************************************************/
int AnalyzeLine(line, key, data)
char   line[255], key[255], data[255];
{
  int i;
 
  if (line[strlen(line) - 1] == '\n') line[strlen(line) - 1] = '\0';
 
  if (line[0] == '#') return (1 == 2);
  if (strlen(line) < 5) return (1 == 2);
 
  i = 0;
  while ((i <= strlen(line)) && (line[i] != ':'))
    i++;
 
  if (i >= strlen(line)) return (1 == 2);
 
  sprintf(key, "%s", line);
  key[i] = '\0';
 
  while (key[0] == ' ')  sprintf(key, "%s", &key[1]);
  while (key[0] == '\t') sprintf(key, "%s", &key[1]);
 
  if (strlen(key) != 4) return (1 == 2);
 
  sprintf(data, "%s", &line[i + 2]);
 
  return (1 == 1);
}


/****************************************************************************/
/* Save preferences in Preference-File defined in defs.h.                   */
/****************************************************************************/
void SavePrefs()
{
  FILE     *preffp;
 
  preffp = fopen(AbsolutFilename(PrefFileName), "wt");
  if (preffp == (FILE *) 0) {
    Message("Can't write preferences in file '%s'", 
            AbsolutFilename(PrefFileName));
  } else {
    fprintf(preffp, "# Properties file of gpstrans - do not edit this file\n");
    fprintf(preffp, "FMAT: %d\n", gPrefs.format);
    fprintf(preffp, "OFFS: %1.2f\n", gPrefs.offset);
    fprintf(preffp, "DATU: %d\n", gPrefs.datum);
    fprintf(preffp, "DEVI: %s\n", gPrefs.Device);
    fclose(preffp);
  }
}
 

/****************************************************************************/
/* Load preferences from Preference-File - set default if not exist.        */
/****************************************************************************/
void LoadPrefs()
{
  char   Line[255], Keyword[255], Data[255];
  int    value;
  float  floatval;
  FILE   *preffp;
 
  gPrefs.format = DMS;
  gPrefs.offset = +0.0;
  gPrefs.datum  = 100;
  sprintf(gPrefs.Device, "%s", DefaultServerDevice);
 
  preffp = fopen(AbsolutFilename(PrefFileName), "rt");
  if (preffp != (FILE *) 0) {
 
    fgets(Line, 255, preffp);
    while (!feof(preffp)) {
      
      if (AnalyzeLine(Line, Keyword, Data)) {
 
        if (!strcmp(Keyword, "FMAT")) {
          sscanf(Data, "%d", &value);
          gPrefs.format = value;
        } else
        if (!strcmp(Keyword, "OFFS")) {
          sscanf(Data, "%f", &floatval);
          gPrefs.offset = (double) floatval;
        } else
        if (!strcmp(Keyword, "DATU")) {
          sscanf(Data, "%d", &value);
          gPrefs.datum = value;
        } else
        if (!strcmp(Keyword, "DEVI")) {
          sprintf(gPrefs.Device, "%s", Data);
        }
      }
      fgets(Line, 255, preffp);
    }
    fclose(preffp);
  }
}
 

/****************************************************************************/
/* Init preferences.                                                        */
/****************************************************************************/
void InitPrefs()
{
  LoadPrefs();
}


/****************************************************************************/
/* Wait for RETURN when called.                                             */
/****************************************************************************/
void WaitforReturn()
{
  printf("Press RETURN for more ");
  fflush(stdout);
  getchar();
}


/****************************************************************************/
/* Setup preferences - datum, format, time offset and serial device.        */
/****************************************************************************/
void SetupProgram()
{
  int    value;
  short  v;
  float  floatval;
  char   *p;

  for (v = 0; v <= nDatums; v++) {
    printf("%3d - %-25s    ", v, gDatum[v].name);
    if (((v + 1) % 2) == 0) printf("\n");
    if ((v == 39) || (v == 85)) WaitforReturn();
  }
  printf("\n");

  do {
    p = getline("Please select datum: ");
    sscanf(p, "%d", &value);
    printf("\n");
  } while ((value < 0) || (value > nDatums) || (p[0] < '0') || (p[0] > '9'));
  gPrefs.datum = value;
  
  printf("\n\n");
  printf("   0 - lat/lon  ddd�mm'ss.s\"\n");
  printf("   1 - lat/lon  ddd�mm.mmm'\n");
  printf("   2 - lat/lon  ddd.ddddd\n");
  printf("   3 - utm/ups Grid\n");
  printf("   4 - British Grid\n");
  printf("   5 - Irish Grid\n");
  printf("   6 - Finnish Grid \n\n");

  do {
    p = getline("Please select format: ");
    sscanf(p, "%d", &value);
    printf("\n");
  } while ((value < 0) || (value > 6) || (p[0] < '0') || (p[0] > '9'));
  gPrefs.format = value;

  printf("\n");
  do {
    p = getline("Please select time offset (-12 to +12): ");
    sscanf(p, "%f", &floatval);
    printf("\n");
  } while ((floatval < -12.0) || (floatval > 12.0));
  gPrefs.offset = (double)floatval;

  printf("\n");
  do {
    p = getline("Please enter serial device name: ");
    sprintf(gPrefs.Device, "%s", p);
    printf("\n");
  } while (strlen(gPrefs.Device) == 0);

  printf("\n\n");
  printf("Preferences have been saved to ~/.gpstrans and will be used in the");
  printf(" future.\n");

  SavePrefs();
  exit(0);
}

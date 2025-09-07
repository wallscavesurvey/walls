#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
//***7.1 #include <strings.h>


#define ProgramName          "GPStrans"
#define ProgramVersion       "0.31b-js"
#define ProgramYear          "1995"

#define MainMenuHeader       "GPS Transfer"

#define BarWindowHeader      "Transfer in progress..."
#define PrefsWindowHeader    "Preferences"

#define PrefFileName         "~/.gpstrans"

#define DefaultServerDevice  "/dev/cua1"
#define ServerBaud           B4800

#define TIMEOUT              10

#define noErr		      0

#define X_NULL               (int) NULL

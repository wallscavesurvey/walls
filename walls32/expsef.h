/*EXPSEF.H -- Interface for SEF export functions (wall-exp) in wallexp.dll */

#ifndef __EXPSEF_H
#define __EXPSEF_H

#include "prjref.h"

#ifndef __TRX_TYPE_H
#include <trx_type.h>
#endif

#define EXP_VERSION 4.0
#define EXP_VERSION_STR "4.0"
#define EXP_DLL_NAME "wallexp.dll"
#define MAXCP_INC 20
#define MAXCC_INC 100
#define EXP_MAX_NAMLEN 20

enum exp_flags {EXP_USEDATES=1,EXP_NOCVTPFX=2,EXP_USE_ALLSEF=4,EXP_USE_VDIST=8};
enum exp_fcn {EXP_OPEN,EXP_CLOSE,EXP_SRVOPEN,EXP_SURVEY,EXP_OPENDIR,EXP_CLOSEDIR,EXP_SETOPTIONS};

typedef struct {
	UINT fcn;
	char *title;
	char *name;
	const char *path;
	DWORD date;
	UINT flags;
	UINT charno;
	UINT lineno;
	UINT numvectors;
	UINT numfixedpts;
	UINT numgroups;
	int maxnamlen;
	double fTotalLength;
	PRJREF *pREF
	;double grid;
	;char *pdatumname;
	;int zone;
} EXPTYP;

typedef struct {
	double di;
	double az;
	double va;
} sphtyp;

#define outSEF_s(s) fputs(s,fpout)
#define outSEF_ln() fputs("\r\n",fpout)

#endif

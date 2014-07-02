/*EXPSEF.H -- Interface for network adjustment library, NETLIB.LIB */

#ifndef __EXPSEF_H
#define __EXPSEF_H

#ifndef __TRX_TYPE_H
#include <trx_type.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define EXP_VERSION 3
#define EXP_DLL_NAME "WALLEXP.DLL"

enum exp_flags {EXP_USEDATES=1,EXP_CVTPFX=2,EXP_INCDET=4,EXP_FORCEFEET=8,EXP_FIXEDCOLS=16};
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
	double fTotalLength;
	double grid;
} EXPTYP;

#ifdef __cplusplus
}
#endif
#endif

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <memory.h>
#include <math.h>
#include <float.h>
#include <stdlib.h>
#include <trx_str.h>
#include "wall_srv.h"

enum eTypP {TYP_D,TYP_A,TYP_V};
enum eTypR {TYP_E,TYP_N,TYP_U}; 
enum eType {U_POLAR=0,U_RECT=1};
enum eCase {U_UPPERCASE,U_LOWERCASE,U_ANYCASE};

#define U_METERS 1.0
#define U_FEET 0.3048
#define PI 3.141592653589793
#define PI2 (PI/2.0)
#define U_DEGREES (PI/180.0)
#define U_PERCENT (PI/400.0)
#define U_MILS (PI/3200.0)
#define U_GRADS (PI/200.0)
#define ERR_FLT 0.001
#define MAX_BACKDELTA (U_DEGREES*5.01)
#define LINLEN 256

extern LPSTR_CB srv_CB;
extern DWORD nLines;

#define DECLINATION_MAX 90.0
#define CORRECTION_MAX (PI/4)
#define VAR_FIXFACT (10.0*FLT_EPSILON)

#define SRV_DLLTYPE "SRV"
#define EXP_DLLTYPE "EXP"

#define NUM_ORDER_ARGS 3
#define OFFSET_ORDER_ARGS 2

typedef char typ_cseq[NUM_ORDER_ARGS+1];
typedef BYTE typ_iseq[NUM_ORDER_ARGS+1];

typedef char PFX_COMPONENT[SRV_LEN_PREFIX];

typedef struct {
     double unitsA,unitsAB,unitsV,unitsVB;
     double declination,gridnorth,rectnorth;
     double incD,incH,incA,incAB,incV,incVB,incS;
     double errAB,errVB;
     double variance[2];
	 PFX_COMPONENT prefix[SRV_MAX_PREFIXCOMPS];
     int nPrefix;
     int bFeet,bFeetS;
     short revAB,revABX,revVB,revVBX;
     int vectype;
     int namecase;
     int tapemeth;
	 int lrudmeth;
     typ_iseq iseq[2];
	 BYTE lrudseq[4];
} SRV_UNITS;

extern SRV_UNITS *u;

enum e_cmd {CMD_NONE,CMD_DATE,CMD_FIX,
			CMD_FLAG,CMD_NOTE,CMD_PREFIX,CMD_PREFIX2,CMD_PREFIX3,
            CMD_SEG,CMD_SYM,CMD_UNITS,CMD_COMMENT};
enum {
       SRV_ERR_OPEN=SRV_ERR_LOCAL,
       SRV_ERR_REOPEN,
       SRV_ERR_SIZNAME,
       SRV_ERR_LINLEN,
       SRV_ERR_NUMARGS,
       SRV_ERR_VERSION,
       SRV_ERR_DECLINATION,
       SRV_ERR_CORRECTION,   //==10
       SRV_ERR_SPECIFIER,
       SRV_ERR_ANGLEUNITS,
       SRV_ERR_ORDER,
       SRV_ERR_AZIMUTH,
       SRV_ERR_DISTANCE,
       SRV_ERR_VERTICAL,
       SRV_ERR_NUMERIC,
       SRV_ERR_VARIANCE,
       SRV_ERR_UNEXPVALUE,
       SRV_ERR_SEGPATH,		//==20
       SRV_ERR_SEGSIZE,
       SRV_ERR_DIRECTIVE,
       SRV_ERR_LINECMD,
       SRV_ERR_UV,
       SRV_ERR_PREFIX,
       SRV_ERR_NOPAREN,
       SRV_ERR_BADSYMBOL,
       SRV_ERR_NONAMEPFX,
       SRV_ERR_CMDBLANK,
       SRV_ERR_NOPARAM,		//==30
       SRV_ERR_SLASH,
       SRV_ERR_BACKDELTA,
       SRV_ERR_TAPEMETH,
       SRV_ERR_NOTEARG,
       SRV_ERR_COMMOPEN,
       SRV_ERR_COMMCLOSE,
       SRV_ERR_DATE,
       SRV_ERR_RESTORE,
       SRV_ERR_SAVE,
       SRV_ERR_SIGNREV,		//==40
       SRV_ERR_EQUALS,
       SRV_ERR_LENUNITS,
       SRV_ERR_BADANGLE,
       SRV_ERR_BADLENGTH,
       SRV_ERR_BLANK,
       SRV_ERR_AZIMUTHSIGN,
       SRV_ERR_TYPEBACK,
       SRV_ERR_FSBSERR,
	   SRV_ERR_LRUDARGS,
	   SRV_ERR_LRUDMETH,	//==50
	   SRV_ERR_LATLONG,
	   SRV_ERR_NODATUM,
	   SRV_ERR_BADCOLOR,
	   SRV_ERR_SPRAYNAME,
	   SRV_ERR_MACRONOVAL,
	   SRV_ERR_MACRONODEF,
	   SRV_ERR_NOMEM,
	   SRV_ERR_LENPREFIX,
	   SRV_ERR_COMPPREFIX,
	   SRV_ERR_CHRPREFIX,  //==60
	   SRV_ERR_ERRLIM
     };

extern double fTemp; /*Global for temporary use*/
extern int ErrBackTyp; /*Set TYP_A or TYP_V when fTemp > FS/BS tolerance

/*wall-srv.c routines --*/
apfcn_i get_fTempAngle(char *p0,BOOL bAzimuth);
BOOL _cdecl log_error(char *fmt,...);

/*wall_pa.c routines --*/
int PASCAL parse_angle_arg(int typ,int i);

/*EOF*/
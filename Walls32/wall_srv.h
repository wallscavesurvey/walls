//wall-srv.c function declarations --

#ifndef __WALL_SRV_H
#define __WALL_SRV_H 
/*
#ifdef __cplusplus
extern "C" {
#endif
*/
#pragma pack(1)

#define SRV_SIZNAME      8        /* Maximum name length (chars)*/
#define SRV_SIZFILE		 8		  /* Length of file name stored in database*/
#define SRV_SIZNAME_STR "8"       /* For error message*/
#define SRV_SEG_DELIM 0x01		  /* Replaces "/" in segment paths*/
#define SRV_SIZ_SEGBUF 256
#define SRV_MAX_FLAGSIZE 200
#define SRV_MAX_SEGNAMLEN 80        /* Max length of a segment name*/
#define SRV_SIZ_SEGNAMBUF 84        /* Seg component buffer length*/
#define SRV_LEN_PREFIX 128        /* Max length of a prefix name plus 1*/
#define SRV_MAX_PREFIXCOMPS 3	  /* Maximum supported prefix levels*/
#define SRV_LEN_PREFIX_DISP (3*8+2)    /* Max length of displayed prefix*/
#define SRV_LEN_NOTESTR (255-3)   /* Max length of note string, excluding null*/
#define SRV_CHAR_COMMAND '#'
#define SRV_CHAR_COMMENT ';'
#define SRV_CHAR_PREFIX  ':'
#define SRV_CHAR_VAR '('
#define SRV_CHAR_VAREND ')'
#define SRV_CHAR_LRUD '<'
#define SRV_CHAR_LRUDEND '>'
#define SRV_CHAR_LINENOTE '/'
#define SRV_CHAR_MACRO '$'

/*Char values for variance flags*/
#define SRV_CHAR_FLTVECT '?'     /* vector floats within network*/
#define SRV_CHAR_FLTTRAV '*'     /* vector's traverse floats within network*/

/*Values for vector flags (we add 0x20 to insure visibility within database)*/
#define NET_VEC_NORMAL   0x20 	 /* ( ) normal vector*/
#define NET_VEC_FLTVECTH 0x01    /* (!) vector floats horizontally*/
#define NET_VEC_FLTTRAVH 0x03    /* (#) traverse floats horizontally*/
#define NET_VEC_FLTVECTV 0x04    /* ($) vector floats vertically*/
#define NET_VEC_FLTTRAVV 0x0C    /* (,) traverse floats vertically*/
#define NET_VEC_FIXED    0x40    /* (0) #FIXed vector record*/

/*NOTE: 0x25 - (%) float vectH,vectV*/
/*		0x2F - (/) float travH,travV*/
/*		0x27 - (') float travH,vectV*/
/*		0x2D - (-) float vectH,travV*/

/*Factor to convert distance in meters to variance equivalent to
  variance(ft) = 3.28*dist(m)/100
  variance(m)  = variance(ft)/(3.28*3.28) = dist(m)/328 = dist(m)*0.003048 */
#define NET_VAL_FACT 0.003048

#pragma pack(1)

typedef struct { /*15*8 + 3*8 = 144 bytes --*/
	char flag;					/* NET_VEC_x flags (see above) Initially SRV_VERSION*/
	BYTE date[3];
	WORD pfx[2];				/* Name prefixes returned by srv_CB or 0*/
	char  nam[2][SRV_SIZNAME];	/* From/to station name (each space extended)*/
	double xyz[3];              /* E-N-U vector components in meters*/
	double hv[2];			    /* Variance of Hz and Vt components*/
	/*The following fields are used only by Walls and wallnet4.dll --*/
	long id[2];                 /* Reserved for name id's*/
	long str_id;                /* NTS rec # if in string (or 0) - used by NETLIB*/
	long str_nxt_vc;            /* NTV rec # of next string vec (or 0) - used by NETLI*/
	double e_xyz[3];            /* Set by adjustment DLL*/
	DWORD  lineno;              /* Line number in file (1-based)*/
	WORD  seg_id;   		    /* Set to -1 by DLL when segment changes */
	WORD  charno;               /* Character offset in line (0-based) of error*/
								/* Also used for flags by srv_Open() */
	BYTE filename[SRV_SIZFILE];

	/* Fields not present in vectyp (or NTV file) -- used only by wall_srv.c*/
	double len_ttl;             /* Vector length used for WALLS segment statistics*/
	BYTE root_datum;			/* WALLSRV will compute UTM if this is !=255*/
	BYTE ref_datum;				/* Datum change necessary if root_datum!=ref_datum*/
	short root_zone;
	short ref_zone;				/* Out-of-zone coordinates computed if root_zone!=ref_zone*/
	float grid;					/* Initial GRID= default*/
} NTVRec;

typedef NTVRec FAR *lpNTVRec;

#define LRUD_BLANK -9999.0f
#define LRUD_BLANK_NEG 9999.0f

#define LRUD_FLG_CS 1

typedef struct {
	WORD pfx;
	char  nam[SRV_SIZNAME];	/* Station name (space extended)*/
	float dim[4];
	float az;				/* Facing azimuth from nam[1] to nam[2]*/
	WORD flags;				/* 0 or LRUD_FLG_CS */
	UINT lineno;
} SRV_LRUD;

#pragma pack()

typedef int (FAR PASCAL * LPSTR_CB)(int fcn_id, LPSTR lpStr);
enum e_fcn_id {
	FCN_SEGMENT, FCN_PREFIX, FCN_NOTE, FCN_FLAG, FCN_SYMBOL, FCN_FLAGNAME,
	FCN_DATEDECL, FCN_LRUD, FCN_ERROR, FCN_CVTDATUM, FCN_CVTLATLON, FCN_CVT2OUTOFZONE
};

enum e_srv_ref { SRV_USEDATES = 1, SRV_FIXVTLEN = 2, SRV_FIXVTVERT = 4 };

int PASCAL srv_Open(LPSTR pathname, lpNTVRec pvec, LPSTR_CB pSrv_CB);
int PASCAL srv_SetOptions(LPSTR pathname);
int PASCAL srv_Next(void);
void PASCAL srv_Close(void);
#define SRV_ERR_LOGMSG 1000 
LPSTR PASCAL srv_ErrMsg(LPSTR buffer, int code);

#pragma pack()
/*
#ifdef __cplusplus
}
#endif
*/
#endif

#ifndef GLOBAL_ED_H
#define GLOBAL_ED_H
#include <nti_file.h>
#include <libloco.h>
#include <stdlib.h>
#include <assert.h>

#define myputc(c) (_nti_buffer_pos<_nti_buffer_lim) ? (*_nti_buffer_pos++=(c)) : (nti_errno=-1)

/* TRUE and FALSE values */
#define TRUE 1
#define FALSE 0

/* For 1st component of plane interl. mode */
#define FIRST		1

#define BIG_ENDIAN	1

/* Define max and min macros */
#ifndef max
#define max(a,b)  (((a)>=(b))?(a):(b))
#define min(a,b)  (((a)<=(b))?(a):(b))
#endif

/****** Constants */
#define DEF_NEAR	0

/* margins for scan lines */
#define	LEFTMARGIN	2
#define RIGHTMARGIN	1

/* alphabet size */
#define MAXA8 (256)
#define MAXA16 (65536)
#define LUTMAX8 (256)
#define LUTMAX16 (4501)

/* lco_reset values */
#define DEFAULT_RESET 64
#define MINRESET 3

/* Quantization threshold basic defaults */
/* These are the defaults for LOSSLESS, 8 bpp. Defaults for other
   cases are derived from these basic values */
#define	BASIC_T1	3
#define	BASIC_T2	7
#define	BASIC_T3	21
#define	BASIC_Ta	5

#define CREGIONS (9)    /* quantization regions for d-a, a-c, c-b */
/* number of different contexts */
#define CONTEXTS1 (CREGIONS*CREGIONS*CREGIONS)
#define CONTEXTS   ((CONTEXTS1+1)/2) /* all regions, with symmetric merging */
#define MAX_C 127
#define MIN_C -128
#define EOLINE	 1
#define NOEOLINE 0
/* Number of end-of-run contexts */
#define EOR_CONTEXTS 2
/* Total number of contexts */
#define TOT_CONTEXTS (CONTEXTS +  EOR_CONTEXTS)
/* index of first end-of-run context */
#define EOR_0	(CONTEXTS)
/* Limit for unary code */
#define LIMIT 23
#define RESRUN    256
/* index of run state */
#define RUNSTATE 0
/* The longest code the bit IO can facilitate */
#define MAXCODELEN 24
/* The stat initialization values */
#define INITNSTAT 1			/* init value for lco_N[] */
#define MIN_INITABSTAT 2    /* min init value for lco_A[] */
#define INITABSLACK 6       /* init value for lco_A is roughly 
							   2^(bpp-INITABSLACK) but not less than above */
#define INITBIASTAT 0		/* init value for lco_B[] */

#ifdef BIG_ENDIAN
#    define ENDIAN8(x)   (x)
#    define ENDIAN16(x)   (x)
#else
#    define ENDIAN8(x) (x&0x000000ff)
#    define ENDIAN16(x) ( ((x>>8)|(x<<8)) & 0x0000ffff)
#endif

/* clipping macro */
#define clip(x,alpha) \
	    if ( x & highmask ) {\
	      if(x < 0) \
			x = 0;\
	      else \
			x = alpha - 1;\
	    }

/* macro to predict Px */
#define predict(Rb, Ra, Rc)	\
{	\
	register pixel minx;	\
	register pixel maxx;	\
	\
	if (Rb > Ra) {	\
		minx = Ra;	\
		maxx = Rb;	\
	} else {	\
		maxx = Ra;	\
		minx = Rb;	\
	}	\
	if (Rc >= maxx)	\
		Px = minx;	\
	else if (Rc <= minx)	\
		Px = maxx;	\
	else	\
		Px = Ra + Rb - Rc;	\
}

#define	alpha 256
#define highmask (-(alpha))
#define ceil_half_alpha (alpha/2)
#define lco_reset DEFAULT_RESET
#define lutmax LUTMAX8
#define lco_near DEF_NEAR
#define qbpp 8
#define bpp 8

/*  Marker identifiers */
#define	SOI		0xFFD8	/* start of image */
#define EOI		0xFFD9	/* end of image */

/* Portability types */
typedef unsigned char byte;
typedef unsigned short word;
typedef unsigned long dword;
typedef unsigned short pixel;

/****** Global variables prototypes */

/* BYTE I/O variables */
//extern byte *lco_inPtr;
extern byte *lco_inEOF;
extern int lco_inErr;
extern byte *lco_outPtr;
extern byte *lco_outEOF;
extern int lco_outErr;

extern pixel *lco_pscanl, *lco_cscanl, *lco_scanl0, *lco_scanl1;
extern word lco_maxcols;
extern int lco_limit_reduce;	/* reduction on above for EOR states */

extern unsigned long lco_melcorder;  /* 2^ melclen */
extern int	lco_melcstate;    /* index to the state array */
extern int	lco_melclen;      /* contents of the state array location*/

extern int  lco_T1, lco_T2, lco_T3;

/* for look-up-tables */
extern int lco_zeroLUT[];     /* lookup table to find number of leading zeroes */
extern int lco_vLUT[3][2 * LUTMAX16];
extern int lco_classmap[CONTEXTS1];
extern bool lco_bInitialized;

void prepareLUTs();
bool lco_Init(int maxCols);

/* statistics tables */
extern int	lco_N[TOT_CONTEXTS],
			lco_A[TOT_CONTEXTS],
			lco_B[TOT_CONTEXTS],
			lco_C[TOT_CONTEXTS];

#define MELCSTATES	32	/* number of melcode states */

extern int lco_J[MELCSTATES];

//#define BUFSIZE ((16*1024)-4) /* Size of input BYTE buffer */
//extern int lco_fp;                /* index into byte  buffer */
//extern byte lco_negbuff[];        /* the buffer */
//#define buff (lco_negbuff+4)

/* BIT I/O variables */
extern dword lco_reg;         /* BIT buffer for input/output */
extern int lco_bits;          /* number of bits free in bit buffer (on output) */

                         /* (number of bits free)-8 in bit buffer (on input)*/
#define BITBUFSIZE (8*sizeof(lco_reg))

#endif
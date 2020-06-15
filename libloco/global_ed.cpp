#include "global_ed.h"

pixel *lco_pscanl, *lco_cscanl, *lco_scanl0, *lco_scanl1;
word lco_maxcols;

//byte *lco_inPtr;
byte *lco_inEOF;
int lco_inErr;
//int lco_inbytes;

byte *lco_outPtr;
byte *lco_outEOF;
int lco_outErr;


int lco_limit_reduce;	/* reduction on above for EOR states */
unsigned long lco_melcorder;  /* 2^ melclen */
int	lco_melcstate;    /* index to the state array */
int	lco_melclen;      /* contents of the state array location
						 indexed by melcstate: the "expected"
						 run length is 2^melclen, shorter runs are
						 encoded by a 1 followed by the run length
						 in binary representation, wit a fixed length
						 of melclen bits */

int lco_J[MELCSTATES] = {
	0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,5,5,6,6,7,
	7,8,9,10,11,12,13,14,15
};

int lco_vLUT[3][2 * LUTMAX16];

int lco_classmap[CONTEXTS1];

int lco_N[TOT_CONTEXTS],
lco_A[TOT_CONTEXTS],
lco_B[TOT_CONTEXTS],
lco_C[TOT_CONTEXTS];

/* Context quantization thresholds  - initially unset */
int	lco_T3 = -1,
lco_T2 = -1,
lco_T1 = -1;

//int lco_fp; 		/* index into byte buffer */
//byte lco_negbuff[BUFSIZE+4];    /* byte I/O buffer, allowing for 4 "negative" locations */
/* BIT I/O variables */
dword lco_reg;         /* BIT buffer for input/output */
int lco_bits;          /* number of bits free in bit buffer (on output) */

bool lco_bInitialized = false;

static void createzeroLUT()
{
	int i, j, k, l;

	j = k = 1; l = 8;
	for (i = 0; i < 256; ++i) {
		lco_zeroLUT[i] = l;
		--k;
		if (k == 0) {
			k = j;
			--l;
			j *= 2;
		}
	}
}

/* Set thresholds to default unless specified by header: */
static void set_thresholds(int *T1p, int *T2p, int *T3p)
{
	int lambda,
		ilambda = 256 / alpha,
		quant = 2 * lco_near + 1,
		lco_T1 = *T1p,
		lco_T2 = *T2p,
		lco_T3 = *T3p;

	if (alpha < 4096)
		lambda = (alpha + 127) / 256;
	else
		lambda = (4096 + 127) / 256;

	if (lco_T1 <= 0) {
		/* compute lossless default */
		if (lambda)
			lco_T1 = lambda * (BASIC_T1 - 2) + 2;
		else {  /* alphabet < 8 bits */
			lco_T1 = BASIC_T1 / ilambda;
			if (lco_T1 < 2) lco_T1 = 2;
		}
		/* adjust for lossy */
		lco_T1 += 3 * lco_near;

		/* check that the default threshold is in bounds */
		if (lco_T1 < lco_near + 1 || lco_T1 >(alpha - 1))
			lco_T1 = lco_near + 1;         /* eliminates the threshold */
	}
	if (lco_T2 <= 0) {
		/* compute lossless default */
		if (lambda)
			lco_T2 = lambda * (BASIC_T2 - 3) + 3;
		else {
			lco_T2 = BASIC_T2 / ilambda;
			if (lco_T2 < 3) lco_T2 = 3;
		}
		/* adjust for lossy */
		lco_T2 += 5 * lco_near;

		/* check that the default threshold is in bounds */
		if (lco_T2 < lco_T1 || lco_T2 >(alpha - 1))
			lco_T2 = lco_T1;         /* eliminates the threshold */
	}
	if (lco_T3 <= 0) {
		/* compute lossless default */
		if (lambda)
			lco_T3 = lambda * (BASIC_T3 - 4) + 4;
		else {
			lco_T3 = BASIC_T3 / ilambda;
			if (lco_T3 < 4) lco_T3 = 4;
		}
		/* adjust for lossy */
		lco_T3 += 7 * lco_near;

		/* check that the default threshold is in bounds */
		if (lco_T3 < lco_T2 || lco_T3 >(alpha - 1))
			lco_T3 = lco_T2;         /* eliminates the threshold */
	}

	*T1p = lco_T1;
	*T2p = lco_T2;
	*T3p = lco_T3;
}

/* Setup Look Up Tables for quantized gradient merging */
static void prepareLUTs()
{
	int i, j, idx;

	/* Build classification tables (lossless or lossy) */

	for (i = -lutmax + 1; i < lutmax; i++) {

		if (i <= -lco_T3)        /* ...... -lco_T3  */
			idx = 7;
		else if (i <= -lco_T2)    /* -(lco_T3-1) ... -lco_T2 */
			idx = 5;
		else if (i <= -lco_T1)    /* -(lco_T2-1) ... -lco_T1 */
			idx = 3;

		else if (i <= -1)     /* -(lco_T1-1) ...  -1 */
			idx = 1;
		else if (i == 0)      /*  just 0 */
			idx = 0;

		else if (i < lco_T1)      /* 1 ... lco_T1-1 */
			idx = 2;
		else if (i < lco_T2)      /* lco_T1 ... lco_T2-1 */
			idx = 4;
		else if (i < lco_T3)      /* lco_T2 ... lco_T3-1 */
			idx = 6;
		else                    /* lco_T3 ... */
			idx = 8;

		lco_vLUT[0][i + lutmax] = CREGIONS * CREGIONS * idx;
		lco_vLUT[1][i + lutmax] = CREGIONS * idx;
		lco_vLUT[2][i + lutmax] = idx;
	}

	/*  prepare context mapping table (symmetric context merging) */
	lco_classmap[0] = 0;
	for (i = 1, j = 0; i < CONTEXTS1; i++) {
		int q1, q2, q3, n1 = 0, n2 = 0, n3 = 0, ineg, sgn;

		if (lco_classmap[i])
			continue;

		q1 = i / (CREGIONS*CREGIONS);		/* first digit */
		q2 = (i / CREGIONS) % CREGIONS;		/* second digit */
		q3 = i % CREGIONS;    			/* third digit */

		if ((q1 % 2) || (q1 == 0 && (q2 % 2)) || (q1 == 0 && q2 == 0 && (q3 % 2)))
			sgn = -1;
		else
			sgn = 1;

		/* compute negative context */
		if (q1) n1 = q1 + ((q1 % 2) ? 1 : -1);
		if (q2) n2 = q2 + ((q2 % 2) ? 1 : -1);
		if (q3) n3 = q3 + ((q3 % 2) ? 1 : -1);

		ineg = (n1*CREGIONS + n2)*CREGIONS + n3;
		j++;    /* next class number */
		lco_classmap[i] = sgn * j;
		lco_classmap[ineg] = -sgn * j;

	}

}

void lco_UnInit()
{
	free(lco_scanl0);
	lco_scanl0 = NULL;
	free(lco_scanl1);
	lco_scanl1 = NULL;
}

bool lco_Init(int maxCols)
{
	lco_maxcols = maxCols;

	/* Allocate memory pools. */
	if (!lco_scanl0) {
		lco_scanl0 = (pixel *)calloc((maxCols + LEFTMARGIN + RIGHTMARGIN), sizeof(pixel));
		if (!lco_scanl0) return false;
	}
	if (!lco_scanl1) {
		lco_scanl1 = (pixel *)calloc((maxCols + LEFTMARGIN + RIGHTMARGIN), sizeof(pixel));
		if (!lco_scanl1) {
			free(lco_scanl0);
			lco_scanl0 = NULL;
			return false;
		}
	}

	/* The thresholds for the scan. Must re-do per scan is change. */
	lco_T1 = lco_T2 = lco_T3 = 0;
	set_thresholds(&lco_T1, &lco_T2, &lco_T3);

	createzeroLUT();
	/* Prepare LUTs for context quantization */
	/* Must re-do when Thresholds change */
	prepareLUTs();
	return lco_bInitialized = true;
}

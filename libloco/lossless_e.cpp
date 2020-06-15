/* lossless_e.c --- the main pipeline which processes a scanline by doing
 *                prediction, context computation, context quantization,
 *                and statistics gathering.
 *
 * Initial code by Alex Jakulin,  Aug. 1995
 *
 * Modified and optimized: Gadiel Seroussi, October 1995
 *
 * Modified and added Restart marker and input tables by:
 * David Cheng-Hsiu Chu, and Ismail R. Ismail march 1999
 */

#include "global_e.h"
#include "bitio_e.h"

static int eor_limit;

/* Do Golomb statistics and ENCODING for LOSS-LESS images */
_inline void lossless_regular_mode(int Q, int SIGN, int Px, pixel *xp)
{
	int At, Nt, Bt, absErrval, Errval, MErrval,
		Ix = *xp;	/* current pixel */
	int	unary;
	int temp;
	byte k;

	Nt = lco_N[Q];
	At = lco_A[Q];


	/* Prediction correction (A.4.2), compute prediction error (A.4.3)
	   , and error quantization (A.4.4) */
	Px = Px + (SIGN)* lco_C[Q];
	/*Px = clipPx[Px+127];*/
	clip(Px, alpha);
	Errval = SIGN * (Ix - Px);


	/* Modulo reduction of predication error (A.4.5) */
	if (Errval < 0)
		Errval += alpha;     /* Errval is now in [0.. alpha-1] */


	/* Estimate k - Golomb coding variable computation (A.5.1) */
	{
		register int nst = Nt;
		for (k = 0; nst < At; nst <<= 1, k++);
	}
	/*k=getk[Nt][At];*/


		/* Do Rice mapping and compute magnitude of Errval */
	Bt = lco_B[Q];

	/* Error Mapping (A.5.2) */
	temp = (k == 0 && ((Bt << 1) <= -Nt));
	if (Errval >= ceil_half_alpha) {
		Errval -= alpha;
		absErrval = -Errval;
		MErrval = (absErrval << 1) - 1 - temp;
	}
	else {
		absErrval = Errval;
		MErrval = (Errval << 1) + temp;
	}


	/* update bias stats (after correction of the difference) (A.6.1) */
	lco_B[Q] = (Bt += Errval);


	/* update Golomb stats */
	lco_A[Q] += absErrval;


	/* check for lco_reset */
	if (Nt == lco_reset) {
		/* lco_reset for Golomb and bias cancelation at the same time */
		lco_N[Q] = (Nt >>= 1);
		lco_A[Q] >>= 1;
		lco_B[Q] = (Bt >>= 1);
	}
	lco_N[Q] = (++Nt);


	/* Do bias estimation for NEXT pixel */
	/* Bias cancelation tries to put error in (-1,0] (A.6.2)*/
	if (Bt <= -Nt) {

		if (lco_C[Q] > MIN_C)
			--lco_C[Q];

		if ((lco_B[Q] += Nt) <= -Nt)
			lco_B[Q] = -Nt + 1;

	}
	else if (Bt > 0) {

		if (lco_C[Q] < MAX_C)
			++lco_C[Q];

		if ((lco_B[Q] -= Nt) > 0)
			lco_B[Q] = 0;
	}

	/* Actually output the code: Mapped Error Encoding (Appendix G) */
	unary = MErrval >> k;
	if (unary < LIMIT) {
		PUT_ZEROES(unary);
		PUTBITS((1 << k) + (MErrval & ((1 << k) - 1)), k + 1);
	}
	else {
		PUT_ZEROES(LIMIT);
		PUTBITS((1 << qbpp) + MErrval - 1, qbpp + 1);
	}
}

/* Do end of run encoding for LOSSLESS images */
_inline void lossless_end_of_run(pixel Ra, pixel Rb, pixel Ix, int RItype)
{
	int Errval,
		MErrval,
		Q,
		absErrval,
		oldmap,
		k,
		At,
		unary;

	register int Nt;

	Q = EOR_0 + RItype;
	Nt = lco_N[Q];
	At = lco_A[Q];

	Errval = Ix - Rb;
	if (RItype)
		At += Nt >> 1;
	else {
		if (Rb < Ra)
			Errval = -Errval;
	}


	/* Estimate k */
	for (k = 0; Nt < At; Nt <<= 1, k++);

	if (Errval < 0)
		Errval += alpha;
	if (Errval >= ceil_half_alpha)
		Errval -= alpha;


	oldmap = (k == 0 && Errval && (lco_B[Q] << 1) < Nt);
	/*  Note: the Boolean variable 'oldmap' is not
		identical to the variable 'map' in the
		JPEG-LS draft. We have
		oldmap = (Errval<0) ? (1-map) : map;
	*/

	/* Error mapping for run-interrupted sample (Figure A.22) */
	if (Errval < 0) {
		MErrval = -(Errval << 1) - 1 - RItype + oldmap;
		lco_B[Q]++;
	}
	else
		MErrval = (Errval << 1) - RItype - oldmap;

	absErrval = (MErrval + 1 - RItype) >> 1;

	/* Update variables for run-interruped sample (Figure A.23) */
	lco_A[Q] += absErrval;
	if (lco_N[Q] == lco_reset) {
		lco_N[Q] >>= 1;
		lco_A[Q] >>= 1;
		lco_B[Q] >>= 1;
	}

	lco_N[Q]++; /* for next pixel */

	/* Do the actual Golomb encoding: */
	eor_limit = LIMIT - lco_limit_reduce;
	unary = MErrval >> k;
	if (unary < eor_limit) {
		PUT_ZEROES(unary);
		PUTBITS((1 << k) + (MErrval & ((1 << k) - 1)), k + 1);
	}
	else {
		PUT_ZEROES(eor_limit);
		PUTBITS((1 << qbpp) + MErrval - 1, qbpp + 1);
	}
}

static void process_run(int runlen, int eoline)
{
	int hits = 0;


	while (runlen >= (long)lco_melcorder) {
		hits++;
		runlen -= lco_melcorder;
		if (lco_melcstate < MELCSTATES) {
			lco_melclen = lco_J[++lco_melcstate];
			lco_melcorder = (1L << lco_melclen);
		}
	}

	/* send the required number of "hit" bits (one per occurrence
	   of a run of length lco_melcorder). This number is never too big:
	   after 31 such "hit" bits, each "hit" would represent a run of 32K
	   pixels.
	*/
	PUT_ONES(hits);

	if (eoline == EOLINE) {
		/* when the run is broken by end-of-line, if there is
		   a non-null remainder, send it as if it were
		   a max length run */
		if (runlen)
			PUT_ONES(1);
		return;
	}

	/* now send the length of the remainder, encoded as a 0 followed
	   by the length in binary representation, to lco_melclen bits */
	lco_limit_reduce = lco_melclen + 1;
	PUTBITS(runlen, lco_limit_reduce);

	/* adjust melcoder parameters */
	if (lco_melcstate) {
		lco_melclen = lco_J[--lco_melcstate];
		lco_melcorder = (1L << lco_melclen);
	}
}

/* For line and plane interleaved mode in LOSS-LESS mode */

void lossless_doscanline(pixel *psl,            /* previous scanline */
	pixel *sl,             /* current scanline */
	int no)     /* number of values in it */

/*** watch it! actual pixels in the scan line are numbered 1 to no .
	 pixels with indices < 1 or > no are dummy "border" pixels  */
{
	int i;
	pixel Ra, Rb, Rc, Rd,   /* context pixels */
		Ix,	            /* current pixel */
		Px; 				/* predicted current pixel */

	int SIGN;			    /* sign of current context */
	int cont;				/* context */

	i = 1;    /* pixel indices in a scan line go from 1 to no */

	/**********************************************/
	/* Do for all pixels in the row in 8-bit mode */
	/**********************************************/

	Rc = psl[0];
	Rb = psl[1];
	Ra = sl[0];

	/*	For 8-bit Image */

	do {
		int RUNcnt;

		Ix = sl[i];
		Rd = psl[i + 1];

		/* Context determination */

		/* Quantize the gradient */
		/* partial context number: if (b-e) is used then its
			contribution is added after determination of the run state.
			Also, sign flipping, if any, occurs after run
			state determination */


		cont = lco_vLUT[0][Rd - Rb + LUTMAX8] +
			lco_vLUT[1][Rb - Rc + LUTMAX8] +
			lco_vLUT[2][Rc - Ra + LUTMAX8];

		if (cont == 0)
		{
			/*************** RUN STATE ***************************/

			RUNcnt = 0;

			if (Ix == Ra) {
				while (1) {

					++RUNcnt;

					if (++i > no) {
						/* Run-lenght coding when reach end of line (A.7.1.2) */
						process_run(RUNcnt, EOLINE);
						return;	 /* end of line */
					}

					Ix = sl[i];

					if (Ix != Ra)	/* Run is broken */
					{
						Rd = psl[i + 1];
						Rb = psl[i];
						break;  /* out of while loop */
					}
					/* Run continues */
				}
			}

			/* we only get here if the run is broken by
				a non-matching symbol */

				/* Run-lenght coding when end of line not reached (A.7.1.2) */
			process_run(RUNcnt, NOEOLINE);


			/* This is the END_OF_RUN state */
			lossless_end_of_run(Ra, Rb, Ix, (Ra == Rb));

		}
		else {

			/*************** REGULAR CONTEXT *******************/

			predict(Rb, Ra, Rc);

			/* do symmetric context merging */
			cont = lco_classmap[cont];

			if (cont < 0) {
				SIGN = -1;
				cont = -cont;
			}
			else
				SIGN = +1;

			/* output a rice code */
			lossless_regular_mode(cont, SIGN, Px, &Ix);
		}

		/* context for next pixel: */
		sl[i] = Ix;

		Ra = Ix;
		Rc = Rb;
		Rb = Rd;
	} while (++i <= no);

}

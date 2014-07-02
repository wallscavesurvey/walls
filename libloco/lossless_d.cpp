/* lossless_d.c --- the main pipeline which processes a scanline by doing
 *                prediction, context computation, context quantization,
 *                and statistics gathering. (for lossless mode)
 *
 * Initial code by Alex Jakulin,  Aug. 1995
 *
 * Modified and optimized: Gadiel Seroussi, October 1995
 *
 * Modified and added Restart marker and input tables by:
 * David Cheng-Hsiu Chu, and Ismail R. Ismail march 1999
 */

#include "global_d.h"
#include "bitio_d.h"

int lco_zeroLUT[256];      /* table to find out number of leading zeros */

#ifndef FILL_MACRO
/* loads more data in the input buffer (inline code )*/
void fillbuffer(int no)
{ 
		assert(no+lco_bits <= 24);
        lco_reg <<= no;
        lco_bits += no;
        while (lco_bits >= 0) {
            byte x = mygetc();
			if ( x == 0xff ) {
				if ( lco_bits < 8 ) {
					myungetc(0xff);
					break;
				}
				x = mygetc();
				if ( !(x & 0x80) )  { /* non-marker: drop 0 */
					lco_reg |= (0xff << lco_bits) | ((x & 0x7f)<<(lco_bits-7));
					lco_bits -= 15;
				}
				else {
					/* marker: hope we know what we're doing */
					/* the "1" bit following ff is NOT dropped */\
					lco_reg |= (0xff << lco_bits) | (x <<(lco_bits-8));
					lco_bits -= 16;
				}
				continue;
			}
            lco_reg |= x << lco_bits;
            lco_bits -= 8;
        }
}
#endif

static int eor_limit;

/* Do Golomb-Rice statistics and DECODING for LOSSLESS images */
_inline int lossless_regular_mode_d(int Q, int SIGN, int Px)
{
	int At, Bt, Nt, Errval, absErrval;
	int current, k;

	/* This function is called only for regular contexts. 
	   End_of_run context is treated separately */

	Nt = lco_N[Q];
	At = lco_A[Q];
	{
		/* Estimate k */
	    register int nst = Nt;
	    for(k=0; nst < At; nst *=2, k++);
	}

	/* Get the number of leading zeros */
	absErrval = 0;
	do {
		int temp;

		temp = lco_zeroLUT[lco_reg >> 24];
		absErrval += temp;
		if (temp != 8) {
			FILLBUFFER(temp + 1);
			break;
		}
		FILLBUFFER(8);
		if(lco_inErr>1) return -1;
	} while (1);

	if ( absErrval < LIMIT ) {
		/* now add the binary part of the Rice code */
		if (k) {
			register unsigned long temp;
			absErrval <<= k;
			GETBITS(temp,k);
			absErrval += temp;
		}
	}
	else {
	    /* the original unary would have been too long:
	       (mapped value)-1 was sent verbatim */
		GETBITS(absErrval, qbpp);
		absErrval ++;
	}

	/* Do the Rice mapping */
	if ( absErrval & 1 ) {        /* negative */
		absErrval = (absErrval + 1) / 2;
		Errval = -absErrval;
	} else {
		absErrval /= 2;
		Errval = absErrval;
	}

	Bt = lco_B[Q];

	if ( k==0 && (2*Bt <= -Nt) )
	{
		/* special case: see encoder side */
		Errval = -(Errval+1);
		absErrval = (Errval<0)? (-Errval):Errval;
	}

	/* center, clip if necessary, and mask final error */
	if ( SIGN == -1 ) {
	    Px -= lco_C[Q];
	    clip(Px, alpha);
	    
	    current = (Px - Errval)&(alpha-1);
	}
	else {
	    Px += lco_C[Q];
	    clip(Px,alpha);
	    
	    current = (Px + Errval)&(alpha-1);
	}


	/* update bias stats */
	lco_B[Q] = (Bt += Errval);

	/* update Golomb-Rice stats */
	lco_A[Q] += absErrval;


	/* check lco_reset (joint for Rice-Golomb and bias cancelation) */
	if(Nt == lco_reset) {
		lco_N[Q] = (Nt >>= 1);
		lco_A[Q] >>= 1;
		lco_B[Q] = (Bt >>= 1);
	}


	/* Do bias estimation for NEXT pixel */
	lco_N[Q] = (++Nt);
	if  ( Bt <= -Nt ) {

	    if (lco_C[Q] > MIN_C)
			--lco_C[Q];

	    Bt = (lco_B[Q] += Nt);

	    if ( Bt <= -Nt ) 
			lco_B[Q] = -Nt+1;

	} else if ( Bt > 0 ) {

	    if (lco_C[Q] < MAX_C)
			++lco_C[Q];

	    Bt = (lco_B[Q] -= Nt);

	    if ( Bt > 0 )
			lco_B[Q] = 0;
	}

	return current;
}

/* Do end of run DECODING for LOSSLESS images */
_inline pixel lossless_end_of_run_d(pixel Ra, pixel Rb, int RItype)
{

	int Ix,
		Errval,
		absErrval,
		MErrval,
		k,
		Q,
		oldmap, 
		Nt,
		At;

		Q = EOR_0 + RItype;
		Nt = lco_N[Q], 
		At = lco_A[Q];

		if ( RItype )
			At += Nt/2;

		/* Estimate k */
		for(k=0; Nt < At; Nt *=2, k++);

		/* read and decode the Golomb code */
		/* Get the number of leading zeros */
		MErrval = 0;
		do {
			int temp;

			temp = lco_zeroLUT[lco_reg >> 24];
			MErrval += temp;
			if (temp != 8) {
				FILLBUFFER(temp + 1);
				break;
			}
			FILLBUFFER(8);
			if(lco_inErr>1) return (pixel)0xFFFF; //error
		} while (1);

		eor_limit = LIMIT - lco_limit_reduce;

		if ( MErrval < eor_limit ) {
			/* now add the binary part of the Golomb code */
			if (k) {
				register unsigned long temp;
				MErrval <<= k;
				GETBITS(temp,k);
				MErrval += temp;
			}
		}
		else {
			/* the original unary would have been too long:
				(mapped value)-1 was sent verbatim */
			GETBITS(MErrval, qbpp);
			MErrval ++;
		}

		oldmap = ( k==0 && (RItype||MErrval) && (2*lco_B[Q]<Nt));
		/* 
			Note: the Boolean variable 'oldmap' is not 
			identical to the variable 'map' in the
			JPEG-LS draft. We have
			oldmap = (qdiff<0) ? (1-map) : map;
		*/

		MErrval += ( RItype + oldmap );

		if ( MErrval & 1 ) { /* negative */
			Errval = oldmap - (MErrval+1)/2;
			absErrval = -Errval-RItype;
			lco_B[Q]++;
		}
		else { /* nonnegative */
			Errval = MErrval/2;
			absErrval = Errval-RItype;
		}


		if ( Rb < Ra )
			Ix = ( Rb - Errval ) & (alpha-1);
		else   /* includes case a==b */
			Ix = ( Rb + Errval ) & (alpha-1);


		/* update stats */
		lco_A[Q] += absErrval;
		if (lco_N[Q] == lco_reset) {
			lco_N[Q] >>= 1;
			lco_A[Q] >>= 1;
			lco_B[Q] >>= 1;
		}

		lco_N[Q]++;  /* for next pixel */

		return (pixel)Ix;			
}

/* decoding routine: reads bits from the input and returns a run length. */
/* argument is the number of pixels left to	end-of-line (bound on run length) */
static int process_run_dec(int lineleft)  
{
	int runlen = 0;
		
	do {
		register int temp, hits;
		temp = lco_zeroLUT[(byte)(~(lco_reg >> 24))];   /* number of leading ones in the
							   input stream, up to 8 */
		for ( hits = 1; hits<=temp; hits++ ) 
		{
			runlen += lco_melcorder;
			if ( runlen >= lineleft )
			{ /* reached end-of-line */
				if ( runlen==lineleft && lco_melcstate < MELCSTATES ) 
				{
					lco_melclen = lco_J[++lco_melcstate];
					lco_melcorder = (1L<<lco_melclen);
				}
				FILLBUFFER(hits); /* actual # of 1's consumed */
				return lineleft; 
			}
			if ( lco_melcstate < MELCSTATES ) 
			{
				lco_melclen = lco_J[++lco_melcstate];
				lco_melcorder = (1L<<lco_melclen);
			}
		}	
		if (temp != 8) 
		{
			FILLBUFFER(temp + 1);  /* consume the leading 0 of the remainder encoding */
			break;
        }
        FILLBUFFER(8);
		if(lco_inErr>1) return -1; //error
	} while ( 1 );

	/* read the length of the remainder */
	if ( lco_melclen ) 
	{
		register int temp;
		GETBITS(temp, lco_melclen);  /*** GETBITS is a macro, not a function */
		runlen += temp;
	}
	lco_limit_reduce = lco_melclen+1;

	/* adjust melcoder parameters */
	if ( lco_melcstate ) 
	{
		lco_melclen = lco_J[--lco_melcstate];
		lco_melcorder = (1L<<lco_melclen);
	}

	return runlen;
}

/* For line and plane interleaved mode in LOSSLESS mode */

int lossless_undoscanline(	pixel *psl,			/* previous scanline */
							pixel *sl,			/* current scanline */
							int no)	/* number of values in it */
/*** watch it! actual pixels in the scan line are numbered 1 to no .
     pixels with indices < 1 or > no are dummy "border" pixels  */
{
	int i, psfix;
	pixel Ra, Rb, Rc, Rd;
	int SIGN;
	int cont;

	psfix = 0;

	/**********************************************/
	/* Do for all pixels in the row in 8-bit mode */
	/**********************************************/

	Rc = psl[0];
	Rb = psl[1];
	Ra = sl[0];

	i = 1;
	do {
		pixel Px;

		Rd = psl[i + 1];

		/* Quantize the gradient */
		cont =  lco_vLUT[0][Rd - Rb + LUTMAX8] +
				lco_vLUT[1][Rb - Rc + LUTMAX8] +
				lco_vLUT[2][Rc - Ra + LUTMAX8];

		if ( cont == 0 )  {

		/*********** RUN STATE **********/

			register int n, m;

			/* get length of the run */
			/* arg is # of pixels left */
		//START_TIMER();
			m = n = process_run_dec(no-i+1);
		//MARK_TIMER(1);

		//START_TIMER();
			if ( m > 0 )  {  /* run of nonzero length, otherwise
								we go directly to the end-of-run 
								state */
				do {
					sl[i++] = Ra;
				} while(--n > 0);

				if (i > no)
					/* end of line */
					return 0;

				/* update context pixels */
				Rb = psl[i];
				Rd = psl[i + 1];
			}
			else if(m==-1) return -1;

		//MARK_TIMER(2);

		//START_TIMER();
			/* Do end of run encoding for LOSSLESS images */
			Ra = lossless_end_of_run_d(Ra, Rb, (Ra==Rb));
			if(Ra==(pixel)0xFFFF) return -1;
		//MARK_TIMER(3);

		}       /* Run state block */ 
		else {
			/************ REGULAR CONTEXT **********/

			//START_TIMER();
				//predict(Rb, Ra, Rc);

			if (Rb > Ra) {
				if (Rc >= Rb)
					Px = Ra;
				else if (Rc <= Ra)
					Px = Rb;
				else
					Px = Ra + Rb - Rc;
			}
			else {
				if (Rc >= Ra)
					Px = Rb;
				else if (Rc <= Rb)
					Px = Ra;
				else
					Px = Ra + Rb - Rc;
			}

			//MARK_TIMER(4);

			//START_TIMER();
			/* map symmetric contexts */
			cont = lco_classmap[cont];

			if (cont < 0) 
			{
				SIGN = -1;
				cont = -cont;
			}
			else
				SIGN = +1;

			/* decode a Rice code of a given context */
			Ra = lossless_regular_mode_d(cont, SIGN, Px);
			if(Ra==(pixel)0xFFFF) return -1;
			//MARK_TIMER(5);
		}

		sl[i] = Ra;
		Rc = Rb;
		Rb = Rd;
		++i;
	} while (i <= no);

	return 0;
}

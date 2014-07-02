
/* encoder.c --- the main module, argument parsing, file I/O
 *
 * 
 * Initial code by Alex Jakulin,  Aug. 1995
 *
 * Modified and optimized: Gadiel Seroussi, October 1995
 *
 * Color Enhancement: Guillermo Sapiro, August 1996
 *
 * Modified and added Restart marker and input tables by:
 * David Cheng-Hsiu Chu, and Ismail R. Ismail march 1999
 */

#include <string.h>
#include <math.h>
#include "global_e.h"
#include "bitio_e.h"

/* Flushes the bit output buffer and the byte output buffer */
static void bitoflush() {
	register unsigned int outbyte;
    
    while (lco_bits < 32) {
		outbyte = lco_reg >> 24;
        myputc(outbyte);
		if ( outbyte == 0xff ) {
			lco_bits += 7;
			lco_reg <<= 7;
			lco_reg &= ~(1<<(8*sizeof(lco_reg)-1)); /* stuff a 0 at MSB */
		} else {
		    lco_bits += 8;
		    lco_reg <<= 8;
		}
	}
}

static void write_marker(int marker)
/* Write a two-byte marker (just the marker identifier) */
{
	myputc((byte)((marker>>8)&0xFF));
	myputc((byte)(marker&0xFF));
}

/* Initialize lco_A[], lco_B[], lco_C[], and lco_N[] arrays */
static void init_stats() 
{
	int i, initabstat, slack;

	slack = 1<<INITABSLACK;
	initabstat = (alpha + slack/2)/slack;
	if ( initabstat < MIN_INITABSTAT ) initabstat = MIN_INITABSTAT;

	/* do the statistics initialization */
	for (i = 0; i < TOT_CONTEXTS; ++i) {
		lco_C[i]= lco_B[i] = 0;
		lco_N[i] = INITNSTAT;
		lco_A[i] = initabstat;
	}
}

int lco_Encode(byte *src,int srcRows,int srcCols)
{
	//Returns 0 or an error code>0
	//In either case, nti_errno will have been set to NTI_ErrWrite if
	//a flush error occurred at a lower level. This is only possible
	//when the generated output is longer than the output buffer.

	if(nti_errno || (!lco_bInitialized && !lco_Init(NTI_BLKWIDTH))) return LCO_ErrInit;

	lco_bits = 32;
	lco_reg = 0;

	/* Write SOI */
	write_marker(SOI);

	/* Write the frame */
	write_marker(srcRows);
	write_marker(srcCols);

	/* initialize stats arrays */
	init_stats();

	/* initialize run processing */
	lco_melcstate = 0;
	lco_melclen = lco_J[0];
	lco_melcorder = 1<<lco_melclen;

	/* Initialize the scanline buffers */
	memset(lco_scanl0,0,(srcCols+3)*sizeof(pixel));
	lco_pscanl = lco_scanl0 + (LEFTMARGIN-1);
	lco_cscanl = lco_scanl1 + (LEFTMARGIN-1);

	for(int n=0;n<srcRows;n++) {

		for(int i=1; i<=srcCols; i++) lco_cscanl[i] = *src++;

		/* 'extend' the edges */
		lco_cscanl[-1]=lco_cscanl[0]=lco_pscanl[1];

		/* process the lines */
		lossless_doscanline(lco_pscanl, lco_cscanl, srcCols);

		/* 'extend' the edges */
		lco_cscanl[srcCols+1] = lco_cscanl[srcCols];

		/* make the current scanline the previous one */
		{
			pixel *temp = lco_pscanl;
			lco_pscanl = lco_cscanl;
			lco_cscanl = temp;
		}

	}
	bitoflush();
	write_marker(EOI);
	return 0;
}

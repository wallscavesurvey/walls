/* decoder.c --- the main module, argument parsing, file I/O 
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

#include "global_ed.h"
#include "bitio_d.h"
#include <string.h>

/*Some I/O Routines --*/

static int get_marker()
/* Seeks a marker in the input stream. Returns the marker */
{
	int c;

	if(mygetc()!=0xFF || !((c=mygetc())&0x80)) return -1;
	return (0xFF00|c);
}
	
static unsigned int read_n_bytes( int n)
/* reads n bytes (0 <= n <= 4) from the input stream */
{
    unsigned int m = 0;

    while(n--) m = (m << 8) | mygetc();
    return m;
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

/* Write one row of pixel values */
_inline void write_one_line(pixel* line, int cols)
{
	if(lco_outPtr+cols>lco_outEOF) lco_outErr=1;
	else
		for(int i=0; i< cols; i++)	*lco_outPtr++=(byte)(*(line+i));
}

/* creates the bit counting look-up table. */

int lco_Decode(byte *dst,int sizDst,int sizSrc)
{
	//NOTE: src (start of data) is 4 bytes past start of input buffer,
	//hence size of input buffer is actually sizSrc+4
	word columns, rows;

	if(!lco_bInitialized && !lco_Init(NTI_BLKWIDTH))
		return LCO_ErrInit;

	//START_TIMER();

	/* Parse the parameters, initialize */
	//NOTE: src (start of data) is 4 bytes past start of input buffer,
	//hence size of input buffer is actually sizSrc+4
	//lco_inPtr=_nti_buffer_pos;
	lco_inEOF=_nti_buffer_pos+sizSrc;
	lco_outPtr=dst;
	lco_outEOF=dst+sizDst;

	lco_outErr=lco_inErr=0;

	/* Read SOI */
	if (get_marker() != SOI) return LCO_ErrNoSOI;

	rows=(word)read_n_bytes(2);
	columns=(word)read_n_bytes(2);

	if(!rows || !columns || rows>lco_maxcols || columns>lco_maxcols) {
		//assert(0);
		return LCO_ErrInitParams;
	}

	/* Initialize the scanline buffers */
	memset(lco_scanl0,0,(columns+3)*sizeof(pixel));
	lco_pscanl = lco_scanl0 + (LEFTMARGIN-1);
	lco_cscanl = lco_scanl1 + (LEFTMARGIN-1);

	/* Initializations for each scan */

	init_stats();

	/* Initialize run processing */
	lco_melcstate = 0;
	lco_melclen = lco_J[0];
	lco_melcorder = 1<<lco_melclen;

    lco_bits = lco_reg = 0;
    FILLBUFFER(24);

	//MARK_TIMER(0);

	/***********************************************************************/
	/*           Plane interleaved mode 				       */
	/***********************************************************************/
	for(word n=0;n<rows;n++) {
		/* 'extend' the edges */
		lco_cscanl[-1]=lco_cscanl[0]=lco_pscanl[1];

		//START_TIMER();
		if(lossless_undoscanline(lco_pscanl, lco_cscanl, columns)) return lco_inErr;
		//MARK_TIMER(1);

		//START_TIMER();
		write_one_line(lco_cscanl+1, columns);
		//MARK_TIMER(6);

		/* extend the edges */
		lco_cscanl[columns+1] = lco_cscanl[columns];

		/* make the current scanline the previous one */
		{
			pixel *temp = lco_pscanl;
			lco_pscanl = lco_cscanl;
			lco_cscanl = temp;
		}
	}

	//START_TIMER();

	_nti_buffer_pos-=2;

	/* Read EOI */
	if (get_marker() != EOI || lco_outPtr!= lco_outEOF || lco_outErr)
	{
		return LCO_ErrDecodeLen;
	}

	//MARK_TIMER(7);
	//END_TIMER(7);

	return 0;
}


/**********************************************************
** Copyright 1998 Earth Resource Mapping Ltd.
** This document contains proprietary source code of
** Earth Resource Mapping Ltd, and can only be used under
** one of the three licenses as described in the
** license.txt file supplied with this distribution.
** See separate license.txt file for license details
** and conditions.
**
** This software is covered by US patent #6,442,298,
** #6,102,897 and #6,633,688.  Rights to use these patents
** is included in the license agreements.
**
** FILE:   	collapse_pyr.c
** CREATED:	1998
** AUTHOR: 	SNS
** PURPOSE:	collapse pryamid functions
** EDITS:
** [01] sns	10sep98	Created file
** [02] sns	10mar99	Updated to NCS logic
** [03] rar	17-Jan-01 Mac port changes
 *******************************************************/


#include "NCSEcw.h"
#if !defined(_WIN32_WCE)
#ifdef WIN32
#define _CRTDBG_MAP_ALLOC
#include "crtdbg.h"
#endif
#endif
#include "NCSLog.h"
#include "NCSBuildNumber.h"

static int qdecode_qmf_line_sidebands(QmfRegionLevelStruct	*p_level);
static int qdecode_qmf_line_a_sideband(QmfRegionLevelStruct *p_level, INT32 *p_sideband);
static int qdecode_qmf_line_blocks(QmfRegionLevelStruct	*p_level);

/*
**
**	[see erw_decompress_start_region() header for more information]
**
**	qdecode_qmf_level_line() -- collapse (reconstruct) a single line from a QMF-style pyramid
**	using a 3-tap (1-2-1) filter. This means we need at any time two input lines in memory
**	(recursively for each level) for any one output line.
**
**	For every two calls to this routine, one line of input LL and other sideband data
**	is generated by recursive calls to smaller levels. This is how expansion of
**	data is carried out.  This routine uses a 3 tap decompression, so we only need 3 lines
**	of subbands present at any time to preform the recursive reconstruction.
**
**	The data flow is complicated by the blocking of compressed data on disk. We have to retrieve
**	the set of blocks required in the X direction to expansion of a line.  However, we DO NOT decompress
**	the all lines for the set of blocks Y blocks, instead we decompress one line at a time.
**	The reasons for this require a bit of explanation:
**
**	1) We don't want to have all lines of a decompressed set of blocks in memory at the same time.
**	This is for memory usage reasons; if (for example) the Y direction blocking factor is 64MB,
**	and we have a TB file with 1,000,000 data lines in the X direction, and we want to decompress
**	the entire line (for example if they are decompressing the entire file for some reason such
**	as merging and recompressing it with other data), we would need 64 * 3 * 500,000 * 32 bits of RAM
**	e.g. 384MB of RAM to hold one line, at 2x that for all levels, e.g. over 600MB of RAM. This is
**	not desirable.
**
**	2) Also, we want to work with the CPU L1 and L2 cache systems as much as possible.  So what we
**	really want to do is decompress a line, use it immediately, and then discard it. This happens
**	to be the way the recursive logic works - it only wants 3 lines in memory during recursive
**	decompression, so for even very large files the 3 lines are likely to fit in CPU L1 and/or L2 cache
**	(and only two lines are used in sequence for any particular subband),
**	resulting in a very significant performance boost as everything can run out of cache.
**
**	3) By keeping blocks compressed, we can potentially cache them (or let the OS do the caching),
**	further boosting performance.
**
**	4) By keeping issues like blocking away from the core recursive QMF rebuilt routines,
**	we reuse the routine with minimal changes for other applications, such as streaming
**	data via transmission from the compression logic (which can also directly compress line by line)
**	straight into the decompression routine.
**
**
**	PROCESSING LOGIC:
**
**	Because of this, our logic is as follows:
**
**	for( number of lines to extract ) {
**		request_line()
**	}
**
**
**	request_line() is actually the top level call into qdecode_qmf_level_line(). That routine
**	in turn recursivelly requests smaller QMF level lines to return LL lines.  The recursive
**	call, once it gets the LL data, then calls the decompression to decompress a single line
**	of the compressed block data (thus keeping blocking well away from the decompression routine),
**	expanding each smaller level line into two lines of this level, and so on up the recursive levels.
**
**	Recall that as this uses a 3 tap filter, two subband lines must be kept in memory at one
**	time.  We store these in a ring buffer, rolling the ring buffer points down by one each
**	time we want to decompress another line of input subbands from input.
**
**	The only final point to note is that line [-1] is a reflection of line [0], and line
**	[y_size] is a reflection of line [y_size - 1], which is done as a test for these lines,
**	and using the reflected line when we have to.
**
**
** Data expansion is expanded by a simple filter as follows for a given [x,y]:
**
** First set baseline LL values:
**	O[0,0]  =  LL[0,0];									O[1,0]	= (LL[0,0]+LL[1,0])/2;
**	O[0,1]  = (LL[0,0]+LL[0,1])/2;						O[1,1]	= (LL[0,0]+LL[1,0]+LL[0,1]+LL[1,1])/4;
**
** Add in LH values. These are added to the above. Input is offset -1 in the Y axis
**	O[0,0] -= (LH[0,-1]+LH[0,0])/2;						O[1,0] -= (LH[0,-1]+LH[1,-1]+LH[0,0]+LH[1,0])/4;
**	O[0,1] +=  LH[0,0];									O[1,1] += (LH[0,0]+LH[1,0])/2;
**
** Add in HL values. These are added to the above. Input is offset -1 in the X axis
**	O[0,0] -= (HL[-1,0]+HL[0,0])/2;						O[1,0] +=  HL[0,0];
**	O[0,1] -= (HL[-1,0]+HL[0,0]+HL[-1,1]+HL[0,1])/4;	O[1,1] += (HL[0,0]+HL[0,1])/2;
**
** Add in HH values. These are added to the above. Input is offset -1 in the X axis and -1 in the Y axis
**	O[0,0] += (HH[-1,-1]+HH[0,-1]+HH[-1,0]+HH[0,0])/4;	O[1,0] -= (HH[0,-1]+HH[0,0])/2;
**	O[0,1] -= (HH[-1,0]+HH[0,0])/2;						O[1,1] +=  HH[0,0];
**
** When when considered relative to the start of the 2 input values for output, converts to:
**
**	O[0,0]  =  LL[1,1];									O[1,0]	= (LL[0,1]+LL[1,1])/2;
**	O[0,0] -= (LH[1,0]+LH[1,1])/2;						O[1,0] -= (LH[0,0]+LH[1,0]+LH[0,1]+LH[1,1])/4;
**	O[0,0] -= (HL[0,1]+HL[1,1])/2;						O[1,0] +=  HL[0,1];
**	O[0,0] += (HH[0,0]+HH[1,0]+HH[0,1]+HH[1,1])/4;		O[1,0] -= (HH[0,0]+HH[0,1])/2;
**
**	O[0,1]  = (LL[1,0]+LL[1,1])/2;						O[1,1]	= (LL[0,0]+LL[1,0]+LL[0,1]+LL[1,1])/4;
**	O[0,1] +=  LH[1,0];									O[1,1] += (LH[0,0]+LH[1,0])/2;
**	O[0,1] -= (HL[0,0]+HL[1,0]+HL[0,1]+HL[1,1])/4;		O[1,1] += (HL[0,0]+HL[0,1])/2;
**	O[0,1] -= (HH[0,0]+HH[1,0])/2;						O[1,1] +=  HH[0,0];
**
** Important things to note:
**	1.	Every 2nd line of output will only use one line of input for a given sideband,
**		enabling certain optimizations to be carried out.  Ditto for X direction.
**	2.	We roll input lines through, so line0 = line1; line1 = <new sideband line>
**	3.	For X<1 or Y<1 on output, we have negative input locations. These get reflected,
**		for example input value [-1,0] becomes [1,0] and [-1,-1] becomes [1,1]
**	4.	We always request one more X pixel to the left and one more X pixel to the right
**		of the line actually needed (so the buffer can potentially be 2 values larger than
**		the actual width of data); these are used for reflection handling on the edges (if really
**		at the edge of data), or for gathering left/right data as required.
**
**	A single output line is generated by a combination of two input lines.
**
*/

int qdecode_qmf_level_line(QmfRegionStruct *p_region, UINT16 level, UINT32 y_line,
	IEEE4 **p_p_output_line)
{
	IEEE4	*p_temp;
	UINT32	band;
	Sideband	sideband;
	QmfRegionLevelStruct	*p_level;

	p_level = &(p_region->p_levels[level]);

	/*
	**	First, recurse up the levels to pre-read input LL lines needed to construct this level
	**	Then gather the LH, HL and HH lines needed. We need these every 2nd output line. We
	**	pre-read 2 input LL,LH,HL and HL lines at the start (p_level_read_lines = 1)
	*/
	while (p_level->read_lines) {
		// roll line 0 to line -1, line1 to line 0
		for (band = 0; band < p_level->used_bands; band++) {
			for (sideband = LL_SIDEBAND; sideband < MAX_SIDEBAND; sideband++) {
				p_temp = p_level->p_p_line0[DECOMP_INDEX];
				p_level->p_p_line0[DECOMP_INDEX] = p_level->p_p_line1[DECOMP_INDEX];
				p_level->p_p_line1[DECOMP_INDEX] = p_temp;
			}
			// set up the new LL pointer, with X reflection, for recursive call
			p_level->p_p_line1_ll_sideband[band] =
				p_level->p_p_line1[band * MAX_SIDEBAND + LL_SIDEBAND] + p_level->reflect_start_x;
		}

		// recursively get LL from smaller level. note that we have to offset by the left
		// reflection value
		if (level)
			if (qdecode_qmf_level_line(p_region, (UINT16)(level - 1),
				p_level->current_line, p_level->p_p_line1_ll_sideband))
				return(1);

		if (p_level->current_line >= p_level->p_qmf->y_size) {
			// Because of potentially 2 line pre-read to fill input, we have to reflect Y lines
			// for the last call.
			for (band = 0; band < p_level->used_bands; band++) {
				for (sideband = LL_SIDEBAND; sideband < MAX_SIDEBAND; sideband++) {
					memcpy(p_level->p_p_line1[DECOMP_INDEX],
						p_level->p_p_line0[DECOMP_INDEX],
						(p_level->level_size_x + p_level->reflect_start_x + p_level->reflect_end_x)
						* sizeof(p_level->p_p_line1[0][0]));
				}
			}
		}
		else {
			// or retrieve one line of the LH, HL and HH sidebands (and LL if this is level 0) from storage
			if (qdecode_qmf_line_sidebands(p_level))
				return(1);
			// have to reflect line 1 to line 0 ONLY if line 0 read AND reflection is needed
			if (p_level->reflect_start_y && p_level->current_line == 0) {
				for (band = 0; band < p_level->used_bands; band++) {
					for (sideband = LL_SIDEBAND; sideband < MAX_SIDEBAND; sideband++) {
						memcpy(p_level->p_p_line0[DECOMP_INDEX],
							p_level->p_p_line1[DECOMP_INDEX],
							(p_level->level_size_x + p_level->reflect_start_x + p_level->reflect_end_x)
							* sizeof(p_level->p_p_line1[0][0]));
					}
				}
			}
		}


		// sort out reflection on start and end of line for all sidebands
		if (p_level->reflect_start_x) {
			for (band = 0; band < p_level->used_bands; band++)
				for (sideband = LL_SIDEBAND; sideband < MAX_SIDEBAND; sideband++)
					p_level->p_p_line1[DECOMP_INDEX][0] = p_level->p_p_line1[DECOMP_INDEX][1];
		}
		// end reflection must consider there may or may not be a start reflection offset
		if (p_level->reflect_end_x) {
			for (band = 0; band < p_level->used_bands; band++)
				for (sideband = LL_SIDEBAND; sideband < MAX_SIDEBAND; sideband++)
					p_level->p_p_line1[DECOMP_INDEX][p_level->level_size_x + p_level->reflect_start_x]
					= p_level->p_p_line1[DECOMP_INDEX][p_level->level_size_x + p_level->reflect_start_x - 1];
		}

		p_level->read_lines -= 1;
		p_level->current_line += 1;
	}

	/*
	**	Now convolve the line.  It depends on if it is an odd or even output line.
	**	Each output line uses two input lines, offset.
	**	The lines, and the X values, are always set up so 0 contains the first
	**	line & X needed, and 1 contains the second line & X needed. So if
	**	output needs input I[-1] and I[0], these are set up as 0 and 1.
	*/
	for (band = 0; band < p_level->used_bands; band++) {
		register	UINT32	x2, xoddeven;
		register	IEEE4	*p_output;

		p_output = p_p_output_line[band];
		x2 = p_level->output_level_size_x;
		xoddeven = p_level->output_level_start_x;

		/*
		**
		**	EVEN LINE VALUES	(0,2,4,...)
		**
		**	O[0,0]  =  LL[1,1];									O[1,0]	= (LL[0,1]+LL[1,1])/2;
		**	O[0,0] -= (LH[1,0]+LH[1,1])/2;						O[1,0] -= (LH[0,0]+LH[1,0]+LH[0,1]+LH[1,1])/4;
		**	O[0,0] -= (HL[0,1]+HL[1,1])/2;						O[1,0] +=  HL[0,1];
		**	O[0,0] += (HH[0,0]+HH[1,0]+HH[0,1]+HH[1,1])/4;		O[1,0] -= (HH[0,0]+HH[0,1])/2;
		**
		*/
		if ((y_line & 0x0001) == 0) {		// Y output 0,2,4,...
			register	IEEE4	*p_ll_line1;
			register	IEEE4	*p_lh_line0, *p_lh_line1;
			register	IEEE4	*p_hl_line1;
			register	IEEE4	*p_hh_line0, *p_hh_line1;

			p_ll_line1 = p_level->p_p_line1[band * MAX_SIDEBAND + LL_SIDEBAND];
			p_lh_line0 = p_level->p_p_line0[band * MAX_SIDEBAND + LH_SIDEBAND];
			p_lh_line1 = p_level->p_p_line1[band * MAX_SIDEBAND + LH_SIDEBAND];
			p_hl_line1 = p_level->p_p_line1[band * MAX_SIDEBAND + HL_SIDEBAND];
			p_hh_line0 = p_level->p_p_line0[band * MAX_SIDEBAND + HH_SIDEBAND];
			p_hh_line1 = p_level->p_p_line1[band * MAX_SIDEBAND + HH_SIDEBAND];

			while (x2) {
				if ((xoddeven & 0x0001) == 0) {	// X output 0,2,4,...
					*p_output++ = p_ll_line1[1]
						- ((p_lh_line0[1] + p_lh_line1[1] + p_hl_line1[0] + p_hl_line1[1]) * (IEEE4)0.5)
						+ ((p_hh_line0[0] + p_hh_line0[1] + p_hh_line1[0] + p_hh_line1[1]) * (IEEE4)0.25);
					p_ll_line1++;
					p_lh_line0++;
					p_lh_line1++;
					p_hl_line1++;
					p_hh_line0++;
					p_hh_line1++;
				}
				else {								// X output 1,3,5,...
					*p_output++ = (((p_ll_line1[0] + p_ll_line1[1]) - (p_hh_line0[0] + p_hh_line1[0])) * (IEEE4)0.5)
						- ((p_lh_line0[0] + p_lh_line0[1] + p_lh_line1[0] + p_lh_line1[1]) * (IEEE4)0.25)
						+ p_hl_line1[0];
				}
				x2--;
				xoddeven++;
			}
		}
		/*
		**	ODD LINE VALUES (1,3,5,...)
		**
		**	O[0,1]  = (LL[1,0]+LL[1,1])/2;						O[1,1]	= (LL[0,0]+LL[1,0]+LL[0,1]+LL[1,1])/4;
		**	O[0,1] +=  LH[1,0];									O[1,1] += (LH[0,0]+LH[1,0])/2;
		**	O[0,1] -= (HL[0,0]+HL[1,0]+HL[0,1]+HL[1,1])/4;		O[1,1] += (HL[0,0]+HL[0,1])/2;
		**	O[0,1] -= (HH[0,0]+HH[1,0])/2;						O[1,1] +=  HH[0,0];
		*/
		else {								// Y output 1,3,5,...
			register	IEEE4	*p_ll_line0, *p_ll_line1;
			register	IEEE4	*p_lh_line0;
			register	IEEE4	*p_hl_line0, *p_hl_line1;
			register	IEEE4	*p_hh_line0;

			p_ll_line0 = p_level->p_p_line0[band * MAX_SIDEBAND + LL_SIDEBAND];
			p_ll_line1 = p_level->p_p_line1[band * MAX_SIDEBAND + LL_SIDEBAND];
			p_lh_line0 = p_level->p_p_line0[band * MAX_SIDEBAND + LH_SIDEBAND];
			p_hl_line0 = p_level->p_p_line0[band * MAX_SIDEBAND + HL_SIDEBAND];
			p_hl_line1 = p_level->p_p_line1[band * MAX_SIDEBAND + HL_SIDEBAND];
			p_hh_line0 = p_level->p_p_line0[band * MAX_SIDEBAND + HH_SIDEBAND];

			while (x2) {
				if ((xoddeven & 0x0001) == 0) {	// X output 0,2,4,...
#ifdef DO_SSE
					_m128 t = _mm_set_p(p_ll_line0[1], p_ll_line1[1], p_hh_line0[0], p_hh_line0[1]);

#else
					*p_output++ = (((p_ll_line0[1] + p_ll_line1[1]) - (p_hh_line0[0] + p_hh_line0[1])) * (IEEE4)0.5)
						+ p_lh_line0[1]
						- ((p_hl_line0[0] + p_hl_line0[1] + p_hl_line1[0] + p_hl_line1[1]) * (IEEE4)0.25);
#endif
					p_ll_line0++;
					p_ll_line1++;
					p_lh_line0++;
					p_hl_line0++;
					p_hl_line1++;
					p_hh_line0++;
				}
				else {								// X output 1,3,5,...
					*p_output++ = ((p_ll_line0[0] + p_ll_line0[1] + p_ll_line1[0] + p_ll_line1[1]) * (IEEE4)0.25)
						+ ((p_lh_line0[0] + p_lh_line0[1] + p_hl_line0[0] + p_hl_line1[0]) * (IEEE4)0.5)
						+ p_hh_line0[0];
				}
				x2--;
				xoddeven++;
			}
		}
	}

	// If the next line is odd, roll the input lines and read another input on next call
	if ((y_line & 0x0001) == 0) // Y output 0,2,4,...
		p_level->read_lines = 1;

	return(0);
}


/*
**	Retrieves the input sidebands for one line (p_level->current_line) of this level.
**	If level 0, this includes LL,
**	otherwise includes just the LH,HL and HH sidebands.  If p_level->have_block is TRUE,
**	then we have already started decoding the X set of blocks that includes this line, and
**	we can just uncompress the appropriate line.
**	If have_block is FALSE, we have to first request the set of blocks covering in the X direction
**	that cover this block, prior to decoding the line. Note that all X blocks
**	for the current line are retrieved, but are held in a compressed state; only one line
**	at a time is decompressed.
**
**	Also: We handle X reflection or side data gathering, by adding in reflect_start_x offset
*/
static int qdecode_qmf_line_sidebands(QmfRegionLevelStruct	*p_level)
{

	// if we are at the end of this set of blocks (or none read yet), get the next set of blocks
	// in the X direction that cover this line.
	if (!p_level->have_blocks)
		if (qdecode_qmf_line_blocks(p_level)) {
			return(1);
		}
	// now decompress this single line into line1 (which will roll
	// down after use)
	if (unpack_line(p_level))
		return(1);

	p_level->next_input_block_y_line += 1;
	// see if we have run to the end of this block
	if ((p_level->next_input_block_y_line >= p_level->p_qmf->y_block_size) ||
		(p_level->current_line - p_level->start_line >= p_level->level_size_y - 1)) {
		unpack_finish_lines(p_level);
		p_level->have_blocks = FALSE;
	}
	return(0);
}


/*
**	Loads the set of blocks in the X direction that p_level->current_line falls into.
**	We have to do a number of things here:
**	1)	set up pointers to the array of X blocks that have been loaded
**	2)  pre-read and throw away any number of lines that are before p_level->current_line
**		(this can happen when we start reading at a line greater than the block level;
**		it will only happen once in the entire recursive reconstruction)
**	3)	Correctly set the p_level->next_input_block_y_line.
**	4)	Set the have_block value
*/
static int qdecode_qmf_line_blocks(QmfRegionLevelStruct	*p_level)
{
	UINT32	next_y_block;
	UINT32	next_y_block_line;
	UINT32	x_block_count;
	UINT32	next_x_block;

	next_y_block = p_level->current_line / p_level->p_qmf->y_block_size;

	next_y_block_line = p_level->current_line - (next_y_block * p_level->p_qmf->y_block_size);
	p_level->next_input_block_y_line = (INT16)next_y_block_line;

	/*
	**	Read the block, and set up decompression ready to decode line by line
	*/
	for (next_x_block = p_level->start_x_block, x_block_count = 0;
		x_block_count < p_level->x_block_count;
		next_x_block++, x_block_count++) {
		UINT8	*p_block;

		p_block = NCScbmReadViewBlock(p_level, next_x_block, next_y_block);	// [02]
		if (!p_block)
			return(1);
		// set up unpacking for line by line. Also skip unwanted lines at the start
		// NOTE! The unpack_start_line_block routine will NOT free the p_block
		// if there is an error

		{
#ifdef WIN32
#if !defined(_WIN32_WCE)
			static char msg[16384] = { '\0' };
#else
			char *msg = (char*)NCSMalloc(16384, TRUE);
			if (msg)
			{
#endif

				__try {
#endif
					if (unpack_start_line_block(p_level, x_block_count, p_block, next_y_block_line)) {
						NCScbmFreeViewBlock(p_level, p_block);
						return(1);
					}
#ifdef WIN32

				}
				__except (p_level->p_region->pNCSFileView->pNCSFile->bIsCorrupt ? EXCEPTION_EXECUTE_HANDLER : NCSDbgGetExceptionInfoMsg(_exception_info(), msg)) {
					if (p_level->p_qmf->level)
						p_block = p_level->p_region->pNCSFileView->pNCSFile->pLevelnZeroBlock;
					else
						p_block = p_level->p_region->pNCSFileView->pNCSFile->pLevel0ZeroBlock;

					if (!p_level->p_region->pNCSFileView->pNCSFile->bIsCorrupt) {
#if !defined(_WIN32_WCE)
						char extended_msg[16384] = { '\0' };
						sprintf(extended_msg,
							"(collapse_pyr) Version : %s\n%s",
							NCS_VERSION_STRING, msg);
						NCSLog(LOG_LOW, extended_msg);//, NCS_FILE_INVALID, (char*)NCSGetErrorText(NCS_FILE_INVALID));
#else
						char *pExtMsg = (char *)NCSMalloc(strlen(msg) + strlen(NCS_VERSION_STRING) + 50, FALSE);
						if (pExtMsg)
						{
							sprintf(pExtMsg,
								"(collapse_pyr) Version : %s\n%s",
								NCS_VERSION_STRING, msg);
							NCSLog(LOG_LOW, pExtMsg);//, NCS_FILE_INVALID, (char*)NCSGetErrorText(NCS_FILE_INVALID));
							NCSFree(pExtMsg);
						}
#endif
						p_level->p_region->pNCSFileView->pNCSFile->bIsCorrupt = TRUE;
					}
				}
#endif
#if defined(_WIN32_WCE)
				NCSFree(msg);
			}
#endif
		}
	}

	p_level->have_blocks = TRUE;
	return(0);
}


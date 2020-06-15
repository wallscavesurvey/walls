//jpeg2000 decoder for nti_file.lib
//#include "stdafx.h"
#include <string.h>
//#include <stdio.h> // so we can use `sscanf' for arg parsing.
#include <math.h>
#include <assert.h>
//#include <iostream>
//#include <fstream>
// Kakadu core includes
#include "kdu_elementary.h"
#include "kdu_messaging.h"
#include "kdu_params.h"
#include "kdu_compressed.h"
#include "kdu_sample_processing.h"
// Application includes
#include <nti_file.h>

int nti_errno_jp2k = 0;

class buf_compressed_source : public kdu_compressed_source {

public: // Member functions

	virtual int get_capabilities()
	{
		return KDU_SOURCE_CAP_SEQUENTIAL /*| KDU_SOURCE_CAP_SEEKABLE*/;
	}

	virtual bool seek(kdu_long offset)
	{ /* [SYNOPSIS] See `kdu_compressed_source::seek' for an explanation. */
		assert(false);
		return false;
	}
	virtual kdu_long get_pos()
	{ /* [SYNOPSIS]
		 See `kdu_compressed_source::get_pos' for an explanation. */
		assert(false);
		return -1;
	}

	virtual int read(kdu_byte *buffer, int num_bytes)
	{
		/* [SYNOPSIS] See `kdu_compressed_source::read' for an explanation. */
		return _nti_read(buffer, num_bytes);
	}
};

class buf_decompressor {
public: // Member functions
	buf_decompressor(kdu_compressed_source *source);
	/* The decompressor object's structure and parameters are derived from
	   the supplied `codestream' object; however, the sample precision and
	   signed/unsigned properties of the data to be written to the VIX file
	   are supplied separately, so as to ensure that they will be consistent
	   with the VIX file header which has already been written. */
	~buf_decompressor() {
		if (codestream.exists()) codestream.destroy();
		delete[] components;
	}

	void set_buffers(LPBYTE *buf)
	{
		for (int c = num_components - 1; c >= 0; c--) {
			components[c].comp_buf = *buf++;
		}
	}

	int get_num_components() { return codestream.get_num_components(); }

	void restart(kdu_compressed_source *source) { codestream.restart(source); }

	void process_frame();
	/* Decompresses the supplied codestream (must already have been created
	   with a valid compressed data source as input), into an internally
	   allocated buffer. */

private: // Declarations
	struct buf_component {
	public: // Component-wide data
		kdu_dims comp_dims; // Dimensions and location of component
		kdu_byte *comp_buf; // Points into master buffer
		int count_val; // Vertical sub-sampling factor for component
	public: // Tile-component specific data managed here
		kdu_dims tile_dims; // Size of current tile-component
		kdu_byte *bp; // Points to next row in current tile
		int counter; // Counts down from `count_val' to 0. Process line when 0
		int precision; // # bits from tile-component stored in VIX sample MSB's
		kdu_pull_ifc engine; // Sample processing engine
		kdu_line_buf line; // Manages a single line buffer
	};

private: // Data
	kdu_sample_allocator allocator; // Manages all sample processing memory
	int num_components;
	buf_component *components;
	kdu_codestream codestream;
};

static buf_decompressor *pDecompressor[2]; //contains pointers to allocated buffers
static buf_compressed_source source;

buf_decompressor::buf_decompressor(kdu_compressed_source *source)
{
	codestream.create(source);
	num_components = codestream.get_num_components();
	components = new buf_component[num_components];
	for (int c = 0; c < num_components; c++) {
		buf_component *comp = components + c;
		codestream.get_dims(c, comp->comp_dims);
		kdu_coords  subs;
		codestream.get_subsampling(c, subs);
		comp->count_val = subs.y;
		comp->comp_buf = NULL;  //Will initialize for each tile
	}
	codestream.enable_restart();
}

static void transfer_line_to_bytes(kdu_line_buf line, void *dst, int width)
{
	kdu_byte *dp = (kdu_byte *)dst;
	kdu_int32 val;
	kdu_sample16 *sp = line.get_buf16();
	if (line.is_absolute()) {
		//reversible --
		for (; width--; dp++, sp++) {
			val = (kdu_int32)sp->ival + 128;
			if (val & 0xFFFFFF00) val = (val < 0) ? 0 : 255;
			*dp = (kdu_byte)val;
		}
	}
	else {
		//non-reversible --
		kdu_int32 offset = 1 << (KDU_FIX_POINT - 1);
		offset += (1 << (KDU_FIX_POINT - 8)) >> 1;
		for (; width--; dp++, sp++) {
			val = (((kdu_int32)sp->ival) + offset) >> (KDU_FIX_POINT - 8);
			if (val & 0xFFFFFF00) val = (val < 0) ? 0 : 255;
			*dp = (kdu_byte)val;
		}
	}
}

void buf_decompressor::process_frame()
{
	int c;
	buf_component *comp;

	// Process the single tile --
	kdu_tile tile = codestream.open_tile(kdu_coords()); //{x=0,y=0}
	bool use_ycc = tile.get_ycc();
	kdu_resolution res;
	kdu_tile_comp tc;
	bool reversible, use_shorts;

	// Initialize tile-components
	for (comp = components, c = 0; c < num_components; c++, comp++) {

		tc = tile.access_component(c);
		res = tc.access_resolution();
		comp->counter = 0;
		comp->precision = tc.get_bit_depth();
		reversible = tc.get_reversible();
		res.get_dims(comp->tile_dims);
		assert((comp->tile_dims & comp->comp_dims) == comp->tile_dims);
		comp->bp = comp->comp_buf;
		use_shorts = (tc.get_bit_depth(true) <= 16);

		if (res.which() == 0)
			comp->engine = kdu_decoder(res.access_subband(LL_BAND), &allocator, use_shorts, 1.0F);
		else
			comp->engine = kdu_synthesis(res, &allocator, use_shorts, 1.0F);

		comp->line.pre_create(&allocator, comp->tile_dims.size.x, reversible, use_shorts);
	}

	// Complete sample buffer allocation
	allocator.finalize();
	for (comp = components, c = 0; c < num_components; c++, comp++)
		comp->line.create();

	// Now for the processing
	bool empty, done = false;
	while (!done) {

		done = empty = true;

		// Decompress a line for each component whose counter is 0
		for (comp = components, c = 0; c < num_components; c++, comp++)
			if (comp->tile_dims.size.y > 0)
			{
				done = false;
				comp->counter--;
				if (comp->counter >= 0)
					continue;
				empty = false;
				comp->engine.pull(comp->line, true);
			}

		// Process newly decompressed line buffers
		if (use_ycc && (components[0].counter < 0))
		{
			assert((components[1].counter < 0) &&
				(components[2].counter < 0));
			kdu_convert_ycc_to_rgb(components[0].line,
				components[1].line,
				components[2].line);
		}

		for (comp = components, c = 0; c < num_components; c++, comp++)
			if (comp->counter < 0)
			{
				comp->tile_dims.size.y--;
				comp->counter += comp->count_val;
				transfer_line_to_bytes(comp->line, comp->bp, comp->tile_dims.size.x);
				comp->bp += comp->comp_dims.size.x;
			}
	}

	// Cleanup and advance to next tile position
	for (comp = components, c = 0; c < num_components; c++, comp++) {
		comp->engine.destroy();
		comp->line.destroy();
	}
	tile.close();
	allocator.restart();
}

void jp2k_Decode_UnInit()
{
	delete pDecompressor[0]; pDecompressor[0] = NULL;
	delete pDecompressor[1]; pDecompressor[1] = NULL;
}

int jp2k_Decode(LPBYTE *pDstBufs, int nIdx)
{
	//One component if nIdx=0, three components if nIdx=1

	try {
		if (!pDecompressor[nIdx]) {
			pDecompressor[nIdx] = new buf_decompressor(&source);
			if (pDecompressor[nIdx]->get_num_components() != (nIdx ? 3 : 1))
				throw 0;
		}
		else pDecompressor[nIdx]->restart(&source);
		pDecompressor[nIdx]->set_buffers(pDstBufs);
		pDecompressor[nIdx]->process_frame();
	}
	catch (...) {
		delete pDecompressor[nIdx]; pDecompressor[nIdx] = NULL;
		return nti_errno_jp2k = JP2K_ErrFormat;
	}

	return 0;
}

LPSTR nti_jp2k_errmsg()
{
	static char msg[64]; //Careful!
	char *s;
	switch (nti_errno_jp2k) {
	case JP2K_ErrFormat: s = "File format error"; break;
	default: s = "Driver returned an error";
	}

	strcpy(msg, "jp2k: ");
	if (s) strcpy(msg + 6, s);
	nti_errno_jp2k = 0;
	return msg;
}

int nti_jp2k_d(CSH_HDRP hp, UINT typ)
{
	nti_xfr_errmsg = nti_jp2k_errmsg;

	if (!hp) {
		//Decode uninitialize
		//===Modify for different methods ===
		jp2k_Decode_UnInit();
		//============
		return 0;
	}

	if (typ) {
		assert(false); //Bad call from trx_csh.lib
		return nti_errno = NTI_ErrArg;
	}

	NTI_NO nti = (NTI_NO)hp->File;
	UINT nComp = nti->H.nBands;

	if (nti->H.nColors) {
		//Use zlib for palette index decompression --
		if (hp->Recno < nti->Lvl.rec[1]) {
			return nti_zlib_d(hp, 0);
		}
		nComp = nti->Lvl.nOvrBands;
	}

	assert(nComp == 1 || nComp == 3);

	if (nti_AllocCacheBuffers(hp, nComp)) return nti_errno;

	if (nti_OpenRecord(nti, hp->Recno)) return nti_errno;
	//_nti_bytes_toread is now set to record length with _nti_buffer_pos=_nti_buffer_end

	unsigned __int64 tm0;
	if (nti_inf_timing)
		QueryPerformanceCounter((LARGE_INTEGER *)&tm0);

	//===Modify for different methods ===

	//jp2k will request bytes from band(s) in a piecemeal fashion --
	if (nti_errno_jp2k = jp2k_Decode(hp->Reserved, nComp / 3))
		return nti_errno = NTI_ErrDecompress;
	//===================================

	if (nti_inf_timing) {
		unsigned __int64 tm1;
		QueryPerformanceCounter((LARGE_INTEGER *)&tm1);
		nti_tm_inf += (tm1 - tm0);
		nti_tm_inf_cnt++;
	}
	return 0;
}


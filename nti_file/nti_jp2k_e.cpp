//jpeg2000 encoder for nti_file.lib
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

/*****************************************************************************/
/*                         buf_target                           */
/*****************************************************************************/

class buf_compressed_target : public kdu_compressed_target {
public: // Member functions
	virtual bool write(const kdu_byte *buf, int num_bytes);
};

static buf_compressed_target target;

bool buf_compressed_target::write(const kdu_byte *buf, int num_bytes)
{
	return _nti_write(buf, num_bytes);
}

/*****************************************************************************/
/*                               kdv_compressor                              */
/*****************************************************************************/

class buf_compressor {
public: // Member functions
	buf_compressor(int nIdx, double fRate);
	/* The compressor object's structure and parameters are derived from
		the supplied `codestream' object; however, that object need not
		ever be used to actually generate compressed data.  The only
		requirement is that the `codestream' objects passed into the
		`process_frame' function all have the same structure. */
	~buf_compressor()
	{
		if (codestream.exists()) codestream.destroy();
	}

	void restart() { codestream.restart(&target); }

	kdu_uint16 get_previous_min_slope() { return previous_min_slope; }

	void load_frame(LPBYTE *src_buf)
	{
		for (int c = num_components - 1; c >= 0; c--)
			components[c].comp_buf = *src_buf++;
	}

	/* Returns true if a new frame can be loaded.  Generates a warning
	   message if only part of a frame remains in the file. */

	void process_frame(kdu_long layer_bytes);
	/* Processes the video data currently loaded into the buffer,
	   compressing it into a new field within the supplied `video' object.
	   Upon entry, the `video' object must already have an open image
	   (i.e., `video->open_image' must be called outside this function).
	   Similarly, the caller is responsible for closing the open image
	   once this function has returned.  The `codestream' object must
	   already have been created for output by the caller; its `flush'
	   function is called from within the present function, immediately
	   before returning.
		  Note that the size of the `layer_bytes' and `layer_thresholds'
	   arrays must match the `num_layer_specs' argument supplied to the
	   constructor.  The contents of these arrays are copied internally,
	   but the original values are not modified by the present function.
		  The function returns the distortion-length slope threshold
	   selected by the rate control algorithm for the final quality layer.
	   This value may be used to provide a bound for the generation of
	   coding passes in the next video frame, thereby saving computation in
	   the CPU-intensive block encoding phase.
		  The `trim_to_rate' argument is passed through to
	   `kdu_codestream::flush'.  If true, that function will perform more
	   careful rate allocation, at the expense of some computation. */
private: // Declarations

	struct buf_component {
	public: // Component-wide data
		kdu_dims comp_dims; // Total size of this component
		kdu_byte *comp_buf; // Points to 1st sample of this component in buffer
		int count_val; // Vertical sub-sampling factor for component
	public: // Tile-component specific data managed here
		kdu_dims tile_dims; // Size of current tile-component
		kdu_byte *bp; // Points to next row in current tile
		int counter; // Counts down from `count_val' to 0. Process line when 0
		int precision; // Number of MSBs used from each VIX sample word
		kdu_push_ifc engine; // Sample processing compression engine
		kdu_line_buf line; // Manages a single line buffer
	};

private: // Data
	int num_components;
	kdu_sample_allocator allocator; // Manages all sample processing memory
	buf_component *components;
	kdu_long layer_bytes;
	kdu_uint16 layer_thresholds;
	kdu_codestream codestream;
	kdu_uint16 previous_min_slope;
};

static buf_compressor *pCompressor; //contains pointers to allocated buffers

static void
transfer_bytes_to_line(void *src, kdu_line_buf line, int width)
{
	kdu_byte *sp = (kdu_byte *)src;
	if (line.get_buf16() != NULL)
	{
		kdu_sample16 *dp = line.get_buf16();
		if (line.is_absolute()) /*reversible!*/
		{
			for (; width--; dp++, sp++)
				dp->ival = (((kdu_int16)*sp) - 128);
		}
		else
		{
			for (; width--; dp++, sp++)
				dp->ival = (((kdu_int16)*sp) - 128) << (KDU_FIX_POINT - 8);
		}
	}
}

/* ========================================================================= */
/*                               kdv_compressor                              */
/* ========================================================================= */

/*****************************************************************************/
/*                        kdv_compressor::kdv_compressor                     */
/*****************************************************************************/

buf_compressor::buf_compressor(int nComp, double fRate) : num_components(nComp)
{
	siz_params siz;
	siz.set(Ssize, 0, 0, NTI_BLKWIDTH); siz.set(Ssize, 0, 1, NTI_BLKWIDTH);
	siz.set(Scomponents, 0, 0, nComp);
	for (int c = 0; c < nComp; c++) {
		siz.set(Ssigned, c, 0, false);
		siz.set(Sprecision, c, 0, 8);
		siz.set(Ssampling, c, 0, 1); siz.set(Ssampling, c, 1, 1);
	}
	((kdu_params *)&siz)->finalize();

	assert(!codestream.exists());
	codestream.create(&siz, &target); //*** assumes target is a kdu_compressed_target
	if (fRate < 0.0) codestream.access_siz()->parse_string("Creversible=yes");

	kdu_params *cod = codestream.access_siz()->access_cluster(COD_params);
	assert(cod != NULL);
	cod->set(Clayers, 0, 0,/*num_specs*/1);
	codestream.access_siz()->finalize_all();

	// Construct sample processing machinery and process frames one by one.
	codestream.enable_restart();

	assert(nComp == codestream.get_num_components());

	components = new buf_component[nComp];
	for (int c = 0; c < nComp; c++)
	{
		buf_component *comp = components + c;
		codestream.get_dims(c, comp->comp_dims);
		kdu_coords subs; codestream.get_subsampling(c, subs);
		comp->count_val = subs.y;
		// Allocate buffers in load_frame() --
	}

	// Per-frame layer bytes and slope thresholds
	layer_bytes = 0;
	layer_thresholds = 0;
	previous_min_slope = 0;
}

/*****************************************************************************/
/*                        kdv_compressor::process_frame                      */
/*****************************************************************************/

void buf_compressor::process_frame(kdu_long layer_bytes)
{
	int c;
	buf_component *comp;

	if (layer_bytes) {
		if (previous_min_slope)
			codestream.set_min_slope_threshold(previous_min_slope);
		else
			codestream.set_max_bytes(layer_bytes);
	}

	this->layer_bytes = layer_bytes;
	layer_thresholds = 0;

	// Process tiles one by one
	kdu_tile tile = codestream.open_tile(kdu_coords()); //{x=0,y=0}
	bool use_ycc = tile.get_ycc();
	assert(use_ycc == (num_components == 3));
	kdu_tile_comp tc;
	kdu_resolution res;
	bool reversible, use_shorts;

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
		assert(res.which());
		comp->engine = kdu_analysis(res, &allocator, use_shorts, 1.0F);
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

		// Load line buffers for each component whose counter is 0
		for (comp = components, c = 0; c < num_components; c++, comp++)
			if (comp->tile_dims.size.y > 0) {
				done = false;
				comp->counter--;
				if (comp->counter >= 0) continue;
				empty = false;
				int width = comp->tile_dims.size.x;
				transfer_bytes_to_line(comp->bp, comp->line, width);
				comp->bp += comp->comp_dims.size.x;
			}

		if (empty) continue;

		// Process newly loaded line buffers
		if (use_ycc && components[0].counter < 0) {
			assert((components[1].counter < 0) &&
				(components[2].counter < 0));
			kdu_convert_rgb_to_ycc(components[0].line,
				components[1].line,
				components[2].line);
		}

		for (comp = components, c = 0; c < num_components; c++, comp++)
			if (comp->counter < 0)
			{
				comp->engine.push(comp->line, true);
				comp->tile_dims.size.y--;
				comp->counter += comp->count_val;
			}
	}

	// Cleanup -- 
	for (comp = components, c = 0; c < num_components; c++, comp++)
	{
		comp->engine.destroy();
		comp->line.destroy();
	}

	tile.close();
	allocator.restart();
	codestream.flush(&this->layer_bytes, 1, &layer_thresholds, false, false);

	kdu_uint16 min_slope = layer_thresholds;
	if (layer_bytes > 0) {
		// See how close to the target size we came.  If we are a long way
		// off, adjust `min_slope' to help ensure that we do not excessively
		// truncate the code-block generation process in subsequent frames.
		// This is only a heuristic.
		kdu_long target = layer_bytes;
		kdu_long actual = this->layer_bytes;
		kdu_long gap = target - actual;
		if (gap > actual)
			min_slope = 0;
		else
			while (gap > (target >> 4))
			{
				gap >>= 1;
				if (min_slope < 64)
				{
					min_slope = 0; break;
				}
				min_slope -= 64;
			}
	}
	previous_min_slope = min_slope;
}

void jp2k_Encode_UnInit()
{
	delete pCompressor;
	pCompressor = NULL;
}

int jp2k_Encode(LPBYTE *pSrc_buf, int nComp, double fRate)
{
	//Returns size in bytes of encoded data
	//nIdx=0, if one component, or nIdx=1 if three components
	//fRate is rate parameter or bpp -- Reversible if fRate<0.0

	kdu_long layer_bytes = 0;
	bool bInitialized = false;

	if (!pCompressor) {
		bInitialized = true;
		try {
			pCompressor = new buf_compressor(nComp, fRate);
		}
		catch (...) {
			delete pCompressor;
			pCompressor = NULL;
			return 0;
		}
	}

	if (!pCompressor) {
		return 0;
	}

	try {

		pCompressor->load_frame(pSrc_buf);

		if (!bInitialized) pCompressor->restart();

		pCompressor->process_frame(
			(fRate > 0.0) ? (kdu_long)floor(fRate * 0.125 * NTI_BLKSIZE) : 0
		);
	}
	catch (...) {
		_nti_bytes_written = 0;
		return 0;
	}

	if (_nti_FlushBuffer()) return 0;

	return _nti_bytes_written;
}

double nti_jp2k_rate = -1.0;

int nti_jp2k_e(CSH_HDRP hp, UINT typ)
{
	if (!typ) return nti_jp2k_d(hp, typ);

	if (!hp) {
		//===Modify for different methods ===
		jp2k_Encode_UnInit();
		//============
		return 0;
	}

	NTI_NO nti = (NTI_NO)hp->File;
	DWORD recno = hp->Recno;
	UINT nComp = nti->H.nBands;

	if (nti->H.nColors) {
		if (recno < nti->Lvl.rec[1]) {
			//Use lossless for palette index compression --
			return nti_zlib_e(hp, typ);
		}
		nComp = nti->Lvl.nOvrBands;
	}

	//jp2k specific assert --
	assert(recno || !pCompressor);

	//Check buffer pointer array --
	assert(nComp == 1 && hp->Reserved[0] && !hp->Reserved[1] && !hp->Reserved[2] ||
		nComp == 3 && hp->Reserved[0] && hp->Reserved[1] && hp->Reserved[2]);

	if (nti_CreateRecord(nti, recno)) return nti_errno;

	unsigned __int64 tm0;
	if (nti_def_timing)
		QueryPerformanceCounter((LARGE_INTEGER *)&tm0);

	//jp2k specific section --
	//=========================
	if (!(typ = jp2k_Encode(hp->Reserved, nComp, nti_jp2k_rate))) {
		nti_xfr_errmsg = nti_jp2k_errmsg;
		nti_errno_jp2k = JP2K_ErrUnknown;
		return nti_errno = NTI_ErrCompress;
	}
	//============

	if (nti_def_timing) {
		unsigned __int64 tm1;
		QueryPerformanceCounter((LARGE_INTEGER *)&tm1);
		nti_tm_def += (tm1 - tm0);
		nti_tm_def_cnt++;
	}

	return _nti_AddOffsetRec(nti, recno, typ);
}


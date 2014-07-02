//webp encoder for nti_file.lib
#include <assert.h>
#include <webp\types.h>
#include <webp/encode.h>
#include <nti_file.h>

LPBYTE webp_buffer;

extern "C" {
	extern uint8_t* webp_encoder_buffer; //,webp_bufdst;
}

float nti_webp_quality=75.0f;
int nti_webp_preset=0;

enum { YUV_FIX = 16,                // fixed-point precision
       YUV_HALF = 1 << (YUV_FIX - 1),
       YUV_MASK = (256 << YUV_FIX) - 1,
       YUV_RANGE_MIN = -227,        // min value of r/g/b output
       YUV_RANGE_MAX = 256 + 226    // max value of r/g/b output
};

static WEBP_INLINE int VP8ClipUV(int v) {
  v = (v + (257 << (YUV_FIX + 2 - 1))) >> (YUV_FIX + 2);
  return ((v & ~0xff) == 0) ? v : (v < 0) ? 0 : 255;
}

static WEBP_INLINE int VP8RGBToY(int r, int g, int b) {
  const int kRound = (1 << (YUV_FIX - 1)) + (16 << YUV_FIX);
  const int luma = 16839 * r + 33059 * g + 6420 * b;
  return (luma + kRound) >> YUV_FIX;  // no need to clip
}

static WEBP_INLINE int VP8RGBToU(int r, int g, int b) {
  const int u = -9719 * r - 19081 * g + 28800 * b;
  return VP8ClipUV(u);
}

static WEBP_INLINE int VP8RGBToV(int r, int g, int b) {
  const int v = +28800 * r - 24116 * g - 4684 * b;
  return VP8ClipUV(v);
}

#define SUM4(ptr) ((ptr)[0] + (ptr)[3] + \
                   (ptr)[rgb_stride] + (ptr)[rgb_stride + 3])
#define SUM2H(ptr) (2 * (ptr)[0] + 2 * (ptr)[3])
#define SUM2V(ptr) (2 * (ptr)[0] + 2 * (ptr)[rgb_stride])
#define SUM1(ptr)  (4 * (ptr)[0])
#define RGB_TO_UV(x, y, SUM) {                           \
  const int src = (2 * (3 * (x) + (y) * rgb_stride)); \
  const int dst = (x) + (y) * picture->uv_stride;        \
  const int r = SUM(r_ptr + src);                        \
  const int g = SUM(g_ptr + src);                        \
  const int b = SUM(b_ptr + src);                        \
  picture->u[dst] = VP8RGBToU(r, g, b);                  \
  picture->v[dst] = VP8RGBToV(r, g, b);                  \
}
#define HALVE(x) (((x) + 1) >> 1)

static int webp_PictureAlloc(WebPPicture* picture)
{
    const int width = picture->width;
    const int height = picture->height;
	const int y_stride = width;
	const int uv_width = HALVE(width);
	const int uv_height = HALVE(height);
	const int uv_stride = uv_width;
	int uv0_stride = 0;
	int a_width, a_stride;
	uint32_t y_size, uv_size, uv0_size, a_size, total_size; //was uint64_t
	uint8_t* mem;

	uv0_size = (uint32_t)(height * uv0_stride);

	// alpha
	a_width = a_stride = 0;
	y_size = (uint32_t)(y_stride * height);
	uv_size = (uint32_t)(uv_stride * uv_height);
	a_size = 0;

	total_size = y_size + a_size + 2 * uv_size + 2 * uv0_size; //98304

	assert(total_size<=98304);

	// Security and validation checks
	if (width <= 0 || height <= 0 ||         // luma/alpha param error
			uv_width < 0 || uv_height < 0) {     // u/v param error
		return 0;
	}

	mem=(uint8_t*)webp_encoder_buffer+(32*1024); //96K available

	// From now on, we're in the clear, we can no longer fail...
	picture->memory_ = (void*)mem;
	picture->y_stride  = y_stride;
	picture->uv_stride = uv_stride;
	picture->a_stride  = a_stride;
	picture->uv0_stride = uv0_stride;
	// TODO(skal): we could align the y/u/v planes and adjust stride.
	picture->y = mem;
	mem += y_size;

	picture->u = mem;
	mem += uv_size;
	picture->v = mem;
	mem += uv_size;

	if (a_size) {
		picture->a = mem;
		mem += a_size;
	}
	if (uv0_size) {
		picture->u0 = mem;
		mem += uv0_size;
		picture->v0 = mem;
		mem += uv0_size;
	}
    return 1;
}

static int webp_Import(WebPPicture* const picture)
{
	const uint8_t* const b_ptr = webp_buffer;  //bgr
	const uint8_t* const g_ptr = b_ptr + 1;
	const uint8_t* const r_ptr = g_ptr + 1;
	const int width = picture->width;
	const int height = picture->height;
	int rgb_stride=(3*width+3)&~3;
	int x, y;

	if (!webp_PictureAlloc(picture)) return 0;

	// Import luma plane
	for (y = 0; y < height; ++y) {
		for (x = 0; x < width; ++x) {
			const int offset = 3 * x + y * rgb_stride;
			picture->y[x + y * picture->y_stride] =
				VP8RGBToY(r_ptr[offset], g_ptr[offset], b_ptr[offset]);
		}
	}

	// Downsample U/V plane
	for (y = 0; y < (height >> 1); ++y) {
		for (x = 0; x < (width >> 1); ++x) {
		RGB_TO_UV(x, y, SUM4);
		}
		if (width & 1) {
		RGB_TO_UV(x, y, SUM2V);
		}
	}
	if (height & 1) {
		for (x = 0; x < (width >> 1); ++x) {
		RGB_TO_UV(x, y, SUM2H);
		}
		if (width & 1) {
		RGB_TO_UV(x, y, SUM1);
		}
	}

  return 1;
}

int webp_MemoryWrite(const uint8_t* data, size_t data_size,const WebPPicture* picture)
{
  WebPMemoryWriter* const w = (WebPMemoryWriter*)picture->custom_ptr;
  if (data_size>0) {
	if(w->size+data_size > w->max_size) return 0;
    memcpy(w->mem + w->size, data, data_size);
	w->size+=data_size;
  }
  return 1;
}

static UINT webp_Encode(UINT width,UINT height, LPBYTE *pResult)
{
  WebPPicture pic;
  WebPConfig config;
  WebPMemoryWriter wrt;

  if (!WebPConfigPreset(&config,(WebPPreset)nti_webp_preset,nti_webp_quality) ||
      !WebPPictureInit(&pic)) {
    return 0;
  }

  //config.lossless = pic.use_argb = (nti_webp_quality==0.0f); //!!lossless;
  pic.width = width;
  pic.height = height;
  pic.writer = webp_MemoryWrite;
  pic.custom_ptr = &wrt;
  wrt.mem = webp_buffer+3*NTI_BLKSIZE;
  wrt.size = 0;
  wrt.max_size = 3*NTI_BLKSIZE;

  if(!webp_Import(&pic) || !WebPEncode(&config, &pic)) {
    return 0;
  }

  *pResult = wrt.mem;
  return wrt.size;
}

int nti_webp_e(CSH_HDRP hp, UINT typ)
{
	if(!typ) return nti_webp_d(hp, typ);

	if(!hp) {
		free(webp_encoder_buffer);
		webp_encoder_buffer=NULL;
		return 0;
	}

	DWORD recno=hp->Recno;
	NTI_NO nti=(NTI_NO)hp->File;

	if(nti->H.nColors && recno<nti->Lvl.rec[1]) {
		//Use lossless for palette index compression --
		return nti_zlib_e(hp, typ);
	}

	nti_xfr_errmsg=nti_webp_errmsg;

	if(!webp_buffer) webp_buffer=(LPBYTE)malloc(6*NTI_BLKSIZE); //192K + 192K
	if(!webp_encoder_buffer) webp_encoder_buffer=(LPBYTE)malloc((96+32)*1024); //128K

	//Check buffer pointer array --
	assert(hp->Buffer == *hp->Reserved && (nti->H.nBands==1 || hp->Reserved[1] && hp->Reserved[2]));

	UINT height, width;

	nti_GetRecDims(nti, recno, &width, &height);

	if(nti_CreateRecord(nti, recno))
		return nti_errno;

	//_nti_buffer_pos has been set to _nti_buffer with _nti_bytes_written=0
	//We'll NOT use _nti_buffer, only _nti_handle, which has been set to offset
	//of encoded length DWORD that we'll write, not WORD!

	assert(_nti_buffer_pos==_nti_buffer); //we'll not use this

	unsigned __int64 tm0;
	if(nti_def_timing)
		QueryPerformanceCounter((LARGE_INTEGER *)&tm0);

	LPBYTE pB=hp->Reserved[0], pG=hp->Reserved[1], pR=hp->Reserved[2];
	if(!pG || !pR) {
		//Source must be grayscale, WebP can only compress RGB!
		assert(!pG && !pR && nti->H.nBands == 1 && !nti->H.nColors);
		pG=pR=pB;
	}

	UINT gapBGR=NTI_BLKWIDTH-width;
	UINT stride=(3*width+3)&~3; //mult of 4
	UINT gapSrc=stride-3*width;
	LPBYTE pSrcLim=webp_buffer+height*stride;

	for(LPBYTE pSrc=webp_buffer; pSrc<pSrcLim; pSrc+=gapSrc, pB+=gapBGR, pG+=gapBGR, pR+=gapBGR) {
		for(UINT r=0; r<width; r++) {
			*pSrc++=*pB++;
			*pSrc++=*pG++;
			*pSrc++=*pR++;
		}
	}

	typ = webp_Encode(width, height, &pSrcLim);

	if(typ<=0)
		return nti_errno=NTI_ErrCompress;

	DWORD sizEncoded=typ;

	if( !::WriteFile(_nti_handle, &sizEncoded, sizeof(DWORD), (DWORD *)&typ, NULL) || sizeof(DWORD)!=(DWORD)typ ||
		!::WriteFile(_nti_handle, pSrcLim, sizEncoded, (DWORD *)&typ, NULL) || sizEncoded!=(DWORD)typ)
		return nti_errno=NTI_ErrWrite;

	if(nti_def_timing) {
		unsigned __int64 tm1;
		QueryPerformanceCounter((LARGE_INTEGER *)&tm1);
		nti_tm_def+=(tm1-tm0);
		nti_tm_def_cnt++;
	}

	return _nti_AddOffsetRec(nti,recno,sizEncoded+sizeof(DWORD));
}

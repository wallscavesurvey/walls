/*
 * Copyright (c) 2011, Tom Distler (http://tdistler.com)
 * All rights reserved.
 *
 * The BSD License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * - Neither the name of the tdistler.com nor the names of its contributors may
 *   be used to endorse or promote products derived from this software without
 *   specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "convolve.h"
#include <stdlib.h>

float KBND_SYMMETRIC(const float *img, int w, int h, int x, int y, float bnd_const)
{
	if (x < 0) x = -1 - x;
	else if (x >= w) x = (w - (x - w)) - 1;
	if (y < 0) y = -1 - y;
	else if (y >= h) y = (h - (y - h)) - 1;
	return img[y*w + x];
}

void _iqa_convolve(float *img, int w, int h, const struct _kernel *k, float *result, int *rw, int *rh)
{
	int x, y, kx, ky, u, v;
	int uc = k->w / 2;
	int vc = k->h / 2;
	int kw_even = (k->w & 1) ? 0 : 1;
	int kh_even = (k->h & 1) ? 0 : 1;
	int dst_w = w - k->w + 1;
	int dst_h = h - k->h + 1;
	int img_offset, k_offset;
	double sum;
	//float scale; //==1.0f
	float *dst = result;

	if (!dst)
		dst = img; /* Convolve in-place */

	/* Kernel is applied to all positions where the kernel is fully contained
	 * in the image */
	 //scale = _calc_scale(k); //==1.0f
	for (y = 0; y < dst_h; ++y) {
		for (x = 0; x < dst_w; ++x) {
			sum = 0.0;
			k_offset = 0;
			ky = y + vc;
			kx = x + uc;
			for (v = -vc; v <= vc - kh_even; ++v) {
				img_offset = (ky + v)*w + kx;
				for (u = -uc; u <= uc - kw_even; ++u, ++k_offset) {
					sum += img[img_offset + u] * k->kernel[k_offset];
				}
			}
			dst[y*dst_w + x] = (float)sum;
			//dst[y*dst_w + x] = (float)(sum * scale);
		}
	}

	if (rw) *rw = dst_w;
	if (rh) *rh = dst_h;
}

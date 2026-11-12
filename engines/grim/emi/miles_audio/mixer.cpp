/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "common/endian.h"
#include "common/textconsole.h"
#include "common/util.h"

#include "engines/grim/emi/miles_audio/driver.h"

namespace Grim {

void EMIMilesDriver::mixerMerge(const byte **src, uint32 *srcFract, const byte *srcEnd, int32 **dst, const int32 *dstEnd, int32 *prevL, int32 *prevR, int ratio, int scaleL, int scaleR, uint flags) {
	if (!(flags & kMixOutputStereo) || (flags & kMixBigEndian)) {
		warning("EMIMilesDriver::mixerMerge(): unsupported mix flags %u", flags);

		return;
	}

	if (!(flags & kMixInterpolate) && (flags & kMixResample)) {
		warning("EMIMilesDriver::mixerMerge(): resampling without interpolation is not supported");

		return;
	}

	if (!(*src < srcEnd && *dst < dstEnd))
		return;

	MixFormat format;

	format.is16 = (flags & kMix16Bit) != 0;
	format.srcStereo = (flags & kMixSourceStereo) != 0;
	format.scaled = (flags & kMixScaled) != 0;
	format.scaleL = scaleL;
	format.scaleR = scaleR;
	format.frameBytes = (format.is16 ? 2 : 1) * (format.srcStereo ? 2 : 1);

	if (!(flags & kMixInterpolate))
		mixCopy(src, srcEnd, dst, dstEnd, format);
	else if (ratio <= 0x10000)
		mixInterpolate(src, srcFract, srcEnd, dst, dstEnd, prevL, prevR, ratio, format);
	else
		mixAverage(src, srcFract, srcEnd, dst, dstEnd, prevL, prevR, ratio, format);
}

void EMIMilesDriver::mixCopy(const byte **src, const byte *srcEnd, int32 **dst, const int32 *dstEnd, const MixFormat &format) {
	const byte *s = *src;
	int32 *d = *dst;

	do {
		int32 left;
		int32 right;

		fetchFrame(s, format, &left, &right);
		emitFrame(d, left, right, format);

		d += 2;
		s += format.frameBytes;
	} while (d < dstEnd && s < srcEnd);

	*src = s;
	*dst = d;
}

void EMIMilesDriver::mixInterpolate(const byte **src, uint32 *srcFract, const byte *srcEnd, int32 **dst, const int32 *dstEnd, int32 *prevL, int32 *prevR, int ratio, const MixFormat &format) {
	const byte *s = *src;
	int32 *d = *dst;
	uint32 fraction = *srcFract << 16;
	int32 previousLeft = *prevL;
	int32 previousRight = *prevR;
	uint32 step = (uint32)ratio << 16;
	int whole = (ratio >> 16) * format.frameBytes;
	int wholePlusOne = ((ratio >> 16) + 1) * format.frameBytes;

	do {
		int32 left;
		int32 right;

		fetchFrame(s, format, &left, &right);

		int weight = (int)(fraction >> 16);
		int32 oldLeft = previousLeft;
		int32 outLeft;
		int32 outRight;

		previousLeft = left;
		outLeft = ((oldLeft * (0xFFFF - weight)) >> 16) + ((left * weight) >> 16);
		outRight = outLeft;

		if (format.srcStereo) {
			int32 oldRight = previousRight;

			previousRight = right;
			outRight = ((oldRight * (0xFFFF - weight)) >> 16) + ((right * weight) >> 16);
		}

		emitFrame(d, outLeft, outRight, format);

		d += 2;

		uint32 oldFraction = fraction;

		fraction += step;
		s += fraction < oldFraction ? wholePlusOne : whole;
	} while (d < dstEnd && s < srcEnd);

	*src = s;
	*dst = d;
	*srcFract = fraction >> 16;
	*prevL = previousLeft;
	*prevR = previousRight;
}

void EMIMilesDriver::mixAverage(const byte **src, uint32 *srcFract, const byte *srcEnd, int32 **dst, const int32 *dstEnd, int32 *prevL, int32 *prevR, int ratio, const MixFormat &format) {
	const byte *s = *src;
	int32 *d = *dst;
	uint32 accumulator = *srcFract;
	int32 previousLeft = *prevL;
	int32 previousRight = *prevR;
	int32 sumLeft;
	int32 sumRight;
	int count = 1;
	bool exhausted = false;

	fetchFrame(s, format, &sumLeft, &sumRight);

	while (!exhausted && d < dstEnd) {
		int64 reciprocal = count == 0 ? 65536 : 65536 / count;
		int32 averageLeft = (int32)(((int64)sumLeft * reciprocal) >> 16);
		int32 averageRight = averageLeft;

		previousLeft = averageLeft;

		if (format.srcStereo) {
			averageRight = (int32)(((int64)sumRight * reciprocal) >> 16);
			previousRight = averageRight;
		}

		emitFrame(d, averageLeft, averageRight, format);

		d += 2;
		accumulator += (uint32)ratio;
		sumLeft = 0;
		sumRight = 0;
		count = 0;

		while (accumulator >= 0x10000) {
			accumulator -= 0x10000;
			s += format.frameBytes;

			if (s >= srcEnd) {
				exhausted = true;

				break;
			}

			int32 left;
			int32 right;

			fetchFrame(s, format, &left, &right);

			sumLeft += left;

			// The disasm sums the left byte again for 8 bit stereo
			if (!format.is16 && format.srcStereo)
				sumRight += left;
			else
				sumRight += right;

			count++;
		}
	}

	*src = s;
	*dst = d;
	*srcFract = accumulator;
	*prevL = previousLeft;
	*prevR = previousRight;
}

void EMIMilesDriver::fetchFrame(const byte *src, const MixFormat &format, int32 *left, int32 *right) const {
	byte frame[4] = { 0, 0, 0, 0 };
	const byte *p = src;
	int available = (int)(_mixSourceLimit - src);

	if (available < format.frameBytes) {
		for (int i = 0; i < available; i++)
			frame[i] = src[i];

		p = frame;
	}

	if (format.is16) {
		*left = READ_LE_INT16(p);
		*right = format.srcStereo ? READ_LE_INT16(p + 2) : *left;
	} else {
		*left = ((int32)p[0] - 128) * 256;
		*right = format.srcStereo ? ((int32)p[1] - 128) * 256 : *left;
	}
}

void EMIMilesDriver::emitFrame(int32 *dst, int32 left, int32 right, const MixFormat &format) {
	if (format.scaled) {
		dst[0] += format.scaleL * left;
		dst[1] += format.scaleR * right;
	} else {
		dst[0] += left * (1 << 11);
		dst[1] += right * (1 << 11);
	}
}

void EMIMilesDriver::copyBuildBuffer(const int32 *build, int16 *out, int count) {
	for (int i = 0; i < count; i++)
		out[i] = (int16)CLIP<int32>(build[i] >> 11, -32768, 32767);
}

} // End of namespace Grim

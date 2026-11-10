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

#ifndef GRIM_EMI_BMUTIL_H
#define GRIM_EMI_BMUTIL_H

#include "common/scummsys.h"

#include "engines/grim/emi/lfile.h"

namespace Grim {

static const uint kBmMaxImages = 32;

struct BmImage {
	int width;
	int height;
	byte *data;
};

struct BmData {
	bool load(LFile *file);
	static int getSize(const BmData *bm);
	static void free(BmData *&bm);
	static int decompressCodec3(byte *dest, uint destSize, const byte *source, uint sourceSize);

	byte reserved0[128];
	int codec;
	int field132;
	int formatField136;
	int bitsPerPixel;
	int formatRest[12];
	void *extraData;
	uint numImages;
	int field200;
	int field204;
	int field208;
	int field212;
	BmImage images[kBmMaxImages];
};

} // End of namespace Grim

#endif

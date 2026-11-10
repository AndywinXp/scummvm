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

#include "common/textconsole.h"

#include "engines/grim/emi/cpcxfile.h"
#include "engines/grim/emi/ctgafile.h"
#include "engines/grim/emi/lbitmap.h"
#include "engines/grim/emi/lmemoryservices.h"
#include "engines/grim/emi/lresource.h"
#include "engines/grim/emi/zfile.h"

namespace Grim {

static LResourceType gBitmapTypePcx("PCX");
static LResourceType gBitmapTypeTga("TGA");
static LResourceType gBitmapTypePic("PIC");
static LResourceType gBitmapTypeSgi("SGI");
static LResourceType gBitmapTypePng("PNG");

LBitmap::LBitmap() {
}

void *LBitmap::operator new(size_t size) {
	return gpMemoryServices->malloc(size);
}

void LBitmap::operator delete(void *block) {
	gpMemoryServices->free(block);
}

void LBitmap::create(int width, int height, const LColorFormat &format, void *pixels) {
}

bool LBitmap::loadFromFile(LFile *file) {
	return false;
}

bool LBitmap::saveToFile(LFile *file) {
	return false;
}

LColorFormat &LBitmap::getColorFormat() {
	return colorFormat;
}

bool LBitmap::loadFile(const char *path) {
	const char *dot = strchr(path, '.');

	if (!dot)
		return false;

	LResourceType type(dot + 1);

	if (type == gBitmapTypePcx) {
		CPCXFile pcxFile;
		LFile *file = gpFileServices->open(path, "rb");

		if (!file)
			return false;

		bool result = pcxFile.readImage(file, this, 0);
		gpFileServices->close(file, true);

		return result;
	}

	if (type == gBitmapTypeTga) {
		CTGAFile tgaFile;
		ZFile *file = ZFileServices::get()->open(path, "rb");

		if (!file)
			return false;

		bool result = tgaFile.readImage(file, this, 0);
		ZFileServices::get()->closeZFile(file);

		return result;
	}

	if (type == gBitmapTypePic)
		warning("LBitmap::loadFile(): PIC images are not supported (%s)", path);
	else if (type == gBitmapTypeSgi)
		warning("LBitmap::loadFile(): SGI images are not supported (%s)", path);
	else if (type == gBitmapTypePng)
		warning("LBitmap::loadFile(): PNG images are not supported (%s)", path);
	else
		warning("LBitmap::loadFile(): unknown image type (%s)", path);

	return false;
}

} // End of namespace Grim

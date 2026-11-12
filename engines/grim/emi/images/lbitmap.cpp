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

#include "engines/grim/emi/images/cpcxfile.h"
#include "engines/grim/emi/images/ctgafile.h"
#include "engines/grim/emi/emiengine.h"
#include "engines/grim/emi/images/lbitmap.h"
#include "engines/grim/emi/core/lmemoryservices.h"
#include "engines/grim/emi/resources/lresource.h"
#include "engines/grim/emi/files/zfile.h"

namespace Grim {

static const LResourceType kBitmapTypePcx("PCX");
static const LResourceType kBitmapTypeTga("TGA");
static const LResourceType kBitmapTypePic("PIC");
static const LResourceType kBitmapTypeSgi("SGI");
static const LResourceType kBitmapTypePng("PNG");

LBitmap::LBitmap(EmiEngine *engine) : _engine(engine) {
}

void *LBitmap::operator new(size_t size) {
	return LMemoryServices::getCurrent()->mallocImpl(size);
}

void LBitmap::operator delete(void *block) {
	LMemoryServices::getCurrent()->freeImpl(block);
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

	if (type == kBitmapTypePcx) {
		CPCXFile pcxFile(_engine);
		LFile *file = _engine->getFileServices()->open(path, "rb");

		if (!file)
			return false;

		bool result = pcxFile.readImage(file, this, 0);
		_engine->getFileServices()->close(file, true);

		return result;
	}

	if (type == kBitmapTypeTga) {
		CTGAFile tgaFile(_engine);
		ZFile *file = _engine->getZFileServices()->open(path, "rb");

		if (!file)
			return false;

		bool result = tgaFile.readImage(file, this, 0);
		_engine->getZFileServices()->closeZFile(file);

		return result;
	}

	if (type == kBitmapTypePic)
		warning("LBitmap::loadFile(): PIC images are not supported (%s)", path);
	else if (type == kBitmapTypeSgi)
		warning("LBitmap::loadFile(): SGI images are not supported (%s)", path);
	else if (type == kBitmapTypePng)
		warning("LBitmap::loadFile(): PNG images are not supported (%s)", path);
	else
		warning("LBitmap::loadFile(): unknown image type (%s)", path);

	return false;
}

} // End of namespace Grim

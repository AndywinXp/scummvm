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

#include "engines/grim/emi/lbitmap.h"

namespace Grim {

LBitmap::LBitmap() {
}

void LBitmap::create(int width, int height, const LColorFormat &format, void *pixels) {
}

bool LBitmap::loadFromFile(LFile *file) {
	return false;
}

bool LBitmap::saveToFile(LFile *file) {
	return false;
}

const LColorFormat &LBitmap::getColorFormat() {
	return colorFormat;
}

bool LBitmap::loadFile(const char *path) {
	// TODO
	warning("LBitmap::loadFile(): image readers not available, cannot load %s", path);

	return false;
}

} // End of namespace Grim

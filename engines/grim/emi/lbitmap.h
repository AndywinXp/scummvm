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

#ifndef GRIM_EMI_LBITMAP_H
#define GRIM_EMI_LBITMAP_H

#include "engines/grim/emi/lcolorformat.h"
#include "engines/grim/emi/lfile.h"

namespace Grim {

class LBitmap {
public:
	LBitmap();

	virtual int getWidth() = 0;
	virtual int getHeight() = 0;
	virtual int getStride() = 0;
	virtual void *getPixels() = 0;
	virtual void create(int width, int height, const LColorFormat &format, void *pixels);
	virtual bool loadFromFile(LFile *file);
	virtual bool saveToFile(LFile *file);
	virtual LColorFormat &getColorFormat();
	virtual void setColorFormat(const LColorFormat &format);
	virtual LString &getName();
	virtual void setName(const char *newName);
	virtual bool loadFile(const char *path);

	LColorFormat colorFormat;
	LString bitmapName;

protected:
	~LBitmap();
};

} // End of namespace Grim

#endif

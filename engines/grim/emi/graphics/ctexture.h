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

#ifndef GRIM_EMI_CTEXTURE_H
#define GRIM_EMI_CTEXTURE_H

#include "common/scummsys.h"

#include "engines/grim/emi/images/labitmap.h"
#include "engines/grim/emi/graphics/lcolorformat.h"

namespace Grim {

class CTexture {
public:
	CTexture();
	virtual ~CTexture();

	void *operator new(size_t size);
	void operator delete(void *block);

	virtual void applyFlags();
	virtual void updateTexture(const LColorFormat *format, uint width, uint height, int stride, const byte *pixels);
	virtual void setup(LABitmap *bitmap);
	virtual int getSize();

	uint _flags;
};

} // End of namespace Grim

#endif

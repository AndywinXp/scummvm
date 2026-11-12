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

#ifndef GRIM_EMI_SHADOW_V2_H
#define GRIM_EMI_SHADOW_V2_H

#include "common/array.h"

#include "engines/grim/emi/graphics/color.h"
#include "engines/grim/emi/files/lfile.h"

namespace Grim {

class EmiEngine;
class WalkPlane_v2;
class WalkPlaneCollection;

class Shadow_v2 {
public:
	explicit Shadow_v2(EmiEngine *engine);
	~Shadow_v2();

	void *operator new(size_t size);
	void operator delete(void *block);

	int getSize();
	void resolveSectors(WalkPlaneCollection *collection);
	float *getPosition(float *out);
	void setName(const LString &newName);
	void setLightName(const LString &newLightName);
	void setColor(const Color &newColor);
	void addSector(const LString &sectorName);

	LString _name;
	LString _lightName;
	float _point[3];
	Common::Array<WalkPlane_v2 *> _sectors;
	Common::Array<LString> _sectorNames;
	Color _color;

private:
	EmiEngine *_engine;
};

} // End of namespace Grim

#endif

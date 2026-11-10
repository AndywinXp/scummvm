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

#ifndef GRIM_EMI_DIMREGION_H
#define GRIM_EMI_DIMREGION_H

#include "common/scummsys.h"

#include "engines/grim/emi/frect.h"

namespace Grim {

class CDrawContext;

class DimRegion {
public:
	DimRegion();

	void *operator new(size_t size);
	void operator delete(void *block);

	static int dimScreen(int reserved, float alpha);
	static int dimRegion(FRect *region, float alpha);
	static bool undimRegion(int regionId);
	static void undimAll();
	static int drawDimRegions(CDrawContext *drawContext);

	DimRegion *prev;
	DimRegion *next;
	FRect rect;
	float alpha;
	int id;

private:
	static DimRegion *_head;
	static DimRegion *_tail;
	static int _count;
	static int _nextId;
};

} // End of namespace Grim

#endif

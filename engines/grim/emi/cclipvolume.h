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

#ifndef GRIM_EMI_CCLIPVOLUME_H
#define GRIM_EMI_CCLIPVOLUME_H

#include "engines/grim/emi/bbox.h"
#include "engines/grim/emi/cplane.h"
#include "engines/grim/emi/cvector.h"

namespace Grim {

class CClipVolume {
public:
	enum {
		kMaxPlanes = 32,
		kMaxPoints = 64
	};

	enum {
		kInside = 1,
		kOutside = 2,
		kPartial = 3
	};

	CClipVolume();
	virtual ~CClipVolume();

	void reset(int newNumPlanes);
	int testBox(const BBox *box, const float *transform);
	int testSphere(const float *sphere);
	int testPoints(const float *points, int count);
	int computeOutcode(const float *point);

	int planeMask;
	int numPlanes;
	CPlane planes[kMaxPlanes];
	int planeType[kMaxPlanes];
	int numPoints;
	CVector points[kMaxPoints];
};

} // End of namespace Grim

#endif

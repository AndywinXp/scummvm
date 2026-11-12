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

#ifndef GRIM_EMI_CLIGHT_H
#define GRIM_EMI_CLIGHT_H

#include "engines/grim/emi/math/bbox.h"
#include "engines/grim/emi/graphics/color.h"

namespace Grim {

class CLight {
public:
	enum {
		kTypeOmni = 1,
		kTypeSpot = 2,
		kTypeDirect = 3,
		kTypeAmbient = 4
	};

	CLight(Color lightColor, float lightIntensity, float lightFocusDistance, float lightSpreadDistance);

	void *operator new(size_t size);
	void operator delete(void *block);

	void setColor(int newR, int newG, int newB);
	void addLighting(Color *accumulator, const float *lightTransform, const float *position, const float *normal);
	float evaluate(const float *lightTransform, const float *position, const float *normal);
	bool isOutOfRange(const float *lightTransform, const BBox *box);

	int type;
	float intensity;
	Color color;
	float focusDistance;
	float spreadDistance;
	float umbraAngle;
	float penumbraAngle;
};

} // End of namespace Grim

#endif

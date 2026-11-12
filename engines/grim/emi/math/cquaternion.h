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

#ifndef GRIM_EMI_CQUATERNION_H
#define GRIM_EMI_CQUATERNION_H

#include "engines/grim/emi/math/cvector.h"

namespace Grim {

class CQuaternion {
public:
	CQuaternion();
	CQuaternion(float newX, float newY, float newZ, float newW);
	CQuaternion(const CVector &eulerDegrees);
	CQuaternion(const CVector &axis, float angle);

	void setEuler(float eulerX, float eulerY, float eulerZ);
	CQuaternion *slerp(const CQuaternion &other, float t, CQuaternion *out) const;
	void setAxisAngle(float angle, const CVector &axis);
	void normalize();
	void invert();
	float dot(const CQuaternion &other) const;
	CQuaternion operator*(float scale) const;
	CQuaternion &operator+=(const CQuaternion &other);

	float x;
	float y;
	float z;
	float w;
};

} // End of namespace Grim

#endif

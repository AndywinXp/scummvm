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

#ifndef GRIM_EMI_CMATRIX_H
#define GRIM_EMI_CMATRIX_H

#include "engines/grim/emi/cquaternion.h"
#include "engines/grim/emi/cvector.h"

namespace Grim {

class CMatrix {
public:
	void setIdentity();
	CMatrix *setFromEuler(const CVector &eulerDegrees);
	void toEuler(CVector *eulerDegrees);
	void toQuaternion(CQuaternion *quaternion);
	CMatrix *setFromQuaternion(const CQuaternion &quaternion);
	CMatrix *transpose(CMatrix *out) const;
	CMatrix *multiply(CMatrix *out, const CMatrix *other) const;

	float m[9];
};

} // End of namespace Grim

#endif

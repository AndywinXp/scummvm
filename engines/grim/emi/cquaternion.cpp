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

#include <math.h>

#include "common/scummsys.h"

#include "engines/grim/emi/cquaternion.h"

namespace Grim {

static const float kDegreesToRadians = 3.14159274f * 0.00555555569f;
static const float kMinLength = 1e-20f;
static const double kSlerpLinearLimit = 0.000001;

CQuaternion::CQuaternion() {
	w = 0.0f;
	z = 0.0f;
	y = 0.0f;
	x = 0.0f;
}

CQuaternion::CQuaternion(float newX, float newY, float newZ, float newW) {
	x = newX;
	y = newY;
	z = newZ;
	w = newW;
}

CQuaternion::CQuaternion(const CVector &eulerDegrees) {
	setEuler(eulerDegrees.x, eulerDegrees.y, eulerDegrees.z);
}

CQuaternion::CQuaternion(const CVector &axis, float angle) {
	setAxisAngle(angle, axis);
}

void CQuaternion::setEuler(float eulerX, float eulerY, float eulerZ) {
	double halfX = (double)kDegreesToRadians * eulerX * 0.5;
	float cosX = (float)cos(halfX);
	double halfY = (double)kDegreesToRadians * eulerY * 0.5;
	float halfYStored = (float)halfY;
	float cosY = (float)cos(halfY);
	double halfZ = (double)kDegreesToRadians * eulerZ * 0.5;
	float halfZStored = (float)halfZ;
	float cosZ = (float)cos(halfZ);
	double sinX = sin(halfX);
	double sinY = sin(halfYStored);
	double sinZ = sin(halfZStored);
	double cosZY = (double)cosZ * cosY;

	w = (float)(sinZ * sinY * sinX + cosZY * cosX);
	x = (float)(cosZY * sinX - sinZ * sinY * cosX);
	y = (float)(sinZ * sinX * cosY + sinY * cosZ * cosX);
	z = (float)(sinZ * cosY * cosX - sinY * sinX * cosZ);
}

CQuaternion *CQuaternion::slerp(const CQuaternion &other, float t, CQuaternion *out) const {
	double dotProduct = (double)other.z * z + (double)other.y * y + (double)other.x * x + (double)w * other.w;
	float cosine = (float)dotProduct;
	float otherX;
	float otherY;
	float otherZ;
	float otherW;

	if (dotProduct >= 0.0) {
		otherX = other.x;
		otherY = other.y;
		otherZ = other.z;
		otherW = other.w;
	} else {
		cosine = -cosine;
		otherX = -other.x;
		otherY = -other.y;
		otherZ = -other.z;
		otherW = -other.w;
	}

	double scaleThis;
	double scaleOther;

	if (1.0 - cosine <= kSlerpLinearLimit) {
		scaleThis = 1.0 - t;
		scaleOther = t;
	} else {
		double angleWide = acos(cosine);
		float angle = (float)angleWide;
		float sine = (float)sin(angleWide);

		scaleThis = sin((1.0 - t) * angle) / sine;
		scaleOther = sin(angle * t) / sine;
	}

	out->x = (float)(scaleThis * x + scaleOther * otherX);
	out->y = (float)(scaleOther * otherY + scaleThis * y);
	out->z = (float)(scaleOther * otherZ + scaleThis * z);
	out->w = (float)(scaleThis * w + scaleOther * otherW);

	return out;
}

void CQuaternion::setAxisAngle(float angle, const CVector &axis) {
	double half = angle * 0.5;
	double sine = sin(half);

	x = (float)(sine * axis.x);
	y = (float)(sine * axis.y);
	z = (float)(sine * axis.z);
	w = (float)cos(half);

	normalize();
}

void CQuaternion::normalize() {
	double length = sqrt((double)x * x + (double)y * y + (double)z * z + (double)w * w);

	if (length >= (double)kMinLength) {
		double inverse = 1.0 / length;

		x = (float)(inverse * x);
		y = (float)(inverse * y);
		z = (float)(inverse * z);
		w = (float)(inverse * w);
	} else {
		w = 1.0f;
		z = 0.0f;
		y = 0.0f;
		x = 0.0f;
	}
}

void CQuaternion::invert() {
	double squaredLength = (double)x * x + (double)y * y + (double)z * z + (double)w * w;

	if (squaredLength == 0.0) {
		// The original could divide by zero here, let's avoid that...
		return;
	}

	double scale = 1.0 / squaredLength;

	x = (float)-(scale * x);
	y = (float)-(scale * y);
	z = (float)-(scale * z);
	w = (float)(scale * w);
}

float CQuaternion::dot(const CQuaternion &other) const {
	return (float)((double)other.w * w + (double)other.z * z + (double)other.y * y + (double)other.x * x);
}

CQuaternion CQuaternion::operator*(float scale) const {
	float scaledW = scale * w;
	float scaledZ = scale * z;
	float scaledY = scale * y;
	float scaledX = scale * x;

	return CQuaternion(scaledX, scaledY, scaledZ, scaledW);
}

CQuaternion &CQuaternion::operator+=(const CQuaternion &other) {
	x = other.x + x;
	y = other.y + y;
	z = other.z + z;
	w = other.w + w;

	return *this;
}

} // End of namespace Grim

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

#include "engines/grim/emi/math/cmatrix.h"

namespace Grim {

static const float kPi = 3.14159274f;
static const float kDegreesToRadians = 3.14159274f * 0.00555555569f;
static const float kHalfTurnDegrees = 180.0f;
static const double kGimbalLimit = 1.1920928955078125e-7;

void CMatrix::setIdentity() {
	m[0] = 1.0f;
	m[1] = 0.0f;
	m[2] = 0.0f;
	m[3] = 0.0f;
	m[4] = 1.0f;
	m[5] = 0.0f;
	m[6] = 0.0f;
	m[7] = 0.0f;
	m[8] = 1.0f;
}

CMatrix *CMatrix::setFromEuler(const CVector &eulerDegrees) {
	m[0] = 1.0f;
	m[4] = 1.0f;
	m[8] = 1.0f;
	m[1] = 0.0f;
	m[2] = 0.0f;
	m[3] = 0.0f;
	m[5] = 0.0f;
	m[6] = 0.0f;
	m[7] = 0.0f;

	double halfX = (double)kDegreesToRadians * eulerDegrees.x * 0.5;
	float cosX = (float)cos(halfX);
	double halfY = (double)kDegreesToRadians * eulerDegrees.y * 0.5;
	float halfYStored = (float)halfY;
	float cosY = (float)cos(halfY);
	double halfZ = (double)kDegreesToRadians * eulerDegrees.z * 0.5;
	float halfZStored = (float)halfZ;
	float cosZ = (float)cos(halfZ);
	double sinX = sin(halfX);
	double sinY = sin(halfYStored);
	double sinZ = sin(halfZStored);
	float cosZY = (float)((double)cosZ * cosY);
	float sinZY = (float)(sinZ * sinY);

	float quatW = (float)(sinZ * sinY * sinX + (double)cosZY * cosX);
	float quatZ = (float)(sinZ * cosY * cosX - sinY * sinX * cosZ);
	float quatY = (float)(sinZ * sinX * cosY + sinY * cosZ * cosX);
	float quatX = (float)((double)cosZY * sinX - (double)sinZY * cosX);

	CQuaternion quaternion(quatX, quatY, quatZ, quatW);

	setFromQuaternion(quaternion);

	return this;
}

void CMatrix::toEuler(CVector *eulerDegrees) {
	float sine = -m[2];

	if (sine <= 1.0f) {
		if (sine < -1.0f)
			sine = -1.0f;
	} else {
		sine = 1.0f;
	}

	double pitch = asin(sine);

	eulerDegrees->y = (float)pitch;

	if (fabs(cos(pitch)) <= kGimbalLimit) {
		eulerDegrees->x = (float)atan2(m[3], m[4]);
		eulerDegrees->z = 0.0f;
	} else {
		eulerDegrees->x = (float)atan2(m[5], m[8]);
		eulerDegrees->z = (float)atan2(m[1], m[0]);
	}

	double factor = (double)kHalfTurnDegrees / kPi;

	eulerDegrees->x = (float)(factor * eulerDegrees->x);
	eulerDegrees->y = (float)(pitch * factor);
	eulerDegrees->z = (float)(factor * eulerDegrees->z);
}

void CMatrix::toQuaternion(CQuaternion *quaternion) {
	double sumYZ = (double)m[4] + m[8];
	float sumYZStored = (float)sumYZ;
	double trace = sumYZ + m[0];

	// The original could divide by zero below, let's avoid that...
	if (trace >= 0.0) {
		double root = sqrt(trace + 1.0);

		quaternion->w = (float)(root * 0.5);

		double scale = root != 0.0 ? 0.5 / root : 0.0;

		quaternion->x = (float)(((double)m[5] - m[7]) * scale);
		quaternion->y = (float)(((double)m[6] - m[2]) * scale);
		quaternion->z = (float)(((double)m[1] - m[3]) * scale);
		quaternion->normalize();

		return;
	}

	int largest = m[4] > (double)m[0] ? 1 : 0;

	if (m[8] > (double)m[4 * largest])
		largest = 2;

	if (largest == 1) {
		double root = sqrt((double)m[4] - ((double)m[0] + m[8]) + 1.0);

		quaternion->y = (float)(root * 0.5);

		double scale = root != 0.0 ? 0.5 / root : 0.0;

		quaternion->z = (float)(((double)m[7] + m[5]) * scale);
		quaternion->x = (float)(((double)m[3] + m[1]) * scale);
		quaternion->w = (float)(((double)m[6] - m[2]) * scale);
	} else if (largest == 2) {
		double root = sqrt((double)m[8] - ((double)m[4] + m[0]) + 1.0);

		quaternion->z = (float)(root * 0.5);

		double scale = root != 0.0 ? 0.5 / root : 0.0;

		quaternion->x = (float)(((double)m[6] + m[2]) * scale);
		quaternion->y = (float)(((double)m[7] + m[5]) * scale);
		quaternion->w = (float)(((double)m[1] - m[3]) * scale);
	} else {
		double root = sqrt((double)m[0] - sumYZStored + 1.0);

		quaternion->x = (float)(root * 0.5);

		double scale = root != 0.0 ? 0.5 / root : 0.0;

		quaternion->y = (float)(((double)m[3] + m[1]) * scale);
		quaternion->z = (float)(((double)m[6] + m[2]) * scale);
		quaternion->w = (float)(((double)m[5] - m[7]) * scale);
	}

	quaternion->normalize();
}

CMatrix *CMatrix::setFromQuaternion(const CQuaternion &quaternion) {
	double twoX = (double)quaternion.x + quaternion.x;
	float twoY = (float)((double)quaternion.y + quaternion.y);
	float twoZ = (float)((double)quaternion.z + quaternion.z);
	float xx = (float)(twoX * quaternion.x);
	float xy = (float)((double)twoY * quaternion.x);
	float xz = (float)((double)twoZ * quaternion.x);
	float yy = (float)((double)twoY * quaternion.y);
	float yz = (float)((double)twoZ * quaternion.y);
	float zz = (float)((double)twoZ * quaternion.z);
	double xw = twoX * quaternion.w;
	double yw = (double)twoY * quaternion.w;
	double zw = (double)twoZ * quaternion.w;

	m[0] = (float)(1.0 - ((double)zz + yy));
	m[1] = (float)(zw + xy);
	m[2] = (float)(xz - yw);
	m[3] = (float)(xy - zw);
	m[4] = (float)(1.0 - ((double)zz + xx));
	m[5] = (float)(xw + yz);
	m[6] = (float)(yw + xz);
	m[7] = (float)(yz - xw);
	m[8] = (float)(1.0 - ((double)yy + xx));

	return this;
}

CMatrix *CMatrix::transpose(CMatrix *out) const {
	float result[9];

	result[0] = m[0];
	result[1] = m[3];
	result[2] = m[6];
	result[3] = m[1];
	result[4] = m[4];
	result[5] = m[7];
	result[6] = m[2];
	result[7] = m[5];
	result[8] = m[8];

	for (int i = 0; i < 9; i++)
		out->m[i] = result[i];

	return out;
}

CMatrix *CMatrix::multiply(CMatrix *out, const CMatrix *other) const {
	float result[9];

	result[0] = (float)((double)m[2] * other->m[6] + (double)other->m[3] * m[1] + (double)other->m[0] * m[0]);
	result[1] = (float)((double)m[2] * other->m[7] + (double)other->m[4] * m[1] + (double)other->m[1] * m[0]);
	result[2] = (float)((double)other->m[2] * m[0] + (double)m[2] * other->m[8] + (double)m[1] * other->m[5]);
	result[3] = (float)((double)other->m[3] * m[4] + (double)m[3] * other->m[0] + (double)other->m[6] * m[5]);
	result[4] = (float)((double)other->m[7] * m[5] + (double)other->m[4] * m[4] + (double)other->m[1] * m[3]);
	result[5] = (float)((double)other->m[8] * m[5] + (double)other->m[5] * m[4] + (double)other->m[2] * m[3]);
	result[6] = (float)((double)m[8] * other->m[6] + (double)m[7] * other->m[3] + (double)other->m[0] * m[6]);
	result[7] = (float)((double)m[8] * other->m[7] + (double)m[7] * other->m[4] + (double)other->m[1] * m[6]);
	result[8] = (float)((double)other->m[2] * m[6] + (double)m[8] * other->m[8] + (double)m[7] * other->m[5]);

	for (int i = 0; i < 9; i++)
		out->m[i] = result[i];

	return out;
}

} // End of namespace Grim

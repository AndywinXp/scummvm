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

#include "common/scummsys.h"

#include "engines/grim/emi/math/bbox.h"

namespace Grim {

enum {
	kFaceMaxY = 0x01,
	kFaceMinY = 0x02,
	kFaceMinX = 0x04,
	kFaceMaxX = 0x08,
	kFaceMinZ = 0x10,
	kFaceMaxZ = 0x20
};

BBox::BBox() {
	_max._z = 0.0f;
	_max._y = 0.0f;
	_max._x = 0.0f;
	_min._z = 0.0f;
	_min._y = 0.0f;
	_min._x = 0.0f;
}

void BBox::init(const float *point) {
	_min._x = point[0];
	_min._y = point[1];
	_min._z = point[2];
	_max._x = point[0];
	_max._y = point[1];
	_max._z = point[2];
}

void BBox::expand(const float *point) {
	if (point[0] >= _min._x) {
		if (point[0] > _max._x)
			_max._x = point[0];
	} else {
		_min._x = point[0];
	}

	if (point[1] >= _min._y) {
		if (point[1] > _max._y)
			_max._y = point[1];
	} else {
		_min._y = point[1];
	}

	if (point[2] >= _min._z) {
		if (point[2] > _max._z)
			_max._z = point[2];
	} else {
		_min._z = point[2];
	}
}

bool BBox::segmentCrossesFaces(const float *start, const float *end, int faceMask) const {
	float deltaX = end[0] - start[0];
	float deltaY = end[1] - start[1];
	float deltaZ = end[2] - start[2];

	// The original could divide by zero below, let's avoid that...
	if ((faceMask & (kFaceMinX | kFaceMaxX)) != 0 && deltaX != 0.0f) {
		float inverse = (float)(1.0 / deltaX);

		if ((faceMask & kFaceMinX) != 0) {
			double factor = ((double)_min._x - start[0]) * inverse;
			float crossY = (float)(factor * deltaY + start[1]);
			float crossZ = (float)(factor * deltaZ + start[2]);

			if (crossY >= _min._y && crossY <= _max._y && crossZ >= _min._z && crossZ <= _max._z)
				return true;
		}

		if ((faceMask & kFaceMaxX) != 0) {
			double factor = ((double)_max._x - start[0]) * inverse;
			float crossY = (float)(factor * deltaY + start[1]);
			float crossZ = (float)(factor * deltaZ + start[2]);

			if (crossY >= _min._y && crossY <= _max._y && crossZ >= _min._z && crossZ <= _max._z)
				return true;
		}
	}

	if ((faceMask & (kFaceMaxY | kFaceMinY)) != 0 && deltaY != 0.0f) {
		float inverse = (float)(1.0 / deltaY);

		if ((faceMask & kFaceMinY) != 0) {
			double factor = ((double)_min._y - start[1]) * inverse;
			float crossX = (float)(factor * deltaX + start[0]);
			float crossZ = (float)(factor * deltaZ + start[2]);

			if (crossX >= _min._x && crossX <= _max._x && crossZ >= _min._z && crossZ <= _max._z)
				return true;
		}

		if ((faceMask & kFaceMaxY) != 0) {
			double factor = ((double)_max._y - start[1]) * inverse;
			float crossX = (float)(factor * deltaX + start[0]);
			float crossZ = (float)(factor * deltaZ + start[2]);

			if (crossX >= _min._x && crossX <= _max._x && crossZ >= _min._z && crossZ <= _max._z)
				return true;
		}
	}

	if ((faceMask & (kFaceMinZ | kFaceMaxZ)) == 0 || deltaZ == 0.0f)
		return false;

	float inverse = (float)(1.0 / deltaZ);

	if ((faceMask & kFaceMinZ) != 0) {
		double factor = ((double)_min._z - start[2]) * inverse;
		float crossX = (float)(factor * deltaX + start[0]);
		float crossY = (float)(factor * deltaY + start[1]);

		if (crossX >= _min._x && crossX <= _max._x && crossY >= _min._y && crossY <= _max._y)
			return true;
	}

	if ((faceMask & kFaceMaxZ) != 0) {
		double factor = ((double)_max._z - start[2]) * inverse;
		float crossX = (float)(factor * deltaX + start[0]);
		float crossY = (float)(factor * deltaY + start[1]);

		if (crossX >= _min._x && crossX <= _max._x && crossY >= _min._y && crossY <= _max._y)
			return true;
	}

	return false;
}

} // End of namespace Grim

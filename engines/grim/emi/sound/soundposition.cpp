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

#include "engines/grim/emi/sound/soundposition.h"

namespace Grim {

SoundPosition::SoundPosition() {
	clear();
}

void SoundPosition::set(float x, float y, float z) {
	_pos._x = x;
	_valid = true;
	_pos._y = y;
	_pos._z = z;
}

void SoundPosition::clear() {
	_valid = false;
	_pos._x = 0.0f;
	_pos._y = 0.0f;
	_pos._z = 0.0f;
}

} // End of namespace Grim

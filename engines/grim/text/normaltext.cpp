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

#include "engines/grim/text/normaltext.h"

namespace Grim {

NormalText::NormalText() : _system(nullptr), _initialized(0), _started(0) {
}

int NormalText::initialize(SysStruct *system) {
	_system = system;

	if (!_initialized) {
		_initialized = 1;

		return 0;
	}

	return 1;
}

void NormalText::terminate() {
	_initialized = 0;
}

int NormalText::startup() {
	if (_initialized && !_started) {
		_started = 1;

		return 0;
	}

	return 1;
}

void NormalText::shutdown() {
	_started = 0;
}

} // End of namespace Grim

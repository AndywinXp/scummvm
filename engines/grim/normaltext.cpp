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

#include "engines/grim/normaltext.h"

namespace Grim {

NormalText::NormalText() : _pSystem(nullptr), _bInitialized(0), _bStarted(0) {
}

int NormalText::initialize(SysStruct *pSystem) {
	_pSystem = pSystem;

	if (!_bInitialized) {
		_bInitialized = 1;

		return 0;
	}

	return 1;
}

void NormalText::terminate() {
	_bInitialized = 0;
}

int NormalText::startup() {
	if (_bInitialized && !_bStarted) {
		_bStarted = 1;

		return 0;
	}

	return 1;
}

void NormalText::shutdown() {
	_bStarted = 0;
}

} // End of namespace Grim

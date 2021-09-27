/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include "scumm/dimuse.h"
#include "scumm/dimuse_v2/dimuse_v2.h"
#include "scumm/dimuse_v2/dimuse_v2_groups.h"

namespace Scumm {

DiMUSEGroupsHandler::DiMUSEGroupsHandler(DiMUSE_v2 *engine) {
	_engine = engine;
}

DiMUSEGroupsHandler::~DiMUSEGroupsHandler() {}

int DiMUSEGroupsHandler::init() {
	for (int i = 0; i < MAX_GROUPS; i++) {
		_effVols[i] = 127;
		_vols[i] = 127;
	}
	return 0;
}

int DiMUSEGroupsHandler::setGroupVol(int id, int volume) {
	int l;

	if (id >= MAX_GROUPS) {
		return -5;
	}

	if (volume == -1) {
		return _vols[id];
	}

	if (volume > 127)
		return -5;

	if (id) {
		_vols[id] = volume;
		_effVols[id] = (_vols[0] * (volume + 1)) / 128;
	} else {
		_effVols[0] = volume;
		_vols[0] = volume;

		for (l = 1; l < MAX_GROUPS; l++) {
			_effVols[l] = (volume * (_vols[id] + 1)) / 128;
		}
	}

	_engine->diMUSEUpdateGroupVolumes();
	return _vols[id];
}

int DiMUSEGroupsHandler::getGroupVol(int id) {
	if (id >= MAX_GROUPS) {
		return -5;
	}

	return _effVols[id];
}

} // End of namespace Scumm

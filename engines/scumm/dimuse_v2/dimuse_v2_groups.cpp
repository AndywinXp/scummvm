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

#include "scumm/dimuse_v2/dimuse_v2.h"

namespace Scumm {

int DiMUSE_v2::groups_moduleInit() {
	for (int i = 0; i < MAX_GROUPS; i++) {
		groupEffVols[i] = 127;
		groupVols[i] = 127;
	}
	return 0;
}

int DiMUSE_v2::groups_moduleDeinit() {
	return 0;
}

int DiMUSE_v2::groups_setGroupVol(int id, int volume) {
	int l;

	if (id >= MAX_GROUPS) {
		return -5;
	}
	if (volume == -1) {
		return groupVols[id];
	}
	if (volume > 127)
		return -5;

	if (id) {
		groupVols[id] = volume;
		groupEffVols[id] = (groupVols[0] * (volume + 1)) / 128;
	} else {
		groupEffVols[0] = volume;
		groupVols[0] = volume;

		for (l = 1; l < MAX_GROUPS; l++) {
			groupEffVols[l] = (volume * (groupVols[id] + 1)) / 128;
		}
	}

	debug(5, "DiMUSE_v2::groups_setGroupVol(): UNIMPLEMENTED wave_setGroupVol()");
	//wave_setGroupVol();
	return groupVols[id];
}

int DiMUSE_v2::groups_getGroupVol(int id) {
	if (id >= MAX_GROUPS) {
		return -5;
	}
	return groupEffVols[id];
}

int DiMUSE_v2::groups_moduleDebug() {
	debug(5, "iMUSE:source:groups.c:groupVols[]: \n");
	for (int i = 0; i < MAX_GROUPS; i++) {
		debug(5, "\t%d: %d\n", i, groupVols[i]);
	}
	debug(5, "\n");

	debug(5, "iMUSE:source:groups.c:groupEffVols[]");
	for (int i = 0; i < MAX_GROUPS; i++) {
		debug(5, "\t%d: %d\n", i, groupEffVols[i]);
	}
	debug(5, "\n");

	return 0;
}

} // End of namespace Scumm

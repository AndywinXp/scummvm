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

int DiMUSE_v2::fades_moduleInit() {
	for (int l = 0; l < MAX_FADES; l++) {
		fades[l].status = 0;
	}
	fadesOn = 0;
	return 0;
}

int DiMUSE_v2::fades_moduleDeinit() {
	return 0;
}

int DiMUSE_v2::fades_save(unsigned char *buffer, int sizeLeft) {
	// We're saving 640 bytes:
	// which means 10 ints (4 bytes each) for 16 times (number of fades)
	if (sizeLeft < 640)
		return -5;
	memcpy(buffer, fades, 640);
	return 640;
}

int DiMUSE_v2::fades_restore(unsigned char *buffer) {
	memcpy(fades, buffer, 640);
	fadesOn = 1;
	return 640;
}

int DiMUSE_v2::fades_fadeParam(int soundId, int opcode, int destinationValue, int fadeLength, int oneShot) {
	if (!soundId || fadeLength < 0)
		return -5;
	if (opcode != 0x500 && opcode != 0x600 && opcode != 0x700 && opcode != 0x800 && opcode != 0xF00 && opcode != 17)
		return -5;

	for (int l = 0; l < MAX_FADES; l++) {
		if (fades[l].status && (fades[l].sound == soundId && !oneShot) && (fades[l].param == opcode || opcode == -1)) {
			fades[l].status = 0;
		}
	}

	if (!fadeLength) {
		debug(5, "DiMUSE_v2::fades_fadeParam(): warning, allocated fade with zero length for sound %d", soundId);
		if (opcode != 0x600 || destinationValue) {
			// cmds_setParam
			cmds_handleCmds(12, soundId, opcode, destinationValue, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
			return 0;
		} else {
			// cmds_stopSound
			cmds_handleCmds(9, soundId, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
			return 0;
		}
	}

	for (int l = 0; l < MAX_FADES; l++) {
		if (!fades[l].status) {
			fades[l].sound = soundId;
			fades[l].param = opcode;
			// cmds_getParam (fetches current volume, with opcode 0x600)
			fades[l].currentVal = cmds_handleCmds(13, soundId, opcode, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
			fades[l].length = fadeLength;
			fades[l].counter = fadeLength;
			fades[l].slope = (destinationValue - fades[l].currentVal) / fadeLength;
			fades[l].modOvfloCounter = 0;
			fades[l].status = 1;
			fadesOn = 1;

			if ((destinationValue - fades[l].currentVal) < 0) {
				fades[l].nudge = -1;
				fades[l].slopeMod = (-(destinationValue - fades[l].currentVal) % fadeLength);
			} else {
				fades[l].nudge = 1;
				fades[l].slopeMod = (destinationValue - fades[l].currentVal) % fadeLength;
			}

			return 0;
		}
	}

	debug(5, "DiMUSE_v2::fades_fadeParam(): unable to allocate fade for sound %d", soundId);
	return -6;
}

void DiMUSE_v2::fades_clearFadeStatus(int soundId, int opcode) {
	for (int l = 0; l < MAX_FADES; l++) {
		if (fades[l].status == 0
			&& fades[l].sound == soundId
			&& (fades[l].param == opcode || opcode == -1)) {
			fades[l].status = 0;
		}
	}
}

void DiMUSE_v2::fades_loop() {
	if (!fadesOn)
		return;
	fadesOn = 0;

	for (int l = 0; l < MAX_FADES; l++) {
		if (fades[l].status) {
			fadesOn = 1;
			if (--fades[l].counter == 0) {
				fades[l].status = 0;
			}

			int currentVolume = fades[l].currentVal + fades[l].slope;
			int currentSlopeMod = fades[l].modOvfloCounter + fades[l].slopeMod;
			fades[l].modOvfloCounter += fades[l].slopeMod;

			if (fades[l].length <= currentSlopeMod) {
				fades[l].modOvfloCounter = currentSlopeMod - fades[l].length;
				currentVolume += fades[l].nudge;
			}

			if (fades[l].currentVal != currentVolume) {
				fades[l].currentVal = currentVolume;

				if ((fades[l].counter % 6) == 0) {
					debug(5, "DiMUSE_v2::fades_loop(): running fade for sound %d with id %d, currently at volume %d", fades[l].sound, l, currentVolume);
					if ((fades[l].param != 0x600) || currentVolume != 0) {
						// IMUSE_CMDS_SetParam
						cmds_handleCmds(12, fades[l].sound, fades[l].param, currentVolume, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
						continue;
					} else {
						// IMUSE_CMDS_StopSound
						cmds_handleCmds(9, fades[l].sound, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
					}
				}
			}
		}
	}
}

void DiMUSE_v2::fades_moduleFree() {
	for (int l = 0; l < MAX_FADES; l++) {
		fades[l].status = 0;
	}
	fadesOn = 0;
}

int DiMUSE_v2::fades_moduleDebug() {
	debug(5, "fadesOn: %d", fadesOn);

	for (int l = 0; l < MAX_FADES; l++) {
		debug(5, "\n");
		debug(5, "fades[%d]: \n", l);
		debug(5, "status: %d \n", fades[l].status);
		debug(5, "sound: %d \n", fades[l].sound);
		debug(5, "param: %d \n", fades[l].param);
		debug(5, "currentVal: %d \n", fades[l].currentVal);
		debug(5, "counter: %d \n", fades[l].counter);
		debug(5, "length: %d \n", fades[l].length);
		debug(5, "slope: %d \n", fades[l].slope);
		debug(5, "slopeMod: %d \n", fades[l].slopeMod);
		debug(5, "modOvfloCounter: %d \n", fades[l].modOvfloCounter);
		debug(5, "nudge: %d \n", fades[l].nudge);
	}
	return 0;
}
} // End of namespace Scumm

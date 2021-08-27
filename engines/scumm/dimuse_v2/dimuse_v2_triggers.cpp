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

int DiMUSE_v2::triggers_moduleInit() {
	for (int l = 0; l < MAX_TRIGGERS; l++) {
		trigs[l].sound = 0;
		defers[l].counter = 0;
	}
	triggers_defersOn = 0;
	triggers_midProcessing = 0;
	return 0;
}

int DiMUSE_v2::triggers_clear() {
	for (int l = 0; l < MAX_TRIGGERS; l++) {
		trigs[l].sound = 0;
		defers[l].counter = 0;
	}
	triggers_defersOn = 0;
	triggers_midProcessing = 0;
	return 0;
}

int DiMUSE_v2::triggers_save(int * buffer, int bufferSize) {
	if (bufferSize < 2848) {
		return -5;
	}
	memcpy(buffer, trigs, 2464);
	memcpy(buffer + 2464, defers, 384);
	return 2848;
}

int DiMUSE_v2::triggers_restore(int * buffer) {
	memcpy(trigs, buffer, 2464);
	memcpy(defers, buffer + 2464, 384);
	triggers_defersOn = 1;
	return 2848;
}

int DiMUSE_v2::triggers_setTrigger(int soundId, char *marker, int opcode, int d, int e, int f, int g, int h, int i, int j, int k, int l, int m, int n) {
	if (soundId == 0) {
		return -5;
	}

	if (marker == NULL) {
		marker = &triggers_empty_marker;
	}

	if (iMUSE_getMarkerLength(marker) >= 256) {
		debug(5, "ERR: attempting to set trig with oversize marker string...");
		return -5;
	}

	for (int index = 0; index < MAX_TRIGGERS; index++) {
		if (trigs[index].sound == 0) {
			trigs[index].sound = soundId;
			trigs[index].clearLater = 0;
			trigs[index].opcode = opcode;
			iMUSE_strcpy(trigs[index].text, marker);
			trigs[index].args_0_ = d;
			trigs[index].args_1_ = e;
			trigs[index].args_2_ = f;
			trigs[index].args_3_ = g;
			trigs[index].args_4_ = h;
			trigs[index].args_5_ = i;
			trigs[index].args_6_ = j;
			trigs[index].args_7_ = k;
			trigs[index].args_8_ = l;
			trigs[index].args_9_ = m;
			return 0;
		}
	}
	debug(5, "ERR: tr unable to alloc trigger...");
	return -6;
}

int DiMUSE_v2::triggers_checkTrigger(int soundId, char *marker, int opcode) {
	int r = 0;
	for (int l = 0; l < MAX_TRIGGERS; l++) {
		if (trigs[l].sound != 0) {
			if (soundId == -1 || trigs[l].sound == soundId) {
				if (marker == (char *)-1 || !iMUSE_compareTEXT(marker, trigs[l].text)) {
					if (opcode == -1 || trigs[l].opcode == opcode)
						r++;
				}
			}
		}
	}

	return r;
}

int DiMUSE_v2::triggers_clearTrigger(int soundId, char *marker, int opcode) {
	for (int l = 0; l < MAX_TRIGGERS; l++) {
		if ((trigs[l].sound != 0) && (soundId == -1 || trigs[l].sound == soundId) &&
			(marker || !iMUSE_compareTEXT(marker, trigs[l].text)) &&
			(opcode == -1 || trigs[l].opcode == opcode)) {

			if (triggers_midProcessing) {
				trigs[l].clearLater = 1;
			} else {
				trigs[l].sound = 0;
			}
		}
	}
	return 0;
}

void DiMUSE_v2::triggers_processTriggers(int soundId, char *marker) {
	char textBuffer[256];
	int r;
	if (iMUSE_getMarkerLength(marker) >= 256) {
		debug(5, "ERR: TgProcessMarker() passed oversize marker string...");
		return;
	}

	iMUSE_strcpy(triggers_textBuffer, marker);
	triggers_midProcessing++;
	for (int l = 0; l < MAX_TRIGGERS; l++) {
		if ((trigs[l].sound && trigs[l].sound == soundId) &&
			(trigs[l].text[0] == '\0' || iMUSE_compareTEXT(triggers_textBuffer, trigs[l].text))) {

			// Save the string into our local buffer for later
			r = 0;
			if (triggers_textBuffer[0] != '\0') {
				do {
					textBuffer[r] = triggers_textBuffer[r];
					r++;
				} while (triggers_textBuffer[r] != '\0');
			}
			textBuffer[r] = '\0';

			trigs[l].sound = 0;
			if (trigs[l].opcode == 0) {
				// Call the script callback (a function which sets _stoppingSequence to 1)
				script_callback(triggers_textBuffer);
			} else {
				if (trigs[l].opcode < 30) {
					// Execute a command
					cmds_handleCmds((int)trigs[l].opcode, trigs[l].args_0_,
						trigs[l].args_1_, trigs[l].args_2_,
						trigs[l].args_3_, trigs[l].args_4_,
						trigs[l].args_5_, trigs[l].args_6_,
						trigs[l].args_7_, trigs[l].args_8_,
						trigs[l].args_9_, -1, -1, -1, -1);
				}
				// This is used by DIG when speech is playing; not clear what its purpose is,
				// but its absence never appeared to disrupt anything
				/* else {
					int(*func)() = trigs[l].opcode;
					func(triggers_textBuffer, trigs[l].args_0_,
						trigs[l].args_1_, trigs[l].args_2_,
						trigs[l].args_3_, trigs[l].args_4_,
						trigs[l].args_5_, trigs[l].args_6_,
						trigs[l].args_7_, trigs[l].args_8_,
						trigs[l].args_9_);
				}*/
			}

			// Restore the global textBuffer
			r = 0;
			if (textBuffer[0] != '\0') {
				do {
					triggers_textBuffer[r] = textBuffer[r];
					r++;
				} while (textBuffer[r] != '\0');
			}
			triggers_textBuffer[r] = '\0';
		}
	}
	if (--triggers_midProcessing == 0) {
		for (int l = 0; l < MAX_TRIGGERS; l++) {
			if (trigs[l].clearLater) {
				trigs[l].sound = 0;
			}
		}
	}
}

int DiMUSE_v2::triggers_deferCommand(int count, int opcode, int c, int d, int e, int f, int g, int h, int i, int j, int k, int l, int m, int n) {
	if (!count) {
		return -5;
	}
	for (int index = 0; index < MAX_DEFERS; index++) {
		if (!defers[index].counter) {
			defers[index].counter = count;
			defers[index].opcode = opcode;
			defers[index].args_0_ = c;
			defers[index].args_1_ = d;
			defers[index].args_2_ = e;
			defers[index].args_3_ = f;
			defers[index].args_4_ = g;
			defers[index].args_5_ = h;
			defers[index].args_6_ = i;
			defers[index].args_7_ = j;
			defers[index].args_8_ = k;
			defers[index].args_9_ = l;
			triggers_defersOn = 1;
			return 0;
		}
	}
	debug(5, "ERR: tr unable to alloc deferred cmd...");
	return -6;
}

void DiMUSE_v2::triggers_loop() {
	if (!triggers_defersOn)
		return;

	triggers_defersOn = 0;
	for (int l = 0; l < MAX_DEFERS; l++) {
		if (defers[l].counter == 0)
			continue;

		triggers_defersOn = 1;
		defers[l].counter--;

		if (defers[l].counter == 1) {
			if (defers[l].opcode != 0) {
				if (defers[l].opcode < 30) {
					cmds_handleCmds((int)trigs[l].opcode,
						trigs[l].args_0_, trigs[l].args_1_,
						trigs[l].args_2_, trigs[l].args_3_,
						trigs[l].args_4_, trigs[l].args_5_,
						trigs[l].args_6_, trigs[l].args_7_,
						trigs[l].args_8_, trigs[l].args_9_, -1, -1, -1, -1);
				}
				// This is used by DIG when speech is playing; not clear what its purpose is,
				// but its absence never appeared to disrupt anything
				/*else {
					int(*func)() = trigs[l].opcode;
					func(trigs[l].text,
						trigs[l].args_0_, trigs[l].args_1_,
						trigs[l].args_2_, trigs[l].args_3_,
						trigs[l].args_4_, trigs[l].args_5_,
						trigs[l].args_6_, trigs[l].args_7_,
						trigs[l].args_8_, trigs[l].args_9_);
				}*/ 
			} else {
				script_callback(trigs[l].text);
			}
		}
	}
}

int DiMUSE_v2::triggers_countPendingSounds(int soundId) {
	int r = 0;
	for (int l = 0; l < MAX_TRIGGERS; l++) {
		if (!trigs[l].sound)
			continue;

		int opcode = trigs[l].opcode;
		if ((opcode == 8 && trigs[l].args_0_ == soundId) || (opcode == 26 && trigs[l].args_1_ == soundId))
			r++;
	}

	for (int l = 0; l < MAX_DEFERS; l++) {
		if (!defers[l].counter)
			continue;

		int opcode = defers[l].opcode;
		if ((opcode == 8 && defers[l].args_0_ == soundId) || (opcode == 26 && defers[l].args_1_ == soundId))
			r++;
	}

	return r;
}

int DiMUSE_v2::triggers_moduleFree() {
	for (int l = 0; l < MAX_TRIGGERS; l++) {
		trigs[l].sound = 0;
		defers[l].counter = 0;
	}
	triggers_defersOn = 0;
	triggers_midProcessing = 0;
	return 0;
}

} // End of namespace Scumm

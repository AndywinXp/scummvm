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
#include "scumm/dimuse_v2/dimuse_v2_triggers.h"

namespace Scumm {

DiMUSETriggersHandler::DiMUSETriggersHandler(DiMUSE_v2 *engine) {
	_engine = engine;
}

DiMUSETriggersHandler::~DiMUSETriggersHandler() {}

int DiMUSETriggersHandler::init() {
	return clearAllTriggers();
}

int DiMUSETriggersHandler::deinit() {
	return clearAllTriggers();
}

int DiMUSETriggersHandler::clearAllTriggers() {
	for (int l = 0; l < MAX_TRIGGERS; l++) {
		_trigs[l].sound = 0;
		_trigs[l].clearLater = 0;
		_defers[l].counter = 0;
	}
	_defersOn = 0;
	_midProcessing = 0;
	return 0;
}

void DiMUSETriggersHandler::saveLoad(Common::Serializer &ser) {
	for (int l = 0; l < MAX_TRIGGERS; l++) {
		ser.syncAsSint32LE(_trigs[l].sound, VER(103));
		ser.syncArray(_trigs[l].text, 256, Common::Serializer::SByte, VER(103));
		ser.syncAsSint32LE(_trigs[l].opcode, VER(103));
		ser.syncAsSint32LE(_trigs[l].args_0_, VER(103));
		ser.syncAsSint32LE(_trigs[l].args_1_, VER(103));
		ser.syncAsSint32LE(_trigs[l].args_2_, VER(103));
		ser.syncAsSint32LE(_trigs[l].args_3_, VER(103));
		ser.syncAsSint32LE(_trigs[l].args_4_, VER(103));
		ser.syncAsSint32LE(_trigs[l].args_5_, VER(103));
		ser.syncAsSint32LE(_trigs[l].args_6_, VER(103));
		ser.syncAsSint32LE(_trigs[l].args_7_, VER(103));
		ser.syncAsSint32LE(_trigs[l].args_8_, VER(103));
		ser.syncAsSint32LE(_trigs[l].args_9_, VER(103));
		ser.syncAsSint32LE(_trigs[l].clearLater, VER(103));
	}

	for (int l = 0; l < MAX_DEFERS; l++) {
		ser.syncAsSint32LE(_defers[l].counter, VER(103));
		ser.syncAsSint32LE(_defers[l].opcode, VER(103));
		ser.syncAsSint32LE(_defers[l].args_0_, VER(103));
		ser.syncAsSint32LE(_defers[l].args_1_, VER(103));
		ser.syncAsSint32LE(_defers[l].args_2_, VER(103));
		ser.syncAsSint32LE(_defers[l].args_3_, VER(103));
		ser.syncAsSint32LE(_defers[l].args_4_, VER(103));
		ser.syncAsSint32LE(_defers[l].args_5_, VER(103));
		ser.syncAsSint32LE(_defers[l].args_6_, VER(103));
		ser.syncAsSint32LE(_defers[l].args_7_, VER(103));
		ser.syncAsSint32LE(_defers[l].args_8_, VER(103));
		ser.syncAsSint32LE(_defers[l].args_9_, VER(103));
	}

	if (ser.isLoading())
		_defersOn = 1;
}

int DiMUSETriggersHandler::setTrigger(int soundId, char *marker, int opcode, int d, int e, int f, int g, int h, int i, int j, int k, int l, int m, int n) {
	if (soundId == 0) {
		return -5;
	}

	if (marker == NULL) {
		marker = &_emptyMarker;
	}

	if (strlen(marker) >= 256) {
		debug(5, "DiMUSETriggersHandler::setTrigger(): ERROR: attempting to set trigger with oversized marker string");
		return -5;
	}

	for (int index = 0; index < MAX_TRIGGERS; index++) {
		if (_trigs[index].sound == 0) {
			_trigs[index].sound = soundId;
			_trigs[index].clearLater = 0;
			_trigs[index].opcode = opcode;
			strcpy(_trigs[index].text, marker);
			_trigs[index].args_0_ = d;
			_trigs[index].args_1_ = e;
			_trigs[index].args_2_ = f;
			_trigs[index].args_3_ = g;
			_trigs[index].args_4_ = h;
			_trigs[index].args_5_ = i;
			_trigs[index].args_6_ = j;
			_trigs[index].args_7_ = k;
			_trigs[index].args_8_ = l;
			_trigs[index].args_9_ = m;
			return 0;
		}
	}
	debug(5, "DiMUSETriggersHandler::setTrigger(): ERROR: unable to allocate trigger \"%s\" for sound %d, every slot is full", marker, soundId);
	return -6;
}

int DiMUSETriggersHandler::checkTrigger(int soundId, char *marker, int opcode) {
	int r = 0;
	for (int l = 0; l < MAX_TRIGGERS; l++) {
		if (_trigs[l].sound != 0) {
			if (soundId == -1 || _trigs[l].sound == soundId) {
				if (marker == (char *)-1 || !strcmp(marker, _trigs[l].text)) {
					if (opcode == -1 || _trigs[l].opcode == opcode)
						r++;
				}
			}
		}
	}

	return r;
}

int DiMUSETriggersHandler::clearTrigger(int soundId, char *marker, int opcode) {
	for (int l = 0; l < MAX_TRIGGERS; l++) {
		if ((_trigs[l].sound != 0) && (soundId == -1 || _trigs[l].sound == soundId) &&
			(!strcmp(marker, (char *)"") || !strcmp(marker, _trigs[l].text)) &&
			(opcode == -1 || _trigs[l].opcode == opcode)) {

			if (_midProcessing) {
				_trigs[l].clearLater = 1;
			} else {
				_trigs[l].sound = 0;
			}
		}
	}
	return 0;
}

void DiMUSETriggersHandler::processTriggers(int soundId, char *marker) {
	char textBuffer[256];
	int r;
	if (strlen(marker) >= 256) {
		debug(5, "DiMUSETriggersHandler::processTriggers(): ERROR: the input marker string is oversized");
		return;
	}

	strcpy(_textBuffer, marker);
	_midProcessing++;
	for (int l = 0; l < MAX_TRIGGERS; l++) {
		if (!_trigs[l].sound ||
			_trigs[l].sound != soundId ||
			_trigs[l].text[0] && strcmp(_textBuffer, _trigs[l].text)) {
			continue;
		}

		// Save the string into our local buffer for later
		r = 0;
		if (_textBuffer[0] != '\0') {
			do {
				textBuffer[r] = _textBuffer[r];
				r++;
			} while (_textBuffer[r] != '\0');
		}
		textBuffer[r] = '\0';

		_trigs[l].sound = 0;
		if (_trigs[l].opcode == 0) {
			// Call the script callback (a function which sets _stoppingSequence to 1)
			_engine->scriptTriggerCallback(_textBuffer);
		} else {
			if (_trigs[l].opcode < 30) {
				// Execute a command
				_engine->cmdsHandleCmd(_trigs[l].opcode, _trigs[l].args_0_,
					_trigs[l].args_1_, _trigs[l].args_2_,
					_trigs[l].args_3_, _trigs[l].args_4_,
					_trigs[l].args_5_, _trigs[l].args_6_,
					_trigs[l].args_7_, _trigs[l].args_8_,
					_trigs[l].args_9_, -1, -1, -1, -1);
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
				_textBuffer[r] = textBuffer[r];
				r++;
			} while (textBuffer[r] != '\0');
		}
		_textBuffer[r] = '\0';
	}
	if (--_midProcessing == 0) {
		for (int l = 0; l < MAX_TRIGGERS; l++) {
			if (_trigs[l].clearLater) {
				_trigs[l].sound = 0;
			}
		}
	}
}

int DiMUSETriggersHandler::deferCommand(int count, int opcode, int c, int d, int e, int f, int g, int h, int i, int j, int k, int l, int m, int n) {
	if (!count) {
		return -5;
	}
	for (int index = 0; index < MAX_DEFERS; index++) {
		if (!_defers[index].counter) {
			_defers[index].counter = count;
			_defers[index].opcode = opcode;
			_defers[index].args_0_ = c;
			_defers[index].args_1_ = d;
			_defers[index].args_2_ = e;
			_defers[index].args_3_ = f;
			_defers[index].args_4_ = g;
			_defers[index].args_5_ = h;
			_defers[index].args_6_ = i;
			_defers[index].args_7_ = j;
			_defers[index].args_8_ = k;
			_defers[index].args_9_ = l;
			_defersOn = 1;
			return 0;
		}
	}
	debug(5, "DiMUSETriggersHandler::deferCommand(): ERROR: couldn't allocate deferred command");
	return -6;
}

void DiMUSETriggersHandler::loop() {
	if (!_defersOn)
		return;

	_defersOn = 0;
	for (int l = 0; l < MAX_DEFERS; l++) {
		if (_defers[l].counter == 0)
			continue;

		_defersOn = 1;
		_defers[l].counter--;

		if (_defers[l].counter == 1) {
			if (_defers[l].opcode != 0) {
				if (_defers[l].opcode < 30) {
					_engine->cmdsHandleCmd(_trigs[l].opcode,
						_trigs[l].args_0_, _trigs[l].args_1_,
						_trigs[l].args_2_, _trigs[l].args_3_,
						_trigs[l].args_4_, _trigs[l].args_5_,
						_trigs[l].args_6_, _trigs[l].args_7_,
						_trigs[l].args_8_, _trigs[l].args_9_, -1, -1, -1, -1);
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
				_engine->scriptTriggerCallback(_trigs[l].text);
			}
		}
	}
}

int DiMUSETriggersHandler::countPendingSounds(int soundId) {
	int r = 0;
	for (int l = 0; l < MAX_TRIGGERS; l++) {
		if (!_trigs[l].sound)
			continue;

		int opcode = _trigs[l].opcode;
		if ((opcode == 8 && _trigs[l].args_0_ == soundId) || (opcode == 26 && _trigs[l].args_1_ == soundId))
			r++;
	}

	for (int l = 0; l < MAX_DEFERS; l++) {
		if (!_defers[l].counter)
			continue;

		int opcode = _defers[l].opcode;
		if ((opcode == 8 && _defers[l].args_0_ == soundId) || (opcode == 26 && _defers[l].args_1_ == soundId))
			r++;
	}

	return r;
}



} // End of namespace Scumm

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
#define DIG_STATE_OFFSET 11
#define DIG_SEQ_OFFSET (DIG_STATE_OFFSET + 65)
#define COMI_STATE_OFFSET 3

int DiMUSE_v2::script_parse(int cmd, int a0, int param1, int param2, int param3, int param4, int param5, int param6, int param7) {
	if (_scriptInitializedFlag || !cmd) {
		switch (cmd) {
		case 0:
			if (_scriptInitializedFlag) {
				debug(5, "DiMUSE_v2::script_parse(): script module already initialized");
				return -1;
			} else {
				_scriptInitializedFlag = 1;
				return script_init();
			}
		case 1:
			_scriptInitializedFlag = 0;
			return script_terminate();
		case 2:
			//result = script_save((_DWORD *)a0, param1);
			break;
		case 3:
			//result = script_restore((int *)a0);
			break;
		case 4:
			script_refresh();
			return 0;
		case 5:
			script_setState(a0);
			return 0;
		case 6:
			script_setSequence(a0);
			return 0;
		case 7:
			return script_setCuePoint();
		case 8:
			return script_setAttribute(a0, param1);
		default:
			debug(5, "DiMUSE_v2::script_parse(): unrecognized opcode (%d)", cmd);
			return -1;
		}
	} else {
		debug(5, "DiMUSE_v2::script_parse(): script module not initialized");
		return -1;
	}

	return -1;
}

int DiMUSE_v2::script_init() {
	_curMusicState = 0;
	_curMusicSeq = 0;
	_nextSeqToPlay = 0;
	memset(_attributes, 0, sizeof(_attributes));
	return 0;
}

int DiMUSE_v2::script_terminate() {
	DiMUSE_terminate();

	_curMusicState = 0;
	_curMusicSeq = 0;
	_nextSeqToPlay = 0;
	memset(_attributes, 0, sizeof(_attributes));
	return 0;
}

int DiMUSE_v2::script_save() { return 0; }
int DiMUSE_v2::script_restore() { return 0; }

void DiMUSE_v2::script_refresh() {
	int soundId;
	int nextSound;

	// Prevent new music from starting, and fade out the current one
	if (_vm->isSmushActive()) {
		//fadeOutMusic(60);
		return;
	}

	if (_stopSequenceFlag) {
		script_setSequence(0);
		_stopSequenceFlag = 0;
	}

	soundId = 0;

	while (1) {
		nextSound = DiMUSE_getNextSound(soundId);
		soundId = nextSound;

		if (!nextSound)
			break;

		if (DiMUSE_getParam(nextSound, 0x1800) && DiMUSE_getParam(soundId, 0x1900) == IMUSE_BUFFER_MUSIC) {
			if (soundId)
				return;
			break;
		}
	}

	if (_curMusicSeq)
		script_setSequence(0);

	flushTracks();
	return;
}

void DiMUSE_v2::script_setState(int soundId) {
	if (_vm->_game.id == GID_DIG) {
		setDigMusicState(soundId);
	} else if (_vm->_game.id == GID_CMI) {
		setComiMusicState(soundId);
	}
}

void DiMUSE_v2::script_setSequence(int soundId) {
	if (_vm->_game.id == GID_DIG) {
		setDigMusicSequence(soundId);
	} else if (_vm->_game.id == GID_CMI) {
		setComiMusicSequence(soundId);
	}
}

int DiMUSE_v2::script_setCuePoint() { return 0; }

int DiMUSE_v2::script_setAttribute(int attrIndex, int attrVal) {
	debug(5, "DiMUSE_v2::script_setAttribute(): unimplemented");
	return 0;
}

int DiMUSE_v2::script_callback(char *marker) {
	if (marker[0] != '_') {
		debug(5, "DiMUSE_v2::script_callback(): got marker != '_end', callback ignored");
		return -1;
	}	
	_stopSequenceFlag = 1;
	return 0;
}

void DiMUSE_v2::setDigMusicState(int stateId) {
	int l, num = -1;

	for (l = 0; _digStateMusicTable[l].soundId != -1; l++) {
		if ((_digStateMusicTable[l].soundId == stateId)) {
			debug(5, "Set music state: %s, %s", _digStateMusicTable[l].name, _digStateMusicTable[l].filename);
			num = l;
			break;
		}
	}

	if (num == -1) {
		for (l = 0; _digStateMusicMap[l].roomId != -1; l++) {
			if ((_digStateMusicMap[l].roomId == stateId)) {
				break;
			}
		}
		num = l;

		int offset = _attributes[_digStateMusicMap[num].offset];
		if (offset == 0) {
			if (_attributes[_digStateMusicMap[num].attribPos] != 0) {
				num = _digStateMusicMap[num].stateIndex3;
			} else {
				num = _digStateMusicMap[num].stateIndex1;
			}
		} else {
			int stateIndex2 = _digStateMusicMap[num].stateIndex2;
			if (stateIndex2 == 0) {
				num = _digStateMusicMap[num].stateIndex1 + offset;
			} else {
				num = stateIndex2;
			}
		}
	}

	debug(5, "Set music state: %s, %s", _digStateMusicTable[num].name, _digStateMusicTable[num].filename);

	if (_curMusicState == num)
		return;

	if (_curMusicSeq == 0) {
		if (num == 0)
			playDigMusic(NULL, &_digStateMusicTable[0], num, false);
		else
			playDigMusic(_digStateMusicTable[num].name, &_digStateMusicTable[num], num, false);
	}

	_curMusicState = num;
}

void DiMUSE_v2::setDigMusicSequence(int seqId) {
	int l, num = -1;

	if (seqId == 0)
		seqId = 2000;

	for (l = 0; _digSeqMusicTable[l].soundId != -1; l++) {
		if ((_digSeqMusicTable[l].soundId == seqId)) {
			debug(5, "Set music sequence: %s, %s", _digSeqMusicTable[l].name, _digSeqMusicTable[l].filename);
			num = l;
			break;
		}
	}

	if (num == -1)
		return;

	if (_curMusicSeq == num)
		return;

	if (num != 0) {
		if (_curMusicSeq && ((_digSeqMusicTable[_curMusicSeq].transitionType == 4)
			|| (_digSeqMusicTable[_curMusicSeq].transitionType == 6))) {
			_nextSeqToPlay = num;
			return;
		} else {
			playDigMusic(_digSeqMusicTable[num].name, &_digSeqMusicTable[num], 0, true);
			_nextSeqToPlay = 0;
			_attributes[DIG_SEQ_OFFSET + num] = 1; // _attributes[COMI_SEQ_OFFSET] in Comi are not used as it doesn't have 'room' attributes table
		}
	} else {
		if (_nextSeqToPlay != 0) {
			playDigMusic(_digSeqMusicTable[_nextSeqToPlay].name, &_digSeqMusicTable[_nextSeqToPlay], 0, true);
			_attributes[DIG_SEQ_OFFSET + _nextSeqToPlay] = 1; // _attributes[COMI_SEQ_OFFSET] in Comi are not used as it doesn't have 'room' attributes table
			num = _nextSeqToPlay;
			_nextSeqToPlay = 0;
		} else {
			if (_curMusicState != 0) {
				playDigMusic(_digStateMusicTable[_curMusicState].name, &_digStateMusicTable[_curMusicState], _curMusicState, true);
			} else {
				playDigMusic(NULL, &_digStateMusicTable[0], _curMusicState, true);
			}

			num = 0;
		}
	}

	_curMusicSeq = num;
}

void DiMUSE_v2::setComiMusicState(int stateId) {
	int l, num = -1;

	/*
	if (stateId == 4) // look into #3604 bug, ignore stateId == 4 it's seems needed after all
		return;
	*/
	if (stateId == 0)
		stateId = 1000;

	for (l = 0; _comiStateMusicTable[l].soundId != -1; l++) {
		if ((_comiStateMusicTable[l].soundId == stateId)) {
			debug(5, "Set music state: %s, %s", _comiStateMusicTable[l].name, _comiStateMusicTable[l].filename);
			num = l;
			break;
		}
	}

	if (num == -1)
		return;

	if (_curMusicState == num)
		return;
	
	if (_curMusicSeq == 0) {
		if (num == 0)
			playComiMusic(NULL, &_comiStateMusicTable[0], num, false);
		else
			playComiMusic(_comiStateMusicTable[num].name, &_comiStateMusicTable[num], num, false);
	}

	_curMusicState = num;
}

void DiMUSE_v2::setComiMusicSequence(int seqId) {
	int l, num = -1;

	if (seqId == 0)
		seqId = 2000;

	for (l = 0; _comiSeqMusicTable[l].soundId != -1; l++) {
		if ((_comiSeqMusicTable[l].soundId == seqId)) {
			debug(5, "Set music sequence: %s, %s", _comiSeqMusicTable[l].name, _comiSeqMusicTable[l].filename);
			num = l;
			break;
		}
	}

	if (num == -1)
		return;

	if (_curMusicSeq == num)
		return;

	if (num != 0) {
		if (_curMusicSeq && ((_comiSeqMusicTable[_curMusicSeq].transitionType == 4)
			|| (_comiSeqMusicTable[_curMusicSeq].transitionType == 6))) {
			_nextSeqToPlay = num;
			return;
		} else {
			playComiMusic(_comiSeqMusicTable[num].name, &_comiSeqMusicTable[num], 0, true);
			_nextSeqToPlay = 0;
		}
	} else {
		if (_nextSeqToPlay != 0) {
			playComiMusic(_comiSeqMusicTable[_nextSeqToPlay].name, &_comiSeqMusicTable[_nextSeqToPlay], 0, true);
			num = _nextSeqToPlay;
			_nextSeqToPlay = 0;
		} else {
			if (_curMusicState != 0) {
				playComiMusic(_comiStateMusicTable[_curMusicState].name, &_comiStateMusicTable[_curMusicState], _curMusicState, true);
			} else {
				playComiMusic(NULL, &_comiStateMusicTable[0], _curMusicState, true);
			}
			num = 0;
		}
	}

	_curMusicSeq = num;
}


void DiMUSE_v2::playDigMusic(const char *songName, const imuseDigTable *table, int attribPos, bool sequence) {
	int hookId = 0;

	if (songName != NULL) {
		if ((_attributes[DIG_SEQ_OFFSET + 38]) && (!_attributes[DIG_SEQ_OFFSET + 41])) {
			if ((attribPos == 43) || (attribPos == 44))
				hookId = 3;
		}

		if ((_attributes[DIG_SEQ_OFFSET + 46] != 0) && (_attributes[DIG_SEQ_OFFSET + 48] == 0)) {
			if ((attribPos == 38) || (attribPos == 39))
				hookId = 3;
		}

		if ((_attributes[DIG_SEQ_OFFSET + 53] != 0)) {
			if ((attribPos == 50) || (attribPos == 51))
				hookId = 3;
		}

		if ((attribPos != 0) && (hookId == 0)) {
			if (table->attribPos != 0)
				attribPos = table->attribPos;
			hookId = _attributes[DIG_STATE_OFFSET + attribPos];
			if (table->hookId != 0) {
				if ((hookId != 0) && (table->hookId > 1)) {
					_attributes[DIG_STATE_OFFSET + attribPos] = 2;
				} else {
					_attributes[DIG_STATE_OFFSET + attribPos] = hookId + 1;
					if (table->hookId < hookId + 1)
						_attributes[DIG_STATE_OFFSET + attribPos] = 1;
				}
			}
		}
	}

	int nextSoundId = 0;
	while (1) {
		nextSoundId = DiMUSE_getNextSound(nextSoundId);
		if (!nextSoundId)
			break;

		// If a sound is found (and its stream is active), fade it out if it's a music track
		if (DiMUSE_getParam(nextSoundId, 0x400) == IMUSE_GROUP_MUSICEFF && !DiMUSE_getParam(nextSoundId, 0x1800))
			DiMUSE_fadeParam(nextSoundId, 0x600, 0, 120);
	}

	int oldSoundId = 0;
	nextSoundId = 0;
	while (1) {
		nextSoundId = DiMUSE_getNextSound(nextSoundId);
		if (!nextSoundId)
			break;

		if (DiMUSE_getParam(nextSoundId, 0x1800) && (DiMUSE_getParam(nextSoundId, 0x1900) == IMUSE_BUFFER_MUSIC)) {
			oldSoundId = nextSoundId; // Could _digStateMusicTable[_curMusicState].soundId be enough as oldSoundId?
			break;
		}
	}

	if (!songName) {
		if (oldSoundId)
			DiMUSE_fadeParam(oldSoundId, 0x600, 0, 120);
		return;
	}

	switch (table->transitionType) {
	case 0:
		debug(5, "DiMUSE_v2::playDigMusic(): NULL transition, ignored");
		break;
	case 1:
		files_openSound(table->soundId);
		if (table->soundId) {
			if (DiMUSE_startSound(table->soundId, 126))
				debug(5, "DiMUSE_v2::playDigMusic(): transition 1, failed to start the sound (%d)", table->soundId);
			DiMUSE_setParam(table->soundId, 0x600, 1);
			DiMUSE_fadeParam(table->soundId, 0x600, 127, 120);
			files_closeSound(table->soundId);
			DiMUSE_setParam(table->soundId, 0x400, IMUSE_GROUP_MUSICEFF);
		} else {
			debug(5, "DiMUSE_v2::playDigMusic(): transition 1, empty soundId, ignored");
		}

		break;
	case 2:
	case 3:
	case 4:
		files_openSound(table->soundId);
		if (table->filename[0] == 0 || table->soundId == 0) {
			if (oldSoundId)
				DiMUSE_fadeParam(oldSoundId, 0x600, 0, 60);
			return;
		}

		if (table->transitionType == 4) {
			_stopSequenceFlag = 0;
			DiMUSE_setTrigger(table->soundId, '_end', 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
		}

		if (oldSoundId) {
			if (table->transitionType == 2) {
				DiMUSE_switchStream(oldSoundId, table->soundId, 1800, 0, 0);
				DiMUSE_setParam(table->soundId, 0x600, 127);
				DiMUSE_setParam(table->soundId, 0x400, IMUSE_GROUP_MUSICEFF);
				DiMUSE_setHook(table->soundId, hookId);
				DiMUSE_processStreams();
				DiMUSE_setParam(table->soundId, 0x400, IMUSE_GROUP_MUSICEFF); // Repeated intentionally
				return;
			}

			if (oldSoundId != table->soundId) {
				if ((!sequence) && (table->attribPos != 0) &&
					(table->attribPos == _digStateMusicTable[_curMusicState].attribPos)) {
					DiMUSE_switchStream(oldSoundId, table->soundId, 1800, 0, 1);
					DiMUSE_setParam(table->soundId, 0x600, 127);
					DiMUSE_setParam(table->soundId, 0x400, IMUSE_GROUP_MUSICEFF);
					DiMUSE_processStreams();
				} else {
					DiMUSE_switchStream(oldSoundId, table->soundId, 1800, 0, 0);
					DiMUSE_setParam(table->soundId, 0x600, 127);
					DiMUSE_setParam(table->soundId, 0x400, IMUSE_GROUP_MUSICEFF);
					DiMUSE_setHook(table->soundId, hookId);
					DiMUSE_processStreams();
					files_closeSound(table->soundId);
					DiMUSE_setParam(table->soundId, 0x400, IMUSE_GROUP_MUSICEFF); // Repeated intentionally
				}
			}
		} else {
			if (DiMUSE_startStream(table->soundId, 126, IMUSE_BUFFER_MUSIC))
				debug(5, "DiMUSE_v2::playDigMusic(): failed to start the stream for sound %d", table->soundId);
			DiMUSE_setParam(table->soundId, 0x600, 127);
			DiMUSE_setParam(table->soundId, 0x400, IMUSE_GROUP_MUSICEFF);
			DiMUSE_setHook(table->soundId, hookId);
		}
		files_closeSound(table->soundId);
		DiMUSE_setParam(table->soundId, 0x400, IMUSE_GROUP_MUSICEFF); // Repeated intentionally
		break;
	case 5:
		debug(5, "DiMUSE_v2::playDigMusic(): no-op transition type (5), ignored");
		break;
	case 6:
		_stopSequenceFlag = 0;
		DiMUSE_setTrigger(12345680, (int)'_end', 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
		break;
	case 7:
		if (oldSoundId)
			DiMUSE_fadeParam(oldSoundId, 0x600, 0, 60);
		break;
	default:
		debug(5, "DiMUSE_v2::playDigMusic(): bogus transition type, ignored");
		break;
	}
}

void DiMUSE_v2::playComiMusic(const char *songName, const imuseComiTable *table, int attribPos, bool sequence) {
	int hookId = 0;
	int fadeDelay = 0;

	if ((songName != NULL) && (attribPos != 0)) {
		if (table->attribPos != 0)
			attribPos = table->attribPos;
		hookId = _attributes[COMI_STATE_OFFSET + attribPos];

		if (table->hookId != 0) {
			if ((hookId != 0) && (table->hookId > 1)) {
				_attributes[COMI_STATE_OFFSET + attribPos] = 2;
			} else {
				_attributes[COMI_STATE_OFFSET + attribPos] = hookId + 1;
				if (table->hookId < hookId + 1)
					_attributes[COMI_STATE_OFFSET + attribPos] = 1;
			}
		}
	}

	int nextSoundId = 0;
	while (1) {
		nextSoundId = DiMUSE_getNextSound(nextSoundId);
		if (!nextSoundId)
			break;

		// If a sound is found (and its stream is active), fade it out if it's a music track
		if (DiMUSE_getParam(nextSoundId, 0x400) == IMUSE_GROUP_MUSICEFF && !DiMUSE_getParam(nextSoundId, 0x1800))
			DiMUSE_fadeParam(nextSoundId, 0x600, 0, 120);
	}

	int oldSoundId = 0;
	nextSoundId = 0;
	while (1) {
		nextSoundId = DiMUSE_getNextSound(nextSoundId);
		if (!nextSoundId)
			break;

		if (DiMUSE_getParam(nextSoundId, 0x1800) && (DiMUSE_getParam(nextSoundId, 0x1900) == IMUSE_BUFFER_MUSIC)) {
			oldSoundId = nextSoundId; // Could _comiStateMusicTable[_curMusicState].soundId be enough as oldSoundId?
			break;
		}
	}
	
	if (!songName) {
		if (oldSoundId)
			DiMUSE_fadeParam(oldSoundId, 0x600, 0, 120);
		return;
	}

	switch (table->transitionType) {
	case 0:
		debug(5, "DiMUSE_v2::playComiMusic(): NULL transition, ignored");
		break;
	case 1:
		files_openSound(table->soundId);
		if (table->soundId) {
			if (DiMUSE_startSound(table->soundId, 126))
				debug(5, "DiMUSE_v2::playComiMusic(): transition 1, failed to start the sound (%d)", table->soundId);
			DiMUSE_setParam(table->soundId, 0x600, 1);
			DiMUSE_fadeParam(table->soundId, 0x600, 127, 120);
			files_closeSound(table->soundId);
			DiMUSE_setParam(table->soundId, 0x400, IMUSE_GROUP_MUSICEFF);
		} else {
			debug(5, "DiMUSE_v2::playComiMusic(): transition 1, empty soundId, ignored");
		}
		break;
	case 2:
	case 3:
	case 4:
	case 10:
	case 11:
	case 12:
		files_openSound(table->soundId);
		if (table->filename[0] == 0 || table->soundId == 0) {
			if (oldSoundId)
				DiMUSE_fadeParam(oldSoundId, 0x600, 0, 60);
			break;
		}

		if (table->transitionType == 4) {
			_stopSequenceFlag = 0;
			DiMUSE_setTrigger(table->soundId, (int)'_end', 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
		}

		if (oldSoundId) {
			fadeDelay = table->fadeOutDelay;
			if (!fadeDelay)
				fadeDelay = 1000;
			else
				fadeDelay = (fadeDelay * 100) / 6; // Set dimuse_table fade out time to millisecond scale

			if (table->transitionType == 2) {
				DiMUSE_switchStream(oldSoundId, table->soundId, fadeDelay, 0, 0);
				DiMUSE_setParam(table->soundId, 0x600, 127);
				DiMUSE_setParam(table->soundId, 0x400, IMUSE_GROUP_MUSICEFF);
				DiMUSE_setHook(table->soundId, table->hookId);
				DiMUSE_processStreams();
			} else if (oldSoundId != table->soundId) {
				if ((!sequence) && (table->attribPos != 0) &&
					(table->attribPos == _comiStateMusicTable[_curMusicState].attribPos)) {

					debug(5, "DiMUSE_v2::playComiMusic(): Starting new sound (%s) with same attribute as old sound (%s)",
						table->name, _comiStateMusicTable[_curMusicState].name);

					DiMUSE_switchStream(oldSoundId, table->soundId, fadeDelay, 0, 1);
					DiMUSE_setParam(table->soundId, 0x600, 127);
					DiMUSE_setParam(table->soundId, 0x400, IMUSE_GROUP_MUSICEFF);
					DiMUSE_processStreams();
				} else {
					switch (table->transitionType) {
					case 10:
						DiMUSE_setTrigger(oldSoundId, 'exit', 26, oldSoundId, table->soundId, fadeDelay, 1, 0, -1, -1, -1, -1, -1, -1);
						DiMUSE_setTrigger(oldSoundId, 'exit', 12, table->soundId, 0x600, 127, - 1, -1, -1, -1, -1, -1, -1, -1);
						DiMUSE_setTrigger(oldSoundId, 'exit', 12, table->soundId, 0x400, 4, - 1, -1, -1, -1, -1, -1, -1, -1);
						DiMUSE_setTrigger(oldSoundId, 'exit', 15, table->soundId, hookId, -1, -1, -1, -1, -1, -1, -1, -1, -1);
						DiMUSE_processStreams();
						break;
					case 11: // Markers actually are 'exit2', which is too long for a char constant; this transition is unused anyway
						DiMUSE_setTrigger(oldSoundId, 'exi2', 26, oldSoundId, table->soundId, fadeDelay, 1, 0, -1, -1, -1, -1, -1, -1);
						DiMUSE_setTrigger(oldSoundId, 'exi2', 12, table->soundId, 0x600, 127, -1, -1, -1, -1, -1, -1, -1, -1);
						DiMUSE_setTrigger(oldSoundId, 'exi2', 12, table->soundId, 0x400, 4, -1, -1, -1, -1, -1, -1, -1, -1);
						DiMUSE_setTrigger(oldSoundId, 'exi2', 15, table->soundId, hookId,- 1, -1, -1, -1, -1, -1, -1, -1, -1);
						DiMUSE_processStreams();
						break;
					case 12:
						DiMUSE_setHook(oldSoundId, table->hookId);
						DiMUSE_setTrigger(oldSoundId, 'exit', 26, oldSoundId, table->soundId, fadeDelay, 1, 0, -1, -1, -1, -1, -1, -1);
						DiMUSE_setTrigger(oldSoundId, 'exit', 12, table->soundId, 0x600, 127, -1, -1, -1, -1, -1, -1, -1, -1);
						DiMUSE_setTrigger(oldSoundId, 'exit', 12, table->soundId, 0x400, 4, -1, -1, -1, -1, -1, -1, -1, -1);
						DiMUSE_setTrigger(oldSoundId, 'exit', 15, table->soundId, hookId, -1, -1, -1, -1, -1, -1, -1, -1, -1);
						DiMUSE_processStreams();
						break;
					default:
						DiMUSE_switchStream(oldSoundId, table->soundId, fadeDelay, 0, 0);
						DiMUSE_setParam(table->soundId, 0x600, 127);
						DiMUSE_setParam(table->soundId, 0x400, 4);
						DiMUSE_setHook(table->soundId, hookId);
						DiMUSE_processStreams();
						break;
					}
				}
			}
		} else {
			if (DiMUSE_startStream(table->soundId, 126, IMUSE_BUFFER_MUSIC))
				debug(5, "DiMUSE_v2::playComiMusic(): failed to start the stream for sound %d", table->soundId);
			DiMUSE_setParam(table->soundId, 0x600, 127);
			DiMUSE_setParam(table->soundId, 0x400, IMUSE_GROUP_MUSICEFF);
			DiMUSE_setHook(table->soundId, hookId);
		}
		files_closeSound(table->soundId);
		DiMUSE_setParam(table->soundId, 0x400, IMUSE_GROUP_MUSICEFF);
		break;
	case 5:
		debug(5, "DiMUSE_v2::playComiMusic(): no-op transition type (5), ignored");
		break;
	case 6:
		_stopSequenceFlag = 0;
		DiMUSE_setTrigger(12345680, '_end', 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
		break;
	case 7:
		if (oldSoundId)
			DiMUSE_fadeParam(oldSoundId, 0x600, 0, 60);
		break;
	case 8:
		if (oldSoundId)
			DiMUSE_setHook(oldSoundId, table->hookId);
		break;
	case 9:
		if (oldSoundId)
			DiMUSE_setHook(oldSoundId, table->hookId);
		_stopSequenceFlag = 0;
		DiMUSE_setTrigger(oldSoundId, '_end', 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
		break;
	default:
		debug(5, "DiMUSE_v2::playComiMusic(): bogus transition type, ignored");
		break;
	}
}

} // End of namespace Scumm

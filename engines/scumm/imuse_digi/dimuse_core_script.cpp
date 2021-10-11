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

#include "scumm/imuse_digi/dimuse_core.h"

namespace Scumm {
#define DIG_STATE_OFFSET 11
#define DIG_SEQ_OFFSET (DIG_STATE_OFFSET + 65)
#define COMI_STATE_OFFSET 3

int IMuseDigital::scriptParse(int cmd, int a, int b) {
	if (_scriptInitializedFlag || !cmd) {
		switch (cmd) {
		case 0:
			if (_scriptInitializedFlag) {
				debug(5, "IMuseDigital::scriptParse(): script module already initialized");
				return -1;
			} else {
				_scriptInitializedFlag = 1;
				return scriptInit();
			}
		case 1:
			_scriptInitializedFlag = 0;
			return scriptTerminate();
		case 2: // script_save(a, b);
		case 3: // script_restore(a);
			break;
		case 4:
			scriptRefresh();
			return 0;
		case 5:
			scriptSetState(a);
			return 0;
		case 6:
			scriptSetSequence(a);
			return 0;
		case 7:
			return scriptSetCuePoint();
		case 8:
			return scriptSetAttribute(a, b);
		default:
			debug(5, "IMuseDigital::scriptParse(): unrecognized opcode (%d)", cmd);
			return -1;
		}
	} else {
		debug(5, "IMuseDigital::scriptParse(): script module not initialized");
		return -1;
	}

	return -1;
}

int IMuseDigital::scriptInit() {
	_curMusicState = 0;
	_curMusicSeq = 0;
	_nextSeqToPlay = 0;
	memset(_attributes, 0, sizeof(_attributes));
	return 0;
}

int IMuseDigital::scriptTerminate() {
	diMUSETerminate();

	_curMusicState = 0;
	_curMusicSeq = 0;
	_nextSeqToPlay = 0;
	memset(_attributes, 0, sizeof(_attributes));
	return 0;
}

int IMuseDigital::scriptSave() { return 0; }
int IMuseDigital::scriptRestore() { return 0; }

void IMuseDigital::scriptRefresh() {
	int soundId;
	int nextSound;

	if (_stopSequenceFlag) {
		scriptSetSequence(0);
		_stopSequenceFlag = 0;
	}

	soundId = 0;

	while (1) {
		nextSound = diMUSEGetNextSound(soundId);
		soundId = nextSound;

		if (!nextSound)
			break;

		if (diMUSEGetParam(nextSound, P_SND_HAS_STREAM) && diMUSEGetParam(soundId, P_STREAM_BUFID) == DIMUSE_BUFFER_MUSIC) {
			if (soundId)
				return;
			break;
		}
	}

	if (_curMusicSeq)
		scriptSetSequence(0);

	flushTracks();
	return;
}

void IMuseDigital::scriptSetState(int soundId) {
	if (_vm->_game.id == GID_DIG) {
		setDigMusicState(soundId);
	} else if (_vm->_game.id == GID_CMI) {
		setComiMusicState(soundId);
	}
}

void IMuseDigital::scriptSetSequence(int soundId) {
	if (_vm->_game.id == GID_DIG) {
		setDigMusicSequence(soundId);
	} else if (_vm->_game.id == GID_CMI) {
		setComiMusicSequence(soundId);
	}
}

int IMuseDigital::scriptSetCuePoint() { return 0; }

int IMuseDigital::scriptSetAttribute(int attrIndex, int attrVal) {
	if (_vm->_game.id == GID_DIG) {
		_attributes[attrIndex] = attrVal;
	}
	return 0;
}

int IMuseDigital::scriptTriggerCallback(char *marker) {
	if (marker[0] != '_') {
		debug(5, "IMuseDigital::scriptTriggerCallback(): got marker != '_end', callback ignored");
		return -1;
	}	
	_stopSequenceFlag = 1;
	return 0;
}

void IMuseDigital::setDigMusicState(int stateId) {
	int l, num = -1;

	for (l = 0; _digStateMusicTable[l].soundId != -1; l++) {
		if ((_digStateMusicTable[l].soundId == stateId)) {
			debug(5, "IMuseDigital::setDigMusicState(): Set music state: %s, %s", _digStateMusicTable[l].name, _digStateMusicTable[l].filename);
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

	debug(5, "IMuseDigital::setDigMusicState(): Set music state: %s, %s", _digStateMusicTable[num].name, _digStateMusicTable[num].filename);

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

void IMuseDigital::setDigMusicSequence(int seqId) {
	int l, num = -1;

	if (seqId == 0)
		seqId = 2000;

	for (l = 0; _digSeqMusicTable[l].soundId != -1; l++) {
		if ((_digSeqMusicTable[l].soundId == seqId)) {
			debug(5, "IMuseDigital::setDigMusicSequence(): Set music sequence: %s, %s", _digSeqMusicTable[l].name, _digSeqMusicTable[l].filename);
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

void IMuseDigital::setComiMusicState(int stateId) {
	int l, num = -1;

	if (stateId == 0)
		stateId = 1000;

	if ((_vm->_game.features & GF_DEMO) && stateId == 1000)
		stateId = 0;

	if (!(_vm->_game.features & GF_DEMO)) {
		for (l = 0; _comiStateMusicTable[l].soundId != -1; l++) {
			if ((_comiStateMusicTable[l].soundId == stateId)) {
				debug(5, "IMuseDigital::setComiMusicState(): Set music state: %s, %s", _comiStateMusicTable[l].name, _comiStateMusicTable[l].filename);
				num = l;
				break;
			}
		}
	}

	if (num == -1 && !(_vm->_game.features & GF_DEMO))
		return;

	if (!(_vm->_game.features & GF_DEMO) && _curMusicState == num) {
		return;
	} else if ((_vm->_game.features & GF_DEMO) && _curMusicState == stateId) {
		return;
	}
		
	
	if (_curMusicSeq == 0) {
		if (_vm->_game.features & GF_DEMO) {
			if (_curMusicSeq == 0) {
				if (stateId == 0)
					playComiDemoMusic(NULL, &_comiDemoStateMusicTable[0], stateId, false);
				else
					playComiDemoMusic(_comiDemoStateMusicTable[stateId].name, &_comiDemoStateMusicTable[stateId], stateId, false);
			}
		} else {
			if (num == 0)
				playComiMusic(NULL, &_comiStateMusicTable[0], num, false);
			else
				playComiMusic(_comiStateMusicTable[num].name, &_comiStateMusicTable[num], num, false);
		}
		
	}

	if (!(_vm->_game.features & GF_DEMO)) {
		_curMusicState = num;
	} else {
		_curMusicState = stateId;
	}
		
}

void IMuseDigital::setComiMusicSequence(int seqId) {
	int l, num = -1;

	if (seqId == 0)
		seqId = 2000;

	for (l = 0; _comiSeqMusicTable[l].soundId != -1; l++) {
		if ((_comiSeqMusicTable[l].soundId == seqId)) {
			debug(5, "IMuseDigital::setComiMusicSequence(): Set music sequence: %s, %s", _comiSeqMusicTable[l].name, _comiSeqMusicTable[l].filename);
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

void IMuseDigital::playDigMusic(const char *songName, const imuseDigTable *table, int attribPos, bool sequence) {
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
		nextSoundId = diMUSEGetNextSound(nextSoundId);
		if (!nextSoundId)
			break;

		// If a sound is found (and its stream is active), fade it out if it's a music track
		if (diMUSEGetParam(nextSoundId, P_GROUP) == DIMUSE_GROUP_MUSICEFF && !diMUSEGetParam(nextSoundId, P_SND_HAS_STREAM))
			diMUSEFadeParam(nextSoundId, P_VOLUME, 0, 120);
	}

	int oldSoundId = 0;
	nextSoundId = 0;
	while (1) {
		nextSoundId = diMUSEGetNextSound(nextSoundId);
		if (!nextSoundId)
			break;

		if (diMUSEGetParam(nextSoundId, P_SND_HAS_STREAM) && (diMUSEGetParam(nextSoundId, P_STREAM_BUFID) == DIMUSE_BUFFER_MUSIC)) {
			oldSoundId = nextSoundId;
			break;
		}
	}

	if (!songName) {
		if (oldSoundId)
			diMUSEFadeParam(oldSoundId, P_VOLUME, 0, 120);
		return;
	}

	switch (table->transitionType) {
	case 0:
		debug(5, "IMuseDigital::playDigMusic(): NULL transition, ignored");
		break;
	case 1:
		_filesHandler->openSound(table->soundId);
		if (table->soundId) {
			if (diMUSEStartSound(table->soundId, 126))
				debug(5, "IMuseDigital::playDigMusic(): transition 1, failed to start the sound (%d)", table->soundId);
			diMUSESetParam(table->soundId, P_VOLUME, 1);
			diMUSEFadeParam(table->soundId, P_VOLUME, 127, 120);
			_filesHandler->closeSound(table->soundId);
			diMUSESetParam(table->soundId, P_GROUP, DIMUSE_GROUP_MUSICEFF);
		} else {
			debug(5, "IMuseDigital::playDigMusic(): transition 1, empty soundId, ignored");
		}

		break;
	case 2:
	case 3:
	case 4:
		_filesHandler->openSound(table->soundId);
		if (table->filename[0] == 0 || table->soundId == 0) {
			if (oldSoundId)
				diMUSEFadeParam(oldSoundId, P_VOLUME, 0, 60);
			return;
		}

		if (table->transitionType == 4) {
			_stopSequenceFlag = 0;
			diMUSESetTrigger(table->soundId, MKTAG('_', 'e', 'n', 'd'), 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
		}

		if (oldSoundId) {
			if (table->transitionType == 2) {
				diMUSESwitchStream(oldSoundId, table->soundId, 1800, 0, 0);
				diMUSESetParam(table->soundId, P_VOLUME, 127);
				diMUSESetParam(table->soundId, P_GROUP, DIMUSE_GROUP_MUSICEFF);
				diMUSESetHook(table->soundId, hookId);
				diMUSEProcessStreams();
				diMUSESetParam(table->soundId, P_GROUP, DIMUSE_GROUP_MUSICEFF); // Repeated intentionally
				return;
			}

			if (oldSoundId != table->soundId) {
				if ((!sequence) && (table->attribPos != 0) &&
					(table->attribPos == _digStateMusicTable[_curMusicState].attribPos)) {
					diMUSESwitchStream(oldSoundId, table->soundId, 1800, 0, 1);
					diMUSESetParam(table->soundId, P_VOLUME, 127);
					diMUSESetParam(table->soundId, P_GROUP, DIMUSE_GROUP_MUSICEFF);
					diMUSEProcessStreams();
				} else {
					diMUSESwitchStream(oldSoundId, table->soundId, 1800, 0, 0);
					diMUSESetParam(table->soundId, P_VOLUME, 127);
					diMUSESetParam(table->soundId, P_GROUP, DIMUSE_GROUP_MUSICEFF);
					diMUSESetHook(table->soundId, hookId);
					diMUSEProcessStreams();
					_filesHandler->closeSound(table->soundId);
					diMUSESetParam(table->soundId, P_GROUP, DIMUSE_GROUP_MUSICEFF); // Repeated intentionally
				}
			}
		} else {
			if (diMUSEStartStream(table->soundId, 126, DIMUSE_BUFFER_MUSIC))
				debug(5, "IMuseDigital::playDigMusic(): failed to start the stream for sound %d", table->soundId);
			diMUSESetParam(table->soundId, P_VOLUME, 127);
			diMUSESetParam(table->soundId, P_GROUP, DIMUSE_GROUP_MUSICEFF);
			diMUSESetHook(table->soundId, hookId);
		}
		_filesHandler->closeSound(table->soundId);
		diMUSESetParam(table->soundId, P_GROUP, DIMUSE_GROUP_MUSICEFF); // Repeated intentionally
		break;
	case 5:
		debug(5, "IMuseDigital::playDigMusic(): no-op transition type (5), ignored");
		break;
	case 6:
		_stopSequenceFlag = 0;
		diMUSESetTrigger(SMUSH_SOUNDID + DIMUSE_BUFFER_MUSIC, MKTAG('_', 'e', 'n', 'd'), 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
		break;
	case 7:
		if (oldSoundId)
			diMUSEFadeParam(oldSoundId, P_VOLUME, 0, 60);
		break;
	default:
		debug(5, "IMuseDigital::playDigMusic(): bogus transition type, ignored");
		break;
	}
}

void IMuseDigital::playComiDemoMusic(const char *songName, const imuseComiTable *table, int attribPos, bool sequence) {
	// This is a stripped down version of playDigMusic
	int hookId = 0;

	if (songName != NULL) {
		if (attribPos != 0) {
			if (table->attribPos != 0)
				attribPos = table->attribPos;
		}
	}

	int nextSoundId = 0;
	while (1) {
		nextSoundId = diMUSEGetNextSound(nextSoundId);
		if (!nextSoundId)
			break;

		// If a sound is found (and its stream is active), fade it out if it's a music track
		if (diMUSEGetParam(nextSoundId, P_GROUP) == DIMUSE_GROUP_MUSICEFF && !diMUSEGetParam(nextSoundId, P_SND_HAS_STREAM))
			diMUSEFadeParam(nextSoundId, P_VOLUME, 0, 120);
	}

	int oldSoundId = 0;
	nextSoundId = 0;
	while (1) {
		nextSoundId = diMUSEGetNextSound(nextSoundId);
		if (!nextSoundId)
			break;

		if (diMUSEGetParam(nextSoundId, P_SND_HAS_STREAM) && (diMUSEGetParam(nextSoundId, P_STREAM_BUFID) == DIMUSE_BUFFER_MUSIC)) {
			oldSoundId = nextSoundId;
			break;
		}
	}

	if (!songName) {
		if (oldSoundId)
			diMUSEFadeParam(oldSoundId, P_VOLUME, 0, 120);
		return;
	}

	switch (table->transitionType) {
	case 3:
		_filesHandler->openSound(table->soundId);
		if (table->filename[0] == 0 || table->soundId == 0) {
			if (oldSoundId)
				diMUSEFadeParam(oldSoundId, P_VOLUME, 0, 60);
			return;
		}

		if (oldSoundId) {
			if (oldSoundId != table->soundId) {
				if ((!sequence) && (table->attribPos != 0) &&
					(table->attribPos == _comiDemoStateMusicTable[_curMusicState].attribPos)) {
					diMUSESwitchStream(oldSoundId, table->soundId, 1800, 0, 1);
					diMUSESetParam(table->soundId, P_VOLUME, 127);
					diMUSESetParam(table->soundId, P_GROUP, DIMUSE_GROUP_MUSICEFF);
					diMUSEProcessStreams();
				} else {
					diMUSESwitchStream(oldSoundId, table->soundId, 1800, 0, 0);
					diMUSESetParam(table->soundId, P_VOLUME, 127);
					diMUSESetParam(table->soundId, P_GROUP, DIMUSE_GROUP_MUSICEFF);
					diMUSESetHook(table->soundId, hookId);
					diMUSEProcessStreams();
					_filesHandler->closeSound(table->soundId);
					diMUSESetParam(table->soundId, P_GROUP, DIMUSE_GROUP_MUSICEFF); // Repeated intentionally
				}
			}
		} else {
			if (diMUSEStartStream(table->soundId, 126, DIMUSE_BUFFER_MUSIC))
				debug(5, "IMuseDigital::playComiDemoMusic(): failed to start the stream for sound %d", table->soundId);
			diMUSESetParam(table->soundId, P_VOLUME, 127);
			diMUSESetParam(table->soundId, P_GROUP, DIMUSE_GROUP_MUSICEFF);
			diMUSESetHook(table->soundId, hookId);
		}
		_filesHandler->closeSound(table->soundId);
		diMUSESetParam(table->soundId, P_GROUP, DIMUSE_GROUP_MUSICEFF); // Repeated intentionally
		break;
	default:
		debug(5, "IMuseDigital::playDigMusic(): bogus or unused transition type, ignored");
		break;
	}
}

void IMuseDigital::playComiMusic(const char *songName, const imuseComiTable *table, int attribPos, bool sequence) {
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
		nextSoundId = diMUSEGetNextSound(nextSoundId);
		if (!nextSoundId)
			break;

		// If a sound is found (and its stream is active), fade it out if it's a music track
		if (diMUSEGetParam(nextSoundId, P_GROUP) == DIMUSE_GROUP_MUSICEFF && !diMUSEGetParam(nextSoundId, P_SND_HAS_STREAM))
			diMUSEFadeParam(nextSoundId, P_VOLUME, 0, 120);
	}

	int oldSoundId = 0;
	nextSoundId = 0;
	while (1) {
		nextSoundId = diMUSEGetNextSound(nextSoundId);
		if (!nextSoundId)
			break;

		if (diMUSEGetParam(nextSoundId, P_SND_HAS_STREAM) && (diMUSEGetParam(nextSoundId, P_STREAM_BUFID) == DIMUSE_BUFFER_MUSIC)) {
			oldSoundId = nextSoundId;
			break;
		}
	}
	
	if (!songName) {
		if (oldSoundId)
			diMUSEFadeParam(oldSoundId, P_VOLUME, 0, 120);
		return;
	}

	switch (table->transitionType) {
	case 0:
		debug(5, "IMuseDigital::playComiMusic(): NULL transition, ignored");
		break;
	case 1:
		_filesHandler->openSound(table->soundId);
		if (table->soundId) {
			if (diMUSEStartSound(table->soundId, 126))
				debug(5, "IMuseDigital::playComiMusic(): transition 1, failed to start the sound (%d)", table->soundId);
			diMUSESetParam(table->soundId, P_VOLUME, 1);
			diMUSEFadeParam(table->soundId, P_VOLUME, 127, 120);
			_filesHandler->closeSound(table->soundId);
			diMUSESetParam(table->soundId, P_GROUP, DIMUSE_GROUP_MUSICEFF);
		} else {
			debug(5, "IMuseDigital::playComiMusic(): transition 1, empty soundId, ignored");
		}
		break;
	case 2:
	case 3:
	case 4:
	case 10:
	case 11:
	case 12:
		_filesHandler->openSound(table->soundId);
		if (table->filename[0] == 0 || table->soundId == 0) {
			if (oldSoundId)
				diMUSEFadeParam(oldSoundId, P_VOLUME, 0, 60);
			break;
		}

		if (table->transitionType == 4) {
			_stopSequenceFlag = 0;
			diMUSESetTrigger(table->soundId, MKTAG('_', 'e', 'n', 'd'), 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
		}

		if (oldSoundId) {
			fadeDelay = table->fadeOutDelay;
			if (!fadeDelay)
				fadeDelay = 1000;
			else
				fadeDelay = (fadeDelay * 100) / 6; // Set dimuse_table fade out time to millisecond scale

			if (table->transitionType == 2) {
				diMUSESwitchStream(oldSoundId, table->soundId, fadeDelay, 0, 0);
				diMUSESetParam(table->soundId, P_VOLUME, 127);
				diMUSESetParam(table->soundId, P_GROUP, DIMUSE_GROUP_MUSICEFF);
				diMUSESetHook(table->soundId, table->hookId);
				diMUSEProcessStreams();
			} else if (oldSoundId != table->soundId) {
				if ((!sequence) && (table->attribPos != 0) &&
					(table->attribPos == _comiStateMusicTable[_curMusicState].attribPos)) {

					debug(5, "IMuseDigital::playComiMusic(): Starting new sound (%s) with same attribute as old sound (%s)",
						table->name, _comiStateMusicTable[_curMusicState].name);

					diMUSESwitchStream(oldSoundId, table->soundId, fadeDelay, 0, 1);
					diMUSESetParam(table->soundId, P_VOLUME, 127);
					diMUSESetParam(table->soundId, P_GROUP, DIMUSE_GROUP_MUSICEFF);
					diMUSEProcessStreams();
				} else {
					switch (table->transitionType) {
					case 10:
						diMUSESetTrigger(oldSoundId, MKTAG('e', 'x', 'i', 't'), 26, oldSoundId, table->soundId, fadeDelay, 1, 0, -1, -1, -1, -1, -1, -1);
						diMUSESetTrigger(oldSoundId, MKTAG('e', 'x', 'i', 't'), 12, table->soundId, P_VOLUME, 127, -1, -1, -1, -1, -1, -1, -1, -1);
						diMUSESetTrigger(oldSoundId, MKTAG('e', 'x', 'i', 't'), 12, table->soundId, P_GROUP, 4, -1, -1, -1, -1, -1, -1, -1, -1);
						diMUSESetTrigger(oldSoundId, MKTAG('e', 'x', 'i', 't'), 15, table->soundId, hookId, -1, -1, -1, -1, -1, -1, -1, -1, -1);
						diMUSEProcessStreams();
						break;
					case 11: // Markers actually are 'exit2', which is too long for a char constant; this transition is unused anyway
						diMUSESetTrigger(oldSoundId, MKTAG('e', 'x', 'i', '2'), 26, oldSoundId, table->soundId, fadeDelay, 1, 0, -1, -1, -1, -1, -1, -1);
						diMUSESetTrigger(oldSoundId, MKTAG('e', 'x', 'i', '2'), 12, table->soundId, P_VOLUME, 127, -1, -1, -1, -1, -1, -1, -1, -1);
						diMUSESetTrigger(oldSoundId, MKTAG('e', 'x', 'i', '2'), 12, table->soundId, P_GROUP, 4, -1, -1, -1, -1, -1, -1, -1, -1);
						diMUSESetTrigger(oldSoundId, MKTAG('e', 'x', 'i', '2'), 15, table->soundId, hookId, -1, -1, -1, -1, -1, -1, -1, -1, -1);
						diMUSEProcessStreams();
						break;
					case 12:
						diMUSESetHook(oldSoundId, table->hookId);
						diMUSESetTrigger(oldSoundId, MKTAG('e', 'x', 'i', 't'), 26, oldSoundId, table->soundId, fadeDelay, 1, 0, -1, -1, -1, -1, -1, -1);
						diMUSESetTrigger(oldSoundId, MKTAG('e', 'x', 'i', 't'), 12, table->soundId, P_VOLUME, 127, -1, -1, -1, -1, -1, -1, -1, -1);
						diMUSESetTrigger(oldSoundId, MKTAG('e', 'x', 'i', 't'), 12, table->soundId, P_GROUP, 4, -1, -1, -1, -1, -1, -1, -1, -1);
						diMUSESetTrigger(oldSoundId, MKTAG('e', 'x', 'i', 't'), 15, table->soundId, hookId, -1, -1, -1, -1, -1, -1, -1, -1, -1);
						diMUSEProcessStreams();
						break;
					default:
						diMUSESwitchStream(oldSoundId, table->soundId, fadeDelay, 0, 0);
						diMUSESetParam(table->soundId, P_VOLUME, 127);
						diMUSESetParam(table->soundId, P_GROUP, 4);
						diMUSESetHook(table->soundId, hookId);
						diMUSEProcessStreams();
						break;
					}
				}
			}
		} else {
			if (diMUSEStartStream(table->soundId, 126, DIMUSE_BUFFER_MUSIC))
				debug(5, "IMuseDigital::playComiMusic(): failed to start the stream for sound %d", table->soundId);
			diMUSESetParam(table->soundId, P_VOLUME, 127);
			diMUSESetParam(table->soundId, P_GROUP, DIMUSE_GROUP_MUSICEFF);
			diMUSESetHook(table->soundId, hookId);
		}
		_filesHandler->closeSound(table->soundId);
		diMUSESetParam(table->soundId, P_GROUP, DIMUSE_GROUP_MUSICEFF);
		break;
	case 5:
		debug(5, "IMuseDigital::playComiMusic(): no-op transition type (5), ignored");
		break;
	case 6:
		_stopSequenceFlag = 0;
		diMUSESetTrigger(SMUSH_SOUNDID + DIMUSE_BUFFER_MUSIC, MKTAG('_', 'e', 'n', 'd'), 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
		break;
	case 7:
		if (oldSoundId)
			diMUSEFadeParam(oldSoundId, P_VOLUME, 0, 60);
		break;
	case 8:
		if (oldSoundId)
			diMUSESetHook(oldSoundId, table->hookId);
		break;
	case 9:
		if (oldSoundId)
			diMUSESetHook(oldSoundId, table->hookId);
		_stopSequenceFlag = 0;
		diMUSESetTrigger(oldSoundId, MKTAG('_', 'e', 'n', 'd'), 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
		break;
	default:
		debug(5, "IMuseDigital::playComiMusic(): bogus transition type, ignored");
		break;
	}
}

} // End of namespace Scumm

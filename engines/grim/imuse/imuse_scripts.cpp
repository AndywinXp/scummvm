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

#include "engines/grim/imuse/imuse_engine.h"

namespace Grim {

int Imuse::scriptParse(int cmd, int a, int b) {
	if (_scriptInitializedFlag || !cmd) {
		switch (cmd) {
		case 0:
			if (_scriptInitializedFlag) {
				Debug::debug(Debug::Sound, "Imuse::scriptParse(): script module already initialized");
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
		case 7: // scriptSetCuePoint(a);
			break;
		case 8:
			return scriptSetAttribute(a, b);
		default:
			Debug::debug(Debug::Sound, "Imuse::scriptParse(): unrecognized transitionType (%d)", cmd);
			return -1;
		}
	} else {
		Debug::debug(Debug::Sound, "Imuse::scriptParse(): script module not initialized");
		return -1;
	}

	return -1;
}

int Imuse::scriptInit() {
	_curMusicState = 0;
	_curMusicSeq = 0;
	_nextSeqToPlay = 0;
	_curMusicCue = 0;
	memset(_attributes, 0, sizeof(_attributes));
	return 0;
}

int Imuse::scriptTerminate() {
	diMUSETerminate();

	_curMusicState = 0;
	_curMusicSeq = 0;
	_nextSeqToPlay = 0;
	_curMusicCue = 0;
	memset(_attributes, 0, sizeof(_attributes));
	return 0;
}

void Imuse::scriptRefresh() {
	int soundId;
	int nextSound;

	if (_attributes[227]) {
		scriptSetSequence(0);
		_attributes[227] = 0;
	}

	soundId = 0;

	while (1) {
		nextSound = diMUSEGetNextSound(soundId);
		soundId = nextSound;

		if (!nextSound)
			break;

		if (diMUSEGetParam(nextSound, DIMUSE_P_SND_HAS_STREAM) && diMUSEGetParam(soundId, DIMUSE_P_STREAM_BUFID) == DIMUSE_BUFFER_MUSIC) {
			if (soundId)
				return;
			break;
		}
	}

	if (_curMusicSeq)
		scriptSetSequence(0);

	flushTracks();
}

void Imuse::scriptSetState(int stateId) {
	int l, num = -1;

	for (l = 0; _stateMusicTable[l].soundId != -1; l++) {
		if ((_stateMusicTable[l].soundId == stateId)) {
			Debug::debug(Debug::Sound, "Imuse::scriptSetState(): Set music state: %s", _stateMusicTable[l].filename);
			num = l;
			break;
		}
	}

	if (num == -1)
		return;

	if (_curMusicState == num) {
		return;
	}

	if (_curMusicSeq == 0) {
		if (num == 0)
			playMusic(nullptr, num, false);
		else
			playMusic(&_stateMusicTable[num], num, false);
	}

	_curMusicState = num;
}

void Imuse::scriptSetSequence(int seqId) {
	int l, num = -1;

	if (seqId == 0)
		seqId = 2000;

	for (l = 0; _seqMusicTable[l].soundId != -1; l++) {
		if ((_seqMusicTable[l].soundId == seqId)) {
			Debug::debug(Debug::Sound, "Imuse::scriptSetSequence(): Set music sequence: %s", _seqMusicTable[l].filename);
			num = l;
			break;
		}
	}

	if (num == -1)
		return;

	if (_curMusicSeq == num)
		return;

	if (num) {
		if (_curMusicSeq && ((_seqMusicTable[_curMusicSeq].transitionType == 4) || (_seqMusicTable[_curMusicSeq].transitionType == 6))) {
			_nextSeqToPlay = num;
			return;
		} else {
			playMusic(&_seqMusicTable[num], 0, true);
			_nextSeqToPlay = 0;
			_attributes[num + 185] = 1;
		}
	} else {
		if (_nextSeqToPlay) {
			playMusic(&_seqMusicTable[_nextSeqToPlay], 0, true);
			num = _nextSeqToPlay;
			_nextSeqToPlay = 0;
			_attributes[num + 185] = 1;
		} else {
			if (_curMusicState) {
				playMusic(&_stateMusicTable[_curMusicState], _curMusicState, true);
			} else {
				playMusic(nullptr, _curMusicState, true);
			}
			num = 0;
		}
	}

	_curMusicSeq = num;
}

int Imuse::scriptSetAttribute(int attrIndex, int attrVal) {
	if (attrIndex != -1 && attrIndex < 228 && attrVal != -1) {
		_attributes[attrIndex] = attrVal;
	}
	return 0;
}

int Imuse::scriptTriggerCallback(char *marker) {
	if (marker[0] != '_') {
		Debug::debug(Debug::Sound, "Imuse::scriptTriggerCallback(): got marker != '_end', callback ignored");
		return -1;
	}
	_attributes[227] = 1;
	return 0;
}

void Imuse::playMusic(const ImuseTable *table, int attribPos, bool sequence) {
	int hookId = 0;
	int fadeDelay = 0;

	if (table && attribPos) {
		if (table->attribPos)
			attribPos = table->attribPos;
		hookId = _attributes[attribPos];

		if (table->hookId) {
			if (hookId && (table->hookId > 1)) {
				_attributes[attribPos + 3] = 2;
			} else {
				_attributes[attribPos + 3] = hookId + 1;
				if (table->hookId < hookId + 1)
					_attributes[attribPos + 3] = 1;
			}
		}
	}

	if (hookId == 0)
		hookId = 100;

	int nextSoundId = 0;
	while (1) {
		nextSoundId = diMUSEGetNextSound(nextSoundId);
		if (!nextSoundId)
			break;

		// If a sound is found (and its stream is active), fade it out if it's a music track
		if (diMUSEGetParam(nextSoundId, DIMUSE_P_GROUP) == DIMUSE_GROUP_MUSICEFF && !diMUSEGetParam(nextSoundId, DIMUSE_P_SND_HAS_STREAM))
			diMUSEFadeParam(nextSoundId, DIMUSE_P_VOLUME, 0, 120);
	}

	int oldSoundId = 0;
	nextSoundId = 0;
	while (1) {
		nextSoundId = diMUSEGetNextSound(nextSoundId);
		if (!nextSoundId)
			break;

		if (diMUSEGetParam(nextSoundId, DIMUSE_P_SND_HAS_STREAM) && (diMUSEGetParam(nextSoundId, DIMUSE_P_STREAM_BUFID) == DIMUSE_BUFFER_MUSIC)) {
			oldSoundId = nextSoundId;
			break;
		}
	}

	if (!table) {
		if (oldSoundId)
			diMUSEFadeParam(oldSoundId, DIMUSE_P_VOLUME, 0, 120);
		return;
	}

	fadeDelay = table->fadeTime60HzTicks;
	if (!fadeDelay)
		fadeDelay = 1000;
	else
		fadeDelay = (fadeDelay * 100) / 6; // Convert 60 Hz ticks to milliseconds

	switch (table->transitionType) {
	case 0:
		Debug::debug(Debug::Sound, "Imuse::playMusic(): NULL transition, ignored");
		break;
	case 2:
	case 3:
	case 4:
	case 12:
		_filesHandler->openSound(table->soundId);
		if (table->filename[0] == 0 || table->soundId == 0) {
			if (oldSoundId)
				diMUSEFadeParam(oldSoundId, DIMUSE_P_VOLUME, 0, 60);
			break;
		}

		if (table->transitionType == 4) {
			_attributes[227] = 0;
			diMUSESetTrigger(table->soundId, MKTAG('_', 'e', 'n', 'd'), 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
		}

		if (oldSoundId) {
			if (table->transitionType == 2) {
				diMUSESwitchStream(oldSoundId, table->soundId, fadeDelay, 0, 0);
				diMUSEFadeParam(table->soundId, DIMUSE_P_VOLUME, table->volume, table->fadeTime60HzTicks);
				diMUSEFadeParam(table->soundId, DIMUSE_P_PAN, table->pan, table->fadeTime60HzTicks);
				diMUSESetParam(table->soundId, DIMUSE_P_GROUP, DIMUSE_GROUP_MUSICEFF);
				diMUSESetHook(table->soundId, hookId);
				diMUSEProcessStreams();
			} else if (oldSoundId == table->soundId) {
				// Sound already streaming: just fade the volume and pan parameters to the target ones
				diMUSEFadeParam(table->soundId, DIMUSE_P_VOLUME, table->volume, table->fadeTime60HzTicks);
				if (table->pan) {
					diMUSEFadeParam(table->soundId, 0x700, table->pan, table->fadeTime60HzTicks);
				}
			} else if (!sequence) {
				if (table->attribPos &&
					(table->attribPos == _stateMusicTable[_curMusicState].attribPos)) {

					Debug::debug(Debug::Sound, "Imuse::playMusic(): Starting new sound (%s) with same attribute as old sound (%s)",
						table->filename, _stateMusicTable[_curMusicState].filename);

					diMUSESwitchStream(oldSoundId, table->soundId, fadeDelay, 0, 1);
					diMUSEFadeParam(table->soundId, DIMUSE_P_VOLUME, table->volume, table->fadeTime60HzTicks);
					diMUSEFadeParam(table->soundId, DIMUSE_P_PAN, table->pan, table->fadeTime60HzTicks);
					diMUSESetParam(table->soundId, DIMUSE_P_GROUP, DIMUSE_GROUP_MUSICEFF);
					if (hookId == 100)
						diMUSESetHook(table->soundId, 100);
					diMUSEProcessStreams();
				} else {
					diMUSESwitchStream(oldSoundId, table->soundId, fadeDelay, 0, 0);
					diMUSEFadeParam(table->soundId, DIMUSE_P_VOLUME, table->volume, table->fadeTime60HzTicks);
					diMUSEFadeParam(table->soundId, DIMUSE_P_PAN, table->pan, table->fadeTime60HzTicks);
					diMUSESetParam(table->soundId, DIMUSE_P_GROUP, DIMUSE_GROUP_MUSICEFF);
					diMUSESetHook(table->soundId, hookId);
					diMUSEProcessStreams();
				}
			}
		} else {
			if (diMUSEStartStream(table->soundId, 126, DIMUSE_BUFFER_MUSIC))
				Debug::debug(Debug::Sound, "Imuse::playMusic(): failed to start the stream for sound %d", table->soundId);

			diMUSESetParam(table->soundId, DIMUSE_P_VOLUME, 0);
			diMUSEFadeParam(table->soundId, DIMUSE_P_VOLUME, table->volume, table->fadeTime60HzTicks);
			diMUSESetParam(table->soundId, DIMUSE_P_PAN, table->pan);
			diMUSESetParam(table->soundId, DIMUSE_P_GROUP, DIMUSE_GROUP_MUSICEFF);
			diMUSESetHook(table->soundId, hookId);
		}
		_filesHandler->closeSound(table->soundId);
		diMUSESetParam(table->soundId, DIMUSE_P_GROUP, DIMUSE_GROUP_MUSICEFF);
		break;
	default:
		Debug::debug(Debug::Sound, "Imuse::playMusic(): unsupported transition type, ignored");
		break;
	}
}

} // End of namespace Scumm

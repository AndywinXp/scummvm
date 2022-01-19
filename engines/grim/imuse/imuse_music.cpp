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
/*
#include "engines/grim/debug.h"

#include "engines/grim/imuse/imuse.h"
#include "engines/grim/imuse/imuse_tables.h"

namespace Grim {

void Imuse::setMusicState(int stateId) {
	int l, num = -1;

	if (stateId == 0)
		stateId = 1000;

	for (l = 0; _stateMusicTable[l].soundId != -1; l++) {
		if (_stateMusicTable[l].soundId == stateId) {
			num = l;
			break;
		}
	}
	assert(num != -1);

	Debug::debug(Debug::Sound, "Imuse::setMusicState(): SoundId %d, filename: %s", _stateMusicTable[l].soundId, _stateMusicTable[l].filename);

	if (_curMusicState == num)
		return;

	if (!_curMusicSeq) {
		playMusic(&_stateMusicTable[num], num, false);
	}

	_curMusicState = num;
}

int Imuse::setMusicSequence(int seqId) {
	int l, num = -1;

	if (seqId == -1)
		return _seqMusicTable[_curMusicSeq].soundId;

	if (seqId == 0)
		seqId = 2000;

	for (l = 0; _seqMusicTable[l].soundId != -1; l++) {
		if (_seqMusicTable[l].soundId == seqId) {
			num = l;
			break;
		}
	}

	assert(num != -1);

	Debug::debug(Debug::Sound, "Imuse::setMusicSequence(): SoundId %d, filename: %s", _seqMusicTable[l].soundId, _seqMusicTable[l].filename);

	if (_curMusicSeq == num)
		return _seqMusicTable[_curMusicSeq].soundId;

	if (num) {
		playMusic(&_seqMusicTable[num], 0, true);
	} else {
		playMusic(&_stateMusicTable[_curMusicState], _curMusicState, true);
		num = 0;
	}

	_curMusicSeq = num;
	return _seqMusicTable[_curMusicSeq].soundId;
}

void Imuse::playMusic(const ImuseTable *table, int attribPos, bool sequence) {
	int hookId = 0;

	if (attribPos) {
		if (table->attribPos)
			attribPos = table->attribPos;
		hookId = _attributes[attribPos];
		if (table->hookId) {
			if (hookId && table->hookId > 1) {
				_attributes[attribPos] = 2;
			} else {
				_attributes[attribPos] = hookId + 1;
				if (table->hookId < hookId + 1)
					_attributes[attribPos] = 1;
			}
		}
	}
	if (hookId == 0)
		hookId = 100;

	if (table->transitionType == 0) {
		fadeOutMusic(120);
		return;
	}

	if (table->transitionType == 2 || table->transitionType == 3) {
		if (table->filename[0] == 0) {
			fadeOutMusic(60);
			return;
		}
		char *soundName = getCurMusicSoundName();
		int pan;

		if (table->pan == 0)
			pan = 64;
		else
			pan = table->pan;
		if (!soundName) {
			startMusic(table->filename, hookId, 0, pan);
			setVolume(table->filename, 0);
			setFadeVolume(table->filename, table->volume, table->fadeTime60HzTicks);
			return;
		}
		int old_pan = getCurMusicPan();
		int old_vol = getCurMusicVol();
		if (old_pan == -1)
			old_pan = 64;
		if (old_vol == -1)
			old_vol = 127;

		if (table->transitionType == 2) {
			fadeOutMusic(table->fadeTime60HzTicks);
			startMusic(table->filename, hookId, table->volume, pan);
			setVolume(table->filename, 0);
			setFadeVolume(table->filename, table->volume, table->fadeTime60HzTicks);
			setFadePan(table->filename, pan, table->fadeTime60HzTicks);
			return;
		}
		if (strcmp(soundName, table->filename) == 0) {
			setFadeVolume(soundName, table->volume, table->fadeTime60HzTicks);
			setFadePan(soundName, pan, table->fadeTime60HzTicks);
			return;
		}

		if (!sequence && table->attribPos && table->attribPos == _stateMusicTable[_curMusicState].attribPos) {
			fadeOutMusicAndStartNew(table->fadeTime60HzTicks, table->filename, hookId, old_vol, old_pan);
			setVolume(table->filename, 0);
			setFadeVolume(table->filename, table->volume, table->fadeTime60HzTicks);
			setFadePan(table->filename, pan, table->fadeTime60HzTicks);
		} else {
			fadeOutMusic(table->fadeTime60HzTicks);
			startMusic(table->filename, hookId, table->volume, pan);
			setVolume(table->filename, 0);
			setFadeVolume(table->filename, table->volume, table->fadeTime60HzTicks);
		}
	}
}

} // end of namespace Grim*/

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

#include "common/system.h"
#include "common/timer.h"

#include "scumm/actor.h"
#include "scumm/scumm_v7.h"
#include "scumm/sound.h"
#include "scumm/dimuse.h"
#include "scumm/dimuse_v2/dimuse_v2.h"
#include "scumm/dimuse_v1/dimuse_bndmgr.h"
#include "scumm/dimuse_v1/dimuse_sndmgr.h"
#include "scumm/dimuse_v1/dimuse_tables.h"

#include "audio/audiostream.h"
#include "audio/mixer.h"

namespace Scumm {

void DiMUSE_v2::timer_handler(void *refCon) {
	DiMUSE_v2 *diMUSE = (DiMUSE_v2 *)refCon;
	diMUSE->callback();
}

DiMUSE_v2::DiMUSE_v2(ScummEngine_v7 *scumm, Audio::Mixer *mixer, int fps)
	: _vm(scumm), _mixer(mixer), DiMUSE(scumm, mixer, fps) {
	assert(_vm);
	assert(mixer);
	_callbackFps = fps;
	_diMUSEMixer = new DiMUSE_InternalMixer(mixer);
	_sound = new DiMUSESndMgr(_vm, true);
	assert(_sound);
	DiMUSE_initialize();
	DiMUSE_initializeScript();
	DiMUSE_allocSoundBuffer(1, 176000, 44000, 88000);
	DiMUSE_allocSoundBuffer(2, 528000, 44000, 352000);
	
	_vm->getTimerManager()->installTimerProc(timer_handler, 1000000 / _callbackFps, this, "DiMUSE_v2");
}

DiMUSE_v2::~DiMUSE_v2() {
	_vm->getTimerManager()->removeTimerProc(timer_handler);
	cmds_deinit();
	DiMUSE_terminate();
	
	delete _sound;
	DiMUSE_deallocSoundBuffer(1);
	DiMUSE_deallocSoundBuffer(2);
}

void DiMUSE_v2::stopSound(int sound) {
	DiMUSE_stopSound(sound);
}

void DiMUSE_v2::stopAllSounds() {
	DiMUSE_stopAllSounds();
}

int DiMUSE_v2::isSoundRunning(int soundId) {
	int result = DiMUSE_getParam(soundId, 0x100) > 0;
	//debug(5, "DiMUSE_v2::isSoundRunning(%d): %d", soundId, result);
	return result;
}

int DiMUSE_v2::startVoice(int soundId, const char *soundName) {

	if (_vm->_game.id == GID_DIG) {
		if (!strcmp(soundName, "PIG.018"))
			strcpy(_currentSpeechFile, "PIG.019");
		else
			strcpy(_currentSpeechFile, soundName);

		//IMUSE_SetTrigger(kTalkSoundID, byte_451808, speechTriggerFunction);
		_sound->closeSoundById(soundId);
		files_openSound(kTalkSoundID);
		DiMUSE_startStream(kTalkSoundID, 127, 1);
		DiMUSE_setParam(kTalkSoundID, 0x400, 2);
		if (_vm->VAR(_vm->VAR_TALK_ACTOR) == _vm->VAR(_vm->VAR_EGO)) {
			DiMUSE_setParam(kTalkSoundID, 0xA00, 0);
			DiMUSE_setParam(kTalkSoundID, 0x600, 127);
		} else {
			DiMUSE_setParam(kTalkSoundID, 0xA00, _radioChatterSFX);
			DiMUSE_setParam(kTalkSoundID, 0x600, 88);
		}
		files_closeSound(kTalkSoundID);
	} else {
		strcpy(_currentSpeechFile, soundName);
		_sound->closeSoundById(soundId);
		files_openSound(kTalkSoundID);
		DiMUSE_startStream(kTalkSoundID, 127, 1);
		DiMUSE_setParam(kTalkSoundID, 0x400, 2);

		// Let's not give the occasion to raise errors here
		if (_vm->isValidActor(_vm->VAR(_vm->VAR_TALK_ACTOR))) {
			Actor *a = _vm->derefActor(_vm->VAR(_vm->VAR_TALK_ACTOR), "DiMUSE_v2::startVoice");
			if (_vm->VAR(_vm->VAR_VOICE_MODE) == 2)
				DiMUSE_setParam(kTalkSoundID, 0x600, 0);
			else
				DiMUSE_setParam(kTalkSoundID, 0x600, a->_talkVolume);

			DiMUSE_setParam(kTalkSoundID, 0x900, a->_talkFrequency);
			DiMUSE_setParam(kTalkSoundID, 0x700, a->_talkPan);
		}

		// The interpreter really calls for processStreams two times in a row,
		// and who am I to contradict it?
		DiMUSE_processStreams();
		DiMUSE_processStreams();
	}

	return 0;
}

void DiMUSE_v2::DiMUSE_allocSoundBuffer(int bufId, int size, int loadSize, int criticalSize) {
	iMUSESoundBuffer *selectedSoundBuf;

	selectedSoundBuf = &_soundBuffers[bufId];
	selectedSoundBuf->buffer = (uint8 *)malloc(size);
	selectedSoundBuf->bufSize = size;
	selectedSoundBuf->loadSize = loadSize;
	selectedSoundBuf->criticalSize = criticalSize;
}

void DiMUSE_v2::DiMUSE_deallocSoundBuffer(int bufId) {
	iMUSESoundBuffer *selectedSoundBuf;

	selectedSoundBuf = &_soundBuffers[bufId];
	free(selectedSoundBuf->buffer);
}

void DiMUSE_v2::refreshScripts() {
	DiMUSE_refreshScript();
}

void DiMUSE_v2::setRadioChatterSFX(bool state) {
	_radioChatterSFX = state;
}

int DiMUSE_v2::startSfx(int soundId, int priority) {
	DiMUSE_startSound(soundId, priority);
	DiMUSE_setParam(soundId, 0x400, 1);
	return 0;
}

void DiMUSE_v2::callback() {
	Common::StackLock lock(_mutex, "DiMUSE_v2::callback()");
	if (cmd_pauseCount)
		return;

	if (!timer_intFlag) {
		timer_intFlag = 1;
		iMUSEHeartbeat();
		timer_intFlag = 0;
	}
}

void DiMUSE_v2::iMUSEHeartbeat() {
	// This is what happens:
	// - Usual audio stuff like fetching and playing sound (and everything 
	//   within waveapi_callback()) happens at a base 50Hz rate;
	// - Triggers and fades handling happens at a (somewhat hacky) 60Hz rate;
	// - Music gain reduction happens at a 10Hz rate.

	int soundId, foundGroupId, musicTargetVolume, musicEffVol, musicVol, tempVol, tempEffVol;

	int usecPerInt = timer_getUsecPerInt(); // Always returns 20000 microseconds (50 Hz)
	waveapi_callback();

	cmd_running60HzCount += usecPerInt;
	while (cmd_running60HzCount >= 16667) {
		cmd_running60HzCount -= 16667;
		fades_loop();
		triggers_loop();
	}

	cmd_running10HzCount += usecPerInt;
	if (cmd_running10HzCount < 100000)
		return;

	do {
		// SPEECH GAIN REDUCTION 10Hz
		cmd_running10HzCount -= 100000;
		soundId = 0;
		musicTargetVolume = groups_setGroupVol(IMUSE_GROUP_MUSIC, -1);
		while (1) { // Check all tracks to see if there's a speech file playing
			soundId = wave_getNextSound(soundId);
			if (!soundId)
				break;

			foundGroupId = -1;
			if (files_getNextSound(soundId) == 2) {
				foundGroupId = wave_getParam(soundId, 0x400); // Check the groupId of this sound
			}

			if (foundGroupId == IMUSE_GROUP_SPEECH) {
				// Remember: when a speech file stops playing this block stops 
				// being executed, so musicTargetVolume returns back to its original value
				musicTargetVolume = (musicTargetVolume * 80) / 128;
				break;
			}
		}

		musicEffVol = groups_setGroupVol(IMUSE_GROUP_MUSICEFF, -1); // MUSIC EFFECTIVE VOLUME GROUP (used for gain reduction)
		musicVol = groups_setGroupVol(IMUSE_GROUP_MUSIC, -1); // MUSIC VOLUME SUBGROUP (keeps track of original music volume)

		if (musicEffVol < musicTargetVolume) { // If there is gain reduction already going on...
			tempEffVol = musicEffVol + 3;
			if (tempEffVol < musicTargetVolume) {
				if (musicVol <= tempEffVol) {
					musicVol = tempEffVol;
				}
			} else if (musicVol <= musicTargetVolume) { // Bring up the music volume immediately when speech stops playing
				musicVol = musicTargetVolume;
			}
			groups_setGroupVol(IMUSE_GROUP_MUSICEFF, musicVol);
		} else if (musicEffVol > musicTargetVolume) {
			// Bring music volume down to target volume with a -18 step if there's speech playing
		    // or else, just cap it to the target if it's out of range
			tempVol = musicEffVol - 18;
			if (tempVol <= musicTargetVolume) {
				if (musicVol >= musicTargetVolume) {
					musicVol = musicTargetVolume;
				}
			} else {
				if (musicVol >= tempVol) {
					musicVol = tempVol;
				}
			}
			groups_setGroupVol(IMUSE_GROUP_MUSICEFF, musicVol);
		}
		
	} while (cmd_running10HzCount >= 100000);
}

void DiMUSE_v2::setVolume(int soundId, int volume) {
	DiMUSE_setParam(soundId, 0x600, volume);
	if (soundId == kTalkSoundID)
		_currentSpeechVolume = volume;
}

void DiMUSE_v2::setPan(int soundId, int pan) {
	DiMUSE_setParam(soundId, 0x700, pan);
	if (soundId == kTalkSoundID)
		_currentSpeechPan = pan;
}

void DiMUSE_v2::setFrequency(int soundId, int frequency) {
	DiMUSE_setParam(soundId, 0x900, frequency);
	if (soundId == kTalkSoundID)
		_currentSpeechFrequency = frequency;
}

int DiMUSE_v2::getCurSpeechVolume() {
	return _currentSpeechVolume;
}

int DiMUSE_v2::getCurSpeechPan() {
	return _currentSpeechPan;
}

int DiMUSE_v2::getCurSpeechFrequency() {
	return _currentSpeechFrequency;
}

void DiMUSE_v2::flushTracks() {
	DiMUSESndMgr::SoundDesc *s = _sound->getSounds();
	for (int i = 0; i < MAX_IMUSE_SOUNDS; i++) {
		DiMUSESndMgr::SoundDesc *curSnd = &s[i];
		if (curSnd && curSnd->inUse) {
			if (curSnd->scheduledForDealloc)
				if (!DiMUSE_getParam(curSnd->soundId, 0x100) && !DiMUSE_getParam(curSnd->soundId, 0x200))
					_sound->closeSound(curSnd);
		}
	}
}

int32 DiMUSE_v2::getCurMusicPosInMs() {
	int soundId, curSoundId;

	curSoundId = 0;
	soundId = 0;
	while (1) {
		curSoundId = DiMUSE_getNextSound(curSoundId);
		if (!curSoundId)
			break;

		if (DiMUSE_getParam(curSoundId, 0x1800) && DiMUSE_getParam(curSoundId, 0x1900) == 2) {
			soundId = curSoundId;
			return DiMUSE_getParam(soundId, 0x1A00);
		}
	}

	return DiMUSE_getParam(soundId, 0x1A00);
}

int32 DiMUSE_v2::getCurVoiceLipSyncWidth() {
	int32 width, height;
	getSpeechLipSyncInfo(&width, &height);
	return width;
}

int32 DiMUSE_v2::getCurVoiceLipSyncHeight() {
	int32 width, height;
	getSpeechLipSyncInfo(&width, &height);
	return height;
}

int32 DiMUSE_v2::getCurMusicLipSyncWidth(int syncId) {
	int32 width, height;
	getMusicLipSyncInfo(syncId, &width, &height);
	return width;
}

int32 DiMUSE_v2::getCurMusicLipSyncHeight(int syncId) {
	int32 width, height;
	getMusicLipSyncInfo(syncId, &width, &height);
	return height;
}

void DiMUSE_v2::getSpeechLipSyncInfo(int32 *width, int32 *height) {
	int curSpeechPosInMs;

	*width = 0;
	*height = 0;

	if (DiMUSE_getParam(kTalkSoundID, 0x100) > 0) {
		curSpeechPosInMs = DiMUSE_getParam(kTalkSoundID, 0x1A00);
		DiMUSE_lipSync(kTalkSoundID, 0, _vm->VAR(_vm->VAR_SYNC) + curSpeechPosInMs + 50, width, height);
	}
}

void DiMUSE_v2::getMusicLipSyncInfo(int syncId, int32 *width, int32 *height) {
	int soundId;
	int speechSoundId;
	int curSpeechPosInMs;

	soundId = 0;
	speechSoundId = 0;
	*width = 0;
	*height = 0;
	while (1) {
		soundId = DiMUSE_getNextSound(soundId);
		if (!soundId)
			break;
		if (DiMUSE_getParam(soundId, 0x1800)) {
			if (DiMUSE_getParam(soundId, 0x1900) == 2) {
				speechSoundId = soundId;
				break;
			}
		}
	}

	if (speechSoundId) {
		curSpeechPosInMs = DiMUSE_getParam(speechSoundId, 0x1A00);
		DiMUSE_lipSync(speechSoundId, syncId, _vm->VAR(_vm->VAR_SYNC) + curSpeechPosInMs + 50, width, height);
	}
}

void DiMUSE_v2::pause(bool p) {
	if (p) {
		debug(5, "DiMUSE_v2::pause(): pausing...");
		DiMUSE_pause();
	} else {
		debug(5, "DiMUSE_v2::pause(): resuming...");
		DiMUSE_resume();
	}
}

void DiMUSE_v2::parseScriptCmds(int cmd, int soundId, int sub_cmd, int d, int e, int f, int g, int h, int i, int j, int k, int l, int m, int n, int o, int p) {
	int b = soundId;
	int c = sub_cmd;
	switch (cmd) {
	case 0x1000:
		// SetState
		DiMUSE_setState(soundId);
		break;
	case 0x1001:
		// SetSequence
		DiMUSE_setSequence(soundId);
		break;
	case 0x1002:
		// SetCuePoint
		break;
	case 0x1003:
		// SetAttribute
		DiMUSE_setAttribute(b, c);
		break;
	case 0x2000:
		// SetGroupSfxVolume
		DiMUSE_setGroupVol_SFX(b);
		break;
	case 0x2001:
		// SetGroupVoiceVolume
		DiMUSE_setGroupVol_Voice(b);
		break;
	case 0x2002:
		// SetGroupMusicVolume
		DiMUSE_setGroupVol_Music(b);
		break;
	case 10: // StopAllSounds
	case 12: // SetParam
	case 14: // FadeParam
		cmds_handleCmds(cmd, soundId, sub_cmd, d, e, f, g, h, i, j, k, l, m, n, o);
		break;
	default:
		error("DiMUSE_v2::doCommand DEFAULT command %d", cmd);
	}

};

int DiMUSE_v2::DiMUSE_terminate() {
	if (_scriptInitializedFlag) {
		DiMUSE_stopAllSounds();
		files_closeAllSounds();
	}
		
	return 0;
}

int DiMUSE_v2::DiMUSE_initialize() {
	return cmds_handleCmds(0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
}

int DiMUSE_v2::DiMUSE_pause() {
	return cmds_handleCmds(3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
}

int DiMUSE_v2::DiMUSE_resume() {
	return cmds_handleCmds(4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
}

int DiMUSE_v2::DiMUSE_save() { return 0; }
int DiMUSE_v2::DiMUSE_restore() { return 0; }

int DiMUSE_v2::DiMUSE_setGroupVol(int groupId, int volume) {
	return cmds_handleCmds(7, groupId, volume, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
}

int DiMUSE_v2::DiMUSE_startSound(int soundId, int priority) {
	return cmds_handleCmds(8, soundId, priority, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
}

int DiMUSE_v2::DiMUSE_stopSound(int soundId) {
	debug(5, "DiMUSE_v2::DiMUSE_stopSound(): %d", soundId);
	return cmds_handleCmds(9, soundId, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
}

int DiMUSE_v2::DiMUSE_stopAllSounds() {
	debug(5, "DiMUSE_v2::DiMUSE_stopAllSounds()");
	return cmds_handleCmds(10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
}

int DiMUSE_v2::DiMUSE_getNextSound(int soundId) {
	return cmds_handleCmds(11, soundId, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
}

int DiMUSE_v2::DiMUSE_setParam(int soundId, int paramId, int value) {
	return cmds_handleCmds(12, soundId, paramId, value, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
}

int DiMUSE_v2::DiMUSE_getParam(int soundId, int paramId) {
	return cmds_handleCmds(13, soundId, paramId, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
}

int DiMUSE_v2::DiMUSE_fadeParam(int soundId, int opcode, int destValue, int fadeLength) {
	return cmds_handleCmds(14, soundId, opcode, destValue, fadeLength, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
}

int DiMUSE_v2::DiMUSE_setHook(int soundId, int hookId) {
	return cmds_handleCmds(15, soundId, hookId, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
}

int DiMUSE_v2::DiMUSE_setTrigger(int soundId, int marker, int opcode, int d, int e, int f, int g, int h, int i, int j, int k, int l, int m, int n) {
	return cmds_handleCmds(17, soundId, marker, opcode, e, f, g, h, i, j, k, l, m, n, -1);
}

int DiMUSE_v2::DiMUSE_startStream(int soundId, int priority, int bufferId) {
 	return cmds_handleCmds(25, soundId, priority, bufferId, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
}

int DiMUSE_v2::DiMUSE_switchStream(int oldSoundId, int newSoundId, int fadeDelay, int fadeSyncFlag2, int fadeSyncFlag1) {
	return cmds_handleCmds(26, oldSoundId, newSoundId, fadeDelay, fadeSyncFlag2, fadeSyncFlag1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
}

int DiMUSE_v2::DiMUSE_processStreams() {
	return cmds_handleCmds(27, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
}

int DiMUSE_v2::DiMUSE_queryStream() { return 0; }
int DiMUSE_v2::DiMUSE_feedStream() { return 0; }

int DiMUSE_v2::DiMUSE_lipSync(int soundId, int syncId, int msPos, int32 *width, int32 *height) {
	// Skip over cmds_handleCmds for this one, I really don't like the idea of having to use intptr_t casts
	// return cmds_handleCmds(30, soundId, syncId, msPos, (intptr_t)width, (intptr_t)height, -1, -1, -1, -1, -1, -1, -1, -1, -1);
	return wave_lipSync(soundId, syncId, msPos, width, height);
}

int DiMUSE_v2::DiMUSE_setGroupVol_Music(int volume) {
	debug(5, "DiMUSE_v2::DiMUSE_setGroupVol_Music(): %d", volume);
	DiMUSE_setGroupVol(3, volume);
	return 0;
}

int DiMUSE_v2::DiMUSE_setGroupVol_SFX(int volume) {
	debug(5, "DiMUSE_v2::DiMUSE_setGroupVol_SFX(): %d", volume);
	DiMUSE_setGroupVol(1, volume);
	return 0;
}

int DiMUSE_v2::DiMUSE_setGroupVol_Voice(int volume) {
	debug(5, "DiMUSE_v2::DiMUSE_setGroupVol_Voice(): %d", volume);
	DiMUSE_setGroupVol(2, volume);
	return 0;
}

int DiMUSE_v2::DiMUSE_getGroupVol_Music() {
	return DiMUSE_setGroupVol(3, -1);
}

int DiMUSE_v2::DiMUSE_getGroupVol_SFX() {
	return DiMUSE_setGroupVol(1, -1);
}

int DiMUSE_v2::DiMUSE_getGroupVol_Voice() {
	return DiMUSE_setGroupVol(2, -1);
}

int DiMUSE_v2::DiMUSE_get_some1() { return 0; }

int DiMUSE_v2::DiMUSE_initializeScript() {
	script_parse(0, -1, -1, -1, -1, -1, -1, -1, -1);
	return 0;
}

int DiMUSE_v2::DiMUSE_terminateScript() { return 0; }
int DiMUSE_v2::DiMUSE_saveScript() { return 0; }
int DiMUSE_v2::DiMUSE_restoreScript() { return 0; }

void DiMUSE_v2::DiMUSE_refreshScript() {
	DiMUSE_processStreams();
	script_parse(4, -1, -1, -1, -1, -1, -1, -1, -1);
}

int DiMUSE_v2::DiMUSE_setState(int soundId) {
	script_parse(5, soundId, -1, -1, -1, -1, -1, -1, -1);
	return 0;
}

int DiMUSE_v2::DiMUSE_setSequence(int soundId) {
	script_parse(6, soundId, -1, -1, -1, -1, -1, -1, -1);
	return 0;
}

int DiMUSE_v2::DiMUSE_setCuePoint() {
	script_parse(7, -1, -1, -1, -1, -1, -1, -1, -1);
	return 0;
}

int DiMUSE_v2::DiMUSE_setAttribute(int attrIndex, int attrVal) {
	if (_vm->_game.id == GID_DIG) {
		_attributes[attrIndex] = attrVal;
	}
	return 0;
}

} // End of namespace Scumm

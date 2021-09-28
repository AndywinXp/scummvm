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
#include "scumm/dimuse_v2/dimuse_v2_defs.h"
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

	_curMixerMusicVolume = _mixer->getVolumeForSoundType(Audio::Mixer::kMusicSoundType);
	_curMixerSpeechVolume = _mixer->getVolumeForSoundType(Audio::Mixer::kSpeechSoundType);
	_curMixerSFXVolume = _mixer->getVolumeForSoundType(Audio::Mixer::kSFXSoundType);
	_currentSpeechVolume = 0;
	_currentSpeechFrequency = 0;
	_currentSpeechPan = 0;

	_waveOutXorTrigger = 0;
	_waveOutWriteIndex = 0;
	_waveOutDisableWrite = 0;
	_waveOutPreferredFeedSize = 0;

	_dispatchFadeSize = 0;

	_stopSequenceFlag = 0;
	_scriptInitializedFlag = 0;

	_radioChatterSFX = false;

	_emptyMarker[0] = '\0';
	_internalMixer = new DiMUSEInternalMixer(mixer);
	_groupsHandler = new DiMUSEGroupsHandler(this);
	_timerHandler = new DiMUSETimerHandler();
	_fadesHandler = new DiMUSEFadesHandler(this);
	_triggersHandler = new DiMUSETriggersHandler(this);
	_filesHandler = new DiMUSEFilesHandler(this, scumm);
	
	diMUSEInitialize();
	diMUSEInitializeScript();
	if (_vm->_game.id == GID_CMI) {
		_filesHandler->allocSoundBuffer(DIMUSE_BUFFER_SPEECH, 176000, 44000, 88000);
		_filesHandler->allocSoundBuffer(DIMUSE_BUFFER_MUSIC, 528000, 44000, 352000);
	} else {
		_filesHandler->allocSoundBuffer(DIMUSE_BUFFER_SPEECH, 88000, 22000, 44000);
		_filesHandler->allocSoundBuffer(DIMUSE_BUFFER_MUSIC, 528000, 11000, 132000);
	}
	
	_filesHandler->allocSoundBuffer(DIMUSE_BUFFER_SFX, 198000, 0, 0);

	_vm->getTimerManager()->installTimerProc(timer_handler, 1000000 / _callbackFps, this, "DiMUSE_v2");
}

DiMUSE_v2::~DiMUSE_v2() {
	_vm->getTimerManager()->removeTimerProc(timer_handler);
	_filesHandler->deallocSoundBuffer(DIMUSE_BUFFER_SPEECH);
	_filesHandler->deallocSoundBuffer(DIMUSE_BUFFER_MUSIC);
	_filesHandler->deallocSoundBuffer(DIMUSE_BUFFER_SFX);
	cmdsDeinit();
	diMUSETerminate();
	delete _internalMixer;
	delete _groupsHandler;
	delete _timerHandler;
	delete _fadesHandler;
	delete _triggersHandler;
	delete _filesHandler;

	// Deinit the Dispatch module
	free(_dispatchBuffer);
	_dispatchBuffer = NULL;

	// Deinit the WaveOut module
	free(_waveOutOutputBuffer);
	_waveOutOutputBuffer = NULL;
}

void DiMUSE_v2::stopSound(int sound) {
	diMUSEStopSound(sound);
}

void DiMUSE_v2::stopAllSounds() {
	diMUSEStopAllSounds();
}

int DiMUSE_v2::isSoundRunning(int soundId) {
	return diMUSEGetParam(soundId, P_SND_TRACK_NUM) > 0;
}

int DiMUSE_v2::startVoice(int soundId, const char *soundName, byte speakingActorId) {
	_filesHandler->closeSoundImmediatelyById(soundId);

	int fileDoesNotExist = 0;
	if (_vm->_game.id == GID_DIG) {
		if (!strcmp(soundName, "PIG.018"))
			fileDoesNotExist = _filesHandler->setCurrentSpeechFile("PIG.019");
		else
			fileDoesNotExist = _filesHandler->setCurrentSpeechFile(soundName);

		if (fileDoesNotExist)
			return 1;

		// Workaround for this particular sound file not playing (this is a bug in the original):
		// this is happening because the sound buffer responsible for speech
		// is still busy with the previous speech file playing during the SAN
		// movie. We just stop the SMUSH speech sound before playing NEXUS.029.
		if (!strcmp(soundName, "NEXUS.029")) {
			diMUSEStopSound(SMUSH_SOUNDID + DIMUSE_BUFFER_SPEECH);
		}

		diMUSEStartStream(kTalkSoundID, 127, DIMUSE_BUFFER_SPEECH);
		diMUSESetParam(kTalkSoundID, P_GROUP, DIMUSE_GROUP_SPEECH);
		if (speakingActorId == _vm->VAR(_vm->VAR_EGO)) {
			diMUSESetParam(kTalkSoundID, P_MAILBOX, 0);
			diMUSESetParam(kTalkSoundID, P_VOLUME, 127);
		} else {
			diMUSESetParam(kTalkSoundID, P_MAILBOX, _radioChatterSFX);
			diMUSESetParam(kTalkSoundID, P_VOLUME, 88);
		}
		_filesHandler->closeSound(kTalkSoundID);
	} else {
		fileDoesNotExist = _filesHandler->setCurrentSpeechFile(soundName);
		if (fileDoesNotExist)
			return 1;

		diMUSEStartStream(kTalkSoundID, 127, DIMUSE_BUFFER_SPEECH);
		diMUSESetParam(kTalkSoundID, P_GROUP, DIMUSE_GROUP_SPEECH);

		// Let's not give the occasion to raise errors here
		if (_vm->isValidActor(_vm->VAR(_vm->VAR_TALK_ACTOR))) {
			Actor *a = _vm->derefActor(_vm->VAR(_vm->VAR_TALK_ACTOR), "DiMUSE_v2::startVoice");
			if (_vm->VAR(_vm->VAR_VOICE_MODE) == 2)
				diMUSESetParam(kTalkSoundID, P_VOLUME, 0);
			else
				diMUSESetParam(kTalkSoundID, P_VOLUME, a->_talkVolume);

			diMUSESetParam(kTalkSoundID, P_TRANSPOSE, a->_talkFrequency);
			diMUSESetParam(kTalkSoundID, P_PAN, a->_talkPan);
		}

		// The interpreter really calls for processStreams two times in a row,
		// and who am I to contradict it?
		diMUSEProcessStreams();
		diMUSEProcessStreams();
	}

	return 0;
}

void DiMUSE_v2::saveLoadEarly(Common::Serializer &s) {
	Common::StackLock lock(_mutex, "DiMUSE_v2::saveLoadEarly()");

	if (s.isLoading()) {
		diMUSEStopAllSounds();
		_filesHandler->closeSoundImmediatelyById(kTalkSoundID);
	}

	if (s.getVersion() < 103) {
		// Just load the current state and sequence, and play them
		debug(5, "DiMUSE_v2::saveLoadEarly(): old savegame detected (version %d), game may load with an undesired audio status", s.getVersion());
		s.skip(4, VER(31), VER(42)); // _volVoice
		s.skip(4, VER(31), VER(42)); // _volSfx
		s.skip(4, VER(31), VER(42)); // _volMusic
		s.syncAsSint32LE(_curMusicState, VER(31));
		s.syncAsSint32LE(_curMusicSeq, VER(31));
		s.skip(4); // _curMusicCue
		s.syncAsSint32LE(_nextSeqToPlay, VER(31));
		s.syncAsByte(_radioChatterSFX, VER(76));
		s.syncArray(_attributes, 188, Common::Serializer::Sint32LE, VER(31));

		int stateSoundId = 0;
		int seqSoundId = 0;

		if (_vm->_game.id == GID_DIG) {
			stateSoundId = _digStateMusicTable[_curMusicState].soundId;
			seqSoundId = _digSeqMusicTable[_curMusicSeq].soundId;
		} else {
			stateSoundId = _comiStateMusicTable[_curMusicState].soundId;
			seqSoundId = _comiSeqMusicTable[_curMusicSeq].soundId;
		}

		_curMusicState = 0;
		_curMusicSeq = 0;
		scriptSetSequence(seqSoundId);
		scriptSetState(stateSoundId);

	} else {
		diMUSESaveLoad(s);
	}
}

void DiMUSE_v2::refreshScripts() {
	if (!_vm->isSmushActive()) {
		diMUSEProcessStreams();
		diMUSERefreshScript();
	}
}

void DiMUSE_v2::setRadioChatterSFX(bool state) {
	_radioChatterSFX = state;
}

int DiMUSE_v2::startSfx(int soundId, int priority) {
	diMUSEStartSound(soundId, priority);
	diMUSESetParam(soundId, P_GROUP, DIMUSE_GROUP_SFX);
	return 0;
}

void DiMUSE_v2::callback() {
	if (_cmdsPauseCount)
		return;

	if (!_timerHandler->getInterruptFlag()) {
		_timerHandler->setInterruptFlag(1);
		diMUSEHeartbeat();
		_timerHandler->setInterruptFlag(0);
	}
}

void DiMUSE_v2::diMUSEHeartbeat() {
	// This is what happens:
	// - Usual audio stuff like fetching and playing sound (and everything 
	//   within waveapi_callback()) happens at a base 50Hz rate;
	// - Triggers and fades handling happens at a (somewhat hacky) 60Hz rate;
	// - Music gain reduction happens at a 10Hz rate.

	int soundId, foundGroupId, musicTargetVolume, musicEffVol, musicVol, tempVol, tempEffVol;

	int usecPerInt = _timerHandler->getUsecPerInt(); // (Set to 50 Hz)
	waveOutCallback();

	// Update volumes

	if (_curMixerMusicVolume != _mixer->getVolumeForSoundType(Audio::Mixer::kMusicSoundType)) {
		_curMixerMusicVolume = _mixer->getVolumeForSoundType(Audio::Mixer::kMusicSoundType);
		diMUSESetMusicGroupVol(CLIP(_mixer->getVolumeForSoundType(Audio::Mixer::kMusicSoundType) / 2, 0, 127));
	}

	if (_curMixerSpeechVolume != _mixer->getVolumeForSoundType(Audio::Mixer::kSpeechSoundType)) {
		_curMixerSpeechVolume = _mixer->getVolumeForSoundType(Audio::Mixer::kSpeechSoundType);
		diMUSESetVoiceGroupVol(CLIP(_mixer->getVolumeForSoundType(Audio::Mixer::kSpeechSoundType) / 2, 0, 127));
	}

	if (_curMixerSFXVolume != _mixer->getVolumeForSoundType(Audio::Mixer::kSFXSoundType)) {
		_curMixerSFXVolume = _mixer->getVolumeForSoundType(Audio::Mixer::kSFXSoundType);
		diMUSESetSFXGroupVol(CLIP(_mixer->getVolumeForSoundType(Audio::Mixer::kSFXSoundType) / 2, 0, 127));
	}

	// Handle fades and triggers

	_cmdsRunning60HzCount += usecPerInt;
	while (_cmdsRunning60HzCount >= 16667) {
		_cmdsRunning60HzCount -= 16667;
		_fadesHandler->loop();
		_triggersHandler->loop();
	}

	_cmdsRunning10HzCount += usecPerInt;
	if (_cmdsRunning10HzCount < 100000)
		return;

	do {
		// SPEECH GAIN REDUCTION 10Hz
		_cmdsRunning10HzCount -= 100000;
		soundId = 0;
		musicTargetVolume = _groupsHandler->setGroupVol(DIMUSE_GROUP_MUSIC, -1);
		while (1) { // Check all tracks to see if there's a speech file playing
			soundId = waveGetNextSound(soundId);
			if (!soundId)
				break;

			foundGroupId = -1;
			if (_filesHandler->getNextSound(soundId) == 2) {
				foundGroupId = waveGetParam(soundId, P_GROUP); // Check the groupId of this sound
			}

			if (foundGroupId == DIMUSE_GROUP_SPEECH) {
				// Remember: when a speech file stops playing this block stops 
				// being executed, so musicTargetVolume returns back to its original value
				musicTargetVolume = (musicTargetVolume * 80) / 128;
				break;
			}
		}

		musicEffVol = _groupsHandler->setGroupVol(DIMUSE_GROUP_MUSICEFF, -1); // MUSIC EFFECTIVE VOLUME GROUP (used for gain reduction)
		musicVol = _groupsHandler->setGroupVol(DIMUSE_GROUP_MUSIC, -1); // MUSIC VOLUME SUBGROUP (keeps track of original music volume)

		if (musicEffVol < musicTargetVolume) { // If there is gain reduction already going on...
			tempEffVol = musicEffVol + 3;
			if (tempEffVol < musicTargetVolume) {
				if (musicVol <= tempEffVol) {
					musicVol = tempEffVol;
				}
			} else if (musicVol <= musicTargetVolume) { // Bring up the music volume immediately when speech stops playing
				musicVol = musicTargetVolume;
			}
			_groupsHandler->setGroupVol(DIMUSE_GROUP_MUSICEFF, musicVol);
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
			_groupsHandler->setGroupVol(DIMUSE_GROUP_MUSICEFF, musicVol);
		}
		
	} while (_cmdsRunning10HzCount >= 100000);
}

void DiMUSE_v2::setVolume(int soundId, int volume) {
	diMUSESetParam(soundId, P_VOLUME, volume);
	if (soundId == kTalkSoundID)
		_currentSpeechVolume = volume;
}

void DiMUSE_v2::setPan(int soundId, int pan) {
	diMUSESetParam(soundId, P_PAN, pan);
	if (soundId == kTalkSoundID)
		_currentSpeechPan = pan;
}

void DiMUSE_v2::setFrequency(int soundId, int frequency) {
	diMUSESetParam(soundId, P_TRANSPOSE, frequency);
	if (soundId == kTalkSoundID)
		_currentSpeechFrequency = frequency;
}

int DiMUSE_v2::getCurSpeechVolume() const {
	return _currentSpeechVolume;
}

int DiMUSE_v2::getCurSpeechPan() const {
	return _currentSpeechPan;
}

int DiMUSE_v2::getCurSpeechFrequency() const {
	return _currentSpeechFrequency;
}

void DiMUSE_v2::flushTracks() {
	_filesHandler->flushSounds();
}

int32 DiMUSE_v2::getCurMusicPosInMs() {
	int soundId, curSoundId;

	curSoundId = 0;
	soundId = 0;
	while (1) {
		curSoundId = diMUSEGetNextSound(curSoundId);
		if (!curSoundId)
			break;

		if (diMUSEGetParam(curSoundId, P_SND_HAS_STREAM) && diMUSEGetParam(curSoundId, P_STREAM_BUFID) == 2) {
			soundId = curSoundId;
			return diMUSEGetParam(soundId, P_SND_POS_IN_MS);
		}
	}

	return diMUSEGetParam(soundId, P_SND_POS_IN_MS);
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

	if (diMUSEGetParam(kTalkSoundID, P_SND_TRACK_NUM) > 0) {
		curSpeechPosInMs = diMUSEGetParam(kTalkSoundID, P_SND_POS_IN_MS);
		diMUSELipSync(kTalkSoundID, 0, _vm->VAR(_vm->VAR_SYNC) + curSpeechPosInMs + 50, width, height);
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
		soundId = diMUSEGetNextSound(soundId);
		if (!soundId)
			break;
		if (diMUSEGetParam(soundId, P_SND_HAS_STREAM)) {
			if (diMUSEGetParam(soundId, P_STREAM_BUFID) == DIMUSE_BUFFER_MUSIC) {
				speechSoundId = soundId;
				break;
			}
		}
	}

	if (speechSoundId) {
		curSpeechPosInMs = diMUSEGetParam(speechSoundId, P_SND_POS_IN_MS);
		diMUSELipSync(speechSoundId, syncId, _vm->VAR(_vm->VAR_SYNC) + curSpeechPosInMs + 50, width, height);
	}
}

void DiMUSE_v2::pause(bool p) {
	if (p) {
		debug(5, "DiMUSE_v2::pause(): pausing...");
		diMUSEPause();
	} else {
		debug(5, "DiMUSE_v2::pause(): resuming...");
		diMUSEResume();
	}
}

void DiMUSE_v2::parseScriptCmds(int cmd, int soundId, int sub_cmd, int d, int e, int f, int g, int h, int i, int j, int k, int l, int m, int n, int o, int p) {
	int b = soundId;
	int c = sub_cmd;
	switch (cmd) {
	case 0x1000:
		// SetState
		diMUSESetState(soundId);
		break;
	case 0x1001:
		// SetSequence
		diMUSESetSequence(soundId);
		break;
	case 0x1002:
		// SetCuePoint
		break;
	case 0x1003:
		// SetAttribute
		diMUSESetAttribute(b, c);
		break;
	case 0x2000:
		// SetGroupSfxVolume
		diMUSESetSFXGroupVol(CLIP(_mixer->getVolumeForSoundType(Audio::Mixer::kSFXSoundType) / 2, 0, 127));
		break;
	case 0x2001:
		// SetGroupVoiceVolume
		diMUSESetVoiceGroupVol(CLIP(_mixer->getVolumeForSoundType(Audio::Mixer::kSpeechSoundType) / 2, 0, 127));
		break;
	case 0x2002:
		// SetGroupMusicVolume
		diMUSESetMusicGroupVol(CLIP(_mixer->getVolumeForSoundType(Audio::Mixer::kMusicSoundType) / 2, 0, 127));
		break;
	case 10: // StopAllSounds
	case 12: // SetParam
	case 14: // FadeParam
		cmdsHandleCmd(cmd, soundId, sub_cmd, d, e, f, g, h, i, j, k, l, m, n, o);
		break;
	default:
		error("DiMUSE_v2::parseScriptCmds(): WARNING: unhandled command %d", cmd);
	}
}

int DiMUSE_v2::diMUSETerminate() {
	if (_scriptInitializedFlag) {
		diMUSEStopAllSounds();
		_filesHandler->closeAllSounds();
	}
		
	return 0;
}

int DiMUSE_v2::diMUSEInitialize() {
	return cmdsHandleCmd(0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
}

int DiMUSE_v2::diMUSEPause() {
	return cmdsHandleCmd(3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
}

int DiMUSE_v2::diMUSEResume() {
	return cmdsHandleCmd(4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
}

void DiMUSE_v2::diMUSESaveLoad(Common::Serializer &ser) {
	cmdsSaveLoad(ser);
	return;
}

int DiMUSE_v2::diMUSESetGroupVol(int groupId, int volume) {
	return cmdsHandleCmd(7, groupId, volume, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
}

int DiMUSE_v2::diMUSEStartSound(int soundId, int priority) {
	return cmdsHandleCmd(8, soundId, priority, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
}

int DiMUSE_v2::diMUSEStopSound(int soundId) {
	debug(5, "DiMUSE_v2::diMUSEStopSound(): %d", soundId);
	return cmdsHandleCmd(9, soundId, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
}

int DiMUSE_v2::diMUSEStopAllSounds() {
	debug(5, "DiMUSE_v2::diMUSEStopAllSounds()");
	return cmdsHandleCmd(10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
}

int DiMUSE_v2::diMUSEGetNextSound(int soundId) {
	return cmdsHandleCmd(11, soundId, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
}

int DiMUSE_v2::diMUSESetParam(int soundId, int paramId, int value) {
	return cmdsHandleCmd(12, soundId, paramId, value, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
}

int DiMUSE_v2::diMUSEGetParam(int soundId, int paramId) {
	return cmdsHandleCmd(13, soundId, paramId, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
}

int DiMUSE_v2::diMUSEFadeParam(int soundId, int opcode, int destValue, int fadeLength) {
	return cmdsHandleCmd(14, soundId, opcode, destValue, fadeLength, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
}

int DiMUSE_v2::diMUSESetHook(int soundId, int hookId) {
	return cmdsHandleCmd(15, soundId, hookId, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
}

int DiMUSE_v2::diMUSESetTrigger(int soundId, int marker, int opcode, int d, int e, int f, int g, int h, int i, int j, int k, int l, int m, int n) {
	return cmdsHandleCmd(17, soundId, marker, opcode, d, e, f, g, h, i, j, k, l, m, n);
}

int DiMUSE_v2::diMUSEStartStream(int soundId, int priority, int bufferId) {
 	return cmdsHandleCmd(25, soundId, priority, bufferId, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
}

int DiMUSE_v2::diMUSESwitchStream(int oldSoundId, int newSoundId, int fadeDelay, int fadeSyncFlag2, int fadeSyncFlag1) {
	return cmdsHandleCmd(26, oldSoundId, newSoundId, fadeDelay, fadeSyncFlag2, fadeSyncFlag1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
}

int DiMUSE_v2::diMUSEProcessStreams() {
	return cmdsHandleCmd(27, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
}

int DiMUSE_v2::diMUSEQueryStream(int soundId, int *bufSize, int *criticalSize, int *freeSpace, int *paused) {
	return cmdsHandleCmd(28, soundId, (uintptr)bufSize, (uintptr)criticalSize, (uintptr)freeSpace, (uintptr)paused, -1, -1, -1, -1, -1, -1, -1, -1, -1);
}

int DiMUSE_v2::diMUSEFeedStream(int soundId, uint8 *srcBuf, int sizeToFeed, int paused) {
	return cmdsHandleCmd(29, soundId, (uintptr)srcBuf, sizeToFeed, paused, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
}

int DiMUSE_v2::diMUSELipSync(int soundId, int syncId, int msPos, int32 *width, int32 *height) {
	return cmdsHandleCmd(30, soundId, syncId, msPos, (uintptr)width, (uintptr)height, -1, -1, -1, -1, -1, -1, -1, -1, -1);
}

int DiMUSE_v2::diMUSESetMusicGroupVol(int volume) {
	debug(5, "DiMUSE_v2::diMUSESetMusicGroupVol(): %d", volume);
	return diMUSESetGroupVol(3, volume);
}

int DiMUSE_v2::diMUSESetSFXGroupVol(int volume) {
	debug(5, "DiMUSE_v2::diMUSESetSFXGroupVol(): %d", volume);
	return diMUSESetGroupVol(1, volume);
}

int DiMUSE_v2::diMUSESetVoiceGroupVol(int volume) {
	debug(5, "DiMUSE_v2::diMUSESetVoiceGroupVol(): %d", volume);
	return diMUSESetGroupVol(2, volume);
}

void DiMUSE_v2::diMUSEUpdateGroupVolumes() {
	waveUpdateGroupVolumes();
}

int DiMUSE_v2::diMUSEInitializeScript() {
	return scriptParse(0, -1, -1);
}

void DiMUSE_v2::diMUSERefreshScript() {
	diMUSEProcessStreams();
	scriptParse(4, -1, -1);
}

int DiMUSE_v2::diMUSESetState(int soundId) {
	return scriptParse(5, soundId, -1);
}

int DiMUSE_v2::diMUSESetSequence(int soundId) {
	return scriptParse(6, soundId, -1);
}

int DiMUSE_v2::diMUSESetAttribute(int attrIndex, int attrVal) {
	return scriptParse(8, attrIndex, attrVal);
}

} // End of namespace Scumm

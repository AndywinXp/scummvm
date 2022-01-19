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

#include "common/system.h"
#include "common/timer.h"
#include "common/textconsole.h"

#include "engines/grim/imuse/imuse_engine.h"
#include "engines/grim/imuse/imuse_defs.h"
#include "engines/grim/imuse/imuse_sndmgr.h"
#include "engines/grim/imuse/imuse_tables.h"

#include "engines/grim/savegame.h"
#include "engines/grim/debug.h"
#include "engines/grim/movie/codecs/vima.h"

#include "audio/audiostream.h"
#include "audio/mixer.h"
#include "audio/decoders/raw.h"

namespace Grim {

Imuse *g_imuse = nullptr;

extern uint16 imuseDestTable[];
extern ImuseTable grimStateMusicTable[];
extern ImuseTable grimSeqMusicTable[];
extern ImuseTable grimDemoStateMusicTable[];
extern ImuseTable grimDemoSeqMusicTable[];

void Imuse::timer_handler(void *refCon) {
	Imuse *diMUSE = (Imuse *)refCon;
	diMUSE->callback();
}

Imuse::Imuse(int fps, bool demo, Audio::Mixer *mixer)
	: _demo(demo), _mixer(mixer) {
	assert(mixer);

	// 50 Hz rate for the callback
	_callbackFps = 50;
	_usecPerInt = 20000;

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
	_callbackInterruptFlag = 0;

	_isEngineDisabled = false;

	_demo = demo;
	_pause = false;

	_emptyMarker[0] = '\0';
	_internalMixer = new IMuseDigiInternalMixer(mixer);
	_groupsHandler = new IMuseDigiGroupsHandler(this);
	_fadesHandler = new IMuseDigiFadesHandler(this);
	_triggersHandler = new IMuseDigiTriggersHandler(this);
	_filesHandler = new IMuseDigiFilesHandler(this, _demo);

	diMUSEInitialize();
	diMUSEInitializeScript();

	_filesHandler->allocSoundBuffer(DIMUSE_BUFFER_SPEECH, 110000, 22000, 44000);
	_filesHandler->allocSoundBuffer(DIMUSE_BUFFER_MUSIC, 220000, 22000, 44000);
	_filesHandler->allocSoundBuffer(DIMUSE_BUFFER_SMUSH, 198000, 0, 0);

	if (_mixer->getOutputBufSize() != 0) {
		_maxQueuedStreams = (_mixer->getOutputBufSize() / _waveOutPreferredFeedSize) + 1;
		// This mixer's optimal output sample rate for this audio engine is 44100
		// if it's higher than that, compensate the number of queued streams in order
		// to try to achieve a better latency compromise
		// TODO: Fix Remastered
		if (_mixer->getOutputRate() > DIMUSE_SAMPLERATE * 2) {
			_maxQueuedStreams -= 2;
		}
	} else {
		Debug::debug(Debug::Sound, "Imuse::Imuse(): WARNING: output audio buffer size not specified for this platform, defaulting _maxQueuedStreams to 4");
		_maxQueuedStreams = 4;
	}

	vimaInit(imuseDestTable);
	if (_demo) {
		_stateMusicTable = grimDemoStateMusicTable;
		_seqMusicTable = grimDemoSeqMusicTable;
	} else {
		_stateMusicTable = grimStateMusicTable;
		_seqMusicTable = grimSeqMusicTable;
	}

	g_system->getTimerManager()->installTimerProc(timer_handler, 1000000 / _callbackFps, this, "iMUSECallback");
}

Imuse::~Imuse() {
	g_system->getTimerManager()->removeTimerProc(timer_handler);
	_filesHandler->deallocSoundBuffer(DIMUSE_BUFFER_SPEECH);
	_filesHandler->deallocSoundBuffer(DIMUSE_BUFFER_MUSIC);
	_filesHandler->deallocSoundBuffer(DIMUSE_BUFFER_SMUSH);
	cmdsDeinit();
	diMUSETerminate();
	delete _internalMixer;
	delete _groupsHandler;
	delete _fadesHandler;
	delete _triggersHandler;
	delete _filesHandler;

	// Deinit the Dispatch module
	free(_dispatchBuffer);
	_dispatchBuffer = nullptr;

	// Deinit the WaveOut module
	free(_waveOutOutputBuffer);
	_waveOutOutputBuffer = nullptr;
}

int Imuse::isSoundRunning(int soundId) {
	return diMUSEGetParam(soundId, DIMUSE_P_SND_TRACK_NUM) > 0;
}

int Imuse::startVoice(int soundId, const char *soundName, byte speakingActorId) {
//	_filesHandler->closeSoundImmediatelyById(soundId);
//
//	int fileDoesNotExist = 0;
//	if (_vm->_game.id == GID_DIG) {
//		if (!strcmp(soundName, "PIG.018"))
//			fileDoesNotExist = _filesHandler->setCurrentSpeechFilename("PIG.019");
//		else
//			fileDoesNotExist = _filesHandler->setCurrentSpeechFilename(soundName);
//
//		if (fileDoesNotExist)
//			return 1;
//
//		// Workaround for this particular sound file not playing (this is a bug in the original):
//		// this is happening because the sound buffer responsible for speech
//		// is still busy with the previous speech file playing during the SAN
//		// movie. We just stop the SMUSH speech sound before playing NEXUS.029.
//		if (!strcmp(soundName, "NEXUS.029")) {
//			diMUSEStopSound(DIMUSE_SMUSH_SOUNDID + DIMUSE_BUFFER_SPEECH);
//		}
//
//		diMUSEStartStream(kTalkSoundID, 127, DIMUSE_BUFFER_SPEECH);
//		diMUSESetParam(kTalkSoundID, DIMUSE_P_GROUP, DIMUSE_GROUP_SPEECH);
//		if (speakingActorId == _vm->VAR(_vm->VAR_EGO)) {
//			diMUSESetParam(kTalkSoundID, DIMUSE_P_MAILBOX, 0);
//			diMUSESetParam(kTalkSoundID, DIMUSE_P_VOLUME, 127);
//		} else {
//			diMUSESetParam(kTalkSoundID, DIMUSE_P_MAILBOX, _radioChatterSFX);
//			diMUSESetParam(kTalkSoundID, DIMUSE_P_VOLUME, 88);
//		}
//		_filesHandler->closeSound(kTalkSoundID);
//	} else if (_vm->_game.id == GID_CMI) {
//		fileDoesNotExist = _filesHandler->setCurrentSpeechFilename(soundName);
//		if (fileDoesNotExist)
//			return 1;
//
//		diMUSEStartStream(kTalkSoundID, 127, DIMUSE_BUFFER_SPEECH);
//		diMUSESetParam(kTalkSoundID, DIMUSE_P_GROUP, DIMUSE_GROUP_SPEECH);
//
//		// Let's not give the occasion to raise errors here
//		if (_vm->isValidActor(_vm->VAR(_vm->VAR_TALK_ACTOR))) {
//			Actor *a = _vm->derefActor(_vm->VAR(_vm->VAR_TALK_ACTOR), "Imuse::startVoice");
//			if (_vm->VAR(_vm->VAR_VOICE_MODE) == 2)
//				diMUSESetParam(kTalkSoundID, DIMUSE_P_VOLUME, 0);
//			else
//				diMUSESetParam(kTalkSoundID, DIMUSE_P_VOLUME, a->_talkVolume);
//
//			diMUSESetParam(kTalkSoundID, DIMUSE_P_TRANSPOSE, a->_talkFrequency);
//			diMUSESetParam(kTalkSoundID, DIMUSE_P_PAN, a->_talkPan);
//
//			_currentSpeechVolume = a->_talkVolume;
//			_currentSpeechFrequency = a->_talkFrequency;
//			_currentSpeechPan = a->_talkPan;
//		}
//
//		// The interpreter really calls for processStreams two times in a row,
//		// and who am I to contradict it?
//		diMUSEProcessStreams();
//		diMUSEProcessStreams();
//	}
//
	return 0;
}

void Imuse::resetState() {
	_curMusicState = 0;
	_curMusicSeq = 0;
	memset(_attributes, 0, sizeof(_attributes));
}

void Imuse::restoreState(SaveGame *savedState) {
}

void Imuse::saveState(SaveGame *savedState) {
}

//_savedState->saveMinorVersion() < 28 allora cose vecchie
//void Imuse::saveLoadEarly(Common::Serializer &s) {
//	Common::StackLock lock(_mutex, "Imuse::saveLoadEarly()");
//
//	if (s.isLoading()) {
//		diMUSEStopAllSounds();
//		_filesHandler->closeSoundImmediatelyById(kTalkSoundID);
//	}
//
//	if (s.getVersion() < 103) {
//		// Just load the current state and sequence, and play them
//		Debug::debug(Debug::Sound, "Imuse::saveLoadEarly(): old savegame detected (version %d), game may load with an undesired audio status", s.getVersion());
//		s.skip(4, VER(31), VER(42)); // _volVoice
//		s.skip(4, VER(31), VER(42)); // _volSfx
//		s.skip(4, VER(31), VER(42)); // _volMusic
//		s.syncAsSint32LE(_curMusicState, VER(31));
//		s.syncAsSint32LE(_curMusicSeq, VER(31));
//		s.syncAsSint32LE(_curMusicCue, VER(31));
//		s.syncAsSint32LE(_nextSeqToPlay, VER(31));
//		s.syncAsByte(_radioChatterSFX, VER(76));
//		s.syncArray(_attributes, 188, Common::Serializer::Sint32LE, VER(31));
//
//		int stateSoundId = 0;
//		int seqSoundId = 0;
//
//		// TODO: Check if FT actually works here
//		if (_vm->_game.id == GID_DIG) {
//			stateSoundId = _digStateMusicTable[_curMusicState].soundId;
//			seqSoundId = _digSeqMusicTable[_curMusicSeq].soundId;
//		} else {
//			if (_vm->_game.features & GF_DEMO) {
//				stateSoundId = _comiDemoStateMusicTable[_curMusicState].soundId;
//			} else {
//				stateSoundId = _comiStateMusicTable[_curMusicState].soundId;
//				seqSoundId = _comiSeqMusicTable[_curMusicSeq].soundId;
//			}
//		}
//
//		_curMusicState = 0;
//		_curMusicSeq = 0;
//		scriptSetSequence(seqSoundId);
//		scriptSetState(stateSoundId);
//		scriptSetCuePoint(_curMusicCue);
//		_curMusicCue = 0;
//	} else {
//		diMUSESaveLoad(s);
//	}
//}

void Imuse::refreshScripts() {
	// TODO: if smush is not active
	diMUSEProcessStreams();
	diMUSERefreshScript();
}

int Imuse::startSfx(int soundId, int priority) {
	diMUSEStartSound(soundId, priority);
	diMUSESetParam(soundId, DIMUSE_P_GROUP, DIMUSE_GROUP_SFX);
	return 0;
}

void Imuse::callback() {
	if (_cmdsPauseCount)
		return;

	if (!_callbackInterruptFlag) {
		_callbackInterruptFlag = 1;
		diMUSEHeartbeat();
		_callbackInterruptFlag = 0;
	}
}

void Imuse::diMUSEHeartbeat() {
	// This is what happens:
	// - Usual audio stuff like fetching and playing sound (and everything
	//   within waveOutCallback()) happens at a 50Hz rate;
	// - Triggers and fades handling also happens at a 50Hz rate;
	// - Music gain reduction happens at a 5Hz rate.

	int soundId, foundGroupId, musicTargetVolume, musicEffVol, musicVol, tempVol, tempEffVol;

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

	bool ready = _cmdsRunning50HzCount < -_usecPerInt;
	_cmdsRunning50HzCount += _usecPerInt;
	if (ready) {
		do {
			_cmdsRunning50HzCount -= 20000;
			_fadesHandler->loop();
			_triggersHandler->loop();
		} while (_cmdsRunning50HzCount >= 20000);
	}

	_cmdsRunning5HzCount += _usecPerInt;
	if (_cmdsRunning5HzCount < 200000)
		return;

	do {
		// SPEECH GAIN REDUCTION 5Hz
		_cmdsRunning5HzCount -= 200000;
		soundId = 0;
		musicTargetVolume = _groupsHandler->setGroupVol(DIMUSE_GROUP_MUSIC, -1);
		while (1) { // Check all tracks to see if there's a speech file playing
			soundId = waveGetNextSound(soundId);
			if (!soundId)
				break;

			foundGroupId = -1;
			if (_filesHandler->getNextSound(soundId) == 2) {
				foundGroupId = waveGetParam(soundId, DIMUSE_P_GROUP); // Check the groupId of this sound
			}

			if (foundGroupId == DIMUSE_GROUP_SPEECH) {
				// Remember: when a speech file stops playing this block stops
				// being executed, so musicTargetVolume returns back to its original value
				musicTargetVolume = (musicTargetVolume * 77) / 128;
				break;
			}
		}

		musicEffVol = _groupsHandler->setGroupVol(DIMUSE_GROUP_MUSICEFF, -1); // MUSIC EFFECTIVE VOLUME GROUP (used for gain reduction)
		musicVol = _groupsHandler->setGroupVol(DIMUSE_GROUP_MUSIC, -1); // MUSIC VOLUME SUBGROUP (keeps track of original music volume)

		if (musicEffVol < musicTargetVolume) { // If there is gain reduction already going on...
			tempEffVol = musicEffVol + 1;
			if (tempEffVol < musicTargetVolume) {
				if (musicVol < tempEffVol) {
					musicVol = tempEffVol;
				}
			} else if (musicVol < musicTargetVolume) { // Bring up the music volume immediately when speech stops playing
				musicVol = musicTargetVolume;
			}
			_groupsHandler->setGroupVol(DIMUSE_GROUP_MUSICEFF, musicVol);
		} else if (musicEffVol > musicTargetVolume) {
			// Bring music volume down to target volume with a -18 (or -6 for FT & DIG demo) step
		    // if there's speech playing or else, just cap it to the target if it's out of range
			tempVol = musicEffVol - 36;
			if (tempVol <= musicTargetVolume) {
				if (musicVol > musicTargetVolume) {
					musicVol = musicTargetVolume;
				}
			} else {
				if (musicVol > tempVol) {
					musicVol = tempVol;
				}
			}
			_groupsHandler->setGroupVol(DIMUSE_GROUP_MUSICEFF, musicVol);
		}

	} while (_cmdsRunning5HzCount >= 200000);
}

void Imuse::setPriority(int soundId, int priority) {
	diMUSESetParam(soundId, DIMUSE_P_PRIORITY, priority);
}

void Imuse::setVolume(int soundId, int volume) {
	diMUSESetParam(soundId, DIMUSE_P_VOLUME, volume);
	if (soundId == kTalkSoundID)
		_currentSpeechVolume = volume;
}

void Imuse::setPan(int soundId, int pan) {
	diMUSESetParam(soundId, DIMUSE_P_PAN, pan);
	if (soundId == kTalkSoundID)
		_currentSpeechPan = pan;
}

void Imuse::setFrequency(int soundId, int frequency) {
	diMUSESetParam(soundId, DIMUSE_P_TRANSPOSE, frequency);
	if (soundId == kTalkSoundID)
		_currentSpeechFrequency = frequency;
}

int Imuse::getCurSpeechVolume() const {
	return _currentSpeechVolume;
}

int Imuse::getCurSpeechPan() const {
	return _currentSpeechPan;
}

int Imuse::getCurSpeechFrequency() const {
	return _currentSpeechFrequency;
}

void Imuse::flushTracks() {
	_filesHandler->flushSounds();
}

// This is used in order to avoid crash everything
// if a compressed audio resource file is found
void Imuse::disableEngine() {
	_isEngineDisabled = true;
}

bool Imuse::isEngineDisabled() {
	return _isEngineDisabled;
}

void Imuse::stopSMUSHAudio() {
	//if (!isFTSoundEngine()) {
	//	if (_vm->_game.id == GID_DIG) {
	//		int foundSoundId, paused;
	//		int32 bufSize, criticalSize, freeSpace;
	//		foundSoundId = diMUSEGetNextSound(0);
	//		while (foundSoundId) {
	//			if (diMUSEGetParam(foundSoundId, DIMUSE_P_SND_HAS_STREAM)) {
	//				diMUSEQueryStream(foundSoundId, bufSize, criticalSize, freeSpace, paused);
	//
	//				// Here, the original engine for DIG explicitly asks for "bufSize == 193900";
	//				// since this works half of the time in ScummVM (because of how we handle sound buffers),
	//				// we do the check but we alternatively check for the SMUSH channel soundId, so to cover the
	//				// remaining cases. This fixes instances in which exiting from a cutscene leaves both
	//				// DiMUSE streams locked, with speech consequently unable to play and a "WARNING: three
	//				// streams in use" message from streamerProcessStreams()
	//				if (bufSize == 193900 || foundSoundId == DIMUSE_SMUSH_SOUNDID + DIMUSE_BUFFER_SMUSH)
	//					diMUSEStopSound(foundSoundId);
	//			}
	//
	//			foundSoundId = diMUSEGetNextSound(foundSoundId);
	//		}
	//	}
	//
	//	diMUSESetSequence(0);
	//}
}

int32 Imuse::getCurMusicPosInMs() {
	int soundId, curSoundId;

	curSoundId = 0;
	soundId = 0;
	while (1) {
		curSoundId = diMUSEGetNextSound(curSoundId);
		if (!curSoundId)
			break;

		if (diMUSEGetParam(curSoundId, DIMUSE_P_SND_HAS_STREAM) && diMUSEGetParam(curSoundId, DIMUSE_P_STREAM_BUFID) == DIMUSE_BUFFER_MUSIC) {
			soundId = curSoundId;
			return diMUSEGetParam(soundId, DIMUSE_P_SND_POS_IN_MS);
		}
	}

	return diMUSEGetParam(soundId, DIMUSE_P_SND_POS_IN_MS);
}

int32 Imuse::getCurVoiceLipSyncWidth() {
	int32 width, height;
	getSpeechLipSyncInfo(width, height);
	return width;
}

int32 Imuse::getCurVoiceLipSyncHeight() {
	int32 width, height;
	getSpeechLipSyncInfo(width, height);
	return height;
}

int32 Imuse::getCurMusicLipSyncWidth(int syncId) {
	int32 width, height;
	getMusicLipSyncInfo(syncId, width, height);
	return width;
}

int32 Imuse::getCurMusicLipSyncHeight(int syncId) {
	int32 width, height;
	getMusicLipSyncInfo(syncId, width, height);
	return height;
}

void Imuse::getSpeechLipSyncInfo(int32 &width, int32 &height) {
	int curSpeechPosInMs;

	width = 0;
	height = 0;

	if (diMUSEGetParam(kTalkSoundID, DIMUSE_P_SND_TRACK_NUM) > 0) {
		curSpeechPosInMs = diMUSEGetParam(kTalkSoundID, DIMUSE_P_SND_POS_IN_MS);
		//diMUSELipSync(kTalkSoundID, 0, _vm->VAR(_vm->VAR_SYNC) + curSpeechPosInMs + 50, width, height);
	}
}

void Imuse::getMusicLipSyncInfo(int syncId, int32 &width, int32 &height) {
	int soundId;
	int speechSoundId;
	int curSpeechPosInMs;

	soundId = 0;
	speechSoundId = 0;
	width = 0;
	height = 0;
	while (1) {
		soundId = diMUSEGetNextSound(soundId);
		if (!soundId)
			break;
		if (diMUSEGetParam(soundId, DIMUSE_P_SND_HAS_STREAM)) {
			if (diMUSEGetParam(soundId, DIMUSE_P_STREAM_BUFID) == DIMUSE_BUFFER_MUSIC) {
				speechSoundId = soundId;
				break;
			}
		}
	}

	if (speechSoundId) {
		curSpeechPosInMs = diMUSEGetParam(speechSoundId, DIMUSE_P_SND_POS_IN_MS);
		//diMUSELipSync(speechSoundId, syncId, _vm->VAR(_vm->VAR_SYNC) + curSpeechPosInMs + 50, width, height);
	}
}

int32 Imuse::getSoundElapsedTimeInMs(int soundId) {
	if (diMUSEGetParam(soundId, DIMUSE_P_SND_HAS_STREAM)) {
		return diMUSEGetParam(soundId, DIMUSE_P_SND_POS_IN_MS);
	}
	return 0;
}

void Imuse::pause(bool p) {
	if (p) {
		Debug::debug(Debug::Sound, "Imuse::pause(): pausing...");
		diMUSEPause();
	} else {
		Debug::debug(Debug::Sound, "Imuse::pause(): resuming...");
		diMUSEResume();
	}
}

void Imuse::setAudioNames(int32 num, char *names) {
	//free(_audioNames);
	//_numAudioNames = num;
	//_audioNames = names;
}

int Imuse::getSoundIdByName(const char *soundName) {
	//if (soundName && soundName[0] != 0) {
	//	for (int r = 0; r < _numAudioNames; r++) {
	//		if (strcmp(soundName, &_audioNames[r * 9]) == 0) {
	//			return r;
	//		}
	//	}
	//}

	return 0;
}

uint8 *Imuse::getExtFormatBuffer(uint8 *srcBuf, int &formatId, int &sampleRate, int &wordSize, int &nChans, int &swapFlag, int32 &audioLength) {
	// This function is used to extract format metadata for sounds in WAV format (speech and sfx)

	int isMCMP;
	uint8 *bufPtr;
	int v10;
	int v11;
	const char *v12;
	unsigned int v13;
	unsigned int v14;
	uint8 *v15;
	unsigned int v16;
	const char *v17;
	unsigned int v18;
	unsigned int v19;
	unsigned int v20;
	uint8 *v21;
	unsigned int v22;
	char v23;
	int v24;
	uint8 *streamBuffera;

	if (!bufPtr)
		return nullptr;

	isMCMP = 0;
	formatId = 0;
	audioLength = 0;
	bufPtr = srcBuf;

	if (READ_BE_UINT32(srcBuf) == MKTAG('M', 'C', 'M', 'P')) {
		//v10 = HIBYTE(*((unsigned __int16 *)srcBuf + 2)) | ((unsigned __int8)*((_WORD *)srcBuf + 2) << 8);
		//v8 = &srcBuf[8 * v10 + 8 + v10 + (HIBYTE(*(unsigned __int16 *)&srcBuf[8 * v10 + 6 + v10]) | ((unsigned __int8)*(_WORD *)&srcBuf[8 * v10 + 6 + v10] << 8))];
		// TODO: Advance ptr
		uint16 numCompItems = READ_BE_UINT16(srcBuf + 4);
		isMCMP = 1;
		error("Imuse::getExtFormatBuffer(): Got MCMP tag!");
	}

	// Follow the WAVE PCM header format
	if (READ_BE_UINT32(bufPtr)      == MKTAG('R', 'I', 'F', 'F') &&
		READ_BE_UINT32(bufPtr + 8)  == MKTAG('W', 'A', 'V', 'E') &&
		READ_BE_UINT32(bufPtr + 12) == MKTAG('f', 'm', 't', ' ')) {

		if (READ_LE_UINT32(bufPtr + 20) == 1) {
			formatId = (isMCMP != 0) + 5;
			nChans = READ_LE_UINT32(bufPtr + 22);
			sampleRate = READ_LE_UINT32(bufPtr + 24);
			wordSize = READ_LE_UINT32(bufPtr + 34);
			swapFlag = 1;

			if (READ_BE_UINT32(bufPtr + 36) == MKTAG('d', 'a', 't', 'a')) {
				audioLength = READ_BE_UINT32(bufPtr + 40);
				return bufPtr + 44;
			}
		}
		return nullptr;
	}

	// Follow the iMUSE MAP format block specification
	if (READ_BE_UINT32(bufPtr) == MKTAG('i', 'M', 'U', 'S')) {
		if (READ_BE_UINT32(bufPtr + 16) == MKTAG('F', 'R', 'M', 'T')) {
			formatId = (isMCMP != 0) + 3;
			wordSize = READ_LE_UINT32(bufPtr + 8);
			sampleRate = READ_LE_UINT32(bufPtr + 9);
			nChans = READ_LE_UINT32(bufPtr + 10);
			swapFlag = 0;
			audioLength = READ_LE_UINT32(bufPtr + 20);
			return bufPtr + 24;
		}
		return nullptr;
	}

	if (isMCMP)
		return nullptr;

	return bufPtr;
}

void Imuse::createSoundHeader(int32 *fakeWavHeader, int sampleRate, int wordSize, int nChans, int32 audioLength) {
	// This function is used to create an iMUSE map for sounds in WAV format (speech and sfx)

	fakeWavHeader[0] =  TO_LE_32(MKTAG('i', 'M', 'U', 'S'));
	fakeWavHeader[1] =  TO_LE_32(audioLength + 80);            // Map size
	fakeWavHeader[2] =  TO_LE_32(MKTAG('M', 'A', 'P', ' '));
	fakeWavHeader[3] =  0x38000000;                            // Block offset
	fakeWavHeader[4] =  TO_LE_32(MKTAG('F', 'R', 'M', 'T'));
	fakeWavHeader[5] =  0x14000000;                            // Block size minus 8, 20 bytes
	fakeWavHeader[6] =  0x50000000;                            // Block offset: 80 bytes
	fakeWavHeader[7] =  0;                                     // Endianness
	fakeWavHeader[8] =  TO_LE_32(wordSize);
	fakeWavHeader[9] =  TO_LE_32(sampleRate);
	fakeWavHeader[10] = TO_LE_32(nChans);
	fakeWavHeader[11] = TO_LE_32(MKTAG('R', 'E', 'G', 'N'));
	fakeWavHeader[12] = 0x08000000;                            // Block size minus 8: 8 bytes
	fakeWavHeader[13] = 0x50000000;                            // Block offset: 80 bytes
	fakeWavHeader[14] = TO_LE_32(audioLength);
	fakeWavHeader[15] = TO_LE_32(MKTAG('S', 'T', 'O', 'P'));
	fakeWavHeader[16] = 0x04000000;                            // Block size minus 8: 4 bytes
	fakeWavHeader[17] = TO_LE_32(audioLength + 80);
	fakeWavHeader[18] = TO_LE_32(MKTAG('D', 'A', 'T', 'A'));
	fakeWavHeader[19] = TO_LE_32(audioLength);                 // Data block size
}

//void Imuse::parseScriptCmds(int cmd, int soundId, int sub_cmd, int d, int e, int f, int g, int h, int i, int j, int k, int l, int m, int n, int o, int p) {
//	int b = soundId;
//	int c = sub_cmd;
//	int id;
//	switch (cmd) {
//	case 0x1000:
//		// SetState
//		diMUSESetState(soundId);
//		break;
//	case 0x1001:
//		// SetSequence
//		diMUSESetSequence(soundId);
//		break;
//	case 0x1002:
//		// SetCuePoint
//		diMUSESetCuePoint(soundId);
//		break;
//	case 0x1003:
//		// SetAttribute
//		diMUSESetAttribute(b, c);
//		break;
//	case 0x2000:
//		// SetGroupSfxVolume
//		diMUSESetSFXGroupVol(CLIP(_mixer->getVolumeForSoundType(Audio::Mixer::kSFXSoundType) / 2, 0, 127));
//		break;
//	case 0x2001:
//		// SetGroupVoiceVolume
//		diMUSESetVoiceGroupVol(CLIP(_mixer->getVolumeForSoundType(Audio::Mixer::kSpeechSoundType) / 2, 0, 127));
//		break;
//	case 0x2002:
//		// SetGroupMusicVolume
//		diMUSESetMusicGroupVol(CLIP(_mixer->getVolumeForSoundType(Audio::Mixer::kMusicSoundType) / 2, 0, 127));
//		break;
//	case 10: // StopAllSounds
//	case 12: // SetParam
//	case 14: // FadeParam
//		cmdsHandleCmd(cmd, nullptr, soundId, sub_cmd, d, e, f, g, h, i, j, k, l, m, n, o);
//		break;
//	default:
//		debug("Imuse::parseScriptCmds(): WARNING: unhandled command %d", cmd);
//	}
//}

int Imuse::diMUSETerminate() {
	if (_scriptInitializedFlag) {
		diMUSEStopAllSounds();
		_filesHandler->closeAllSounds();
	}

	return 0;
}

int Imuse::diMUSEInitialize() {
	return cmdsHandleCmd(0);
}

int Imuse::diMUSEPause() {
	return cmdsHandleCmd(3);
}

int Imuse::diMUSEResume() {
	return cmdsHandleCmd(4);
}

void Imuse::diMUSESaveLoad(Common::Serializer &ser) {
//	cmdsSaveLoad(ser);
}

int Imuse::diMUSESetGroupVol(int groupId, int volume) {
	return cmdsHandleCmd(7, nullptr, groupId, volume);
}

int Imuse::diMUSEStartSound(int soundId, int priority) {
	return cmdsHandleCmd(8, nullptr, soundId, priority);
}

int Imuse::diMUSEStopSound(int soundId) {
	Debug::debug(Debug::Sound, "Imuse::diMUSEStopSound(): %d", soundId);
	return cmdsHandleCmd(9, nullptr, soundId);
}

int Imuse::diMUSEStopAllSounds() {
	Debug::debug(Debug::Sound, "Imuse::diMUSEStopAllSounds()");
	return cmdsHandleCmd(10);
}

int Imuse::diMUSEGetNextSound(int soundId) {
	return cmdsHandleCmd(11, nullptr, soundId);
}

int Imuse::diMUSESetParam(int soundId, int paramId, int value) {
	return cmdsHandleCmd(12, nullptr, soundId, paramId, value);
}

int Imuse::diMUSEGetParam(int soundId, int paramId) {
	return cmdsHandleCmd(13, nullptr, soundId, paramId);
}

int Imuse::diMUSEFadeParam(int soundId, int transitionType, int destValue, int fadeLength) {
	return cmdsHandleCmd(14, nullptr, soundId, transitionType, destValue, fadeLength);
}

int Imuse::diMUSESetHook(int soundId, int hookId) {
	return cmdsHandleCmd(15, nullptr, soundId, hookId);
}

int Imuse::diMUSESetTrigger(int soundId, int marker, int transitionType, int d, int e, int f, int g, int h, int i, int j, int k, int l, int m, int n) {
	return cmdsHandleCmd(17, nullptr, soundId, marker, transitionType, d, e, f, g, h, i, j, k, l, m, n);
}

int Imuse::diMUSEStartStream(int soundId, int priority, int bufferId) {
	return cmdsHandleCmd(25, nullptr, soundId, priority, bufferId);
}

int Imuse::diMUSESwitchStream(int oldSoundId, int newSoundId, int fadeDelay, int fadeSyncFlag2, int fadeSyncFlag1) {
	return cmdsHandleCmd(26, nullptr, oldSoundId, newSoundId, fadeDelay, fadeSyncFlag2, fadeSyncFlag1);
}

int Imuse::diMUSEProcessStreams() {
	return cmdsHandleCmd(27);
}

void Imuse::diMUSEQueryStream(int soundId, int32 &bufSize, int32 &criticalSize, int32 &freeSpace, int &paused) {
	waveQueryStream(soundId, bufSize, criticalSize, freeSpace, paused);
}

int Imuse::diMUSEFeedStream(int soundId, uint8 *srcBuf, int32 sizeToFeed, int paused) {
	return cmdsHandleCmd(29, srcBuf, soundId, -1, sizeToFeed, paused);
}

int Imuse::diMUSESetMusicGroupVol(int volume) {
	Debug::debug(Debug::Sound, "Imuse::diMUSESetMusicGroupVol(): %d", volume);
	return diMUSESetGroupVol(3, volume);
}

int Imuse::diMUSESetSFXGroupVol(int volume) {
	Debug::debug(Debug::Sound, "Imuse::diMUSESetSFXGroupVol(): %d", volume);
	return diMUSESetGroupVol(1, volume);
}

int Imuse::diMUSESetVoiceGroupVol(int volume) {
	Debug::debug(Debug::Sound, "Imuse::diMUSESetVoiceGroupVol(): %d", volume);
	return diMUSESetGroupVol(2, volume);
}

void Imuse::diMUSEUpdateGroupVolumes() {
	waveUpdateGroupVolumes();
}

int Imuse::diMUSEInitializeScript() {
	return scriptParse(0, -1, -1);
}

void Imuse::diMUSERefreshScript() {
	scriptParse(4, -1, -1);
}

int Imuse::diMUSESetState(int soundId) {
	return scriptParse(5, soundId, -1);
}

int Imuse::diMUSESetSequence(int soundId) {
	return scriptParse(6, soundId, -1);
}

int Imuse::diMUSESetCuePoint(int cueId) {
	return scriptParse(7, cueId, -1);
}

int Imuse::diMUSESetAttribute(int attrIndex, int attrVal) {
	return scriptParse(8, attrIndex, attrVal);
}

// Debugger utility functions

void Imuse::listStates() {
//	_vm->getDebugger()->debugPrintf("+---------------------------------+\n");
//	_vm->getDebugger()->debugPrintf("| stateId |         name          |\n");
//	_vm->getDebugger()->debugPrintf("+---------+-----------------------+\n");
//	if (_vm->_game.id == GID_CMI) {
//		if (_vm->_game.features & GF_DEMO) {
//			for (int i = 0; _comiDemoStateMusicTable[i].soundId != -1; i++) {
//				_vm->getDebugger()->debugPrintf("|  %4d   | %20s  |\n", _comiDemoStateMusicTable[i].soundId, _comiDemoStateMusicTable[i].name);
//			}
//		} else {
//			for (int i = 0; _comiStateMusicTable[i].soundId != -1; i++) {
//				_vm->getDebugger()->debugPrintf("|  %4d   | %20s  |\n", _comiStateMusicTable[i].soundId, _comiStateMusicTable[i].name);
//			}
//		}
//	} else if (_vm->_game.id == GID_DIG) {
//		for (int i = 0; _digStateMusicTable[i].soundId != -1; i++) {
//			_vm->getDebugger()->debugPrintf("|  %4d   | %20s  |\n", _digStateMusicTable[i].soundId, _digStateMusicTable[i].name);
//		}
//	} else if (_vm->_game.id == GID_FT) {
//		for (int i = 0; _ftStateMusicTable[i].name[0]; i++) {
//			_vm->getDebugger()->debugPrintf("|  %4d   | %21s |\n", i, _ftStateMusicTable[i].name);
//		}
//	}
//	_vm->getDebugger()->debugPrintf("+---------+-----------------------+\n\n");
}

void Imuse::listSeqs() {
//	_vm->getDebugger()->debugPrintf("+--------------------------------+\n");
//	_vm->getDebugger()->debugPrintf("|  seqId  |         name         |\n");
//	_vm->getDebugger()->debugPrintf("+---------+----------------------+\n");
//	if (_vm->_game.id == GID_CMI) {
//		for (int i = 0; _comiSeqMusicTable[i].soundId != -1; i++) {
//			_vm->getDebugger()->debugPrintf("|  %4d   | %20s |\n", _comiSeqMusicTable[i].soundId, _comiSeqMusicTable[i].name);
//		}
//	} else if (_vm->_game.id == GID_DIG) {
//		for (int i = 0; _digSeqMusicTable[i].soundId != -1; i++) {
//			_vm->getDebugger()->debugPrintf("|  %4d   | %20s |\n", _digSeqMusicTable[i].soundId, _digSeqMusicTable[i].name);
//		}
//	} else if (_vm->_game.id == GID_FT) {
//		for (int i = 0; _ftSeqNames[i].name[0]; i++) {
//			_vm->getDebugger()->debugPrintf("|  %4d   | %20s |\n", i, _ftSeqNames[i].name);
//		}
//	}
//	_vm->getDebugger()->debugPrintf("+---------+----------------------+\n\n");
}

void Imuse::listCues() {
//	int curId = -1;
//	if (_curMusicSeq) {
//		_vm->getDebugger()->debugPrintf("Available cues for current sequence:\n");
//		_vm->getDebugger()->debugPrintf("+---------------------------------------+\n");
//		_vm->getDebugger()->debugPrintf("|   cueName   | transitionType | volume |\n");
//		_vm->getDebugger()->debugPrintf("+-------------+----------------+--------+\n");
//		for (int i = 0; i < 4; i++) {
//			curId = ((_curMusicSeq - 1) * 4) + i;
//			_vm->getDebugger()->debugPrintf("|  %9s  |        %d       |  %3d   |\n",
//				_ftSeqMusicTable[curId].audioName, (int)_ftSeqMusicTable[curId].transitionType, (int)_ftSeqMusicTable[curId].volume);
//		}
//		_vm->getDebugger()->debugPrintf("+-------------+----------------+--------+\n\n");
//	} else {
//		_vm->getDebugger()->debugPrintf("Current sequence is NULL, no cues available.\n\n");
//	}
}

void Imuse::listTracks() {
//	_vm->getDebugger()->debugPrintf("Virtual audio tracks currently playing:\n");
//	_vm->getDebugger()->debugPrintf("+-------------------------------------------------------------------------+\n");
//	_vm->getDebugger()->debugPrintf("| # | soundId | group | hasStream | vol/effVol/pan  | priority | jumpHook |\n");
//	_vm->getDebugger()->debugPrintf("+---+---------+-------+-----------+-----------------+----------+----------+\n");
//
//	for (int i = 0; i < _trackCount; i++) {
//		IMuseDigiTrack curTrack = _tracks[i];
//		if (curTrack.soundId != 0) {
//			_vm->getDebugger()->debugPrintf("| %1d |  %5d  |   %d   |     %d     |   %3d/%3d/%3d   |   %3d    |   %3d    |\n",
//				i, curTrack.soundId, curTrack.group, diMUSEGetParam(curTrack.soundId, DIMUSE_P_SND_HAS_STREAM),
//				curTrack.vol, curTrack.effVol, curTrack.pan, curTrack.priority, curTrack.jumpHook);
//		} else {
//			_vm->getDebugger()->debugPrintf("| %1d |   ---   |  ---  |    ---    |   ---/---/---   |   ---    |   ---    |\n", i);
//		}
//	}
//	_vm->getDebugger()->debugPrintf("+---+---------+-------+-----------+-----------------+----------+----------+\n\n");
}

void Imuse::listGroups() {
//	_vm->getDebugger()->debugPrintf("Volume groups:\n");
//	_vm->getDebugger()->debugPrintf("\tSFX:      %3d\n", _groupsHandler->getGroupVol(DIMUSE_GROUP_SFX));
//	_vm->getDebugger()->debugPrintf("\tSPEECH:   %3d\n", _groupsHandler->getGroupVol(DIMUSE_GROUP_SPEECH));
//	_vm->getDebugger()->debugPrintf("\tMUSIC:    %3d\n", _groupsHandler->getGroupVol(DIMUSE_GROUP_MUSIC));
//	_vm->getDebugger()->debugPrintf("\tMUSICEFF: %3d\n\n", _groupsHandler->getGroupVol(DIMUSE_GROUP_MUSICEFF));
}

} // End of namespace Grim

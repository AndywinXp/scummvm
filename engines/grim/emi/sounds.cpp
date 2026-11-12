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

#include <math.h>

#include "common/algorithm.h"
#include "common/array.h"
#include "common/str.h"
#include "common/stream.h"
#include "common/system.h"
#include "common/textconsole.h"

#include "engines/grim/emi/audiosystem.h"
#include "engines/grim/emi/caudiodevice.h"
#include "engines/grim/emi/cmusicps2.h"
#include "engines/grim/emi/cpathingfileservices.h"
#include "engines/grim/emi/cwaveaudiodevice.h"
#include "engines/grim/emi/cwavechannel.h"
#include "engines/grim/emi/cwavesound.h"
#include "engines/grim/emi/emiengine.h"
#include "engines/grim/emi/lconsoleservices.h"
#include "engines/grim/emi/lipdata.h"
#include "engines/grim/emi/lstring.h"
#include "engines/grim/emi/profilertimer.h"
#include "engines/grim/emi/sounds.h"
#include "engines/grim/emi/streamscript.h"
#include "engines/grim/grim.h"

namespace Grim {

static ProfilerTimer soundsUpdateTimer("SoundsUpdate()", ProfilerTimer::kTrackNesting);

static const uint kMaxHandle = 0xFFFF;
static const uint kUpdateIntervalMs = 30;
static const uint kThrottledUpdateIntervalMs = 100;
static const int kMaxStartAttempts = 16;
static const int kReverbPresetCount = 28;
static const float kLoadingLoopFadeSeconds = 0.5f;
static const int kReverbPresets[kReverbPresetCount] = {
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,
	14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27
};

AudioSystem *Sounds::audioSystem = nullptr;
CSoundCue::SoundCueMap Sounds::soundMap;
uint16 Sounds::soundCounter = 1;
int16 Sounds::audioMixAhead = 16;
bool Sounds::soundsEnabled = true;
bool Sounds::musicEnabled = true;
bool Sounds::musicCategoryEnabled = true;
bool Sounds::voiceEnabled = true;
bool Sounds::sfxEnabled = true;
byte Sounds::voiceFxEnabled = 1;
int Sounds::savedMusicState = -1;
bool Sounds::musicActive = false;
int Sounds::musicState = 0;
bool Sounds::musicSet = false;
float Sounds::musicVolume = 1.0f;
int Sounds::pushedMusicState = -1;
int Sounds::pushedMusicPosition = -1;
uint Sounds::lastUpdateMs = 0;
uint Sounds::lastThrottledUpdateMs = 0;
float Sounds::dbStep = (float)((double)(0.0f - -40.0f) * (double)0.01010101f);
float Sounds::invDbStep = (float)(1.0 / (double)Sounds::dbStep);
float Sounds::unityGain = (float)pow(10.0, (double)0.0f * (double)0.05f);
float Sounds::defaultMasterGain = (float)pow(10.0, (double)0.0f * (double)0.05f);
float Sounds::movieMasterGain = (float)pow(10.0, (double)-4.0f * (double)0.05f);
CSoundCue *Sounds::loadingLoopCue = nullptr;
int Sounds::loadingLoopFade = 0;
uint Sounds::loadingLoopFadeMs = 0;

LResource *Sounds::createWaveSoundResource() {
	return new CWaveSound();
}

float Sounds::soundVolumeToGain(int volume) {
	if (volume >= 100)
		return unityGain;

	if (volume > 0)
		return (float)pow(10.0, ((double)(volume - 1) * (double)dbStep + (double)-40.0f) * (double)0.05f);

	return 0.0f;
}

int Sounds::soundGainToVolume(float gain) {
	if (gain >= unityGain)
		return 100;

	if (!(gain > 0.0f))
		return 0;

	return (int)((log10((double)gain) * 20.0 - (double)-40.0f) * (double)invDbStep) + 1;
}

uint16 Sounds::registerSound(CSoundCue *cue) {
	uint16 handle = soundCounter;

	soundMap[handle] = cue;
	cue->handle = soundCounter;
	soundCounter++;

	return handle;
}

void Sounds::unregisterSound(uint16 soundHandle) {
	soundMap.erase(soundHandle);
}

CSoundCue *Sounds::findSoundCue(uint soundHandle) {
	CSoundCue *cue = nullptr;

	// The disasm keys the map with the whole dword
	if (soundHandle > kMaxHandle)
		return nullptr;

	if (soundMap.tryGetVal((uint16)soundHandle, cue))
		return cue;

	return nullptr;
}

bool Sounds::isCueRegistered(CSoundCue *cue) {
	for (CSoundCue::SoundCueMap::iterator it = soundMap.begin(); it != soundMap.end(); ++it) {
		if (it->_value == cue)
			return true;
	}

	return false;
}

bool Sounds::soundsUpdate(const bool *force) {
	soundsUpdateTimer.start();

	// PS2 updates sounds on every call, without an interval
	if (g_grim->getGamePlatform() == Common::kPlatformPS2) {
		if (!audioSystem) {
			soundsUpdateTimer.stop();

			return false;
		}

		updateLoadingLoop();
		CSoundCue::updateSoundCues();
		setAudioMixAhead(&audioMixAhead, false);
		AudioSystem::updateAudioDevice();
		updateMusic();
		soundsUpdateTimer.stop();

		return true;
	}

	if (audioSystem) {
		uint now = g_system->getMillis();

		if (*force || now - lastUpdateMs > kUpdateIntervalMs || now < lastUpdateMs) {
			lastUpdateMs = now;
			CSoundCue::updateSoundCues();

			int16 keepPreference = -1;

			setAudioMixAhead(&keepPreference, false);
			AudioSystem::updateAudioDevice();
			updateMusic();
			soundsUpdateTimer.stop();

			return true;
		}
	}

	soundsUpdateTimer.stop();

	return false;
}

bool Sounds::soundsUpdateThrottled(const bool *force) {
	if (!audioSystem)
		return false;

	uint now = g_system->getMillis();

	if (!*force && now - lastThrottledUpdateMs <= kThrottledUpdateIntervalMs && now >= lastThrottledUpdateMs)
		return false;

	lastThrottledUpdateMs = now;

	// The disasm only serves the Miles driver here
	return true;
}

void Sounds::startSounds(bool startMusicNow) {
	if (!soundsEnabled || audioSystem)
		return;

	audioSystem = new AudioSystem();

	if (AudioSystem::audioAvailable) {
		restoreMasterGain();
		AudioSystem::lipCallback = soundCueLipCallback;

		if (musicEnabled && musicCategoryEnabled && startMusicNow)
			startMusic(-1);

		if (g_grim->getGamePlatform() == Common::kPlatformPS2) {
			loadingLoopCue = CSoundCue::newSoundCue(LString("@LOADLOOP.SCX"), 1, nullptr);

			// PS2 keeps this cue out of the cue table, so it is never saved, released or updated with the others
			if (loadingLoopCue)
				loadingLoopCue->unlinkFromCueList();
		}
	} else {
		delete audioSystem;
		audioSystem = nullptr;
		musicEnabled = false;
		soundsEnabled = false;
		sfxEnabled = false;
		voiceEnabled = false;
		musicCategoryEnabled = false;
	}
}

void Sounds::shutdownSounds() {
	if (audioSystem) {
		stopMusic();

		if (g_grim->getGamePlatform() == Common::kPlatformPS2)
			CMusicPs2::resetModule();

		releaseAllSounds();

		// PS2 never frees it, but we must so that the engine can be restarted
		if (loadingLoopCue) {
			CSoundCue::deleteSoundCue(loadingLoopCue);
			loadingLoopCue = nullptr;
		}

		loadingLoopFade = 0;

		delete audioSystem;
		audioSystem = nullptr;
	}
}

void Sounds::releaseAllSounds() {
	// PS2 only deletes the cues in the cue table
	if (g_grim->getGamePlatform() == Common::kPlatformPS2) {
		soundMap.clear();
		CSoundCue::releaseAllCues();

		return;
	}

	Common::Array<uint16> handles;

	for (CSoundCue::SoundCueMap::iterator it = soundMap.begin(); it != soundMap.end(); ++it)
		handles.push_back(it->_key);

	Common::sort(handles.begin(), handles.end());

	for (uint i = 0; i < handles.size(); i++)
		stopSound(handles[i]);

	soundMap.clear();
	CSoundCue::updateSoundCues();

	if (audioSystem) {
		AudioSystem::updateAudioDevice();
		AudioSystem::stopAllChannels();
	}

	CSoundCue::releaseAllCues();
}

void Sounds::releaseUnlockedSounds() {
	CSoundCue::releaseUnlockedCues();
}

void Sounds::soundCueLipCallback(const char *soundName, CSoundCue *cue) {
	LipData *lipData = nullptr;
	int lipSize = LipData::readFile(soundName, &lipData);

	if (lipSize)
		cue->setLipData(lipData, lipSize);
}

void Sounds::saveSounds(tSaveRestoreAccess *access) {
	access(&soundCounter, sizeof(soundCounter));
	AudioSystem::saveAudioSettings(access);
	CSoundCue::saveCues(access);
}

void Sounds::restoreSounds(tSaveRestoreAccess *access) {
	access(&soundCounter, sizeof(soundCounter));
	AudioSystem::restoreAudioSettings(access);
	CSoundCue::restoreCues(access, &soundMap);
}

void Sounds::setSoundSystemEnabled(bool enabled) {
	soundsEnabled = enabled;
}

void Sounds::setMusicSystemEnabled(bool enabled) {
	musicEnabled = enabled;
}

uint Sounds::loadSound(const char *soundName, int category) {
	return loadSound(LString(soundName), category);
}

uint Sounds::loadSound(const LString &soundName, int category) {
	if (!audioSystem)
		return 0;

	uint dotPosition = soundName.find('.');
	bool hasDot = dotPosition != LString::npos;
	LString fullName;

	if (!hasDot || !LString(soundName.c_str() + dotPosition + 1).equalsIgnoreCase("SND"))
		fullName = LString("@") + soundName;
	else
		fullName = soundName;

	const char *defaultExtension = ".wav";
	int cueCategory = 0;

	switch (category) {
	case 1:
		if (!sfxEnabled)
			return 0;

		cueCategory = 1;
		defaultExtension = ".aif";

		break;

	case 2:
		if (!voiceEnabled)
			return 0;

		cueCategory = 2;

		break;

	case 3:
		if (!musicCategoryEnabled)
			return 0;

		cueCategory = 3;

		break;

	default:
		break;
	}

	if (g_grim->getGamePlatform() == Common::kPlatformPS2)
		defaultExtension = ".SCX";

	if (!hasDot)
		fullName += defaultExtension;

	CSoundCue *cue = CSoundCue::newSoundCue(fullName, cueCategory, nullptr);

	if (!cue)
		return 0;

	if (category == 2) {
		LipData *lipData = nullptr;
		int lipSize = LipData::readFile(soundName.c_str(), &lipData);

		if (lipSize)
			cue->setLipData(lipData, lipSize);
	}

	return registerSound(cue);
}

void Sounds::playSound(const LString &soundName, int volume) {
	if (!audioSystem || !sfxEnabled)
		return;

	float gain = soundVolumeToGain(volume);
	LString fullName(soundName);
	LString extension;

	LStringUtil::getExtension(fullName, extension);
	fullName.clear();

	if (extension != "snd" && extension != "SND")
		fullName += '@';

	fullName += soundName;

	if (soundName.find('.') == LString::npos)
		fullName += g_grim->getGamePlatform() == Common::kPlatformPS2 ? ".SCX" : ".aif";

	CSoundCue *cue = CSoundCue::newSoundCue(fullName, 1, nullptr);

	if (cue) {
		cue->setVolume(gain);
		cue->start(true);
	}
}

void Sounds::playSoundFrom(const LString &soundName, float x, float y, float z, int16 volume) {
	// PS2 ignores positional sounds for two frames after a set change
	if (g_grim->getGamePlatform() == Common::kPlatformPS2 && static_cast<EmiEngine *>(g_grim)->isSetChanging())
		return;

	uint soundHandle = loadSound(soundName, 1);

	if (soundHandle) {
		startSound(soundHandle, 1, false, volume);
		setSoundPosition(soundHandle, x, y, z);
		unregisterSound((uint16)soundHandle);
	}
}

void Sounds::playLoadedSoundFrom(uint soundHandle, float x, float y, float z, bool loop, int16 volume) {
	startSound(soundHandle, 0, loop, volume);
	setSoundPosition(soundHandle, x, y, z);
}

void Sounds::startSound(uint soundHandle, int startMode, bool loop, int volume) {
	CSoundCue *cue = findSoundCue(soundHandle);

	if (!cue)
		return;

	if (cue->category == 1) {
		if (!sfxEnabled)
			return;
	} else if (cue->category == 2 && !voiceEnabled) {
		return;
	}

	if (loop)
		cue->flags |= CSoundCue::kFlagLoop;
	else
		cue->flags &= ~CSoundCue::kFlagLoop;

	cue->setVolume(soundVolumeToGain(volume));

	int attempts = 0;

	if (cue->start(startMode != 0))
		return;

	if (cue->category == 2) {
		while (attempts < kMaxStartAttempts) {
			CSoundCue::stopCueToFreeChannel(true, true);
			attempts++;

			if (cue->start(startMode != 0))
				return;
		}
	} else {
		while (attempts < kMaxStartAttempts) {
			CSoundCue::stopCueToFreeChannel(true, false);
			attempts++;

			if (cue->start(startMode != 0))
				return;
		}
	}
}

void Sounds::stopSound(uint soundHandle) {
	CSoundCue *cue = findSoundCue(soundHandle);

	if (cue)
		CSoundCue::deleteSoundCue(cue);

	unregisterSound((uint16)soundHandle);
}

void Sounds::stopSoundPlayback(uint soundHandle) {
	CSoundCue *cue = findSoundCue(soundHandle);

	if (cue) {
		if ((cue->flags & CSoundCue::kFlagReleaseWhenDone) != 0)
			unregisterSound((uint16)soundHandle);

		cue->stop();
	}
}

void Sounds::releaseSoundWhenDone(uint soundHandle) {
	CSoundCue *cue = findSoundCue(soundHandle);

	if (cue) {
		if (cue->getState() == CSoundCue::kStatePlaying) {
			cue->flags |= CSoundCue::kFlagReleaseWhenDone;
			unregisterSound((uint16)soundHandle);

			return;
		}

		CSoundCue::deleteSoundCue(cue);
	}

	unregisterSound((uint16)soundHandle);
}

void Sounds::pauseSound(uint soundHandle) {
	CSoundCue *cue = findSoundCue(soundHandle);

	if (cue)
		cue->pause();
}

void Sounds::resumeSound(uint soundHandle) {
	CSoundCue *cue = findSoundCue(soundHandle);

	if (cue)
		cue->resume();
}

bool Sounds::isSoundPlaying(uint soundHandle) {
	CSoundCue *cue = findSoundCue(soundHandle);

	return cue && cue->getState() == CSoundCue::kStatePlaying;
}

bool Sounds::isSoundPlayingNoFiniteLoop(uint soundHandle) {
	CSoundCue *cue = findSoundCue(soundHandle);

	if (!cue)
		return false;

	return cue->getState() == CSoundCue::kStatePlaying && !cue->hasFiniteLoopCount();
}

bool Sounds::isSoundPaused(uint soundHandle) {
	CSoundCue *cue = findSoundCue(soundHandle);

	return cue && cue->getState() == CSoundCue::kStatePaused;
}

void Sounds::setSoundPosition(uint soundHandle, float x, float y, float z) {
	CSoundCue *cue = findSoundCue(soundHandle);

	if (!cue)
		return;

	if (cue->category == 1) {
		if (!sfxEnabled)
			return;
	} else if (cue->category == 2 && !voiceEnabled) {
		return;
	}

	cue->setPosition(x, y, z);
}

bool Sounds::setSoundVolume(uint soundHandle, int volume) {
	CSoundCue *cue = findSoundCue(soundHandle);

	if (!cue)
		return false;

	cue->setVolume(soundVolumeToGain(volume));

	return true;
}

bool Sounds::getSoundVolume(uint soundHandle, int *volume) {
	CSoundCue *cue = findSoundCue(soundHandle);

	if (!cue)
		return false;

	*volume = soundGainToVolume(cue->getVolume());

	return true;
}

int Sounds::getSoundPosition(uint soundHandle) {
	CSoundCue *cue = findSoundCue(soundHandle);

	if (!cue)
		return 0;

	return cue->getPositionMs(nullptr);
}

int Sounds::getSoundLength(uint soundHandle) {
	CSoundCue *cue = findSoundCue(soundHandle);

	if (!cue)
		return 0;

	// The disasm returns the handle if the channel writes no length
	int length = (int)soundHandle;

	cue->getPositionMs(&length);

	return length;
}

LipData *Sounds::getSoundLipData(uint soundHandle) {
	CSoundCue *cue = findSoundCue(soundHandle);

	if (!cue)
		return nullptr;

	return cue->lipData;
}

bool Sounds::setCategoryVolume(int category, int volume) {
	if (!audioSystem)
		return false;

	float gain = soundVolumeToGain(volume);

	switch (category) {
	case 0:
		if (soundsEnabled)
			AudioSystem::setMasterGain(gain);

		return true;

	case 1:
		if (sfxEnabled)
			AudioSystem::setCategoryGain(1, gain);

		return true;

	case 2:
		if (voiceEnabled)
			AudioSystem::setCategoryGain(2, gain);

		return true;

	case 3:
		if (musicCategoryEnabled) {
			AudioSystem::setCategoryGain(3, gain);
			setMusicVolume(gain);
		}

		return true;

	default:
		break;
	}

	return false;
}

int Sounds::getCategoryVolume(int category) {
	if (!audioSystem)
		return 0;

	float gain = 0.0f;

	switch (category) {
	case 0:
		gain = AudioSystem::getMasterGain();

		break;

	case 1:
		gain = AudioSystem::getCategoryGain(1);

		break;

	case 2:
		gain = AudioSystem::getCategoryGain(2);

		break;

	case 3:
		gain = AudioSystem::getCategoryGain(3);

		break;

	default:
		break;
	}

	return soundGainToVolume(gain);
}

void Sounds::setCategoryEnabled(int category, bool enabled) {
	if (enabled) {
		if (!soundsEnabled)
			return;
	} else {
		setCategoryVolume(category, 0);
	}

	switch (category) {
	case 1:
		sfxEnabled = enabled;

		break;

	case 2:
		voiceEnabled = enabled;

		break;

	case 3:
		if (enabled && musicEnabled) {
			startMusic(savedMusicState);
			musicCategoryEnabled = enabled;
		} else {
			if (musicCategoryEnabled)
				savedMusicState = stopMusic();

			musicCategoryEnabled = enabled;
		}

		break;

	default:
		break;
	}
}

void Sounds::setVoiceFxEnabled(int enabled) {
	AudioSystem::setGlobalReverbEnabled(enabled);
	voiceFxEnabled = (byte)enabled;
}

void Sounds::setReverb(int preset, float *param1, float *param2, float *param3, float *param4, float *param5) {
	if (!voiceFxEnabled)
		return;

	if (!audioSystem)
		return;

	if (preset < 0 || preset >= kReverbPresetCount) {
		warning("Sounds::setReverb(): the preset %d is out of range", preset);

		return;
	}

	AudioSystem::setGlobalReverb(kReverbPresets[preset], param1, param2, param3, param4, param5);
}

void Sounds::restoreMasterGain() {
	if (audioSystem)
		AudioSystem::setMasterGain(defaultMasterGain);
}

void Sounds::lowerMasterGainForMovie() {
	if (audioSystem)
		AudioSystem::setMasterGain(movieMasterGain);
}

void Sounds::setAudioMixAhead(const int16 *mixAhead, bool store) {
	if (*mixAhead < 0) {
		uint16 storedPreference = (uint16)audioMixAhead;

		AudioSystem::setAudioDevicePreference(&storedPreference);

		return;
	}

	uint16 newPreference = (uint16)*mixAhead;

	AudioSystem::setAudioDevicePreference(&newPreference);

	if (store)
		audioMixAhead = *mixAhead;
}

void Sounds::setSoundListener(CNode *listener) {
	AudioSystem::setAudioListener(listener);
}

void Sounds::suspendAudio() {
	if (audioSystem) {
		if (!AudioSystem::digitalHandleReleased) {
			if (AudioSystem::device)
				AudioSystem::device->releaseDigitalHandle();

			AudioSystem::digitalHandleReleased = true;
		}
	}
}

void Sounds::resumeAudio() {
	if (audioSystem) {
		if (AudioSystem::digitalHandleReleased) {
			if (AudioSystem::device)
				AudioSystem::device->reacquireDigitalHandle();

			AudioSystem::digitalHandleReleased = false;
		}
	}
}

void Sounds::pauseAll() {
	if (audioSystem) {
		if (AudioSystem::device)
			AudioSystem::device->pauseAll();
	}
}

void Sounds::resumeAll() {
	if (audioSystem) {
		if (AudioSystem::device)
			AudioSystem::device->resumeAll();
	}
}

void Sounds::stopAllSounds() {
	stopAllMusic();
	releaseAllSounds();
}

void Sounds::updateMusic() {
	if (musicActive) {
		if (g_grim->getGamePlatform() == Common::kPlatformPS2)
			CMusicPs2::updateStates();
		else
			StreamScript::updateStreams();
	}
}

void Sounds::startMusic(int newState) {
	if (musicActive)
		return;

	if (g_grim->getGamePlatform() == Common::kPlatformPS2) {
		if (!CMusicPs2::openMusicTable()) {
			CMusicPs2::clearAudioSearchDirs();

			return;
		}

		musicActive = true;

		if (newState != -1) {
			bool immediate = false;

			setMusicState(newState, -1, immediate);
		}

		return;
	}

	const char *setDirectory = musicSet ? "Mego\\" : "Spago\\";
	Common::SeekableReadStream *secrets = StreamScript::openStream("Textures\\secrets.m4b");

	if (secrets) {
		LString directory = LString("Textures\\") + setDirectory;

		StreamScript::pushAudioSearchDir(directory.c_str());

		delete secrets;
	}

	const char *cdPath = CPathingFileServices::getCdPath();

	if (cdPath) {
		LString directory = LString(cdPath) + "\\" + "Textures\\";

		StreamScript::pushAudioSearchDir(directory.c_str());

		directory += setDirectory;

		StreamScript::pushAudioSearchDir(directory.c_str());
	} else {
		StreamScript::pushAudioSearchDir("Textures\\");
	}

	const char *musicScript = "FullMonkeyMap.imt";

	if (!StreamScript::hasAudioFile(musicScript)) {
		int disc = EmiEngine::getDiscNumber();

		if (disc == 1)
			musicScript = "FullMonkeyMap1.imt";
		else if (disc == 2)
			musicScript = "FullMonkeyMap2.imt";
	}

	if (!StreamScript::openMusicScript(musicScript)) {
		StreamScript::clearAudioSearchDirs();

		return;
	}

	musicActive = true;

	if (newState != -1) {
		bool immediate = false;

		setMusicState(newState, -1, immediate);
	}
}

int Sounds::stopMusic() {
	if (musicActive) {
		if (g_grim->getGamePlatform() == Common::kPlatformPS2) {
			CMusicPs2::closeMusicTable();
			CMusicPs2::clearAudioSearchDirs();
		} else {
			StreamScript::closeStreamScript();
			StreamScript::clearAudioSearchDirs();
		}

		musicActive = false;
	}

	return musicState;
}

void Sounds::setMusicVolume(float volume) {
	if (musicActive) {
		musicVolume = volume;

		if (g_grim->getGamePlatform() == Common::kPlatformPS2)
			CMusicPs2::setMusicTableVolume(volume);
		else
			StreamScript::setMusicStreamVolume(volume);
	}
}

void Sounds::setMusicState(int newState, int startMs, const bool &immediate) {
	if (g_grim->getGamePlatform() == Common::kPlatformPS2) {
		if (musicActive && !CMusicPs2::isSameFile(newState, musicState)) {
			musicState = newState;

			CMusicPs2::startState(newState, startMs, immediate);
		}

		return;
	}

	if (musicActive) {
		uint stateId = (uint)newState;

		musicState = newState;

		StreamScript::startMusicStream(stateId, startMs, immediate);
	}
}

void Sounds::saveMusicState(tSaveRestoreAccess *access) {
	access(&musicVolume, sizeof(musicVolume));
	access(&musicState, sizeof(musicState));
	access(&pushedMusicState, sizeof(pushedMusicState));
	access(&pushedMusicPosition, sizeof(pushedMusicPosition));
}

void Sounds::restoreMusicState(tSaveRestoreAccess *access) {
	access(&musicVolume, sizeof(musicVolume));

	setMusicVolume(musicVolume);

	access(&musicState, sizeof(musicState));
	access(&pushedMusicState, sizeof(pushedMusicState));
	access(&pushedMusicPosition, sizeof(pushedMusicPosition));
}

void Sounds::pauseMusicStreams() {
	// PS2 has no music pause
	if (g_grim->getGamePlatform() == Common::kPlatformPS2)
		return;

	if (musicActive)
		StreamScript::pauseStreams();
}

void Sounds::resumeMusicStreams() {
	if (g_grim->getGamePlatform() == Common::kPlatformPS2)
		return;

	if (musicActive)
		StreamScript::resumeStreams();
}

void Sounds::stopAllMusic() {
	if (musicActive) {
		musicState = 0;

		if (g_grim->getGamePlatform() == Common::kPlatformPS2)
			CMusicPs2::closeAllStates();
		else
			StreamScript::closeAllStreams();
	}
}

void Sounds::selectMusicSet(int16 set) {
	if (musicActive && musicSet == (set != 0))
		return;

	musicSet = set != 0;

	int stoppedState = stopMusic();

	startMusic(stoppedState);
}

bool Sounds::consumeMusicFlags(const uint &musicId, bool &looped, bool &ended) {
	if (g_grim->getGamePlatform() == Common::kPlatformPS2) {
		int stateId = (int)musicId;

		return CMusicPs2::consumeStateFlags(stateId, looped, ended);
	}

	return StreamScript::consumeMusicStreamFlags(musicId, looped, ended);
}

void Sounds::pushMusicState(int musicId, const bool &immediate) {
	if (musicActive) {
		uint stateId = (uint)musicState;
		bool relative = false;

		pushedMusicState = musicState;

		if (g_grim->getGamePlatform() == Common::kPlatformPS2)
			pushedMusicPosition = CMusicPs2::getStatePositionMs(musicState, relative);
		else
			pushedMusicPosition = StreamScript::getMusicStreamPositionMs(stateId, relative);

		setMusicState(musicId, -1, immediate);
	}
}

void Sounds::popMusicState(const bool &immediate) {
	if (musicActive && pushedMusicState != -1) {
		setMusicState(pushedMusicState, pushedMusicPosition, immediate);

		pushedMusicState = -1;
	}
}

void Sounds::flushMusicStateStack() {
	pushedMusicState = -1;
}

int Sounds::getMusicPositionMs(const uint &musicId) {
	bool relative = true;

	if (g_grim->getGamePlatform() == Common::kPlatformPS2) {
		int stateId = (int)musicId;

		return CMusicPs2::getStatePositionMs(stateId, relative);
	}

	return StreamScript::getMusicStreamPositionMs(musicId, relative);
}

void Sounds::killMusicAndWait(bool loadingLoop) {
	bool immediate = false;
	bool force = true;

	setMusicState(0, -1, immediate);

	if (gpConsoleServices)
		gpConsoleServices->logDebug("SOUNDS WAITING FOR MUSIC TO FADE!!\n");

	if (loadingLoop && CMusicPs2::getStreamPosition())
		fadeLoadingLoop(1);

	while (CMusicPs2::getStreamPosition())
		soundsUpdate(&force);

	if (gpConsoleServices)
		gpConsoleServices->logDebug("SOUNDS DONE WAITING FOR MUSIC FADE.\n");
}

void Sounds::killMusicForSet(const char *setName) {
	// The first set change only arms the SPU clear
	if (static_cast<EmiEngine *>(g_grim)->armSpuClear())
		clearSpuMemory();

	killMusicAndWait(setName && scumm_stricmp(setName, "blank.set") != 0);
}

void Sounds::stopAllForLoad() {
	stopAllMusic();
	releaseAllSounds();

	if (static_cast<EmiEngine *>(g_grim)->isSpuClearArmed())
		clearSpuMemory();

	// The IOP closes streams asynchronously and waits, while ours close immediately
	CMusicPs2::closeStream();
}

void Sounds::clearSpuMemory() {
	if (!audioSystem || !AudioSystem::device)
		return;

	CWaveAudioDevice *device = dynamic_cast<CWaveAudioDevice *>(AudioSystem::device);

	if (!device)
		return;

	for (uint i = 0; i < device->channels.size(); i++) {
		if (device->channels[i])
			device->channels[i]->endSpuSample();
	}
}

void Sounds::fadeLoadingLoop(int direction) {
	if (!loadingLoopCue)
		return;

	if (direction != 1 && direction != -1)
		return;

	if (direction == -1 && loadingLoopCue->getState() != CSoundCue::kStatePlaying)
		return;

	loadingLoopFade = direction;
	loadingLoopFadeMs = g_system->getMillis();

	if (loadingLoopFade == 1) {
		loadingLoopCue->flags |= CSoundCue::kFlagLoop;
		loadingLoopCue->setVolume(0.0f);
		loadingLoopCue->play(false);
	}
}

void Sounds::updateLoadingLoop() {
	if (!loadingLoopFade)
		return;

	uint now = g_system->getMillis();
	float step = (float)(now - loadingLoopFadeMs) / 1000.0f / kLoadingLoopFadeSeconds;
	float volume = loadingLoopCue->volume;

	if (loadingLoopFade > 0) {
		volume += step;

		if (!(volume <= 1.0f))
			volume = 1.0f;

		if (volume == 1.0f)
			loadingLoopFade = 0;
	} else if (loadingLoopFade == -1) {
		// The fade-out starts one update late
		loadingLoopFade = -2;
	} else {
		volume -= step;

		if (volume < 0.0f)
			volume = 0.0f;

		if (volume == 0.0f) {
			loadingLoopCue->stop();
			loadingLoopFade = 0;
		}
	}

	loadingLoopCue->setVolume(volume);

	if (loadingLoopFade)
		loadingLoopFadeMs = now;
}

void Sounds::setStereoAudio(bool stereo) {
	CWaveChannel::setSpuStereo(stereo);
}

} // End of namespace Grim

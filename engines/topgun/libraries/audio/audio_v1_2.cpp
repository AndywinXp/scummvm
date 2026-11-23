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

#include "common/config-manager.h"
#include "common/endian.h"
#include "common/system.h"
#include "common/textconsole.h"

#include "topgun/libraries/audio/audio_v1_2_3.h"
#include "topgun/libraries/utils/utils.h"
#include "topgun/topgun_v1_2_3.h"

namespace Topgun {

AudioLibrary_v1_2::AudioLibrary_v1_2(TopgunEngine_v1_2 *engine) : AudioLibrary(engine) {
}

TopgunEngine_v1_2 *AudioLibrary_v1_2::engine() const {
	return static_cast<TopgunEngine_v1_2 *>(_engine);
}

AudioLibrary_v1_2::~AudioLibrary_v1_2() {
	freeAudioContext();

	if (_waveOutStream) {
		g_system->getMixer()->stopHandle(_waveOutHandle);
		delete _waveOutStream;
		_waveOutStream = nullptr;
	}

	freeWaveOutBuffers();
}

uint16 AudioLibrary_v1_2::audioInitLibrary(uint16 hwnd, uint16 version) {
	if (version != TG_AUDIO_V1_2_VERSION)
		return 0;

	if (!_libMainDone) {
		_libMainDone = true;
		libMain();
	}

	if (!allocAudioContext())
		return 0;

	_hwnd = hwnd;
	return 1;
}

void AudioLibrary_v1_2::audioExitLibrary() {
	releaseCDDeviceIfOwned();
	destroyAllWaveObjectsAndCloseDevice();
	freeAudioContext();
}

void AudioLibrary_v1_2::audioService() {
	pumpAudioPlayback();
	cdFadeTick();
}

void AudioLibrary_v1_2::audioActivate(uint16 on) {
	updateWaveEnabledFlags(on);

	if (on == 0) {
		stopCDIfOwned();
		audioCloseWave();
	} else if (_activeSounds != 0) {
		audioOpenWave(&_waveFormat);
	}
}

uint16 AudioLibrary_v1_2::libMain() {
	readAudioIniSettings();
	findCdAuxDevice();
	probeWaveOutCapabilities();
	return 1;
}

bool AudioLibrary_v1_2::allocAudioContext() {
	if (_contextCreated) {
		warning("AudioLibrary_v1_2::allocAudioContext(): the library holds a single context");
		return false;
	}

	_contextCreated = true;
	_hwnd = 0;
	_mute = false;
	_soundCount = 0;
	_activeSounds = 0;
	_ringHead = 0;
	_ringTail = 0;
	_buffersInFlight = 0;
	_waveOut8Bit = false;
	_waveFormat = WaveFormat();
	_chosenFormatMask = 0;
	_userFormatMask = 0;
	_userVolumeSet = false;
	_userVolume = 0;
	_waveVolumeLeft = 0;
	_waveVolumeRight = 0;
	_savedWaveVolumeLeft = 0;
	_savedWaveVolumeRight = 0;
	_masterFadeMode = 0;
	_fadeTargetVolumeLeft = 0;
	_fadeTargetVolumeRight = 0;
	_fadeNextTick = 0;
	_fadeVolumeStepLeft = 0;
	_fadeVolumeStepRight = 0;
	_fadeStepsRemaining = 0;
	_masterAvgVolumeLeft = 0;
	_masterAvgVolumeRight = 0;
	_cdSuspend = false;
	return true;
}

void AudioLibrary_v1_2::freeAudioContext() {
	UtilsLibrary *utilsLibrary = engine()->utilsLibrary();

	for (uint16 i = 0; i < _soundCount; i++) {
		utilsLibrary->tgFree(_sounds[i]->data);
		delete _sounds[i];
		_sounds[i] = nullptr;
	}

	_soundCount = 0;
	_contextCreated = false;
}

uint16 AudioLibrary_v1_2::readAudioIniInt(const char *key, uint16 defaultValue) {
	if (!ConfMan.hasKey(key))
		return defaultValue;

	return (uint16)ConfMan.getInt(key);
}

void AudioLibrary_v1_2::readAudioIniSettings() {
	_iniWaveFormatMask = readAudioIniInt("waveformatmask", TG_AUDIO_V1_2_DEFAULT_FORMAT_MASK);
	_disableWaveVolume = (readAudioIniInt("disablewavevolume", 0) != 0);
	_maxWaveBuffers = readAudioIniInt("maxwavebuffers", TG_AUDIO_V1_2_DEFAULT_WAVE_BUFFERS);

	if (_maxWaveBuffers > TG_AUDIO_V1_2_MAX_WAVE_BUFFERS)
		_maxWaveBuffers = TG_AUDIO_V1_2_MAX_WAVE_BUFFERS;
	else if (_maxWaveBuffers < TG_AUDIO_V1_2_MIN_WAVE_BUFFERS)
		_maxWaveBuffers = TG_AUDIO_V1_2_MIN_WAVE_BUFFERS;
}

uint32 AudioLibrary_v1_2::audioGetMasterVolume() {
	averageStereoVolumes(audioGetCdVolume(), audioGetWaveVolume());
	return ((uint32)_masterAvgVolumeRight << 16) | _masterAvgVolumeLeft;
}

void AudioLibrary_v1_2::audioSetMasterVolume(uint32 volume) {
	uint16 deltaLeft = (uint16)((volume & 0xFFFF) - _masterAvgVolumeLeft);
	uint16 deltaRight = (uint16)((volume >> 16) - _masterAvgVolumeRight);

	uint32 cdVolume = addClampedStereoVolumeDelta(deltaLeft, deltaRight, audioGetCdVolume());
	uint32 waveVolume = addClampedStereoVolumeDelta(deltaLeft, deltaRight, audioGetWaveVolume());

	audioSetCdVolume(cdVolume);
	audioSetWaveVolume(waveVolume);
	averageStereoVolumes(cdVolume, waveVolume);
}

uint32 AudioLibrary_v1_2::audioGetWaveVolume() {
	return ((uint32)_waveVolumeRight << 16) | _waveVolumeLeft;
}

void AudioLibrary_v1_2::audioSetWaveVolume(uint32 volume) {
	if (_noWaveDevice)
		return;

	_userVolumeSet = true;
	_userVolume = volume;
	setWaveOutVolume(volume);
}

void AudioLibrary_v1_2::audioSetWaveFormats(uint16 mask) {
	_userFormatMask = mask;
}

void AudioLibrary_v1_2::audioMute(int16 muted) {
	bool previous = _mute;

	if (_engine->_version >= kVersionV1_2_1) {
		if (muted < 0)
			_mute = !previous;
		else
			_mute = (muted > 0);
	} else {
		_mute = (muted != 0);
	}

	if (_mute != previous) {
		updateWaveEnabledFlags(1);
		muteCDAudio();

		if (_engine->_version >= kVersionV1_2_1 && _mute)
			resetWaveOutQueue();
	}
}

uint16 AudioLibrary_v1_2::audioGetWaveDeviceInfo(uint32 *info) {
	if (info) {
		info[0] = _waveOutCapsFormats;
		info[1] = _waveOutStream ? (uint32)(uint16)_chosenFormatMask : 0;
	}

	if (_noWaveDevice)
		return 0;

	return _waveDeviceIsSync ? 2 : 1;
}

uint16 AudioLibrary_v1_2::audioMasterFadeIn(uint32 milliseconds) {
	if (_noWaveDevice)
		return 1;

	if (_waveOutStream)
		return 0;

	if (_masterFadeMode != 0)
		return 0;

	_fadeStepsRemaining = (uint16)((milliseconds * 5) / 1000);

	if (_fadeStepsRemaining == 0)
		return 1;

	_masterFadeMode = 1;
	_fadeTargetVolumeLeft = _waveVolumeLeft;
	_fadeTargetVolumeRight = _waveVolumeRight;

	uint32 steps = _fadeStepsRemaining;

	_fadeVolumeStepLeft = (uint16)(((steps >> 1) + _fadeTargetVolumeLeft) / steps);
	_fadeVolumeStepRight = (uint16)(((steps >> 1) + _fadeTargetVolumeRight) / steps);
	setWaveOutVolume(0);
	_fadeNextTick = _engine->getEngineMillis() + TG_AUDIO_FADE_TICK_INTERVAL;
	return 1;
}

uint16 AudioLibrary_v1_2::audioMasterFadeOut(uint32 milliseconds) {
	if (_noWaveDevice)
		return 1;

	if (!_waveOutStream)
		return 0;

	if (_masterFadeMode != 0)
		return 0;

	_fadeStepsRemaining = (uint16)((milliseconds * 5) / 1000);

	if (_fadeStepsRemaining == 0) {
		setWaveOutVolume(0);
		return 1;
	}

	_masterFadeMode = 0xFF;
	_fadeTargetVolumeLeft = _waveVolumeLeft;
	_fadeTargetVolumeRight = _waveVolumeRight;

	uint32 steps = _fadeStepsRemaining;

	_fadeVolumeStepLeft = (uint16)(((steps >> 1) + _fadeTargetVolumeLeft) / steps);
	_fadeVolumeStepRight = (uint16)(((steps >> 1) + _fadeTargetVolumeRight) / steps);
	_fadeNextTick = _engine->getEngineMillis() + TG_AUDIO_FADE_TICK_INTERVAL;
	return 1;
}

void AudioLibrary_v1_2::masterFadeTick() {
	uint32 now = _engine->getEngineMillis();

	if (now < _fadeNextTick)
		return;

	_fadeNextTick = now + TG_AUDIO_FADE_TICK_INTERVAL;

	if (_masterFadeMode == 1)
		masterFadeUpTick();
	else
		masterFadeDownTick();
}

void AudioLibrary_v1_2::masterFadeDownTick() {
	int32 newLeft = (int32)_waveVolumeLeft - _fadeVolumeStepLeft;
	int32 newRight = (int32)_waveVolumeRight - _fadeVolumeStepRight;

	if (newLeft < 0)
		newLeft = 0;

	if (newRight < 0)
		newRight = 0;

	_waveVolumeLeft = (uint16)newLeft;
	_waveVolumeRight = (uint16)newRight;

	_fadeStepsRemaining--;

	if (_fadeStepsRemaining == 0 || (_waveVolumeLeft | _waveVolumeRight) == 0) {
		_masterFadeMode = 0;
		audioStopAllWaves(nullptr);
		_waveVolumeLeft = _fadeTargetVolumeLeft;
		_waveVolumeRight = _fadeTargetVolumeRight;
	}

	setWaveOutVolume(((uint32)_waveVolumeRight << 16) | _waveVolumeLeft);
}

AudioLibrary_v1_2_3::AudioLibrary_v1_2_3(TopgunEngine_v1_2_3 *engine) : AudioLibrary_v1_2(engine) {
}

TopgunEngine_v1_2_3 *AudioLibrary_v1_2_3::engine() const {
	return static_cast<TopgunEngine_v1_2_3 *>(_engine);
}

AudioLibrary_v1_2_3::~AudioLibrary_v1_2_3() {
	closeMidiOutDevice();
	freeAudioContext();
}

uint16 AudioLibrary_v1_2_3::audioInitLibrary(uint16 hwnd, uint16 version) {
	if (version != TG_AUDIO_V1_2_3_VERSION)
		return 0;

	if (!_libMainDone) {
		_libMainDone = true;
		initializeAudioSubsystem();
	}

	if (!allocAudioContext())
		return 0;

	_hwnd = hwnd;
	return 1;
}

void AudioLibrary_v1_2_3::audioExitLibrary() {
	releaseCDDeviceIfOwned();
	destroyAllMidiObjects();
	destroyAllWaveObjectsAndCloseDevice();
	freeAudioContext();
}

void AudioLibrary_v1_2_3::audioService() {
	pumpAudioPlayback();
	midiServiceTick();
}

void AudioLibrary_v1_2_3::audioActivate(uint16 on) {
	updateWaveEnabledFlags(on);

	if (on == 0) {
		audioStopCd();
		closeMidiOutDevice();
		audioCloseWave();
		return;
	}

	if (_paused)
		return;

	if (_activeSounds != 0)
		openWaveDevice(&_waveFormat);

	if (_currentMidi) {
		openAndConfigureMidiOut();
		_currentMidi->index = 0;
		midiSequencerStep(_currentMidi);
	}
}

void AudioLibrary_v1_2_3::audioPause(uint16 paused) {
	bool state = (paused != 0);

	if (state == _paused)
		return;

	_paused = state;
	pauseAllPlayback(state ? 1 : 0);

	if (_currentMidi)
		pauseOrResumeMidiPlayback(state ? 1 : 0);
}

void AudioLibrary_v1_2_3::audioMute(int16 flag) {
	bool previous = _mute;

	if (flag < 0)
		_mute = !previous;
	else
		_mute = (flag > 0);

	if (_mute != previous) {
		muteCDAudio();
		waveMuteImpl();
		muteMidiOutput();
	}
}

void AudioLibrary_v1_2_3::audioResetLibrary() {
	_maxPriority = 0;
}

bool AudioLibrary_v1_2_3::allocAudioContext() {
	if (!AudioLibrary_v1_2::allocAudioContext())
		return false;

	_paused = false;
	_maxPriority = 0;
	_waveMuteSavedVolume = 0;
	_currentMidi = nullptr;
	_midiCount = 0;
	_midiOut = nullptr;
	_midiOriginalVolume = 0;
	_midiUserVolume = 0;
	_midiMuteSavedVolume = 0;
	_midiUserVolumeSet = false;
	_midiTimerArmed = false;
	return true;
}

void AudioLibrary_v1_2_3::freeAudioContext() {
	UtilsLibrary *utilsLibrary = engine()->utilsLibrary();

	for (uint16 i = 0; i < _midiCount; i++) {
		utilsLibrary->tgFree(_midiObjects[i]->data);
		delete _midiObjects[i];
		_midiObjects[i] = nullptr;
	}

	_midiCount = 0;
	_currentMidi = nullptr;
	AudioLibrary_v1_2::freeAudioContext();
}

void AudioLibrary_v1_2_3::initializeAudioSubsystem() {
	readAudioIniSettings();
	findCdAuxDevice();
	probeMidiOutCapabilities();
	probeWaveOutCapabilities();
}

void AudioLibrary_v1_2_3::readAudioIniSettings() {
	AudioLibrary_v1_2::readAudioIniSettings();

	_iniMidiVolume = readAudioIniInt("midivolume", 0);
	_remapMidiDrums = (readAudioIniInt("remapmididrums", 0) != 0);
}

void AudioLibrary_v1_2_3::writeAudioIniInt(const char *key, uint16 value) {
	writeAudioIniString(key, Common::String::format("%i", (int16)value).c_str());
}

void AudioLibrary_v1_2_3::writeAudioIniString(const char *key, const char *value) {
	ConfMan.set(key, value);
	ConfMan.flushToDisk();
}

void AudioLibrary_v1_2_3::saveMidiVolumeToIni() {
	writeAudioIniInt("midivolume", _iniMidiVolume);
}

} // End of namespace Topgun

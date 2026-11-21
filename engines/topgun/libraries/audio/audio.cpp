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

#include "common/endian.h"
#include "common/system.h"
#include "common/textconsole.h"

#include "topgun/libraries/audio/audio.h"
#include "topgun/libraries/audio/midi.h"
#include "topgun/libraries/utils.h"
#include "topgun/topgun.h"

namespace Topgun {

AudioLibrary::AudioLibrary(TopgunEngine *engine) : _engine(engine) {
}

AudioLibrary::~AudioLibrary() {
	UtilsLibrary *utilsLibrary = _engine->utilsLibrary();

	for (uint32 i = 0; i < _waveObjects.size(); i++) {
		WaveObject *wave = _waveObjects[i];

		utilsLibrary->tgFree(wave->stage);
		utilsLibrary->tgFree(wave->data);
		delete wave->fileStream;
		delete wave;
	}

	_waveObjects.clear();

	if (_waveOutStream) {
		g_system->getMixer()->stopHandle(_waveOutHandle);
		delete _waveOutStream;
		_waveOutStream = nullptr;
		freeWaveOutBuffers();
	}

	delete _sequencer;
	_sequencer = nullptr;
}

uint16 AudioLibrary::audioInitLibrary(uint16 version) {
	if ((version >> 8) != 1 || (version & 0xFF) != 0)
		return 0;

	uint16 previousCount = _initCount;
	_initCount++;

	if (previousCount == 0) {
		probeWaveOutCapabilities();
		findCdAuxDevice();
		findMasterAuxDevice();
	}

	if (_owner != 0)
		_engine->requestClose();

	return _initCount;
}

void AudioLibrary::audioExitLibrary() {
	_initCount--;

	if (_initCount == 0) {
		releaseCDDeviceIfOwned();
		shutdownMidi();
		destroyAllWaveObjectsAndCloseDevice();
		restoreMasterVolume();
	}
}

void AudioLibrary::audioService() {
	pumpAudioPlayback();
}

uint32 AudioLibrary::audioCommand(uint16 command, uint16 handle, uint32 value, void *block) {
	switch (command) {
	case kCmdCdPlayRange:
	case kCmdMidiPlayFile:
	case kCmdWaveCreate:
	case kCmdWaveStartGroup:
	case kCmdWaveStopGroup:
	case kCmdSoundGetFormat:
	case kCmdSoundStart:
	case kCmdSoundStop:
		if (!block) {
			warning("AudioLibrary::audioCommand(): command %d needs a parameter block", command);
			return 0;
		}

		break;

	default:
		break;
	}

	if (command >= kFirstSoundCommand && command <= kLastSoundCommand)
		return dispatchWaveSoundCommand(handle, command, value, block);

	if (command >= kFirstWaveCommand && command <= kLastWaveCommand)
		return dispatchWaveDeviceCommand(command, handle, value, block);

	if (command >= kFirstCdCommand && command <= kLastCdCommand)
		return dispatchCDCommand(command, handle, value, block);

	if (command >= kFirstMidiCommand && command <= kLastMidiCommand)
		return dispatchMidiCommand(command, handle, value, block);

	if (command >= kFirstMasterCommand && command <= kLastMasterCommand)
		return dispatchMasterVolumeCommand(command, handle, value, block);

	return 0;
}

uint16 AudioLibrary::audioActivate(uint16 active) {
	if (active == 0) {
		audioCommand(kCmdCdClose, 0, 0, nullptr);
		return (uint16)audioCommand(kCmdWaveClose, 0, 0, nullptr);
	}

	return (uint16)audioCommand(kCmdWaveOpen, 0, 0, nullptr);
}

uint16 AudioLibrary::audioSetOwner(uint16 owner) {
	uint16 previousOwner = _owner;

	_owner = owner;
	return previousOwner;
}

uint32 AudioLibrary::dispatchCDCommand(uint16 command, uint16 handle, uint32 value, void *block) {
	switch (command) {
	case kCmdCdIsOpen:
		return (_cdDeviceId != 0) ? 1 : 0;

	case kCmdCdGetPosition:
		if (_cdDeviceId == 0)
			return 0;

		return getCDCurrentPosition();

	case kCmdCdGetVolume:
		return readCDVolumeLevels();

	case kCmdCdGetTrackCount:
		return getCDTrackCount();

	case kCmdCdPlayFrom:
		return startCDPlayback(value, 0);

	case kCmdCdPlayRange: {
		const byte *range = (const byte *)block;

		return startCDPlayback(READ_LE_UINT32(range), READ_LE_UINT32(range + 4));
	}

	case kCmdCdPlayTracks:
		return playCDTrackRangeByNumber(value & 0xFF, (value >> 8) & 0xFF);

	case kCmdCdSetVolume:
		setCDAudioVolume(value);
		return 1;

	case kCmdCdClose:
		closeCDAudioDevice();
		return 1;

	default:
		return 0;
	}
}

uint32 AudioLibrary::dispatchMidiCommand(uint16 command, uint16 handle, uint32 value, void *block) {
	switch (command) {
	case kCmdMidiPlayDefault:
		playDefaultMidiFile();
		return 1;

	case kCmdMidiPlayFile:
		return playMidiFile((const char *)block);

	case kCmdMidiStop:
		stopAndCloseMidi();
		return 1;

	default:
		return 0;
	}
}

uint32 AudioLibrary::dispatchMasterVolumeCommand(uint16 command, uint16 handle, uint32 value, void *block) {
	switch (command) {
	case kCmdMasterGetVolume:
		return getMasterVolume();

	case kCmdMasterSetVolume:
		setMasterVolume(value);
		return 1;

	default:
		return 0;
	}
}

uint32 AudioLibrary::dispatchWaveDeviceCommand(uint16 command, uint16 handle, uint32 value, void *block) {
	switch (command) {
	case kCmdWaveClose:
		if (_waveOutStream)
			closeWaveOutDevice();

		return 1;

	case kCmdWaveCreate:
		return audioCreateWave((SoundCreateInfo *)block, handle);

	case kCmdWaveDestroyAll:
		destroyAllWaveObjects();
		return 1;

	case kCmdWavePause:
		return pauseAllPlayback(handle);

	case kCmdWaveGetVolume:
		return ((uint32)_waveVolumeRight << 16) | _waveVolumeLeft;

	case kCmdWaveGetFormats:
		return _waveOutCapsFormats;

	case kCmdWaveGetChosenFormat:
		return _chosenFormatMask;

	case kCmdWaveOpen:
		if (!_waveOutStream)
			openWaveOutDevice();

		return 1;

	case kCmdWaveStartGroup:
		startWavesBySelector((byte *)block);
		return 1;

	case kCmdWaveSetVolume:
		setWaveOutVolume(value);
		return 1;

	case kCmdWaveStopGroup: {
		byte *params = (byte *)block;

		if (params[2] != 0)
			startMasterFadeOut(params[2]);
		else
			stopWavesBySelector(params);

		return 1;
	}

	case kCmdWaveFadeOut:
		return startMasterFadeOut(handle);

	case kCmdWaveFadeUp:
		return startMasterFadeUp(handle);

	default:
		return 0;
	}
}

uint32 AudioLibrary::dispatchWaveSoundCommand(uint16 handle, uint16 command, uint32 value, void *block) {
	if (handle == 0)
		return 0;

	WaveObject *wave = findWaveByHandle(handle);
	if (!wave)
		return 0;

	switch (command) {
	case kCmdSoundDestroy:
		destroyWaveObject(wave);
		return 1;

	case kCmdSoundGetSourceSize:
		return wave->sourceSize;

	case kCmdSoundGetVolume:
		return wave->volume;

	case kCmdSoundGetFormat:
		writeWaveFormat((byte *)block, wave->format);
		return 1;

	case kCmdSoundIsActive:
		return wave->active ? 1 : 0;

	case kCmdSoundStart:
		startWavePlayback(wave, (const byte *)block);
		return 1;

	case kCmdSoundRewind:
		rewindSound(wave);
		return 1;

	case kCmdSoundSetVolume:
		wave->volume = (byte)value;

		if (wave->volume > kMaxVolumeLevel)
			wave->volume = kMaxVolumeLevel;

		wave->currentVolume = wave->volume;
		return 1;

	case kCmdSoundStop:
		stopWaveObject(wave, (const byte *)block);
		return 1;

	default:
		return 0;
	}
}

uint16 AudioLibrary::auxDeviceTechnology(uint16 device) const {
	return kAuxTechnologyCd;
}

void AudioLibrary::readWaveFormat(const byte *source, WaveFormat &format) {
	format.tag = READ_LE_UINT16(source);
	format.channels = READ_LE_UINT16(source + 2);
	format.sampleRate = READ_LE_UINT32(source + 4);
	format.averageBytesPerSecond = READ_LE_UINT32(source + 8);
	format.blockAlign = READ_LE_UINT16(source + 12);
	format.bitsPerSample = READ_LE_UINT16(source + 14);
}

void AudioLibrary::writeWaveFormat(byte *destination, const WaveFormat &format) {
	WRITE_LE_UINT16(destination, format.tag);
	WRITE_LE_UINT16(destination + 2, format.channels);
	WRITE_LE_UINT32(destination + 4, format.sampleRate);
	WRITE_LE_UINT32(destination + 8, format.averageBytesPerSecond);
	WRITE_LE_UINT16(destination + 12, format.blockAlign);
	WRITE_LE_UINT16(destination + 14, format.bitsPerSample);
}

uint32 AudioLibrary::addClampedStereoVolumeDelta(uint16 deltaLeft, uint16 deltaRight, uint32 volume) {
	int32 left = (int32)(volume & 0xFFFF) + (int32)(int16)deltaLeft;
	int32 right = (int32)(volume >> 16) + (int32)(int16)deltaRight;

	if (left < 0)
		left = 0;
	else if (left > 0xFFFF)
		left = 0xFFFF;

	if (right < 0)
		right = 0;
	else if (right > 0xFFFF)
		right = 0xFFFF;

	return ((uint32)right << 16) | (uint32)left;
}

void AudioLibrary::averageStereoVolumes(uint32 volumeA, uint32 volumeB) {
	_masterAvgVolumeLeft = (uint16)(((volumeA & 0xFFFF) + (volumeB & 0xFFFF)) >> 1);
	_masterAvgVolumeRight = (uint16)(((volumeA >> 16) + (volumeB >> 16)) >> 1);
}

uint32 AudioLibrary::getMasterVolume() {
	if (!_masterAuxPresent) {
		uint32 cdVolume = audioCommand(kCmdCdGetVolume, 0, 0, nullptr);
		uint32 waveVolume = audioCommand(kCmdWaveGetVolume, 0, 0, nullptr);

		averageStereoVolumes(cdVolume, waveVolume);
		return ((uint32)_masterAvgVolumeRight << 16) | _masterAvgVolumeLeft;
	}

	return _masterAuxVolume;
}

uint16 AudioLibrary::setMasterVolume(uint32 volume) {
	if (!_masterAuxPresent)
		return setMasterVolumeEmulated(volume);

	_masterAuxVolume = volume;
	return 1;
}

uint16 AudioLibrary::setMasterVolumeEmulated(uint32 volume) {
	uint16 deltaLeft = (uint16)((volume & 0xFFFF) - _masterAvgVolumeLeft);
	uint16 deltaRight = (uint16)((volume >> 16) - _masterAvgVolumeRight);

	uint32 cdVolume = addClampedStereoVolumeDelta(deltaLeft, deltaRight, audioCommand(kCmdCdGetVolume, 0, 0, nullptr));
	uint32 waveVolume = addClampedStereoVolumeDelta(deltaLeft, deltaRight, audioCommand(kCmdWaveGetVolume, 0, 0, nullptr));

	audioCommand(kCmdCdSetVolume, 0, cdVolume, nullptr);
	audioCommand(kCmdWaveSetVolume, 0, waveVolume, nullptr);
	averageStereoVolumes(cdVolume, waveVolume);
	return 1;
}

void AudioLibrary::restoreMasterVolume() {
	if (_masterAuxPresent)
		_masterAuxVolume = _masterAuxSavedVolume;
}

uint16 AudioLibrary::findMasterAuxDevice() {
	for (uint16 i = 0; i < kNumAuxDevices; i++) {
		if (auxDeviceTechnology(i) == kAuxTechnologyMaster) {
			_masterAuxPresent = true;
			_masterAuxId = i;
			_masterAuxSavedVolume = _masterAuxVolume;
			break;
		}
	}

	return 1;
}

void AudioLibrary::masterFadeUpTick() {
	uint32 newLeft = (uint32)_waveVolumeLeft + _fadeVolumeStepLeft;
	uint32 newRight = (uint32)_waveVolumeRight + _fadeVolumeStepRight;

	if (newLeft > _fadeTargetVolumeLeft)
		newLeft = _fadeTargetVolumeLeft;

	if (newRight > _fadeTargetVolumeRight)
		newRight = _fadeTargetVolumeRight;

	_waveVolumeLeft = (uint16)newLeft;
	_waveVolumeRight = (uint16)newRight;

	_fadeStepsRemaining--;

	if (_fadeStepsRemaining == 0 || (_waveVolumeLeft == _fadeTargetVolumeLeft && _waveVolumeRight == _fadeTargetVolumeRight))
		_masterFadeMode = 0;

	setWaveOutVolume(((uint32)_waveVolumeRight << 16) | _waveVolumeLeft);
}

void AudioLibrary::masterFadeDownTick() {
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
		byte stopBlock[3] = { 1, 0, 0 };

		_masterFadeMode = 0;
		stopWavesBySelector(stopBlock);
		_waveVolumeLeft = _fadeTargetVolumeLeft;
		_waveVolumeRight = _fadeTargetVolumeRight;
	}

	setWaveOutVolume(((uint32)_waveVolumeRight << 16) | _waveVolumeLeft);
}

uint16 AudioLibrary::startMasterFadeUp(uint16 duration) {
	if (_waveOutStream)
		return 0;

	if (duration == 0)
		return 0;

	_masterFadeMode = 1;
	_fadeStepsRemaining = (uint16)(duration * 5);
	_fadeTargetVolumeLeft = _waveVolumeLeft;
	_fadeTargetVolumeRight = _waveVolumeRight;

	uint32 steps = _fadeStepsRemaining;

	_fadeVolumeStepLeft = (uint16)(((uint32)_waveVolumeLeft + (steps >> 1)) / steps);
	_fadeVolumeStepRight = (uint16)(((uint32)_waveVolumeRight + (steps >> 1)) / steps);
	setWaveOutVolume(0);
	_fadeNextTick = _engine->getEngineMillis() + kFadeTickInterval;
	return 1;
}

uint16 AudioLibrary::startMasterFadeOut(uint16 duration) {
	if (!_waveOutStream)
		return 0;

	uint16 stepCount = (uint16)(duration * 5);
	if (stepCount == 0) {
		warning("AudioLibrary::startMasterFadeOut(): zero duration");
		return 0;
	}

	_masterFadeMode = 0xFF;
	_fadeStepsRemaining = stepCount;
	_fadeTargetVolumeLeft = _waveVolumeLeft;
	_fadeTargetVolumeRight = _waveVolumeRight;

	uint32 steps = _fadeStepsRemaining;

	_fadeVolumeStepLeft = (uint16)(((uint32)_waveVolumeLeft + (steps >> 1)) / steps);
	_fadeVolumeStepRight = (uint16)(((uint32)_waveVolumeRight + (steps >> 1)) / steps);
	_fadeNextTick = _engine->getEngineMillis() + kFadeTickInterval;
	return 1;
}

} // End of namespace Topgun

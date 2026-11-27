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
#include "common/textconsole.h"

#include "topgun/libraries/audio/audio_v1_2_4.h"
#include "topgun/libraries/utils/utils.h"
#include "topgun/topgun_v1_2_4.h"

namespace Topgun {

uint16 AudioLibrary_v1_2::audioCreateWave(SoundCreateInfo_v1_2 *info) {
	if (!info)
		return 0;

	if (info->readProc == 0)
		return 0;

	SoundObject *sound = allocWaveObject();

	if (!sound && info->closeProc != 0)
		engine()->audioSourceClose(info->closeProc, info->handle);

	if (!sound)
		return 0;

	if (!initWaveObjectFromDescriptor(info, sound)) {
		removeWaveFromArray(sound);
		return 0;
	}

	return sound->handle;
}

uint16 AudioLibrary_v1_2::audioDestroyWave(uint16 handle) {
	if (handle == 0)
		return 0;

	SoundObject *sound = findWaveObject(handle);

	if (!sound) {
		warning("AudioLibrary_v1_2::audioDestroyWave(): unknown sound %d", handle);
		return 0;
	}

	if (sound->active) {
		_activeSounds--;
		resetWaveOutQueue();
	}

	removeWaveFromArray(sound);
	return 0;
}

uint16 AudioLibrary_v1_2::audioPlayWave(uint16 handle, const byte *params) {
	SoundObject *sound = findWaveObject(handle);

	if (!sound)
		return 0;

	byte defaultParams[TG_AUDIO_V1_2_4_PLAY_PARAMS_SIZE] = {};

	if (!params)
		params = defaultParams;

	if (startWavePlayback(sound, params))
		return 1;

	finishWaveObject(sound);
	return 0;
}

void AudioLibrary_v1_2::audioStopAllWaves(const byte *params) {
	if (_noWaveDevice)
		return;

	if (params && READ_LE_UINT32(params) != 0) {
		audioMasterFadeOut(READ_LE_UINT32(params));
		return;
	}

	resetWaveOutQueue();

	for (uint16 i = 0; i < _soundCount; i++)
		stopWaveObject(_sounds[i], nullptr);
}

uint16 AudioLibrary_v1_2::audioStopWave(uint16 handle, const byte *params) {
	if (_noWaveDevice)
		return 1;

	if (_engine->_version >= kVersionV1_2_3_1 && handle == 0) {
		audioStopAllWaves(params);
		return 1;
	}

	SoundObject *sound = findWaveObject(handle);

	if (!sound)
		return 0;

	stopWaveObject(sound, params);
	return 1;
}

uint16 AudioLibrary_v1_2::audioGetWaveSoundInfo(uint16 handle, byte *info) {
	if (!info)
		return 0;

	SoundObject *sound = findWaveObject(handle);

	if (!sound)
		return 0;

	memset(info, 0, TG_AUDIO_V1_2_SOUND_INFO_SIZE);
	writeWaveFormat(info, sound->format);
	WRITE_LE_UINT32(info + TG_AUDIO_V1_2_SOUND_INFO_LENGTH, sound->sourceLength);

	uint32 decodedLength = sound->sourceLength;

	if (sound->adpcmEncoded)
		decodedLength *= (sound->format.bitsPerSample == 0x10) ? 4 : 2;

	WRITE_LE_UINT32(info + TG_AUDIO_V1_2_SOUND_INFO_DECODED, decodedLength);
	info[TG_AUDIO_V1_2_SOUND_INFO_VOLUME] = sound->volume;
	info[TG_AUDIO_V1_2_SOUND_INFO_ACTIVE] = sound->active ? 1 : 0;
	return 1;
}

uint16 AudioLibrary_v1_2::audioSetWaveSoundVolume(uint16 handle, uint32 volume) {
	SoundObject *sound = findWaveObject(handle);

	if (!sound)
		return 0;

	sound->volume = (byte)volume;

	if (sound->volume > TG_AUDIO_MAX_VOLUME_LEVEL)
		sound->volume = TG_AUDIO_MAX_VOLUME_LEVEL;

	sound->currentVolume = sound->volume;
	return 1;
}

void AudioLibrary_v1_2::audioBreakWaveLoops(uint16 handle) {
	if (_noWaveDevice)
		return;

	SoundObject *sound = findWaveObject(handle);

	if (sound)
		sound->loops = 0;
}

void AudioLibrary_v1_2::audioClearNotifications() {
	for (uint16 i = 0; i < _soundCount; i++) {
		_sounds[i]->notifyPending = false;
		_sounds[i]->notifyArmed = false;
	}
}

AudioLibrary_v1_2::SoundObject *AudioLibrary_v1_2::allocWaveObject() {
	if (_soundCount >= ((_engine->_version >= kVersionV1_2_3_3) ? TG_AUDIO_V1_2_3_3_MAX_SOUNDS : TG_AUDIO_V1_2_MAX_SOUNDS))
		return nullptr;

	SoundObject *sound = new SoundObject();

	do {
		sound->handle = _nextSoundHandle;
		_nextSoundHandle = (_nextSoundHandle == 0xFFFF) ? 1 : _nextSoundHandle + 1;
	} while (findWaveObject(sound->handle));

	_sounds[_soundCount] = sound;
	_soundCount++;
	return sound;
}

uint16 AudioLibrary_v1_2::removeWaveFromArray(SoundObject *sound) {
	uint16 index = 0;

	while (index < _soundCount && _sounds[index] != sound)
		index++;

	if (_soundCount > index) {
		closeSoundSource(sound);
		engine()->utilsLibrary()->tgFree(sound->data);
		delete sound;

		for (uint16 i = index; i + 1 < _soundCount; i++)
			_sounds[i] = _sounds[i + 1];

		_soundCount--;
		_sounds[_soundCount] = nullptr;
	}

	return 0;
}

bool AudioLibrary_v1_2::initWaveObjectFromDescriptor(const SoundCreateInfo_v1_2 *info, SoundObject *sound) {
	UtilsLibrary *utilsLibrary = engine()->utilsLibrary();

	sound->callbackHandle = info->handle;
	sound->readProc = info->readProc;
	sound->rewindProc = info->rewindProc;
	sound->closeProc = info->closeProc;
	uint16 notifyOffset = TG_AUDIO_V1_2_INFO_OFFSET_ID;
	uint16 lengthOffset = TG_AUDIO_V1_2_INFO_OFFSET_LENGTH;
	uint16 formatOffset = 0;
	uint16 flagsOffset = TG_AUDIO_V1_2_INFO_OFFSET_FLAGS;
	uint16 preloadFlag = TG_AUDIO_V1_2_INFO_FLAG_PRELOAD;
	uint16 overlayFlag = TG_AUDIO_V1_2_INFO_FLAG_OVERLAY;
	uint16 adpcmFlag = TG_AUDIO_V1_2_INFO_FLAG_ADPCM;

	if (_engine->_version >= kVersionV1_2_3_1) {
		notifyOffset = TG_AUDIO_V1_2_3_1_INFO_OFFSET_ID;
		lengthOffset = TG_AUDIO_V1_2_3_1_INFO_OFFSET_LENGTH;
		formatOffset = TG_AUDIO_V1_2_3_1_INFO_OFFSET_FORMAT;
		flagsOffset = TG_AUDIO_V1_2_3_1_INFO_OFFSET_FLAGS;
		preloadFlag = TG_AUDIO_V1_2_3_1_INFO_FLAG_PRELOAD;
		overlayFlag = TG_AUDIO_V1_2_3_1_INFO_FLAG_OVERLAY;
		adpcmFlag = TG_AUDIO_V1_2_3_1_INFO_FLAG_ADPCM;
	}

	sound->notifyWParam = READ_LE_UINT16(info->header + notifyOffset);
	sound->sourceLength = READ_LE_UINT32(info->header + lengthOffset);
	readWaveFormat(info->header + formatOffset, sound->format);
	sound->volume = TG_AUDIO_MAX_VOLUME_LEVEL;
	sound->currentVolume = TG_AUDIO_MAX_VOLUME_LEVEL;

	uint16 flags = READ_LE_UINT16(info->header + flagsOffset);

	sound->isStatic = (flags & preloadFlag) != 0;
	sound->overlay = (flags & overlayFlag) != 0;
	sound->enabled = (!_mute && !_cdSuspend);
	sound->adpcmEncoded = (flags & adpcmFlag) != 0;

	if (sound->adpcmEncoded)
		sound->adpcmShift = (sound->format.bitsPerSample == 0x10) ? 2 : 1;

	if (_cdSuspend)
		stopCDIfOwned();

	uint32 bufferRingBytes = (uint32)_maxWaveBuffers << 13;

	if ((sound->sourceLength << sound->adpcmShift) <= bufferRingBytes)
		sound->isStatic = true;

	if (sound->isStatic) {
		sound->size = sound->sourceLength;
	} else {
		sound->size = bufferRingBytes >> sound->adpcmShift;
		sound->lowWater = sound->size >> 1;
	}

	sound->data = utilsLibrary->tgMalloc(sound->size, 0);

	if (!sound->data)
		return false;

	if (!compactAndRefillSoundBuffer(sound))
		return false;

	if (sound->isStatic) {
		if (!closeSoundSource(sound))
			return false;
	}

	return true;
}

AudioLibrary_v1_2::SoundObject *AudioLibrary_v1_2::findWaveObject(uint16 handle) {
	for (uint16 i = 0; i < _soundCount; i++) {
		if (_sounds[i]->handle == handle)
			return _sounds[i];
	}

	return nullptr;
}

bool AudioLibrary_v1_2::doesWaveNeedConversion(const SoundObject *sound) {
	return !(_waveFormat.channels == sound->format.channels && _waveFormat.sampleRate == sound->format.sampleRate &&
	         _waveFormat.bitsPerSample == sound->format.bitsPerSample);
}

void AudioLibrary_v1_2::computeRandomizedRepeatDelay(SoundObject *sound, uint32 now) {
	byte seconds;

	if (sound->randomRange == 0)
		seconds = sound->randomMin;
	else
		seconds = (byte)(((_randomSource.getRandomNumber(0x7FFF) & 0xFF) % sound->randomRange) + sound->randomMin);

	sound->nextTrigger = now + (uint32)seconds * 1000;
}

void AudioLibrary_v1_2::setupWaveFadeIn(SoundObject *sound, uint32 milliseconds, byte volume) {
	uint32 fadeLength = (_waveFormat.averageBytesPerSecond * milliseconds) / 1000;

	if (!_waveOut8Bit)
		fadeLength >>= 1;

	sound->fadeInReload = fadeLength / volume;
	sound->fadeInCount = sound->fadeInReload;

	if (sound->fadeInReload != 0) {
		sound->currentVolume = 0;
		sound->fadingIn = true;
	}
}

void AudioLibrary_v1_2::setupWaveFadeOut(SoundObject *sound, uint32 milliseconds, byte volume, uint16 mode) {
	uint32 fadeLength = (_waveFormat.averageBytesPerSecond * milliseconds) / 1000;

	if (!_waveOut8Bit)
		fadeLength >>= 1;

	sound->fadeOutReload = fadeLength / (uint32)(volume + 1);
	sound->fadeOutCount = sound->fadeOutReload;

	if (sound->fadeOutReload != 0)
		sound->fadingOut = true;

	if (mode == 0)
		return;

	uint32 total = sound->sourceLength << sound->adpcmShift;

	if (!_waveOut8Bit)
		total >>= 1;

	total *= (uint32)(int32)(int16)(sound->loops + 1);

	if (sound->fadingIn)
		total -= sound->fadeInCount * sound->volume;

	if (total >= fadeLength)
		sound->fadeOutCount += total - fadeLength;
}

bool AudioLibrary_v1_2::startWavePlayback(SoundObject *sound, const byte *params) {
	sound->fadingOut = false;
	sound->fadingIn = false;

	if (_engine->_version >= kVersionV1_2_1 && (params[TG_AUDIO_V1_2_PLAY_OFFSET_FLAGS] & TG_AUDIO_V1_2_1_PLAY_FLAG_SET_VOLUME))
		sound->volume = (params[TG_AUDIO_V1_2_PLAY_OFFSET_FLAGS] >> TG_AUDIO_V1_2_1_PLAY_VOLUME_SHIFT) & TG_AUDIO_V1_2_1_PLAY_VOLUME_MASK;

	sound->currentVolume = sound->volume;
	sound->loops = (int16)READ_LE_UINT16(params + TG_AUDIO_V1_2_PLAY_OFFSET_LOOPS);

	if (_engine->_version >= kVersionV1_2_1)
		sound->initialLoops = sound->loops;

	sound->notifyArmed = (params[TG_AUDIO_V1_2_PLAY_OFFSET_FLAGS] & TG_AUDIO_V1_2_PLAY_FLAG_NOTIFY) != 0;
	sound->notifyLParam = sound->notifyArmed ? READ_LE_UINT16(params + TG_AUDIO_V1_2_PLAY_OFFSET_NOTIFY) : 0;
	sound->randomMode = false;

	if (_mute)
		return false;

	if (!audioOpenWave(&sound->format))
		return false;

	sound->needsConversion = doesWaveNeedConversion(sound);

	if (sound->needsConversion)
		convertInitState(&sound->convert, &_waveFormat, &sound->format, sound->sourceLength << sound->adpcmShift);

	if (_masterFadeMode != 0) {
		_masterFadeMode = 0;
		setWaveOutVolume(((uint32)_fadeTargetVolumeRight << 16) | _fadeTargetVolumeLeft);
	}

	if (READ_LE_UINT32(params + TG_AUDIO_V1_2_PLAY_OFFSET_FADE_IN) != 0 && sound->volume != 0)
		setupWaveFadeIn(sound, READ_LE_UINT32(params + TG_AUDIO_V1_2_PLAY_OFFSET_FADE_IN), sound->volume);

	if (READ_LE_UINT32(params + TG_AUDIO_V1_2_PLAY_OFFSET_FADE_OUT) != 0 && sound->volume != 0 && sound->loops >= 0)
		setupWaveFadeOut(sound, READ_LE_UINT32(params + TG_AUDIO_V1_2_PLAY_OFFSET_FADE_OUT), sound->volume, 1);

	rewindSound(sound);

	if (!sound->isStatic && _cdSuspend)
		stopCDIfOwned();

	if (READ_LE_UINT32(params + TG_AUDIO_V1_2_PLAY_OFFSET_RANDOM_MIN) != 0) {
		scheduleRandomRepeatPlayback(sound, params);
		return true;
	}

	// From the later versions the keep flag only protects the output queue
	if (!sound->overlay && (_engine->_version >= kVersionV1_2_1 || !(params[TG_AUDIO_V1_2_PLAY_OFFSET_FLAGS] & TG_AUDIO_V1_2_PLAY_FLAG_KEEP_OTHERS)))
		deactivateAllWaves();

	activateWaveObject(sound);

	if (_buffersInFlight != 0 && !(params[TG_AUDIO_V1_2_PLAY_OFFSET_FLAGS] & TG_AUDIO_V1_2_PLAY_FLAG_KEEP_OTHERS))
		resetWaveOutQueue();

	return true;
}

void AudioLibrary_v1_2::scheduleRandomRepeatPlayback(SoundObject *sound, const byte *params) {
	uint16 minimumOffset = (_engine->_version >= kVersionV1_2_3_2) ? TG_AUDIO_V1_2_3_2_PLAY_OFFSET_RANDOM_MIN : TG_AUDIO_V1_2_PLAY_OFFSET_RANDOM_MIN;
	uint16 maximumOffset = (_engine->_version >= kVersionV1_2_3_2) ? TG_AUDIO_V1_2_3_2_PLAY_OFFSET_RANDOM_MAX : TG_AUDIO_V1_2_PLAY_OFFSET_RANDOM_MAX;

	sound->randomMode = true;

	if (sound->active)
		_activeSounds--;

	sound->active = false;

	byte minimumSeconds = (byte)(READ_LE_UINT32(params + minimumOffset) / 1000);
	byte maximumSeconds = (byte)(READ_LE_UINT32(params + maximumOffset) / 1000);

	if (minimumSeconds < maximumSeconds)
		sound->randomRange = (byte)(maximumSeconds - minimumSeconds + 1);
	else
		sound->randomRange = 0;

	sound->randomMin = minimumSeconds;
	computeRandomizedRepeatDelay(sound, _engine->getEngineMillis());
}

void AudioLibrary_v1_2::activateWaveObject(SoundObject *sound) {
	if (_engine->_version >= kVersionV1_2_1)
		sound->loops = sound->initialLoops;

	if (!sound->active)
		_activeSounds++;

	sound->active = true;
}

void AudioLibrary_v1_2::deactivateAllWaves() {
	for (uint16 i = 0; i < _soundCount; i++) {
		SoundObject *sound = _sounds[i];

		if (sound->active && (_engine->_version < kVersionV1_2_1 || !sound->overlay)) {
			_activeSounds--;
			sound->randomMode = false;
			sound->active = false;
		}
	}
}

void AudioLibrary_v1_2::finishWaveObject(SoundObject *sound) {
	if (sound->active) {
		sound->active = false;
		_activeSounds--;
	}

	if (sound->randomMode) {
		computeRandomizedRepeatDelay(sound, _engine->getEngineMillis());
		return;
	}

	if (sound->notifyArmed && _hwnd != 0) {
		if (_engine->_version >= kVersionV1_2_3_3) {
			sound->notifyPending = true;
		} else {
			engine()->postMessage(TG_AUDIO_V1_2_NOTIFY_MESSAGE, sound->notifyWParam, sound->notifyLParam);
		}

		sound->notifyArmed = false;
	}
}

void AudioLibrary_v1_2::stopWaveObject(SoundObject *sound, const byte *params) {
	if (_engine->_version >= kVersionV1_2_1) {
		sound->notifyPending = false;
		sound->notifyArmed = false;
	}

	sound->randomMode = false;

	if (!sound->active)
		return;

	uint32 fadeMilliseconds = params ? READ_LE_UINT32(params) : 0;

	if (fadeMilliseconds != 0 && sound->currentVolume != 0) {
		setupWaveFadeOut(sound, fadeMilliseconds, sound->currentVolume, 0);
	} else {
		if (_engine->_version >= kVersionV1_2_1) {
			finishWaveObject(sound);
		} else {
			sound->active = false;
			_activeSounds--;
		}

		resetWaveOutQueue();
	}

	pumpAudioPlayback();
}

bool AudioLibrary_v1_2::soundEndOrLoop(SoundObject *sound) {
	if (sound->loops == 0) {
		finishWaveObject(sound);
		return false;
	}

	if (sound->loops > 0)
		sound->loops--;

	if (sound->adpcmEncoded)
		sound->adpcm = AdpcmState();

	if (sound->isStatic) {
		sound->available = sound->size;
		sound->readPos = 0;
	}

	return true;
}

uint16 AudioLibrary_v1_2::rewindSourceStream(SoundObject *sound) {
	if (sound->rewindProc == 0)
		return 0;

	return (uint16)engine()->audioSourceRewind(sound->rewindProc, sound->callbackHandle);
}

void AudioLibrary_v1_2::prepareWaveChunk(SoundObject *sound, uint16 outputBytes) {
	sound->chunkLength = 0;

	if (sound->sourceLength <= sound->consumed) {
		if (!soundEndOrLoop(sound))
			return;

		sound->consumed = 0;
	}

	if (sound->available == 0) {
		if (!compactAndRefillSoundBuffer(sound))
			return;
	}

	uint32 sourceBytes = computeSourceBytesForOutput(sound, outputBytes);

	if (sourceBytes == 0)
		return;

	if (sound->adpcmEncoded) {
		if (!ensureAdpcmScratchBuffer(sound, sourceBytes))
			return;
	}

	if (sound->needsConversion) {
		if (!ensureResampleScratchBuffer(outputBytes))
			return;
	}

	sound->chunk = sound->data + sound->readPos;
	sound->chunkLength = sourceBytes;
	sound->available -= sourceBytes;
	sound->consumed += sourceBytes;
	sound->readPos += sourceBytes;

	if (sound->adpcmEncoded) {
		decodeImaAdpcm(sound->format.channels == 2, sourceBytes, sound->chunk, _adpcmScratch, &sound->adpcm);
		sound->chunkLength = sourceBytes << sound->adpcmShift;
		sound->chunk = _adpcmScratch;
	}

	if (sound->needsConversion) {
		uint32 produced = convertSampleRate(&sound->convert, _resampleScratch, _resampleScratchSize, sound->chunk,
		                                    sound->chunkLength, nullptr);

		sound->chunkLength = (uint16)produced;
		sound->chunk = _resampleScratch;
	}
}

void AudioLibrary_v1_2::rewindSound(SoundObject *sound) {
	if (sound->adpcmEncoded)
		sound->adpcm = AdpcmState();

	if (sound->isStatic) {
		sound->available = sound->size;
		sound->consumed = 0;
		sound->readPos = 0;
		return;
	}

	if (sound->consumed == 0)
		return;

	if ((_engine->_version >= kVersionV1_2_3_1) ? (_cdDeviceId != 0) : _cdSuspend)
		stopCDIfOwned();

	sound->available = 0;
	sound->readFromSource = 0;
	sound->consumed = 0;
	sound->endOfSource = false;
	rewindSourceStream(sound);
	compactAndRefillSoundBuffer(sound);
}

uint16 AudioLibrary_v1_2::compactAndRefillSoundBuffer(SoundObject *sound) {
	if (sound->readPos != 0) {
		if (sound->available != 0)
			engine()->utilsLibrary()->copyHugeBytes(sound->data, sound->data + sound->readPos, sound->available);

		sound->readPos = 0;
	}

	uint32 wanted = sound->size - sound->available;

	if (wanted == 0)
		return 0;

	uint32 remaining = sound->sourceLength - sound->readFromSource;

	if (remaining <= wanted) {
		sound->endOfSource = true;
		wanted = remaining;
	}

	uint32 delivered = engine()->audioSourceRead(sound->readProc, sound->callbackHandle, sound->data + sound->available, wanted);

	if (delivered != wanted)
		return 0;

	sound->available += wanted;
	sound->readFromSource += wanted;

	if (!sound->isStatic && sound->endOfSource && sound->loops != 0) {
		sound->readFromSource = 0;
		sound->endOfSource = false;
		return rewindSourceStream(sound);
	}

	return 1;
}

bool AudioLibrary_v1_2::closeSoundSource(SoundObject *sound) {
	if (sound->closeProc != 0) {
		if ((_engine->_version >= kVersionV1_2_3_1) ? (_cdDeviceId != 0) : _cdSuspend)
			stopCDIfOwned();

		engine()->audioSourceClose(sound->closeProc, sound->callbackHandle);
		sound->closeProc = 0;
	}

	return true;
}

bool AudioLibrary_v1_2_4::initWaveObjectFromDescriptor(const SoundCreateInfo_v1_2 *info, SoundObject *sound) {
	UtilsLibrary *utilsLibrary = engine()->utilsLibrary();

	sound->callbackHandle = info->handle;
	sound->readProc = info->readProc;
	sound->rewindProc = info->rewindProc;
	sound->closeProc = info->closeProc;
	uint16 notifyOffset = TG_AUDIO_V1_2_3_2_INFO_OFFSET_NOTIFY;
	uint16 formatOffset = TG_AUDIO_V1_2_3_2_INFO_OFFSET_FORMAT;

	if (_engine->_version >= kVersionV1_2_4) {
		notifyOffset = TG_AUDIO_V1_2_4_INFO_OFFSET_NOTIFY;
		formatOffset = TG_AUDIO_V1_2_4_INFO_OFFSET_FORMAT;
	}

	sound->notifyWParam = READ_LE_UINT16(info->header + notifyOffset);
	sound->sourceLength = READ_LE_UINT32(info->header + TG_AUDIO_V1_2_3_2_INFO_OFFSET_LENGTH);
	readWaveFormat(info->header + formatOffset, sound->format);
	sound->volume = TG_AUDIO_MAX_VOLUME_LEVEL;
	sound->currentVolume = TG_AUDIO_MAX_VOLUME_LEVEL;
	sound->enabled = (_cdDeviceId == 0);
	sound->isStatic = (info->header[TG_AUDIO_V1_2_3_2_INFO_OFFSET_PRELOAD] != 0);

	if (_engine->_version < kVersionV1_2_3_3 || (info->header[TG_AUDIO_V1_2_3_2_INFO_OFFSET_EXEMPT] != 0 && _engine->_version < kVersionV1_2_7))
		sound->priority = 0;
	else if (_engine->_version < kVersionV1_2_4)
		sound->priority = info->header[TG_AUDIO_V1_2_3_3_INFO_OFFSET_PRIORITY];
	else
		sound->priority = (int16)READ_LE_UINT16(info->header + TG_AUDIO_V1_2_4_INFO_OFFSET_PRIORITY);

	sound->exempt = (info->header[TG_AUDIO_V1_2_3_2_INFO_OFFSET_EXEMPT] != 0);
	sound->adpcmEncoded = (info->header[TG_AUDIO_V1_2_3_2_INFO_OFFSET_ADPCM] != 0);

	if (sound->adpcmEncoded)
		sound->adpcmShift = (sound->format.bitsPerSample == 0x10) ? 2 : 1;

	if (_cdDeviceId != 0)
		stopCDIfOwned();

	uint32 bufferRingBytes = (uint32)_maxWaveBuffers << 13;

	if ((sound->sourceLength << sound->adpcmShift) <= bufferRingBytes)
		sound->isStatic = true;

	if (sound->isStatic) {
		sound->size = sound->sourceLength;
	} else {
		sound->size = bufferRingBytes >> sound->adpcmShift;
		sound->lowWater = sound->size >> 1;
	}

	sound->data = utilsLibrary->tgMalloc(sound->size, 0);

	if (!sound->data)
		return false;

	if (!compactAndRefillSoundBuffer(sound))
		return false;

	if (sound->isStatic)
		closeSoundSource(sound);

	return true;
}

bool AudioLibrary_v1_2_4::startWavePlayback(SoundObject *sound, const byte *params) {
	bool busy = (_engine->_version == kVersionV1_2_5) ? (_activeSounds != 0) : (_buffersInFlight != 0);

	if (_engine->_version < kVersionV1_2_4)
		busy = true;

	bool exemptFromPriority = sound->exempt;

	if (_engine->_version >= kVersionV1_2_7)
		exemptFromPriority = (sound->exempt && sound->priority == 0);

	if (!exemptFromPriority && busy && _maxPriority > sound->priority)
		return true;

	sound->fadingOut = false;
	sound->fadingIn = false;
	sound->notifyPending = false;

	if (params[TG_AUDIO_V1_2_3_2_PLAY_OFFSET_SET_VOLUME] != 0)
		sound->volume = params[TG_AUDIO_V1_2_3_2_PLAY_OFFSET_VOLUME];

	sound->currentVolume = sound->volume;
	sound->loops = (int16)READ_LE_UINT16(params + TG_AUDIO_V1_2_3_2_PLAY_OFFSET_LOOPS);
	sound->initialLoops = sound->loops;

	if (!openWaveDevice(&sound->format))
		return false;

	sound->needsConversion = doesWaveNeedConversion(sound);

	if (sound->needsConversion)
		convertInitState(&sound->convert, &_waveFormat, &sound->format, sound->sourceLength << sound->adpcmShift);

	if (_masterFadeMode != 0) {
		_masterFadeMode = 0;
		setWaveOutVolume(((uint32)_fadeTargetVolumeRight << 16) | _fadeTargetVolumeLeft);
	}

	uint32 fadeInMilliseconds = READ_LE_UINT32(params + TG_AUDIO_V1_2_3_2_PLAY_OFFSET_FADE_IN);

	if (fadeInMilliseconds != 0 && sound->volume != 0)
		setupWaveFadeIn(sound, fadeInMilliseconds, sound->volume);

	uint32 fadeOutMilliseconds = READ_LE_UINT32(params + TG_AUDIO_V1_2_3_2_PLAY_OFFSET_FADE_OUT);

	if (fadeOutMilliseconds != 0 && sound->volume != 0 && sound->loops >= 0)
		setupWaveFadeOut(sound, fadeOutMilliseconds, sound->volume, 1);

	rewindSound(sound);

	if (_cdDeviceId != 0 && !sound->isStatic)
		stopCDIfOwned();

	sound->randomMode = false;

	if (READ_LE_UINT32(params + TG_AUDIO_V1_2_3_2_PLAY_OFFSET_RANDOM_MIN) != 0) {
		scheduleRandomRepeatPlayback(sound, params);
		return true;
	}

	if (!sound->exempt)
		stopAllNonExemptWaves();

	if (_buffersInFlight != 0 && params[TG_AUDIO_V1_2_3_2_PLAY_OFFSET_NO_FLUSH] == 0)
		resetWaveOutQueue();

	activateWaveObject(sound);
	sound->notifyArmed = (params[TG_AUDIO_V1_2_3_2_PLAY_OFFSET_NOTIFY] != 0);

	if (sound->notifyArmed)
		sound->notifyLParam = READ_LE_UINT16(params + TG_AUDIO_V1_2_3_2_PLAY_OFFSET_NOTIFY_PARAM);
	else
		sound->notifyLParam = 0;

	if (_engine->_version >= kVersionV1_2_7 || sound->notifyArmed)
		armWaveNotificationEndTime(sound);

	return true;
}

void AudioLibrary_v1_2_4::stopAllNonExemptWaves() {
	for (uint16 i = 0; i < _soundCount; i++) {
		SoundObject *sound = _sounds[i];

		if (sound->active && !sound->exempt) {
			_activeSounds--;
			sound->randomMode = false;
			sound->active = false;
		}
	}

	_maxPriority = 0;
}

void AudioLibrary_v1_2_4::activateWaveObject(SoundObject *sound) {
	if (!sound->exempt || (_engine->_version >= kVersionV1_2_7 && sound->priority != 0))
		_maxPriority = sound->priority;

	sound->loops = sound->initialLoops;

	if (!sound->active)
		_activeSounds++;

	sound->active = true;
}

uint32 AudioLibrary_v1_2_4::audioGetWaveSoundTime(uint16 handle) {
	SoundObject *sound = findWaveObject(handle);

	if (!sound)
		return 0;

	uint32 bytesPerTenthSecond = sound->format.averageBytesPerSecond / 10;

	if (bytesPerTenthSecond == 0) {
		warning("AudioLibrary_v1_2_4::audioGetWaveSoundTime(): the average byte rate of sound %d is below 10", sound->handle);
		return 0;
	}

	return ((sound->sourceLength * 100) << sound->adpcmShift) / bytesPerTenthSecond;
}

void AudioLibrary_v1_2_4::armWaveNotificationEndTime(SoundObject *sound) {
	uint32 duration = 0;

	if (_engine->_version >= kVersionV1_2_5) {
		uint32 bytesPerTenthSecond = sound->format.averageBytesPerSecond / 10;

		if (bytesPerTenthSecond != 0) {
			uint32 length = sound->sourceLength * 100;

			if (_engine->_version >= kVersionV1_2_7)
				length <<=sound->adpcmShift;

			duration = length / bytesPerTenthSecond;
		} else {
			warning("AudioLibrary_v1_2_4::armWaveNotificationEndTime(): the average byte rate of sound %d is below 10", sound->handle);
		}
	} else {
		if (sound->format.averageBytesPerSecond != 0)
			duration = (sound->sourceLength * 1000) / sound->format.averageBytesPerSecond;
		else
			warning("AudioLibrary_v1_2_4::armWaveNotificationEndTime(): the average byte rate of sound %d is zero", sound->handle);
	}

	sound->notifyEndTime = _engine->getEngineMillis() + duration;
}

} // End of namespace Topgun

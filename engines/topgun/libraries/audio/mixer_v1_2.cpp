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

#include "topgun/libraries/audio/audio_v1_2_4.h"
#include "topgun/libraries/utils/utils.h"
#include "topgun/topgun_v1_2_4.h"

namespace Topgun {

bool AudioLibrary_v1_2::probeWaveOutCapabilities() {
	if (!g_system->getMixer()->isReady()) {
		_noWaveDevice = true;
		return true;
	}

	_waveOutCapsFormats = TG_WAVEOUT_V1_2_ALL_FORMATS;
	_waveOutCapsSupport = 0;

	if (_waveOutCapsSupport & TG_WAVEOUT_V1_2_SUPPORT_SYNC)
		_waveDeviceIsSync = true;

	return true;
}

int16 AudioLibrary_v1_2::findWaveFormatIndexExact(uint16 mask, const WaveFormat *format) {
	for (uint16 i = 0; i < TG_AUDIO_NUM_OUTPUT_FORMATS; i++) {
		if ((mask & kFormatBits[i]) == 0)
			continue;

		if (kFormatChannels[i] != format->channels)
			continue;

		if (kFormatBytesPerSample[i] * 8 != format->bitsPerSample)
			continue;

		if (kFormatRates[i] != format->sampleRate)
			continue;

		return (int16)i;
	}

	return -1;
}

int16 AudioLibrary_v1_2::findBestWaveFormatIndex(const WaveFormat *format) {
	uint16 mask = (_userFormatMask != 0) ? _userFormatMask : _iniWaveFormatMask;

	mask &= (uint16)(_waveOutCapsFormats & 0xFFFF);

	if (mask == 0)
		return -1;

	if (format) {
		int16 index = findWaveFormatIndexExact(mask, format);

		if (index >= 0)
			return index;

		WaveFormat retry = *format;

		if (retry.bitsPerSample == 16) {
			retry.bitsPerSample = 8;
			index = findWaveFormatIndexExact(mask, &retry);

			if (index >= 0)
				return index;
		}
	}

	for (uint16 i = 0; i < TG_AUDIO_NUM_OUTPUT_FORMATS; i++) {
		if (kFormatBits[i] & mask)
			return (int16)i;
	}

	return -1;
}

bool AudioLibrary_v1_2::applyWaveFormatIndex(const WaveFormat *format) {
	int16 index = findBestWaveFormatIndex(format);

	if (index == -1)
		return false;

	_chosenFormatMask = kFormatBits[index];
	_waveFormat.tag = 1;
	_waveFormat.channels = kFormatChannels[index];
	_waveFormat.sampleRate = kFormatRates[index];
	_waveFormat.bitsPerSample = kFormatBytesPerSample[index] * 8;
	_waveFormat.blockAlign = (byte)((_waveFormat.bitsPerSample >> 3) * kFormatChannels[index]);
	_waveFormat.averageBytesPerSecond = (uint32)kFormatRates[index] * _waveFormat.blockAlign;
	return true;
}

bool AudioLibrary_v1_2::allocateWaveOutBuffers() {
	Common::StackLock lock(g_system->getMixer()->mutex());

	for (uint16 i = 0; i < _maxWaveBuffers; i++) {
		WaveRingEntry &entry = _waveRing[i];

		entry.data = new byte[TG_WAVEOUT_V1_2_BUFFER_SIZE];
		memset(entry.data, 0, TG_WAVEOUT_V1_2_BUFFER_SIZE);
		entry.length = TG_WAVEOUT_V1_2_BUFFER_SIZE;
		entry.flags = TG_WAVEOUT_V1_2_HEADER_PREPARED;
		entry.busy = false;
	}

	return true;
}

void AudioLibrary_v1_2::freeWaveOutBuffers() {
	UtilsLibrary *utilsLibrary = engine()->utilsLibrary();

	{
		Common::StackLock lock(g_system->getMixer()->mutex());

		for (uint16 i = 0; i < TG_AUDIO_V1_2_MAX_WAVE_BUFFERS; i++) {
			WaveRingEntry &entry = _waveRing[i];

			delete[] entry.data;
			entry.data = nullptr;
			entry.length = 0;
			entry.flags = 0;
			entry.busy = false;
		}
	}

	_adpcmScratch = utilsLibrary->tgFree(_adpcmScratch);
	_adpcmScratchSize = 0;
	_resampleScratch = utilsLibrary->tgFree(_resampleScratch);
	_resampleScratchSize = 0;
}

bool AudioLibrary_v1_2::finishWaveOutOpen() {
	if (!allocateWaveOutBuffers())
		return false;

	_waveOut8Bit = (_waveFormat.bitsPerSample == 8);
	_ringTail = 0;
	_ringHead = 0;

	if (!_disableWaveVolume) {
		_savedWaveVolumeLeft = (uint16)(_hardwareWaveVolume & 0xFFFF);
		_savedWaveVolumeRight = (uint16)(_hardwareWaveVolume >> 16);
	}

	_waveVolumeLeft = _savedWaveVolumeLeft;
	_waveVolumeRight = _savedWaveVolumeRight;
	return true;
}

bool AudioLibrary_v1_2::finishWaveOutOpenSync() {
	if (!allocateWaveOutBuffers())
		return false;

	_waveOut8Bit = (_waveFormat.bitsPerSample == 8);
	_ringTail = 0;
	_ringHead = 0;

	if (!_disableWaveVolume) {
		_savedWaveVolumeLeft = (uint16)(_hardwareWaveVolume & 0xFFFF);
		_savedWaveVolumeRight = (uint16)(_hardwareWaveVolume >> 16);
	}

	_waveVolumeLeft = _savedWaveVolumeLeft;
	_waveVolumeRight = _savedWaveVolumeRight;
	return true;
}

bool AudioLibrary_v1_2::openWaveOutDevice(const WaveFormat *format) {
	if (_noWaveDevice)
		return false;

	if (!format)
		return false;

	if (_waveOutStream && _activeSounds != 0)
		return true;

	if (_waveOutStream && _waveFormat.channels == format->channels && _waveFormat.bitsPerSample == format->bitsPerSample &&
		_waveFormat.sampleRate == format->sampleRate)
		return true;

	if (_waveOutStream)
		audioCloseWave();

	if (_activeSounds == 0) {
		if (!applyWaveFormatIndex(format))
			return false;
	}

	Audio::Mixer *mixer = g_system->getMixer();

	if (!mixer->isReady()) {
		warning("AudioLibrary_v1_2::openWaveOutDevice(): the mixer is not ready");
		return false;
	}

	_waveOutStream = new WaveOutStream(_waveRing, _waveFormat.sampleRate, _waveFormat.channels == 2,
	                                   _waveFormat.bitsPerSample == 8, _maxWaveBuffers);
	mixer->playStream(Audio::Mixer::kPlainSoundType, &_waveOutHandle, _waveOutStream, -1,
	                  Audio::Mixer::kMaxChannelVolume, 0, DisposeAfterUse::NO);
	applyWaveOutVolume(_hardwareWaveVolume);

	bool opened = _waveDeviceIsSync ? finishWaveOutOpenSync() : finishWaveOutOpen();

	if (!opened)
		audioCloseWave();

	return opened;
}

void AudioLibrary_v1_2::audioCloseWave() {
	if (!_waveOutStream)
		return;

	audioStopAllWaves(nullptr);

	if (_savedWaveVolumeLeft != _waveVolumeLeft || _savedWaveVolumeRight != _waveVolumeRight)
		setWaveOutVolume(((uint32)_savedWaveVolumeRight << 16) | _savedWaveVolumeLeft);

	freeWaveOutBuffers();
	g_system->getMixer()->stopHandle(_waveOutHandle);
	delete _waveOutStream;
	_waveOutStream = nullptr;
}

uint16 AudioLibrary_v1_2::audioOpenWave(const WaveFormat *format) {
	if (!openWaveOutDevice(format)) {
		if (_engine->_version >= kVersionV1_2_1)
			audioCloseWave();

		return 0;
	}

	if (_userVolumeSet)
		setWaveOutVolume(_userVolume);

	return 1;
}

void AudioLibrary_v1_2::destroyAllWaveObjectsAndCloseDevice() {
	uint16 index = 0;

	while (index < _soundCount) {
		audioDestroyWave(_sounds[index]->handle);
		index++;
	}

	audioCloseWave();
}

void AudioLibrary_v1_2::resetWaveOutQueue() {
	if (!_waveOutStream || _buffersInFlight == 0)
		return;

	{
		Common::StackLock lock(g_system->getMixer()->mutex());

		for (uint16 i = 0; i < _maxWaveBuffers; i++) {
			if (_waveRing[i].flags & TG_WAVEOUT_HEADER_IN_QUEUE)
				_waveRing[i].flags = (_waveRing[i].flags & ~(uint32)TG_WAVEOUT_HEADER_IN_QUEUE) | TG_WAVEOUT_HEADER_DONE;
		}

		_waveOutStream->resetPlayback();
	}

	// Resetting the device also lifts its pause
	g_system->getMixer()->pauseHandle(_waveOutHandle, false);

	for (uint16 i = 0; i < _maxWaveBuffers; i++)
		_waveRing[i].busy = false;

	_ringTail = 0;
	_ringHead = 0;
	_buffersInFlight = 0;
}

void AudioLibrary_v1_2::retireBuffer() {
	bool done;

	{
		Common::StackLock lock(g_system->getMixer()->mutex());
		done = (_waveRing[_ringTail].flags & TG_WAVEOUT_HEADER_DONE) != 0;
	}

	if (!done)
		return;

	_buffersInFlight--;
	_waveRing[_ringTail].busy = false;
	_ringTail++;

	if (_ringTail >= _maxWaveBuffers)
		_ringTail = 0;
}

bool AudioLibrary_v1_2::retireBufferAndRefill() {
	if (_buffersInFlight != 0)
		retireBuffer();

	if (_activeSounds == 0)
		return false;

	if (_waveRing[_ringHead].busy)
		return false;

	uint16 mixedSounds = _waveOut8Bit ? mixAllWaves8Bit() : mixAllWaves16Bit();

	if (mixedSounds == 0)
		return false;

	WaveRingEntry &entry = _waveRing[_ringHead];

	entry.busy = true;

	{
		Common::StackLock lock(g_system->getMixer()->mutex());

		entry.flags &= ~(uint32)(TG_WAVEOUT_HEADER_DONE | TG_WAVEOUT_HEADER_BEGIN_LOOP | TG_WAVEOUT_HEADER_END_LOOP);
		entry.flags |= TG_WAVEOUT_HEADER_IN_QUEUE;
	}

	_buffersInFlight++;
	_ringHead++;

	if (_ringHead >= _maxWaveBuffers)
		_ringHead = 0;

	return true;
}

uint16 AudioLibrary_v1_2::mixAllWaves16Bit() {
	uint16 mixedSounds = 0;
	uint16 maxBytes = 0;
	byte *destination = _waveRing[_ringHead].data;

	for (uint16 i = 0; i < _soundCount; i++) {
		SoundObject *sound = _sounds[i];

		if (sound->active && sound->enabled) {
			mixedSounds++;

			uint16 produced = mixSound16Bit(mixedSounds, TG_WAVEOUT_V1_2_BUFFER_SIZE, destination, sound);

			if (produced > maxBytes)
				maxBytes = produced;

			if (_engine->_version >= kVersionV1_2_7 && _maxPriority < sound->priority)
				_maxPriority = sound->priority;
		}
	}

	_waveRing[_ringHead].length = maxBytes;
	return mixedSounds;
}

uint16 AudioLibrary_v1_2::mixAllWaves8Bit() {
	uint16 mixedSounds = 0;
	uint16 maxBytes = 0;
	byte *destination = _waveRing[_ringHead].data;

	for (uint16 i = 0; i < _soundCount; i++) {
		SoundObject *sound = _sounds[i];

		if (sound->active && sound->enabled) {
			mixedSounds++;

			uint16 produced = mixSound8Bit(mixedSounds, TG_WAVEOUT_V1_2_BUFFER_SIZE, destination, sound);

			if (produced > maxBytes)
				maxBytes = produced;

			if (_engine->_version >= kVersionV1_2_7 && _maxPriority < sound->priority)
				_maxPriority = sound->priority;
		}
	}

	_waveRing[_ringHead].length = maxBytes;
	return mixedSounds;
}

void AudioLibrary_v1_2::pumpAudioPlayback() {
	if (!_waveOutStream)
		return;

	if (_soundCount == 0)
		return;

	if (_masterFadeMode != 0)
		masterFadeTick();

	if (retireBufferAndRefill())
		return;

	if (_cdSuspend)
		return;

	for (uint16 i = 0; i < _soundCount; i++) {
		SoundObject *sound = _sounds[i];

		if (!sound->enabled)
			continue;

		if (!sound->isStatic && !sound->endOfSource && sound->available <= sound->lowWater)
			compactAndRefillSoundBuffer(sound);

		if (sound->randomMode && !sound->active && _engine->getEngineMillis() >= sound->nextTrigger) {
			if (_engine->_version >= kVersionV1_2_1)
				rewindSound(sound);

			activateWaveObject(sound);
		}
	}
}

void AudioLibrary_v1_2::resetWaveQueueAndEnableAllWaves(uint16 on) {
	_cdSuspend = (on != 0);

	if (on != 0)
		resetWaveOutQueue();

	updateWaveEnabledFlags(1);

	if (_engine->_version >= kVersionV1_2_3_1)
		updateMidiEnabledFlags(1);
}

void AudioLibrary_v1_2::updateWaveEnabledFlags(uint16 on) {
	if (_mute)
		on = 0;

	bool cdOpen = _cdSuspend;

	if (_engine->_version >= kVersionV1_2_3_1)
		cdOpen = (_cdDeviceId != 0);

	for (uint16 i = 0; i < _soundCount; i++) {
		SoundObject *sound = _sounds[i];

		if (on != 0 && (!cdOpen || (sound->isStatic && (_engine->_version >= kVersionV1_2_1 || sound->overlay))))
			sound->enabled = true;
		else
			sound->enabled = false;
	}
}

void AudioLibrary_v1_2::pauseAllPlayback(uint16 paused) {
	if (!_waveOutStream)
		return;

	if (_engine->_version < kVersionV1_2_3_2 && _buffersInFlight == 0)
		return;

	// The mixer counts pauses but the original device does not, and the game can pause twice in a row
	if (_engine->_version >= kVersionV1_2_3_1 && _engine->_version < kVersionV1_2_4 && paused != 0)
		g_system->getMixer()->pauseHandle(_waveOutHandle, false);

	g_system->getMixer()->pauseHandle(_waveOutHandle, paused != 0);
}

uint16 AudioLibrary_v1_2::mixSound8Bit(uint16 mixCount, uint16 bytes, byte *destination, SoundObject *sound) {
	uint16 requested = bytes;
	uint16 remaining = bytes;

	for (;;) {
		prepareWaveChunk(sound, remaining);

		uint16 chunkLength = (uint16)sound->chunkLength;

		if (chunkLength == 0)
			break;

		remaining -= chunkLength;

		const byte *source = sound->chunk;

		if (sound->currentVolume == TG_AUDIO_MAX_VOLUME_LEVEL && !sound->fadingIn && !sound->fadingOut) {
			if (mixCount == 1) {
				memcpy(destination, source, chunkLength);
			} else {
				for (uint16 i = 0; i < chunkLength; i++) {
					int32 value = (int32)source[i] + destination[i] - 0x100;

					if (value > 0x7F)
						value = 0xFF;
					else if (value < -128)
						value = 0;
					else
						value += 0x80;

					destination[i] = (byte)value;
				}
			}

			destination += chunkLength;
		} else {
			for (uint16 i = 0; i < chunkLength; i++) {
				int32 value = source[i];

				if (sound->currentVolume != TG_AUDIO_MAX_VOLUME_LEVEL)
					value = (value * (int8)sound->currentVolume) >> 4;

				if (mixCount != 1) {
					value += *destination;
					value -= 0x100;

					if (value > 0x7F)
						value = 0xFF;
					else if (value < -128)
						value = 0;
					else
						value += 0x80;
				}

				*destination = (byte)value;
				destination++;

				if (sound->fadingIn) {
					sound->fadeInCount--;

					if (sound->fadeInCount == 0) {
						sound->fadeInCount = sound->fadeInReload;
						sound->currentVolume++;

						if (sound->currentVolume >= sound->volume)
							sound->fadingIn = false;
					}
				} else if (sound->fadingOut) {
					sound->fadeOutCount--;

					if (sound->fadeOutCount == 0) {
						sound->fadeOutCount = sound->fadeOutReload;
						sound->currentVolume--;

						if (sound->currentVolume == 0)
							sound->fadingOut = false;
					}
				}
			}
		}

		if (remaining == 0)
			break;

		if (!sound->active)
			break;
	}

	mixCount--;

	if (mixCount == 0 && remaining != 0)
		memset(destination, 0x80, remaining);

	return requested - remaining;
}

uint16 AudioLibrary_v1_2::mixSound16Bit(uint16 mixCount, uint16 bytes, byte *destination, SoundObject *sound) {
	uint16 requested = bytes;
	uint16 remaining = bytes;

	for (;;) {
		prepareWaveChunk(sound, remaining);

		uint16 chunkLength = (uint16)sound->chunkLength;

		if (chunkLength == 0)
			break;

		remaining -= chunkLength;

		const byte *source = sound->chunk;
		uint16 samples = chunkLength >> 1;

		if (samples == 0)
			warning("AudioLibrary_v1_2::mixSound16Bit(): chunk of a single byte");

		if (sound->currentVolume == TG_AUDIO_MAX_VOLUME_LEVEL && !sound->fadingIn && !sound->fadingOut) {
			if (mixCount == 1) {
				memcpy(destination, source, samples * 2);
			} else {
				for (uint16 i = 0; i < samples; i++) {
					int32 value = READ_LE_INT16(source + i * 2) + READ_LE_INT16(destination + i * 2);

					if (value > 0x7FFF)
						value = 0x7FFF;
					else if (value < -0x7FFF)
						value = -0x7FFF;

					WRITE_LE_UINT16(destination + i * 2, (uint16)value);
				}
			}

			destination += samples * 2;
		} else {
			for (uint16 i = 0; i < samples; i++) {
				int32 value = READ_LE_INT16(source + i * 2);

				if (sound->currentVolume != TG_AUDIO_MAX_VOLUME_LEVEL)
					value = (value * (int8)sound->currentVolume) >> 4;

				if (mixCount != 1) {
					value += READ_LE_INT16(destination);

					if (value > 0x7FFF)
						value = 0x7FFF;
					else if (value < -0x7FFF)
						value = -0x7FFF;
				}

				WRITE_LE_UINT16(destination, (uint16)value);
				destination += 2;

				if (sound->fadingIn) {
					sound->fadeInCount--;

					if (sound->fadeInCount == 0) {
						sound->fadeInCount = sound->fadeInReload;
						sound->currentVolume++;

						if (sound->currentVolume >= sound->volume)
							sound->fadingIn = false;
					}
				} else if (sound->fadingOut) {
					sound->fadeOutCount--;

					if (sound->fadeOutCount == 0) {
						sound->fadeOutCount = sound->fadeOutReload;
						sound->currentVolume--;

						if (sound->currentVolume == 0)
							sound->fadingOut = false;
					}
				}
			}
		}

		if (remaining == 0)
			break;

		if (!sound->active)
			break;
	}

	mixCount--;

	if (mixCount == 0) {
		uint16 silentSamples = remaining >> 1;

		if (silentSamples != 0)
			memset(destination, 0, silentSamples * 2);
	}

	return requested - remaining;
}

void AudioLibrary_v1_2_4::resetWaveQueueAndEnableAllWaves(uint16 on) {
	if (on != 0)
		resetWaveOutQueue();

	updateWaveEnabledFlags(1);
}

void AudioLibrary_v1_2_4::resetWaveOutQueue() {
	if (!_waveOutStream || _buffersInFlight == 0)
		return;

	AudioLibrary_v1_2::resetWaveOutQueue();
	_maxPriority = 0;
}

void AudioLibrary_v1_2_4::retireBuffer() {
	bool done;

	{
		Common::StackLock lock(g_system->getMixer()->mutex());
		done = (_waveRing[_ringTail].flags & TG_WAVEOUT_HEADER_DONE) != 0;
	}

	if (!done)
		return;

	_waveRing[_ringTail].busy = false;
	_buffersInFlight--;

	if (_buffersInFlight == 0 && _engine->_version < kVersionV1_2_7)
		_maxPriority = 0;

	_ringTail++;

	if (_ringTail >= _maxWaveBuffers)
		_ringTail = 0;
}

bool AudioLibrary_v1_2_4::finishWaveOutOpen() {
	if (!AudioLibrary_v1_2::finishWaveOutOpen())
		return false;

	if (_engine->_version >= kVersionV1_2_7 && _iniWaveVolume != 0) {
		_userVolume = ((uint32)_iniWaveVolume << 16) | _iniWaveVolume;
		setWaveOutVolume(_userVolume);
	}

	if (_engine->_version >= kVersionV1_2_4 && _mute)
		waveMuteImpl();

	return true;
}

bool AudioLibrary_v1_2_4::openWaveOutDevice(const WaveFormat *format) {
	WaveFormat defaultFormat;

	if (!format && _engine->_version >= kVersionV1_2_4) {
		defaultFormat.tag = TG_AUDIO_V1_2_4_DEFAULT_FORMAT_TAG;
		defaultFormat.channels = 1;
		defaultFormat.sampleRate = TG_AUDIO_V1_2_4_DEFAULT_SAMPLE_RATE;
		defaultFormat.averageBytesPerSecond = TG_AUDIO_V1_2_4_DEFAULT_BYTES_PER_SECOND;
		defaultFormat.blockAlign = TG_AUDIO_V1_2_4_DEFAULT_BLOCK_ALIGN;
		defaultFormat.bitsPerSample = TG_AUDIO_V1_2_4_DEFAULT_BITS_PER_SAMPLE;
		format = &defaultFormat;
	}

	return AudioLibrary_v1_2::openWaveOutDevice(format);
}

bool AudioLibrary_v1_2_4::openWaveDevice(const WaveFormat *format) {
	if (!openWaveOutDevice(format)) {
		audioCloseWave();
		return false;
	}

	if (_userVolumeSet && _engine->_version < kVersionV1_2_7)
		setWaveOutVolume(_userVolume);

	return true;
}

void AudioLibrary_v1_2_4::waveMuteImpl() {
	if (_mute) {
		_waveMuteSavedVolume = ((uint32)_waveVolumeRight << 16) | _waveVolumeLeft;
		setWaveOutVolume(0);
	} else {
		setWaveOutVolume(_waveMuteSavedVolume);
	}
}

uint32 AudioLibrary_v1_2_4::audioGetWaveVolume() {
	if (_engine->_version < kVersionV1_2_4)
		return AudioLibrary_v1_2::audioGetWaveVolume();

	if (_userVolumeSet && _engine->_version >= kVersionV1_2_9)
		return _userVolume;

	if (_waveOutStream)
		return ((uint32)_waveVolumeRight << 16) | _waveVolumeLeft;

	if (!openWaveOutDevice(nullptr))
		return 0;

	uint32 volume = ((uint32)_waveVolumeRight << 16) | _waveVolumeLeft;

	audioCloseWave();
	return volume;
}

void AudioLibrary_v1_2_4::audioSetWaveVolume(uint32 volume) {
	AudioLibrary_v1_2::audioSetWaveVolume(volume);

	if (_engine->_version >= kVersionV1_2_7 && !_noWaveDevice) {
		_iniWaveVolume = (uint16)volume;
		saveWaveVolumeToIni();
	}
}

void AudioLibrary_v1_2_4::pumpAudioPlayback() {
	if (!_waveOutStream)
		return;

	if (_soundCount == 0 && (_engine->_version < kVersionV1_2_9 || _buffersInFlight == 0))
		return;

	if (_masterFadeMode != 0)
		masterFadeTick();

	if (_engine->_version >= kVersionV1_2_7)
		_maxPriority = 0;

	if (retireBufferAndRefill())
		return;

	if (_cdDeviceId != 0 && _engine->_version < kVersionV1_2_7)
		return;

	for (uint16 i = 0; i < _soundCount; i++) {
		SoundObject *sound = _sounds[i];

		if (!sound->enabled)
			continue;

		if (_engine->_version >= kVersionV1_2_7) {
			if (_engine->getEngineMillis() < sound->notifyEndTime && _maxPriority < sound->priority)
				_maxPriority = sound->priority;

			if (sound->notifyPending && _engine->getEngineMillis() > sound->notifyEndTime) {
				engine()->postMessage(TG_AUDIO_V1_2_NOTIFY_MESSAGE, sound->notifyWParam, sound->notifyLParam);
				sound->notifyPending = false;
			}

			if (_cdDeviceId != 0)
				continue;
		}

		if (!sound->isStatic && !sound->endOfSource && sound->available <= sound->lowWater)
			compactAndRefillSoundBuffer(sound);

		if (_engine->_version < kVersionV1_2_7 && sound->notifyPending && _engine->getEngineMillis() > sound->notifyEndTime) {
			engine()->postMessage(TG_AUDIO_V1_2_NOTIFY_MESSAGE, sound->notifyWParam, sound->notifyLParam);
			sound->notifyPending = false;
		}

		if (sound->randomMode && !sound->active && _engine->getEngineMillis() >= sound->nextTrigger) {
			rewindSound(sound);
			activateWaveObject(sound);
		}
	}
}

void AudioLibrary_v1_2_4::updateWaveEnabledFlags(uint16 on) {
	for (uint16 i = 0; i < _soundCount; i++) {
		SoundObject *sound = _sounds[i];

		if (on == 0)
			sound->enabled = false;
		else if (_cdDeviceId == 0)
			sound->enabled = true;
		else
			sound->enabled = sound->isStatic;
	}
}

} // End of namespace Topgun

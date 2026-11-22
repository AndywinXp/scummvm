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
#include "topgun/libraries/resource/resource.h"
#include "topgun/libraries/utils/utils.h"
#include "topgun/topgun.h"

namespace Topgun {

AudioLibrary::WaveObject *AudioLibrary::allocWaveObject() {
	for (uint32 attempt = 0; attempt < 0xFFFF; attempt++) {
		uint16 handle = _nextWaveHandle;
		_nextWaveHandle = (_nextWaveHandle == 0xFFFF) ? 1 : _nextWaveHandle + 1;

		if (!findWaveByHandle(handle)) {
			WaveObject *wave = new WaveObject();
			wave->handle = handle;
			_waveObjects.push_back(wave);
			return wave;
		}
	}

	return nullptr;
}

void AudioLibrary::setupWaveFadeIn(WaveObject *wave, uint16 seconds, uint16 volume) {
	uint32 step = _waveFormat.averageBytesPerSecond * (uint32)seconds;

	if (!_waveOut8Bit)
		step >>= 1;

	wave->fadeReload = step / (uint32)volume;
	wave->fadeCount = wave->fadeReload;

	if (wave->fadeReload != 0) {
		wave->currentVolume = 0;
		wave->fadingIn = true;
	}
}

void AudioLibrary::setupWaveFadeOut(WaveObject *wave, uint16 seconds, uint16 currentVolume) {
	uint32 step = _waveFormat.averageBytesPerSecond * (uint32)seconds;

	if (!_waveOut8Bit)
		step >>= 1;

	wave->fadeReload = step / ((uint32)currentVolume + 1);
	wave->fadeCount = wave->fadeReload;

	if (wave->fadeReload != 0)
		wave->fadingOut = true;
}

uint16 AudioLibrary::audioCreateWave(SoundCreateInfo *info, uint16 flags) {
	bool deviceReady;

	if (_engine->_version >= kVersionV1_1) {
		if (_waveObjects.empty()) {
			WaveFormat format;

			readWaveFormat(info->header, format);
			deviceReady = openWaveOutDevice(&format);
		} else {
			deviceReady = (_waveOutStream != nullptr);
		}
	} else {
		deviceReady = (_waveOutStream || openWaveOutDevice(nullptr));
	}

	if (deviceReady) {
		WaveObject *wave = allocWaveObject();

		if (wave) {
			if (!initWaveObjectFromDescriptor(wave, info, flags)) {
				removeWaveFromArray(wave);
				return 0;
			}

			if (_engine->_version < kVersionV1_1)
				pumpAudioPlayback();
			else if (wave->streaming)
				compactAndRefillSoundBuffer(wave);

			return wave->handle;
		}
	}

	closeCreateInfoSource(info);
	return 0;
}

void AudioLibrary::destroyAllWaveObjects() {
	resetWaveOutQueue();

	uint32 index = 0;

	if (!_waveObjects.empty()) {
		do {
			removeWaveFromArray(_waveObjects[index]);
			index++;
		} while (index < _waveObjects.size());
	}

	_activeSoundCount = 0;
}

void AudioLibrary::destroyWaveObject(WaveObject *wave) {
	if (wave->active)
		_activeSoundCount--;

	removeWaveFromArray(wave);
}

int AudioLibrary::findWaveIndex(const WaveObject *wave) const {
	for (uint32 i = 0; i < _waveObjects.size(); i++) {
		if (_waveObjects[i] == wave)
			return (int)i;
	}

	return -1;
}

AudioLibrary::WaveObject *AudioLibrary::findWaveByHandle(uint16 handle) const {
	for (uint32 i = 0; i < _waveObjects.size(); i++) {
		if (_waveObjects[i]->handle == handle)
			return _waveObjects[i];
	}

	return nullptr;
}

uint16 AudioLibrary::removeWaveFromArray(WaveObject *wave) {
	UtilsLibrary *utilsLibrary = _engine->utilsLibrary();

	int index = findWaveIndex(wave);
	if (index < 0)
		return 0;

	if (hasOpenSource(wave)) {
		if (_cdSuspend)
			closeCDAudioDevice();

		closeSource(wave);
	}

	if (wave->stage)
		utilsLibrary->tgFree(wave->stage);

	if (wave->data)
		utilsLibrary->tgFree(wave->data);

	delete wave;
	_waveObjects.remove_at(index);

	if (_waveObjects.empty() && _engine->_version < kVersionV1_1)
		closeWaveOutDevice();

	return 0;
}

uint16 AudioLibrary::initWaveObjectFromDescriptor(WaveObject *wave, SoundCreateInfo *info, uint16 flags) {
	UtilsLibrary *utilsLibrary = _engine->utilsLibrary();

	wave->loops = 1;
	wave->waveInfo = info->waveInfo;
	wave->fileStream = info->fileStream;

	const byte *header = info->header;
	wave->sourceSize = READ_LE_UINT32(header + 0x10);
	readWaveFormat(header, wave->format);

	wave->volume = TG_AUDIO_MAX_VOLUME_LEVEL;
	wave->currentVolume = TG_AUDIO_MAX_VOLUME_LEVEL;

	if (flags & TG_AUDIO_BACKGROUND_FLAG)
		wave->backgroundGroup = true;

	wave->needsConversion = !(_waveFormat.channels == wave->format.channels &&
	                          _waveFormat.sampleRate == wave->format.sampleRate &&
	                          _waveFormat.bitsPerSample == wave->format.bitsPerSample);
	wave->busy = true;

	if (flags & TG_AUDIO_IN_MEMORY_FLAG) {
		wave->size = wave->sourceSize;
		wave->data = utilsLibrary->tgMalloc(wave->size, 0);

		if (wave->data)
			memcpy(wave->data, header + TG_AUDIO_HEADER_CHUNK_SIZE, wave->size);

		if (wave->needsConversion) {
			if (convertWaveDataInPlace(wave) == 0)
				return 0;
		}

		wave->available = wave->size;
	} else if (flags & TG_AUDIO_LOAD_FLAG) {
		if (_cdSuspend)
			closeCDAudioDevice();

		wave->size = wave->sourceSize;
		wave->data = utilsLibrary->tgMalloc(wave->sourceSize, 1);

		if (!wave->data)
			return 0;

		if ((uint32)readSource(wave, wave->data, wave->sourceSize) != wave->sourceSize) {
			wave->data = utilsLibrary->tgFree(wave->data);
			return 0;
		}

		if (hasOpenSource(wave))
			closeSource(wave);

		wave->waveInfo = 0;

		if (wave->needsConversion) {
			if (convertWaveDataInPlace(wave) == 0)
				return 0;
		}

		wave->available = wave->size;
	} else {
		wave->streaming = true;
		wave->size = _streamBufferSize;
		wave->data = utilsLibrary->tgMalloc(_streamBufferSize, 1);

		if (!wave->data)
			return 0;

		if (_cdSuspend)
			closeCDAudioDevice();

		wave->sourceStart = (uint32)seekSource(wave, 0, SEEK_CUR);

		if (wave->needsConversion) {
			wave->stageSize = _streamBufferSize;
			wave->stage = utilsLibrary->tgMalloc(_streamBufferSize, 0);

			if (!wave->stage)
				return 0;

			convertInitState(&wave->convert, &_waveFormat, &wave->format, wave->sourceSize);
		}
	}

	wave->busy = false;
	return 1;
}

void AudioLibrary::computeRandomizedRepeatDelay(WaveObject *wave, uint32 now) {
	byte seconds;

	if (wave->randomRange == 0)
		seconds = wave->randomMin;
	else
		seconds = (byte)(((_randomSource.getRandomNumber(0x7FFF) & 0xFF) % wave->randomRange) + wave->randomMin);

	wave->nextTrigger = now + (uint32)seconds * 1000;
}

void AudioLibrary::startWavesBySelector(byte *params) {
	if (params[0] & 1) {
		resetWaveOutQueue();
		params[0] &= 0xFE;
	}

	bool selectBackground = (params[1] & 0x80) != 0;
	bool selectForeground = !selectBackground && (params[1] & 0x40);
	bool selectAll = !selectBackground && !selectForeground;

	for (uint32 i = 0; i < _waveObjects.size(); i++) {
		WaveObject *wave = _waveObjects[i];

		if ((selectBackground && wave->backgroundGroup) || (selectForeground && !wave->backgroundGroup) || selectAll)
			startWavePlayback(wave, params);
	}
}

void AudioLibrary::startWavePlayback(WaveObject *wave, const byte *params) {
	if (_engine->_version >= kVersionV1_1 && _muted)
		return;

	if (_masterFadeMode != 0) {
		_masterFadeMode = 0;
		setWaveOutVolume(((uint32)_fadeTargetVolumeRight << 16) | _fadeTargetVolumeLeft);
	}

	wave->fadingOut = false;
	wave->fadingIn = false;
	wave->currentVolume = wave->volume;
	wave->loops = (params[0] & 0x80) ? params[6] : 1;

	if (params[2] != 0 && wave->volume != 0)
		setupWaveFadeIn(wave, params[2], wave->volume);

	if (!wave->backgroundGroup) {
		byte stopBlock[4] = { 0x01, 0xC0, 0x00, 0x00 };

		stopWavesBySelector(stopBlock);
	}

	if (_engine->_version < kVersionV1_1 || wave->active || wave->endOfStream)
		rewindSound(wave);

	wave->randomRepeat = (params[0] & 4) != 0;

	if (wave->randomRepeat) {
		wave->active = false;
		wave->randomMin = params[4];
		wave->randomRange = (params[4] < params[5]) ? (byte)(params[5] - params[4] + 1) : 0;
		computeRandomizedRepeatDelay(wave, _engine->getEngineMillis());
	} else if (!wave->active) {
		wave->active = true;
		_activeSoundCount++;
	}

	pumpAudioPlayback();
}

void AudioLibrary::rewindSound(WaveObject *wave) {
	if (!wave->streaming) {
		wave->available = wave->size;
	} else {
		if (_cdSuspend)
			closeCDAudioDevice();

		seekSource(wave, (int32)wave->sourceStart, SEEK_SET);
		wave->endOfStream = false;
		wave->sourceRead = 0;
		wave->available = 0;
		wave->stageAvailable = 0;
	}

	wave->readPos = 0;
}

void AudioLibrary::stopWavesBySelector(byte *params) {
	if (params[0] & 1) {
		resetWaveOutQueue();
		params[0] &= 0xFE;
	}

	bool selectBackground = (params[1] & 0x80) != 0;
	bool selectForeground = !selectBackground && (params[1] & 0x40);
	bool selectAll = !selectBackground && !selectForeground;

	for (uint32 i = 0; i < _waveObjects.size(); i++) {
		WaveObject *wave = _waveObjects[i];

		if ((selectBackground && wave->backgroundGroup) || (selectForeground && !wave->backgroundGroup) || selectAll)
			stopWaveObject(wave, params);
	}
}

void AudioLibrary::stopWaveObject(WaveObject *wave, const byte *params) {
	if (wave->active) {
		if (params[0] & 1)
			resetWaveOutQueue();

		if (params[2] != 0 && wave->currentVolume != 0)
			setupWaveFadeOut(wave, params[2], wave->currentVolume);
		else
			finishWaveObject(wave);

		pumpAudioPlayback();
	}

	wave->randomRepeat = false;
}

void AudioLibrary::finishWaveObject(WaveObject *wave) {
	wave->active = false;
	_activeSoundCount--;

	if (wave->randomRepeat) {
		computeRandomizedRepeatDelay(wave, _engine->getEngineMillis());
		wave->loops = 1;
	}
}

uint32 AudioLibrary::refillSoundBuffer(WaveObject *wave, byte *destination, uint32 count) {
	uint32 delivered = 0;

	for (;;) {
		if (wave->sourceSize == 0 && wave->loops == 0) {
			warning("AudioLibrary::refillSoundBuffer(): empty source");
			wave->endOfStream = true;
			return delivered;
		}

		uint32 remaining = wave->sourceSize - wave->sourceRead;
		uint32 chunk;

		if (remaining <= count) {
			wave->endOfStream = true;
			chunk = remaining;
		} else {
			chunk = count;
		}

		if ((uint32)readSource(wave, destination, chunk) != chunk) {
			wave->endOfStream = true;
			return delivered;
		}

		delivered += chunk;
		wave->sourceRead += chunk;
		count -= chunk;
		destination += chunk;

		if (wave->endOfStream) {
			if (wave->loops != 0) {
				wave->loops--;

				if (wave->loops == 0)
					return delivered;
			}

			wave->endOfStream = false;
			seekSource(wave, (int32)wave->sourceStart, SEEK_SET);
			wave->sourceRead = 0;
		}

		if (count == 0)
			return delivered;
	}
}

uint32 AudioLibrary::refillSoundBufferConverted(WaveObject *wave, byte *destination, uint32 count) {
	uint32 written = 0;

	if (count == 0)
		return 0;

	for (;;) {
		if (wave->endOfStream)
			return written;

		if (wave->stageAvailable == 0) {
			wave->stageCurrent = wave->stage;

			uint32 staged = refillSoundBuffer(wave, wave->stage, wave->stageSize);
			if (staged == 0)
				return written;

			wave->stageAvailable = staged;
		}

		uint32 consumed = 0;
		uint32 produced = convertSampleRate(&wave->convert, destination, count, wave->stageCurrent, wave->stageAvailable, &consumed);

		destination += produced;
		count -= produced;
		written += produced;
		wave->stageCurrent += consumed;
		wave->stageAvailable -= consumed;

		if (count == 0)
			return written;
	}
}

uint32 AudioLibrary::compactAndRefillSoundBuffer(WaveObject *wave) {
	if (wave->busy || _cdSuspend)
		return 0;

	wave->busy = true;

	if (wave->available != 0)
		_engine->utilsLibrary()->copyHugeBytes(wave->data, wave->data + wave->readPos, wave->available);

	wave->readPos = 0;

	uint32 want = wave->size - wave->available;
	uint32 delivered;

	if (wave->needsConversion)
		delivered = refillSoundBufferConverted(wave, wave->data + wave->available, want);
	else
		delivered = refillSoundBuffer(wave, wave->data + wave->available, want);

	wave->available += delivered;
	wave->busy = false;
	return wave->available;
}

bool AudioLibrary::hasOpenSource(const WaveObject *wave) const {
	return wave->waveInfo != 0 || wave->fileStream;
}

int32 AudioLibrary::readSource(WaveObject *wave, byte *destination, uint32 count) {
	if (wave->fileStream)
		return (int32)wave->fileStream->read(destination, count);

	return _engine->resourceLibrary()->waveRead(wave->waveInfo, destination, (int32)count);
}

int32 AudioLibrary::seekSource(WaveObject *wave, int32 offset, int16 origin) {
	if (wave->fileStream) {
		if (!wave->fileStream->seek(offset, origin))
			return -1;

		return (int32)wave->fileStream->pos();
	}

	return _engine->resourceLibrary()->waveSeek(wave->waveInfo, offset, origin);
}

void AudioLibrary::closeSource(WaveObject *wave) {
	if (wave->fileStream) {
		delete wave->fileStream;
		wave->fileStream = nullptr;
	} else {
		_engine->resourceLibrary()->waveDone(wave->waveInfo, 0);
	}
}

void AudioLibrary::closeCreateInfoSource(SoundCreateInfo *info) {
	if (info->fileStream) {
		delete info->fileStream;
		info->fileStream = nullptr;
	} else if (info->waveInfo != 0) {
		_engine->resourceLibrary()->waveDone(info->waveInfo, 0);
	}
}

} // End of namespace Topgun

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
#include "common/util.h"

#include "topgun/libraries/audio/audio.h"
#include "topgun/topgun.h"

namespace Topgun {

const uint16 AudioLibrary::kFormatBits[TG_AUDIO_NUM_OUTPUT_FORMATS] = { 0x0004, 0x0001, 0x0008, 0x0002, 0x0080, 0x0020, 0x0040, 0x0010, 0x0800, 0x0200, 0x0400, 0x0100 };
const uint16 AudioLibrary::kFormatRates[TG_AUDIO_NUM_OUTPUT_FORMATS] = { 11025, 11025, 11025, 11025, 22050, 22050, 22050, 22050, 44100, 44100, 44100, 44100 };
const byte AudioLibrary::kFormatChannels[TG_AUDIO_NUM_OUTPUT_FORMATS] = { 1, 1, 2, 2, 2, 2, 1, 1, 2, 2, 1, 1 };
const byte AudioLibrary::kFormatBytesPerSample[TG_AUDIO_NUM_OUTPUT_FORMATS] = { 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1 };

bool AudioLibrary::soundEndOrLoop(WaveObject *wave) {
	if (!wave->streaming) {
		wave->available = wave->size;
		wave->readPos = 0;

		if (wave->loops == 0)
			return false;

		wave->loops--;
		if (wave->loops != 0)
			return false;
	} else {
		if (!wave->endOfStream)
			return false;

		if (wave->stageAvailable != 0)
			return false;
	}

	finishWaveObject(wave);
	return true;
}

void AudioLibrary::mixSound8Bit(WaveObject *wave) {
	byte *destination = _waveRing[_ringHead].data;
	uint32 remaining = _waveBufferSize;
	int32 mixCount = (int32)_mixSoundCount;
	bool silence = false;
	bool warnedOverflow = false;

	for (;;) {
		if (wave->streaming && wave->available == 0) {
			if (compactAndRefillSoundBuffer(wave) == 0) {
				silence = true;
				break;
			}
		}

		const byte *source = wave->data + wave->readPos;
		uint32 chunk = remaining;

		if (chunk >= wave->available)
			chunk = wave->available;

		if (chunk == 0)
			return;

		wave->available -= chunk;
		uint32 after = remaining - chunk;
		wave->readPos += chunk;

		if (wave->currentVolume == TG_AUDIO_MAX_VOLUME_LEVEL && !wave->fadingIn && !wave->fadingOut) {
			if (mixCount < 2) {
				memcpy(destination, source, chunk);
			} else if (_engine->_version >= 1) {
				for (uint32 i = 0; i < chunk; i++)
					destination[i] = (byte)CLIP<int32>((int32)destination[i] + source[i] - 0x80, 0, 255);
			} else if (mixCount == 2) {
				for (uint32 i = 0; i < chunk; i++)
					destination[i] = (byte)((destination[i] + source[i]) >> 1);
			} else if (mixCount == 3) {
				for (uint32 i = 0; i < chunk; i++)
					destination[i] = (byte)((2 * destination[i] + source[i]) / 3);
			} else if (mixCount == 4) {
				for (uint32 i = 0; i < chunk; i++)
					destination[i] = (byte)((3 * destination[i] + source[i]) >> 2);
			} else {
				for (uint32 i = 0; i < chunk; i++) {
					uint32 dividendLow = (uint32)destination[i] * (uint32)(mixCount - 1) + source[i];
					int64 dividend = ((int64)source[i] << 32) + (int64)dividendLow;
					int64 quotient = dividend / mixCount;

					if (quotient > 0x7FFFFFFF) {
						if (!warnedOverflow) {
							warning("AudioLibrary::mixSound8Bit(): divide overflow");
							warnedOverflow = true;
						}

						destination[i] = (byte)(dividendLow / (uint32)mixCount);
					} else {
						destination[i] = (byte)quotient;
					}
				}
			}

			destination += chunk;
		} else {
			for (uint32 i = 0; i < chunk; i++) {
				int32 sample = source[i];

				if (wave->currentVolume != TG_AUDIO_MAX_VOLUME_LEVEL)
					sample = (sample * (int8)wave->currentVolume) >> 4;

				int32 existing = *destination;
				int32 result;

				if (mixCount < 2)
					result = sample;
				else if (_engine->_version >= 1)
					result = CLIP<int32>(existing + sample - 0x80, 0, 255);
				else if (mixCount == 2)
					result = (existing + sample) >> 1;
				else if (mixCount == 3)
					result = (2 * existing + sample) / 3;
				else if (mixCount == 4)
					result = (3 * existing + sample) >> 2;
				else
					result = (existing * (mixCount - 1) + sample) / mixCount;

				*destination = (byte)result;
				destination++;

				if (wave->fadingIn) {
					wave->fadeCount--;

					if (wave->fadeCount == 0) {
						wave->fadeCount = wave->fadeReload;
						wave->currentVolume++;

						if (wave->currentVolume >= wave->volume)
							wave->fadingIn = false;
					}
				} else if (wave->fadingOut) {
					wave->fadeCount--;

					if (wave->fadeCount == 0) {
						wave->fadeCount = wave->fadeReload;
						wave->currentVolume--;

						if (wave->currentVolume == 0)
							wave->fadingOut = false;
					}
				}
			}
		}

		remaining = after;

		if (wave->available == 0)
			soundEndOrLoop(wave);

		if (remaining == 0)
			return;

		if (wave->active)
			continue;

		silence = true;
		break;
	}

	if (silence && mixCount == 1)
		memset(destination, 0x80, remaining);
}

void AudioLibrary::mixSound16Bit(WaveObject *wave) {
	byte *destination = _waveRing[_ringHead].data;
	uint32 remaining = _waveBufferSize & ~1U;
	int32 mixCount = (int32)_mixSoundCount;
	bool silence = false;

	for (;;) {
		if (wave->streaming && wave->available == 0) {
			if (compactAndRefillSoundBuffer(wave) == 0) {
				silence = true;
				break;
			}
		}

		const byte *source = wave->data + wave->readPos;
		uint32 chunk = remaining;

		if (chunk >= wave->available)
			chunk = wave->available;

		if (chunk == 0)
			return;

		wave->available -= chunk;
		uint32 after = remaining - chunk;
		wave->readPos += chunk;

		uint32 samples = chunk >> 1;

		if (samples == 0)
			warning("AudioLibrary::mixSound16Bit(): chunk of a single byte");

		if (wave->currentVolume == TG_AUDIO_MAX_VOLUME_LEVEL && !wave->fadingIn && !wave->fadingOut) {
			if (mixCount < 2) {
				memcpy(destination, source, samples * sizeof(int16));
			} else {
				for (uint32 i = 0; i < samples; i++) {
					int32 existing = READ_LE_INT16(destination + i * 2);
					int32 sample = READ_LE_INT16(source + i * 2);
					int32 result;

					if (_engine->_version >= 1)
						result = CLIP<int32>(existing + sample, -32767, 32767);
					else if (mixCount == 2)
						result = (existing + sample) >> 1;
					else if (mixCount == 3)
						result = (2 * existing + sample) / 3;
					else if (mixCount == 4)
						result = (3 * existing + sample) >> 2;
					else
						result = ((existing + sample) * (mixCount - 1)) / mixCount;

					WRITE_LE_UINT16(destination + i * 2, (uint16)result);
				}
			}

			destination += samples * 2;
		} else {
			for (uint32 i = 0; i < samples; i++) {
				int32 sample = READ_LE_INT16(source + i * 2);

				if (wave->currentVolume != TG_AUDIO_MAX_VOLUME_LEVEL)
					sample = (sample * (int8)wave->currentVolume) >> 4;

				int32 existing = READ_LE_INT16(destination);
				int32 result;

				if (mixCount < 2)
					result = sample;
				else if (_engine->_version >= 1)
					result = CLIP<int32>(existing + sample, -32767, 32767);
				else if (mixCount == 2)
					result = (existing + sample) >> 1;
				else if (mixCount == 3)
					result = (4 * existing + sample) / 3;
				else if (mixCount == 4)
					result = (3 * existing + sample) >> 2;
				else
					result = (existing * (mixCount - 1) + sample) / mixCount;

				WRITE_LE_UINT16(destination, (uint16)result);
				destination += 2;

				if (wave->fadingIn) {
					wave->fadeCount--;

					if (wave->fadeCount == 0) {
						wave->fadeCount = wave->fadeReload;
						wave->currentVolume++;

						if (wave->currentVolume >= wave->volume)
							wave->fadingIn = false;
					}
				} else if (wave->fadingOut) {
					wave->fadeCount--;

					if (wave->fadeCount == 0) {
						wave->fadeCount = wave->fadeReload;
						wave->currentVolume--;

						if (wave->currentVolume == 0)
							wave->fadingOut = false;
					}
				}
			}
		}

		remaining = after;

		if (wave->available == 0)
			soundEndOrLoop(wave);

		if (remaining == 0)
			return;

		if (wave->active)
			continue;

		silence = true;
		break;
	}

	if (silence && mixCount == 1)
		memset(destination, 0, remaining);
}

void AudioLibrary::mixAllWavesAndQueueBuffer() {
	_mixSoundCount = 0;

	uint32 count = _waveObjects.size();
	for (uint32 i = 0; i < count; i++) {
		WaveObject *wave = _waveObjects[i];

		if (wave->active && !wave->suspended) {
			_mixSoundCount++;

			if (_waveOut8Bit)
				mixSound8Bit(wave);
			else
				mixSound16Bit(wave);
		}
	}

	if (_mixSoundCount == 0)
		return;

	{
		Common::StackLock lock(g_system->getMixer()->mutex());
		WaveRingEntry &entry = _waveRing[_ringHead];

		entry.busy = true;
		entry.flags &= ~(uint32)(TG_WAVEOUT_HEADER_DONE | TG_WAVEOUT_HEADER_BEGIN_LOOP | TG_WAVEOUT_HEADER_END_LOOP);
		entry.flags |= TG_WAVEOUT_HEADER_IN_QUEUE;
	}

	_buffersInFlight++;
	_ringHead = (_ringHead + 1 == TG_WAVEOUT_RING_SIZE) ? 0 : _ringHead + 1;
}

bool AudioLibrary::retireBufferAndRefill() {
	if (_buffersInFlight != 0) {
		bool done;

		{
			Common::StackLock lock(g_system->getMixer()->mutex());
			done = (_waveRing[_ringTail].flags & TG_WAVEOUT_HEADER_DONE) != 0;
		}

		if (done) {
			_buffersInFlight--;
			_waveRing[_ringTail].busy = false;
			_ringTail = (_ringTail + 1 == TG_WAVEOUT_RING_SIZE) ? 0 : _ringTail + 1;
		}
	}

	if (_activeSoundCount != 0 && !_waveRing[_ringHead].busy) {
		mixAllWavesAndQueueBuffer();
		return true;
	}

	return false;
}

void AudioLibrary::pumpAudioPlayback() {
	if (!_waveRunning)
		return;

	if (_waveObjects.empty())
		return;

	if (_masterFadeMode != 0) {
		uint32 now = _engine->getEngineMillis();

		if (now >= _fadeNextTick) {
			_fadeNextTick = now + TG_AUDIO_FADE_TICK_INTERVAL;

			if (_masterFadeMode == 1)
				masterFadeUpTick();
			else
				masterFadeDownTick();
		}
	}

	bool mixed = retireBufferAndRefill();

	if (mixed && _engine->_version >= 1)
		return;

	if (_cdSuspend)
		return;

	uint32 count = _waveObjects.size();
	for (uint32 i = 0; i < count; i++) {
		WaveObject *wave = _waveObjects[i];

		if (wave->streaming && !wave->endOfStream && (_engine->_version >= 1 ? wave->available <= _refillThreshold : wave->available < _refillThreshold))
			compactAndRefillSoundBuffer(wave);

		if (wave->randomRepeat && !wave->active && _engine->getEngineMillis() >= wave->nextTrigger) {
			rewindSound(wave);
			wave->active = true;
			wave->loops = 1;
			_activeSoundCount++;
		}
	}
}

void AudioLibrary::allocateWaveOutBuffers() {
	for (uint32 i = 0; i < TG_WAVEOUT_RING_SIZE; i++) {
		WaveRingEntry &entry = _waveRing[i];

		entry.data = new byte[_waveBufferSize];
		memset(entry.data, 0, _waveBufferSize * sizeof(byte));
		entry.length = _waveBufferSize;

		if (!_waveOut8Bit)
			entry.length &= ~1U;

		entry.flags = 0;
		entry.busy = false;
	}
}

void AudioLibrary::freeWaveOutBuffers() {
	for (uint32 i = 0; i < TG_WAVEOUT_RING_SIZE; i++) {
		delete[] _waveRing[i].data;
		_waveRing[i].data = nullptr;
	}
}

bool AudioLibrary::findBestWaveFormatIndex(const WaveFormat *wanted) {
	bool soundFormat = (_engine->_version >= 1);
	uint16 formatMask = soundFormat ? TG_AUDIO_WAVE_FORMAT_MASK_V1_1 : TG_AUDIO_WAVE_FORMAT_MASK;
	uint16 mask = (uint16)(formatMask & (_waveOutCapsFormats & 0xFFFF));
	uint32 index = TG_AUDIO_NUM_OUTPUT_FORMATS;

	if (soundFormat && wanted) {
		for (uint32 i = 0; i < TG_AUDIO_NUM_OUTPUT_FORMATS && index == TG_AUDIO_NUM_OUTPUT_FORMATS; i++) {
			if (kFormatChannels[i] == wanted->channels && kFormatBytesPerSample[i] * 8 == wanted->bitsPerSample &&
				kFormatRates[i] == wanted->sampleRate && (kFormatBits[i] & mask))
				index = i;
		}
	}

	if (index == TG_AUDIO_NUM_OUTPUT_FORMATS) {
		index = 0;

		while (index < TG_AUDIO_NUM_OUTPUT_FORMATS && !(kFormatBits[index] & mask))
			index++;
	}

	if (index == TG_AUDIO_NUM_OUTPUT_FORMATS)
		return false;

	_chosenFormatMask = kFormatBits[index];
	_waveFormat.tag = 1;
	_waveFormat.channels = kFormatChannels[index];
	_waveFormat.sampleRate = kFormatRates[index];
	_waveFormat.blockAlign = kFormatChannels[index] * kFormatBytesPerSample[index];
	_waveFormat.averageBytesPerSecond = (uint16)(_waveFormat.blockAlign * kFormatRates[index]);
	_waveFormat.bitsPerSample = kFormatBytesPerSample[index] * 8;

	if (TG_AUDIO_WAVE_BUFFER_SIZE != 0)
		_waveBufferSize = TG_AUDIO_WAVE_BUFFER_SIZE;
	else if (_engine->_version >= 1)
		_waveBufferSize = (uint32)_waveFormat.channels << 13;
	else
		_waveBufferSize = TG_AUDIO_DEFAULT_WAVE_BUFFER_SIZE;

	_streamBufferSize = (TG_AUDIO_DATA_BUFFER_SIZE != 0) ? TG_AUDIO_DATA_BUFFER_SIZE : (_waveBufferSize << 2);
	_refillThreshold = (_engine->_version >= 1) ? (_streamBufferSize >> 1) : ((_streamBufferSize * 3) >> 2);
	return true;
}

void AudioLibrary::closeWaveOutDevice() {
	if (!_waveOutStream)
		return;

	resetWaveOutQueue();

	if (_engine->_version >= 1) {
		for (uint32 i = 0; i < _waveObjects.size(); i++) {
			WaveObject *wave = _waveObjects[i];

			if (wave->active) {
				wave->active = false;
				_activeSoundCount--;
			}

			wave->randomRepeat = false;
		}
	}

	if (_savedWaveVolumeLeft != _waveVolumeLeft || _savedWaveVolumeRight != _waveVolumeRight)
		setWaveOutVolume(((uint32)_savedWaveVolumeRight << 16) | _savedWaveVolumeLeft);

	g_system->getMixer()->stopHandle(_waveOutHandle);
	delete _waveOutStream;
	_waveOutStream = nullptr;
	freeWaveOutBuffers();

	_waveRunning = false;
	_chosenFormatMask = 0;
}

bool AudioLibrary::openWaveOutDevice(const WaveFormat *wanted) {
	if (_engine->_version >= 1 && _waveOutStream) {
		if (!wanted)
			return true;

		if (_waveFormat.channels == wanted->channels && _waveFormat.bitsPerSample == wanted->bitsPerSample &&
			_waveFormat.sampleRate == wanted->sampleRate)
			return true;

		closeWaveOutDevice();
	}

	if (!findBestWaveFormatIndex(wanted))
		return false;

	Audio::Mixer *mixer = g_system->getMixer();
	if (!mixer->isReady()) {
		warning("AudioLibrary::openWaveOutDevice(): the mixer is not ready");
		return false;
	}

	allocateWaveOutBuffers();

	_waveRunning = true;
	_waveOut8Bit = (_waveFormat.bitsPerSample == 8);
	_ringTail = 0;
	_ringHead = 0;

	_waveOutStream = new WaveOutStream(_waveRing, _waveFormat.sampleRate, _waveFormat.channels == 2, _waveOut8Bit);
	mixer->playStream(Audio::Mixer::kSFXSoundType, &_waveOutHandle, _waveOutStream, -1,
	                  Audio::Mixer::kMaxChannelVolume, 0, DisposeAfterUse::NO);

	_savedWaveVolumeLeft = (uint16)(_hardwareWaveVolume & 0xFFFF);
	_savedWaveVolumeRight = (uint16)(_hardwareWaveVolume >> 16);
	_waveVolumeLeft = _savedWaveVolumeLeft;
	_waveVolumeRight = _savedWaveVolumeRight;

	if (_engine->_version < 1 || !_disableWaveVolume)
		applyWaveOutVolume(_hardwareWaveVolume);

	return true;
}

uint16 AudioLibrary::pauseAllPlayback(uint16 resume) {
	if (!_waveOutStream)
		return 0;

	if (resume != 0 && !_waveRunning) {
		_waveRunning = true;
		g_system->getMixer()->pauseHandle(_waveOutHandle, false);
	} else if (resume == 0 && _waveRunning) {
		_waveRunning = false;
		g_system->getMixer()->pauseHandle(_waveOutHandle, true);
	}

	return 1;
}

void AudioLibrary::setWaveOutVolume(uint32 volume) {
	if (!_waveOutStream)
		return;

	if (_engine->_version >= 1 && _disableWaveVolume)
		return;

	_waveVolumeLeft = (uint16)(volume & 0xFFFF);
	_waveVolumeRight = (uint16)(volume >> 16);
	_hardwareWaveVolume = volume;
	applyWaveOutVolume(volume);
}

void AudioLibrary::applyWaveOutVolume(uint32 volume) {
	uint32 left = volume & 0xFFFF;
	uint32 right = volume >> 16;
	uint32 loudest = MAX(left, right);
	byte channelVolume = (byte)((loudest * 255 + 32767) / 65535);
	int8 balance = 0;

	if (left != right && loudest != 0) {
		if (right > left)
			balance = (int8)((127 * (right - left) + right / 2) / right);
		else
			balance = (int8)(-(int32)((127 * (left - right) + left / 2) / left));
	}

	Audio::Mixer *mixer = g_system->getMixer();

	mixer->setChannelVolume(_waveOutHandle, channelVolume);
	mixer->setChannelBalance(_waveOutHandle, balance);
}

void AudioLibrary::resetWaveOutQueue() {
	if (!_waveOutStream || _buffersInFlight == 0)
		return;

	{
		Common::StackLock lock(g_system->getMixer()->mutex());

		for (uint32 i = 0; i < TG_WAVEOUT_RING_SIZE; i++) {
			if (_waveRing[i].flags & TG_WAVEOUT_HEADER_IN_QUEUE)
				_waveRing[i].flags = (_waveRing[i].flags & ~(uint32)TG_WAVEOUT_HEADER_IN_QUEUE) | TG_WAVEOUT_HEADER_DONE;
		}

		_waveOutStream->resetPlayback();
	}

	for (uint32 i = 0; i < TG_WAVEOUT_RING_SIZE; i++)
		_waveRing[i].busy = false;

	_ringHead = 0;
	_ringTail = 0;
	_buffersInFlight = 0;
}

bool AudioLibrary::probeWaveOutCapabilities() {
	_waveOutCapsFormats = 0x0FFF;
	return true;
}

void AudioLibrary::destroyAllWaveObjectsAndCloseDevice() {
	_waveRunning = false;
	destroyAllWaveObjects();
	closeWaveOutDevice();
}

} // End of namespace Topgun

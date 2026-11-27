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

#include "topgun/libraries/audio/audio.h"
#include "topgun/libraries/audio/audio_v1_2_4.h"
#include "topgun/libraries/audio/audio_v1_2_tables.h"
#include "topgun/libraries/utils/utils.h"
#include "topgun/topgun_v1_2_4.h"

namespace Topgun {

uint32 AudioLibrary::audioResampleWave(const WaveFormat *destinationFormat, const WaveFormat *sourceFormat, uint32 sourceBytes, byte *destinationBuffer, const byte *sourceBuffer) {
	ConvertState state;

	uint32 outputBytes = convertInitState(&state, destinationFormat, sourceFormat, sourceBytes);

	if (destinationBuffer && sourceBuffer)
		outputBytes = convertSampleRate(&state, destinationBuffer, outputBytes, sourceBuffer, sourceBytes, nullptr);

	return outputBytes;
}

uint32 AudioLibrary::convertPcmMode0Mono8ToStereo8(byte *destination, const byte *source, uint32 destinationCapacity, uint32 sourceBytes, uint32 *consumed) {
	uint32 destinationUnits = destinationCapacity >> 1;
	uint32 sourceUnits = sourceBytes;

	while (destinationUnits != 0 && sourceUnits != 0) {
		byte sample = source[0];

		destination[0] = sample;
		destination[1] = sample;
		source += 1;
		destination += 2;
		destinationUnits--;
		sourceUnits--;
	}

	if (consumed)
		*consumed = sourceBytes - sourceUnits;

	return destinationCapacity - (destinationUnits << 1);
}

uint32 AudioLibrary::convertPcmMode1Mono8ToStereo16(byte *destination, const byte *source, uint32 destinationCapacity, uint32 sourceBytes, uint32 *consumed) {
	uint32 destinationUnits = destinationCapacity >> 2;
	uint32 sourceUnits = sourceBytes;

	while (destinationUnits != 0 && sourceUnits != 0) {
		uint16 sample = (uint16)((int16)(((int32)source[0] - 0x80) << 8));

		WRITE_LE_UINT16(destination, sample);
		WRITE_LE_UINT16(destination + 2, sample);
		source += 1;
		destination += 4;
		destinationUnits--;
		sourceUnits--;
	}

	if (consumed)
		*consumed = sourceBytes - sourceUnits;

	return destinationCapacity - (destinationUnits << 2);
}

uint32 AudioLibrary::convertPcmMode2Mono16ToStereo8(byte *destination, const byte *source, uint32 destinationCapacity, uint32 sourceBytes, uint32 *consumed) {
	uint32 destinationUnits = destinationCapacity >> 1;
	uint32 sourceUnits = sourceBytes >> 1;

	while (destinationUnits != 0 && sourceUnits != 0) {
		int32 sample = READ_LE_INT16(source);
		byte value = (byte)((sample >> 8) + 0x80);

		destination[0] = value;
		destination[1] = value;
		source += 2;
		destination += 2;
		destinationUnits--;
		sourceUnits--;
	}

	if (consumed)
		*consumed = sourceBytes - (sourceUnits << 1);

	return destinationCapacity - (destinationUnits << 1);
}

uint32 AudioLibrary::convertPcmMode3Mono16ToStereo16(byte *destination, const byte *source, uint32 destinationCapacity, uint32 sourceBytes, uint32 *consumed) {
	uint32 destinationUnits = destinationCapacity >> 2;
	uint32 sourceUnits = sourceBytes >> 1;

	while (destinationUnits != 0 && sourceUnits != 0) {
		uint16 sample = READ_LE_UINT16(source);

		WRITE_LE_UINT16(destination, sample);
		WRITE_LE_UINT16(destination + 2, sample);
		source += 2;
		destination += 4;
		destinationUnits--;
		sourceUnits--;
	}

	if (consumed)
		*consumed = sourceBytes - (sourceUnits << 1);

	return destinationCapacity - (destinationUnits << 2);
}

uint32 AudioLibrary::convertPcmMode4Stereo8ToMono8(byte *destination, const byte *source, uint32 destinationCapacity, uint32 sourceBytes, uint32 *consumed) {
	uint32 destinationUnits = destinationCapacity;
	uint32 sourceUnits = sourceBytes >> 1;

	while (destinationUnits != 0 && sourceUnits != 0) {
		destination[0] = source[0];
		source += 2;
		destination += 1;
		destinationUnits--;
		sourceUnits--;
	}

	if (consumed)
		*consumed = sourceBytes - (sourceUnits << 1);

	return destinationCapacity - destinationUnits;
}

uint32 AudioLibrary::convertPcmMode5Stereo8ToMono16(byte *destination, const byte *source, uint32 destinationCapacity, uint32 sourceBytes, uint32 *consumed) {
	uint32 destinationUnits = destinationCapacity >> 1;
	uint32 sourceUnits = sourceBytes >> 1;

	while (destinationUnits != 0 && sourceUnits != 0) {
		WRITE_LE_UINT16(destination, (uint16)((int16)(((int32)source[0] - 0x80) << 8)));
		source += 2;
		destination += 2;
		destinationUnits--;
		sourceUnits--;
	}

	if (consumed)
		*consumed = sourceBytes - (sourceUnits << 1);

	return destinationCapacity - (destinationUnits << 1);
}

uint32 AudioLibrary::convertPcmMode6Stereo16ToMono8(byte *destination, const byte *source, uint32 destinationCapacity, uint32 sourceBytes, uint32 *consumed) {
	uint32 destinationUnits = destinationCapacity;
	uint32 sourceUnits = sourceBytes >> 2;

	while (destinationUnits != 0 && sourceUnits != 0) {
		int32 sample = READ_LE_INT16(source);

		destination[0] = (byte)((sample >> 8) + 0x80);
		source += 4;
		destination += 1;
		destinationUnits--;
		sourceUnits--;
	}

	if (consumed)
		*consumed = sourceBytes - (sourceUnits << 2);

	return destinationCapacity - destinationUnits;
}

uint32 AudioLibrary::convertPcmMode7Stereo16ToMono16(byte *destination, const byte *source, uint32 destinationCapacity, uint32 sourceBytes, uint32 *consumed) {
	uint32 destinationUnits = destinationCapacity >> 1;
	uint32 sourceUnits = sourceBytes >> 2;

	while (destinationUnits != 0 && sourceUnits != 0) {
		WRITE_LE_UINT16(destination, READ_LE_UINT16(source));
		source += 4;
		destination += 2;
		destinationUnits--;
		sourceUnits--;
	}

	if (consumed)
		*consumed = sourceBytes - (sourceUnits << 2);

	return destinationCapacity - (destinationUnits << 1);
}

uint32 AudioLibrary::convertPcmMode8Mono8ToMono16(byte *destination, const byte *source, uint32 destinationCapacity, uint32 sourceBytes, uint32 *consumed) {
	uint32 destinationUnits = destinationCapacity >> 1;
	uint32 sourceUnits = sourceBytes;

	while (destinationUnits != 0 && sourceUnits != 0) {
		WRITE_LE_UINT16(destination, (uint16)((int16)(((int32)source[0] - 0x80) << 8)));
		source += 1;
		destination += 2;
		destinationUnits--;
		sourceUnits--;
	}

	if (consumed)
		*consumed = sourceBytes - sourceUnits;

	return destinationCapacity - (destinationUnits << 1);
}

uint32 AudioLibrary::convertPcmMode9Mono16ToMono8(byte *destination, const byte *source, uint32 destinationCapacity, uint32 sourceBytes, uint32 *consumed) {
	uint32 destinationUnits = destinationCapacity;
	uint32 sourceUnits = sourceBytes >> 1;

	while (destinationUnits != 0 && sourceUnits != 0) {
		int32 sample = READ_LE_INT16(source);

		destination[0] = (byte)((sample >> 8) + 0x80);
		source += 2;
		destination += 1;
		destinationUnits--;
		sourceUnits--;
	}

	if (consumed)
		*consumed = sourceBytes - (sourceUnits << 1);

	return destinationCapacity - destinationUnits;
}

void AudioLibrary::selectConvertMode(ConvertState *state) {
	state->mode = 0x0B;

	if (state->decimateFactor == state->interpolateFactor) {
		if (state->sourceMono == state->destinationMono) {
			if (state->source16 == state->destination16)
				state->mode = 0x0A;
			else
				state->mode = state->source16 ? 9 : 8;
		} else if (state->sourceMono) {
			if (state->source16 == state->destination16)
				state->mode = state->source16 ? 3 : 0;
			else
				state->mode = state->source16 ? 2 : 1;
		} else {
			if (state->source16 == state->destination16)
				state->mode = state->source16 ? 7 : 4;
			else
				state->mode = state->source16 ? 6 : 5;
		}
	}
}

void AudioLibrary::resampleDecimateRead(const ConvertState *state, const byte *&source) {
	byte count = state->decimateFactor;
	uint32 frameBytes = (state->source16 ? 2 : 1) * (state->sourceMono ? 1 : 2);

	_resampleAccumL = 0;
	_resampleAccumR = 0;

	for (;;) {
		if (_resampleInputBytesRemaining < frameBytes) {
			warning("AudioLibrary::resampleDecimateRead(): input ends inside a sample");
			_resampleInputBytesRemaining = 0;
			break;
		}

		int32 left;
		int32 right;

		if (state->source16) {
			left = READ_LE_INT16(source);
			source += 2;
			_resampleInputBytesRemaining -= 2;
		} else {
			left = ((int32)source[0] << 8) - 0x8000;
			source += 1;
			_resampleInputBytesRemaining -= 1;
		}

		_resampleAccumL += left;

		if (!state->sourceMono) {
			if (state->source16) {
				right = READ_LE_INT16(source);
				source += 2;
				_resampleInputBytesRemaining -= 2;
			} else {
				right = ((int32)source[0] << 8) - 0x8000;
				source += 1;
				_resampleInputBytesRemaining -= 1;
			}
		} else {
			right = left;
		}

		_resampleAccumR += right;

		if (_resampleInputBytesRemaining == 0)
			break;

		count--;
		if (count == 0)
			break;
	}

	_resampleAccumL >>= state->decimateShift;
	_resampleAccumR >>= state->decimateShift;
}

void AudioLibrary::resampleDecimateEmit(const ConvertState *state, byte *&destination) {
	uint32 sampleBytes = state->destination16 ? 2 : 1;
	uint32 frameBytes = sampleBytes * (state->destinationMono ? 1 : 2);

	if (_resampleOutputBytesRemaining < frameBytes) {
		warning("AudioLibrary::resampleDecimateEmit(): output ends inside a sample");
		_resampleOutputBytesRemaining = 0;
		return;
	}

	if (state->destination16) {
		WRITE_LE_UINT16(destination, (uint16)_resampleAccumL);
		destination += 2;
		_resampleOutputBytesRemaining -= 2;
	} else {
		*destination = (byte)(((_resampleAccumL + 0x8000) >> 8) & 0xFF);
		destination += 1;
		_resampleOutputBytesRemaining -= 1;
	}

	if (!state->destinationMono) {
		if (state->destination16) {
			WRITE_LE_UINT16(destination, (uint16)_resampleAccumR);
			destination += 2;
			_resampleOutputBytesRemaining -= 2;
		} else {
			*destination = (byte)(((_resampleAccumR + 0x8000) >> 8) & 0xFF);
			destination += 1;
			_resampleOutputBytesRemaining -= 1;
		}
	}
}

void AudioLibrary::resampleInterpolateRead(const ConvertState *state, const byte *&source) {
	uint32 frameBytes = (state->source16 ? 2 : 1) * (state->sourceMono ? 1 : 2);

	if (_resampleInputBytesRemaining < frameBytes) {
		warning("AudioLibrary::resampleInterpolateRead(): input ends inside a sample");
		_resampleInputBytesRemaining = 0;
		return;
	}

	int32 left;
	int32 right;

	if (state->source16) {
		left = READ_LE_INT16(source);
		source += 2;
		_resampleInputBytesRemaining -= 2;
	} else {
		left = ((int32)source[0] - 0x80) << 8;
		source += 1;
		_resampleInputBytesRemaining -= 1;
	}

	_resampleAccumL = left;

	if (!state->sourceMono) {
		if (state->source16) {
			right = READ_LE_INT16(source);
			source += 2;
			_resampleInputBytesRemaining -= 2;
		} else {
			right = ((int32)source[0] - 0x80) << 8;
			source += 1;
			_resampleInputBytesRemaining -= 1;
		}
	} else {
		right = left;
	}

	_resampleAccumR = right;

	if (_resampleInputBytesRemaining >= frameBytes) {
		if (state->source16) {
			_resampleNextSampleL = READ_LE_INT16(source);

			if (!state->sourceMono)
				_resampleNextSampleR = READ_LE_INT16(source + 2);
			else
				_resampleNextSampleR = _resampleNextSampleL;
		} else {
			_resampleNextSampleL = ((int32)source[0] - 0x80) << 8;

			if (!state->sourceMono)
				_resampleNextSampleR = ((int32)source[1] - 0x80) << 8;
			else
				_resampleNextSampleR = _resampleNextSampleL;
		}
	} else {
		if (_resampleInputBytesRemaining != 0)
			warning("AudioLibrary::resampleInterpolateRead(): input ends inside a sample");

		_resampleNextSampleL = _resampleAccumL;
		_resampleNextSampleR = _resampleAccumR;
	}
}

void AudioLibrary::resampleInterpolateEmit(const ConvertState *state, byte *&destination) {
	uint32 sampleBytes = state->destination16 ? 2 : 1;
	uint32 frameBytes = sampleBytes * (state->destinationMono ? 1 : 2);
	byte count = state->interpolateFactor;

	_resampleStepL = (_resampleNextSampleL - _resampleAccumL) >> state->interpolateShift;
	_resampleStepR = (_resampleNextSampleR - _resampleAccumR) >> state->interpolateShift;

	for (;;) {
		if (_resampleOutputBytesRemaining < frameBytes) {
			warning("AudioLibrary::resampleInterpolateEmit(): output ends inside a sample");
			_resampleOutputBytesRemaining = 0;
			break;
		}

		if (state->destination16) {
			WRITE_LE_UINT16(destination, (uint16)_resampleAccumL);
			destination += 2;
			_resampleOutputBytesRemaining -= 2;
		} else {
			*destination = (byte)(((_resampleAccumL + 0x8000) >> 8) & 0xFF);
			destination += 1;
			_resampleOutputBytesRemaining -= 1;
		}

		if (!state->destinationMono) {
			if (state->destination16) {
				WRITE_LE_UINT16(destination, (uint16)_resampleAccumR);
				destination += 2;
				_resampleOutputBytesRemaining -= 2;
			} else {
				*destination = (byte)(((_resampleAccumR + 0x8000) >> 8) & 0xFF);
				destination += 1;
				_resampleOutputBytesRemaining -= 1;
			}
		}

		_resampleAccumL += _resampleStepL;
		_resampleAccumR += _resampleStepR;

		if (_resampleOutputBytesRemaining == 0)
			break;

		count--;
		if (count == 0)
			break;
	}
}

uint32 AudioLibrary::convertInitState(ConvertState *state, const WaveFormat *destinationFormat, const WaveFormat *sourceFormat, uint32 sourceBytes) {
	state->source16 = 0;
	state->destination16 = 0;
	state->sourceMono = 0;
	state->destinationMono = 0;
	state->decimateShift = 0;
	state->decimateFactor = 1;
	state->interpolateFactor = 1;

	if (sourceFormat->channels == 1)
		state->sourceMono = 1;

	if (destinationFormat->channels == 1)
		state->destinationMono = 1;

	if (sourceFormat->bitsPerSample == 16)
		state->source16 = 1;

	if (destinationFormat->bitsPerSample == 16)
		state->destination16 = 1;

	uint32 frames = 0;
	if (sourceFormat->blockAlign != 0)
		frames = sourceBytes / sourceFormat->blockAlign;
	else
		warning("AudioLibrary::convertInitState(): source block alignment is zero");

	if (destinationFormat->sampleRate != sourceFormat->sampleRate) {
		if (destinationFormat->sampleRate == 0 || sourceFormat->sampleRate == 0) {
			warning("AudioLibrary::convertInitState(): sample rate is zero");
		} else if (destinationFormat->sampleRate > sourceFormat->sampleRate) {
			uint32 factor = destinationFormat->sampleRate / sourceFormat->sampleRate;

			state->interpolateFactor = (byte)factor;
			state->interpolateShift = (factor == 2) ? 1 : 2;
			frames = frames * factor;
		} else {
			uint32 factor = sourceFormat->sampleRate / destinationFormat->sampleRate;

			state->decimateFactor = (byte)factor;
			state->decimateShift = (factor == 2) ? 1 : 2;
			frames = frames / (uint32)(int16)factor;
		}
	}

	selectConvertMode(state);

	return frames * destinationFormat->blockAlign;
}

uint32 AudioLibrary::convertSampleRate(ConvertState *state, byte *destination, uint32 destinationCapacity, const byte *source, uint32 sourceBytes, uint32 *consumed) {
	switch (state->mode) {
	case 0:
		return convertPcmMode0Mono8ToStereo8(destination, source, destinationCapacity, sourceBytes, consumed);

	case 1:
		return convertPcmMode1Mono8ToStereo16(destination, source, destinationCapacity, sourceBytes, consumed);

	case 2:
		return convertPcmMode2Mono16ToStereo8(destination, source, destinationCapacity, sourceBytes, consumed);

	case 3:
		return convertPcmMode3Mono16ToStereo16(destination, source, destinationCapacity, sourceBytes, consumed);

	case 4:
		return convertPcmMode4Stereo8ToMono8(destination, source, destinationCapacity, sourceBytes, consumed);

	case 5:
		return convertPcmMode5Stereo8ToMono16(destination, source, destinationCapacity, sourceBytes, consumed);

	case 6:
		return convertPcmMode6Stereo16ToMono8(destination, source, destinationCapacity, sourceBytes, consumed);

	case 7:
		return convertPcmMode7Stereo16ToMono16(destination, source, destinationCapacity, sourceBytes, consumed);

	case 8:
		return convertPcmMode8Mono8ToMono16(destination, source, destinationCapacity, sourceBytes, consumed);

	case 9:
		return convertPcmMode9Mono16ToMono8(destination, source, destinationCapacity, sourceBytes, consumed);

	case 0x0A: {
		uint32 copied = (destinationCapacity >= sourceBytes) ? sourceBytes : destinationCapacity;

		_engine->utilsLibrary()->copyHugeBytes(destination, source, copied);

		if (consumed)
			*consumed = copied;

		return copied;
	}

	default:
		break;
	}

	_resampleOutputBytesRemaining = destinationCapacity;
	_resampleInputBytesRemaining = sourceBytes;

	if (state->decimateFactor <= state->interpolateFactor) {
		while (_resampleOutputBytesRemaining != 0 && _resampleInputBytesRemaining != 0) {
			resampleInterpolateRead(state, source);
			resampleInterpolateEmit(state, destination);
		}
	} else {
		while (_resampleOutputBytesRemaining != 0 && _resampleInputBytesRemaining != 0) {
			resampleDecimateRead(state, source);
			resampleDecimateEmit(state, destination);
		}
	}

	if (consumed)
		*consumed = sourceBytes - _resampleInputBytesRemaining;

	return destinationCapacity - _resampleOutputBytesRemaining;
}

uint16 AudioLibrary::convertWaveDataInPlace(WaveObject *wave) {
	UtilsLibrary *utilsLibrary = _engine->utilsLibrary();

	wave->stage = wave->data;
	wave->stageCurrent = wave->data;
	wave->stageAvailable = wave->size;
	wave->stageSize = wave->size;

	wave->size = convertInitState(&wave->convert, &_waveFormat, &wave->format, wave->sourceSize);
	wave->data = utilsLibrary->tgMalloc(wave->size, 1);

	if (!wave->data)
		return 0;

	convertSampleRate(&wave->convert, wave->data, wave->size, wave->stageCurrent, wave->stageAvailable, nullptr);

	wave->stage = utilsLibrary->tgFree(wave->stage);
	wave->needsConversion = false;
	return 1;
}

bool AudioLibrary_v1_2::ensureAdpcmScratchBuffer(const SoundObject *sound, uint32 sourceBytes) {
	// The decoder always writes 16-bit samples, four bytes for every source byte
	uint32 required = MAX<uint32>(sourceBytes << sound->adpcmShift, sourceBytes << 2);

	if (required > _adpcmScratchSize) {
		byte *block = engine()->utilsLibrary()->tgRealloc(_adpcmScratch, required, 0);

		if (!block)
			return false;

		_adpcmScratch = block;
		_adpcmScratchSize = required;
	}

	return true;
}

bool AudioLibrary_v1_2::ensureResampleScratchBuffer(uint32 size) {
	if (size > _resampleScratchSize) {
		byte *block = engine()->utilsLibrary()->tgRealloc(_resampleScratch, size, 0);

		if (!block)
			return false;

		_resampleScratch = block;
		_resampleScratchSize = size;
	}

	return true;
}

uint32 AudioLibrary_v1_2::computeSourceBytesForOutput(const SoundObject *sound, uint32 outputBytes) {
	uint32 bytes = outputBytes;

	if (sound->adpcmEncoded)
		bytes >>= sound->adpcmShift;

	if (sound->needsConversion) {
		bytes = (bytes * sound->convert.decimateFactor) / sound->convert.interpolateFactor;
		bytes <<= sound->convert.source16;
		bytes >>= sound->convert.destination16;
		bytes <<= (sound->convert.sourceMono ? 0 : 1);
		bytes >>= (sound->convert.destinationMono ? 0 : 1);
	}

	if (bytes > sound->available)
		bytes = sound->available;

	if (sound->sourceLength - sound->consumed < bytes)
		bytes = sound->sourceLength - sound->consumed;

	return bytes;
}

uint32 AudioLibrary_v1_2::decodeImaAdpcm(bool stereo, uint32 count, const byte *source, byte *destination, AdpcmState *state) {
	for (uint32 i = 0; i < count; i++) {
		byte value = source[i];

		for (uint32 half = 0; half < 2; half++) {
			byte nibble = value & 0x0F;
			bool rightChannel = stereo && half == 1;
			int32 &predictor = rightChannel ? state->predictorRight : state->predictorLeft;
			uint16 &index = rightChannel ? state->indexRight : state->indexLeft;

			int32 difference = kImaDiffTable[(uint16)((nibble & 7) + index)];

			if (nibble & 8)
				difference = -difference;

			predictor += difference;

			if (predictor > 0x7FFF)
				predictor = 0x7FFF;
			else if (predictor < -0x8000)
				predictor = -0x8000;

			int32 newIndex = (int16)(kImaIndexAdjustTable[nibble] + index);

			if (newIndex < 0)
				newIndex = 0;
			else if (newIndex > TG_AUDIO_V1_2_ADPCM_MAX_INDEX)
				newIndex = TG_AUDIO_V1_2_ADPCM_MAX_INDEX;

			index = (uint16)newIndex;

			WRITE_LE_UINT16(destination, (uint16)predictor);
			destination += 2;
			value >>= 4;
		}
	}

	return count;
}

} // End of namespace Topgun

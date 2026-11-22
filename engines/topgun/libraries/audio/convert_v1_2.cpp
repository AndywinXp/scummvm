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

#include "topgun/libraries/audio/audio_v1_2.h"
#include "topgun/libraries/audio/audio_v1_2_tables.h"
#include "topgun/libraries/utils/utils.h"
#include "topgun/topgun_v1_2.h"

namespace Topgun {

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

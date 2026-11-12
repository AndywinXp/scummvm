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

#include "engines/grim/emi/sound/cmp3file.h"
#include "engines/grim/emi/sound/cwavesound.h"

namespace Grim {

static const double kMp3SampleRate = 44100.0;
static const uint kMp3HeaderSize = 32;
static const uint kMp3PcmRatio = 11;
static const uint kMp3WaveHeaderSize = 24;
static const uint kMp3FormatChunkSize = 4;
static const uint kMp3FormatTag = 85;

bool CMp3File::read(LFile *file, CWaveSound *sound) {
	file->seek(0, SEEK_END);

	uint size = file->tell();

	file->seek(0, SEEK_SET);

	sound->_sampleRate = kMp3SampleRate;
	sound->_formatTag = CWaveSound::kFormatMp3;
	sound->_bitsPerSample = 16;
	sound->_numChannels = 2;
	sound->_littleEndian = true;
	sound->_dataSize = kMp3PcmRatio * size;

	sound->reallocData(size + kMp3HeaderSize);

	byte *samples = sound->_samples;

	if (!samples)
		return false;

	memcpy(samples, "RIFF", 4);
	WRITE_LE_UINT32(samples + 4, size + kMp3WaveHeaderSize);
	memcpy(samples + 8, "WAVEfmt ", 8);
	WRITE_LE_UINT32(samples + 16, kMp3FormatChunkSize);
	WRITE_LE_UINT32(samples + 20, kMp3FormatTag);
	memcpy(samples + 24, "data", 4);
	WRITE_LE_UINT32(samples + 28, size);

	file->read(samples + kMp3HeaderSize, size);

	return true;
}

} // End of namespace Grim

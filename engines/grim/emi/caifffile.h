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

#ifndef GRIM_EMI_CAIFFFILE_H
#define GRIM_EMI_CAIFFFILE_H

#include "common/scummsys.h"

#include "engines/grim/emi/lfile.h"

namespace Grim {

class CWaveSound;

struct AiffHeader {
	uint formSize;
	bool isAifc;
};

class CAiffFile {
public:
	enum {
		kMaxMarkers = 16,
		kVimaTableSize = 89 * 64
	};

	virtual bool read(LFile *file, CWaveSound *sound);
	virtual bool write(LFile *file, CWaveSound *sound);

	bool parse(LFile *file, CWaveSound *sound, int *dataPosition, uint *dataSize);
	bool readMarkers(LFile *file, CWaveSound *sound, int chunkSize);
	bool readInstrument(LFile *file, CWaveSound *sound);

	static bool readFormHeader(LFile *file, AiffHeader *header);
	static bool readChunkHeader(LFile *file, uint32 *header);
	static bool readCommonChunk(LFile *file, const int *chunkSize, const AiffHeader *header, CWaveSound *sound);
	static bool readSoundData(LFile *file, CWaveSound *sound, int chunkSize, int *dataPosition, uint *dataSize);
	static bool skipChunk(LFile *file, CWaveSound *sound, int chunkSize);
	static bool readLoop(LFile *file, uint16 *loop);
	static double extendedToDouble(const byte *extended);
	static void decodeVimaBlockReset(byte *dest, const byte *source, uint size);
	static void decodeVimaBlock(byte *dest, const byte *source, uint size);

	uint16 loopBeginId;
	uint16 loopEndId;
	int numMarkers;
	uint16 markerId[kMaxMarkers];
	uint markerPos[kMaxMarkers];

private:
	static uint16 readBigEndian16(LFile *file);
	static uint32 readBigEndian32(LFile *file);
};

} // End of namespace Grim

#endif

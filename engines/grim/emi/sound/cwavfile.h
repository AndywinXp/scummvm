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

#ifndef GRIM_EMI_CWAVFILE_H
#define GRIM_EMI_CWAVFILE_H

#include "common/noncopyable.h"
#include "common/scummsys.h"

#include "engines/grim/emi/files/lfile.h"

namespace Grim {

class CWaveSound;
class EmiEngine;

class CWavFile : public Common::NonCopyable {
public:
	enum {
		kVimaTableSize = 89 * 64,
		kMcmpReadBufferSize = 100000,
		kMcmpDecodeBufferSize = 0x2000,
		kPathBufferSize = 1000000,
		kMaxPathLength = 259
	};

	explicit CWavFile(EmiEngine *engine);
	virtual ~CWavFile();

	virtual bool read(LFile *file, CWaveSound *sound);
	virtual bool readMcmpFromPath(LFile *file, CWaveSound *sound);
	virtual bool readMcmp(LFile *file, CWaveSound *sound);
	virtual bool write(LFile *file, CWaveSound *sound);

	static bool parseWav(const byte *buffer, uint bufferSize, CWaveSound *sound);
	static bool readChunkHeader(LFile *file, uint32 *header);
	static bool readChunkHeaderMem(const byte *buffer, uint bufferSize, int *position, uint32 *header);
	static void memRead(const byte *buffer, uint bufferSize, int *position, void *dest, uint count);
	static int memSeek(int *position, uint limit, uint offset, int whence);
	int decompressMcmp(LFile *file, byte *output, uint outputSize, uint *decodedSize);
	int decompressMcmpFromPath(LFile *file, byte *output, uint outputSize, uint *decodedSize);
	static const char *getFilePath(LFile *file);
	void decodeMcmpBlock(byte *dest, const byte *source, uint size);

private:
	EmiEngine *_engine;
	uint16 _vimaTable[kVimaTableSize];
	bool _vimaTableReady;
	byte *_mcmpReadBuffer;
	byte *_mcmpDecodeBuffer;
};

} // End of namespace Grim

#endif

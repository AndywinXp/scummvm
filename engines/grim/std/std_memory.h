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

#ifndef GRIM_STD_MEMORY_H
#define GRIM_STD_MEMORY_H

#include "common/scummsys.h"

namespace Grim {

/**
 * The memory manager of the game. Every block is preceded by a header; the manager keeps the number of blocks, the
 * sum of their sizes (the size requested, the header is not counted) and the highest sum reached. The sum decides
 * when the resources are purged.
 */
class StdMemory {
public:
	StdMemory();

	void *malloc(int size);
	void free(void *block);
	void *realloc(void *block, int size);
	void *verify(void *block);
	void *tagAlloc(void *block, const char *file, int state);

	int getNumBlocks() const { return _numBlocks; }
	int getTotalBytes() const { return _totalBytes; }
	int getPeakBytes() const { return _peakBytes; }

private:
	// The size of the header is a multiple of 8, so the block that follows it is aligned
	struct Header {
		Header *next;
		Header *prev;
		const char *file;
		int state;
		int line;
		int size;
		int magic;
		int reserved;
	};

	int _numBlocks;
	int _totalBytes;
	int _peakBytes;
	Header *_newest;
};

} // End of namespace Grim

#endif

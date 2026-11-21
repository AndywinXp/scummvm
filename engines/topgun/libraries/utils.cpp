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

#include "common/textconsole.h"

#include "topgun/libraries/utils.h"
#include "topgun/topgun.h"

namespace Topgun {

UtilsLibrary::UtilsLibrary(TopgunEngine *engine) : _engine(engine) {
}

UtilsLibrary::~UtilsLibrary() {
}

void UtilsLibrary::copyHugeBytes(byte *destination, const byte *source, uint32 count) {
	memmove(destination, source, count);
}

byte *UtilsLibrary::tgMalloc(uint32 size, uint16 zeroInit) {
	if (size == 0)
		return nullptr;

	byte *block = (byte *)malloc(size);
	if (!block)
		error("UtilsLibrary::tgMalloc(): not enough memory to allocate %u bytes", size);

	if (zeroInit != 0)
		memset(block, 0, size);

	_blockSizes[block] = size;
	return block;
}

byte *UtilsLibrary::tgPtrCpy(const byte *block) {
	if (!block)
		return nullptr;

	if (!_blockSizes.contains((byte *)block)) {
		warning("UtilsLibrary::tgPtrCpy(): block was not allocated by tgMalloc");
		return nullptr;
	}

	uint32 size = _blockSizes[(byte *)block];
	byte *copy = tgMalloc(size, 0);
	memcpy(copy, block, size);
	return copy;
}

byte *UtilsLibrary::tgRealloc(byte *block, uint32 newSize, uint16 zeroInit) {
	if (!block)
		return tgMalloc(newSize, zeroInit);

	if (!_blockSizes.contains(block)) {
		warning("UtilsLibrary::tgRealloc(): block was not allocated by tgMalloc");
		return nullptr;
	}

	if (newSize == 0)
		error("UtilsLibrary::tgRealloc(): invalid zero size");

	uint32 oldSize = _blockSizes[block];
	byte *newBlock = (byte *)realloc(block, newSize);
	if (!newBlock)
		error("UtilsLibrary::tgRealloc(): not enough memory to allocate %u bytes", newSize);

	if (zeroInit != 0 && newSize > oldSize)
		memset(newBlock + oldSize, 0, newSize - oldSize);

	_blockSizes.erase(block);
	_blockSizes[newBlock] = newSize;
	return newBlock;
}

byte *UtilsLibrary::tgFree(byte *block) {
	if (!block)
		return nullptr;

	if (!_blockSizes.contains(block)) {
		warning("UtilsLibrary::tgFree(): block was not allocated by tgMalloc");
		return nullptr;
	}

	_blockSizes.erase(block);
	free(block);
	return nullptr;
}

} // End of namespace Topgun

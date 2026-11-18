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

#include "engines/grim/std/std_memory.h"

namespace Grim {

// Each header ends with this value
static const int kMagic = 0x12345678;

StdMemory::StdMemory() : _numBlocks(0), _totalBytes(0), _peakBytes(0), _newest(nullptr) {
}

void *StdMemory::malloc(int size) {
	Header *header = (Header *)::malloc(size + sizeof(Header));

	if (!header)
		error("StdMemory::malloc(): out of memory");

	_numBlocks++;

	int newTotal = _totalBytes + size;
	bool newPeak = _peakBytes <= newTotal;

	_totalBytes += size;

	if (newPeak)
		_peakBytes = newTotal;

	header->next = nullptr;
	header->prev = _newest;
	header->file = nullptr;
	header->state = _numBlocks;
	header->line = 0;
	header->size = size;
	header->magic = kMagic;

	if (_newest)
		_newest->next = header;

	_newest = header;

	return header + 1;
}

void StdMemory::free(void *block) {
	if (!block)
		return;

	verify(block);

	Header *header = (Header *)block - 1;

	_numBlocks--;
	_totalBytes -= header->size;

	if (header == _newest)
		_newest = header->prev;

	if (header->next)
		header->next->prev = header->prev;

	if (header->prev)
		header->prev->next = header->next;

	::free(header);
}

void *StdMemory::realloc(void *block, int size) {
	if (!block)
		return malloc(size);

	if (!size) {
		free(block);
		return nullptr;
	}

	verify(block);

	void *newBlock = malloc(size);
	int oldSize = ((Header *)block - 1)->size;
	int newSize = ((Header *)newBlock - 1)->size;

	memcpy(newBlock, block, oldSize < newSize ? oldSize : newSize);
	free(block);

	return newBlock;
}

void *StdMemory::verify(void *block) {
	if (((Header *)block - 1)->magic != kMagic)
		error("StdMemory::verify(): the block has no valid header");

	return block;
}

void *StdMemory::tagAlloc(void *block, const char *file, int state) {
	Header *header = (Header *)block - 1;

	if (header->magic == kMagic) {
		header->file = file;
		header->state = state;
	}

	return block;
}

} // End of namespace Grim

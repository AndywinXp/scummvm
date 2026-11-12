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

#include "common/file.h"
#include "common/str.h"
#include "common/system.h"
#include "common/textconsole.h"

#include "engines/grim/grim.h"
#include "engines/grim/emi/emiengine.h"
#include "engines/grim/emi/files/lfile.h"
#include "engines/grim/emi/core/lmemoryservices.h"

namespace Grim {

static const uint kBlockHeaderSize = 16;

LMemoryServices *LMemoryServices::getCurrent() {
	if (!g_grim)
		return nullptr;

	return static_cast<EmiEngine *>(g_grim)->getMemoryServices();
}

void *LMemoryServices::LMemoryHandler::operator new(size_t size) {
	return LMemoryServices::getCurrent()->mallocImpl(size);
}

void LMemoryServices::LMemoryHandler::operator delete(void *block) {
	LMemoryServices::getCurrent()->freeImpl(block);
}

ThreadBase::ThreadBase() {
	_lockCount = 0;
}

ThreadBase::~ThreadBase() {
}

int ThreadBase::lock() {
	_lockCount++;
	return _lockCount;
}

void ThreadBase::unlock() {
	_lockCount--;
}

ThreadLock::~ThreadLock() {
	_base->unlock();
}

LMemoryServices::LMemoryServices() : _hardCap(0x1000000), _desiredCap(0xC00000), _capLimitsActive(true),
		_memHandlers(nullptr), _handlerCount(0), _memHandlersArraySize(4) {
	_bytesAllocated = 0;
	_memHandlers = (LMemoryHandler **)LMemoryServices::mallocImpl(_memHandlersArraySize * sizeof(LMemoryHandler *));
}

LMemoryServices::~LMemoryServices() {
	LMemoryServices::freeImpl(_memHandlers);
	_memHandlers = nullptr;
}

void *LMemoryServices::mallocImpl(uint size) {
	if (size == 0)
		return nullptr;

	byte *block = (byte *)malloc(size + kBlockHeaderSize);
	if (!block) {
		warning("LMemoryServices::mallocImpl(): out of memory");
		return nullptr;
	}

	*(uint *)block = size;
	_bytesAllocated += size;

	return block + kBlockHeaderSize;
}

void *LMemoryServices::reallocImpl(void *block, uint size) {
	if (!block) {
		byte *newBlock = (byte *)malloc(size + kBlockHeaderSize);
		if (!newBlock) {
			warning("LMemoryServices::reallocImpl(): out of memory");
			return nullptr;
		}

		*(uint *)newBlock = size;
		_bytesAllocated += size;

		return newBlock + kBlockHeaderSize;
	}

	byte *oldBlock = (byte *)block - kBlockHeaderSize;
	uint oldSize = *(uint *)oldBlock;

	if (size == 0) {
		warning("LMemoryServices::reallocImpl(): zero size");
		freeImpl(block);
		return nullptr;
	}

	byte *newBlock = (byte *)realloc(oldBlock, size + kBlockHeaderSize);
	if (!newBlock) {
		warning("LMemoryServices::reallocImpl(): out of memory");
		return nullptr;
	}

	*(uint *)newBlock = size;
	_bytesAllocated = _bytesAllocated - oldSize + size;

	return newBlock + kBlockHeaderSize;
}

void LMemoryServices::freeImpl(void *block) {
	if (!block)
		return;

	byte *oldBlock = (byte *)block - kBlockHeaderSize;
	_bytesAllocated -= *(uint *)oldBlock;

	free(oldBlock);
}

void LMemoryServices::verify(void *block) {
}

void LMemoryServices::verifyAll() {
}

void *LMemoryServices::memsetImpl(void *dest, byte value, uint size) {
	memset(dest, value, size);

	return dest;
}

void *LMemoryServices::memcpyImpl(void *dest, void *src, uint size) {
	memcpy(dest, src, size);

	return dest;
}

bool LMemoryServices::dumpAllocations(LFile *file) {
	return false;
}

bool LMemoryServices::dumpLeaks(const char *fileName) {
	return false;
}

void LMemoryServices::setHardMemoryCap(uint cap) {
	_hardCap = cap;
}

const uint &LMemoryServices::getHardMemoryCap() {
	return _hardCap;
}

const uint &LMemoryServices::getDesiredMemoryCap() {
	return _desiredCap;
}

void LMemoryServices::activateMemoryCaps(bool active) {
	_capLimitsActive = active;
}

bool LMemoryServices::registerMemoryHandler(LMemoryHandler *handler) {
	_threadBase.lock();

	if (_handlerCount >= _memHandlersArraySize) {
		LMemoryHandler **oldHandlers = _memHandlers;
		uint newSize = _memHandlersArraySize + 4;

		LMemoryHandler **newHandlers = (LMemoryHandler **)LMemoryServices::mallocImpl(newSize * sizeof(LMemoryHandler *));
		_memHandlers = newHandlers;
		if (!newHandlers) {
			_memHandlers = oldHandlers;
			_threadBase.unlock();
			return false;
		}

		memset(newHandlers, 0, newSize * sizeof(LMemoryHandler *));
		memcpy(newHandlers, oldHandlers, _memHandlersArraySize * sizeof(LMemoryHandler *));
		LMemoryServices::freeImpl(oldHandlers);
		_memHandlersArraySize = newSize;
	}

	_memHandlers[_handlerCount] = handler;
	_handlerCount++;
	_threadBase.unlock();

	return true;
}

bool LMemoryServices::unregisterMemoryHandler(LMemoryHandler *handler) {
	_threadBase.lock();

	bool result = false;
	uint index = 0;

	if (_handlerCount != 0) {
		while (_memHandlers[index] != handler) {
			index++;
			if (index >= _handlerCount) {
				_threadBase.unlock();
				return false;
			}
		}

		memmove(&_memHandlers[index], &_memHandlers[index + 1], (_handlerCount - index - 1) * sizeof(LMemoryHandler *));
		result = true;
		_handlerCount--;
	}

	_threadBase.unlock();

	return result;
}

LDebugMemoryServices::LDebugMemoryServices() : _allocationId(0) {
	_numAllocations = 0;
	_bytesAllocated = 0;
	_bytesWithOverhead = 0;
	_maxBytes = 0;
	_newestAllocation = nullptr;
}

LDebugMemoryServices::~LDebugMemoryServices() {
	dumpLeaks("leaks2.txt");
}

void *LDebugMemoryServices::mallocImpl(uint size) {
	uint handlerIndex = 0;

	_threadBase.lock();

	if (_capLimitsActive) {
		uint overhead = _bytesWithOverhead;

		if (overhead + size >= _hardCap) {
			uint freed = 0;
			uint needed = size + overhead - _hardCap;
			uint totalFreed = 0;

			if (_handlerCount != 0) {
				while (needed > freed) {
					uint before = overhead;

					_memHandlers[handlerIndex]->purgeAmount(needed - freed);
					overhead = _bytesWithOverhead;
					handlerIndex++;
					totalFreed += before - overhead;

					if (handlerIndex >= _handlerCount)
						break;

					freed = totalFreed;
				}
			}
		}
	}

	LAllocation *allocation = (LAllocation *)malloc(size + sizeof(LAllocation));
	if (!allocation) {
		warning("LDebugMemoryServices::mallocImpl(): out of memory");
		_threadBase.unlock();
		return nullptr;
	}

	allocation->newer = nullptr;
	allocation->older = _newestAllocation;
	allocation->id = _allocationId;
	allocation->size = size;
	allocation->guard = 0x12345678;

	_allocationId++;
	_numAllocations++;
	_bytesAllocated += size;
	_bytesWithOverhead += size + sizeof(LAllocation);

	if ((int)_bytesWithOverhead > _maxBytes)
		_maxBytes = _bytesWithOverhead;

	if (_newestAllocation)
		_newestAllocation->newer = allocation;

	_newestAllocation = allocation;

	_threadBase.unlock();

	return allocation + 1;
}

void *LDebugMemoryServices::reallocImpl(void *block, uint size) {
	if (!block)
		return mallocImpl(size);

	if (size == 0) {
		freeImpl(block);
		return nullptr;
	}

	verify(block);

	void *newBlock = mallocImpl(size);
	if (!newBlock) {
		warning("LDebugMemoryServices::reallocImpl(): out of memory");
		return nullptr;
	}

	LAllocation *newHeader = (LAllocation *)newBlock - 1;
	LAllocation *oldHeader = (LAllocation *)block - 1;

	uint copySize = newHeader->size;
	if (copySize >= oldHeader->size)
		copySize = oldHeader->size;

	memcpy(newBlock, block, copySize);
	freeImpl(block);

	return newBlock;
}

void LDebugMemoryServices::freeImpl(void *block) {
	_threadBase.lock();

	if (block) {
		LAllocation *allocation = (LAllocation *)block - 1;

		verify(block);
		_bytesAllocated -= allocation->size;
		_numAllocations--;
		_bytesWithOverhead = _bytesWithOverhead - allocation->size - sizeof(LAllocation);

		if (allocation == _newestAllocation)
			_newestAllocation = allocation->older;

		if (allocation->newer)
			allocation->newer->older = allocation->older;

		if (allocation->older)
			allocation->older->newer = allocation->newer;

		free(allocation);
	}

	_threadBase.unlock();
}

void LDebugMemoryServices::verify(LAllocation *allocation) {
}

void LDebugMemoryServices::verify(void *block) {
	verify((LAllocation *)block - 1);
}

void LDebugMemoryServices::verifyAll() {
	LAllocation *allocation;

	for (allocation = _newestAllocation; allocation; allocation = allocation->older)
		verify(allocation);
}

bool LDebugMemoryServices::dumpAllocations(LFile *file) {
	static const char *dayNames[7] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
	static const char *monthNames[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
	int totalBytes = 0;
	int allocations = 0;

	_threadBase.lock();

	if (!file) {
		_threadBase.unlock();
		return false;
	}

	TimeDate td;
	g_system->getTimeAndDate(td);
	file->writeFormat("Memory Allocation Report: %s %s %2d %02d:%02d:%02d %d\n\n", dayNames[td.tm_wday], monthNames[td.tm_mon],
		td.tm_mday, td.tm_hour, td.tm_min, td.tm_sec, td.tm_year + 1900);

	LAllocation *allocation = _newestAllocation;
	while (allocation) {
		totalBytes += allocation->size;
		allocations++;
		file->writeFormat("#%d : %d bytes", allocation->id, allocation->size);

		if ((int)allocation->size > 1024)
			file->writeFormat("!!!!");

		file->writeFormat("\n");
		allocation = allocation->older;
	}

	file->writeFormat("\tAllocations: %d\n", allocations);
	file->writeFormat("\tAllocated Bytes: %d (%fk)\n", totalBytes, (double)totalBytes * 0.0009765625);
	file->writeFormat("\tMax memory allocated %d (%f MB)\n", _maxBytes, (double)_maxBytes * 0.00000095367432);

	_threadBase.unlock();

	return true;
}

bool LDebugMemoryServices::dumpLeaks(const char *fileName) {
	static const char *dayNames[7] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
	static const char *monthNames[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
	int totalBytes = 0;
	int leaks = 0;

	_threadBase.lock();

	Common::DumpFile file;
	if (!file.open(Common::Path(fileName))) {
		_threadBase.unlock();
		return false;
	}

	TimeDate td;
	g_system->getTimeAndDate(td);
	file.writeString(Common::String::format("Depend report %s %s%3d %02d:%02d:%02d %d\n\n", dayNames[td.tm_wday], monthNames[td.tm_mon],
		td.tm_mday, td.tm_hour, td.tm_min, td.tm_sec, td.tm_year + 1900));

	LAllocation *allocation = _newestAllocation;
	while (allocation) {
		totalBytes += allocation->size;
		leaks++;
		file.writeString(Common::String::format("#%d : %d bytes\n", allocation->id, allocation->size));
		file.flush();
		allocation = allocation->older;
	}

	file.writeString(Common::String::format("\tLeaks %d\n", leaks));
	file.writeString(Common::String::format("\tLeaked bytes %d (%fk)\n", totalBytes, (double)totalBytes * 0.0009765625));
	file.writeString(Common::String::format("\tMax memory allocated %d (%f MB)\n", _maxBytes, (double)_maxBytes * 0.00000095367432));
	file.close();

	_threadBase.unlock();

	return leaks > 0;
}

} // End of namespace Grim

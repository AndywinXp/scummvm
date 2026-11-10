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

#include "engines/grim/emi/lfile.h"
#include "engines/grim/emi/lmemoryservices.h"

namespace Grim {

static const uint kBlockHeaderSize = 16;

uint LMemoryServices::mHardCap = 0x1000000;
uint LMemoryServices::mDesiredCap = 0xC00000;
bool LMemoryServices::mbCapLimitsActive = true;
LMemoryServices::LMemoryHandler **LMemoryServices::mMemHandlers = nullptr;
uint LMemoryServices::mNHandlers = 0;
uint LMemoryServices::mMemHandlersArraySize = 0;

uint LDebugMemoryServices::allocationId = 0;

static LMemoryServices gDefaultMemoryServices;
LMemoryServices *gpMemoryServices = &gDefaultMemoryServices;

void *LMemoryServices::LMemoryHandler::operator new(size_t size) {
	return gpMemoryServices->malloc(size);
}

void LMemoryServices::LMemoryHandler::operator delete(void *block) {
	gpMemoryServices->free(block);
}

ThreadBase::ThreadBase() {
	lockCount = 0;
}

ThreadBase::~ThreadBase() {
}

int ThreadBase::lock() {
	lockCount++;
	return lockCount;
}

void ThreadBase::unlock() {
	lockCount--;
}

ThreadLock::~ThreadLock() {
	base->unlock();
}

LMemoryServices::LMemoryServices() {
	bytesAllocated = 0;
	mMemHandlersArraySize = 4;
	mNHandlers = 0;
	mMemHandlers = (LMemoryHandler **)gpMemoryServices->malloc(mMemHandlersArraySize * sizeof(LMemoryHandler *));
	mbCapLimitsActive = true;
}

LMemoryServices::~LMemoryServices() {
	gpMemoryServices->free(mMemHandlers);
	mMemHandlers = nullptr;
}

void *LMemoryServices::malloc(uint size) {
	if (size == 0)
		return nullptr;

	byte *block = (byte *)::malloc(size + kBlockHeaderSize);
	if (!block) {
		warning("LMemoryServices::malloc(): out of memory");
		return nullptr;
	}

	*(uint *)block = size;
	bytesAllocated += size;

	return block + kBlockHeaderSize;
}

void *LMemoryServices::realloc(void *block, uint size) {
	if (!block) {
		byte *newBlock = (byte *)::malloc(size + kBlockHeaderSize);
		if (!newBlock) {
			warning("LMemoryServices::realloc(): out of memory");
			return nullptr;
		}

		*(uint *)newBlock = size;
		bytesAllocated += size;

		return newBlock + kBlockHeaderSize;
	}

	byte *oldBlock = (byte *)block - kBlockHeaderSize;
	uint oldSize = *(uint *)oldBlock;

	if (size == 0) {
		warning("LMemoryServices::realloc(): zero size");
		free(block);
		return nullptr;
	}

	byte *newBlock = (byte *)::realloc(oldBlock, size + kBlockHeaderSize);
	if (!newBlock) {
		warning("LMemoryServices::realloc(): out of memory");
		return nullptr;
	}

	*(uint *)newBlock = size;
	bytesAllocated = bytesAllocated - oldSize + size;

	return newBlock + kBlockHeaderSize;
}

void LMemoryServices::free(void *block) {
	if (!block)
		return;

	byte *oldBlock = (byte *)block - kBlockHeaderSize;
	bytesAllocated -= *(uint *)oldBlock;

	::free(oldBlock);
}

void LMemoryServices::verify(void *block) {
}

void LMemoryServices::verifyAll() {
}

void *LMemoryServices::memset(void *dest, byte value, uint size) {
	::memset(dest, value, size);

	return dest;
}

void *LMemoryServices::memcpy(void *dest, void *src, uint size) {
	::memcpy(dest, src, size);

	return dest;
}

bool LMemoryServices::dumpAllocations(LFile *file) {
	return false;
}

bool LMemoryServices::dumpLeaks(const char *fileName) {
	return false;
}

void LMemoryServices::setHardMemoryCap(uint cap) {
	mHardCap = cap;
}

const uint &LMemoryServices::getHardMemoryCap() {
	return mHardCap;
}

const uint &LMemoryServices::getDesiredMemoryCap() {
	return mDesiredCap;
}

void LMemoryServices::activateMemoryCaps(bool active) {
	mbCapLimitsActive = active;
}

bool LMemoryServices::registerMemoryHandler(LMemoryHandler *handler) {
	threadBase.lock();

	if (mNHandlers >= mMemHandlersArraySize) {
		LMemoryHandler **oldHandlers = mMemHandlers;
		uint newSize = mMemHandlersArraySize + 4;

		LMemoryHandler **newHandlers = (LMemoryHandler **)gpMemoryServices->malloc(newSize * sizeof(LMemoryHandler *));
		mMemHandlers = newHandlers;
		if (!newHandlers) {
			mMemHandlers = oldHandlers;
			threadBase.unlock();
			return false;
		}

		::memset(newHandlers, 0, newSize * sizeof(LMemoryHandler *));
		::memcpy(newHandlers, oldHandlers, mMemHandlersArraySize * sizeof(LMemoryHandler *));
		gpMemoryServices->free(oldHandlers);
		mMemHandlersArraySize = newSize;
	}

	mMemHandlers[mNHandlers] = handler;
	mNHandlers++;
	threadBase.unlock();

	return true;
}

bool LMemoryServices::unregisterMemoryHandler(LMemoryHandler *handler) {
	threadBase.lock();

	bool result = false;
	uint index = 0;

	if (mNHandlers != 0) {
		while (mMemHandlers[index] != handler) {
			index++;
			if (index >= mNHandlers) {
				threadBase.unlock();
				return false;
			}
		}

		memmove(&mMemHandlers[index], &mMemHandlers[index + 1], (mNHandlers - index - 1) * sizeof(LMemoryHandler *));
		result = true;
		mNHandlers--;
	}

	threadBase.unlock();

	return result;
}

LDebugMemoryServices::LDebugMemoryServices() {
	numAllocations = 0;
	bytesAllocated = 0;
	bytesWithOverhead = 0;
	maxBytes = 0;
	newestAllocation = nullptr;
}

LDebugMemoryServices::~LDebugMemoryServices() {
	dumpLeaks("leaks2.txt");
}

void *LDebugMemoryServices::malloc(uint size) {
	uint handlerIndex = 0;

	threadBase.lock();

	if (mbCapLimitsActive) {
		uint overhead = bytesWithOverhead;

		if (overhead + size >= mHardCap) {
			uint freed = 0;
			uint needed = size + overhead - mHardCap;
			uint totalFreed = 0;

			if (mNHandlers != 0) {
				while (needed > freed) {
					uint before = overhead;

					mMemHandlers[handlerIndex]->purgeAmount(needed - freed);
					overhead = bytesWithOverhead;
					handlerIndex++;
					totalFreed += before - overhead;

					if (handlerIndex >= mNHandlers)
						break;

					freed = totalFreed;
				}
			}
		}
	}

	LAllocation *allocation = (LAllocation *)::malloc(size + sizeof(LAllocation));
	if (!allocation) {
		warning("LDebugMemoryServices::malloc(): out of memory");
		threadBase.unlock();
		return nullptr;
	}

	allocation->newer = nullptr;
	allocation->older = newestAllocation;
	allocation->id = allocationId;
	allocation->size = size;
	allocation->guard = 0x12345678;

	allocationId++;
	numAllocations++;
	bytesAllocated += size;
	bytesWithOverhead += size + sizeof(LAllocation);

	if ((int)bytesWithOverhead > maxBytes)
		maxBytes = bytesWithOverhead;

	if (newestAllocation)
		newestAllocation->newer = allocation;

	newestAllocation = allocation;

	threadBase.unlock();

	return allocation + 1;
}

void *LDebugMemoryServices::realloc(void *block, uint size) {
	if (!block)
		return malloc(size);

	if (size == 0) {
		free(block);
		return nullptr;
	}

	verify(block);

	void *newBlock = malloc(size);
	if (!newBlock) {
		warning("LDebugMemoryServices::realloc(): out of memory");
		return nullptr;
	}

	LAllocation *newHeader = (LAllocation *)newBlock - 1;
	LAllocation *oldHeader = (LAllocation *)block - 1;

	uint copySize = newHeader->size;
	if (copySize >= oldHeader->size)
		copySize = oldHeader->size;

	::memcpy(newBlock, block, copySize);
	free(block);

	return newBlock;
}

void LDebugMemoryServices::free(void *block) {
	threadBase.lock();

	if (block) {
		LAllocation *allocation = (LAllocation *)block - 1;

		verify(block);
		bytesAllocated -= allocation->size;
		numAllocations--;
		bytesWithOverhead = bytesWithOverhead - allocation->size - sizeof(LAllocation);

		if (allocation == newestAllocation)
			newestAllocation = allocation->older;

		if (allocation->newer)
			allocation->newer->older = allocation->older;

		if (allocation->older)
			allocation->older->newer = allocation->newer;

		::free(allocation);
	}

	threadBase.unlock();
}

void LDebugMemoryServices::verify(LAllocation *allocation) {
}

void LDebugMemoryServices::verify(void *block) {
	verify((LAllocation *)block - 1);
}

void LDebugMemoryServices::verifyAll() {
	LAllocation *allocation;

	for (allocation = newestAllocation; allocation; allocation = allocation->older)
		verify(allocation);
}

bool LDebugMemoryServices::dumpAllocations(LFile *file) {
	static const char *dayNames[7] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
	static const char *monthNames[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
	int totalBytes = 0;
	int allocations = 0;

	threadBase.lock();

	if (!file) {
		threadBase.unlock();
		return false;
	}

	TimeDate td;
	g_system->getTimeAndDate(td);
	file->writeFormat("Memory Allocation Report: %s %s %2d %02d:%02d:%02d %d\n\n", dayNames[td.tm_wday], monthNames[td.tm_mon],
		td.tm_mday, td.tm_hour, td.tm_min, td.tm_sec, td.tm_year + 1900);

	LAllocation *allocation = newestAllocation;
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
	file->writeFormat("\tMax memory allocated %d (%f MB)\n", maxBytes, (double)maxBytes * 0.00000095367432);

	threadBase.unlock();

	return true;
}

bool LDebugMemoryServices::dumpLeaks(const char *fileName) {
	static const char *dayNames[7] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
	static const char *monthNames[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
	int totalBytes = 0;
	int leaks = 0;

	threadBase.lock();

	Common::DumpFile file;
	if (!file.open(Common::Path(fileName))) {
		threadBase.unlock();
		return false;
	}

	TimeDate td;
	g_system->getTimeAndDate(td);
	file.writeString(Common::String::format("Depend report %s %s%3d %02d:%02d:%02d %d\n\n", dayNames[td.tm_wday], monthNames[td.tm_mon],
		td.tm_mday, td.tm_hour, td.tm_min, td.tm_sec, td.tm_year + 1900));

	LAllocation *allocation = newestAllocation;
	while (allocation) {
		totalBytes += allocation->size;
		leaks++;
		file.writeString(Common::String::format("#%d : %d bytes\n", allocation->id, allocation->size));
		file.flush();
		allocation = allocation->older;
	}

	file.writeString(Common::String::format("\tLeaks %d\n", leaks));
	file.writeString(Common::String::format("\tLeaked bytes %d (%fk)\n", totalBytes, (double)totalBytes * 0.0009765625));
	file.writeString(Common::String::format("\tMax memory allocated %d (%f MB)\n", maxBytes, (double)maxBytes * 0.00000095367432));
	file.close();

	threadBase.unlock();

	return leaks > 0;
}

} // End of namespace Grim

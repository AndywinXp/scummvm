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

#ifndef GRIM_EMI_LMEMORYSERVICES_H
#define GRIM_EMI_LMEMORYSERVICES_H

#include "common/scummsys.h"

namespace Grim {

class LFile;

class ThreadBase {
public:
	ThreadBase();
	~ThreadBase();

	int lock();
	void unlock();

	int lockCount;
};

class ThreadLock {
public:
	~ThreadLock();

	ThreadBase *base;
};

struct LAllocation {
	LAllocation *newer;
	LAllocation *older;
	uint id;
	uint size;
	uint guard;
};

class LMemoryServices {
public:
	class LMemoryHandler {
	public:
		virtual ~LMemoryHandler() {}

		void *operator new(size_t size);
		void operator delete(void *block);

		virtual uint purgeAmount(uint bytes) = 0;
	};

	LMemoryServices();
	~LMemoryServices();

	virtual void *malloc(uint size);
	virtual void *realloc(void *block, uint size);
	virtual void free(void *block);
	virtual void verify(void *block);
	virtual void verifyAll();
	virtual void *memset(void *dest, byte value, uint size);
	virtual void *memcpy(void *dest, void *src, uint size);
	virtual bool dumpAllocations(LFile *file);
	virtual bool dumpLeaks(const char *fileName);

	void setHardMemoryCap(uint cap);
	const uint &getHardMemoryCap();
	const uint &getDesiredMemoryCap();
	void activateMemoryCaps(bool active);
	bool registerMemoryHandler(LMemoryHandler *handler);
	bool unregisterMemoryHandler(LMemoryHandler *handler);

	ThreadBase threadBase;
	uint bytesAllocated;

	static uint mHardCap;
	static uint mDesiredCap;
	static bool mbCapLimitsActive;
	static LMemoryHandler **mMemHandlers;
	static uint mNHandlers;
	static uint mMemHandlersArraySize;
};

class LDebugMemoryServices : public LMemoryServices {
public:
	LDebugMemoryServices();
	~LDebugMemoryServices();

	void *malloc(uint size) override;
	void *realloc(void *block, uint size) override;
	void free(void *block) override;
	void verify(LAllocation *allocation);
	void verify(void *block) override;
	void verifyAll() override;
	bool dumpAllocations(LFile *file) override;
	bool dumpLeaks(const char *fileName) override;

	uint numAllocations;
	uint bytesWithOverhead;
	int maxBytes;
	LAllocation *newestAllocation;

	static uint allocationId;
};

extern LMemoryServices *gpMemoryServices;

} // End of namespace Grim

#endif

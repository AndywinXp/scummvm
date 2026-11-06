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

#ifndef GRIM_RES_H
#define GRIM_RES_H

#include "common/endian.h"
#include "common/scummsys.h"
#include "common/str.h"

#include "engines/grim/std/std_sys.h"

namespace Common {
class DumpFile;
class ReadStream;
class WriteStream;
}

namespace Grim {

enum ResTags {
	kResTagDefaultHandler = MKTAG('D', 'E', 'F', 'H'),
	kResTagResf = MKTAG('R', 'E', 'S', 'F')
};

struct ResHandlerMsg {
	int op;
	const char *pName;
	int tag;
	void *data;
	int result;
};

class ResHandler {
public:
	virtual ~ResHandler() {}

	virtual int handler(ResHandlerMsg *pMsg) = 0;
};

struct ResRef {
	int valid;
	int serial;
	ResRef *pPrev;
	ResRef *pNext;
	void *handle;
};

struct ResNode {
	ResNode *pNext;
	ResNode *pPrev;
	char *pName;
	uint hash;
	int tag;
	void *handle;
	int serial;
	uint16 flags;
	ResRef *pRefs;
	int size;
};

struct ResTag {
	ResTag *pNext;
	ResTag *pPrev;
	int tag;
	ResHandler *pHandler;
};

class SputRender;

class Res {
public:
	Res();
	~Res();

	void setSputRender(SputRender *pSputRender) { _pSputRender = pSputRender; }

	int sysInitialize(SysStruct *pHS);
	void sysTerminate();
	int sysStartup();
	int sysShutdown();
	int sysRegisterHandler(int tag, ResHandler *pHandler);
	int sysSave(Common::WriteStream *pStream);
	int sysLoad(Common::ReadStream *pStream);
	int allocCompletion(int tag, const char *name, void *handle);
	void *getResource(int tag, const char *name);
	int setPurgeOnFree(void *handle, int bPurge);
	int freeResources(uint bytesWanted, uint *pBytesFreed);
	int dump(const char *path, int tag);
	int log(const char *path);
	ResTag *getResTagStruct(int tag);
	ResNode *checkAllocatedRes(void *handle);
	void *handleFromSerialNumber(int serial);
	ResNode *resourceHandleFromSerialNumber(int serial);
	int loaded(int tag, const char *name);
	ResNode *isResourceLoaded(int tag, const char *name);
	ResNode *checkForNeedLoadRes(int tag, const char *name, int *pResult);
	ResNode *checkList1(int tag, const char *name);
	ResNode *locateResByName(int tag, const char *name, ResNode *pStart);
	ResNode *load(int tag, const char *name, void *handle);
	void unlinkNode(ResNode *pNode);
	int freeNode(ResNode *pNode);
	int loadFileData(ResHandlerMsg *pMsg);
	int getUserField(ResNode *pNode);
	int totalAllocated();
	void checkAnotherLists(ResNode *pNode);
	uint changeFileNameToUpper(const char *src, char *dst);
	int findExternalFile(const char *name, char *pOutPath, uint size);
	void setReference(ResRef *pRef, void *handle);
	void removeReference(ResRef *pRef);
	void allocatedIterFirst(ResNode **ppNext, void **pHandle, int tag);
	void allocatedIterNext(ResNode **ppNext, void **pHandle, int tag);
	void touch(void *handle);
	void resolveToSerialNumber(ResRef *pRef);
	void freeAll();
	int countDependents(void *handle);
	int initLists(int tag, const char *name);
	void setMemoryLimitMB(int megabytes);
	int setLogFile(const char *path);

	int getMemoryLimit() const { return _memoryLimit; }

private:
	SysStruct *_pResHS;
	int _fResSysInitialized;
	int _bStarted;
	ResTag *_pTagHead;
	ResNode *_pNodeHead;
	ResNode *_pNodeTail;
	ResHandler *_pDefaultHandler;
	Common::DumpFile *_logFile;
	int _memoryLimit;
	SputRender *_pSputRender;
};

} // End of namespace Grim

#endif

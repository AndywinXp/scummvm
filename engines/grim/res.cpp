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

#include "common/archive.h"
#include "common/file.h"
#include "common/fs.h"
#include "common/stream.h"
#include "common/textconsole.h"

#include "engines/grim/grim.h"
#include "engines/grim/imuse/imuse_engine.h"
#include "engines/grim/lab.h"
#include "engines/grim/res.h"
#include "engines/grim/sputrender.h"

namespace Grim {

static const int kLabPriorityBase = -1000;
static const int kDirectoryPriorityBase = 2;

Res::Res(bool isDemo) : _pResHS(nullptr), _fResSysInitialized(0), _bStarted(0), _nextLabPriority(kLabPriorityBase),
		_nextDirectoryPriority(kDirectoryPriorityBase),
		_pTagHead(nullptr), _pNodeHead(nullptr), _pNodeTail(nullptr), _pDefaultHandler(nullptr),
		_logFile(nullptr), _memoryLimit(0x1800000), _pSputRender(nullptr), _bDemo(isDemo) {
}

Res::~Res() {
	delete _logFile;
	_logFile = nullptr;
}

int Res::sysInitialize(SysStruct *pHS) {
	_pResHS = pHS;

	if (_fResSysInitialized)
		return 1;

	_fResSysInitialized = 1;
	_pTagHead = nullptr;
	_pNodeTail = nullptr;
	_pNodeHead = nullptr;

	return 0;
}

void Res::sysTerminate() {
	_fResSysInitialized = 0;
}

int Res::sysStartup() {
	if (!_fResSysInitialized)
		return 1;

	if (_bStarted)
		return 1;

	_bStarted = 1;

	return 0;
}

int Res::sysShutdown() {
	if (_bStarted) {
		fileCloseAll();
		freeAll();

		while (_pTagHead) {
			ResTag *pNext = _pTagHead->pNext;
			delete _pTagHead;
			_pTagHead = pNext;
		}
	}

	_bStarted = 0;

	return 0;
}

int Res::sysRegisterHandler(int tag, ResHandler *pHandler) {
	if (tag == kResTagDefaultHandler) {
		_pDefaultHandler = pHandler;
		return 1;
	}

	ResTag *pTag = _pTagHead;

	while (pTag) {
		if (pTag->tag == tag)
			break;

		pTag = pTag->pNext;
	}

	if (pTag)
		return 2;

	pTag = new ResTag();
	pTag->tag = tag;
	pTag->pHandler = pHandler;
	pTag->pNext = _pTagHead;

	if (_pTagHead)
		_pTagHead->pPrev = pTag;

	pTag->pPrev = nullptr;
	_pTagHead = pTag;

	return 1;
}

int Res::fileOpen(const char *path) {
	if (!*path || path[strlen(path) - 1] == '\\' || path[strlen(path) - 1] == '/')
		return fileOpenSearch(path);

	return openLabFile(path);
}

int Res::fileClose(const char *path) {
	int index = lookupFileHandle(path);

	if (index >= 0) {
		SearchMan.remove(_labFiles[index].archiveName);
		_labFiles.remove_at(index);
	}

	return 1;
}

void Res::fileCloseAll() {
	while (!_labFiles.empty()) {
		Common::String path(_labFiles[0].path);

		fileClose(path.c_str());
	}

	_nextLabPriority = kLabPriorityBase;
	_nextDirectoryPriority = kDirectoryPriorityBase;
}

int Res::fileMoveToFront(const char *path) {
	int index = lookupFileHandle(path);

	if (index < 0)
		return 0;

	if (_labFiles[index].flags == kResLabFileLab) {
		SearchMan.setPriority(_labFiles[index].archiveName, _nextLabPriority);
		_nextLabPriority++;
	} else {
		SearchMan.setPriority(_labFiles[index].archiveName, _nextDirectoryPriority);
		_nextDirectoryPriority++;
	}

	return 1;
}

int Res::openLabFile(const char *path) {
	Common::Path labPath(path, Common::Path::kNativeSeparator);
	Common::String name(labPath.baseName());

	name.toLowercase();

	if (SearchMan.hasArchive(name))
		return 1;

	if (!SearchMan.hasFile(Common::Path(name))) {
		warning("Res::openLabFile(): %s is not reachable", path);
		return 0;
	}

	Lab *pLab = new Lab();

	if (!pLab->open(Common::Path(name), false)) {
		delete pLab;
		pLab = nullptr;

		return 0;
	}

	SearchMan.add(name, pLab, _nextLabPriority, true);

	ResLabFile labFile;

	labFile.path = path;
	labFile.archiveName = name;
	labFile.flags = kResLabFileLab;
	_labFiles.push_back(labFile);

	return 1;
}

int Res::fileOpenSearch(const char *dirPath) {
	Common::String directoryName(dirPath);

	if (!directoryName.empty() && (directoryName.lastChar() == '\\' || directoryName.lastChar() == '/'))
		directoryName.deleteLastChar();

	if (directoryName.empty())
		return 0;

	Common::FSNode directory(Common::Path(directoryName, Common::Path::kNativeSeparator));
	Common::FSList files;

	if (!directory.exists() || !directory.isDirectory() || !directory.getChildren(files, Common::FSNode::kListFilesOnly) || files.empty())
		return 0;

	Common::String name(directory.getPath().toString());

	if (SearchMan.hasArchive(name))
		return 1;

	SearchMan.add(name, new Common::FSDirectory(directory, 1), _nextDirectoryPriority, true);

	ResLabFile labFile;

	labFile.path = dirPath;
	labFile.archiveName = name;
	labFile.flags = kResLabFileDirectory;
	_labFiles.push_back(labFile);

	return 1;
}

int Res::lookupFileHandle(const char *path) {
	for (uint i = 0; i < _labFiles.size(); i++) {
		if (_labFiles[i].path == path)
			return i;
	}

	return -1;
}

int Res::allocCompletion(int tag, const char *name, void *handle) {
	if (checkAllocatedRes(handle))
		return 4;

	ResNode *pNode = load(tag, name, handle);

	if (!pNode)
		return 0;

	if (_logFile)
		_logFile->writeString(Common::String::format("Resource <%-12s> allocated: %8d bytes\n", name, getUserField(pNode)));

	return 1;
}

void *Res::getResource(int tag, const char *name) {
	int result;
	ResNode *pNode = checkForNeedLoadRes(tag, name, &result);

	if (pNode) {
		checkAnotherLists(pNode);
		return pNode->handle;
	}

	return nullptr;
}

int Res::setPurgeOnFree(void *handle, int bPurge) {
	int result = 0;
	ResNode *pNode = checkAllocatedRes(handle);

	if (pNode) {
		if (!(pNode->flags & 8))
			result = 1;

		if (bPurge)
			pNode->flags &= 0xFFF7;
		else
			pNode->flags |= 8;
	}

	return result;
}

int Res::freeResources(uint bytesWanted, uint *pBytesFreed) {
	int bFreedAny = 1;
	int numVisited = 0;

	*pBytesFreed = 0;

	if (bytesWanted) {
		while (bFreedAny) {
			ResNode *pNode = _pNodeTail;
			bFreedAny = 0;

			if (pNode) {
				int bListEnded = 0;

				while (*pBytesFreed < bytesWanted) {
					numVisited++;

					if (g_grim->getImuse() && !(numVisited % 5))
						g_grim->getImuse()->soundsPeriodic();

					ResNode *pPrev = pNode->pPrev;

					if (!pNode->pRefs && !(pNode->flags & 8)) {
						*pBytesFreed += getUserField(pNode);
						unlinkNode(pNode);
						freeNode(pNode);
						bFreedAny = 1;
					}

					pNode = pPrev;

					if (!pNode) {
						bListEnded = 1;
						break;
					}
				}

				if (!bListEnded)
					return 1;
			}

			if (*pBytesFreed >= bytesWanted)
				break;
		}
	}

	if (*pBytesFreed >= bytesWanted)
		return 1;

	return 0;
}

int Res::dump(const char *path, int tag) {
	int totalBytes = 0;
	int numItems = 0;
	Common::DumpFile file;

	if (!file.open(path))
		return 0;

	for (ResNode *pNode = _pNodeHead; pNode; pNode = pNode->pNext) {
		if (!tag || pNode->tag == tag) {
			const char *pStatus = "";

			if (pNode->pRefs)
				pStatus = "REFERENCED";

			file.writeString(Common::String::format("Resource %-12s,\t\tsize = %7d\t%s\n", pNode->pName, getUserField(pNode), pStatus));
			totalBytes += getUserField(pNode);
			numItems++;
		}
	}

	file.writeString(Common::String::format("Total: %d items, %d bytes\n", numItems, totalBytes));
	file.close();

	return 1;
}

int Res::log(const char *path) {
	if (path) {
		delete _logFile;
		_logFile = new Common::DumpFile();

		if (!_logFile->open(path)) {
			delete _logFile;
			_logFile = nullptr;
		}

		return 1;
	}

	if (_logFile) {
		delete _logFile;
		_logFile = nullptr;
	}

	return 1;
}

ResTag *Res::getResTagStruct(int tag) {
	ResTag *pTag = _pTagHead;

	while (pTag) {
		if (pTag->tag == tag)
			return pTag;

		pTag = pTag->pNext;
	}

	return nullptr;
}

ResNode *Res::checkAllocatedRes(void *handle) {
	ResNode *pNode = _pNodeHead;

	while (pNode) {
		if (pNode->handle == handle)
			return pNode;

		pNode = pNode->pNext;
	}

	return nullptr;
}

void *Res::handleFromSerialNumber(int serial) {
	ResNode *pNode = resourceHandleFromSerialNumber(serial);

	if (pNode)
		return pNode->handle;

	return nullptr;
}

int Res::serialFromHandle(void *handle) {
	ResNode *pNode = checkAllocatedRes(handle);

	if (pNode)
		return pNode->serial;

	return (int)(intptr)handle;
}

ResNode *Res::resourceHandleFromSerialNumber(int serial) {
	ResNode *pNode = _pNodeHead;

	while (pNode) {
		if (pNode->serial == serial)
			return pNode;

		pNode = pNode->pNext;
	}

	return nullptr;
}

int Res::loaded(int tag, const char *name) {
	return isResourceLoaded(tag, name) != nullptr;
}

ResNode *Res::isResourceLoaded(int tag, const char *name) {
	char upperName[260];

	if (strlen(name) >= sizeof(upperName)) {
		warning("Res::isResourceLoaded(): name too long");
		return nullptr;
	}

	uint hash = changeFileNameToUpper(name, upperName);
	ResNode *pNode = _pNodeHead;

	while (pNode) {
		if (pNode->tag == tag && pNode->hash == hash && !(pNode->flags & 2) && !strcmp(pNode->pName, upperName))
			return pNode;

		pNode = pNode->pNext;
	}

	return nullptr;
}

ResNode *Res::checkForNeedLoadRes(int tag, const char *name, int *pResult) {
	ResNode *pNode = isResourceLoaded(tag, name);

	if (pNode) {
		*pResult = 4;
		return pNode;
	}

	ResTag *pTag = getResTagStruct(tag);
	ResHandlerMsg msg;

	msg.op = 1;
	msg.pName = name;
	msg.tag = tag;
	msg.data = nullptr;
	msg.result = 0;

	// The demo has no loading image and does not update the sounds while loading
	if (!_bDemo)
		_pSputRender->drawLoadingImage();

	if (pTag)
		*pResult = pTag->pHandler->handler(&msg);
	else if (_pDefaultHandler)
		*pResult = _pDefaultHandler->handler(&msg);
	else
		*pResult = loadFileData(&msg);

	if (g_grim->getImuse() && !_bDemo)
		g_grim->getImuse()->soundsPeriodic();

	if (*pResult == 1) {
		pNode = checkList1(tag, name);

		// The demo asks the handler for the size every time it is needed
		if (pNode && !_bDemo) {
			ResHandlerMsg sizeMsg;

			sizeMsg.op = 3;
			sizeMsg.pName = pNode->pName;
			sizeMsg.tag = pNode->tag;
			sizeMsg.data = pNode->handle;
			sizeMsg.result = 0;

			if (pTag) {
				pTag->pHandler->handler(&sizeMsg);
			} else {
				warning("Res::checkForNeedLoadRes(): no handler registered for tag %d", tag);

				if (_pDefaultHandler)
					_pDefaultHandler->handler(&sizeMsg);
				else
					loadFileData(&sizeMsg);
			}

			pNode->size = sizeMsg.result;
		}
	}

	return pNode;
}

ResNode *Res::checkList1(int tag, const char *name) {
	return locateResByName(tag, name, _pNodeHead);
}

ResNode *Res::locateResByName(int tag, const char *name, ResNode *pStart) {
	char upperName[260];

	if (strlen(name) >= sizeof(upperName)) {
		warning("Res::locateResByName(): name too long");
		return nullptr;
	}

	uint hash = changeFileNameToUpper(name, upperName);
	ResNode *pNode = pStart;

	while (pNode) {
		if (pNode->tag == tag && pNode->hash == hash && !strcmp(pNode->pName, upperName))
			return pNode;

		pNode = pNode->pNext;
	}

	return nullptr;
}

ResNode *Res::load(int tag, const char *name, void *handle) {
	ResNode *pNode = new ResNode();

	pNode->tag = tag;
	pNode->handle = handle;
	pNode->serial = (int)(intptr)handle;
	pNode->flags = 0;
	pNode->pRefs = nullptr;
	pNode->pName = nullptr;

	pNode->pName = new char[strlen(name) + 1];
	pNode->hash = changeFileNameToUpper(name, pNode->pName);
	pNode->flags |= 4;

	pNode->pNext = _pNodeHead;

	if (_pNodeHead)
		_pNodeHead->pPrev = pNode;

	pNode->pPrev = nullptr;
	_pNodeHead = pNode;

	if (!_pNodeTail)
		_pNodeTail = pNode;

	checkAnotherLists(pNode);

	return pNode;
}

void Res::unlinkNode(ResNode *pNode) {
	int numRefs = 0;
	ResTag *pTag = getResTagStruct(pNode->tag);
	ResRef *pRef = pNode->pRefs;

	while (pRef) {
		numRefs++;

		if (!pRef->valid)
			warning("Res::unlinkNode(): reference of %s is not valid", pNode->pName);

		pRef->valid = 0;
		pRef = pRef->pNext;
	}

	if (pTag) {
		ResHandlerMsg msg;

		msg.op = 2;
		msg.pName = pNode->pName;
		msg.tag = pNode->tag;
		msg.data = pNode->handle;
		msg.result = 0;

		if (pTag->pHandler->handler(&msg) == 1) {
			if (_logFile)
				_logFile->writeString(Common::String::format("Resource <%-12s> freed (%d references)\n", pNode->pName, numRefs));
		}
	}
}

int Res::freeNode(ResNode *pNode) {
	if (pNode->pPrev)
		pNode->pPrev->pNext = pNode->pNext;

	if (pNode->pNext)
		pNode->pNext->pPrev = pNode->pPrev;

	if (_pNodeHead == pNode)
		_pNodeHead = pNode->pNext;

	if (_pNodeTail == pNode)
		_pNodeTail = pNode->pPrev;

	if (pNode->flags & 4) {
		delete[] pNode->pName;
		pNode->pName = nullptr;
	}

	delete pNode;

	return 1;
}

int Res::loadFileData(ResHandlerMsg *pMsg) {
	if (pMsg->op != 1) {
		if (pMsg->op == 2)
			delete[] static_cast<byte *>(pMsg->data);

		return 0;
	}

	if (pMsg->tag == kResTagResf)
		return 0;

	Common::SeekableReadStream *pFile = _pResHS->openFile(pMsg->pName);

	if (!pFile)
		return 4;

	uint size = (uint)pFile->size();
	byte *pData = new byte[size];

	pFile->read(pData, size);
	allocCompletion(pMsg->tag, pMsg->pName, pData);
	delete pFile;

	return 1;
}

int Res::getUserField(ResNode *pNode) {
	if (!pNode)
		return 0;

	if (_bDemo) {
		ResTag *pTag = getResTagStruct(pNode->tag);

		if (!pTag)
			return 0;

		ResHandlerMsg sizeMsg;

		sizeMsg.op = 3;
		sizeMsg.pName = pNode->pName;
		sizeMsg.tag = pNode->tag;
		sizeMsg.data = pNode->handle;
		sizeMsg.result = 0;

		pTag->pHandler->handler(&sizeMsg);

		return sizeMsg.result;
	}

	return pNode->size;
}

int Res::totalAllocated() {
	// TODO: the EXE returns the bytes allocated by the whole memory manager, here only the resources are counted
	int totalBytes = 0;

	for (ResNode *pNode = _pNodeHead; pNode; pNode = pNode->pNext)
		totalBytes += getUserField(pNode);

	return totalBytes;
}

void Res::checkAnotherLists(ResNode *pNode) {
	if (_pNodeHead == pNode)
		return;

	if (pNode->pPrev)
		pNode->pPrev->pNext = pNode->pNext;

	if (pNode->pNext)
		pNode->pNext->pPrev = pNode->pPrev;

	if (_pNodeTail == pNode)
		_pNodeTail = pNode->pPrev;

	pNode->pNext = _pNodeHead;
	_pNodeHead->pPrev = pNode;
	pNode->pPrev = nullptr;
	_pNodeHead = pNode;
}

uint Res::changeFileNameToUpper(const char *src, char *dst) {
	uint length = strlen(src);
	uint hash = (((length | ((length | (length << 8)) << 8)) << 8) | length);
	const char *pSrc = src + 1;
	char c = toupper((byte)*src);

	*dst = c;
	char *pDst = dst + 1;

	while (c) {
		int next = *pSrc++;
		hash = c ^ ((hash >> 31) | (2 * hash));
		c = toupper((byte)next);
		*pDst++ = c;
	}

	return hash;
}

int Res::findExternalFile(const char *name, char *pOutPath, uint size) {
	char upperName[260];

	if (strlen(name) >= sizeof(upperName)) {
		warning("Res::findExternalFile(): name too long");
		return 0;
	}

	if (!size) {
		warning("Res::findExternalFile(): empty output buffer");
		return 0;
	}

	changeFileNameToUpper(name, upperName);

	if (!SearchMan.hasFile(Common::Path(upperName)))
		return 0;

	memset(pOutPath, 0, size);
	strncpy(pOutPath, upperName, size - 1);

	return 1;
}

void Res::setReference(ResRef *pRef, void *handle) {
	ResNode *pNode = checkAllocatedRes(handle);

	if (!pRef) {
		warning("Res::setReference(): no reference given");
		return;
	}

	if (pRef->valid)
		warning("Res::setReference(): reference is already valid");

	if (pNode) {
		pRef->valid = 1;
		pRef->serial = (int)(intptr)handle;
		pRef->handle = handle;
		pRef->pPrev = nullptr;
		pRef->pNext = pNode->pRefs;

		if (pRef->pNext)
			pRef->pNext->pPrev = pRef;

		pNode->pRefs = pRef;
		checkAnotherLists(pNode);
	} else {
		pRef->valid = 0;
	}
}

void Res::removeReference(ResRef *pRef) {
	if (!pRef->valid)
		return;

	ResNode *pNode = checkAllocatedRes(pRef->handle);

	if (pNode) {
		if (pNode->pRefs == pRef)
			pNode->pRefs = pRef->pNext;

		if (pRef->pPrev)
			pRef->pPrev->pNext = pRef->pNext;

		if (pRef->pNext)
			pRef->pNext->pPrev = pRef->pPrev;

		pRef->valid = 0;
		pRef->pNext = nullptr;
		pRef->pPrev = nullptr;
	}
}

void Res::allocatedIterFirst(ResNode **ppNext, void **pHandle, int tag) {
	ResNode *pNode = _pNodeHead;

	while (pNode) {
		if (pNode->tag == tag) {
			*ppNext = pNode->pNext;
			*pHandle = pNode->handle;
			return;
		}

		pNode = pNode->pNext;
	}

	*pHandle = nullptr;
	*ppNext = nullptr;
}

void Res::allocatedIterNext(ResNode **ppNext, void **pHandle, int tag) {
	ResNode *pNode = *ppNext;

	while (pNode) {
		if (pNode->tag == tag) {
			*ppNext = pNode->pNext;
			*pHandle = pNode->handle;
			return;
		}

		pNode = pNode->pNext;
	}

	*pHandle = nullptr;
	*ppNext = nullptr;
}

void Res::touch(void *handle) {
	ResNode *pNode = checkAllocatedRes(handle);

	if (pNode)
		checkAnotherLists(pNode);
}

void Res::resolveToSerialNumber(ResRef *pRef) {
	ResNode *pNode = resourceHandleFromSerialNumber(pRef->serial);

	if (pNode) {
		pRef->valid = 0;
		pRef->pNext = nullptr;
		pRef->pPrev = nullptr;
		setReference(pRef, pNode->handle);
	}
}

int Res::sysSave(Common::WriteStream *pStream) {
	int numNodes = 0;

	for (ResNode *pNode = _pNodeHead; pNode; pNode = pNode->pNext) {
		if ((pNode->pRefs && !strstr(pNode->pName, "TEMP")) || (pNode->flags & 8))
			numNodes++;
	}

	pStream->writeSint32LE(numNodes);

	if (pStream->err())
		return 1;

	for (ResNode *pNode = _pNodeHead; pNode; pNode = pNode->pNext) {
		if ((pNode->pRefs && !strstr(pNode->pName, "TEMP")) || (pNode->flags & 8)) {
			int length = strlen(pNode->pName);

			pStream->writeSint32LE(length);

			if (pStream->err())
				return 1;

			pStream->write(pNode->pName, length);

			if (pStream->err())
				return 1;

			pStream->writeSint32LE(pNode->tag);

			if (pStream->err())
				return 1;

			// The EXE overwrote the serial with the handle here, which supposedly
			// worked only because its heap addresses are the same on every run
			// pNode->serial = (int)(intptr)pNode->handle;
			pStream->writeSint32LE(pNode->serial);

			if (pStream->err())
				return 1;
		}
	}

	return 0;
}

int Res::sysLoad(Common::ReadStream *pStream) {
	char fileName[256];

	int numNodes = pStream->readSint32LE();

	if (pStream->err() || pStream->eos())
		return 1;

	if (numNodes <= 0)
		return 0;

	for (int i = 0; i < numNodes; i++) {
		int length = pStream->readSint32LE();

		if (pStream->err() || pStream->eos())
			return 1;

		if (length > 255)
			return 1;

		if (length < 0) {
			warning("Res::sysLoad(): negative name length");
			return 1;
		}

		if (pStream->read(fileName, length) != (uint32)length)
			return 1;

		fileName[length] = 0;

		int tag = pStream->readSint32LE();

		if (pStream->err() || pStream->eos())
			return 1;

		int result;
		ResNode *pNode = checkForNeedLoadRes(tag, fileName, &result);

		if (!result)
			return 1;

		if (!pNode) {
			warning("Res::sysLoad(): %s was not loaded", fileName);
			return 1;
		}

		pNode->serial = pStream->readSint32LE();

		if (pStream->err() || pStream->eos())
			return 1;
	}

	return 0;
}

void Res::freeAll() {
	ResNode *pResult = _pNodeHead;
	ResNode *pNode = _pNodeHead;

	while (pNode) {
		ResNode *pNext = pNode->pNext;

		if (!(pNode->flags & 8)) {
			unlinkNode(pNode);
			freeNode(pNode);
			pResult = _pNodeHead;
		}

		pNode = pNext;
	}

	if (pResult) {
		do {
			unlinkNode(pResult);
			freeNode(_pNodeHead);
			pResult = _pNodeHead;
		} while (_pNodeHead);
	}
}

int Res::countDependents(void *handle) {
	int count = 0;
	ResNode *pNode = checkAllocatedRes(handle);

	if (pNode) {
		for (ResRef *pRef = pNode->pRefs; pRef; pRef = pRef->pNext)
			count++;
	}

	return count;
}

int Res::initLists(int tag, const char *name) {
	int count = 0;
	ResNode *pNode = checkList1(tag, name);

	if (pNode) {
		for (ResRef *pRef = pNode->pRefs; pRef; pRef = pRef->pNext)
			count++;
	}

	return count;
}

void Res::setMemoryLimitMB(int megabytes) {
	_memoryLimit = (int)((uint)megabytes << 20);
}

int Res::setLogFile(const char *path) {
	return log(path) == 1;
}

} // End of namespace Grim

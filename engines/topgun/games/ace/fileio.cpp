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
#include "common/endian.h"
#include "common/path.h"
#include "common/savefile.h"
#include "common/system.h"
#include "common/textconsole.h"
#include "common/util.h"

#include "topgun/games/ace/fileio.h"

namespace Topgun {

enum {
	kFunctionFileOpen,
	kFunctionFileClose,
	kFunctionFileGetC,
	kFunctionFilePutC,
	kFunctionFileGetS,
	kFunctionFilePutS,
	kFunctionFileGetL,
	kFunctionFilePutL,
	kFunctionFileRead,
	kFunctionFileWrite,
	kFunctionFileReadLine,
	kFunctionFileWriteLine,
	kFunctionFileSeek,
	kFunctionFileLength,
	kFunctionFileFindFirst,
	kFunctionFileFindNext,
	kFunctionFileFindClose,
	kFunctionFileSetDataPath,
	kFunctionFileSetDataPathRel,
	kFunctionFileDataOpen,
	kFunctionFileDirCreate,
	kFunctionFileDirCreateRel,
	kFunctionFileDirRemove,
	kFunctionFileDirRemoveRel,
	kFunctionFileGetTempDir,
	kFunctionCount
};

FileIoPlugin::FileIoPlugin(PluginHost *host) : _host(host), _findIndex(0), _findActive(false) {
}

bool FileIoPlugin::init() {
	initCtx();
	initDefaultPath();
	return true;
}

void FileIoPlugin::free() {
	freeCtx();
}

int FileIoPlugin::findFunction(const Common::String &name) const {
	if (name.equalsIgnoreCase("FILEOPEN"))
		return kFunctionFileOpen;
	if (name.equalsIgnoreCase("FILECLOSE"))
		return kFunctionFileClose;
	if (name.equalsIgnoreCase("FILEGETC"))
		return kFunctionFileGetC;
	if (name.equalsIgnoreCase("FILEPUTC"))
		return kFunctionFilePutC;
	if (name.equalsIgnoreCase("FILEGETS"))
		return kFunctionFileGetS;
	if (name.equalsIgnoreCase("FILEPUTS"))
		return kFunctionFilePutS;
	if (name.equalsIgnoreCase("FILEGETL"))
		return kFunctionFileGetL;
	if (name.equalsIgnoreCase("FILEPUTL"))
		return kFunctionFilePutL;
	if (name.equalsIgnoreCase("FILEREAD"))
		return kFunctionFileRead;
	if (name.equalsIgnoreCase("FILEWRITE"))
		return kFunctionFileWrite;
	if (name.equalsIgnoreCase("FILEREADLINE"))
		return kFunctionFileReadLine;
	if (name.equalsIgnoreCase("FILEWRITELINE"))
		return kFunctionFileWriteLine;
	if (name.equalsIgnoreCase("FILESEEK"))
		return kFunctionFileSeek;
	if (name.equalsIgnoreCase("FILELENGTH"))
		return kFunctionFileLength;
	if (name.equalsIgnoreCase("FILEFINDFIRST"))
		return kFunctionFileFindFirst;
	if (name.equalsIgnoreCase("FILEFINDNEXT"))
		return kFunctionFileFindNext;
	if (name.equalsIgnoreCase("FILEFINDCLOSE"))
		return kFunctionFileFindClose;
	if (name.equalsIgnoreCase("FILESETDATAPATH"))
		return kFunctionFileSetDataPath;
	if (name.equalsIgnoreCase("FILESETDATAPATHREL"))
		return kFunctionFileSetDataPathRel;
	if (name.equalsIgnoreCase("FILEDATAOPEN"))
		return kFunctionFileDataOpen;
	if (name.equalsIgnoreCase("FILEDIRCREATE"))
		return kFunctionFileDirCreate;
	if (name.equalsIgnoreCase("FILEDIRCREATEREL"))
		return kFunctionFileDirCreateRel;
	if (name.equalsIgnoreCase("FILEDIRREMOVE"))
		return kFunctionFileDirRemove;
	if (name.equalsIgnoreCase("FILEDIRREMOVEREL"))
		return kFunctionFileDirRemoveRel;
	if (name.equalsIgnoreCase("FILEGETTEMPDIR"))
		return kFunctionFileGetTempDir;

	return -1;
}

uint16 FileIoPlugin::callFunction(int function, const uint16 *arguments, uint16 count) {
	if (function < 0 || function >= kFunctionCount) {
		warning("FileIoPlugin::callFunction(): invalid function %d", function);
		return 0;
	}

	uint16 expected = 0;
	switch (function) {
	case kFunctionFileFindClose:
		expected = 0;
		break;
	case kFunctionFileClose:
	case kFunctionFileLength:
	case kFunctionFileFindNext:
	case kFunctionFileSetDataPath:
	case kFunctionFileSetDataPathRel:
	case kFunctionFileDirCreate:
	case kFunctionFileDirCreateRel:
	case kFunctionFileDirRemove:
	case kFunctionFileDirRemoveRel:
	case kFunctionFileGetTempDir:
		expected = 1;
		break;
	case kFunctionFileOpen:
	case kFunctionFileReadLine:
	case kFunctionFileWriteLine:
	case kFunctionFileDataOpen:
		expected = 2;
		break;
	case kFunctionFileGetC:
	case kFunctionFilePutC:
	case kFunctionFileGetS:
	case kFunctionFilePutS:
	case kFunctionFileGetL:
	case kFunctionFilePutL:
	case kFunctionFileRead:
	case kFunctionFileWrite:
	case kFunctionFileSeek:
	case kFunctionFileFindFirst:
		expected = 3;
		break;
	}

	if (count < expected)
		warning("FileIoPlugin::callFunction(): function %d expects %d arguments, got %d", function, expected, count);

	uint16 values[3];
	for (int i = 0; i < 3; i++)
		values[i] = (i < count) ? arguments[i] : 0;

	switch (function) {
	case kFunctionFileOpen:
		return fileOpen(values[0], values[1]);
	case kFunctionFileClose:
		return fileClose(values[0]);
	case kFunctionFileGetC:
		return fileGetC(values[0], values[1], values[2]);
	case kFunctionFilePutC:
		return filePutC(values[0], values[1], values[2]);
	case kFunctionFileGetS:
		return fileGetS(values[0], values[1], values[2]);
	case kFunctionFilePutS:
		return filePutS(values[0], values[1], values[2]);
	case kFunctionFileGetL:
		return fileGetL(values[0], values[1], values[2]);
	case kFunctionFilePutL:
		return filePutL(values[0], values[1], values[2]);
	case kFunctionFileRead:
		return fileRead(values[0], values[1], values[2]);
	case kFunctionFileWrite:
		return fileWrite(values[0], values[1], values[2]);
	case kFunctionFileReadLine:
		return fileReadLine(values[0], values[1]);
	case kFunctionFileWriteLine:
		return fileWriteLine(values[0], values[1]);
	case kFunctionFileSeek:
		return fileSeek(values[0], values[1], values[2]);
	case kFunctionFileLength:
		return fileLength(values[0]);
	case kFunctionFileFindFirst:
		return fileFindFirst(values[0], values[1], values[2]);
	case kFunctionFileFindNext:
		return fileFindNext(values[0]);
	case kFunctionFileFindClose:
		return fileFindClose();
	case kFunctionFileSetDataPath:
		return fileSetDataPath(values[0]);
	case kFunctionFileSetDataPathRel:
		return fileSetDataPathRel(values[0]);
	case kFunctionFileDataOpen:
		return fileDataOpen(values[0], values[1]);
	case kFunctionFileDirCreate:
		return fileDirCreate(values[0]);
	case kFunctionFileDirCreateRel:
		return fileDirCreateRel(values[0]);
	case kFunctionFileDirRemove:
		return fileDirRemove(values[0]);
	case kFunctionFileDirRemoveRel:
		return fileDirRemoveRel(values[0]);
	case kFunctionFileGetTempDir:
		return fileGetTempDir(values[0]);
	}

	return 0;
}

uint16 FileIoPlugin::readItemsToVars(uint16 handle, uint16 address, uint16 count, uint32 itemSize) {
	byte buffer[4];
	uint16 done = 0;

	while (done < count) {
		if ((uint32)_host->mmioRead(handle, buffer, itemSize) != itemSize)
			break;

		if (itemSize == 1) {
			_host->writeAddress(address, (_host->readAddress(address) & 0xFF00) | buffer[0]);
		} else if (itemSize == 2) {
			_host->writeAddress(address, READ_LE_UINT16(buffer));
		} else {
			_host->writeAddress(address, READ_LE_UINT16(buffer));
			_host->writeAddress(address + 2, 0);
		}

		address += 2;
		done++;
	}

	return done;
}

uint16 FileIoPlugin::writeItemsFromVars(uint16 handle, uint16 address, uint16 count, uint32 itemSize) {
	byte buffer[4];
	uint16 done = 0;

	while (done < count) {
		uint16 value = _host->readAddress(address);

		if (itemSize == 1) {
			buffer[0] = (byte)value;
		} else if (itemSize == 2) {
			WRITE_LE_UINT16(buffer, value);
		} else {
			WRITE_LE_UINT16(buffer, value);
			WRITE_LE_UINT16(buffer + 2, 0);
		}

		if ((uint32)_host->mmioWrite(handle, buffer, itemSize) != itemSize)
			break;

		address += 2;
		done++;
	}

	return done;
}

uint16 FileIoPlugin::fileClose(uint16 handle) {
	return _host->mmioClose(handle);
}

uint16 FileIoPlugin::fileGetC(uint16 handle, uint16 address, uint16 count) {
	return readItemsToVars(handle, address, count, 1);
}

uint16 FileIoPlugin::fileGetL(uint16 handle, uint16 address, uint16 count) {
	return readItemsToVars(handle, address, count, 4);
}

uint16 FileIoPlugin::fileGetS(uint16 handle, uint16 address, uint16 count) {
	return readItemsToVars(handle, address, count, 2);
}

uint16 FileIoPlugin::fileLength(uint16 handle) {
	int32 position = _host->mmioSeek(handle, 0, SEEK_CUR);
	int32 length = _host->mmioSeek(handle, 0, SEEK_END);
	_host->mmioSeek(handle, position, SEEK_SET);

	return (uint16)length;
}

uint16 FileIoPlugin::fileOpen(uint16 nameRef, uint16 flags) {
	char *name = _host->getString(nameRef);
	if (!name)
		return 0;

	if ((flags & kFileIoOpenDelete) != 0)
		return (_host->mmioOpen(name, kMmioDelete) != 0) ? 1 : 0;

	uint32 mode = kMmioRead;
	if ((flags & kFileIoOpenRead) != 0 && (flags & kFileIoOpenWrite) != 0)
		mode = kMmioReadWrite;
	else if ((flags & kFileIoOpenWrite) != 0)
		mode = kMmioWrite;

	if ((flags & kFileIoOpenCreateNew) != 0) {
		if (_host->mmioOpen(name, kMmioExist) != 0)
			return 0;

		mode |= kMmioCreate;
	}

	if ((flags & kFileIoOpenCreate) != 0 || (flags & kFileIoOpenCreateAlternate) != 0)
		mode |= kMmioCreate;

	if ((flags & kFileIoOpenMustExist) != 0) {
		if (_host->mmioOpen(name, kMmioExist) == 0)
			return 0;
	}

	if ((flags & (kFileIoOpenUnknown80 | kFileIoOpenMustExist)) != 0 && (flags & (kFileIoOpenRead | kFileIoOpenWrite)) == 0)
		return 0;

	return _host->mmioOpen(name, mode | kMmioDenyNone);
}

uint16 FileIoPlugin::filePutC(uint16 handle, uint16 address, uint16 count) {
	return writeItemsFromVars(handle, address, count, 1);
}

uint16 FileIoPlugin::filePutL(uint16 handle, uint16 address, uint16 count) {
	return writeItemsFromVars(handle, address, count, 4);
}

uint16 FileIoPlugin::filePutS(uint16 handle, uint16 address, uint16 count) {
	return writeItemsFromVars(handle, address, count, 2);
}

uint16 FileIoPlugin::fileRead(uint16 handle, uint16 stringRef, uint16 count) {
	uint16 size = MIN<uint16>(count, kFileIoStringTransferLimit);

	char *string = _host->getVariableString(stringRef);
	if (!string)
		return 0;

	return (uint16)_host->mmioRead(handle, (byte *)string, size);
}

uint16 FileIoPlugin::fileWrite(uint16 handle, uint16 stringRef, uint16 count) {
	uint16 size = MIN<uint16>(count, kFileIoStringTransferLimit);

	char *string = _host->getVariableString(stringRef);
	if (!string)
		return 0;

	return (uint16)_host->mmioWrite(handle, (const byte *)string, size);
}

uint16 FileIoPlugin::fileReadLine(uint16 handle, uint16 stringRef) {
	char *line = _host->getVariableString(stringRef);
	if (!line)
		return 0;

	int position = 0;
	uint16 length = 0;

	line[0] = '\0';

	// The byte before the first string does not exist
	bool previousIsReturn = (line != _host->getVariableString(1)) && line[-1] == '\r';

	if (!previousIsReturn) {
		while (length < kFileIoStringTransferLimit) {
			int32 result = _host->mmioRead(handle, (byte *)line + position, 1);
			position++;

			if (result == 0)
				break;

			length++;

			if (line[position - 1] == '\r')
				break;
		}
	}

	if (line[position - 1] == '\r') {
		line[position - 1] = '\0';

		byte skipped;
		_host->mmioRead(handle, &skipped, 1);
	}

	return length;
}

uint16 FileIoPlugin::fileWriteLine(uint16 handle, uint16 stringRef) {
	char *line = _host->getVariableString(stringRef);
	if (!line)
		return 0;

	if (_host->mmioWrite(handle, (const byte *)line, strlen(line)) == 0)
		return 0;

	return (uint16)_host->mmioWrite(handle, (const byte *)"\r\n", 2);
}

uint16 FileIoPlugin::fileSeek(uint16 handle, uint16 offset, uint16 origin) {
	int seekOrigin = SEEK_SET;

	// Origins 0, 1 and 2 all seek from the end of the file
	if (origin == 0 || origin == 1 || origin == 2)
		seekOrigin = SEEK_END;

	return (uint16)_host->mmioSeek(handle, offset, seekOrigin);
}

uint16 FileIoPlugin::fileFindFirst(uint16 patternRef, uint16 resultRef, uint16 usePath) {
	_findActive = true;
	_findNames.clear();
	_findIndex = 0;

	char *pattern = _host->getString(patternRef);
	if (!pattern) {
		warning("FileIoPlugin::fileFindFirst(): invalid pattern %d", patternRef);
		return 0;
	}

	Common::String fullPattern;
	if (usePath != 0)
		fullPattern = _defaultPath;

	fullPattern += pattern;

	Common::String filePattern = Common::Path(fullPattern, '\\').getLastComponent().toString();

	Common::ArchiveMemberList members;
	SearchMan.listMatchingMembers(members, Common::Path(filePattern));

	for (Common::ArchiveMemberList::const_iterator it = members.begin(); it != members.end(); ++it)
		_findNames.push_back((*it)->getFileName());

	Common::StringArray saves = g_system->getSavefileManager()->listSavefiles(filePattern);
	for (uint32 i = 0; i < saves.size(); i++) {
		bool known = false;

		for (uint32 j = 0; j < _findNames.size(); j++) {
			if (_findNames[j].equalsIgnoreCase(saves[i]))
				known = true;
		}

		if (!known)
			_findNames.push_back(saves[i]);
	}

	if (_findNames.empty())
		return 0;

	char *result = _host->getVariableString(resultRef);
	if (!result)
		warning("FileIoPlugin::fileFindFirst(): invalid result string %d", resultRef);
	else
		Common::strlcpy(result, _findNames[0].c_str(), kFileIoStringSlotSize);

	return _findNames.size();
}

uint16 FileIoPlugin::fileFindNext(uint16 resultRef) {
	if (!_findActive)
		return 0;

	if (_findIndex + 1 >= _findNames.size()) {
		_findIndex = _findNames.size();
		return 0;
	}

	_findIndex++;

	char *result = _host->getVariableString(resultRef);
	if (!result)
		warning("FileIoPlugin::fileFindNext(): invalid result string %d", resultRef);
	else
		Common::strlcpy(result, _findNames[_findIndex].c_str(), kFileIoStringSlotSize);

	return 1;
}

uint16 FileIoPlugin::fileFindClose() {
	_findActive = false;
	_findNames.clear();
	_findIndex = 0;

	return 0;
}

void FileIoPlugin::initDefaultPath() {
	char buffer[kFileIoPathSize] = "";

	_host->getModulePath(buffer, kFileIoPathSize);
	_defaultPath = buffer;

	if (!_defaultPath.empty() && _defaultPath.lastChar() != '\\')
		_defaultPath += '\\';
}

uint16 FileIoPlugin::fileSetDataPath(uint16 pathRef) {
	char *path = _host->getString(pathRef);
	if (!path)
		return 0;

	if (path[0] == '\0')
		_dataPath = _defaultPath;
	else
		_dataPath = path;

	if (!_dataPath.empty() && _dataPath.lastChar() != '\\')
		_dataPath += '\\';

	return 1;
}

uint16 FileIoPlugin::fileSetDataPathRel(uint16 relativeRef) {
	char *relative = _host->getString(relativeRef);
	if (!relative)
		return 0;

	_dataPath = _defaultPath + relative;

	if (!_dataPath.empty() && _dataPath.lastChar() != '\\')
		_dataPath += '\\';

	return 1;
}

uint16 FileIoPlugin::fileDataOpen(uint16 nameRef, uint16 flags) {
	char *name = _host->getString(nameRef);
	if (!name)
		return 0;

	Common::String path;
	if (!_dataPath.empty())
		path = _dataPath;
	else
		path = _defaultPath;

	path += name;

	if ((nameRef & kFileIoConstantStringFlag) == 0) {
		if (path.size() >= kFileIoStringSlotSize)
			warning("FileIoPlugin::fileDataOpen(): the path '%s' does not fit in a string", path.c_str());

		Common::strlcpy(name, path.c_str(), kFileIoStringSlotSize);
	} else {
		uint32 length = path.size() + 1;

		if (length > _host->getConstantStringRoom(nameRef)) {
			warning("FileIoPlugin::fileDataOpen(): the path '%s' does not fit in the constant strings", path.c_str());
			return 0;
		}

		// The original writes over the constant strings that follow a longer path
		if (length > strlen(name) + 1)
			warning("FileIoPlugin::fileDataOpen(): the path '%s' overwrites the following constant strings", path.c_str());

		memcpy(name, path.c_str(), length);
	}

	return fileOpen(nameRef, flags);
}

uint16 FileIoPlugin::fileDirCreate(uint16 pathRef) {
	char *path = _host->getString(pathRef);
	if (!path)
		return 0;

	warning("FileIoPlugin::fileDirCreate(): cannot create the directory '%s'", path);
	return 0;
}

uint16 FileIoPlugin::fileDirCreateRel(uint16 relativeRef) {
	char *relative = _host->getString(relativeRef);
	if (!relative)
		return 0;

	warning("FileIoPlugin::fileDirCreateRel(): cannot create the directory '%s%s'", _defaultPath.c_str(), relative);
	return 0;
}

uint16 FileIoPlugin::fileDirRemove(uint16 pathRef) {
	char *path = _host->getString(pathRef);
	if (!path)
		return 0;

	warning("FileIoPlugin::fileDirRemove(): cannot remove the directory '%s'", path);
	return 0;
}

uint16 FileIoPlugin::fileDirRemoveRel(uint16 relativeRef) {
	char *relative = _host->getString(relativeRef);
	if (!relative)
		return 0;

	warning("FileIoPlugin::fileDirRemoveRel(): cannot remove the directory '%s%s'", _defaultPath.c_str(), relative);
	return 0;
}

uint16 FileIoPlugin::fileGetTempDir(uint16 resultRef) {
	char *result = _host->getVariableString(resultRef);
	if (!result) {
		warning("FileIoPlugin::fileGetTempDir(): invalid result string %d", resultRef);
		return 0;
	}

	result[0] = '\0';

	warning("FileIoPlugin::fileGetTempDir(): there is no temporary directory");
	return 0;
}

uint16 FileIoPlugin::freeCtx() {
	_findActive = false;
	_findNames.clear();
	_findIndex = 0;

	return 0;
}

uint16 FileIoPlugin::initCtx() {
	_findActive = false;
	_findNames.clear();
	_findIndex = 0;

	return 1;
}

} // End of namespace Topgun

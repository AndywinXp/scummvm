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

#include "engines/grim/emi/fileutil.h"
#include "engines/grim/emi/lmemoryservices.h"

namespace Grim {

void *FileUtilFind::operator new(size_t size) {
	return LMemoryServices::getCurrent()->mallocImpl(size);
}

void FileUtilFind::operator delete(void *block) {
	LMemoryServices::getCurrent()->freeImpl(block);
}

FileUtilFind *FileUtilFind::newFind(const char *directory, int mode, const char *name) {
	FileUtilFind *created = new FileUtilFind();
	created->callCount = 0;
	created->find = nullptr;

	switch (mode) {
	case 0:
	case 1:
	case 2:
		created->pattern = LString::format("%s*.*", directory);
		break;
	case 3:
		if (*name == '.')
			name++;

		created->pattern = LString::format("%s*.%s", directory, name);
		break;
	case 4:
		created->pattern = LString::format("%s%s", directory, name);
		break;
	default:
		break;
	}

	return created;
}

void FileUtilFind::disposeFind(FileUtilFind *&target) {
	if (!target)
		return;

	if (target->find) {
		target->find->close();
		delete target->find;
		target->find = nullptr;
	}

	delete target;
	target = nullptr;
}

bool FileUtilFind::findNext(FileUtilEntry *entry) {
	int count = callCount;
	callCount = count + 1;

	if (count == 0) {
		int separator = -1;

		for (uint i = 0; i < pattern.size(); i++) {
			if (pattern[i] == '/' || pattern[i] == '\\')
				separator = i;
		}

		LString directory;
		LString filePattern = pattern;

		if (separator >= 0) {
			directory = pattern.substr(0, separator + 1);
			filePattern = pattern.substr(separator + 1);
		}

		find = new FileFind(filePattern.c_str(), directory.c_str());
	}

	char name[260];
	uint nameSize = sizeof(name);
	uint attributes = 0;

	if (!find->next(name, &nameSize, &attributes))
		return false;

	entry->isDirectory = (attributes & kFileFindDirectory) != 0;
	entry->path.clear();

	int slash = -1;

	for (uint i = 0; i < pattern.size(); i++) {
		if (pattern[i] == '/')
			slash = i;
	}

	if (slash >= 0 && !entry->isDirectory)
		entry->path = pattern.substr(0, slash + 1);

	entry->path += name;
	entry->writeTime = 0;

	return true;
}

} // End of namespace Grim

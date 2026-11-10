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

namespace Grim {

FileUtilFind *fileUtilNewFind(const char *directory, int mode, const char *name) {
	FileUtilFind *find = new FileUtilFind();
	find->callCount = 0;
	find->find = nullptr;

	switch (mode) {
	case 0:
	case 1:
	case 2:
		find->pattern = LString::format("%s*.*", directory);
		break;
	case 3:
		if (*name == '.')
			name++;

		find->pattern = LString::format("%s*.%s", directory, name);
		break;
	case 4:
		find->pattern = LString::format("%s%s", directory, name);
		break;
	default:
		break;
	}

	return find;
}

void fileUtilDisposeFind(FileUtilFind *&find) {
	if (!find)
		return;

	if (find->find) {
		find->find->close();
		delete find->find;
		find->find = nullptr;
	}

	delete find;
	find = nullptr;
}

bool fileUtilFindNext(FileUtilFind *find, FileUtilEntry *entry) {
	if (!find)
		return false;

	int callCount = find->callCount;
	find->callCount = callCount + 1;

	if (callCount == 0) {
		int separator = -1;

		for (uint i = 0; i < find->pattern.size(); i++) {
			if (find->pattern[i] == '/' || find->pattern[i] == '\\')
				separator = i;
		}

		LString directory;
		LString filePattern = find->pattern;

		if (separator >= 0) {
			directory = find->pattern.substr(0, separator + 1);
			filePattern = find->pattern.substr(separator + 1);
		}

		find->find = new FileFind(filePattern.c_str(), directory.c_str());
	}

	char name[260];
	uint nameSize = sizeof(name);
	uint attributes = 0;

	if (!find->find->next(name, &nameSize, &attributes))
		return false;

	entry->isDirectory = (attributes & kFileFindDirectory) != 0;
	entry->path.clear();

	int slash = -1;

	for (uint i = 0; i < find->pattern.size(); i++) {
		if (find->pattern[i] == '/')
			slash = i;
	}

	if (slash >= 0 && !entry->isDirectory)
		entry->path = find->pattern.substr(0, slash + 1);

	entry->path += name;
	entry->writeTime = 0;

	return true;
}

} // End of namespace Grim

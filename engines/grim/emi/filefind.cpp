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

#include "common/config-manager.h"
#include "common/path.h"
#include "common/str.h"

#include "engines/grim/emi/filefind.h"
#include "engines/grim/emi/lmemoryservices.h"

namespace Grim {

void *FileFind::operator new(size_t size) {
	return gpMemoryServices->malloc(size);
}

void FileFind::operator delete(void *block) {
	gpMemoryServices->free(block);
}

FileFind::FileFind(const char *pattern, const char *directory) {
	_current = 0;

	Common::FSNode node(ConfMan.getPath("path"));

	if (directory && *directory != '~') {
		if (strchr(directory, ':')) {
			node = Common::FSNode(Common::Path(directory, '\\'));
		} else {
			Common::String component;

			for (const char *p = directory; ; p++) {
				if (*p == '\\' || *p == '/' || *p == 0) {
					if (!component.empty() && component != ".")
						node = node.getChild(component);

					component.clear();

					if (*p == 0)
						break;
				} else {
					component += *p;
				}
			}
		}
	}

	if (!node.isDirectory())
		return;

	Common::FSList children;
	node.getChildren(children, Common::FSNode::kListAll);

	bool matchAll = strcmp(pattern, "*.*") == 0 || strcmp(pattern, "*") == 0;

	for (uint i = 0; i < children.size(); i++) {
		if (matchAll || Common::matchString(children[i].getName().c_str(), pattern, true))
			_entries.push_back(children[i]);
	}
}

void FileFind::close() {
	_entries.clear();
	_current = 0;
}

bool FileFind::next(char *name, uint *bufferSize, uint *attributes) {
	if (_current >= _entries.size()) {
		*bufferSize = 0;

		return false;
	}

	Common::String entryName = _entries[_current].getName();

	if (entryName.size() + 1 > *bufferSize) {
		*bufferSize = entryName.size() + 1;

		return false;
	}

	Common::strcpy_s(name, *bufferSize, entryName.c_str());
	*attributes = _entries[_current].isDirectory() ? kFileFindDirectory : kFileFindNormal;
	_current++;

	return true;
}

} // End of namespace Grim

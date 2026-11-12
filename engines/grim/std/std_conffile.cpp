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

#include "common/str.h"
#include "common/stream.h"
#include "common/textconsole.h"

#include "engines/grim/std/std_conffile.h"

namespace Grim {

StdConffile::StdConffile(SysStruct *hs) : _hs(hs), _linenum(0), _open(0), _openFile(nullptr), _stackLevel(0), _line(nullptr) {
	memset(_filename, 0, sizeof(_filename));
	memset(_entry, 0, sizeof(_entry));
	memset(_filenameStack, 0, sizeof(_filenameStack));
	memset(_bufferStack, 0, sizeof(_bufferStack));
	memset(_linenumStack, 0, sizeof(_linenumStack));
	memset(_lineStack, 0, sizeof(_lineStack));
	memset(_openFileStack, 0, sizeof(_openFileStack));
}

StdConffile::~StdConffile() {
}

int StdConffile::open(const char *path) {
	return fOpen(path);
}

int StdConffile::fOpen(const char *path) {
	if (_open)
		pushState();

	if (!strcmp(path, "none")) {
		_openFile = nullptr;
	} else {
		_openFile = _hs->openFile(path);
		if (!_openFile) {
			if (_open)
				popState();
			return 0;
		}
	}

	_line = new char[1024];
	Common::strlcpy(_filename, path, 128);
	_linenum = 0;
	_open = 1;

	return 1;
}

void StdConffile::close() {
	if (_open) {
		if (_openFile) {
			delete _openFile;
			_openFile = nullptr;
		}

		delete[] _line;
		_line = nullptr;

		if (_stackLevel)
			popState();
		else
			_open = 0;
	}
}

int StdConffile::readLine() {
	char *dst = _line;
	int gotLine = 0;
	uint bufLeft = 1023;

	while (bufLeft) {
		if (!getsFile(_openFile, dst, bufLeft))
			return 0;

		_linenum++;

		char c = *dst;
		if (c != ';' && c != '#' && c != '\n' && c != '\r') {
			char *comment = strchr(dst, '#');
			if (comment)
				*comment = 0;

			for (char *p = dst; *p; p++) {
				if (*p >= 'A' && *p <= 'Z')
					*p += 0x20;
			}

			int len = strlen(_line);
			bool continued = false;

			if (len >= 2)
				continued = _line[len - 2] == '\\';
			else
				warning("StdConffile::readLine(): line too short to check for a continuation");

			if (continued) {
				dst = &_line[len - 2];
				bufLeft = 1024 - len;
			} else {
				char last = _line[len - 1];

				gotLine = 1;
				if (last == '\r' || last == '\n')
					_line[len - 1] = 0;
			}
		}

		if (gotLine)
			return 1;
	}

	return 1;
}

void StdConffile::pushState() {
	int level = _stackLevel;

	Common::strlcpy(_filenameStack[level], _filename, 128);
	_linenumStack[level] = _linenum;
	_linenum = 0;
	_openFileStack[level] = _openFile;
	_openFile = nullptr;
	memcpy(_lineStack[level], _entry, sizeof(_entry));
	_bufferStack[level] = _line;
	_stackLevel = level + 1;
}

void StdConffile::popState() {
	if (!_stackLevel)
		return;

	int level = --_stackLevel;

	Common::strlcpy(_filename, _filenameStack[level], 128);
	_openFile = _openFileStack[level];
	_linenum = _linenumStack[level];
	_line = _bufferStack[level];
	memcpy(_entry, _lineStack[level], sizeof(_entry));
}

char *StdConffile::getsFile(Common::SeekableReadStream *file, char *buffer, int size) {
	int count = 0;

	while (count < size - 1 && file->pos() < file->size()) {
		char c = file->readByte();

		buffer[count++] = c;
		if (c == '\n')
			break;
	}

	if (!count)
		return nullptr;

	buffer[count] = 0;
	return buffer;
}

} // End of namespace Grim

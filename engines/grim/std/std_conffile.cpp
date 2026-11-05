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

StdConffile::StdConffile(SysStruct *pHS) : _pHS(pHS), _linenum(0), _bOpen(0), _openFile(nullptr), _stackLevel(0), _aLine(nullptr) {
	memset(_pFilename, 0, sizeof(_pFilename));
	memset(_entry, 0, sizeof(_entry));
	memset(_aFilenameStack, 0, sizeof(_aFilenameStack));
	memset(_apBufferStack, 0, sizeof(_apBufferStack));
	memset(_linenumStack, 0, sizeof(_linenumStack));
	memset(_aLineStack, 0, sizeof(_aLineStack));
	memset(_openFileStack, 0, sizeof(_openFileStack));
}

StdConffile::~StdConffile() {
}

int StdConffile::open(const char *path) {
	return fOpen(path);
}

int StdConffile::fOpen(const char *path) {
	if (_bOpen)
		pushState();

	if (!strcmp(path, "none")) {
		_openFile = nullptr;
	} else {
		_openFile = _pHS->openFile(path);
		if (!_openFile) {
			if (_bOpen)
				popState();
			return 0;
		}
	}

	_aLine = new char[1024];
	Common::strlcpy(_pFilename, path, 128);
	_linenum = 0;
	_bOpen = 1;

	return 1;
}

void StdConffile::close() {
	if (_bOpen) {
		if (_openFile) {
			delete _openFile;
			_openFile = nullptr;
		}

		delete[] _aLine;
		_aLine = nullptr;

		if (_stackLevel)
			popState();
		else
			_bOpen = 0;
	}
}

int StdConffile::readLine() {
	char *pDst = _aLine;
	int gotLine = 0;
	uint bufLeft = 1023;

	while (bufLeft) {
		if (!getsFile(_openFile, pDst, bufLeft))
			return 0;

		_linenum++;

		char c = *pDst;
		if (c != ';' && c != '#' && c != '\n' && c != '\r') {
			char *pComment = strchr(pDst, '#');
			if (pComment)
				*pComment = 0;

			for (char *p = pDst; *p; p++) {
				if (*p >= 'A' && *p <= 'Z')
					*p += 0x20;
			}

			int len = strlen(_aLine);
			bool continued = false;

			if (len >= 2)
				continued = _aLine[len - 2] == '\\';
			else
				warning("StdConffile::readLine(): line too short to check for a continuation");

			if (continued) {
				pDst = &_aLine[len - 2];
				bufLeft = 1024 - len;
			} else {
				char last = _aLine[len - 1];

				gotLine = 1;
				if (last == '\r' || last == '\n')
					_aLine[len - 1] = 0;
			}
		}

		if (gotLine)
			return 1;
	}

	return 1;
}

void StdConffile::pushState() {
	int level = _stackLevel;

	Common::strlcpy(_aFilenameStack[level], _pFilename, 128);
	_linenumStack[level] = _linenum;
	_linenum = 0;
	_openFileStack[level] = _openFile;
	_openFile = nullptr;
	memcpy(_aLineStack[level], _entry, sizeof(_entry));
	_apBufferStack[level] = _aLine;
	_stackLevel = level + 1;
}

void StdConffile::popState() {
	if (!_stackLevel)
		return;

	int level = --_stackLevel;

	Common::strlcpy(_pFilename, _aFilenameStack[level], 128);
	_openFile = _openFileStack[level];
	_linenum = _linenumStack[level];
	_aLine = _apBufferStack[level];
	memcpy(_entry, _aLineStack[level], sizeof(_entry));
}

char *StdConffile::getsFile(Common::SeekableReadStream *pFile, char *pBuffer, int size) {
	int count = 0;

	while (count < size - 1 && pFile->pos() < pFile->size()) {
		char c = pFile->readByte();

		pBuffer[count++] = c;
		if (c == '\n')
			break;
	}

	if (!count)
		return nullptr;

	pBuffer[count] = 0;
	return pBuffer;
}

} // End of namespace Grim

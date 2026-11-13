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

#include "common/endian.h"
#include "common/str.h"
#include "common/textconsole.h"

#include "engines/grim/emi/emiengine.h"
#include "engines/grim/emi/core/lmemoryservices.h"
#include "engines/grim/emi/text/cjkstring.h"
#include "engines/grim/emi/text/polyglot_v2.h"
#include "engines/grim/std/std_sort.h"
#include "engines/grim/wuregistry.h"

namespace Grim {

Polyglot_v2::Polyglot_v2(EmiEngine *engine) : _engine(engine), _table(nullptr), _lines(nullptr), _lineCount(0), _randomSeed(0),
		_cjkTable(nullptr), _cjkLines(nullptr), _cjkModeBuffer(), _defaultBuffer() {
}

Polyglot_v2::~Polyglot_v2() {
	shutdown();
}

int Polyglot_v2::nextKeyByte() {
	_randomSeed = _randomSeed * 214013 + 2531011;

	int value = (int)((_randomSeed >> 16) & 0x7FFF);

	return (int)((double)value / 32767.0 * 254.0 + 1.0);
}

void Polyglot_v2::shutdown() {
	if (_table) {
		_engine->getMemoryServices()->freeImpl(_table);
		_table = nullptr;
	}

	if (_lines) {
		_engine->getMemoryServices()->freeImpl(_lines);
		_lines = nullptr;
	}

	if (_cjkTable) {
		_engine->getMemoryServices()->freeImpl(_cjkTable);
		_cjkTable = nullptr;
	}

	if (_cjkLines) {
		_engine->getMemoryServices()->freeImpl(_cjkLines);
		_cjkLines = nullptr;
	}

	_cjkModeBuffer.clear();
	_lineCount = 0;
}

LFile *Polyglot_v2::openFileNoBundle(const char *path, const char *mode) {
	LFileServices *services = nullptr;

	if (_engine->getNumFileServices() > 1)
		services = _engine->popFileServices();

	LFile *file = _engine->getFileServices()->open(path, mode);

	if (services)
		_engine->pushFileServices(services);

	return file;
}

void Polyglot_v2::configure(const char *tabFile) {
	if (_engine->isCjkMode()) {
		configureCjk(tabFile);
		return;
	}

	LFile *file = openFileNoBundle(tabFile, "rb");

	if (!file) {
		file = _engine->getFileServices()->open(tabFile, "rb");

		if (!file)
			return;
	}

	byte header[4] = { 0, 0, 0, 0 };
	file->read(header, 4);

	file->seek(0, SEEK_END);
	int size = file->tell();
	file->seek(0, SEEK_SET);

	// Extra byte for the terminator
	_table = (char *)_engine->getMemoryServices()->mallocImpl((size + 1) * sizeof(char));
	file->read(_table, size);
	_table[size] = 0;

	_engine->getFileServices()->close(file, true);

	if (READ_LE_UINT32(header) == MKTAG('E', 'N', 'C', 'R')) {
		_randomSeed = 22;
		size -= 4;

		for (int i = 0; i < size; i++)
			_table[i] = (char)(_table[i + 4] ^ nextKeyByte());
	}

	int position = 0;

	do {
		position = (int)(findChar(_table + position, 13) - _table) + 2;
		_lineCount++;
	} while (position < size - 1);

	_lines = (char **)_engine->getMemoryServices()->mallocImpl(_lineCount * sizeof(char *));

	position = 0;
	int lineIndex = 0;

	do {
		char *line = _table + position;
		char *lineEnd = findChar(line, 13);

		*lineEnd = 0;

		char *tab = strchr(line, 9);

		if (tab)
			*tab = 0;

		for (char *p = line; *p; p++) {
			if (*p >= 'a' && *p <= 'z')
				*p -= 0x20;
		}

		_lines[lineIndex] = line;
		lineIndex++;
		position = (int)(lineEnd - _table) + 2;
	} while (position < size - 1);

	sort(_lines, _lineCount, sizeof(char *), WuRegistry::compareNames);
}

const char *Polyglot_v2::getTranslation(const char *msg) {
	char key[16];

	if (!msg)
		return nullptr;

	if (*msg != '/')
		return msg;

	const char *closingSlash = strchr(msg + 1, '/');

	if (!closingSlash)
		return msg;

	const char *text = closingSlash + 1;

	if (!_table)
		return text;

	Common::strlcpy(key, msg + 1, sizeof(key));

	char *slash = strchr(key, '/');

	if (slash)
		*slash = 0;

	for (char *p = key; *p; p++) {
		if (*p >= 'a' && *p <= 'z')
			*p -= 0x20;
	}

	const char *keyString = key;
	char **found = (char **)binarySearch(&keyString, _lines, _lineCount, sizeof(char *), WuRegistry::compareNames);

	if (!found)
		return text;

	const char *translation = *found + strlen(*found) + 1;

	if (!*translation)
		return msg;

	return translation;
}

int Polyglot_v2::compareCjkLines(const void *a, const void *b) {
	return CjkString::compare(*(const uint16 *const *)a, *(const uint16 *const *)b);
}

void Polyglot_v2::configureCjk(const char *tabFile) {
	LFile *file = openFileNoBundle(tabFile, "rb");

	if (!file) {
		file = _engine->getFileServices()->open(tabFile, "rb");

		if (!file)
			return;
	}

	byte header[4] = { 0, 0, 0, 0 };
	file->read(header, 4);

	file->seek(0, SEEK_END);
	int size = file->tell();
	file->seek(0, SEEK_SET);

	if (size < 0)
		size = 0;

	shutdown();

	// Two extra units: the terminator, and the unit after it that a line without a tab can reach
	_cjkTable = (uint16 *)_engine->getMemoryServices()->mallocImpl((size / 2 + 2) * sizeof(uint16));

	if (!_cjkTable) {
		_engine->getFileServices()->close(file, true);
		return;
	}

	int bytesRead = (int)file->read(_cjkTable, size);

	_engine->getFileServices()->close(file, true);

	if (bytesRead != size) {
		warning("Polyglot_v2::configureCjk(): could only read %d of %d bytes of %s", bytesRead, size, tabFile);

		size = bytesRead;
	}

	uint units = size / 2;
	const byte *bytes = (const byte *)_cjkTable;

	for (uint i = 0; i < units; i++)
		_cjkTable[i] = READ_LE_UINT16(bytes + 2 * i);

	_cjkTable[units] = 0;
	_cjkTable[units + 1] = 0;

	if (READ_LE_UINT32(header) == MKTAG('E', 'N', 'C', 'R'))
		warning("Polyglot_v2::configureCjk(): %s is encrypted, which is not supported here", tabFile);

	if (CjkString::find(_cjkTable, "Korea"))
		_engine->_glyphAdvance = 14;

	uint position = 0;
	uint lineEnd;
	int count = 0;

	do {
		lineEnd = position;

		while (_cjkTable[lineEnd] != 0 && _cjkTable[lineEnd] != 13)
			lineEnd++;

		position = lineEnd + 2;
		count++;
	} while (lineEnd + 2 < units);

	_cjkLines = (uint16 **)_engine->getMemoryServices()->mallocImpl(count * sizeof(uint16 *));

	if (!_cjkLines) {
		shutdown();
		return;
	}

	position = 0;
	int lineIndex = 0;

	do {
		uint16 *line = _cjkTable + position;

		lineEnd = position;

		while (_cjkTable[lineEnd] != 0 && _cjkTable[lineEnd] != 13)
			lineEnd++;

		_cjkTable[lineEnd] = 0;

		uint16 *tab = (uint16 *)CjkString::findChar(line, 9);

		if (tab)
			*tab = 0;

		CjkString::lowerAscii(line);

		_cjkLines[lineIndex] = line;
		lineIndex++;
		position = lineEnd + 2;
	} while (lineEnd + 2 < units);

	_lineCount = count;

	sort(_cjkLines, _lineCount, sizeof(uint16 *), compareCjkLines);
}

const uint16 *Polyglot_v2::toCjkUnits(const char *text) {
	uint written = CjkString::widen(text, _defaultBuffer, kDefaultBufferSize);

	// The disasm could overflow its buffer here, let's avoid that...
	if (text[written] != 0)
		warning("Polyglot_v2::toCjkUnits(): the text is too long, cutting it to %d units", (int)written);

	return _defaultBuffer;
}

const uint16 *Polyglot_v2::getCjkTranslation(const char *msg, int mode) {
	if (!msg)
		return nullptr;

	if (*msg != '/')
		return toCjkUnits(msg);

	const char *closingSlash = strchr(msg + 1, '/');

	if (!closingSlash)
		return toCjkUnits(msg);

	if (!_cjkTable)
		return toCjkUnits(closingSlash + 1);

	char narrowKey[16];

	Common::strlcpy(narrowKey, msg + 1, sizeof(narrowKey));

	char *slash = strchr(narrowKey, '/');

	if (slash)
		*slash = 0;

	for (char *p = narrowKey; *p; p++) {
		if (*p >= 'A' && *p <= 'Z')
			*p += 0x20;
	}

	uint16 key[16];
	CjkString::widen(narrowKey, key, 16);

	const uint16 *keyPointer = key;
	uint16 **found = (uint16 **)binarySearch(&keyPointer, _cjkLines, _lineCount, sizeof(uint16 *), compareCjkLines);

	if (!found)
		return toCjkUnits(closingSlash + 1);

	const uint16 *translation = *found + CjkString::length(*found) + 1;

	if (!*translation)
		return toCjkUnits(msg);

	if (mode == 1)
		return toCjkUnits(closingSlash + 1);

	if (mode == 2) {
		// The disasm appends to a small buffer here, this one can grow
		_cjkModeBuffer.clear();

		for (const uint16 *p = key; *p; p++)
			_cjkModeBuffer.push_back(*p);

		for (const uint16 *p = translation; *p; p++)
			_cjkModeBuffer.push_back(*p);

		_cjkModeBuffer.push_back(0);

		return &_cjkModeBuffer[0];
	}

	return translation;
}

} // End of namespace Grim

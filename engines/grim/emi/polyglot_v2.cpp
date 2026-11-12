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

#include "engines/grim/emi/emiengine.h"
#include "engines/grim/emi/lmemoryservices.h"
#include "engines/grim/emi/polyglot_v2.h"
#include "engines/grim/std/std_sort.h"
#include "engines/grim/wuregistry.h"

namespace Grim {

Polyglot_v2::Polyglot_v2(EmiEngine *engine) : _engine(engine), _table(nullptr), _lines(nullptr), _lineCount(0), _randomSeed(0) {
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

} // End of namespace Grim

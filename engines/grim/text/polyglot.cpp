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
#include "common/stream.h"
#include "common/textconsole.h"

#include "engines/grim/text/polyglot.h"
#include "engines/grim/std/std_sort.h"
#include "engines/grim/wuregistry.h"

namespace Grim {

Polyglot::Polyglot() : _pHS(nullptr), _pLanguageMalloc(nullptr), _ppLanguageLine(nullptr), _lineCount(0) {
}

int Polyglot::initialize(SysStruct *pHS) {
	_pHS = pHS;

	return 0;
}

int Polyglot::startup() {
	return 0;
}

void Polyglot::shutdown() {
	delete[] _pLanguageMalloc;
	_pLanguageMalloc = nullptr;

	delete[] _ppLanguageLine;
	_ppLanguageLine = nullptr;
}

void Polyglot::configure(const char *tabFile) {
	Common::SeekableReadStream *pFile = _pHS->openFile(tabFile);

	if (!pFile)
		return;

	int size = pFile->size();
	uint32 magic = 0;

	if (size >= 4)
		magic = pFile->readUint32LE();

	pFile->seek(0);

	_pLanguageMalloc = new char[size + 3];
	memset(_pLanguageMalloc + size, 0, 3);
	pFile->read(_pLanguageMalloc, size);
	delete pFile;

	if (magic == MKTAG('E', 'N', 'C', 'R')) {
		size -= 4;

		for (int i = 0; i < size; i++)
			_pLanguageMalloc[i] = _pLanguageMalloc[i + 4] ^ 0xDD;
	}

	char *pLine = _pLanguageMalloc;

	do {
		pLine = findChar(pLine, 13) + 2;
		_lineCount++;
	} while (pLine - _pLanguageMalloc < size - 1);

	_ppLanguageLine = new char *[_lineCount];

	pLine = _pLanguageMalloc;
	int lineIndex = 0;

	do {
		char *pLineEnd = findChar(pLine, 13);

		*pLineEnd = 0;

		char *pTab = strchr(pLine, 9);

		if (pTab)
			*pTab = 0;

		for (char *p = pLine; *p; p++) {
			if (*p >= 'a' && *p <= 'z')
				*p -= 0x20;
		}

		_ppLanguageLine[lineIndex] = pLine;
		pLine = pLineEnd + 2;
		lineIndex++;
	} while (pLine - _pLanguageMalloc < size - 1);

	sort(_ppLanguageLine, _lineCount, sizeof(char *), WuRegistry::compareNames);
}

char *Polyglot::findChar(char *s, int c) {
	char *pResult = s;

	for (char i = *s; i; i = *++pResult) {
		if (i == c)
			break;
	}

	return pResult;
}

const char *Polyglot::getTranslation(const char *msg) {
	char key[256];

	if (!msg)
		return nullptr;

	if (*msg != '/')
		return msg;

	const char *pText = strchr(msg + 1, '/');

	if (pText) {
		pText++;
	} else {
		warning("Polyglot::getTranslation(): the message has no closing slash");
		pText = msg;
	}

	if (!_pLanguageMalloc)
		return pText;

	if (strlen(msg + 1) >= sizeof(key))
		warning("Polyglot::getTranslation(): the message is too long, truncated");

	Common::strlcpy(key, msg + 1, sizeof(key));

	char *pSlash = strchr(key, '/');

	if (pSlash)
		*pSlash = 0;

	for (char *p = key; *p; p++) {
		if (*p >= 'a' && *p <= 'z')
			*p -= 0x20;
	}

	const char *pKey = key;
	char **ppFound = (char **)binarySearch(&pKey, _ppLanguageLine, _lineCount, sizeof(char *), WuRegistry::compareNames);

	if (!ppFound)
		return pText;

	const char *pTranslation = *ppFound + strlen(*ppFound) + 1;

	if (!*pTranslation)
		return msg;

	return pTranslation;
}

} // End of namespace Grim

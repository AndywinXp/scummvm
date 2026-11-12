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

#ifndef GRIM_EMI_LFORMATPARSER_H
#define GRIM_EMI_LFORMATPARSER_H

#include "common/array.h"
#include "common/hashmap.h"

#include "engines/grim/emi/lfile.h"

namespace Grim {

class EmiEngine;

struct KeywordInfo {
	void set(const char *keywordName, int keywordId, int keywordArgMask);

	const char *name;
	int id;
	uint argMask;
};

union KeywordArg {
	int intValue;
	float floatValue;
	char *stringValue;
};

class LFormatParser {
public:
	explicit LFormatParser(EmiEngine *engine);

	virtual bool handleKeyword(int id, int numArgs, KeywordArg *args);

	void addKeyword(const char *name, int id, int argMask);
	void setFile(LFile *newFile);
	void writeEndOfSection();
	bool readEndOfSection();
	void writeComment(const char *text);
	void writeVersion(int versionMajor, int versionMinor);
	void writeEntry(int id, ...);
	bool readVersion(int versionMajor, int versionMinor);
	bool readEntry(int id, ...);
	bool parseFile(const char *path, int versionMajor, int versionMinor);
	LFile *openFile(const char *path, bool readMode);
	KeywordInfo *findKeyword(const char *name);
	bool parseRemaining(int versionMajor, int versionMinor);
	bool parse(int versionMajor, int versionMinor);
	int tokenize();

	static bool skipCopyrightBanner(LFile *file);
	static bool writeCopyrightBanner(LFile *file);

	LFile *file;
	Common::Array<LFile *> fileStack;
	bool reading;
	bool binaryData;
	int numTokens;
	char *tokens[16];
	char lineBuffer[256];
	Common::HashMap<int, KeywordInfo> keywords;

protected:
	~LFormatParser();

	EmiEngine *_engine;
};

} // End of namespace Grim

#endif

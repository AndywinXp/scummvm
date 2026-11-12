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

#ifndef GRIM_EMI_CSNDPARSER_H
#define GRIM_EMI_CSNDPARSER_H

#include "engines/grim/emi/lformatparser.h"

namespace Grim {

class CSnd;

class CSndParser : public LFormatParser {
public:
	enum {
		kKeywordSound = 0,
		kKeywordCutoff = 1,
		kKeywordMvol = 2,
		kKeywordFreq = 3,
		kKeywordPan = 4,
		kKeywordPrio = 5,
		kKeywordFlags = 6,
		kKeywordType = 7,
		kKeywordAiRange = 9,
		kKeywordAiSeriousness = 10,
		kKeywordStream = 11,
		kKeywordGroup = 13,
		kKeywordStDelay = 14,
		kKeywordRndDelay = 15
	};

	explicit CSndParser(EmiEngine *engine);
	~CSndParser();

	void *operator new(size_t size);
	void operator delete(void *block);

	bool handleKeyword(int id, int numArgs, KeywordArg *args) override;

	bool parseSound(int id, int numArgs, KeywordArg *args);
	bool ignoreKeyword(int id, int numArgs, KeywordArg *args);
	bool parseCutoff(int id, int numArgs, KeywordArg *args);
	bool parseMvol(int id, int numArgs, KeywordArg *args);
	bool parseFreq(int id, int numArgs, KeywordArg *args);
	bool parsePan(int id, int numArgs, KeywordArg *args);
	bool parsePrio(int id, int numArgs, KeywordArg *args);
	bool parseFlags(int id, int numArgs, KeywordArg *args);
	bool parseGroup(int id, int numArgs, KeywordArg *args);
	bool parseStDelay(int id, int numArgs, KeywordArg *args);
	bool parseRndDelay(int id, int numArgs, KeywordArg *args);

	CSnd *snd;
};

} // End of namespace Grim

#endif

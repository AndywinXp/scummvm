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

#include "engines/grim/emi/sound/csnd.h"
#include "engines/grim/emi/sound/csndparser.h"
#include "engines/grim/emi/sound/cwavesound.h"
#include "engines/grim/emi/core/lmemoryservices.h"
#include "engines/grim/emi/resources/lresourcehandle.h"

namespace Grim {

enum {
	kArgMaskInt = 1,
	kArgMaskFloat = 2,
	kArgMaskString = 3,
	kArgMaskTwoFloats = 10
};

CSndParser::CSndParser(EmiEngine *engine) : LFormatParser(engine) {
	snd = nullptr;

	addKeyword("SOUND", kKeywordSound, kArgMaskString);
	addKeyword("CUTOFF", kKeywordCutoff, kArgMaskTwoFloats);
	addKeyword("MVOL", kKeywordMvol, kArgMaskFloat);
	addKeyword("FREQ", kKeywordFreq, kArgMaskFloat);
	addKeyword("PAN", kKeywordPan, kArgMaskFloat);
	addKeyword("PRIO", kKeywordPrio, kArgMaskFloat);
	addKeyword("GROUP", kKeywordGroup, kArgMaskString);
	addKeyword("FLAGS", kKeywordFlags, kArgMaskInt);
	addKeyword("TYPE", kKeywordType, kArgMaskString);
	addKeyword("AI_RANGE", kKeywordAiRange, kArgMaskFloat);
	addKeyword("AI_SERIOUSNESS", kKeywordAiSeriousness, kArgMaskFloat);
	addKeyword("STREAM", kKeywordStream, kArgMaskString);
	addKeyword("ST_DELAY", kKeywordStDelay, kArgMaskFloat);
	addKeyword("RND_DELAY", kKeywordRndDelay, kArgMaskTwoFloats);
}

CSndParser::~CSndParser() {
}

void *CSndParser::operator new(size_t size) {
	return LMemoryServices::getCurrent()->mallocImpl(size);
}

void CSndParser::operator delete(void *block) {
	LMemoryServices::getCurrent()->freeImpl(block);
}

bool CSndParser::handleKeyword(int id, int numArgs, KeywordArg *args) {
	bool result = true;

	if (!snd)
		return false;

	switch (id) {
	case kKeywordSound:
		result = parseSound(kKeywordSound, numArgs, args);

		break;

	case kKeywordCutoff:
		result = parseCutoff(kKeywordCutoff, numArgs, args);

		break;

	case kKeywordMvol:
		result = parseMvol(kKeywordMvol, numArgs, args);

		break;

	case kKeywordFreq:
		result = parseFreq(kKeywordFreq, numArgs, args);

		break;

	case kKeywordPan:
		result = parsePan(kKeywordPan, numArgs, args);

		break;

	case kKeywordPrio:
		result = parsePrio(kKeywordPrio, numArgs, args);

		break;

	case kKeywordFlags:
		result = parseFlags(kKeywordFlags, numArgs, args);

		break;

	case kKeywordType:
		result = true;

		break;

	case kKeywordAiRange:
		result = ignoreKeyword(kKeywordAiRange, numArgs, args);

		break;

	case kKeywordAiSeriousness:
		result = ignoreKeyword(kKeywordAiSeriousness, numArgs, args);

		break;

	case kKeywordStream:
		result = ignoreKeyword(kKeywordStream, numArgs, args);

		break;

	case kKeywordGroup:
		result = parseGroup(kKeywordGroup, numArgs, args);

		break;

	case kKeywordStDelay:
		result = parseStDelay(kKeywordStDelay, numArgs, args);

		break;

	case kKeywordRndDelay:
		result = parseRndDelay(kKeywordRndDelay, numArgs, args);

		break;

	default:
		break;
	}

	return result;
}

bool CSndParser::parseSound(int id, int numArgs, KeywordArg *args) {
	if (!snd || numArgs < 1)
		return false;

	// The disasm never frees this handle
	LResourceHandle<CWaveSound> *waveHandle = new LResourceHandle<CWaveSound>(_engine, LString(args[0].stringValue));
	LResourceHandle<CWaveSound> handle;

	handle.setResource(waveHandle->resource);

	if (handle.resource) {
		LResourceHandle<LSound> soundHandle;

		soundHandle.setResource(handle.resource);
		snd->setSoundHandle(soundHandle);
	}

	return handle.resource != nullptr;
}

bool CSndParser::ignoreKeyword(int id, int numArgs, KeywordArg *args) {
	return false;
}

bool CSndParser::parseCutoff(int id, int numArgs, KeywordArg *args) {
	if (!snd || numArgs < 2)
		return false;

	snd->setCutoff0(args[0].floatValue);
	snd->setCutoff1(args[1].floatValue);

	return true;
}

bool CSndParser::parseMvol(int id, int numArgs, KeywordArg *args) {
	if (!snd || numArgs < 1)
		return false;

	snd->setMvol(args[0].floatValue);

	return true;
}

bool CSndParser::parseFreq(int id, int numArgs, KeywordArg *args) {
	if (!snd || numArgs < 1)
		return false;

	snd->setFreq(args[0].floatValue);

	return true;
}

bool CSndParser::parsePan(int id, int numArgs, KeywordArg *args) {
	if (!snd || numArgs < 1)
		return false;

	snd->setPan(args[0].floatValue);

	return true;
}

bool CSndParser::parsePrio(int id, int numArgs, KeywordArg *args) {
	if (!snd || numArgs < 1)
		return false;

	snd->setPrio(args[0].floatValue);

	return true;
}

bool CSndParser::parseFlags(int id, int numArgs, KeywordArg *args) {
	if (numArgs < 1)
		return false;

	snd->setFlags(args[0].intValue);

	return true;
}

bool CSndParser::parseGroup(int id, int numArgs, KeywordArg *args) {
	if (numArgs < 1)
		return false;

	if (!args[0].stringValue)
		return false;

	LString group(args[0].stringValue);

	if (group == "Voice")
		snd->setGroup(2);
	else if (group == "Music")
		snd->setGroup(3);
	else if (group == "System")
		snd->setGroup(4);
	else
		snd->setGroup(1);

	return true;
}

bool CSndParser::parseStDelay(int id, int numArgs, KeywordArg *args) {
	if (numArgs < 1)
		return false;

	snd->setStDelay(args[0].floatValue);

	return true;
}

bool CSndParser::parseRndDelay(int id, int numArgs, KeywordArg *args) {
	if (numArgs < 2)
		return false;

	snd->setRndDelay0(args[0].floatValue);
	snd->setRndDelay1(args[1].floatValue);

	return true;
}

} // End of namespace Grim

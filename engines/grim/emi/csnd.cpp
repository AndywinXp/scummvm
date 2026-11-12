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

#include "common/textconsole.h"

#include "engines/grim/emi/csnd.h"
#include "engines/grim/emi/csndparser.h"
#include "engines/grim/emi/lformatparser.h"
#include "engines/grim/grim.h"

namespace Grim {

LResourceType CSnd::sndType("SND");

CSnd::CSnd() {
	volume = 1.0f;
	mvol = 1.0f;
	prio = 0.5f;
	pan = 0.0f;
	cutoff0 = 1.0f;
	cutoff1 = 50.0f;
	flags = 0;
	group = 1;
	freq = 1.0f;
	stDelay = 0.0f;
	rndDelay0 = 0.0f;
	rndDelay1 = 0.0f;
	field28 = 0.0f;
	field24 = 0.0f;
}

CSnd::~CSnd() {
}

LResourceHandle<LSound> *CSnd::getSoundHandle() {
	return &sound;
}

void CSnd::setSoundHandle(const LResourceHandle<LSound> &handle) {
	sound.setResource(handle.resource);
}

float CSnd::getPrio() {
	return prio;
}

float CSnd::getPan() {
	return pan;
}

float CSnd::getVolume() {
	return volume;
}

float CSnd::getMvol() {
	return mvol;
}

float CSnd::getFreq() {
	return freq;
}

float CSnd::getCutoff0() {
	return cutoff0;
}

float CSnd::getCutoff1() {
	return cutoff1;
}

float CSnd::getField24() {
	return field24;
}

float CSnd::getField28() {
	return field28;
}

float CSnd::getStDelay() {
	return stDelay;
}

float CSnd::getRndDelay0() {
	return rndDelay0;
}

float CSnd::getRndDelay1() {
	return rndDelay1;
}

int CSnd::getFlags() {
	return flags;
}

int CSnd::getGroup() {
	return group;
}

void CSnd::setPrio(float value) {
	prio = value;
}

void CSnd::setPan(float value) {
	pan = value;
}

void CSnd::setVolume(float value) {
	volume = value;
}

void CSnd::setMvol(float value) {
	mvol = value;
}

void CSnd::setFreq(float value) {
	freq = value;
}

int CSnd::setCutoff0(float value) {
	cutoff0 = value;

	return value != 0.0f;
}

int CSnd::setCutoff1(float value) {
	cutoff1 = value;

	return value != 0.0f;
}

float CSnd::setField24(float value) {
	field24 = value;

	return value;
}

float CSnd::setField28(float value) {
	field28 = value;

	return value;
}

void CSnd::setStDelay(float value) {
	stDelay = value;
}

void CSnd::setRndDelay0(float value) {
	rndDelay0 = value;
}

void CSnd::setRndDelay1(float value) {
	rndDelay1 = value;
}

void CSnd::setFlags(int value) {
	flags = value;
}

void CSnd::setGroup(int value) {
	group = value;
}

bool CSnd::save(LFile *file) {
	if (!file)
		return false;

	CSndParser parser;

	parser.setFile(file);
	LFormatParser::writeCopyrightBanner(file);
	parser.writeVersion(0, 0);
	parser.writeComment("");
	parser.writeComment(" Tanis Sound Description File");
	parser.writeComment("");

	const char *soundName = "";

	// The disasm does not check the resource
	if (sound.resource)
		soundName = sound.resource->name.c_str();
	else
		warning("CSnd::save(): the sound has no resource");

	parser.writeEntry(CSndParser::kKeywordSound, soundName);
	parser.writeEntry(CSndParser::kKeywordCutoff, cutoff0, cutoff1);
	parser.writeEntry(CSndParser::kKeywordMvol, mvol);
	parser.writeEntry(CSndParser::kKeywordFreq, freq);
	parser.writeEntry(CSndParser::kKeywordPan, pan);
	parser.writeEntry(CSndParser::kKeywordPrio, prio);

	switch (group) {
	case 2:
		parser.writeEntry(CSndParser::kKeywordGroup, "Voice");

		break;

	case 3:
		parser.writeEntry(CSndParser::kKeywordGroup, "Music");

		break;

	case 4:
		parser.writeEntry(CSndParser::kKeywordGroup, "System");

		break;

	default:
		parser.writeEntry(CSndParser::kKeywordGroup, "Sfx");

		break;
	}

	parser.writeEntry(CSndParser::kKeywordFlags, flags);

	return true;
}

bool CSnd::resLoadItem(const char *itemName) {
	CSndParser *parser = new CSndParser();

	parser->snd = this;

	bool result = parser->parseFile(itemName, 0, 0);

	if (result) {
		resSetType(sndType);
		resSetName(itemName);
		resSetLoaded(true);
	} else {
		LString aifName(itemName);
		uint dot = aifName.find('.');

		if (dot != LString::npos)
			aifName = aifName.substr(0, dot);

		aifName += g_grim->getGamePlatform() == Common::kPlatformPS2 ? ".SCX" : ".aif";

		KeywordArg argument;

		argument.stringValue = &aifName[0];

		if (parser->parseSound(CSndParser::kKeywordSound, 1, &argument)) {
			resSetType(sndType);
			resSetName(itemName);
			result = true;
			resSetLoaded(true);
		} else {
			parser->snd = nullptr;
		}
	}

	delete parser;

	return result;
}

} // End of namespace Grim

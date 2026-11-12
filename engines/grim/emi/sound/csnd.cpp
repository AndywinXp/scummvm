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

#include "engines/grim/emi/sound/csnd.h"
#include "engines/grim/emi/sound/csndparser.h"
#include "engines/grim/emi/emiengine.h"
#include "engines/grim/emi/core/lformatparser.h"

namespace Grim {

const LResourceType CSnd::kSndType("SND");

CSnd::CSnd() {
	_volume = 1.0f;
	_mvol = 1.0f;
	_prio = 0.5f;
	_pan = 0.0f;
	_cutoff0 = 1.0f;
	_cutoff1 = 50.0f;
	_flags = 0;
	_group = 1;
	_freq = 1.0f;
	_stDelay = 0.0f;
	_rndDelay0 = 0.0f;
	_rndDelay1 = 0.0f;
	_field28 = 0.0f;
	_field24 = 0.0f;
}

CSnd::~CSnd() {
}

LResourceHandle<LSound> *CSnd::getSoundHandle() {
	return &_sound;
}

void CSnd::setSoundHandle(const LResourceHandle<LSound> &handle) {
	_sound.setResource(handle._resource);
}

float CSnd::getPrio() {
	return _prio;
}

float CSnd::getPan() {
	return _pan;
}

float CSnd::getVolume() {
	return _volume;
}

float CSnd::getMvol() {
	return _mvol;
}

float CSnd::getFreq() {
	return _freq;
}

float CSnd::getCutoff0() {
	return _cutoff0;
}

float CSnd::getCutoff1() {
	return _cutoff1;
}

float CSnd::getField24() {
	return _field24;
}

float CSnd::getField28() {
	return _field28;
}

float CSnd::getStDelay() {
	return _stDelay;
}

float CSnd::getRndDelay0() {
	return _rndDelay0;
}

float CSnd::getRndDelay1() {
	return _rndDelay1;
}

int CSnd::getFlags() {
	return _flags;
}

int CSnd::getGroup() {
	return _group;
}

void CSnd::setPrio(float value) {
	_prio = value;
}

void CSnd::setPan(float value) {
	_pan = value;
}

void CSnd::setVolume(float value) {
	_volume = value;
}

void CSnd::setMvol(float value) {
	_mvol = value;
}

void CSnd::setFreq(float value) {
	_freq = value;
}

int CSnd::setCutoff0(float value) {
	_cutoff0 = value;

	return value != 0.0f;
}

int CSnd::setCutoff1(float value) {
	_cutoff1 = value;

	return value != 0.0f;
}

float CSnd::setField24(float value) {
	_field24 = value;

	return value;
}

float CSnd::setField28(float value) {
	_field28 = value;

	return value;
}

void CSnd::setStDelay(float value) {
	_stDelay = value;
}

void CSnd::setRndDelay0(float value) {
	_rndDelay0 = value;
}

void CSnd::setRndDelay1(float value) {
	_rndDelay1 = value;
}

void CSnd::setFlags(int value) {
	_flags = value;
}

void CSnd::setGroup(int value) {
	_group = value;
}

bool CSnd::save(LFile *file) {
	if (!file)
		return false;

	CSndParser parser(_engine);

	parser.setFile(file);
	LFormatParser::writeCopyrightBanner(file);
	parser.writeVersion(0, 0);
	parser.writeComment("");
	parser.writeComment(" Tanis Sound Description File");
	parser.writeComment("");

	const char *soundName = "";

	// The disasm does not check the resource
	if (_sound._resource)
		soundName = _sound._resource->_name.c_str();
	else
		warning("CSnd::save(): the sound has no resource");

	parser.writeEntry(CSndParser::kKeywordSound, soundName);
	parser.writeEntry(CSndParser::kKeywordCutoff, _cutoff0, _cutoff1);
	parser.writeEntry(CSndParser::kKeywordMvol, _mvol);
	parser.writeEntry(CSndParser::kKeywordFreq, _freq);
	parser.writeEntry(CSndParser::kKeywordPan, _pan);
	parser.writeEntry(CSndParser::kKeywordPrio, _prio);

	switch (_group) {
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

	parser.writeEntry(CSndParser::kKeywordFlags, _flags);

	return true;
}

bool CSnd::resLoadItem(const char *itemName) {
	CSndParser *parser = new CSndParser(_engine);

	parser->_snd = this;

	bool result = parser->parseFile(itemName, 0, 0);

	if (result) {
		resSetType(kSndType);
		resSetName(itemName);
		resSetLoaded(true);
	} else {
		LString aifName(itemName);
		uint dot = aifName.find('.');

		if (dot != LString::npos)
			aifName = aifName.substr(0, dot);

		aifName += _engine->getGamePlatform() ==Common::kPlatformPS2 ? ".SCX" : ".aif";

		KeywordArg argument;

		argument.stringValue = &aifName[0];

		if (parser->parseSound(CSndParser::kKeywordSound, 1, &argument)) {
			resSetType(kSndType);
			resSetName(itemName);
			result = true;
			resSetLoaded(true);
		} else {
			parser->_snd = nullptr;
		}
	}

	delete parser;

	return result;
}

} // End of namespace Grim

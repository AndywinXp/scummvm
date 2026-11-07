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

#include "audio/mixer.h"

#include "common/config-manager.h"
#include "common/endian.h"
#include "common/textconsole.h"
#include "common/util.h"

#include "engines/grim/std/std_sort.h"
#include "engines/grim/wuregistry.h"

namespace Grim {

static Common::ConfigManager::Domain *getConfDomain() {
	Common::ConfigManager::Domain *pDomain = ConfMan.getActiveDomain();

	if (!pDomain)
		pDomain = ConfMan.getDomain(Common::ConfigManager::kApplicationDomain);

	return pDomain;
}

static bool isStorableName(const char *valueName) {
	uint length = strlen(valueName);

	if (!length || valueName[0] == ' ' || valueName[length - 1] == ' ' || valueName[0] == '#' || valueName[0] == '[')
		return false;

	for (uint i = 0; i < length; i++) {
		if (valueName[i] == '=' || (byte)valueName[i] < 0x20)
			return false;
	}

	return true;
}

static bool isStorableString(const char *pData) {
	uint length = strlen(pData);

	if (length && (pData[0] == ' ' || pData[0] == '\t' || pData[length - 1] == ' ' || pData[length - 1] == '\t'))
		return false;

	for (uint i = 0; i < length; i++) {
		if (pData[i] == '\r' || pData[i] == '\n')
			return false;
	}

	return true;
}

static bool canParseInt(const Common::String &text) {
	char *pEnd;

	if (text.empty())
		return true;

	strtol(text.c_str(), &pEnd, 0);

	return pEnd != text.c_str();
}

static bool canParseFloat(const Common::String &text) {
	char *pEnd;

	if (text.empty())
		return true;

	strtof(text.c_str(), &pEnd);

	return pEnd != text.c_str();
}

static bool isNumberText(const Common::String &text) {
	char *pEnd;

	if (text.empty())
		return false;

	strtod(text.c_str(), &pEnd);

	return *pEnd == '\0';
}

static bool queryText(const Common::String &text, byte *pData, uint *pSize) {
	if (text.size() > *pSize) {
		*pSize = text.size();
		return false;
	}

	if (text.size())
		memcpy(pData, text.c_str(), text.size());

	if (text.size() < *pSize)
		pData[text.size()] = 0;

	*pSize = text.size();

	return true;
}

enum {
	kMapString,
	kMapInt,
	kMapVolume,
	kMapTalkSpeed,
	kMapSpeechMode,
	kMapBool,
	kMapPath,
	kMapFloat,
	kMapLastSet
};

struct WuRegistryMapping {
	const char *name;
	const char *key;
	int kind;
};

static const WuRegistryMapping g_mappings[] = {
	{ "good_times", "game_devel_mode", kMapBool },
	{ "GrimDeveloper", "game_devel_mode", kMapBool },
	{ "GrimDataDir", "path", kMapPath },
	{ "GrimLastSet", "last_set", kMapLastSet },
	{ "MusicVolume", "music_volume", kMapVolume },
	{ "SfxVolume", "sfx_volume", kMapVolume },
	{ "VoiceVolume", "speech_volume", kMapVolume },
	{ "LastSavedGame", "last_saved_game", kMapString },
	{ "Gamma", "grim_gamma", kMapFloat },
	{ "GammaCorrection", "grim_gamma", kMapFloat },
	{ "VoiceEffects", "voice_effects", kMapString },
	{ "TextSpeed", "talkspeed", kMapTalkSpeed },
	{ "TextMode", "subtitles", kMapSpeechMode },
	{ "SpeechMode", "subtitles", kMapSpeechMode },
	{ "MovementMode", "movement", kMapString },
	{ "JoystickEnabled", "joystick", kMapString },
	{ "SpewOnError", "spew_on_error", kMapString },
	{ "Transcript", "transcript", kMapString },
	{ "DirectorsCommentary", "directors_commentary", kMapInt },
	{ "Widescreen", "widescreen", kMapInt },
	{ "Language", "grim_language", kMapInt },
	{ "ResolutionScaling", "resolution_scaling", kMapInt },
	{ "MouseSpeed", "mouse_speed", kMapInt },
	{ "AdvancedLighting", "advanced_lighting", kMapInt },
	{ "DirectorsCommentaryVolume", "directors_commentary_volume", kMapVolume },
	{ "RenderingMode", "rendering_mode", kMapInt },
	{ "Fullscreen", "grim_fullscreen", kMapInt }
};

static uint convertVolumeToMixer(uint volume) {
	return CLIP<uint>(volume * 2, 0, Audio::Mixer::kMaxMixerVolume);
}

static uint convertVolumeFromMixer(uint volume) {
	return CLIP<uint>(volume / 2, 0, 127);
}

static uint convertTalkSpeedToGUI(uint talkSpeed) {
	return CLIP<uint>(talkSpeed * 255 / 10, 0, 255);
}

static uint convertTalkSpeedFromGUI(uint talkSpeed) {
	return CLIP<uint>(talkSpeed * 10 / 255, 1, 10);
}

static bool convertSubtitlesToGUI(uint speechMode) {
	return speechMode == 1 || speechMode == 3;
}

static bool convertSpeechMuteToGUI(uint speechMode) {
	return speechMode == 1;
}

static uint convertSpeechModeFromGUI(bool bSubtitles, bool bSpeechMute) {
	if (!bSubtitles && !bSpeechMute)
		return 2;

	if (bSubtitles && !bSpeechMute)
		return 3;

	if (bSubtitles && bSpeechMute)
		return 1;

	warning("WuRegistry: both subtitles and speech are off, assuming subtitles only");

	return 1;
}

static const WuRegistryMapping *findMapping(const char *valueName) {
	for (uint i = 0; i < ARRAYSIZE(g_mappings); i++) {
		if (!scumm_stricmp(g_mappings[i].name, valueName))
			return &g_mappings[i];
	}

	return nullptr;
}

static bool isStringKind(int kind) {
	return kind == kMapString || kind == kMapPath || kind == kMapFloat || kind == kMapLastSet;
}

static uint getMappedType(const WuRegistryMapping *pMapping) {
	if (pMapping->kind == kMapFloat)
		return ConfMan.hasKey(pMapping->key) ? WuRegistry::kValueBinary : 0;

	if (isStringKind(pMapping->kind))
		return WuRegistry::kValueString;

	if (pMapping->kind == kMapBool && !ConfMan.getBool(pMapping->key))
		return 0;

	return WuRegistry::kValueBinary;
}

static bool readMappedText(const WuRegistryMapping *pMapping, Common::String *pText);

static bool readMappedInt(const WuRegistryMapping *pMapping, int *pValue) {
	switch (pMapping->kind) {
	case kMapInt:
		*pValue = ConfMan.getInt(pMapping->key);
		return true;
	case kMapVolume:
		*pValue = convertVolumeFromMixer(ConfMan.getInt(pMapping->key));
		return true;
	case kMapTalkSpeed:
		*pValue = convertTalkSpeedFromGUI(ConfMan.getInt(pMapping->key));
		return true;
	case kMapSpeechMode:
		*pValue = convertSpeechModeFromGUI(ConfMan.getBool("subtitles"), ConfMan.getBool("speech_mute"));
		return true;
	case kMapBool:
		*pValue = ConfMan.getBool(pMapping->key) ? 1 : 0;
		return *pValue != 0;
	default:
		break;
	}

	Common::String text;

	readMappedText(pMapping, &text);
	*pValue = atoi(text.c_str());

	return true;
}

static bool readMappedText(const WuRegistryMapping *pMapping, Common::String *pText) {
	if (pMapping->kind == kMapPath) {
		*pText = ConfMan.getPath(pMapping->key).toString(Common::Path::kNativeSeparator);
		return true;
	}

	if (isStringKind(pMapping->kind)) {
		*pText = ConfMan.get(pMapping->key);
		return true;
	}

	int value;

	if (!readMappedInt(pMapping, &value))
		return false;

	*pText = Common::String::format("%d", value);

	return true;
}

static void writeMappedInt(const WuRegistryMapping *pMapping, int value);

static void writeMappedText(const WuRegistryMapping *pMapping, const char *pText) {
	switch (pMapping->kind) {
	case kMapString:
	case kMapFloat:
		ConfMan.set(pMapping->key, pText);
		break;
	case kMapPath:
		ConfMan.setPath(pMapping->key, Common::Path(pText, Common::Path::kNativeSeparator));
		break;
	case kMapLastSet:
		if (ConfMan.hasKey(pMapping->key))
			ConfMan.set(pMapping->key, pText);

		break;
	case kMapBool:
		ConfMan.setBool(pMapping->key, !scumm_stricmp(pText, "true"));
		break;
	default:
		writeMappedInt(pMapping, atoi(pText));
		break;
	}
}

static void writeMappedInt(const WuRegistryMapping *pMapping, int value) {
	switch (pMapping->kind) {
	case kMapInt:
		ConfMan.setInt(pMapping->key, value);
		break;
	case kMapVolume:
		ConfMan.setInt(pMapping->key, convertVolumeToMixer(value));
		break;
	case kMapTalkSpeed:
		ConfMan.setInt(pMapping->key, convertTalkSpeedToGUI(value));
		break;
	case kMapSpeechMode:
		ConfMan.setBool("subtitles", convertSubtitlesToGUI(value));
		ConfMan.setBool("speech_mute", convertSpeechMuteToGUI(value));
		break;
	case kMapBool:
		ConfMan.setBool(pMapping->key, value != 0);
		break;
	default:
		writeMappedText(pMapping, Common::String::format("%d", value).c_str());
		break;
	}
}

WuRegistry::WuRegistry() : _bStarted(0), _aValueNames(nullptr), _numValues(0) {
}

WuRegistry::~WuRegistry() {
	if (_aValueNames)
		freeIndex();
}

int WuRegistry::startup(const char *version) {
	_bStarted = 1;

	ConfMan.registerDefault("subtitles", true);
	ConfMan.registerDefault("talkspeed", 179);
	ConfMan.registerDefault("game_devel_mode", false);

	if (ConfMan.getInt("talkspeed") == 60)
		ConfMan.setInt("talkspeed", 179);

	ConfMan.set("joystick", "false");

	char storedVersion[128];

	if (getStr("Version", storedVersion, sizeof(storedVersion), nullptr) && scumm_stricmp(storedVersion, version))
		warning("WuRegistry::startup(): the stored version '%s' differs from '%s', the stored values are kept", storedVersion, version);

	saveStr("Version", version);

	return loadIndex();
}

int WuRegistry::loadIndex() {
	_numValues = _values.size();
	_aValueNames = new char *[_numValues];

	for (uint i = 0; i < _numValues; i++) {
		Common::String name = _values[i].name;

		name.toLowercase();
		_aValueNames[i] = new char[name.size() + 1];
		memcpy(_aValueNames[i], name.c_str(), name.size() + 1);
	}

	sort(_aValueNames, _numValues, sizeof(char *), compareNames);

	return 0;
}

int WuRegistry::compareNames(const void *pA, const void *pB) {
	return strcmp(*(const char *const *)pA, *(const char *const *)pB);
}

void WuRegistry::shutdown() {
	freeIndex();
	_bStarted = 0;

	ConfMan.flushToDisk();
}

void WuRegistry::freeIndex() {
	for (uint i = 0; i < _numValues; i++)
		delete[] _aValueNames[i];

	delete[] _aValueNames;
	_aValueNames = nullptr;
	_numValues = 0;
}

int WuRegistry::saveInt(const char *valueName, int value) {
	Common::String key;
	const WuRegistryMapping *pMapping = findMapping(valueName);

	if (pMapping) {
		writeMappedInt(pMapping, value);

		return 1;
	}

	if (claimKey(valueName, kValueBinary, &key)) {
		addIndex(valueName);
		ConfMan.setInt(key, value);

		return 1;
	}

	byte data[4];

	WRITE_LE_UINT32(data, (uint32)value);
	setValue(valueName, kValueBinary, data, 4);
	addIndex(valueName);

	return 1;
}

int WuRegistry::addIndex(const char *valueName) {
	int index = findIndex(valueName);

	if (!index) {
		Common::String name = valueName;

		name.toLowercase();

		char **aNewNames = new char *[_numValues + 1];

		for (uint i = 0; i < _numValues; i++)
			aNewNames[i] = _aValueNames[i];

		delete[] _aValueNames;
		_aValueNames = aNewNames;
		_numValues++;

		_aValueNames[_numValues - 1] = new char[name.size() + 1];
		memcpy(_aValueNames[_numValues - 1], name.c_str(), name.size() + 1);

		sort(_aValueNames, _numValues, sizeof(char *), compareNames);
	}

	return index;
}

int WuRegistry::findIndex(const char *valueName) {
	const WuRegistryMapping *pMapping = findMapping(valueName);

	if (pMapping)
		return getMappedType(pMapping) ? 1 : 0;

	if (!_aValueNames)
		return 1;

	Common::String name = valueName;

	name.toLowercase();

	const char *pKey = name.c_str();
	void *pFound = binarySearch(&pKey, _aValueNames, _numValues, sizeof(char *), compareNames);

	if (pFound)
		return 1;

	return ConfMan.hasKey(valueName) ? 1 : 0;
}

int WuRegistry::saveFloat(const char *valueName, float value) {
	Common::String key;
	const WuRegistryMapping *pMapping = findMapping(valueName);

	if (pMapping && pMapping->kind == kMapFloat) {
		ConfMan.set(pMapping->key, Common::String::format("%.9g", value));
		addIndex(valueName);

		return 1;
	}

	if (pMapping) {
		if (!(value > -2147483648.0f && value < 2147483648.0f)) {
			warning("WuRegistry::saveFloat(): the value of '%s' does not fit an integer", valueName);
			return 1;
		}

		writeMappedInt(pMapping, (int)value);

		return 1;
	}

	if (claimKey(valueName, kValueBinary, &key)) {
		addIndex(valueName);
		ConfMan.set(key, Common::String::format("%.9g", value));

		return 1;
	}

	byte data[4];
	uint32 bits;

	memcpy(&bits, &value, sizeof(bits));
	WRITE_LE_UINT32(data, bits);
	setValue(valueName, kValueBinary, data, 4);
	addIndex(valueName);

	return 1;
}

int WuRegistry::getInt(const char *valueName, int defaultValue) {
	byte data[4] = { 0, 0, 0, 0 };
	uint size = 4;
	Common::String key;
	const WuRegistryMapping *pMapping = findMapping(valueName);

	if (!findIndex(valueName))
		return defaultValue;

	if (pMapping) {
		int mappedValue;

		if (readMappedInt(pMapping, &mappedValue))
			return mappedValue;

		return defaultValue;
	}

	if (findConfKey(valueName, &key)) {
		if (!canParseInt(ConfMan.get(key))) {
			warning("WuRegistry::getInt(): the value of '%s' is not a number", valueName);
			return defaultValue;
		}

		return ConfMan.getInt(key);
	}

	if (queryValue(valueName, nullptr, data, &size)) {
		if (size < 4)
			warning("WuRegistry::getInt(): the value is shorter than 4 bytes");

		return (int)READ_LE_UINT32(data);
	}

	return defaultValue;
}

float WuRegistry::getFloat(const char *valueName, float defaultValue) {
	byte data[4] = { 0, 0, 0, 0 };
	uint size = 4;
	Common::String key;
	const WuRegistryMapping *pMapping = findMapping(valueName);

	if (!findIndex(valueName))
		return defaultValue;

	if (pMapping && pMapping->kind == kMapFloat) {
		if (!canParseFloat(ConfMan.get(pMapping->key))) {
			warning("WuRegistry::getFloat(): the value of '%s' is not a number", valueName);
			return defaultValue;
		}

		return ConfMan.getFloat(pMapping->key);
	}

	if (pMapping) {
		int mappedValue;

		if (readMappedInt(pMapping, &mappedValue))
			return (float)mappedValue;

		return defaultValue;
	}

	if (findConfKey(valueName, &key)) {
		if (!canParseFloat(ConfMan.get(key))) {
			warning("WuRegistry::getFloat(): the value of '%s' is not a number", valueName);
			return defaultValue;
		}

		return ConfMan.getFloat(key);
	}

	if (queryValue(valueName, nullptr, data, &size)) {
		if (size < 4)
			warning("WuRegistry::getFloat(): the value is shorter than 4 bytes");

		uint32 bits = READ_LE_UINT32(data);
		float value;

		memcpy(&value, &bits, sizeof(value));

		return value;
	}

	return defaultValue;
}

int WuRegistry::saveBinary(const char *valueName, const byte *pData, uint size) {
	// TODO: binary values are not persisted
	setValue(valueName, kValueBinary, pData, size);
	addIndex(valueName);

	return 1;
}

int WuRegistry::getBinary(const char *valueName, byte *pData, uint size) {
	if (findIndex(valueName) && queryValue(valueName, nullptr, pData, &size))
		return 1;

	return 0;
}

int WuRegistry::saveStr(const char *valueName, const char *pData) {
	Common::String key;
	const WuRegistryMapping *pMapping = findMapping(valueName);

	if (pMapping) {
		int mappedResult = addIndex(valueName);

		writeMappedText(pMapping, pData);

		return mappedResult;
	}

	bool bStorable = isStorableString(pData);

	if (!bStorable)
		warning("WuRegistry::saveStr(): the string of '%s' cannot be stored in the configuration", valueName);

	if (bStorable && claimKey(valueName, kValueString, &key)) {
		int result = addIndex(valueName);

		ConfMan.set(key, pData);

		return result;
	}

	setValue(valueName, kValueString, (const byte *)pData, strlen(pData));

	return addIndex(valueName);
}

int WuRegistry::getStr(const char *valueName, char *pData, uint size, const char *pDefault) {
	int result = findIndex(valueName);

	if (!result)
		return 0;

	if (!size) {
		warning("WuRegistry::getStr(): empty buffer");
		return 0;
	}

	Common::String key;
	uint type = kValueString;
	uint storedSize = size;
	bool bQueried;
	const WuRegistryMapping *pMapping = findMapping(valueName);
	Common::String text;

	if (pMapping)
		bQueried = readMappedText(pMapping, &text) && queryText(text, (byte *)pData, &storedSize);
	else if (findConfKey(valueName, &key))
		bQueried = queryText(ConfMan.get(key), (byte *)pData, &storedSize);
	else
		bQueried = queryValue(valueName, &type, (byte *)pData, &storedSize);

	if (!bQueried) {
		if (pDefault) {
			strncpy(pData, pDefault, size - 1);
			pData[size - 1] = 0;
		}

		return 0;
	}

	if (type == kValueString && storedSize >= size) {
		warning("WuRegistry::getStr(): the string has no room for its terminator");
		pData[size - 1] = 0;
	}

	return 1;
}

uint WuRegistry::queryType(const char *valueName) {
	const WuRegistryMapping *pMapping = findMapping(valueName);

	if (pMapping)
		return getMappedType(pMapping);

	int index = findValue(valueName);
	Common::String key;

	if (index >= 0 && !_values[index].bInConfMan)
		return _values[index].type;

	if (!findConfKey(valueName, &key))
		return 0;

	if (index >= 0)
		return _values[index].type;

	return isNumberText(ConfMan.get(key)) ? kValueBinary : kValueString;
}

int WuRegistry::findValue(const char *valueName) const {
	for (uint i = 0; i < _values.size(); i++) {
		if (_values[i].name.equalsIgnoreCase(valueName))
			return i;
	}

	return -1;
}

void WuRegistry::setValue(const char *valueName, uint type, const byte *pData, uint size) {
	int index = findValue(valueName);

	if (index < 0) {
		WuRegistryValue value;

		value.name = valueName;
		value.type = type;
		value.bInConfMan = false;
		_values.push_back(value);
		index = _values.size() - 1;
	}

	WuRegistryValue &value = _values[index];

	if (value.bInConfMan) {
		Common::ConfigManager::Domain *pDomain = getConfDomain();

		if (pDomain)
			pDomain->erase(value.name);

		value.bInConfMan = false;
	}

	value.type = type;
	value.data.resize(size);

	if (size)
		memcpy(value.data.data(), pData, size);
}

bool WuRegistry::claimKey(const char *valueName, uint type, Common::String *pKey) {
	if (!isStorableName(valueName)) {
		warning("WuRegistry::claimKey(): the value name '%s' cannot be stored in the configuration", valueName);
		return false;
	}

	int index = findValue(valueName);

	if (index < 0) {
		WuRegistryValue value;

		value.name = valueName;
		value.type = type;
		value.bInConfMan = true;
		_values.push_back(value);
		index = _values.size() - 1;
	}

	WuRegistryValue &value = _values[index];

	value.type = type;
	value.bInConfMan = true;
	value.data.clear();

	*pKey = value.name;

	return true;
}

bool WuRegistry::findConfKey(const char *valueName, Common::String *pKey) const {
	int index = findValue(valueName);

	if (index >= 0) {
		if (!_values[index].bInConfMan)
			return false;

		*pKey = _values[index].name;
	} else {
		*pKey = valueName;
	}

	return ConfMan.hasKey(*pKey);
}

bool WuRegistry::queryValue(const char *valueName, uint *pType, byte *pData, uint *pSize) const {
	int index = findValue(valueName);

	if (index < 0 || _values[index].bInConfMan)
		return false;

	const WuRegistryValue &value = _values[index];
	uint storedSize = value.data.size();

	if (pType)
		*pType = value.type;

	if (!pData)
		return true;

	if (storedSize > *pSize) {
		*pSize = storedSize;
		return false;
	}

	if (storedSize)
		memcpy(pData, value.data.data(), storedSize);

	if (value.type == kValueString && storedSize < *pSize)
		pData[storedSize] = 0;

	*pSize = storedSize;

	return true;
}

} // End of namespace Grim

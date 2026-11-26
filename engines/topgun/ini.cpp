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
#include "common/savefile.h"
#include "common/str.h"
#include "common/system.h"
#include "common/textconsole.h"

#include "topgun/topgun_v1_2.h"

namespace Topgun {

Common::String TopgunEngine_v1_2::getIniFileName() const {
	// The profile lives in the save folder under the code name of the game
	return getGameId() + ".ini";
}

void TopgunEngine_v1_2::loadIniFile() {
	if (_iniFileLoaded)
		return;

	_iniFileLoaded = true;

	// Section and key names are not limited to the characters INIFile accepts by default
	_iniFile.allowNonEnglishCharacters();
	_iniFile.suppressValuelessLineWarning();

	Common::String fileName = getIniFileName();

	if (!g_system->getSavefileManager()->exists(fileName))
		return;

	if (!_iniFile.loadFromSaveFile(fileName)) {
		warning("TopgunEngine_v1_2::loadIniFile(): cannot read '%s', starting with an empty profile", fileName.c_str());
		_iniFile.clear();
	}
}

void TopgunEngine_v1_2::saveIniFile() {
	Common::String fileName = getIniFileName();

	Common::OutSaveFile *saveFile = g_system->getSavefileManager()->openForSaving(fileName, false);
	if (!saveFile) {
		warning("TopgunEngine_v1_2::saveIniFile(): cannot write '%s'", fileName.c_str());
		return;
	}

	if (!_iniFile.saveToStream(*saveFile))
		warning("TopgunEngine_v1_2::saveIniFile(): cannot write '%s'", fileName.c_str());

	delete saveFile;
}

uint16 TopgunEngine_v1_2::getPrivateProfileString(const char *section, const char *key, char *buffer, uint16 size) {
	loadIniFile();

	Common::String value;
	_iniFile.getKey(key, section, value);

	uint16 length = (uint16)MIN<uint32>(value.size(), size - 1);
	memcpy(buffer, value.c_str(), length);
	buffer[length] = '\0';

	return length;
}

void TopgunEngine_v1_2::writePrivateProfileString(const char *section, const char *key, const char *value) {
	loadIniFile();

	if (!_iniFile.isValidName(section)) {
		warning("TopgunEngine_v1_2::writePrivateProfileString(): the section name '%s' cannot be stored (key '%s')", section, key ? key : "");
		return;
	}

	if (!key) {
		_iniFile.removeSection(section);
	} else {
		if (!_iniFile.isValidName(key)) {
			warning("TopgunEngine_v1_2::writePrivateProfileString(): the key name '%s' cannot be stored", key);
			return;
		}

		if (!value)
			_iniFile.removeKey(key, section);
		else
			_iniFile.setKey(key, section, value);
	}

	saveIniFile();
}

Common::String TopgunEngine_v1_2::makeUniqueBackupFileName() const {
	Common::String fileName;

	for (uint16 number = 1; number <= TG_INI_V1_2_BACKUP_LAST; number++) {
		fileName = Common::String::format("%s.%03d", getGameId().c_str(), number);

		if (!g_system->getSavefileManager()->exists(fileName))
			return fileName;
	}

	return fileName;
}

bool TopgunEngine_v1_2::copyFileBlocks(Common::SeekableReadStream *source, Common::WriteStream *destination) {
	byte buffer[TG_INI_V1_2_COPY_CHUNK];
	uint32 remaining = source->size();
	bool result = true;

	while (result && remaining != 0) {
		uint32 chunk = MIN<uint32>(remaining, TG_INI_V1_2_COPY_CHUNK);
		remaining -= chunk;

		result = (source->read(buffer, chunk) == chunk && destination->write(buffer, chunk) == chunk);
	}

	return result;
}

void TopgunEngine_v1_2::decodeIniString(byte key, char *string) {
	char decoded[TG_INI_V1_2_BUFFER_SIZE + 1];
	uint16 length = 0;

	const byte *source = (const byte *)string;

	if (*source != 0) {
		do {
			byte low = source[0];
			byte high = source[1];

			decoded[length] = (char)(byte)((byte)((byte)(low - TG_INI_V1_2_CIPHER_BASE) | (byte)(high << TG_INI_V1_2_CIPHER_SHIFT)) - key);
			length++;

			if (high == 0) {
				warning("TopgunEngine_v1_2::decodeIniString(): the string has an odd number of characters");
				break;
			}

			source += 2;
		} while (*source != 0 && length < TG_INI_V1_2_BUFFER_SIZE);
	}

	decoded[length] = '\0';
	memcpy(string, decoded, length + 1);
}

void TopgunEngine_v1_2::encodeIniString(byte key, char *string) {
	char encoded[2 * TG_INI_V1_2_BUFFER_SIZE + 1];
	uint16 length = 0;

	const byte *source = (const byte *)string;

	while (*source != 0 && length + 2 < (uint16)sizeof(encoded)) {
		byte shifted = (byte)(*source + key);

		encoded[length] = (char)((shifted & 0x0F) + TG_INI_V1_2_CIPHER_BASE);

		// The shift is arithmetic
		int high = (shifted >> TG_INI_V1_2_CIPHER_SHIFT);
		if ((shifted & 0x80) != 0)
			high -= 16;

		encoded[length + 1] = (char)(high + TG_INI_V1_2_CIPHER_BASE);

		length += 2;
		source++;
	}

	encoded[length] = '\0';
	memcpy(string, encoded, length + 1);
}

uint16 TopgunEngine_v1_2::parseIniInteger(const char *text) {
	while (*text == ' ' || (*text >= '\t' && *text <= '\r'))
		text++;

	bool negative = false;

	if (*text == '-') {
		negative = true;
		text++;
	} else if (*text == '+') {
		text++;
	}

	// The total wraps at 16 bits
	uint16 total = 0;

	while (*text >= '0' && *text <= '9') {
		total = (uint16)(total * 10 + (*text - '0'));
		text++;
	}

	return negative ? (uint16)(0 - total) : total;
}

bool TopgunEngine_v1_2::parseIniHexadecimal(const char *text, uint32 &value) {
	while (*text == ' ' || (*text >= '\t' && *text <= '\r'))
		text++;

	if (text[0] == '0' && (text[1] == 'x' || text[1] == 'X'))
		text += 2;

	bool found = false;
	uint32 total = 0;

	for (;;) {
		char character = *text;
		uint32 digit;

		if (character >= '0' && character <= '9')
			digit = character - '0';
		else if (character >= 'a' && character <= 'f')
			digit = character - 'a' + 10;
		else if (character >= 'A' && character <= 'F')
			digit = character - 'A' + 10;
		else
			break;

		total = (total << 4) | digit;
		found = true;
		text++;
	}

	if (found)
		value = total;

	return found;
}

void TopgunEngine_v1_2::unescapeIniString(char *destination, const char *source) {
	uint16 length = 0;

	while (*source != '\0') {
		char character = *source;
		source++;

		if (character == '\\') {
			character = *source;
			source++;

			if (character == '\0')
				break;

			if (character >= 'A' && character <= 'Z')
				character = (char)(character + ('a' - 'A'));

			if (character == 'n')
				character = '\n';
			else if (character == 'r')
				character = '\r';
			else if (character == 't')
				character = '\t';
		}

		if (length + 1 >= TG_INI_V1_2_BUFFER_SIZE) {
			warning("TopgunEngine_v1_2::unescapeIniString(): the string does not fit in the buffer");
			break;
		}

		destination[length] = character;
		length++;
	}

	destination[length] = '\0';
}

void TopgunEngine_v1_2::escapeIniString(char *destination, uint16 size, const char *source) {
	uint16 length = 0;

	while (*source != '\0') {
		char character = *source;
		source++;

		if (length + 3 > size) {
			warning("TopgunEngine_v1_2::escapeIniString(): the string does not fit in the buffer");
			break;
		}

		if (character == '\\') {
			destination[length] = character;
			length++;
			destination[length] = character;
			length++;
		} else if (character == '\t') {
			destination[length] = '\\';
			destination[length + 1] = 't';
			length += 2;
		} else if (character == '\n') {
			destination[length] = '\\';
			destination[length + 1] = 'n';
			length += 2;
		} else if (character == '\r') {
			destination[length] = '\\';
			destination[length + 1] = 'r';
			length += 2;
		} else if (character == '"') {
			destination[length] = '\\';
			destination[length + 1] = '"';
			length += 2;
		} else {
			destination[length] = character;
			length++;
		}
	}

	destination[length] = '\0';
}

void TopgunEngine_v1_2::iniReadString(byte obfuscation, uint16 stringId, const char *key, const char *section) {
	char *destination = getStringFromId(stringId);
	if (!destination)
		return;

	char buffer[TG_INI_V1_2_BUFFER_SIZE];

	if (getPrivateProfileString(section, key, buffer, TG_INI_V1_2_BUFFER_SIZE) == 0)
		return;

	// The versions from 1.2.7 up to 1.2.8 copy the stored string as it is, although they escape it when writing
	if (_version >= kVersionV1_2_1 && (_version < kVersionV1_2_7 || _version >= kVersionV1_2_9)) {
		char unescaped[TG_INI_V1_2_BUFFER_SIZE];
		unescapeIniString(unescaped, buffer);
		Common::strlcpy(buffer, unescaped, TG_INI_V1_2_BUFFER_SIZE);
	}

	if (obfuscation != 0)
		decodeIniString((byte)key[0], buffer);

	Common::strlcpy(destination, buffer, TG_INI_V1_2_COPY_SIZE);
}

void TopgunEngine_v1_2::iniReadInt(byte obfuscation, uint16 variable, const char *key, const char *section) {
	char buffer[TG_INI_V1_2_BUFFER_SIZE];

	if (getPrivateProfileString(section, key, buffer, TG_INI_V1_2_BUFFER_SIZE) == 0)
		return;

	if (obfuscation != 0)
		decodeIniString((byte)key[0], buffer);

	writeVariable(variable, parseIniInteger(buffer));
}

void TopgunEngine_v1_2::iniWriteStringEncoded(byte obfuscation, uint16 stringId, const char *key, const char *section) {
	const char *value = getStringFromId(stringId);
	if (!value)
		return;

	char escaped[TG_INI_V1_2_3_STRING_SIZE];

	if (_version >= kVersionV1_2_1) {
		escapeIniString(escaped, TG_INI_V1_2_3_STRING_SIZE, value);
		value = escaped;
	}

	char buffer[TG_INI_V1_2_BUFFER_SIZE];

	if (obfuscation != 0) {
		Common::strlcpy(buffer, value, TG_INI_V1_2_COPY_SIZE);
		encodeIniString((byte)key[0], buffer);
		value = buffer;
	}

	// A value made only of spaces removes the key
	if (_version >= kVersionV1_2_6_2) {
		const char *first = value;

		while (*first == ' ')
			first++;

		if (*first == '\0')
			value = nullptr;
	}

	writePrivateProfileString(section, key, value);
}

void TopgunEngine_v1_2::iniWriteInt(byte obfuscation, uint16 variable, const char *key, const char *section) {
	Common::String number = Common::String::format("%d", (int16)readVariable(variable));

	char buffer[TG_INI_V1_2_BUFFER_SIZE];
	Common::strlcpy(buffer, number.c_str(), TG_INI_V1_2_BUFFER_SIZE);

	if (obfuscation != 0)
		encodeIniString((byte)key[0], buffer);

	writePrivateProfileString(section, key, buffer);
}

void TopgunEngine_v1_2::o_iniAccess(const byte *record) {
	const char *section;

	if (READ_LE_UINT16(record + TG_INI_V1_2_RECORD_SECTION) != 0)
		section = getStringFromId(READ_LE_UINT16(record + TG_INI_V1_2_RECORD_SECTION));
	else
		section = !_containerTitle.empty() ? _containerTitle.c_str() : _appTitle.c_str();

	const char *key = getStringFromId(READ_LE_UINT16(record + TG_INI_V1_2_RECORD_KEY));
	if (!section || !key)
		return;

	byte obfuscation = record[TG_INI_V1_2_RECORD_OBFUSCATION];
	uint16 value = READ_LE_UINT16(record + TG_INI_V1_2_RECORD_VALUE);

	if (READ_LE_UINT16(record) == kOpV1_2IniWrite) {
		if (record[TG_INI_V1_2_RECORD_STRING_MODE] != 0)
			iniWriteStringEncoded(obfuscation, value, key, section);
		else
			iniWriteInt(obfuscation, value, key, section);
	} else if (record[TG_INI_V1_2_RECORD_STRING_MODE] == 0) {
		iniReadInt(obfuscation, value, key, section);
	} else if ((record[TG_INI_V1_2_RECORD_VALUE + 1] & 0x80) == 0) {
		iniReadString(obfuscation, value, key, section);
	}
}

void TopgunEngine_v1_2::o_flushIni(const byte *record) {
	const char *section;

	if (READ_LE_UINT16(record + 2) != 0)
		section = getStringFromId(READ_LE_UINT16(record + 2));
	else
		section = !_containerTitle.empty() ? _containerTitle.c_str() : _appTitle.c_str();

	if (section)
		writePrivateProfileString(section, nullptr, nullptr);
}

void TopgunEngine_v1_2::o_backupIni() {
	Common::SaveFileManager *saveFileManager = g_system->getSavefileManager();

	Common::InSaveFile *source = saveFileManager->openForLoading(getIniFileName());
	if (!source)
		return;

	Common::OutSaveFile *destination = saveFileManager->openForSaving(makeUniqueBackupFileName(), false);
	if (destination) {
		copyFileBlocks(source, destination);
		delete destination;
	}

	delete source;
}

void TopgunEngine_v1_2::saveScreenSaverTimes() {
	uint32 elapsed = (getEngineMillis() - _startTick) / 1000;

	char buffer[TG_INI_V1_2_BUFFER_SIZE];

	if (getPrivateProfileString(_appTitle.c_str(), "ScreenSaver2", buffer, TG_INI_V1_2_BUFFER_SIZE) != 0) {
		decodeIniString(TG_INI_V1_2_SCREEN_SAVER_KEY, buffer);

		uint32 previous = 0;
		if (parseIniHexadecimal(buffer, previous))
			elapsed += previous;
	}

	Common::String start = Common::String::format("%x", _startTick / 1000);
	writePrivateProfileString(_appTitle.c_str(), "ScreenSaver1", start.c_str());

	Common::String total = Common::String::format("%x", elapsed);
	Common::strlcpy(buffer, total.c_str(), TG_INI_V1_2_BUFFER_SIZE);
	encodeIniString(TG_INI_V1_2_SCREEN_SAVER_KEY, buffer);
	writePrivateProfileString(_appTitle.c_str(), "ScreenSaver2", buffer);
}

} // End of namespace Topgun

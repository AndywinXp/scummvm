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
#include "common/util.h"

#include "engines/grim/emi/emiengine.h"
#include "engines/grim/emi/core/lconsoleservices.h"
#include "engines/grim/emi/core/lformatparser.h"

namespace Grim {

static const char kEndOfSection[] = "END_OF_SECTION";
static const char kSeparators[] = " =\t:()\n\r";
static const char kCopyrightBanner[] = "Copyright (c) 1998, LucasArts Entertainment Company LLC\r\n";
static const uint kEndOfSectionLength = 14;
static const uint kCopyrightBannerSize = 57;
static const uint kBinaryMagicSize = 4;
static const byte kBinaryMagicByte = 0x1A;
static const int kLineBufferSize = 256;
static const int kMaxTokens = 16;

void KeywordInfo::set(const char *keywordName, int keywordId, int keywordArgMask) {
	name = keywordName;
	id = keywordId;
	argMask = keywordArgMask;
}

LFormatParser::LFormatParser(EmiEngine *engine) {
	_engine = engine;
	_file = nullptr;
	_reading = true;
	_binaryData = false;
	_lineBuffer[0] = 0;
}

LFormatParser::~LFormatParser() {
	while (!_fileStack.empty()) {
		_file = _fileStack.back();
		_fileStack.pop_back();
	}
}

bool LFormatParser::handleKeyword(int id, int numArgs, KeywordArg *args) {
	return false;
}

void LFormatParser::addKeyword(const char *name, int id, int argMask) {
	_keywords[id].set(name, id, argMask);
}

void LFormatParser::setFile(LFile *newFile) {
	_file = newFile;
	_reading = (newFile->_openFlags & 1) != 0;
}

void LFormatParser::writeEndOfSection() {
	_file->writeFormat("%s\n", kEndOfSection);
}

bool LFormatParser::readEndOfSection() {
	if (_file->readLine(_lineBuffer, kLineBufferSize)) {
		while (!tokenize()) {
			if (!_file->readLine(_lineBuffer, kLineBufferSize))
				break;
		}
	}

	if (scumm_strnicmp(_lineBuffer, kEndOfSection, kEndOfSectionLength) == 0)
		return true;

	if (_engine->getConsoleServices())
		_engine->getConsoleServices()->logError("Expected EOF/EOS reading of %s\n", _file->_fileName.c_str());
	else
		warning("LFormatParser::readEndOfSection(): expected EOF/EOS reading of %s", _file->_fileName.c_str());

	return false;
}

void LFormatParser::writeComment(const char *text) {
	_file->writeFormat("// %s\n", text);
}

void LFormatParser::writeVersion(int versionMajor, int versionMinor) {
	_file->writeFormat("VERSION %d.%d\n", versionMajor, versionMinor);
}

void LFormatParser::writeEntry(int id, ...) {
	KeywordInfo *keyword = nullptr;
	Common::HashMap<int, KeywordInfo>::iterator it;

	for (it = _keywords.begin(); it != _keywords.end(); ++it) {
		if (it->_value.id == id) {
			keyword = &it->_value;
			break;
		}
	}

	if (!keyword) {
		warning("LFormatParser::writeEntry(): unknown keyword %d", id);
		return;
	}

	_file->writeFormat("%s:", keyword->name);

	va_list args;
	va_start(args, id);

	uint argMask = keyword->argMask;

	while (argMask) {
		switch (argMask & 3) {
		case 1:
			_file->writeFormat("\t%d", va_arg(args, int));
			break;
		case 2:
			_file->writeFormat("\t%f", va_arg(args, double));
			break;
		case 3:
			_file->writeFormat("\t%s", va_arg(args, char *));
			break;
		default:
			break;
		}

		argMask >>= 2;
	}

	va_end(args);

	_file->writeFormat("\n");
}

bool LFormatParser::readVersion(int versionMajor, int versionMinor) {
	if (_file->readLine(_lineBuffer, kLineBufferSize) || scumm_strnicmp(_lineBuffer, kEndOfSection, kEndOfSectionLength) == 0)
		return true;

	if (_engine->getConsoleServices())
		_engine->getConsoleServices()->logError("Unexpected EOF/EOS reading version of %s\n", _file->_fileName.c_str());
	else
		warning("LFormatParser::readVersion(): unexpected EOF/EOS reading version of %s", _file->_fileName.c_str());

	return false;
}

bool LFormatParser::readEntry(int id, ...) {
	do {
		if (!_file->readLine(_lineBuffer, kLineBufferSize) || scumm_strnicmp(_lineBuffer, kEndOfSection, kEndOfSectionLength) == 0) {
			if (_engine->getConsoleServices())
				_engine->getConsoleServices()->logError("Unexpected EOF/EOS reading token from %s\n", _file->_fileName.c_str());
			else
				warning("LFormatParser::readEntry(): unexpected EOF/EOS reading token from %s", _file->_fileName.c_str());

			return false;
		}

		tokenize();
	} while (!_numTokens);

	KeywordInfo *keyword = findKeyword(_tokens[0]);

	if (!keyword)
		return false;

	va_list args;
	va_start(args, id);

	uint argMask = keyword->argMask;
	int tokenIndex = 1;

	int *intDest;
	float *floatDest;
	char *stringDest;

	while (argMask) {
		switch (argMask & 3) {
		case 1:
			intDest = va_arg(args, int *);

			if (tokenIndex >= _numTokens)
				*intDest = 0;
			else
				*intDest = (int)strtol(_tokens[tokenIndex], nullptr, 0);

			break;
		case 2:
			floatDest = va_arg(args, float *);

			if (tokenIndex >= _numTokens)
				*floatDest = 0.0f;
			else
				*floatDest = (float)strtod(_tokens[tokenIndex], nullptr);

			break;
		case 3:
			stringDest = va_arg(args, char *);

			if (tokenIndex >= _numTokens)
				*stringDest = 0;
			else
				memcpy(stringDest, _tokens[tokenIndex], strlen(_tokens[tokenIndex]) + 1);

			break;
		default:
			break;
		}

		tokenIndex++;
		argMask >>= 2;
	}

	va_end(args);

	return true;
}

bool LFormatParser::parseFile(const char *path, int versionMajor, int versionMinor) {
	_file = openFile(path, true);

	if (!_file)
		return false;

	bool result = parse(versionMajor, versionMinor);

	_engine->getFileServices()->close(_file, true);

	return result;
}

LFile *LFormatParser::openFile(const char *path, bool readMode) {
	_reading = readMode;

	LFile *opened = _engine->getFileServices()->open(path, readMode ? "rb" : "wb");

	if (!opened)
		return nullptr;

	_file = opened;
	_reading = (opened->_openFlags & 1) != 0;

	if (!_reading) {
		opened->writeFormat("%s", kCopyrightBanner);
	} else {
		byte banner[kCopyrightBannerSize];

		if (opened->read(banner, kCopyrightBannerSize) != kCopyrightBannerSize) {
			_engine->getFileServices()->close(opened, true);
			return nullptr;
		}
	}

	byte magic[kBinaryMagicSize];

	if (_reading) {
		if (opened->read(magic, kBinaryMagicSize) != kBinaryMagicSize) {
			_engine->getFileServices()->close(opened, true);
			return nullptr;
		}

		if (magic[0] == kBinaryMagicByte && magic[1] == kBinaryMagicByte && magic[2] == kBinaryMagicByte && magic[3] == kBinaryMagicByte) {
			_binaryData = true;
		} else {
			opened->seek(-(int)kBinaryMagicSize, SEEK_CUR);
			_binaryData = false;
		}
	} else if (_binaryData) {
		memset(magic, kBinaryMagicByte, kBinaryMagicSize);
		opened->write(magic, kBinaryMagicSize);
	}

	opened->_fileName = path;

	return opened;
}

KeywordInfo *LFormatParser::findKeyword(const char *name) {
	Common::HashMap<int, KeywordInfo>::iterator it;

	// The disasm compares the first token, not the argument
	for (it = _keywords.begin(); it != _keywords.end(); ++it) {
		if (it->_value.name && scumm_stricmp(_tokens[0], it->_value.name) == 0)
			return &it->_value;
	}

	return nullptr;
}

bool LFormatParser::parseRemaining(int versionMajor, int versionMinor) {
	return _binaryData || parse(versionMajor, versionMinor);
}

bool LFormatParser::parse(int versionMajor, int versionMinor) {
	KeywordArg args[kMaxTokens];

	if (!_file->readLine(_lineBuffer, kLineBufferSize)) {
		if (_engine->getConsoleServices())
			_engine->getConsoleServices()->logError("Unexpected EOF in file\n");
		else
			warning("LFormatParser::parse(): unexpected EOF in file");

		return false;
	}

	if (!_file->readLine(_lineBuffer, kLineBufferSize))
		return true;

	while (scumm_strnicmp(_lineBuffer, kEndOfSection, kEndOfSectionLength) != 0) {
		tokenize();

		if (_numTokens) {
			if (_tokens[0][0] == '#') {
				static const char kInclude[] = "include";
				bool isInclude = true;

				for (int i = 1; i < 8; i++) {
					if (tolower((byte)_tokens[0][i]) != (byte)kInclude[i - 1]) {
						isInclude = false;
						break;
					}
				}

				if (isInclude) {
					if (_numTokens < 2) {
						warning("LFormatParser::parse(): include without a file name");
					} else {
						bool includeResult;

						_fileStack.push_back(_file);
						_file = openFile(_tokens[1], true);

						if (_file) {
							includeResult = parse(versionMajor, versionMinor);
							_engine->getFileServices()->close(_file, true);
						} else {
							includeResult = false;
						}

						_file = _fileStack.back();
						_fileStack.pop_back();

						if (!includeResult) {
							if (_engine->getConsoleServices())
								_engine->getConsoleServices()->logError("Failed to include <%s>, while parsing file: %s\n", _tokens[1], _file->_fileName.c_str());
							else
								warning("LFormatParser::parse(): failed to include <%s>, while parsing file: %s", _tokens[1], _file->_fileName.c_str());
						}
					}
				}
			} else {
				KeywordInfo *keyword = findKeyword(_tokens[0]);

				if (keyword) {
					uint argMask = keyword->argMask;

					for (int i = 1; i < _numTokens; argMask >>= 2) {
						switch (argMask & 3) {
						case 1:
							args[i - 1].intValue = (int)strtol(_tokens[i], nullptr, 0);
							break;
						case 2:
							args[i - 1].floatValue = (float)strtod(_tokens[i], nullptr);
							break;
						case 3:
							args[i - 1].stringValue = _tokens[i];
							break;
						default:
							_numTokens = i;
							i = 100;
							break;
						}

						i++;
					}

					if (!handleKeyword(keyword->id, _numTokens - 1, args))
						return false;
				}
			}
		}

		if (!_file->readLine(_lineBuffer, kLineBufferSize))
			return true;
	}

	return true;
}

int LFormatParser::tokenize() {
	char *cursor = _lineBuffer;

	_numTokens = 0;

	do {
		while (*cursor && strchr(kSeparators, *cursor))
			cursor++;

		if (!*cursor || (*cursor == '/' && cursor[1] == '/'))
			break;

		char *end;

		if (*cursor == '"') {
			_tokens[_numTokens++] = cursor + 1;
			end = strchr(cursor + 1, '"');

			if (!end)
				return _numTokens;
		} else {
			_tokens[_numTokens++] = cursor;
			end = cursor + 1;

			while (!strchr(kSeparators, *end))
				end++;

			// The disasm keeps scanning after the terminator, let's stop there...
			if (!*end)
				return _numTokens;
		}

		*end = 0;
		cursor = end + 1;
	} while (_numTokens < kMaxTokens);

	return _numTokens;
}

bool LFormatParser::skipCopyrightBanner(LFile *file) {
	byte banner[kCopyrightBannerSize];

	return file->read(banner, kCopyrightBannerSize) == kCopyrightBannerSize;
}

bool LFormatParser::writeCopyrightBanner(LFile *file) {
	file->writeFormat("%s", kCopyrightBanner);

	return true;
}

} // End of namespace Grim

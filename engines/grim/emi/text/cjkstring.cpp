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

#include "engines/grim/emi/text/cjkstring.h"

namespace Grim {

CjkString::CjkString() {
	_chars.push_back(0);
}

CjkString::CjkString(const uint16 *text) {
	assign(text);
}

CjkString::CjkString(const uint16 *text, uint length) {
	_chars.reserve(length + 1);
	for (uint i = 0; i < length; i++)
		_chars.push_back(text[i]);
	_chars.push_back(0);
}

const uint16 *CjkString::c_str() const {
	return &_chars[0];
}

uint CjkString::size() const {
	return _chars.size() - 1;
}

bool CjkString::empty() const {
	return _chars.size() == 1;
}

uint16 CjkString::operator[](uint index) const {
	return _chars[index];
}

void CjkString::assign(const uint16 *text) {
	// Built apart first: text may point into this string
	Common::Array<uint16> chars;
	uint count = length(text);
	chars.reserve(count + 1);
	for (uint i = 0; i < count; i++)
		chars.push_back(text[i]);
	chars.push_back(0);
	_chars = chars;
}

void CjkString::clear() {
	_chars.clear();
	_chars.push_back(0);
}

uint CjkString::length(const uint16 *text) {
	uint count = 0;
	while (text[count] != 0)
		count++;
	return count;
}

int CjkString::compare(const uint16 *a, const uint16 *b) {
	while (*a != 0 && *a == *b) {
		a++;
		b++;
	}
	if (*a == *b)
		return 0;
	return *a < *b ? -1 : 1;
}

const uint16 *CjkString::findChar(const uint16 *text, uint16 character) {
	while (*text != character) {
		if (*text == 0)
			return nullptr;
		text++;
	}
	return text;
}

const uint16 *CjkString::find(const uint16 *text, const char *asciiNeedle) {
	if (*asciiNeedle == 0)
		return text;

	for (; *text != 0; text++) {
		uint i = 0;
		while (asciiNeedle[i] != 0 && text[i] == (uint16)(byte)asciiNeedle[i])
			i++;
		if (asciiNeedle[i] == 0)
			return text;
		// A mismatch on the terminator means the needle cannot fit in what is left
		if (text[i] == 0)
			return nullptr;
	}
	return nullptr;
}

void CjkString::lowerAscii(uint16 *text) {
	for (; *text != 0; text++) {
		if (*text >= 'A' && *text <= 'Z')
			*text += 'a' - 'A';
	}
}

uint CjkString::copy(uint16 *buffer, uint bufferUnits, const uint16 *text) {
	if (bufferUnits == 0)
		return 0;

	uint written = 0;
	while (text[written] != 0 && written < bufferUnits - 1) {
		buffer[written] = text[written];
		written++;
	}
	buffer[written] = 0;
	return written;
}

uint CjkString::widen(const char *narrow, uint16 *buffer, uint bufferUnits) {
	if (bufferUnits == 0)
		return 0;

	uint written = 0;
	while (narrow[written] != 0 && written < bufferUnits - 1) {
		buffer[written] = (uint16)(int)(signed char)narrow[written];
		written++;
	}
	buffer[written] = 0;
	return written;
}

CjkString CjkString::fromNarrow(const char *narrow) {
	CjkString result;
	result._chars.clear();
	for (uint i = 0; narrow[i] != 0; i++)
		result._chars.push_back((uint16)(int)(signed char)narrow[i]);
	result._chars.push_back(0);
	return result;
}

void CjkString::appendUtf8(Common::String *result, uint32 codePoint) {
	if (codePoint < 0x80) {
		*result += (char)codePoint;
	} else if (codePoint < 0x800) {
		*result += (char)(0xC0 | (codePoint >> 6));
		*result += (char)(0x80 | (codePoint & 0x3F));
	} else if (codePoint < 0x10000) {
		*result += (char)(0xE0 | (codePoint >> 12));
		*result += (char)(0x80 | ((codePoint >> 6) & 0x3F));
		*result += (char)(0x80 | (codePoint & 0x3F));
	} else {
		*result += (char)(0xF0 | (codePoint >> 18));
		*result += (char)(0x80 | ((codePoint >> 12) & 0x3F));
		*result += (char)(0x80 | ((codePoint >> 6) & 0x3F));
		*result += (char)(0x80 | (codePoint & 0x3F));
	}
}

Common::String CjkString::toUtf8(const uint16 *text) {
	Common::String result;
	for (uint i = 0; text[i] != 0; i++) {
		uint32 unit = text[i];
		if (unit >= 0xD800 && unit <= 0xDBFF && text[i + 1] >= 0xDC00 && text[i + 1] <= 0xDFFF) {
			unit = 0x10000 + ((unit - 0xD800) << 10) + (text[i + 1] - 0xDC00);
			i++;
		} else if (unit >= 0xD800 && unit <= 0xDFFF) {
			unit = 0xFFFD;
		}
		appendUtf8(&result, unit);
	}
	return result;
}

// Returns 0 only at the terminator. Bad sequences give U+FFFD and
// consume the bytes that were examined, so the caller always advances.
uint32 CjkString::nextUtf8(const char **cursor) {
	const byte *p = (const byte *)*cursor;
	uint32 first = p[0];
	if (first == 0)
		return 0;
	if (first < 0x80) {
		*cursor += 1;
		return first;
	}

	uint extra;
	uint32 codePoint;
	uint32 minimum;
	if ((first & 0xE0) == 0xC0) {
		extra = 1;
		codePoint = first & 0x1F;
		minimum = 0x80;
	} else if ((first & 0xF0) == 0xE0) {
		extra = 2;
		codePoint = first & 0x0F;
		minimum = 0x800;
	} else if ((first & 0xF8) == 0xF0) {
		extra = 3;
		codePoint = first & 0x07;
		minimum = 0x10000;
	} else {
		*cursor += 1;
		return 0xFFFD;
	}

	uint used = 1;
	while (used <= extra && (p[used] & 0xC0) == 0x80) {
		codePoint = (codePoint << 6) | (p[used] & 0x3F);
		used++;
	}
	*cursor += used;

	if (used != extra + 1 || codePoint < minimum || codePoint > 0x10FFFF ||
			(codePoint >= 0xD800 && codePoint <= 0xDFFF))
		return 0xFFFD;
	return codePoint;
}

uint CjkString::fromUtf8(const char *utf8, uint16 *buffer, uint bufferUnits) {
	if (bufferUnits == 0)
		return 0;

	uint written = 0;
	const char *cursor = utf8;
	for (;;) {
		uint32 codePoint = nextUtf8(&cursor);
		if (codePoint == 0)
			break;

		uint needed = codePoint >= 0x10000 ? 2 : 1;
		if (written + needed > bufferUnits - 1)
			break;

		if (needed == 2) {
			codePoint -= 0x10000;
			buffer[written++] = (uint16)(0xD800 + (codePoint >> 10));
			buffer[written++] = (uint16)(0xDC00 + (codePoint & 0x3FF));
		} else {
			buffer[written++] = (uint16)codePoint;
		}
	}
	buffer[written] = 0;
	return written;
}

} // End of namespace Grim

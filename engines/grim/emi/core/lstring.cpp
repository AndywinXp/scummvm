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

#include "engines/grim/emi/core/lstring.h"
#include "engines/grim/emi/core/lmemoryservices.h"

namespace Grim {

// The memory services count a capacity of (length | 31) characters, preceded by a count byte and followed by the
// terminator. The characters are held by Common::String: the services only count the block
static const uint kCapacityMask = 31;
static const uint kBlockOverhead = 2;

LString::LString() : _capacity(0) {
}

LString::LString(const char *str) : Common::String(str), _capacity(0) {
	if (size() > 0)
		copyBlock(size());
}

LString::LString(const char *str, uint32 len) : Common::String(str, len), _capacity(0) {
	if (size() > 0)
		copyBlock(size());
}

LString::LString(const Common::String &str) : Common::String(str), _capacity(0) {
	if (size() > 0)
		copyBlock(size());
}

LString::LString(const LString &other) : Common::String(other), _capacity(0) {
	if (size() > 0)
		copyBlock(size());
}

LString::~LString() {
	releaseBlock();
}

LString &LString::operator=(const LString &other) {
	Common::String::operator=(other);
	grow(size(), true);

	return *this;
}

LString &LString::operator=(const Common::String &str) {
	Common::String::operator=(str);
	grow(size(), true);

	return *this;
}

LString &LString::operator=(const char *str) {
	Common::String::operator=(str);
	grow(size(), true);

	return *this;
}

LString &LString::operator=(char c) {
	Common::String::operator=(c);
	grow(size(), true);

	return *this;
}

LString &LString::operator+=(const Common::String &str) {
	uint oldLength = size();

	Common::String::operator+=(str);
	lengthChanged(oldLength);

	return *this;
}

LString &LString::operator+=(const char *str) {
	uint oldLength = size();

	Common::String::operator+=(str);
	lengthChanged(oldLength);

	return *this;
}

LString &LString::operator+=(char c) {
	uint oldLength = size();

	Common::String::operator+=(c);
	lengthChanged(oldLength);

	return *this;
}

void LString::assign(const Common::String &str) {
	Common::String::assign(str);
	grow(size(), true);
}

void LString::assign(size_t count, char c) {
	Common::String::assign(count, c);
	grow(size(), true);
}

void LString::assign(const char *str) {
	Common::String::assign(str);
	grow(size(), true);
}

void LString::assign(const char *str, size_t count) {
	Common::String::assign(str, count);
	grow(size(), true);
}

void LString::insertChar(char c, uint32 p) {
	uint oldLength = size();

	Common::String::insertChar(c, p);
	lengthChanged(oldLength);
}

void LString::insertString(const char *str, uint32 p) {
	uint oldLength = size();

	Common::String::insertString(str, p);
	lengthChanged(oldLength);
}

void LString::insertString(const Common::String &str, uint32 p) {
	uint oldLength = size();

	Common::String::insertString(str, p);
	lengthChanged(oldLength);
}

void LString::replace(uint32 pos, uint32 count, const Common::String &str) {
	uint oldLength = size();

	Common::String::replace(pos, count, str);
	lengthChanged(oldLength);
}

void LString::replace(uint32 pos, uint32 count, const char *str) {
	uint oldLength = size();

	Common::String::replace(pos, count, str);
	lengthChanged(oldLength);
}

void LString::wordWrap(const uint32 maxLength) {
	uint oldLength = size();

	Common::String::wordWrap(maxLength);
	lengthChanged(oldLength);
}

void LString::trim() {
	uint oldLength = size();

	Common::String::trim();
	lengthChanged(oldLength);
}

void LString::releaseBlock() {
	LMemoryServices *memory = LMemoryServices::getCurrent();

	if (_capacity > 0 && memory)
		memory->uncountBlock((_capacity + kBlockOverhead) * sizeof(byte));

	_capacity = 0;
}

// The new block is counted before the old one is released
void LString::copyBlock(uint length) {
	uint capacity = length | kCapacityMask;
	LMemoryServices *memory = LMemoryServices::getCurrent();

	if (memory)
		memory->countBlock((capacity + kBlockOverhead) * sizeof(byte));

	releaseBlock();

	_capacity = memory ? capacity : 0;
}

// Makes room for the length. The block of a string that is trimmed is reused only while it is small enough: the old
// block is released before the new one is counted
void LString::grow(uint length, bool trim) {
	if (length == 0) {
		if (trim)
			releaseBlock();

		return;
	}

	if (!trim) {
		if (_capacity >= length)
			return;

		copyBlock(length);

		return;
	}

	if (_capacity <= kCapacityMask && _capacity >= length)
		return;

	releaseBlock();
	copyBlock(length);
}

// The block only grows with the string: removing characters keeps it
void LString::lengthChanged(uint oldLength) {
	if (size() > oldLength)
		grow(size(), false);
}

LString &LStringUtil::getExtension(LString &string, LString &extension) {
	size_t position = string.rfind('.');

	if (position == LString::npos)
		return string;

	extension = string.substr(position + 1);

	return string;
}

LString &LStringUtil::stringToLower(LString &string) {
	string.toLowercase();

	return string;
}

} // End of namespace Grim

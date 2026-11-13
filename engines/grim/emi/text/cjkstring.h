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

#ifndef GRIM_EMI_CJKSTRING_H
#define GRIM_EMI_CJKSTRING_H

#include "common/array.h"
#include "common/scummsys.h"
#include "common/str.h"

namespace Grim {

// Text of the Korean and Chinese versions: UTF-16 units, BMP only.
class CjkString {
public:
	CjkString();
	explicit CjkString(const uint16 *text);
	CjkString(const uint16 *text, uint length);

	const uint16 *c_str() const;	// always NUL terminated
	uint size() const;				// units, without the terminator
	bool empty() const;
	uint16 operator[](uint index) const;	// index == size() gives the terminator
	void assign(const uint16 *text);
	void clear();

	// Every unit up to the terminating 0, like wcslen.
	static uint length(const uint16 *text);

	// Unsigned unit compare, like wcscmp. Returns -1, 0 or 1.
	static int compare(const uint16 *a, const uint16 *b);

	// Like wcschr: pointer to the first unit equal to character, or null.
	// Searching for 0 returns the terminator.
	static const uint16 *findChar(const uint16 *text, uint16 character);

	// Like wcsstr with an ASCII needle: case sensitive, text must be NUL terminated.
	static const uint16 *find(const uint16 *text, const char *asciiNeedle);

	// Lower-cases A-Z only, up to the terminator, like _wcslwr in the C locale.
	static void lowerAscii(uint16 *text);

	// Copies at most bufferUnits - 1 units and always terminates.
	// Returns the units copied, without the terminator.
	static uint copy(uint16 *buffer, uint bufferUnits, const uint16 *text);

	// Each byte becomes a unit by sign extension, no code page: 0x80..0xFF give 0xFF80..0xFFFF.
	// Copies at most bufferUnits - 1 units and always terminates, so a text that did
	// not fit is the case where narrow[returned value] != 0. Returns the units written.
	static uint widen(const char *narrow, uint16 *buffer, uint bufferUnits);

	// Same widening into a new string, with no size limit.
	static CjkString fromNarrow(const char *narrow);

	// UTF-16 to UTF-8. Surrogate pairs are joined, a lone surrogate becomes U+FFFD.
	static Common::String toUtf8(const uint16 *text);

	// UTF-8 to UTF-16, bounded like copy(). Invalid input becomes U+FFFD,
	// characters above U+FFFF become surrogate pairs. Returns the units written.
	static uint fromUtf8(const char *utf8, uint16 *buffer, uint bufferUnits);

private:
	static void appendUtf8(Common::String *result, uint32 codePoint);
	static uint32 nextUtf8(const char **cursor);

	Common::Array<uint16> _chars;	// includes the terminating 0
};

} // End of namespace Grim

#endif

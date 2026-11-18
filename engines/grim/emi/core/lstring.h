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

#ifndef GRIM_EMI_LSTRING_H
#define GRIM_EMI_LSTRING_H

#include "common/hash-str.h"
#include "common/scummsys.h"
#include "common/str.h"

namespace Grim {

// The characters of the string are the ones of Common::String. The memory services count what the string of the game
// takes: a block of (length | 31) + 2 bytes, a count byte followed by the characters and a terminator. Each string
// counts its own block, the copies do not share it
class LString : public Common::String {
public:
	LString();
	LString(const char *str);
	LString(const char *str, uint32 len);
	LString(const Common::String &str);
	LString(const LString &other);
	~LString();

	LString &operator=(const LString &other);
	LString &operator=(const Common::String &str);
	LString &operator=(const char *str);
	LString &operator=(char c);
	LString &operator+=(const Common::String &str);
	LString &operator+=(const char *str);
	LString &operator+=(char c);

	void assign(const Common::String &str);
	void assign(size_t count, char c);
	void assign(const char *str);
	void assign(const char *str, size_t count);
	void insertChar(char c, uint32 p);
	void insertString(const char *str, uint32 p);
	void insertString(const Common::String &str, uint32 p);
	void replace(uint32 pos, uint32 count, const Common::String &str);
	void replace(uint32 pos, uint32 count, const char *str);
	void wordWrap(const uint32 maxLength);
	void trim();

private:
	void releaseBlock();
	void copyBlock(uint length);
	void grow(uint length, bool trim);
	void lengthChanged(uint oldLength);

	uint _capacity;
};

class LStringUtil {
public:
	static LString &getExtension(LString &string, LString &extension);
	static LString &stringToLower(LString &string);
};

} // End of namespace Grim

namespace Common {

template<>
struct Hash<Grim::LString> {
	uint operator()(const Grim::LString &s) const {
		return s.hash();
	}
};

} // End of namespace Common

#endif

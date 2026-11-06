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

#ifndef GRIM_WUREGISTRY_H
#define GRIM_WUREGISTRY_H

#include "common/array.h"
#include "common/scummsys.h"
#include "common/str.h"

namespace Grim {

struct WuRegistryValue {
	Common::String name;
	uint type;
	bool bInConfMan;
	Common::Array<byte> data;
};

class WuRegistry {
public:
	enum ValueType {
		kValueString = 1,
		kValueBinary = 3
	};

	WuRegistry();
	~WuRegistry();

	int startup(const char *version);
	int loadIndex();
	void shutdown();
	void freeIndex();
	int saveInt(const char *valueName, int value);
	int addIndex(const char *valueName);
	int findIndex(const char *valueName);
	int saveFloat(const char *valueName, float value);
	int getInt(const char *valueName, int defaultValue);
	float getFloat(const char *valueName, float defaultValue);
	int saveBinary(const char *valueName, const byte *pData, uint size);
	int getBinary(const char *valueName, byte *pData, uint size);
	int saveStr(const char *valueName, const char *pData);
	int getStr(const char *valueName, char *pData, uint size, const char *pDefault);
	uint queryType(const char *valueName);

	static int compareNames(const void *pA, const void *pB);

private:
	int findValue(const char *valueName) const;
	bool claimKey(const char *valueName, uint type, Common::String *pKey);
	bool findConfKey(const char *valueName, Common::String *pKey) const;
	void setValue(const char *valueName, uint type, const byte *pData, uint size);
	bool queryValue(const char *valueName, uint *pType, byte *pData, uint *pSize) const;

	int _bStarted;
	Common::Array<WuRegistryValue> _values;
	char **_aValueNames;
	uint _numValues;
};

} // End of namespace Grim

#endif

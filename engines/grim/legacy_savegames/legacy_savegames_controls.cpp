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

#include "common/array.h"
#include "common/debug.h"
#include "common/hash-str.h"
#include "common/hashmap.h"
#include "common/str.h"
#include "common/textconsole.h"
#include "common/util.h"

#include "engines/grim/legacy_savegames/legacy_savegames_controls.h"
#include "engines/grim/sputcontrol.h"

#include "engines/grim/lua/lapi.h"
#include "engines/grim/lua/lobject.h"
#include "engines/grim/lua/lua.h"

namespace Grim {

// A global of _controls.lua that holds system.controls.<control>
struct KeyGlobal {
	const char *name;
	const char *control;
};

// _controls.lua lines 4 to 82, the only globals made from system.controls
static const KeyGlobal kKeyGlobals[] = {
	{ "RETURNKEY", "KEY_RETURN" },
	{ "SPACEKEY", "KEY_SPACE" },
	{ "PERIODKEY", "KEY_PERIOD" },
	{ "COMMAKEY", "KEY_COMMA" },
	{ "HOMEKEY", "KEY_HOME" },
	{ "ESCAPEKEY", "KEY_ESCAPE" },
	{ "EQUALSKEY", "KEY_EQUALS" },
	{ "MINUSKEY", "KEY_MINUS" },
	{ "LCONTROLKEY", "KEY_LCONTROL" },
	{ "RCONTROLKEY", "KEY_RCONTROL" },
	{ "LSHIFTKEY", "KEY_LSHIFT" },
	{ "RSHIFTKEY", "KEY_RSHIFT" },
	{ "LALTKEY", "KEY_LMENU" },
	{ "RALTKEY", "KEY_RMENU" },
	{ "BSKEY", "KEY_BACK" },
	{ "AKEY", "KEY_A" },
	{ "BKEY", "KEY_B" },
	{ "CKEY", "KEY_C" },
	{ "DKEY", "KEY_D" },
	{ "EKEY", "KEY_E" },
	{ "FKEY", "KEY_F" },
	{ "GKEY", "KEY_G" },
	{ "HKEY", "KEY_H" },
	{ "IKEY", "KEY_I" },
	{ "JKEY", "KEY_J" },
	{ "KKEY", "KEY_K" },
	{ "LKEY", "KEY_L" },
	{ "MKEY", "KEY_M" },
	{ "NKEY", "KEY_N" },
	{ "OKEY", "KEY_O" },
	{ "PKEY", "KEY_P" },
	{ "QKEY", "KEY_Q" },
	{ "RKEY", "KEY_R" },
	{ "SKEY", "KEY_S" },
	{ "TKEY", "KEY_T" },
	{ "UKEY", "KEY_U" },
	{ "VKEY", "KEY_V" },
	{ "WKEY", "KEY_W" },
	{ "XKEY", "KEY_X" },
	{ "YKEY", "KEY_Y" },
	{ "ZKEY", "KEY_Z" },
	{ "NUMPAD0KEY", "KEY_NUMPAD0" },
	{ "NUMPAD1KEY", "KEY_NUMPAD1" },
	{ "NUMPAD2KEY", "KEY_NUMPAD2" },
	{ "NUMPAD3KEY", "KEY_NUMPAD3" },
	{ "NUMPAD4KEY", "KEY_NUMPAD4" },
	{ "NUMPAD5KEY", "KEY_NUMPAD5" },
	{ "NUMPAD6KEY", "KEY_NUMPAD6" },
	{ "NUMPAD7KEY", "KEY_NUMPAD7" },
	{ "NUMPAD8KEY", "KEY_NUMPAD8" },
	{ "NUMPAD9KEY", "KEY_NUMPAD9" },
	{ "ADDKEY", "KEY_ADD" },
	{ "NUMPADENTERKEY", "KEY_NUMPADENTER" },
	{ "NUMPADDEL", "KEY_DECIMAL" },
	{ "UPKEY", "KEY_UP" },
	{ "DOWNKEY", "KEY_DOWN" },
	{ "RIGHTKEY", "KEY_RIGHT" },
	{ "LEFTKEY", "KEY_LEFT" },
	{ "F1KEY", "KEY_F1" },
	{ "F2KEY", "KEY_F2" },
	{ "F3KEY", "KEY_F3" },
	{ "F4KEY", "KEY_F4" },
	{ "F5KEY", "KEY_F5" },
	{ "F6KEY", "KEY_F6" },
	{ "F7KEY", "KEY_F7" },
	{ "F8KEY", "KEY_F8" },
	{ "F9KEY", "KEY_F9" },
	{ "F10KEY", "KEY_F10" },
	{ "F11KEY", "KEY_F11" },
	{ "F12KEY", "KEY_F12" },
	{ "KEY1", "KEY_1" },
	{ "KEY2", "KEY_2" },
	{ "KEY3", "KEY_3" },
	{ "KEY4", "KEY_4" },
	{ "KEY5", "KEY_5" },
	{ "KEY6", "KEY_6" },
	{ "KEY7", "KEY_7" },
	{ "KEY8", "KEY_8" },
	{ "KEY9", "KEY_9" }
};

// _local.lua: copies of the globals above (HOTKEY_EXAMINE = EKEY ...)
static const char *const kHotKeyGlobals[] = {
	"HOTKEY_EXAMINE", "HOTKEY_PICKUP", "HOTKEY_USE", "HOTKEY_INVENTORY",
	"ALTHOTKEY_EXAMINE", "ALTHOTKEY_PICKUP", "ALTHOTKEY_USE", "ALTHOTKEY_INVENTORY",
	"HOTKEY_MOVEMENT_MODE", "HOTKEY_TEXT_MODE", "HOTKEY_JOYSTICK_MODE", "HOTKEY_BRIGHTNESS",
	"HOTKEY_EXIT", "HOTKEY_QUIT"
};

struct JoystickGlobal {
	const char *name;
	int32 value;
};

static const int kNumJoystickGlobals = 4;

// _controls.lua lines 171 to 174, the values the new engine leaves in place; the legacy engine set
// them to its own key numbers at boot, and its savegames hold those
static const JoystickGlobal kJoystickGlobals[kNumJoystickGlobals] = {
	{ "JOYSTICK_X_LEFT", 256 },
	{ "JOYSTICK_X_RIGHT", 257 },
	{ "JOYSTICK_Y_UP", 258 },
	{ "JOYSTICK_Y_DOWN", 259 }
};

static const int32 kNoCode = -1;
static const int kMaxListedCodes = 20;

// A table entry with a number key and a number value
struct NumberEntry {
	int32 key;
	float value;
};

// The control numbers are whole and not negative
static bool toControlCode(float number, int32 *codePointer) {
	if (!(number >= 0.0f && number <= 1000000.0f))
		return false;

	int32 code = (int32)number;

	if ((float)code != number)
		return false;

	*codePointer = code;

	return true;
}

static bool isLiveNode(const Node *node) {
	return ttype(&node->ref) != LUA_T_NIL && ttype(&node->val) != LUA_T_NIL;
}

class ControlsRemapper {
public:
	ControlsRemapper(int systemRef, const ControlSymbol *symbols, int numSymbols);

	Common::String remap();

private:
	bool remapAll();
	bool readOldControls(lua_Object systemTable);
	void readOldJoysticks();
	void buildValueMap();
	void addNewCode(int32 oldCode, int newCode);
	void replaceControls(lua_Object systemTable);
	void remapGlobals();
	int32 findCodeByName(const KeyGlobal &global, int32 oldCode);
	void remapControlMap();
	void remapKeyTable(lua_Object table, const char *what, bool isMappedIds);
	bool mapJoystickValue(float oldValue, float *newValue) const;
	bool readGlobalCode(const char *name, int32 *code);
	void setGlobalCode(const char *name, int32 code);
	void setTableNumber(lua_Object table, int32 key, float value);
	void removeTableKey(lua_Object table, int32 key);
	void noteUnknownCode(int32 code);
	void warnAboutLeftovers();
	Common::String makeSummary() const;

	int _systemRef;
	const ControlSymbol *_symbols;
	int _numSymbols;

	Common::Array<Common::String> _oldNames;
	Common::HashMap<Common::String, int32> _oldCodeByName;
	Common::HashMap<Common::String, int> _newCodeByName;
	// An old number can have several names and the joystick globals, so it can become several new numbers
	Common::HashMap<int32, Common::Array<int> > _newCodesByOld;
	int32 _oldJoystick[kNumJoystickGlobals];
	bool _hasOldJoystick[kNumJoystickGlobals];

	Common::String _droppedNames;
	int _numAbsentNames;
	int _numDroppedNames;
	Common::Array<int32> _unknownCodes;
	int _numGlobalsByName;
	int _numHotKeys;
	int _numJoystick;
	int _numGlobalsSkipped;
	int _numTables;
	int _numKeysMoved;
	int _numKeysDropped;
	int _numKeysLeft;
	int _numValuesLeft;
};

ControlsRemapper::ControlsRemapper(int systemRef, const ControlSymbol *symbols, int numSymbols) : _systemRef(systemRef),
		_symbols(symbols), _numSymbols(numSymbols), _numAbsentNames(0), _numDroppedNames(0), _numGlobalsByName(0), _numHotKeys(0), _numJoystick(0),
		_numGlobalsSkipped(0), _numTables(0), _numKeysMoved(0), _numKeysDropped(0), _numKeysLeft(0), _numValuesLeft(0) {
	for (int i = 0; i < kNumJoystickGlobals; i++) {
		_oldJoystick[i] = kNoCode;
		_hasOldJoystick[i] = false;
	}

	for (int i = 0; i < _numSymbols; i++)
		_newCodeByName.setVal(_symbols[i].name, i);
}

Common::String ControlsRemapper::remap() {
	lua_beginblock();
	bool isDone = remapAll();
	lua_endblock();

	if (!isDone)
		return "the control numbers were not changed";

	warnAboutLeftovers();

	return makeSummary();
}

bool ControlsRemapper::remapAll() {
	lua_Object systemTable = lua_getref(_systemRef);

	if (!lua_istable(systemTable)) {
		warning("legacyRemapControls(): the Lua ref %d is not the system table", _systemRef);
		return false;
	}

	if (!readOldControls(systemTable))
		return false;

	readOldJoysticks();
	buildValueMap();
	replaceControls(systemTable);
	remapGlobals();
	remapControlMap();

	lua_Object mappedIds = lua_rawgetglobal("AxisHandlerMappedIds");

	if (lua_istable(mappedIds))
		remapKeyTable(mappedIds, "AxisHandlerMappedIds", true);
	else
		warning("legacyRemapControls(): the savegame has no AxisHandlerMappedIds");

	// It only exists once a dialog has run
	lua_Object lastValues = lua_rawgetglobal("AxisHandlerLastValues");

	if (lua_istable(lastValues))
		remapKeyTable(lastValues, "AxisHandlerLastValues", false);

	return true;
}

bool ControlsRemapper::readOldControls(lua_Object systemTable) {
	lua_pushobject(systemTable);
	lua_pushstring("controls");
	lua_Object controls = lua_rawgettable();

	if (!lua_istable(controls)) {
		warning("legacyRemapControls(): the system table of the savegame has no controls table");
		return false;
	}

	Hash *hash = avalue(luaA_Address(controls));

	for (int32 i = 0; i < hash->nhash; i++) {
		const Node *node = &hash->node[i];
		int32 code;

		if (!isLiveNode(node))
			continue;

		if (ttype(&node->ref) != LUA_T_STRING || ttype(&node->val) != LUA_T_NUMBER || !toControlCode(nvalue(&node->val), &code)) {
			warning("legacyRemapControls(): an entry of the old controls table is not a name with a control number");
			continue;
		}

		Common::String name(svalue(&node->ref));

		_oldNames.push_back(name);
		_oldCodeByName.setVal(name, code);
	}

	if (_oldNames.empty()) {
		warning("legacyRemapControls(): the old controls table is empty");
		return false;
	}

	return true;
}

void ControlsRemapper::readOldJoysticks() {
	for (int i = 0; i < kNumJoystickGlobals; i++)
		_hasOldJoystick[i] = readGlobalCode(kJoystickGlobals[i].name, &_oldJoystick[i]);
}

void ControlsRemapper::addNewCode(int32 oldCode, int newCode) {
	Common::Array<int> &newCodes = _newCodesByOld[oldCode];

	for (uint i = 0; i < newCodes.size(); i++) {
		if (newCodes[i] == newCode)
			return;
	}

	newCodes.push_back(newCode);
}

void ControlsRemapper::buildValueMap() {
	Common::Array<Common::String> absentNames;

	for (uint i = 0; i < _oldNames.size(); i++) {
		const Common::String &name = _oldNames[i];

		if (_newCodeByName.contains(name))
			addNewCode(_oldCodeByName.getVal(name), _newCodeByName.getVal(name));
		else
			absentNames.push_back(name);
	}

	// The legacy engine gave these the numbers of the hat directions, so the keys of one are also the keys of the other
	for (int i = 0; i < kNumJoystickGlobals; i++) {
		if (_hasOldJoystick[i])
			addNewCode(_oldJoystick[i], kJoystickGlobals[i].value);
	}

	_numAbsentNames = absentNames.size();

	// A name that is only another name of a known number (KEY_NP0, KEY_JOY1_DPAD_U) loses nothing
	for (uint i = 0; i < absentNames.size(); i++) {
		if (!_newCodesByOld.contains(_oldCodeByName.getVal(absentNames[i]))) {
			_droppedNames += " " + absentNames[i];
			_numDroppedNames++;
		}
	}
}

void ControlsRemapper::replaceControls(lua_Object systemTable) {
	lua_Object controls = lua_createtable();

	lua_pushobject(systemTable);
	lua_pushstring("controls");
	lua_pushobject(controls);
	lua_rawsettable();

	for (int i = 0; i < _numSymbols; i++) {
		lua_pushobject(controls);
		lua_pushstring(_symbols[i].name);
		lua_pushnumber((float)i);
		lua_rawsettable();
	}
}

void ControlsRemapper::remapGlobals() {
	const int numKeys = ARRAYSIZE(kKeyGlobals);
	Common::Array<int32> oldCodes;
	Common::Array<int32> newCodes;

	for (int i = 0; i < numKeys; i++) {
		int32 oldCode = kNoCode;
		int32 newCode = kNoCode;

		// Absent in the demo scripts, which have fewer keys
		if (readGlobalCode(kKeyGlobals[i].name, &oldCode)) {
			newCode = findCodeByName(kKeyGlobals[i], oldCode);

			if (newCode != kNoCode) {
				setGlobalCode(kKeyGlobals[i].name, newCode);
				_numGlobalsByName++;
			} else {
				_numGlobalsSkipped++;
			}
		}

		oldCodes.push_back(oldCode);
		newCodes.push_back(newCode);
	}

	const int numHotKeys = ARRAYSIZE(kHotKeyGlobals);

	for (int i = 0; i < numHotKeys; i++) {
		int32 oldCode = kNoCode;
		int32 newCode = kNoCode;

		if (!readGlobalCode(kHotKeyGlobals[i], &oldCode))
			continue;

		for (int j = 0; j < numKeys && newCode == kNoCode; j++) {
			if (oldCodes[j] == oldCode)
				newCode = newCodes[j];
		}

		// A language that sets the hot key from system.controls directly
		if (newCode == kNoCode && _newCodesByOld.contains(oldCode))
			newCode = _newCodesByOld.getVal(oldCode)[0];

		if (newCode == kNoCode) {
			warning("legacyRemapControls(): the number %d of %s is not a known key, it is left as it is", oldCode, kHotKeyGlobals[i]);
			_numGlobalsSkipped++;
			continue;
		}

		setGlobalCode(kHotKeyGlobals[i], newCode);
		_numHotKeys++;
	}

	for (int i = 0; i < kNumJoystickGlobals; i++) {
		if (!_hasOldJoystick[i])
			continue;

		setGlobalCode(kJoystickGlobals[i].name, kJoystickGlobals[i].value);
		_numJoystick++;
	}
}

int32 ControlsRemapper::findCodeByName(const KeyGlobal &global, int32 oldCode) {
	if (!_oldCodeByName.contains(global.control) || _oldCodeByName.getVal(global.control) != oldCode) {
		warning("legacyRemapControls(): %s = %d is not the old value of %s, it is left as it is", global.name, oldCode, global.control);
		return kNoCode;
	}

	if (!_newCodeByName.contains(global.control)) {
		warning("legacyRemapControls(): %s is not in the new controls table, %s is left as it is", global.control, global.name);
		return kNoCode;
	}

	return _newCodeByName.getVal(global.control);
}

void ControlsRemapper::remapControlMap() {
	lua_Object controlMap = lua_rawgetglobal("control_map");

	if (!lua_istable(controlMap)) {
		warning("legacyRemapControls(): the savegame has no control_map");
		return;
	}

	Hash *hash = avalue(luaA_Address(controlMap));

	// Nothing is written into control_map itself, so the nodes stay where they are
	for (int32 i = 0; i < hash->nhash; i++) {
		Node *node = &hash->node[i];

		if (!isLiveNode(node) || ttype(&node->val) != LUA_T_ARRAY)
			continue;

		const char *name = (ttype(&node->ref) == LUA_T_STRING) ? svalue(&node->ref) : "control_map";

		luaA_pushobject(&node->val);
		remapKeyTable(lua_pop(), name, false);
	}
}

// The keys of the table, and the values too for the ids of the axis handlers, get their new numbers
void ControlsRemapper::remapKeyTable(lua_Object table, const char *what, bool isMappedIds) {
	Hash *hash = avalue(luaA_Address(table));
	Common::Array<NumberEntry> entries;

	for (int32 i = 0; i < hash->nhash; i++) {
		const Node *node = &hash->node[i];
		NumberEntry entry;

		if (!isLiveNode(node))
			continue;

		if (ttype(&node->ref) != LUA_T_NUMBER || ttype(&node->val) != LUA_T_NUMBER || !toControlCode(nvalue(&node->ref), &entry.key)) {
			_numKeysLeft++;
			continue;
		}

		entry.value = nvalue(&node->val);
		entries.push_back(entry);
	}

	_numTables++;

	// All old keys go first, a new number can be the old number of another key
	for (uint i = 0; i < entries.size(); i++)
		removeTableKey(table, entries[i].key);

	for (uint i = 0; i < entries.size(); i++) {
		const NumberEntry &entry = entries[i];
		float value = entry.value;

		if (!_newCodesByOld.contains(entry.key)) {
			noteUnknownCode(entry.key);
			_numKeysDropped++;
			continue;
		}

		if (isMappedIds && !mapJoystickValue(value, &value)) {
			int32 valueCode;

			if (toControlCode(value, &valueCode))
				noteUnknownCode(valueCode);

			_numValuesLeft++;
		}

		const Common::Array<int> &newCodes = _newCodesByOld.getVal(entry.key);

		for (uint j = 0; j < newCodes.size(); j++) {
			setTableNumber(table, newCodes[j], value);
			_numKeysMoved++;
		}
	}

	debug(2, "legacyRemapControls(): %s has %u entries", what, entries.size());
}

// The ids of the axis handlers are the joystick globals
bool ControlsRemapper::mapJoystickValue(float oldValue, float *newValue) const {
	int32 oldCode;

	if (!toControlCode(oldValue, &oldCode))
		return false;

	for (int i = 0; i < kNumJoystickGlobals; i++) {
		if (_hasOldJoystick[i] && _oldJoystick[i] == oldCode) {
			*newValue = (float)kJoystickGlobals[i].value;
			return true;
		}
	}

	return false;
}

bool ControlsRemapper::readGlobalCode(const char *name, int32 *code) {
	TObject *object = luaA_Address(lua_rawgetglobal(name));

	if (ttype(object) == LUA_T_NIL)
		return false;

	if (ttype(object) != LUA_T_NUMBER || !toControlCode(nvalue(object), code)) {
		warning("legacyRemapControls(): the global %s is not a control number, it is left as it is", name);
		_numGlobalsSkipped++;
		return false;
	}

	return true;
}

void ControlsRemapper::setGlobalCode(const char *name, int32 code) {
	lua_pushnumber((float)code);
	lua_rawsetglobal(name);
}

void ControlsRemapper::setTableNumber(lua_Object table, int32 key, float value) {
	lua_pushobject(table);
	lua_pushnumber((float)key);
	lua_pushnumber(value);
	lua_rawsettable();
}

void ControlsRemapper::removeTableKey(lua_Object table, int32 key) {
	lua_pushobject(table);
	lua_pushnumber((float)key);
	lua_pushnil();
	lua_rawsettable();
}

void ControlsRemapper::noteUnknownCode(int32 code) {
	if ((int)_unknownCodes.size() >= kMaxListedCodes)
		return;

	for (uint i = 0; i < _unknownCodes.size(); i++) {
		if (_unknownCodes[i] == code)
			return;
	}

	_unknownCodes.push_back(code);
}

void ControlsRemapper::warnAboutLeftovers() {
	if (_numDroppedNames > 0)
		warning("legacyRemapControls(): %d old control numbers have no name in the new table:%s", _numDroppedNames, _droppedNames.c_str());

	if (!_unknownCodes.empty()) {
		Common::String codes;

		for (uint i = 0; i < _unknownCodes.size(); i++)
			codes += Common::String::format(" %d", _unknownCodes[i]);

		warning("legacyRemapControls(): these numbers have no new control, the entries are dropped or kept as they are:%s", codes.c_str());
	}
}

Common::String ControlsRemapper::makeSummary() const {
	return Common::String::format("%u old control names, %d not in the new table, %d of them without a number there; globals: %d by name, %d hot keys, %d joystick, %d left; "
		"%d key tables: %d keys moved, %d dropped, %d left, %d values left",
		_oldNames.size(), _numAbsentNames, _numDroppedNames, _numGlobalsByName, _numHotKeys, _numJoystick, _numGlobalsSkipped,
		_numTables, _numKeysMoved, _numKeysDropped, _numKeysLeft, _numValuesLeft);
}

Common::String legacyRemapControls(int systemRef, const ControlSymbol *symbols, int numSymbols) {
	ControlsRemapper remapper(systemRef, symbols, numSymbols);

	return remapper.remap();
}

} // End of namespace Grim

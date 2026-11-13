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

#include "common/str.h"

#include "engines/grim/emi/costume/choreinstance.h"
#include "engines/grim/emi/costume/choreset.h"
#include "engines/grim/emi/emiengine.h"
#include "engines/grim/emi/resources/lresourcehandle.h"
#include "engines/grim/emi/lua_v2_opcodes/lua_v2.h"

namespace Grim {

void Lua_V2::IsChoreValid() {
	lua_Object idObj = lua_getparam(1);

	if (!lua_isnumber(idObj)) {
		logDemo(kConsoleWarning, "IsChoreValid: the first parameter must be a chore id.\n");

		return;
	}

	uint instanceId = (uint)luaFtol(lua_getnumber(idObj));

	if (ChoreInstance::isInstanceValid(_engine, instanceId))
		lua_pushnumber(1.0f);
	else
		lua_pushnumber(0.0f);
}

void Lua_V2::IsChorePlaying() {
	lua_Object idObj = lua_getparam(1);

	if (!lua_isnumber(idObj)) {
		logDemo(kConsoleWarning, "IsChorePlaying: the first parameter must be a chore id.\n");

		return;
	}

	uint instanceId = (uint)luaFtol(lua_getnumber(idObj));

	if (ChoreInstance::isInstanceValid(_engine, instanceId) && ChoreInstance::getInstance(_engine, instanceId)->isPlaying())
		lua_pushnumber(1.0f);
	else
		lua_pushnil();
}

void Lua_V2::IsChoreLooping() {
	lua_Object idObj = lua_getparam(1);

	if (!lua_isnumber(idObj)) {
		logDemo(kConsoleWarning, "IsChoreLooping: the first parameter must be a chore id.\n");

		return;
	}

	uint instanceId = (uint)luaFtol(lua_getnumber(idObj));

	if (ChoreInstance::isInstanceValid(_engine, instanceId) && ChoreInstance::getInstance(_engine, instanceId)->isLooping())
		lua_pushnumber(1.0f);
	else
		lua_pushnil();
}

void Lua_V2::SetChoreLooping() {
	lua_Object idObj = lua_getparam(1);
	lua_Object loopObj = lua_getparam(2);

	if (!lua_isnumber(idObj)) {
		logDemo(kConsoleWarning, "SetChoreLooping: the first parameter must be a chore id.\n");

		return;
	}

	uint instanceId = (uint)luaFtol(lua_getnumber(idObj));
	bool looping = lua_isnil(loopObj) == 0;

	if (ChoreInstance::isInstanceValid(_engine, instanceId))
		ChoreInstance::getInstance(_engine, instanceId)->setLooping(looping);
}

void Lua_V2::PlayChore() {
	lua_Object idObj = lua_getparam(1);

	if (!lua_isnumber(idObj)) {
		logDemo(kConsoleWarning, "PlayChore: the first parameter must be a chore id.\n");

		return;
	}

	uint instanceId = (uint)luaFtol(lua_getnumber(idObj));

	if (ChoreInstance::isInstanceValid(_engine, instanceId))
		ChoreInstance::getInstance(_engine, instanceId)->setPlaying();
}

void Lua_V2::FadeInChore() {
	lua_Object idObj = lua_getparam(1);
	lua_Object secondsObj = lua_getparam(2);

	logDemo(kConsoleWarning, "FadeInChore: this function is obsolete, stop using it.\n");

	if (!lua_isnumber(idObj)) {
		logDemo(kConsoleWarning, "FadeInChore: the first parameter must be a chore id.\n");

		return;
	}

	if (!lua_isnumber(secondsObj)) {
		logDemo(kConsoleWarning, "FadeInChore: the second parameter must be a number of seconds.\n");

		return;
	}

	uint instanceId = (uint)luaFtol(lua_getnumber(idObj));
	float seconds = lua_getnumber(secondsObj);

	if (ChoreInstance::isInstanceValid(_engine, instanceId))
		ChoreInstance::getInstance(_engine, instanceId)->fadeIn(seconds);
}

void Lua_V2::FadeOutChore() {
	lua_Object idObj = lua_getparam(1);
	lua_Object secondsObj = lua_getparam(2);

	logDemo(kConsoleWarning, "FadeOutChore: this function is obsolete, stop using it.\n");

	if (!lua_isnumber(idObj)) {
		logDemo(kConsoleWarning, "FadeOutChore: the first parameter must be a chore id.\n");

		return;
	}

	if (!lua_isnumber(secondsObj)) {
		logDemo(kConsoleWarning, "FadeOutChore: the second parameter must be a number of seconds.\n");

		return;
	}

	uint instanceId = (uint)luaFtol(lua_getnumber(idObj));
	float seconds = lua_getnumber(secondsObj);

	if (ChoreInstance::isInstanceValid(_engine, instanceId))
		ChoreInstance::getInstance(_engine, instanceId)->fadeOut(seconds);
}

void Lua_V2::StopChore() {
	lua_Object idObj = lua_getparam(1);
	lua_Object secondsObj = lua_getparam(2);
	float seconds = 0.0f;

	if (!lua_isnumber(idObj)) {
		logDemo(kConsoleWarning, "StopChore: the first parameter must be a chore id.\n");

		return;
	}

	if (!lua_isnil(secondsObj) && lua_isnumber(secondsObj))
		seconds = lua_getnumber(secondsObj);

	uint instanceId = (uint)luaFtol(lua_getnumber(idObj));

	if (!(seconds > 0.0f)) {
		if (ChoreInstance::isInstanceValid(_engine, instanceId))
			ChoreInstance::getInstance(_engine, instanceId)->stop();
	} else if (ChoreInstance::isInstanceValid(_engine, instanceId)) {
		ChoreInstance::getInstance(_engine, instanceId)->fadeOut(seconds);
	}
}

void Lua_V2::PauseChore() {
	lua_Object idObj = lua_getparam(1);

	if (!lua_isnumber(idObj)) {
		logDemo(kConsoleWarning, "PauseChore: the first parameter must be a chore id.\n");

		return;
	}

	uint instanceId = (uint)luaFtol(lua_getnumber(idObj));

	if (ChoreInstance::isInstanceValid(_engine, instanceId))
		ChoreInstance::getInstance(_engine, instanceId)->pause();
}

void Lua_V2::AdvanceChore() {
	lua_Object idObj = lua_getparam(1);
	lua_Object secondsObj = lua_getparam(2);

	if (!lua_isnumber(idObj)) {
		logDemo(kConsoleWarning, "AdvanceChore: the first parameter must be a chore id.\n");

		return;
	}

	if (!lua_isnumber(secondsObj)) {
		logDemo(kConsoleWarning, "AdvanceChore: the second parameter must be a number of seconds.\n");

		return;
	}

	uint instanceId = (uint)luaFtol(lua_getnumber(idObj));
	float seconds = lua_getnumber(secondsObj);

	if (ChoreInstance::isInstanceValid(_engine, instanceId))
		ChoreInstance::getInstance(_engine, instanceId)->update(seconds, false);
}

void Lua_V2::CompleteChore() {
	lua_Object idObj = lua_getparam(1);

	if (!lua_isnumber(idObj)) {
		logDemo(kConsoleWarning, "CompleteChore: the first parameter must be a chore id.\n");

		return;
	}

	uint instanceId = (uint)luaFtol(lua_getnumber(idObj));

	if (ChoreInstance::isInstanceValid(_engine, instanceId))
		ChoreInstance::getInstance(_engine, instanceId)->complete();
}

void Lua_V2::LockChore() {
	lua_Object choreObj = lua_getparam(1);
	lua_Object choreSetObj = lua_getparam(2);

	if (!lua_isstring(choreObj)) {
		logDemo(kConsoleWarning, "LockChore: the first parameter must be a chore name.\n");
		lua_pushnil();

		return;
	}

	if (!lua_isstring(choreSetObj)) {
		logDemo(kConsoleWarning, "LockChore: the second parameter must be a costume file name.\n");
		lua_pushnil();

		return;
	}

	LString choreSetName = lua_getstring(choreSetObj);
	LResourceHandle<ChoreSet> choreSetHandle(_engine, choreSetName);

	if (choreSetHandle.lock() && choreSetHandle._typed) {
		LString choreName = lua_getstring(choreObj);
		Chore *chore = choreSetHandle._typed->findChore(choreName);

		if (chore) {
			chore->lockComponents();

			// The demo keeps no list of locked names
			if (!_engine->isDemo()) {
				LockedChore lockedChore;

				lockedChore.choreName = choreName;
				lockedChore.choreSetName = choreSetName;
				_lockedChores.push_back(lockedChore);
			}
		} else {
			logDemo(kConsoleWarning, "LockChore: chore %s not found in costume file %s.\n", choreName.c_str(), choreSetName.c_str());
		}
	} else {
		logDemo(kConsoleWarning, "LockChore: costume file %s not found.\n", choreSetName.c_str());
	}

	choreSetHandle.unlock();
}

void Lua_V2::UnlockChore() {
	lua_Object choreObj = lua_getparam(1);
	lua_Object choreSetObj = lua_getparam(2);

	if (!lua_isstring(choreObj)) {
		logDemo(kConsoleWarning, "UnlockChore: the first parameter must be a chore name.\n");
		lua_pushnil();

		return;
	}

	if (!lua_isstring(choreSetObj)) {
		logDemo(kConsoleWarning, "UnlockChore: the second parameter must be a costume file name.\n");
		lua_pushnil();

		return;
	}

	LString choreSetName = lua_getstring(choreSetObj);
	LResourceHandle<ChoreSet> choreSetHandle(_engine, choreSetName);

	if (choreSetHandle.lock() && choreSetHandle._typed) {
		LString choreName = lua_getstring(choreObj);
		Chore *chore = choreSetHandle._typed->findChore(choreName);

		if (chore) {
			chore->unlockComponents();

			for (uint i = 0; !_engine->isDemo() && i < _lockedChores.size(); i++) {
				if (_lockedChores[i].choreSetName == choreSetName && _lockedChores[i].choreName == choreName) {
					_lockedChores.remove_at(i);

					break;
				}
			}
		} else {
			logDemo(kConsoleWarning, "UnlockChore: chore %s not found in costume file %s.\n", choreName.c_str(), choreSetName.c_str());
		}
	} else {
		logDemo(kConsoleWarning, "UnlockChore: costume file %s not found.\n", choreSetName.c_str());
	}

	choreSetHandle.unlock();
}

void Lua_V2::LockChoreSet() {
	lua_Object choreSetObj = lua_getparam(1);

	if (!lua_isstring(choreSetObj)) {
		logDemo(kConsoleWarning, "LockChoreSet: the parameter must be a costume file name.\n");
		lua_pushnil();

		return;
	}

	LString choreSetName = lua_getstring(choreSetObj);
	LResourceHandle<ChoreSet> choreSetHandle(_engine, choreSetName);

	if (!_engine->isDemo())
		_lockedChoreSets.push_back(choreSetName);

	if (choreSetHandle.lock() && choreSetHandle._typed) {
		for (uint i = 0; i < choreSetHandle._typed->_chores.size(); i++) {
			Chore *chore = choreSetHandle._typed->_chores[i];

			if (chore)
				chore->lockComponents();
		}
	} else {
		logDemo(kConsoleWarning, "LockChoreSet: costume file %s not found.\n", choreSetName.c_str());
	}

	choreSetHandle.unlock();
}

void Lua_V2::UnlockChoreSet() {
	lua_Object choreSetObj = lua_getparam(1);

	if (!lua_isstring(choreSetObj)) {
		logDemo(kConsoleWarning, "UnlockChoreSet: the parameter must be a costume file name.\n");
		lua_pushnil();

		return;
	}

	LString choreSetName = lua_getstring(choreSetObj);
	LResourceHandle<ChoreSet> choreSetHandle(_engine, choreSetName);

	for (uint i = 0; !_engine->isDemo() && i < _lockedChoreSets.size(); i++) {
		if (_lockedChoreSets[i] == choreSetName) {
			_lockedChoreSets.remove_at(i);

			break;
		}
	}

	if (choreSetHandle.lock() && choreSetHandle._typed) {
		for (uint i = 0; i < choreSetHandle._typed->_chores.size(); i++) {
			Chore *chore = choreSetHandle._typed->_chores[i];

			if (chore)
				chore->unlockComponents();
		}
	} else {
		logDemo(kConsoleWarning, "UnlockChoreSet: costume file %s not found.\n", choreSetName.c_str());
	}

	choreSetHandle.unlock();
}

} // end of namespace Grim

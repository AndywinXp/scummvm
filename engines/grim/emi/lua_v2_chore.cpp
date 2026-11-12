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

#include "engines/grim/emi/choreinstance.h"
#include "engines/grim/emi/choreset.h"
#include "engines/grim/emi/lresourcehandle.h"
#include "engines/grim/emi/lua_v2.h"

namespace Grim {

void Lua_V2::IsChoreValid() {
	lua_Object idObj = lua_getparam(1);

	if (!lua_isnumber(idObj))
		return;

	uint instanceId = (uint)luaFtol(lua_getnumber(idObj));

	if (ChoreInstance::isInstanceValid(_engine, instanceId))
		lua_pushnumber(1.0f);
	else
		lua_pushnumber(0.0f);
}

void Lua_V2::IsChorePlaying() {
	lua_Object idObj = lua_getparam(1);

	if (!lua_isnumber(idObj))
		return;

	uint instanceId = (uint)luaFtol(lua_getnumber(idObj));

	if (ChoreInstance::isInstanceValid(_engine, instanceId) && ChoreInstance::getInstance(_engine, instanceId)->isPlaying())
		lua_pushnumber(1.0f);
	else
		lua_pushnil();
}

void Lua_V2::IsChoreLooping() {
	lua_Object idObj = lua_getparam(1);

	if (!lua_isnumber(idObj))
		return;

	uint instanceId = (uint)luaFtol(lua_getnumber(idObj));

	if (ChoreInstance::isInstanceValid(_engine, instanceId) && ChoreInstance::getInstance(_engine, instanceId)->isLooping())
		lua_pushnumber(1.0f);
	else
		lua_pushnil();
}

void Lua_V2::SetChoreLooping() {
	lua_Object idObj = lua_getparam(1);
	lua_Object loopObj = lua_getparam(2);

	if (!lua_isnumber(idObj))
		return;

	uint instanceId = (uint)luaFtol(lua_getnumber(idObj));
	bool looping = lua_isnil(loopObj) == 0;

	if (ChoreInstance::isInstanceValid(_engine, instanceId))
		ChoreInstance::getInstance(_engine, instanceId)->setLooping(looping);
}

void Lua_V2::PlayChore() {
	lua_Object idObj = lua_getparam(1);

	if (!lua_isnumber(idObj))
		return;

	uint instanceId = (uint)luaFtol(lua_getnumber(idObj));

	if (ChoreInstance::isInstanceValid(_engine, instanceId))
		ChoreInstance::getInstance(_engine, instanceId)->setPlaying();
}

void Lua_V2::FadeInChore() {
	lua_Object idObj = lua_getparam(1);
	lua_Object secondsObj = lua_getparam(2);

	if (!lua_isnumber(idObj) || !lua_isnumber(secondsObj))
		return;

	uint instanceId = (uint)luaFtol(lua_getnumber(idObj));
	float seconds = lua_getnumber(secondsObj);

	if (ChoreInstance::isInstanceValid(_engine, instanceId))
		ChoreInstance::getInstance(_engine, instanceId)->fadeIn(seconds);
}

void Lua_V2::FadeOutChore() {
	lua_Object idObj = lua_getparam(1);
	lua_Object secondsObj = lua_getparam(2);

	if (!lua_isnumber(idObj) || !lua_isnumber(secondsObj))
		return;

	uint instanceId = (uint)luaFtol(lua_getnumber(idObj));
	float seconds = lua_getnumber(secondsObj);

	if (ChoreInstance::isInstanceValid(_engine, instanceId))
		ChoreInstance::getInstance(_engine, instanceId)->fadeOut(seconds);
}

void Lua_V2::StopChore() {
	lua_Object idObj = lua_getparam(1);
	lua_Object secondsObj = lua_getparam(2);
	float seconds = 0.0f;

	if (!lua_isnumber(idObj))
		return;

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

	if (!lua_isnumber(idObj))
		return;

	uint instanceId = (uint)luaFtol(lua_getnumber(idObj));

	if (ChoreInstance::isInstanceValid(_engine, instanceId))
		ChoreInstance::getInstance(_engine, instanceId)->pause();
}

void Lua_V2::AdvanceChore() {
	lua_Object idObj = lua_getparam(1);
	lua_Object secondsObj = lua_getparam(2);

	if (!lua_isnumber(idObj) || !lua_isnumber(secondsObj))
		return;

	uint instanceId = (uint)luaFtol(lua_getnumber(idObj));
	float seconds = lua_getnumber(secondsObj);

	if (ChoreInstance::isInstanceValid(_engine, instanceId))
		ChoreInstance::getInstance(_engine, instanceId)->update(seconds, false);
}

void Lua_V2::CompleteChore() {
	lua_Object idObj = lua_getparam(1);

	if (!lua_isnumber(idObj))
		return;

	uint instanceId = (uint)luaFtol(lua_getnumber(idObj));

	if (ChoreInstance::isInstanceValid(_engine, instanceId))
		ChoreInstance::getInstance(_engine, instanceId)->complete();
}

void Lua_V2::LockChore() {
	lua_Object choreObj = lua_getparam(1);
	lua_Object choreSetObj = lua_getparam(2);

	if (!lua_isstring(choreObj) || !lua_isstring(choreSetObj)) {
		lua_pushnil();

		return;
	}

	LString choreSetName = lua_getstring(choreSetObj);
	LResourceHandle<ChoreSet> choreSetHandle(_engine, choreSetName);

	if (choreSetHandle.lock() && choreSetHandle.typed) {
		LString choreName = lua_getstring(choreObj);
		Chore *chore = choreSetHandle.typed->findChore(choreName);

		if (chore) {
			LockedChore lockedChore;

			lockedChore.choreName = choreName;
			lockedChore.choreSetName = choreSetName;
			chore->lockComponents();
			_lockedChores.push_back(lockedChore);
		}
	}

	choreSetHandle.unlock();
}

void Lua_V2::UnlockChore() {
	lua_Object choreObj = lua_getparam(1);
	lua_Object choreSetObj = lua_getparam(2);

	if (!lua_isstring(choreObj) || !lua_isstring(choreSetObj)) {
		lua_pushnil();

		return;
	}

	LString choreSetName = lua_getstring(choreSetObj);
	LResourceHandle<ChoreSet> choreSetHandle(_engine, choreSetName);

	if (choreSetHandle.lock() && choreSetHandle.typed) {
		LString choreName = lua_getstring(choreObj);
		Chore *chore = choreSetHandle.typed->findChore(choreName);

		if (chore) {
			chore->unlockComponents();

			for (uint i = 0; i < _lockedChores.size(); i++) {
				if (_lockedChores[i].choreSetName == choreSetName && _lockedChores[i].choreName == choreName) {
					_lockedChores.remove_at(i);

					break;
				}
			}
		}
	}

	choreSetHandle.unlock();
}

void Lua_V2::LockChoreSet() {
	lua_Object choreSetObj = lua_getparam(1);

	if (!lua_isstring(choreSetObj)) {
		lua_pushnil();

		return;
	}

	LString choreSetName = lua_getstring(choreSetObj);
	LResourceHandle<ChoreSet> choreSetHandle(_engine, choreSetName);

	_lockedChoreSets.push_back(choreSetName);

	if (choreSetHandle.lock() && choreSetHandle.typed) {
		for (uint i = 0; i < choreSetHandle.typed->chores.size(); i++) {
			Chore *chore = choreSetHandle.typed->chores[i];

			if (chore)
				chore->lockComponents();
		}
	}

	choreSetHandle.unlock();
}

void Lua_V2::UnlockChoreSet() {
	lua_Object choreSetObj = lua_getparam(1);

	if (!lua_isstring(choreSetObj)) {
		lua_pushnil();

		return;
	}

	LString choreSetName = lua_getstring(choreSetObj);
	LResourceHandle<ChoreSet> choreSetHandle(_engine, choreSetName);

	for (uint i = 0; i < _lockedChoreSets.size(); i++) {
		if (_lockedChoreSets[i] == choreSetName) {
			_lockedChoreSets.remove_at(i);

			break;
		}
	}

	if (choreSetHandle.lock() && choreSetHandle.typed) {
		for (uint i = 0; i < choreSetHandle.typed->chores.size(); i++) {
			Chore *chore = choreSetHandle.typed->chores[i];

			if (chore)
				chore->unlockComponents();
		}
	}

	choreSetHandle.unlock();
}

} // end of namespace Grim

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

#include "engines/grim/emi/background.h"
#include "engines/grim/emi/choreinstance.h"
#include "engines/grim/emi/choreset.h"
#include "engines/grim/emi/emiengine.h"
#include "engines/grim/emi/lfile.h"
#include "engines/grim/emi/lresourcehandle.h"
#include "engines/grim/emi/lua_v2.h"
#include "engines/grim/emi/resourcemanager.h"
#include "engines/grim/emi/room_v2.h"
#include "engines/grim/emi/sounds.h"

namespace Grim {

void Lua_V2::dumpResources(int mode) {
	LFile *logFile = nullptr;
	lua_Object fileNameObj = lua_getparam(1);

	if (lua_isstring(fileNameObj))
		logFile = gpFileServices->open(lua_getstring(fileNameObj), "w");

	ResourceManager *resourceManager = dynamic_cast<ResourceManager *>(gpResourceManager);

	if (resourceManager) {
		resourceManager->setLogFile(logFile);

		switch (mode) {
		case 0:
			resourceManager->dumpAll(nullptr);

			break;
		case 1:
			resourceManager->dumpLru(nullptr);

			break;
		case 2:
			resourceManager->dumpUsed(nullptr);

			break;
		case 3:
			resourceManager->dumpUnused(nullptr);

			break;
		default:
			break;
		}

		resourceManager->detachLogFile();
	}

	if (logFile)
		gpFileServices->close(logFile, true);
}

void Lua_V2::purgeResources(int mode) {
	ResourceManager *resourceManager = dynamic_cast<ResourceManager *>(gpResourceManager);

	if (!resourceManager)
		return;

	if (mode == 0)
		resourceManager->purgeAll();
	else if (mode == 1)
		resourceManager->purgeUnused();
	else if (mode == 2)
		resourceManager->purgeUnlocked();
}

void Lua_V2::ReportAllResources() {
	dumpResources(0);
}

void Lua_V2::ReportLRUResources() {
	dumpResources(1);
}

void Lua_V2::ReportUsedResources() {
	dumpResources(2);
}

void Lua_V2::ReportUnusedResources() {
	dumpResources(3);
}

void Lua_V2::PurgeAmount() {
	lua_Object amountObj = lua_getparam(1);
	ResourceManager *resourceManager = dynamic_cast<ResourceManager *>(gpResourceManager);

	if (resourceManager && !lua_isnil(amountObj)) {
		if (lua_isnumber(amountObj))
			resourceManager->purgeAmount((uint)luaFtol(lua_getnumber(amountObj)));
	}
}

void Lua_V2::PurgeAllResources() {
	purgeResources(0);
}

void Lua_V2::PurgeUnusedResources() {
	purgeResources(1);
}

void Lua_V2::PurgeUnlockedResources() {
	purgeResources(2);
}

void Lua_V2::NukeAllScriptLocks() {
	for (uint i = 0; i < _lockedSounds.size(); i++)
		Sounds::stopSound(_lockedSounds[i]);

	_lockedSounds.clear();

	for (uint i = 0; i < _lockedChores.size(); i++) {
		LResourceHandle<ChoreSet> choreSetHandle(_lockedChores[i].choreSetName);

		if (choreSetHandle.lock() && choreSetHandle.typed) {
			Chore *chore = choreSetHandle.typed->findChore(_lockedChores[i].choreName);

			if (chore)
				chore->unlockComponents();
		}

		choreSetHandle.unlock();
	}

	_lockedChores.clear();

	for (uint i = 0; i < _lockedChoreSets.size(); i++) {
		LResourceHandle<ChoreSet> choreSetHandle(_lockedChoreSets[i]);

		if (choreSetHandle.lock() && choreSetHandle.typed) {
			for (uint j = 0; j < choreSetHandle.typed->chores.size(); j++) {
				Chore *chore = choreSetHandle.typed->chores[j];

				if (chore)
					chore->unlockComponents();
			}
		}

		choreSetHandle.unlock();
	}

	_lockedChoreSets.clear();

	for (uint i = 0; i < _lockedRooms.size(); i++) {
		LResourceHandle<Room_v2> roomHandle(_lockedRooms[i]);

		if (roomHandle.lock() && roomHandle.typed) {
			Room_v2 *room = roomHandle.typed;

			room->lockCount--;
			gpResourceManager->unlockResource(room);

			if (room->lockCount == 0 && room->purgeOnFree)
				gpResourceManager->purgeResource(room, true);
		}

		roomHandle.unlock();
	}

	_lockedRooms.clear();

	for (uint i = 0; i < _lockedBackgrounds.size(); i++) {
		LResourceHandle<Background> backgroundHandle(_lockedBackgrounds[i]);

		if (backgroundHandle.lock() && backgroundHandle.typed) {
			Background *background = backgroundHandle.typed;

			background->lockCount--;
			gpResourceManager->unlockResource(background);

			if (background->lockCount == 0 && background->purgeOnFree)
				gpResourceManager->purgeResource(background, true);
		}

		backgroundHandle.unlock();
	}

	_lockedBackgrounds.clear();
}

void Lua_V2::FRUTEY_Begin() {
	const char *path = "FRUTEY";
	lua_Object pathObj = lua_getparam(1);

	if (lua_isstring(pathObj))
		path = lua_getstring(pathObj);

	EmiEngine::beginFrameDump(path);
}

void Lua_V2::FRUTEY_End() {
	EmiEngine::endFrameDump();
}

} // end of namespace Grim

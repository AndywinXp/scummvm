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

#include "engines/grim/grim.h"
#include "engines/grim/legacy_savegames/legacy_savegames_import.h"
#include "engines/grim/lua_v1_opcodes/lua_v1.h"
#include "engines/grim/res.h"
#include "engines/grim/sputobjectstate.h"
#include "engines/grim/sputresource.h"
#include "engines/grim/sputroom.h"
#include "engines/grim/text/sputtext.h"
#include "engines/grim/walkplane.h"

namespace Grim {

// Values of the legacy engine: the object state positions are the types of the new engine
static const int kLegacyMaxStatePosition = 3;

// File names of the object states have 32 characters including the terminator
static const uint kMaxStateFileNameSize = 32;

// Volumes of a room that no script has changed
static const int kDefaultMinVolume = 20;
static const int kDefaultMaxVolume = 127;

static int getStateIndex(Room *room, int setupIndex, SputObjectState *state) {
	int index = 0;

	for (SputObjectState *other = room->setups[setupIndex].ObjectStates; other; other = other->next) {
		if (other == state)
			return index;

		index++;
	}

	return -1;
}

static int packTextColor(const LegacyColor &color) {
	return color.r | (color.g << 8) | (color.b << 16);
}

bool LegacyImporter::planRooms() {
	int currentIndex = findSetIndex(_data.grim.currSetId);

	if (currentIndex < 0) {
		warning("LegacyImporter::planRooms(): the current set %d is not in the savegame", _data.grim.currSetId);
		return false;
	}

	if (!archiveHasFile(_data.sets[currentIndex].name)) {
		warning("LegacyImporter::planRooms(): the set file %s of the current room is missing", _data.sets[currentIndex].name.c_str());
		return false;
	}

	// The sets that are neither current nor locked are loaded again by the scripts when they need them
	for (uint i = 0; i < _data.sets.size(); i++) {
		const LegacySet &set = _data.sets[i];

		if ((int)i == currentIndex || !set.locked)
			continue;

		if (!archiveHasFile(set.name)) {
			warning("LegacyImporter::planRooms(): the set file %s is missing, the locked set is skipped", set.name.c_str());
			continue;
		}

		LegacyRoomPlan plan;

		plan.setIndex = i;
		plan.isCurrent = false;
		planStates(set, plan);
		_roomPlans.push_back(plan);
	}

	LegacyRoomPlan currentPlan;

	currentPlan.setIndex = (uint)currentIndex;
	currentPlan.isCurrent = true;
	planStates(_data.sets[currentIndex], currentPlan);
	_roomPlans.push_back(currentPlan);

	return true;
}

void LegacyImporter::planStates(const LegacySet &set, LegacyRoomPlan &plan) {
	// The list of the legacy set starts with the newest state
	for (uint i = set.objectStateIds.size(); i > 0; i--) {
		int32 stateId = set.objectStateIds[i - 1];
		const LegacyObjectState *statePointer = findObjectState(stateId);

		if (!statePointer) {
			warning("LegacyImporter::planStates(): the object state %d of set %s is not in the savegame", stateId, set.name.c_str());
			continue;
		}

		if (statePointer->position < 0 || statePointer->position > kLegacyMaxStatePosition) {
			warning("LegacyImporter::planStates(): the object state %d has the position %d, it is skipped", stateId, statePointer->position);
			continue;
		}

		const LegacyBitmap *bitmap = findBitmap(statePointer->bitmapId);

		if (!bitmap || bitmap->fileName.empty() || bitmap->fileName.size() >= kMaxStateFileNameSize) {
			warning("LegacyImporter::planStates(): the object state %d has no usable bitmap, it is skipped", stateId);
			continue;
		}

		if (!archiveHasFile(bitmap->fileName)) {
			warning("LegacyImporter::planStates(): the bitmap %s of an object state is missing, the state is skipped", bitmap->fileName.c_str());
			continue;
		}

		LegacyStatePlan state;

		state.oldId = stateId;
		state.setupIndex = statePointer->setupId;
		state.type = statePointer->position;
		state.bitmapName = bitmap->fileName;
		state.colorFrame = -1;
		state.zFrame = -1;

		// The legacy bitmaps count their frames from 1 and a hidden state shows none
		if (statePointer->visibility && bitmap->currImage > 0)
			state.colorFrame = bitmap->currImage - 1;

		if (statePointer->zBitmapId) {
			const LegacyBitmap *zBitmap = findBitmap(statePointer->zBitmapId);

			if (!zBitmap || zBitmap->fileName.empty() || zBitmap->fileName.size() >= kMaxStateFileNameSize) {
				warning("LegacyImporter::planStates(): the object state %d has no usable z bitmap, it is skipped", stateId);
				continue;
			}

			if (!archiveHasFile(zBitmap->fileName)) {
				warning("LegacyImporter::planStates(): the z bitmap %s of an object state is missing, the state is skipped", zBitmap->fileName.c_str());
				continue;
			}

			state.zBitmapName = zBitmap->fileName;

			if (statePointer->visibility && zBitmap->currImage > 0)
				state.zFrame = zBitmap->currImage - 1;
		}

		plan.states.push_back(state);
	}
}

void LegacyImporter::importFonts() {
	for (uint i = 0; i < _fontPlan.size(); i++) {
		const LegacyBitmapFont &font = _data.bitmapFonts[_fontPlan[i]];
		void *handle = _grim->_sputResource->alloc(kSputTagFont, font.fileName.c_str());
		ResNode *node = handle ? _grim->_res->checkAllocatedRes(handle) : nullptr;

		if (!node) {
			warning("LegacyImporter::importFonts(): the font %s cannot be loaded", font.fileName.c_str());
			continue;
		}

		_ctx.fontSerials.setVal(font.id, node->serial);
	}
}

void LegacyImporter::importText() {
	const LegacySayLineDefaults &defaults = _data.grim.sayLineDefaults;
	Common::String defaultFont = _defaultFont;
	bool hasSayLineDefaults = false;

	if (defaults.font.kind == kLegacyFontBitmap) {
		const LegacyBitmapFont *font = findBitmapFont(defaults.font.id);

		if (font && _ctx.fontSerials.contains(font->id)) {
			defaultFont = font->fileName;
			hasSayLineDefaults = true;
		}
	}

	SputText *sputText = _grim->_sputText;

	setDefaultFonts(sputText, _printLineFont.c_str(), defaultFont.c_str(), _blastTextFont.c_str());

	if (hasSayLineDefaults)
		setSayLineDefaults(sputText, defaults.x, defaults.y, packTextColor(defaults.fgColor), defaults.justify);
}

bool LegacyImporter::importRooms() {
	SputRoom *sputRoom = _grim->_sputRoom;
	Room *currentRoom = nullptr;

	for (uint i = 0; i < _roomPlans.size(); i++) {
		const LegacyRoomPlan &plan = _roomPlans[i];
		const LegacySet &set = _data.sets[plan.setIndex];
		Room *room = static_cast<Room *>(_grim->_sputResource->alloc(kSputTagSet, set.name.c_str()));

		if (!room) {
			warning("LegacyImporter::importRooms(): the set %s cannot be loaded", set.name.c_str());

			if (plan.isCurrent)
				return false;

			continue;
		}

		int curSetup = (int)set.currSetup;

		if (set.setups.size() != room->numSetups)
			warning("LegacyImporter::importRooms(): the set %s has %u setups in the savegame and %u in the game data", set.name.c_str(), set.setups.size(), room->numSetups);

		if (curSetup >= (int)room->numSetups) {
			warning("LegacyImporter::importRooms(): the setup %d of the set %s does not exist", curSetup, set.name.c_str());
			curSetup = 0;
		}

		beginRoom(sputRoom, room, curSetup, set.locked ? 1 : 0);
		importObjectStates(room, plan);

		if (plan.isCurrent)
			currentRoom = room;
	}

	if (!currentRoom)
		return false;

	const LegacySet &currentSet = _data.sets[_roomPlans.back().setIndex];
	int minVolume = kDefaultMinVolume;
	int maxVolume = kDefaultMaxVolume;

	// The legacy engine keeps 0 and 0 until a script sets the volumes of the set
	if (currentSet.minVolume != 0 || currentSet.maxVolume != 0) {
		minVolume = CLIP(currentSet.minVolume, 0, 255);
		maxVolume = CLIP(currentSet.maxVolume, 0, 255);
	}

	setCurrentRoom(sputRoom, currentRoom, (byte)minVolume, (byte)maxVolume, currentSet.soundMaxDist, currentSet.soundPanFactor, currentSet.soundMaxRadius);
	importWalkPlanes(currentSet);

	return true;
}

void LegacyImporter::importObjectStates(Room *room, const LegacyRoomPlan &plan) {
	SputRoom *sputRoom = _grim->_sputRoom;
	Common::Array<SputObjectState *> added;

	for (uint i = 0; i < plan.states.size(); i++) {
		const LegacyStatePlan &state = plan.states[i];
		SputObjectState *statePointer = nullptr;

		if (state.setupIndex < 0 || (uint)state.setupIndex >= room->numSetups)
			warning("LegacyImporter::importObjectStates(): the object state %d is for the setup %d, the room has %u setups", state.oldId, state.setupIndex, room->numSetups);
		else
			statePointer = addRoomObjectState(sputRoom, state.setupIndex, state.type, state.bitmapName.c_str(), state.zBitmapName.c_str(), state.colorFrame, state.zFrame);

		added.push_back(statePointer);
	}

	// The index in the list is only known when the whole list is there
	for (uint i = 0; i < added.size(); i++) {
		if (!added[i])
			continue;

		LegacyStateRef ref;

		ref.setupIndex = plan.states[i].setupIndex;
		ref.index = getStateIndex(room, ref.setupIndex, added[i]);
		ref.luaHandle = Lua_V1::getObjectStateHandle(added[i]);
		_ctx.objectStates.setVal(plan.states[i].oldId, ref);
	}
}

void LegacyImporter::importWalkPlanes(const LegacySet &set) {
	WalkSystem *walkSystem = _grim->_walkSystem;
	uint numMissing = 0;

	for (uint i = 0; i < set.sectors.size(); i++) {
		const LegacySector &sector = set.sectors[i];
		WalkPlane *plane = walkSystem->planeNumbered(sector.id);

		if (!plane) {
			numMissing++;
			continue;
		}

		if (sector.name != plane->_name)
			warning("LegacyImporter::importWalkPlanes(): the sector %d is %s in the savegame and %s in the game data", sector.id, sector.name.c_str(), plane->_name);

		plane->_active = sector.visible ? 1 : 0;
		plane->setType(sector.type);
		plane->_enabled = 1;
	}

	if (numMissing)
		warning("LegacyImporter::importWalkPlanes(): %u sectors of the set %s are not in the game data", numMissing, set.name.c_str());
}

} // End of namespace Grim

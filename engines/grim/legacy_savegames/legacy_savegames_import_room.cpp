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
#include "engines/grim/lua_v1.h"
#include "engines/grim/res.h"
#include "engines/grim/sputobjectstate.h"
#include "engines/grim/sputresource.h"
#include "engines/grim/sputroom.h"
#include "engines/grim/sputtext.h"
#include "engines/grim/walkplane.h"

namespace Grim {

// Values of the legacy engine: the object state positions are the types of the new engine
static const int kLegacyMaxStatePosition = 3;

// File names of the object states have 32 characters including the terminator
static const uint kMaxStateFileNameSize = 32;

// Volumes of a room that no script has changed
static const int kDefaultMinVolume = 20;
static const int kDefaultMaxVolume = 127;

static int getStateIndex(Room *pRoom, int setupIndex, SputObjectState *pState) {
	int index = 0;

	for (SputObjectState *pOther = pRoom->pSetups[setupIndex].ObjectStates; pOther; pOther = pOther->pNext) {
		if (pOther == pState)
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
		const LegacyObjectState *pState = findObjectState(stateId);

		if (!pState) {
			warning("LegacyImporter::planStates(): the object state %d of set %s is not in the savegame", stateId, set.name.c_str());
			continue;
		}

		if (pState->position < 0 || pState->position > kLegacyMaxStatePosition) {
			warning("LegacyImporter::planStates(): the object state %d has the position %d, it is skipped", stateId, pState->position);
			continue;
		}

		const LegacyBitmap *pBitmap = findBitmap(pState->bitmapId);

		if (!pBitmap || pBitmap->fileName.empty() || pBitmap->fileName.size() >= kMaxStateFileNameSize) {
			warning("LegacyImporter::planStates(): the object state %d has no usable bitmap, it is skipped", stateId);
			continue;
		}

		if (!archiveHasFile(pBitmap->fileName)) {
			warning("LegacyImporter::planStates(): the bitmap %s of an object state is missing, the state is skipped", pBitmap->fileName.c_str());
			continue;
		}

		LegacyStatePlan state;

		state.oldId = stateId;
		state.setupIndex = pState->setupId;
		state.type = pState->position;
		state.bitmapName = pBitmap->fileName;
		state.colorFrame = -1;
		state.zFrame = -1;

		// The legacy bitmaps count their frames from 1 and a hidden state shows none
		if (pState->visibility && pBitmap->currImage > 0)
			state.colorFrame = pBitmap->currImage - 1;

		if (pState->zBitmapId) {
			const LegacyBitmap *pZBitmap = findBitmap(pState->zBitmapId);

			if (!pZBitmap || pZBitmap->fileName.empty() || pZBitmap->fileName.size() >= kMaxStateFileNameSize) {
				warning("LegacyImporter::planStates(): the object state %d has no usable z bitmap, it is skipped", stateId);
				continue;
			}

			if (!archiveHasFile(pZBitmap->fileName)) {
				warning("LegacyImporter::planStates(): the z bitmap %s of an object state is missing, the state is skipped", pZBitmap->fileName.c_str());
				continue;
			}

			state.zBitmapName = pZBitmap->fileName;

			if (pState->visibility && pZBitmap->currImage > 0)
				state.zFrame = pZBitmap->currImage - 1;
		}

		plan.states.push_back(state);
	}
}

void LegacyImporter::importFonts() {
	for (uint i = 0; i < _fontPlan.size(); i++) {
		const LegacyBitmapFont &font = _data.bitmapFonts[_fontPlan[i]];
		void *handle = _pGrim->_pSputResource->alloc(kSputTagFont, font.fileName.c_str());
		ResNode *pNode = handle ? _pGrim->_pRes->checkAllocatedRes(handle) : nullptr;

		if (!pNode) {
			warning("LegacyImporter::importFonts(): the font %s cannot be loaded", font.fileName.c_str());
			continue;
		}

		_ctx.fontSerials.setVal(font.id, pNode->serial);
	}
}

void LegacyImporter::importText() {
	const LegacySayLineDefaults &defaults = _data.grim.sayLineDefaults;
	Common::String defaultFont = _defaultFont;
	bool hasSayLineDefaults = false;

	if (defaults.font.kind == kLegacyFontBitmap) {
		const LegacyBitmapFont *pFont = findBitmapFont(defaults.font.id);

		if (pFont && _ctx.fontSerials.contains(pFont->id)) {
			defaultFont = pFont->fileName;
			hasSayLineDefaults = true;
		}
	}

	SputText *pSputText = _pGrim->_pSputText;

	setDefaultFonts(pSputText, _printLineFont.c_str(), defaultFont.c_str(), _blastTextFont.c_str());

	if (hasSayLineDefaults)
		setSayLineDefaults(pSputText, defaults.x, defaults.y, packTextColor(defaults.fgColor), defaults.justify);
}

bool LegacyImporter::importRooms() {
	SputRoom *pSputRoom = _pGrim->_pSputRoom;
	Room *pCurrentRoom = nullptr;

	for (uint i = 0; i < _roomPlans.size(); i++) {
		const LegacyRoomPlan &plan = _roomPlans[i];
		const LegacySet &set = _data.sets[plan.setIndex];
		Room *pRoom = static_cast<Room *>(_pGrim->_pSputResource->alloc(kSputTagSet, set.name.c_str()));

		if (!pRoom) {
			warning("LegacyImporter::importRooms(): the set %s cannot be loaded", set.name.c_str());

			if (plan.isCurrent)
				return false;

			continue;
		}

		int curSetup = (int)set.currSetup;

		if (set.setups.size() != pRoom->numSetups)
			warning("LegacyImporter::importRooms(): the set %s has %u setups in the savegame and %u in the game data", set.name.c_str(), set.setups.size(), pRoom->numSetups);

		if (curSetup >= (int)pRoom->numSetups) {
			warning("LegacyImporter::importRooms(): the setup %d of the set %s does not exist", curSetup, set.name.c_str());
			curSetup = 0;
		}

		beginRoom(pSputRoom, pRoom, curSetup, set.locked ? 1 : 0);
		importObjectStates(pRoom, plan);

		if (plan.isCurrent)
			pCurrentRoom = pRoom;
	}

	if (!pCurrentRoom)
		return false;

	const LegacySet &currentSet = _data.sets[_roomPlans.back().setIndex];
	int minVolume = kDefaultMinVolume;
	int maxVolume = kDefaultMaxVolume;

	// The legacy engine keeps 0 and 0 until a script sets the volumes of the set
	if (currentSet.minVolume != 0 || currentSet.maxVolume != 0) {
		minVolume = CLIP(currentSet.minVolume, 0, 255);
		maxVolume = CLIP(currentSet.maxVolume, 0, 255);
	}

	setCurrentRoom(pSputRoom, pCurrentRoom, (byte)minVolume, (byte)maxVolume, currentSet.soundMaxDist, currentSet.soundPanFactor, currentSet.soundMaxRadius);
	importWalkPlanes(currentSet);

	return true;
}

void LegacyImporter::importObjectStates(Room *pRoom, const LegacyRoomPlan &plan) {
	SputRoom *pSputRoom = _pGrim->_pSputRoom;
	Common::Array<SputObjectState *> added;

	for (uint i = 0; i < plan.states.size(); i++) {
		const LegacyStatePlan &state = plan.states[i];
		SputObjectState *pState = nullptr;

		if (state.setupIndex < 0 || (uint)state.setupIndex >= pRoom->numSetups)
			warning("LegacyImporter::importObjectStates(): the object state %d is for the setup %d, the room has %u setups", state.oldId, state.setupIndex, pRoom->numSetups);
		else
			pState = addRoomObjectState(pSputRoom, state.setupIndex, state.type, state.bitmapName.c_str(), state.zBitmapName.c_str(), state.colorFrame, state.zFrame);

		added.push_back(pState);
	}

	// The index in the list is only known when the whole list is there
	for (uint i = 0; i < added.size(); i++) {
		if (!added[i])
			continue;

		LegacyStateRef ref;

		ref.setupIndex = plan.states[i].setupIndex;
		ref.index = getStateIndex(pRoom, ref.setupIndex, added[i]);
		ref.luaHandle = Lua_V1::getObjectStateHandle(added[i]);
		_ctx.objectStates.setVal(plan.states[i].oldId, ref);
	}
}

void LegacyImporter::importWalkPlanes(const LegacySet &set) {
	WalkSystem *pWalkSystem = _pGrim->_pWalkSystem;
	uint numMissing = 0;

	for (uint i = 0; i < set.sectors.size(); i++) {
		const LegacySector &sector = set.sectors[i];
		WalkPlane *pPlane = pWalkSystem->planeNumbered(sector.id);

		if (!pPlane) {
			numMissing++;
			continue;
		}

		if (sector.name != pPlane->name)
			warning("LegacyImporter::importWalkPlanes(): the sector %d is %s in the savegame and %s in the game data", sector.id, sector.name.c_str(), pPlane->name);

		pPlane->bActive = sector.visible ? 1 : 0;
		pPlane->setType(sector.type);
		pPlane->bEnabled = 1;
	}

	if (numMissing)
		warning("LegacyImporter::importWalkPlanes(): %u sectors of the set %s are not in the game data", numMissing, set.name.c_str());
}

} // End of namespace Grim

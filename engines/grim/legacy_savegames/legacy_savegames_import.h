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

#ifndef GRIM_LEGACY_SAVEGAMES_IMPORT_H
#define GRIM_LEGACY_SAVEGAMES_IMPORT_H

#include "common/array.h"
#include "common/hash-str.h"
#include "common/hashmap.h"
#include "common/scummsys.h"
#include "common/str.h"
#include "common/stream.h"

#include "engines/grim/legacy_savegames/legacy_savegames.h"
#include "engines/grim/legacy_savegames/legacy_savegames_lua.h"

namespace Grim {

class Cos;
class GrimEngine;
class SputActorModule;
class SputRender;
class SputRoom;
class SputSmush;
class SputText;
struct ModelInst;
struct RdKeyframe;
struct Room;
struct SputObjectState;

// Where an object state of the legacy savegame lives in the running game
struct LegacyStateRef {
	int setupIndex;
	int index;
	int luaHandle;
};

// The old pool ids and what they became, used by the module imports and by the Lua userdata
struct LegacyImportContext {
	LegacyImportContext() : nextDeadId(-1) {}

	// Gives an object that is gone an id that no live object has, the same id for the same old one
	int32 getDeadId(Common::HashMap<int32, int32> &deadIds, int32 oldId);

	// Old BitmapFont id -> Res serial of the FONT
	Common::HashMap<int32, int32> fontSerials;
	// Old ObjectState id -> position in its room and Lua handle, only for the states that were imported
	Common::HashMap<int32, LegacyStateRef> objectStates;
	// Old Actor id -> index in the actor array, filled by importActors()
	Common::HashMap<int32, int32> actorIndices;
	// Lower case costume file name -> Res serial of the COST, which is the Cos handle, filled by importActors()
	Common::HashMap<Common::String, int32> costumeSerials;

	// Userdata whose object is gone, one map for every old tag
	Common::HashMap<int32, int32> deadActors;
	Common::HashMap<int32, int32> deadFonts;
	Common::HashMap<int32, int32> deadTtfFonts;
	Common::HashMap<int32, int32> deadTextObjects;
	Common::HashMap<int32, int32> deadObjectStates;
	Common::HashMap<int32, int32> deadBitmaps;
	int32 nextDeadId;
};

// An object state to add to a room
struct LegacyStatePlan {
	int32 oldId;
	int setupIndex;
	int type;
	Common::String bitmapName;
	Common::String zBitmapName;
	int colorFrame;
	int zFrame;
};

// A room to load, the current one is always the last
struct LegacyRoomPlan {
	uint setIndex;
	bool isCurrent;
	// Oldest state first, because every new state goes to the head of its list
	Common::Array<LegacyStatePlan> states;
};

// Loads a savegame of the old ScummVM engine into the running game: prepare() reads and checks it
// without touching the game, apply() replaces the game after GrimEngine::restorePrologue()
class LegacyImporter : public LegacyCostumeProvider, public LegacyLuaMapper {
public:
	explicit LegacyImporter(GrimEngine *grim);
	~LegacyImporter();

	bool prepare(Common::SeekableReadStream *stream);
	bool apply();

	Common::SeekableReadStream *openCostumeFile(const Common::String &name) override;
	bool mapUserdata(int32 oldTag, int32 oldId, int32 *newTag, int32 *newId) override;

	// Looks for the file the way GrimEngine::openFile() does
	static bool archiveHasFile(const Common::String &name);

	const LegacySaveData &getData() const { return _data; }
	LegacyImportContext &getContext() { return _ctx; }
	const LegacyBitmap *findBitmap(int32 id) const;
	const LegacyBitmapFont *findBitmapFont(int32 id) const;

	// Puts the actors, their costumes and their set membership in place and fills the actor and costume maps of the context
	bool importActors();

	// The next two are for the actor import, they touch what the modules keep private
	static int setChoreInstance(Cos *cos, int inst, int choreIndex, uint32 state, int time);
	static int addPuppetTrack(SputActorModule *actors, ModelInst *modelInst, RdKeyframe *keyframe, int priority, int priorityMasked, uint status, float frame, float fade, float fadeSpeed);

private:
	const LegacyObjectState *findObjectState(int32 id) const;
	int findSetIndex(int32 id) const;
	bool planRooms();
	void planStates(const LegacySet &set, LegacyRoomPlan &plan);
	void planFonts();
	void checkPools();

	void importFonts();
	void importText();
	bool importRooms();
	void importObjectStates(Room *room, const LegacyRoomPlan &plan);
	void importWalkPlanes(const LegacySet &set);
	int importCameraActor();
	void importSelectedActor();
	void importShadowColor();
	void importRender(int camActorIndex);
	void importPrimitives();
	void importMovie();
	bool importLua();
	void importSounds();
	void applyDeveloperMode();
	void repeatMusicPause();
	// Renumbers the key codes of the legacy Lua state to the ones of the control table
	void remapControls();

	// Bodies that touch what the modules keep private (all of them are friends of this class)
	static void resetActors(SputActorModule *actors);
	static void beginRoom(SputRoom *sputRoom, Room *room, int curSetup, int roomRef);
	static SputObjectState *addRoomObjectState(SputRoom *sputRoom, int setupIndex, int objectType, const char *bitmapName, const char *zbufferName, int colorFrame, int zFrame);
	static void setCurrentRoom(SputRoom *sputRoom, Room *room, byte minVol, byte maxVol, float maxDist, float panFactor, float maxRadius);
	static void getDefaultFontNames(SputText *sputText, Common::String *printLine, Common::String *defaultValue, Common::String *blastText);
	static void setDefaultFonts(SputText *sputText, const char *printLineFont, const char *defaultFont, const char *blastTextFont);
	static void setSayLineDefaults(SputText *sputText, int x, int y, int fgColor, int justify);
	static void setRenderState(SputRender *sputRender, int irisMode, int irisCenterX, int irisCenterY, int irisDuration, int irisElapsed, int camActorIndex);
	static void startMovie(SputSmush *sputSmush, const char *name, int looping, int x, int y);
	void getEngineRefs(LegacyLuaRefs *refs);

	GrimEngine *_grim;
	LegacySaveData _data;
	LegacyImportContext _ctx;
	Common::SeekableReadStream *_luaStream;
	Common::Array<LegacyRoomPlan> _roomPlans;
	Common::Array<uint> _fontPlan;
	Common::String _printLineFont;
	Common::String _defaultFont;
	Common::String _blastTextFont;
	bool _canPlayMovie;
	int _prevMode;
};

} // End of namespace Grim

#endif

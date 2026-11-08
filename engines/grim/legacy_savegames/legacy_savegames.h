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

#ifndef GRIM_LEGACY_SAVEGAMES_H
#define GRIM_LEGACY_SAVEGAMES_H

#include "common/array.h"
#include "common/scummsys.h"
#include "common/stream.h"
#include "common/str.h"

namespace Grim {

static const uint32 kLegacySaveMajorVersion = 22;
static const uint32 kLegacySaveMaxMinorVersion = 30;

struct LegacyVector3 {
	float x, y, z;
};

struct LegacyColor {
	byte r, g, b;
};

struct LegacyPoint {
	int16 x, y;
};

enum LegacyFontKind {
	kLegacyFontNone,
	kLegacyFontBitmap,
	kLegacyFontTtf
};

struct LegacyFontRef {
	LegacyFontKind kind;
	int32 id;
};

enum LegacyLightType {
	kLegacyLightUnset = 0,
	kLegacyLightOmni = 1,
	kLegacyLightSpot = 2,
	kLegacyLightDirect = 3,
	kLegacyLightAmbient = 4
};

struct LegacyBitmap {
	int32 id;
	Common::String fileName;
	int32 currImage;
};

struct LegacyBitmapFont {
	int32 id;
	Common::String fileName;
};

struct LegacyTtfFont {
	int32 id;
	Common::String fileName;
	int32 size;
};

struct LegacyObjectState {
	int32 id;
	bool visibility;
	int32 setupId;
	int32 position;
	int32 bitmapId;
	int32 zBitmapId;
};

struct LegacySetup {
	Common::String name;
	int32 bkgndBitmapId;
	int32 bkgndZBitmapId;
	LegacyVector3 pos;
	LegacyVector3 interest;
	float roll;
	float fov;
	float nclip;
	float fclip;
};

struct LegacySector {
	int32 numVertices;
	int32 id;
	int32 type;
	bool visible;
	float height;
	Common::String name;
	Common::Array<LegacyVector3> vertices;
	LegacyVector3 normal;
	float shrinkRadius;
	bool invalid;
	Common::Array<LegacyVector3> origVertices;
};

struct LegacyLight {
	Common::String name;
	bool enabled;
	int32 type;
	LegacyVector3 pos;
	LegacyVector3 dir;
	LegacyColor color;
	float intensity;
	float umbraAngle;
	float penumbraAngle;
	float falloffNear;
	float falloffFar;
};

struct LegacySetShadow {
	Common::String name;
	LegacyVector3 shadowPoint;
	int32 numSectors;
	Common::Array<Common::String> sectorNames;
	LegacyColor color;
};

struct LegacySet {
	int32 id;
	Common::String name;
	Common::Array<Common::String> colormapNames;
	uint32 currSetup;
	bool locked;
	bool enableLights;
	int32 minVolume;
	int32 maxVolume;
	float soundMaxDist;
	float soundPanFactor;
	float soundMaxRadius;
	Common::Array<int32> objectStateIds;
	Common::Array<LegacySetup> setups;
	Common::Array<LegacySector> sectors;
	Common::Array<LegacyLight> lights;
	Common::Array<LegacySetShadow> shadows;
};

struct LegacyTextObject {
	int32 id;
	LegacyColor fgColor;
	int32 x, y;
	int32 width, height;
	int32 justify;
	int32 numberLines;
	int32 duration;
	bool blastDraw;
	bool isSpeech;
	int32 elapsedTime;
	LegacyFontRef font;
	Common::String textId;
};

struct LegacyPrimitive {
	int32 id;
	int32 type;
	LegacyColor color;
	uint32 filled;
	LegacyPoint points[4];
};

struct LegacyActionChore {
	bool hasCostume;
	Common::String costumeFileName;
	int32 chore;
};

struct LegacyShadowPlane {
	Common::String setName;
	Common::String sectorName;
};

struct LegacyActorShadow {
	Common::String name;
	LegacyVector3 pos;
	Common::Array<LegacyShadowPlane> planes;
	bool active;
	bool dontNegate;
};

struct LegacyChore {
	bool hasPlayed;
	bool playing;
	bool looping;
	int32 currTime;
	bool paused;
};

struct LegacyAnimState {
	bool active;
	int32 repeatMode;
	int32 time;
	int32 fadeMode;
	float fade;
	int32 fadeLength;
	bool paused;
};

// Only the members that belong to the component tag are meaningful
struct LegacyComponent {
	int32 index;
	uint32 tag;
	bool visible;
	bool meshVisible;
	bool hasMatrixPosition;
	LegacyVector3 matrixPosition;
	LegacyAnimState anim;
	int32 activeTexture;
	bool spriteVisible;
};

struct LegacyHeadJoint {
	float pitch, yaw, roll;
};

struct LegacyHead {
	int32 joint1Node, joint2Node, joint3Node;
	float maxPitch, maxYaw, maxRoll;
	bool hasJointAngles;
	LegacyHeadJoint joints[3];
};

struct LegacyCostume {
	Common::String fileName;
	Common::Array<Common::String> prevFileNames;
	bool hasColormap;
	Common::String colormapName;
	Common::Array<LegacyChore> chores;
	Common::Array<LegacyComponent> components;
	Common::Array<int32> playingChoreIds;
	float lookAtRate;
	LegacyHead head;
};

struct LegacyActor {
	int32 id;
	Common::String name;
	Common::String setName;
	LegacyColor talkColor;
	LegacyVector3 pos;
	float pitch, yaw, roll;
	float walkRate;
	float turnRate;
	float turnRateMultiplier;
	bool followBoxes;
	float reflectionAngle;
	bool visible;
	bool lookingMode;
	float scale;
	float timeScale;
	bool puckOrient;
	Common::String talkSoundName;
	bool talking;
	bool backgroundTalk;
	int32 talkSoundId;
	uint32 collisionMode;
	float collisionScale;
	bool hasLipSync;
	Common::String lipSyncFileName;
	Common::Array<LegacyCostume> costumes;
	bool turning;
	bool singleTurning;
	float moveYaw, movePitch, moveRoll;
	bool walking;
	LegacyVector3 destPos;
	LegacyActionChore restChore;
	LegacyActionChore walkChore;
	bool walkedLast;
	bool walkedCur;
	LegacyActionChore leftTurnChore;
	LegacyActionChore rightTurnChore;
	int32 lastTurnDir;
	int32 currTurnDir;
	LegacyActionChore talkChores[10];
	int32 talkAnim;
	LegacyActionChore mumbleChore;
	Common::Array<LegacyActorShadow> shadows;
	int32 activeShadowSlot;
	int32 sayLineText;
	LegacyVector3 lookAtVector;
	Common::Array<LegacyVector3> path;
	bool drawnToClean;
	bool fakeUnbound;
};

struct LegacySayLineDefaults {
	LegacyColor fgColor;
	LegacyFontRef font;
	int32 height;
	int32 justify;
	int32 width;
	int32 x, y;
	int32 duration;
};

struct LegacyGrimState {
	uint32 mode;
	uint32 previousMode;
	int32 selectedActorId;
	LegacySayLineDefaults sayLineDefaults;
	int32 movieSubtitleTextObjectId;
	int32 currSetId;
	bool hasMovieSetup;
	Common::String movieSetup;
};

struct LegacyDriverState {
	LegacyColor shadowColor;
	bool renderBitmaps;
	bool renderZBitmaps;
};

struct LegacyIrisState {
	bool playing;
	uint32 direction;
	int32 x1, y1, x2, y2;
	int32 length;
	int32 currTime;
};

struct LegacyMovieState {
	Common::String fileName;
	int32 frame;
	float movieTime;
	bool videoFinished;
	bool videoLooping;
	int32 x, y;
};

// Everything the legacy format holds except the Lua state
struct LegacySaveData {
	uint32 minorVersion;
	Common::Array<byte> thumbnail;
	Common::Array<Common::String> saveStrings;
	Common::Array<LegacyBitmap> bitmaps;
	Common::Array<LegacyBitmapFont> bitmapFonts;
	Common::Array<LegacyTtfFont> ttfFonts;
	Common::Array<LegacyObjectState> objectStates;
	Common::Array<LegacySet> sets;
	Common::Array<LegacyTextObject> textObjects;
	Common::Array<LegacyPrimitive> primitives;
	bool isTalkingBackground;
	Common::Array<LegacyActor> actors;
	LegacyGrimState grim;
	LegacyDriverState driver;
	Common::Array<byte> soundData;
	LegacyMovieState movie;
	LegacyIrisState iris;
};

struct LegacyCosChore {
	int32 id;
	int32 length;
	int32 tracks;
	Common::String name;
};

struct LegacyCosComponent {
	// Lower case, without trailing blanks
	Common::String name;
	int32 parent;
};

// What a .cos file says about the size of a costume record
struct LegacyCostumeLayout {
	Common::Array<uint32> componentTags;
	// Same indexes as componentTags
	Common::Array<LegacyCosComponent> components;
	Common::Array<LegacyCosChore> chores;
};

class LegacyCostumeProvider {
public:
	virtual ~LegacyCostumeProvider() {}

	// Name as stored in the savegame, the caller returns the stream (or nullptr)
	virtual Common::SeekableReadStream *openCostumeFile(const Common::String &name) = 0;
};

bool parseLegacyCostumeFile(Common::SeekableReadStream *stream, const Common::String &name, LegacyCostumeLayout &layout);

// Bounds-checked little endian reader over one section payload, owns the stream
class LegacyReader {
public:
	LegacyReader(Common::SeekableReadStream *stream, uint32 minorVersion, const Common::String &context);
	~LegacyReader();

	uint32 minorVersion() const { return _minor; }
	bool failed() const { return _failed; }
	uint32 remaining() const;

	void fail(const char *reason);
	bool finish();

	byte readByte();
	bool readBool();
	uint16 readUint16();
	int32 readSint32();
	uint32 readUint32();
	float readFloat();
	Common::String readString();
	LegacyVector3 readVector3();
	LegacyColor readColor();
	LegacyFontRef readFontRef();
	void readBytes(void *buffer, uint32 size);
	void skip(uint32 size);

	// Reads a count and checks the following items can fit in the payload
	int32 readSignedCount(uint32 minItemSize, bool negativeIsEmpty);
	uint32 readUnsignedCount(uint32 minItemSize);
	bool checkCount(int64 count, uint32 minItemSize);

private:
	LegacyReader(const LegacyReader &);
	LegacyReader &operator=(const LegacyReader &);

	bool canRead(uint32 size);

	Common::SeekableReadStream *_stream;
	uint32 _minor;
	Common::String _context;
	bool _failed;
};

struct LegacySectionInfo {
	uint32 tag;
	uint32 offset;
	uint32 size;
};

class LegacySaveFile {
public:
	LegacySaveFile(Common::SeekableReadStream *stream, DisposeAfterUse::Flag dispose);
	~LegacySaveFile();

	// Peeks at the magic without consuming anything
	static bool isLegacySave(Common::SeekableReadStream *stream);
	static Common::String tagToString(uint32 tag);

	bool open();

	uint32 getMajor() const { return _major; }
	uint32 getMinor() const { return _minor; }
	uint32 getNumSections() const { return _sections.size(); }
	const LegacySectionInfo &getSectionInfo(uint32 index) const { return _sections[index]; }
	bool hasSection(uint32 tag) const;
	uint32 getSectionSize(uint32 tag) const;

	// The caller deletes the stream, nullptr if the section does not exist
	Common::SeekableReadStream *openSection(uint32 tag);
	Common::SeekableReadStream *openLuaSection();

	bool readThumbnail(Common::Array<byte> &pixels);
	bool readSaveStrings(Common::Array<Common::String> &strings);
	bool readBitmaps(Common::Array<LegacyBitmap> &bitmaps);
	bool readBitmapFonts(Common::Array<LegacyBitmapFont> &fonts);
	bool readTtfFonts(Common::Array<LegacyTtfFont> &fonts);
	bool readObjectStates(Common::Array<LegacyObjectState> &states);
	bool readSets(Common::Array<LegacySet> &sets);
	bool readTextObjects(Common::Array<LegacyTextObject> &texts);
	bool readPrimitives(Common::Array<LegacyPrimitive> &primitives);
	bool readActors(LegacyCostumeProvider *costumes, bool &isTalkingBackground, Common::Array<LegacyActor> &actors);
	bool readGrimState(LegacyGrimState &grim);
	bool readDriverState(LegacyDriverState &driver);
	bool readSoundData(Common::Array<byte> &data);
	bool readMovieState(LegacyMovieState &movie);
	bool readIrisState(LegacyIrisState &iris);

	// Everything but the Lua section; SIMG and SUBS may be missing
	bool readAll(LegacyCostumeProvider *costumes, LegacySaveData &data);

private:
	LegacySaveFile(const LegacySaveFile &);
	LegacySaveFile &operator=(const LegacySaveFile &);

	const LegacySectionInfo *findSection(uint32 tag) const;

	Common::SeekableReadStream *_stream;
	DisposeAfterUse::Flag _dispose;
	uint32 _major;
	uint32 _minor;
	Common::Array<LegacySectionInfo> _sections;
};

// One item per line, meant to be compared with the output of an independent parser
Common::String describeLegacySave(const LegacySaveFile &file, const LegacySaveData &data);

// For the save/load menu: the stream is the decompressed legacy savegame, it is not deleted
bool legacyReadSaveStrings(Common::SeekableReadStream *stream, Common::Array<Common::String> &strings);
bool legacyReadThumbnail(Common::SeekableReadStream *stream, Common::Array<byte> &pixels);

} // End of namespace Grim

#endif
